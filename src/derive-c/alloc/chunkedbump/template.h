/// @brief A chunked bump allocator that allocates memory in fixed-size blocks.

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
    #define BLOCK_SIZE 65536
#endif

DC_STATIC_ASSERT(BLOCK_SIZE > 0, "Block size must be larger than zero");

#define BLOCK_VECTOR NS(NAME, block_vector)

#pragma push_macro("ALLOC")

typedef struct {
    void* ptr;
    size_t num_blocks;
} NS(NAME, block_info);

static NS(NAME, block_info) NS(NAME, block_info_clone)(NS(NAME, block_info) const* self) {
    return *self;
}
static void NS(NAME, block_info_delete)(NS(NAME, block_info) * /* self */) {}
static void NS(NAME, block_info_debug)(NS(NAME, block_info) const* /* self */,
                                       dc_debug_fmt /* fmt */, FILE* /* stream */) {}

#define ITEM NS(NAME, block_info)               // [DERIVE-C] for template
#define ITEM_CLONE NS(NAME, block_info_clone)   // [DERIVE-C] for template
#define ITEM_DELETE NS(NAME, block_info_delete) // [DERIVE-C] for template
#define ITEM_DEBUG NS(NAME, block_info_debug)   // [DERIVE-C] for template
#define INTERNAL_NAME BLOCK_VECTOR              // [DERIVE-C] for template
#include <derive-c/container/vector/dynamic/template.h>

#pragma pop_macro("ALLOC")

typedef struct {
    BLOCK_VECTOR blocks;
    size_t current_block_idx;
    size_t current_block_offset;
    NS(ALLOC, ref) alloc_ref;
    dc_gdb_marker derive_c_chunkedbumpalloc;
} SELF;

DC_PUBLIC static SELF NS(SELF, new)(NS(ALLOC, ref) alloc_ref) {
    return (SELF){
        .blocks = NS(BLOCK_VECTOR, new)(alloc_ref),
        .current_block_idx = 0,
        .current_block_offset = 0,
        .alloc_ref = alloc_ref,
        .derive_c_chunkedbumpalloc = {},
    };
}

DC_INTERNAL static void* PRIV(NS(SELF, allocate_new_blocks))(SELF* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0);

    size_t num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t total_size = num_blocks * BLOCK_SIZE;

    void* base_ptr = NS(ALLOC, allocate_uninit)(self->alloc_ref, total_size);

    for (size_t i = 0; i < num_blocks; i++) {
        void* block_ptr = (char*)base_ptr + (i * BLOCK_SIZE);
        size_t block_count = (i == 0) ? num_blocks : 0;

        NS(NAME, block_info) info = {.ptr = block_ptr, .num_blocks = block_count};
        NS(BLOCK_VECTOR, push)(&self->blocks, info);
    }

    self->current_block_idx = NS(BLOCK_VECTOR, size)(&self->blocks) - 1;
    self->current_block_offset = size % BLOCK_SIZE;

    if (self->current_block_offset == 0 && size > 0) {
        self->current_block_offset = BLOCK_SIZE;
    }

    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, base_ptr, size);

    return base_ptr;
}

DC_PUBLIC static void* NS(SELF, allocate_uninit)(SELF* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0, "Cannot allocate zero sized");

    size_t num_blocks = NS(BLOCK_VECTOR, size)(&self->blocks);
    if (num_blocks > 0) {
        size_t remaining = BLOCK_SIZE - self->current_block_offset;

        if (remaining >= size) {
            NS(NAME, block_info)
            const* current_info = NS(BLOCK_VECTOR, read)(&self->blocks, self->current_block_idx);
            void* allocation_ptr = (char*)current_info->ptr + self->current_block_offset;

            self->current_block_offset += size;

            dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                                  allocation_ptr, size);

            return allocation_ptr;
        }
    }

    return PRIV(NS(SELF, allocate_new_blocks))(self, size);
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
    DC_ASSUME(ptr); // deallocate requires non-null pointer
    DC_ASSERT(size > 0, "Cannot deallocate zero sized");

    // Bump allocators don't free individual items, but we poison the memory
    // to catch use-after-free bugs
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, ptr, size);
}

