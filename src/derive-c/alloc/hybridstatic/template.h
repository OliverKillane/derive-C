/// @brief A hybrid of a bump allocator on a statically allocated buffer, and any other allocator.
/// - Useful for reducing allocation overhead when the expected number of allocations is known.
/// - Once the static allocation is exhausted, it uses the fallback allocator.

// TODO(oliverkillane): This allocator stores the size of the allocation.
//  - This is unecessary, assuming correct old_sizes are provided.
//  - It is possible to remove this, and we should for a more optimal allocator

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

#if CAPACITY <= UINT8_MAX
    #define USED uint8_t
    #define UNALIGNED_VALID
#elif CAPACITY <= UINT16_MAX
    #define USED uint16_t
#elif CAPACITY <= UINT32_MAX
    #define USED uint32_t
#else
    #define USED uint64_t
#endif

#if defined UNALIGNED_VALID
static USED PRIV(NS(SELF, read_used))(const void* ptr) { return *(const USED*)ptr; }
static void PRIV(NS(SELF, write_used))(void* ptr, USED value) { *((USED*)ptr) = value; }
    #undef UNALIGNED_VALID
#else
// JUSTIFY: Special functions for reading the used count
//  - These values are misaligned, so would be UB to access directly.
static USED PRIV(NS(SELF, read_used))(const void* ptr) {
    USED value;
    memcpy(&value, ptr, sizeof(USED));
    return value;
}

static void PRIV(NS(SELF, write_used))(void* ptr, USED value) { memcpy(ptr, &value, sizeof(USED)); }
#endif

typedef char NS(SELF, buffer)[CAPACITY];

typedef struct {
    NS(SELF, buffer) * buffer;
    NS(ALLOC, ref) alloc_ref;
    USED head_offset;
    dc_gdb_marker derive_c_hybridstaticalloc;
} SELF;

static bool PRIV(NS(SELF, contains_ptr))(SELF const* self, void* ptr) {
    void* buffer_start = &(*self->buffer)[0];
    void* buffer_end = &(*self->buffer)[sizeof(NS(SELF, buffer))];
    return buffer_start <= ptr && ptr < buffer_end;
}

static SELF NS(SELF, new)(NS(SELF, buffer) * buffer, NS(ALLOC, ref) alloc_ref) {
    SELF self = {
        .buffer = buffer,
        .alloc_ref = alloc_ref,
        .head_offset = 0,
        .derive_c_hybridstaticalloc = {},
    };
    // JUSTIFY: Zeroed buffer
    //           - For easier debugging & view in gdb.
    //           - Additionally allows for calloc to be malloc.
    memset(*self.buffer, 0, CAPACITY);

    // JUSTIFY: no capabilities on init
    //  - Protect access by users outside of malloc/calloc
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, self.buffer,
                          CAPACITY);

    return self;
}

static void* PRIV(NS(SELF, static_allocate_zeroed))(SELF* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0);

    if (self->head_offset + (size + sizeof(USED)) > CAPACITY) {
        return NULL;
    }

    char* ptr = &(*self->buffer)[self->head_offset];
    USED* used_ptr = (USED*)ptr;

    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, used_ptr,
                          sizeof(USED));
    USED size_value = (USED)size;
    PRIV(NS(SELF, write_used))(used_ptr, size_value);
    char* allocation_ptr = ptr + sizeof(USED);
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, used_ptr,
                          sizeof(USED));

    self->head_offset += size + sizeof(USED);
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                          allocation_ptr, size);

    return allocation_ptr;
}

static void* NS(SELF, allocate_zeroed)(SELF* self, size_t size) {
    void* allocation_ptr = PRIV(NS(SELF, static_allocate_zeroed))(self, size);
    if (allocation_ptr == NULL) {
        return NS(ALLOC, allocate_zeroed)(self->alloc_ref, size);
    }
    return allocation_ptr;
}

static void* PRIV(NS(SELF, static_allocate_uninit))(SELF* self, size_t size) {
    void* allocation_ptr = PRIV(NS(SELF, static_allocate_zeroed))(self, size);

    if (allocation_ptr) {
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                              allocation_ptr, size);
    }

    return allocation_ptr;
}

static void* NS(SELF, allocate_uninit)(SELF* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0, "Cannot allocate zero sized");

    void* statically_allocated = PRIV(NS(SELF, static_allocate_uninit))(self, size);

    if (statically_allocated == NULL) {
        return NS(ALLOC, allocate_uninit)(self->alloc_ref, size);
    }

    return statically_allocated;
}

