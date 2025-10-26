/// @brief A very simple bump allocator making use of a provided fixed size buffer (e.g. statically
/// allocated).
///  - Useful when an upper bound for allocation size is known at compile time
///  - Can be used when no global allocator is available (e.g. embedded systems, kernel dev)
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <derive-c/alloc/trait.h>
#include <derive-c/core/debug/fmt.h>
#include <derive-c/core/debug/gdb_marker.h>
#include <derive-c/core/debug/memory_tracker.h>
#include <derive-c/core/prelude.h>

#include <derive-c/core/self/def.h>

#if !defined CAPACITY
    #if !defined PLACEHOLDERS
        #error "The capacity of the static allocator must be defined"
    #endif
    #define CAPACITY 1024
#endif

#if CAPACITY > (1ULL << 30)
    #error "CAPACITY must not exceed 1 GiB"
#endif

#if CAPACITY <= UINT8_MAX
    #define USED uint8_t
#elif CAPACITY <= UINT16_MAX
    #define USED uint16_t
#elif CAPACITY <= UINT32_MAX
    #define USED uint32_t
#else
    #define USED uint64_t
#endif

typedef char NS(SELF, buffer)[CAPACITY];

typedef struct {
    size_t used;
    NS(SELF, buffer) * buffer;
    gdb_marker derive_c_staticbumpalloc;
} SELF;

static size_t NS(SELF, metadata_size) = sizeof(USED);

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
    memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_NONE, self.buffer, CAPACITY);

    return self;
}

/// Clear the allocator, note that all data should be freed before this occurs.
static void NS(SELF, clear)(SELF* self) {
    ASSUME(self);

#if !defined NDEBUG
    // JUSTIFY: Allocations & sizes zeroed on free in debug, we check all data has been freed.
    memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_READ_WRITE, (*self->buffer),
                       self->used);
    for (size_t i = 0; i < self->used; i++) {
        if ((*self->buffer)[i] != 0) {
            PANIC("Data not freed before clearing the static bump allocator");
        }
    }
#endif
    memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_WRITE, (*self->buffer),
                       self->used);
    memset((*self->buffer), 0, self->used);
    memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_NONE, (*self->buffer),
                       CAPACITY);
    self->used = 0;
}

static USED NS(SELF, get_used)(SELF const* self) {
    ASSUME(self);
    return self->used;
}

static void* NS(SELF, malloc)(SELF* self, size_t size) {
    ASSUME(self);
    if (self->used + (size + sizeof(USED)) > CAPACITY) {
        return NULL;
    }
    char* ptr = &(*self->buffer)[self->used];
    USED* used_ptr = (USED*)ptr;

    memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_WRITE, used_ptr, sizeof(USED));
    *used_ptr = size;
    char* allocation_ptr = ptr + sizeof(USED);
    memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_NONE, used_ptr, sizeof(USED));

    self->used += size + sizeof(USED);
    memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_WRITE, allocation_ptr, size);

    return allocation_ptr;
}

static void NS(SELF, free)(SELF* self, void* ptr) {
    ASSUME(self);
    ASSUME(ptr);

#if !defined NDEBUG
    // JUSTIFY: Zero memory in debug.
    //           - Expensive for release, but helpful when debugging
    // NOTE: This means that users should free, before they clear and reuse the buffer.
    USED* used_ptr = (USED*)((char*)ptr - sizeof(USED));
    memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_READ_WRITE, used_ptr,
                       sizeof(USED));
    memset(ptr, 0, *used_ptr);
    memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_NONE, ptr, *used_ptr);
    *used_ptr = 0;
    memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_NONE, used_ptr, sizeof(USED));
#endif
}

static void* NS(SELF, realloc)(SELF* self, void* ptr, size_t new_size) {
    ASSUME(self);

    if (!ptr) {
        return NS(SELF, malloc)(self, new_size);
    }

    char* byte_ptr = (char*)ptr;
    USED* old_size = (USED*)(byte_ptr - sizeof(USED));
    memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_READ_WRITE, old_size,
                       sizeof(USED));
    const bool was_last_alloc = (byte_ptr + *old_size == &(*self->buffer)[self->used]);

    if (was_last_alloc) {
        if (new_size > *old_size) {
            size_t increase = new_size - *old_size;
            if (self->used + increase > CAPACITY) {
                return NULL;
            }
            self->used += increase;
            memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_WRITE,
                               (char*)ptr + *old_size, increase);
        } else if (new_size < *old_size) {
            size_t decrease = *old_size - new_size;
            self->used -= decrease;

            // JUSTIFY: Zeroing the end of the old allocation
            // - Future calls to calloc may reuse this memory
            memset((char*)ptr + new_size, 0, decrease);

            memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_NONE,
                               (char*)ptr + new_size, decrease);
        }

        *old_size = new_size;
        memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_NONE, old_size,
                           sizeof(USED));
        return ptr;
    }

    if (new_size < *old_size) {
        // JUSTIFY: Shrinking an allocation
        //           - Can become a noop - as we cannot re-extend the allocation
        //             afterwards - no metadata for unused capacity not at end of buffer.
        *old_size = new_size;
        memory_tracker_set(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_NONE, old_size,
                           sizeof(USED));
        return ptr;
    }

    void* new_buff = NS(SELF, malloc)(self, new_size);
    if (!new_buff) {
        return NULL;
    }

    memcpy(new_buff, ptr, *old_size);

    NS(SELF, free)(self, ptr);
    return new_buff;
}

static void* NS(SELF, calloc)(SELF* self, size_t count, size_t size) {
    // JUSTIFY:
    //  - We already zeroed the buffer in `new()`
    return NS(SELF, malloc)(self, count * size);
}

static void NS(SELF, debug)(SELF const* self, debug_fmt fmt, FILE* stream) {
    fprintf(stream, STRINGIFY(SELF) "@%p {\n", self);
    fmt = debug_fmt_scope_begin(fmt);
    debug_fmt_print(fmt, stream, "capacity: %lu,\n", CAPACITY);
    debug_fmt_print(fmt, stream, "used: %lu,\n", self->used);
    debug_fmt_print(fmt, stream, "buffer: %p,\n", self->buffer);
    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "}");
}

#undef CAPACITY
#undef USED

TRAIT_ALLOC(SELF);
#include <derive-c/core/self/undef.h>