DC_PUBLIC static void* NS(SELF, reallocate)(SELF* self, void* ptr, size_t old_size,
                                            size_t new_size) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);
    DC_ASSERT(old_size > 0, "Cannot reallocate zero sized");
    DC_ASSERT(new_size > 0, "Cannot allocate zero sized");

    // Check if this is the last allocation and can be extended in place
    size_t num_blocks = NS(BLOCK_VECTOR, size)(&self->blocks);
    if (num_blocks > 0 && new_size > old_size) {
        NS(NAME, block_info)
        const* current_info = NS(BLOCK_VECTOR, read)(&self->blocks, self->current_block_idx);
        char* expected_last_alloc =
            (char*)current_info->ptr + self->current_block_offset - old_size;

        if ((char*)ptr == expected_last_alloc) {
            // This is the last allocation - check if we can extend in place
            size_t extension = new_size - old_size;
            size_t remaining = BLOCK_SIZE - self->current_block_offset;

            if (extension <= remaining) {
                // Can extend in place!
                dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                                      (char*)ptr + old_size, extension);
                self->current_block_offset += extension;
                return ptr;
            }
        }
    }

    // Can't extend in place, allocate new memory and copy
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

    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, ptr, old_size);

    return new_ptr;
}

DC_PUBLIC static void NS(SELF, reset)(SELF* self) {
    DC_ASSUME(self);

    size_t num_blocks = NS(BLOCK_VECTOR, size)(&self->blocks);
    for (size_t i = 0; i < num_blocks; i++) {
        NS(NAME, block_info) const* info = NS(BLOCK_VECTOR, read)(&self->blocks, i);
        if (info->num_blocks > 0) {
            size_t total_size = info->num_blocks * BLOCK_SIZE;
            dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                                  info->ptr, total_size);
            NS(ALLOC, deallocate)(self->alloc_ref, info->ptr, total_size);
        }
    }

    NS(BLOCK_VECTOR, remove_at)(&self->blocks, 0, num_blocks);
    self->current_block_idx = 0;
    self->current_block_offset = 0;
}

DC_PUBLIC static void NS(SELF, delete)(SELF* self) {
    DC_ASSUME(self);

    NS(SELF, reset)(self);
    NS(BLOCK_VECTOR, delete)(&self->blocks);
}

DC_PUBLIC static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    DC_ASSUME(self);
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);

    dc_debug_fmt_print(fmt, stream, "block_size: %zu,\n", (size_t)BLOCK_SIZE);
    dc_debug_fmt_print(fmt, stream, "num_blocks: %zu,\n", NS(BLOCK_VECTOR, size)(&self->blocks));
    dc_debug_fmt_print(fmt, stream, "current_block_idx: %zu,\n", self->current_block_idx);
    dc_debug_fmt_print(fmt, stream, "current_block_offset: %zu,\n", self->current_block_offset);

    size_t total_allocated = 0;
    size_t num_blocks = NS(BLOCK_VECTOR, size)(&self->blocks);
    for (size_t i = 0; i < num_blocks; i++) {
        NS(NAME, block_info) const* info = NS(BLOCK_VECTOR, read)(&self->blocks, i);
        if (info->num_blocks > 0) {
            total_allocated += info->num_blocks * BLOCK_SIZE;
        }
    }
    dc_debug_fmt_print(fmt, stream, "total_allocated: %zu,\n", total_allocated);

    dc_debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(NS(NS(ALLOC, ref), deref)(self->alloc_ref), fmt, stream);
    fprintf(stream, "\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#undef BLOCK_VECTOR
#undef BLOCK_SIZE

DC_TRAIT_REFERENCABLE_BY_PTR(SELF);

DC_TRAIT_ALLOC(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>
