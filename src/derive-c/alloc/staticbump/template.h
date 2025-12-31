/// @brief A very simple bump allocator making use of a provided fixed size buffer (e.g. statically
/// allocated).
///  - Useful when an upper bound for allocation size is known at compile time
///  - Can be used when no global allocator is available (e.g. embedded systems, kernel dev)

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/self/def.h>

#if !defined CAPACITY
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("No CAPACITY")
    #endif
    #define CAPACITY 1024
#endif

#if CAPACITY > (1ULL << 30)
TEMPLATE_ERROR("CAPACITY must not exceed 1 GiB")
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
    size_t used;
    NS(SELF, buffer) * buffer;
    dc_gdb_marker derive_c_staticbumpalloc;
} SELF;

static size_t const NS(SELF, capacity) = CAPACITY;
static size_t const NS(SELF, metadata_size) = sizeof(USED);

static SELF NS(SELF, new)(NS(SELF, buffer) * buffer) {
    SELF self = {
        .used = 0,
        .buffer = buffer,
        .derive_c_staticbumpalloc = {},
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

/// Clear the allocator, note that all data should be freed before this occurs.
static void NS(SELF, clear)(SELF* self) {
    DC_ASSUME(self);

#if !defined NDEBUG
    // JUSTIFY: Allocations & sizes zeroed on free in debug, we check all data has been freed.
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                          (*self->buffer), self->used);
    for (size_t i = 0; i < self->used; i++) {
        if ((*self->buffer)[i] != 0) {
            DC_PANIC("Data not freed before clearing the static bump allocator");
        }
    }
#endif
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, (*self->buffer),
                          self->used);
    memset((*self->buffer), 0, self->used);
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, (*self->buffer),
                          CAPACITY);
    self->used = 0;
}

static USED NS(SELF, get_used)(SELF const* self) {
    DC_ASSUME(self);
    return self->used;
}

static void* NS(SELF, malloc)(SELF* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0, "Cannot allocate zero sized");

    if (self->used + (size + sizeof(USED)) > CAPACITY) {
        return NULL;
    }
    char* ptr = &(*self->buffer)[self->used];
    USED* used_ptr = (USED*)ptr;

    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, used_ptr,
                          sizeof(USED));
    USED size_value = (USED)size;
    PRIV(NS(SELF, write_used))(used_ptr, size_value);
    char* allocation_ptr = ptr + sizeof(USED);
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, used_ptr,
                          sizeof(USED));

    self->used += size + sizeof(USED);
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, allocation_ptr,
                          size);

    return allocation_ptr;
}

static void NS(SELF, free)(SELF* self, void* ptr) {
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

static void* NS(SELF, realloc)(SELF* self, void* ptr, size_t new_size) {
    DC_ASSUME(self);
    DC_ASSERT(new_size > 0, "Cannot allocate zero sized");

    if (!ptr) {
        return NS(SELF, malloc)(self, new_size);
    }

    char* byte_ptr = (char*)ptr;
    USED* old_size_ptr = (USED*)(byte_ptr - sizeof(USED));
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                          old_size_ptr, sizeof(USED));
    USED old_size_value = PRIV(NS(SELF, read_used))(old_size_ptr);
    const bool was_last_alloc = (byte_ptr + old_size_value == &(*self->buffer)[self->used]);

    if (was_last_alloc) {
        if (new_size > old_size_value) {
            size_t increase = new_size - old_size_value;
            if (self->used + increase > CAPACITY) {
                return NULL;
            }
            self->used += increase;
            dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                                  (char*)ptr + old_size_value, increase);
        } else if (new_size < old_size_value) {
            size_t decrease = old_size_value - new_size;
            self->used -= decrease;

            // JUSTIFY: Zeroing the end of the old allocation
            // - Future calls to calloc may reuse this memory
            memset((char*)ptr + new_size, 0, decrease);

            dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE,
                                  (char*)ptr + new_size, decrease);
        }

        USED new_size_value = (USED)new_size;
        PRIV(NS(SELF, write_used))(old_size_ptr, new_size_value);
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, old_size_ptr,
                              sizeof(USED));
        return ptr;
    }

    if (new_size < old_size_value) {
        // JUSTIFY: Shrinking an allocation
        //           - Can become a noop - as we cannot re-extend the allocation
        //             afterwards - no metadata for unused capacity not at end of buffer.
        USED new_size_value = (USED)new_size;
        PRIV(NS(SELF, write_used))(old_size_ptr, new_size_value);
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, old_size_ptr,
                              sizeof(USED));
        return ptr;
    }

    void* new_buff = NS(SELF, malloc)(self, new_size);
    if (!new_buff) {
        return NULL;
    }

    memcpy(new_buff, ptr, old_size_value);

    NS(SELF, free)(self, ptr);
    return new_buff;
}

static void* NS(SELF, calloc)(SELF* self, size_t count, size_t size) {
    // JUSTIFY:
    //  - We already zeroed the buffer in `new()`
    return NS(SELF, malloc)(self, count * size);
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    DC_ASSUME(self);
    fprintf(stream, STRINGIFY(SELF) "@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "capacity: %lu,\n", CAPACITY);
    dc_debug_fmt_print(fmt, stream, "used: %lu,\n", self->used);
    dc_debug_fmt_print(fmt, stream, "buffer: %p,\n", self->buffer);
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#undef USED
#undef CAPACITY

static void NS(SELF, delete)(SELF* self) { DC_ASSUME(self); }

DC_TRAIT_REFERENCABLE_BY_PTR(SELF);

DC_TRAIT_ALLOC(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/includes/undef.h>
