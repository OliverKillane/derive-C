/// @brief A hybrid of a bump allocator on a statically allocated buffer, and any other allocator.
/// - Useful for reducing allocation overhead when the expected number of allocations is known.
/// - Once the static allocation is exhausted, it uses the fallback allocator.
/// - Simplified: no size storage, no buffer zeroing, tracks only head and last allocation

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined CAPACITY
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("No CAPACITY")
    #endif
    #define CAPACITY 1024
#endif

DC_STATIC_ASSERT(CAPACITY > 0, "Capacity must be larger than zero");

typedef char NS(SELF, buffer)[CAPACITY];

typedef struct {
    NS(SELF, buffer) * buffer;
    NS(ALLOC, ref) alloc_ref;
    char* head_ptr;       // Points to end of used buffer
    char* last_alloc_ptr; // Points to last allocation, or NULL if none
    dc_gdb_marker derive_c_hybridstaticalloc;
} SELF;

DC_INTERNAL static bool PRIV(NS(SELF, contains_ptr))(SELF const* self, void* ptr) {
    void* buffer_start = &(*self->buffer)[0];
    void* buffer_end = &(*self->buffer)[CAPACITY];
    return buffer_start <= ptr && ptr < buffer_end;
}

DC_PUBLIC static SELF NS(SELF, new)(NS(SELF, buffer) * buffer, NS(ALLOC, ref) alloc_ref) {
    SELF self = {
        .buffer = buffer,
        .alloc_ref = alloc_ref,
        .head_ptr = &(*buffer)[0],
        .last_alloc_ptr = NULL,
        .derive_c_hybridstaticalloc = {},
    };

    // JUSTIFY: no capabilities on init
    //  - Protect access by users outside of malloc/calloc
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, self.buffer,
                          CAPACITY);

    return self;
}

DC_PUBLIC static void* PRIV(NS(SELF, static_allocate_zeroed))(SELF* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0);

    char* buffer_end = &(*self->buffer)[CAPACITY];
    if (self->head_ptr + size > buffer_end) {
        return NULL;
    }

    char* allocation_ptr = self->head_ptr;

    // Set permissions before zeroing (buffer might be poisoned)
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, allocation_ptr,
                          size);

    // Zero the allocation
    memset(allocation_ptr, 0, size);

    self->last_alloc_ptr = allocation_ptr;
    self->head_ptr += size;

    // Now set to read-write
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                          allocation_ptr, size);

    return allocation_ptr;
}

DC_PUBLIC static void* NS(SELF, allocate_zeroed)(SELF* self, size_t size) {
    void* allocation_ptr = PRIV(NS(SELF, static_allocate_zeroed))(self, size);
    if (allocation_ptr == NULL) {
        return NS(ALLOC, allocate_zeroed)(self->alloc_ref, size);
    }
    return allocation_ptr;
}

DC_PUBLIC static void* PRIV(NS(SELF, static_allocate_uninit))(SELF* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0);

    char* buffer_end = &(*self->buffer)[CAPACITY];
    if (self->head_ptr + size > buffer_end) {
        return NULL;
    }

    char* allocation_ptr = self->head_ptr;

    self->last_alloc_ptr = allocation_ptr;
    self->head_ptr += size;

    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, allocation_ptr,
                          size);

    return allocation_ptr;
}

DC_PUBLIC static void* NS(SELF, allocate_uninit)(SELF* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0, "Cannot allocate zero sized");

    void* statically_allocated = PRIV(NS(SELF, static_allocate_uninit))(self, size);

    if (statically_allocated == NULL) {
        return NS(ALLOC, allocate_uninit)(self->alloc_ref, size);
    }

    return statically_allocated;
}

DC_PUBLIC static void PRIV(NS(SELF, static_deallocate))(SELF* self, void* ptr, size_t size) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);

    char* byte_ptr = (char*)ptr;

    // If this is the last allocation, we can reclaim the space
    if (byte_ptr == self->last_alloc_ptr) {
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, ptr, size);
        self->head_ptr = byte_ptr;
        self->last_alloc_ptr = NULL;
    } else {
        // Not the last allocation, just mark as inaccessible
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, ptr, size);
    }
}

DC_PUBLIC static void NS(SELF, deallocate)(SELF* self, void* ptr, size_t size) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);

    if (PRIV(NS(SELF, contains_ptr))(self, ptr)) {
        PRIV(NS(SELF, static_deallocate))(self, ptr, size);
    } else {
        NS(ALLOC, deallocate)(self->alloc_ref, ptr, size);
    }
}

DC_PUBLIC static void* PRIV(NS(SELF, static_reallocate))(SELF* self, void* ptr, size_t old_size,
                                                         size_t new_size) {
    char* byte_ptr = (char*)ptr;

    if (new_size == old_size) {
        return ptr;
    }

    // If this is the last allocation, we can grow/shrink in place
    if (byte_ptr == self->last_alloc_ptr) {
        char* buffer_end = &(*self->buffer)[CAPACITY];

        if (new_size > old_size) {
            // Growing
            size_t increase = new_size - old_size;
            if (self->head_ptr + increase <= buffer_end) {
                dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                                      self->head_ptr, increase);
                self->head_ptr += increase;
                return ptr;
            }
        } else {
            // Shrinking
            size_t decrease = old_size - new_size;
            dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE,
                                  byte_ptr + new_size, decrease);
            self->head_ptr -= decrease;
            return ptr;
        }
    } else if (new_size < old_size) {
        // Not the last allocation, but shrinking - just mark extra space as inaccessible
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE,
                              byte_ptr + new_size, old_size - new_size);
        return ptr;
    }

    // Need to allocate new buffer and copy
    void* new_buff = NS(SELF, allocate_uninit)(self, new_size);
    memcpy(new_buff, ptr, old_size < new_size ? old_size : new_size);

    PRIV(NS(SELF, static_deallocate))(self, ptr, old_size);
    return new_buff;
}

DC_PUBLIC static void* NS(SELF, reallocate)(SELF* self, void* ptr, size_t old_size,
                                            size_t new_size) {
    DC_ASSUME(self);
    DC_ASSERT(new_size > 0, "Cannot allocate zero sized");

    if (!ptr) {
        return NS(SELF, allocate_uninit)(self, new_size);
    }

    if (!PRIV(NS(SELF, contains_ptr))(self, ptr)) {
        return NS(ALLOC, reallocate)(self->alloc_ref, ptr, old_size, new_size);
    }

    return PRIV(NS(SELF, static_reallocate))(self, ptr, old_size, new_size);
}

DC_PUBLIC static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    DC_ASSUME(self);
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "capacity: %zu,\n", (size_t)CAPACITY);
    dc_debug_fmt_print(fmt, stream, "used: %zu,\n", (size_t)(self->head_ptr - &(*self->buffer)[0]));
    dc_debug_fmt_print(fmt, stream, "buffer: %p,\n", (void*)self->buffer);
    dc_debug_fmt_print(fmt, stream, "last_alloc: %p,\n", (void*)self->last_alloc_ptr);
    dc_debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(NS(NS(ALLOC, ref), deref)(self->alloc_ref), fmt, stream);
    fprintf(stream, "\n");
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#undef CAPACITY

DC_PUBLIC static void NS(SELF, delete)(SELF* self) {
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                          &self->buffer[0], sizeof(NS(SELF, buffer)));
    DC_ASSUME(self);
}

DC_TRAIT_REFERENCABLE_BY_PTR(SELF);

DC_TRAIT_ALLOC(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>
