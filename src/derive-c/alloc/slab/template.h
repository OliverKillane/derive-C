/// @brief A slab allocator for fixed-size allocations with freelist tracking.

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined BLOCK_SIZE
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("No BLOCK_SIZE")
    #endif
    #define BLOCK_SIZE 64
#endif

#if !defined SLAB_SIZE
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("No SLAB_SIZE")
    #endif
    #define SLAB_SIZE 4096
#endif

DC_STATIC_ASSERT(BLOCK_SIZE > 0, "Block size must be larger than zero");
DC_STATIC_ASSERT(SLAB_SIZE >= BLOCK_SIZE, "Slab size must be at least block size");
DC_STATIC_ASSERT(BLOCK_SIZE >= sizeof(void*),
                 "Block size must be at least pointer size for freelist");

#define SLAB_VECTOR NS(NAME, slab_vector)

#pragma push_macro("ALLOC")

typedef struct {
    void* ptr;
} NS(NAME, slab_info);

static NS(NAME, slab_info) NS(NAME, slab_info_clone)(NS(NAME, slab_info) const* self) {
    return *self;
}
static void NS(NAME, slab_info_delete)(NS(NAME, slab_info) * /* self */) {}
static void NS(NAME, slab_info_debug)(NS(NAME, slab_info) const* /* self */, dc_debug_fmt /* fmt */,
                                      FILE* /* stream */) {}

#define ITEM NS(NAME, slab_info)               // [DERIVE-C] for template
#define ITEM_CLONE NS(NAME, slab_info_clone)   // [DERIVE-C] for template
#define ITEM_DELETE NS(NAME, slab_info_delete) // [DERIVE-C] for template
#define ITEM_DEBUG NS(NAME, slab_info_debug)   // [DERIVE-C] for template
#define INTERNAL_NAME SLAB_VECTOR              // [DERIVE-C] for template
#include <derive-c/container/vector/dynamic/template.h>

#pragma pop_macro("ALLOC")

typedef struct {
    SLAB_VECTOR slabs;
    void* free_list_head;
    NS(ALLOC, ref) alloc_ref;
    dc_gdb_marker derive_c_slaballoc;
} SELF;

DC_PUBLIC static SELF NS(SELF, new)(NS(ALLOC, ref) alloc_ref) {
    return (SELF){
        .slabs = NS(SLAB_VECTOR, new)(alloc_ref),
        .free_list_head = NULL,
        .alloc_ref = alloc_ref,
        .derive_c_slaballoc = {},
    };
}

DC_INTERNAL static void PRIV(NS(SELF, allocate_new_slab))(SELF* self) {
    DC_ASSUME(self);

    size_t blocks_per_slab = SLAB_SIZE / BLOCK_SIZE;
    void* slab_ptr = NS(ALLOC, allocate_uninit)(self->alloc_ref, SLAB_SIZE);

    NS(NAME, slab_info) info = {.ptr = slab_ptr};
    NS(SLAB_VECTOR, push)(&self->slabs, info);

    for (size_t i = 0; i < blocks_per_slab; i++) {
        void* block_ptr = (char*)slab_ptr + (i * BLOCK_SIZE);
        // Temporarily unpoison to write the freelist pointer
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                              block_ptr, sizeof(void*));
        *(void**)block_ptr = self->free_list_head;
        self->free_list_head = block_ptr;
        // Mark the entire block as inaccessible (NONE) - including the freelist pointer
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, block_ptr,
                              BLOCK_SIZE);
    }
}

DC_PUBLIC static void* NS(SELF, allocate_uninit)(SELF* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0, "Cannot allocate zero sized");

    if (size > BLOCK_SIZE) {
        return NS(ALLOC, allocate_uninit)(self->alloc_ref, size);
    }

    if (self->free_list_head == NULL) {
        PRIV(NS(SELF, allocate_new_slab))(self);
    }

    void* block = self->free_list_head;

    // Temporarily mark as READ_WRITE to read the freelist pointer
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE, block,
                          sizeof(void*));
    void* next = *(void**)block;
    self->free_list_head = next;

    // Mark the allocated region as writable but uninitialized (WRITE)
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, block, size);
    // Mark any remaining space in the block as inaccessible
    if (size < BLOCK_SIZE) {
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE,
                              (char*)block + size, BLOCK_SIZE - size);
    }

    return block;
}