static void PRIV(NS(SELF, static_deallocate))(SELF* self, void* ptr, size_t size) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);

    USED* used_ptr = (USED*)((char*)ptr - sizeof(USED));
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE, used_ptr,
                          sizeof(USED));
    USED old_size_value = PRIV(NS(SELF, read_used))(used_ptr);
    DC_ASSERT(size == old_size_value, "size must match that originally allocated");
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, ptr,
                          old_size_value);
    USED zero = 0;
    PRIV(NS(SELF, write_used))(used_ptr, zero);
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, used_ptr,
                          sizeof(USED));
}

static void NS(SELF, deallocate)(SELF* self, void* ptr, size_t size) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);

    if (PRIV(NS(SELF, contains_ptr))(self, ptr)) {
        PRIV(NS(SELF, static_deallocate))(self, ptr, size);
    } else {
        NS(ALLOC, deallocate)(self->alloc_ref, ptr, size);
    }
}

static void* PRIV(NS(SELF, static_reallocate))(SELF* self, void* ptr, size_t old_size,
                                               size_t new_size) {
    char* byte_ptr = (char*)ptr;
    USED* old_size_ptr = (USED*)(byte_ptr - sizeof(USED));
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                          old_size_ptr, sizeof(USED));
    USED old_size_value = PRIV(NS(SELF, read_used))(old_size_ptr);
    DC_ASSUME(old_size_value == old_size, "Realloc old size does not match original size");

    if (new_size == old_size_value) {
        return ptr;
    }

    const bool was_last_alloc = (byte_ptr + old_size_value == &(*self->buffer)[self->head_offset]);

    if (was_last_alloc) {
        if (new_size > old_size_value) {
            size_t increase = new_size - old_size_value;
            if (self->head_offset + increase <= CAPACITY) {
                self->head_offset += increase;
                dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                                      (char*)ptr + old_size_value, increase);
                PRIV(NS(SELF, write_used))(old_size_ptr, (USED)new_size);
                dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE,
                                      old_size_ptr, sizeof(USED));
                return ptr;
            }
        } else if (new_size < old_size_value) {
            size_t decrease = old_size_value - new_size;
            self->head_offset -= decrease;

            // JUSTIFY: Zeroing the end of the old allocation
            // - Future calls to calloc may reuse this memory
            memset((char*)ptr + new_size, 0, decrease);

            dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE,
                                  (char*)ptr + new_size, decrease);
            PRIV(NS(SELF, write_used))(old_size_ptr, (USED)new_size);
            dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE,
                                  old_size_ptr, sizeof(USED));
            return ptr;
        }
    } else if (new_size < old_size_value) {
        // JUSTIFY: Shrinking an allocation
        //           - Can become a noop - as we cannot re-extend the allocation
        //             afterwards - no metadata for unused capacity not at end of buffer.
        PRIV(NS(SELF, write_used))(old_size_ptr, (USED)new_size);
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, old_size_ptr,
                              sizeof(USED));
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE,
                              (char*)ptr + new_size, old_size_value - new_size);
        return ptr;
    }

    // JUSTIFY: Not using `static_` methods for the new buffer.
    //  - We may be out of room in the static buffer, so we also need to optionally use the backup
    //  allocator.
    void* new_buff = NS(SELF, allocate_uninit)(self, new_size);
    memcpy(new_buff, ptr, old_size_value);

    PRIV(NS(SELF, static_deallocate))(self, ptr, old_size_value);
    return new_buff;
}

static void* NS(SELF, reallocate)(SELF* self, void* ptr, size_t old_size, size_t new_size) {
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

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    DC_ASSUME(self);
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "capacity: %lu,\n", CAPACITY);
    dc_debug_fmt_print(fmt, stream, "used: %lu,\n", self->head_offset);
    dc_debug_fmt_print(fmt, stream, "buffer: %p,\n", self->buffer);
    dc_debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(NS(NS(ALLOC, ref), deref)(self->alloc_ref), fmt, stream);
    fprintf(stream, "\n");
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#undef USED
#undef CAPACITY

static void NS(SELF, delete)(SELF* self) {
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                          &self->buffer[0], sizeof(NS(SELF, buffer)));
    DC_ASSUME(self);
}

DC_TRAIT_REFERENCABLE_BY_PTR(SELF);

DC_TRAIT_ALLOC(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>
