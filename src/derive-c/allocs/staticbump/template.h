/// @brief A very simple bump allocator making use of a provided fixed size buffer (e.g. statically
/// allocated).
///  - Useful when an upper bound for allocation size is known at compile time
///  - Can be used when no global allocator is available (e.g. embedded systems, kernel dev)
#include <stdint.h>
#include <string.h>

#include <derive-c/core.h>
#include <derive-c/panic.h>
#include <derive-c/self.h>

#ifndef CAPACITY
#ifndef __clang_analyzer__
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

typedef struct {
    size_t used;
    char buffer[CAPACITY];
    gdb_marker derive_c_staticbumpalloc;
} SELF;

static size_t NAME(SELF, metadata_size) = sizeof(USED);

static SELF NAME(SELF, new)() {
    SELF self = {.used = 0, .derive_c_staticbumpalloc = {}};
    // JUSTIFY: Zeroed buffer
    //           - For easier debugging & view in gdb.
    //           - Additionally allows for calloc to be malloc.
    memset(self.buffer, 0, CAPACITY);
    return self;
}

/// Clear the allocator, note that all data should be freed before this occurs.
static void NAME(SELF, clear)(SELF* self) {
    DEBUG_ASSERT(self);

#ifndef NDEBUG
    // JUSTIFY: Allocations & sizes zeroed on free in debug, we check all data has been freed.
    for (size_t i = 0; i < self->used; i++) {
        if (self->buffer[i] != 0) {
            PANIC("Data not freed before clearing the static bump allocator");
        }
    }
#endif
    memset(self->buffer, 0, self->used);
    self->used = 0;
}

static USED NAME(SELF, get_used)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->used;
}

static void* NAME(SELF, malloc)(SELF* self, size_t size) {
    DEBUG_ASSERT(self);
    if (self->used + (size + sizeof(USED)) > CAPACITY) {
        return NULL;
    }
    char* ptr = &self->buffer[self->used];
    USED* used_ptr = (USED*)ptr;
    *used_ptr = size;
    self->used += size + sizeof(USED);
    return ptr + sizeof(USED);
}

static void NAME(SELF, free)(SELF* DEBUG_UNUSED(self), void* DEBUG_UNUSED(ptr)) {
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(ptr);

#ifndef NDEBUG
    // JUSTIFY: Zero memory in debug.
    //           - Expensive for release, but helpful when debugging
    // NOTE: This means that users should free, before they clear and reuse the buffer.
    USED* used_ptr = (USED*)((char*)ptr - sizeof(USED));
    memset(ptr, 0, *used_ptr);
    *used_ptr = 0;
#endif
}

static void* NAME(SELF, realloc)(SELF* self, void* ptr, size_t new_size) {
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(ptr);

    char* byte_ptr = (char*)ptr;
    USED* old_size = (USED*)(byte_ptr - sizeof(USED));
    const bool was_last_alloc = (byte_ptr + *old_size == self->buffer + self->used);

    if (was_last_alloc) {
        if (self->used + (new_size - *old_size) > CAPACITY) {
            return NULL;
        }

        self->used += (new_size - *old_size);
        *old_size = new_size;
        return ptr;
    }

    if (new_size < *old_size) {
        // JUSTIFY: Shrinking an allocation
        //           - Can become a noop - as we cannot re-extend the allocation
        //             afterwards - no metadata for unused capacity not at end of buffer.
        *old_size = new_size;
        return ptr;
    }

    void* new_buff = NAME(SELF, malloc)(self, new_size);
    if (!new_buff) {
        return NULL;
    }

    memcpy(new_buff, ptr, *old_size);

    NAME(SELF, free)(self, ptr);
    return new_buff;
}

static void* NAME(SELF, calloc)(SELF* self, size_t count, size_t size) {
    // JUSTIFY:
    //  - We already zeroed the buffer in `new()`
    return NAME(SELF, malloc)(self, count * size);
}

#undef USED
#undef ALLOC
#undef SELF
