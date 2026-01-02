/// @brief A hybrid of a bump allocator on a statically allocated buffer, and any other allocator.
/// - Useful for reducing allocation overhead when the expected number of allocations is known.
/// - Once the static allocation is exhausted, it uses the fallback allocator.

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined CAPACITY
    #if !defined PLACEHOLDERS
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

static void* PRIV(NS(SELF, static_calloc))(SELF* self, size_t size) {
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

static void* NS(SELF, calloc)(SELF* self, size_t count, size_t size) {
    void* allocation_ptr = PRIV(NS(SELF, static_calloc))(self, count * size);
    if (allocation_ptr == NULL) {
        return NS(ALLOC, calloc)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), count, size);
    }
    return allocation_ptr;
}

static void* PRIV(NS(SELF, static_malloc))(SELF* self, size_t size) {
    void* allocation_ptr = PRIV(NS(SELF, static_calloc))(self, size);

    if (allocation_ptr) {
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                              allocation_ptr, size);
    }

    return allocation_ptr;
}

static void* NS(SELF, malloc)(SELF* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0, "Cannot allocate zero sized");

    void* statically_allocated = PRIV(NS(SELF, static_malloc))(self, size);

    if (statically_allocated == NULL) {
        return NS(ALLOC, malloc)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), size);
    }

    return statically_allocated;
}

static void PRIV(NS(SELF, static_free))(SELF* self, void* ptr) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);

#if !defined NDEBUG
    // JUSTIFY: Zero memory in debug.
    //           - Expensive for release, but helpful when debugging
    // NOTE: This means that users should free, before they clear and reuse the buffer.
    USED* used_ptr = (USED*)((char*)ptr - sizeof(USED));
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE, used_ptr,
                          sizeof(USED));
    USED old_size_value = PRIV(NS(SELF, read_used))(used_ptr);
    memset(ptr, 0, old_size_value);
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, ptr,
                          old_size_value);
    USED zero = 0;
    PRIV(NS(SELF, write_used))(used_ptr, zero);
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, used_ptr,
                          sizeof(USED));
#endif
}

static void NS(SELF, free)(SELF* self, void* ptr) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);

    if (PRIV(NS(SELF, contains_ptr))(self, ptr)) {
        PRIV(NS(SELF, static_free))(self, ptr);
    } else {
        NS(ALLOC, free)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), ptr);
    }
}

static void* PRIV(NS(SELF, static_realloc))(SELF* self, void* ptr, size_t new_size) {
    char* byte_ptr = (char*)ptr;
    USED* old_size_ptr = (USED*)(byte_ptr - sizeof(USED));
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                          old_size_ptr, sizeof(USED));
    USED old_size_value = PRIV(NS(SELF, read_used))(old_size_ptr);

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
    void* new_buff = NS(SELF, malloc)(self, new_size);
    memcpy(new_buff, ptr, old_size_value);

    PRIV(NS(SELF, static_free))(self, ptr);
    return new_buff;
}

static void* NS(SELF, realloc)(SELF* self, void* ptr, size_t new_size) {
    DC_ASSUME(self);
    DC_ASSERT(new_size > 0, "Cannot allocate zero sized");

    if (!ptr) {
        return NS(SELF, malloc)(self, new_size);
    }

    if (!PRIV(NS(SELF, contains_ptr))(self, ptr)) {
        return NS(ALLOC, realloc)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), ptr, new_size);
    }

    return PRIV(NS(SELF, static_realloc))(self, ptr, new_size);
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    DC_ASSUME(self);
    fprintf(stream, STRINGIFY(SELF) "@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "capacity: %lu,\n", CAPACITY);
    dc_debug_fmt_print(fmt, stream, "used: %lu,\n", self->head_offset);
    dc_debug_fmt_print(fmt, stream, "buffer: %p,\n", self->buffer);
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