DC_PUBLIC static void* NS(SELF, allocate_zeroed)(SELF* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0, "Cannot allocate zero sized");

    void* ptr = NS(SELF, allocate_uninit)(self, size);
    memset(ptr, 0, size);
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE, ptr, size);
    return ptr;
}

DC_PUBLIC static void NS(SELF, deallocate)(SELF* self, void* ptr, size_t size) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);
    DC_ASSERT(size > 0, "Cannot deallocate zero sized");

    if (size > BLOCK_SIZE) {
        NS(ALLOC, deallocate)(self->alloc_ref, ptr, size);
        return;
    }

    // Temporarily unpoison to write the freelist pointer
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE, ptr,
                          sizeof(void*));
    *(void**)ptr = self->free_list_head;
    self->free_list_head = ptr;

    // Mark the entire block as inaccessible (NONE) - including the freelist pointer
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, ptr, BLOCK_SIZE);
}

DC_PUBLIC static void* NS(SELF, reallocate)(SELF* self, void* ptr, size_t old_size,
                                            size_t new_size) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);
    DC_ASSERT(old_size > 0, "Cannot reallocate zero sized");
    DC_ASSERT(new_size > 0, "Cannot allocate zero sized");

    if (old_size > BLOCK_SIZE && new_size > BLOCK_SIZE) {
        return NS(ALLOC, reallocate)(self->alloc_ref, ptr, old_size, new_size);
    }

    void* new_ptr = NS(SELF, allocate_uninit)(self, new_size);

    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);

    if (new_size <= old_size) {
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                              new_ptr, new_size);
    } else {
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                              new_ptr, copy_size);
    }

    NS(SELF, deallocate)(self, ptr, old_size);

    return new_ptr;
}

DC_PUBLIC static void NS(SELF, reset)(SELF* self) {
    DC_ASSUME(self);

    size_t num_slabs = NS(SLAB_VECTOR, size)(&self->slabs);
    for (size_t i = 0; i < num_slabs; i++) {
        NS(NAME, slab_info) const* info = NS(SLAB_VECTOR, read)(&self->slabs, i);
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, info->ptr,
                              SLAB_SIZE);
        NS(ALLOC, deallocate)(self->alloc_ref, info->ptr, SLAB_SIZE);
    }

    NS(SLAB_VECTOR, remove_at)(&self->slabs, 0, num_slabs);
    self->free_list_head = NULL;
}

DC_PUBLIC static void NS(SELF, delete)(SELF* self) {
    DC_ASSUME(self);

    NS(SELF, reset)(self);
    NS(SLAB_VECTOR, delete)(&self->slabs);
}

DC_PUBLIC static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    DC_ASSUME(self);
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);

    dc_debug_fmt_print(fmt, stream, "block_size: %zu,\n", (size_t)BLOCK_SIZE);
    dc_debug_fmt_print(fmt, stream, "slab_size: %zu,\n", (size_t)SLAB_SIZE);
    dc_debug_fmt_print(fmt, stream, "blocks_per_slab: %zu,\n", (size_t)(SLAB_SIZE / BLOCK_SIZE));
    dc_debug_fmt_print(fmt, stream, "num_slabs: %zu,\n", NS(SLAB_VECTOR, size)(&self->slabs));

    dc_debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(NS(NS(ALLOC, ref), deref)(self->alloc_ref), fmt, stream);
    fprintf(stream, "\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#undef SLAB_VECTOR
#undef SLAB_SIZE
#undef BLOCK_SIZE

DC_TRAIT_REFERENCABLE_BY_PTR(SELF);

DC_TRAIT_ALLOC(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>
