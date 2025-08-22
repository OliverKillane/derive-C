/// An allocator for the standard allocator, which only has global state,
// and just uses malloc/calloc/free.

#pragma once

#include <derive-c/core.h>
#include <derive-c/panic.h>
#include <stdlib.h>

#if defined __cplusplus
    struct nullalloc {
        char UNUSED(_dummy_cpp_object_size_compatibility);
    };
#else
    typedef struct {
    } nullalloc;
#endif

static nullalloc NAME(nullalloc, get)() { return (nullalloc){}; }

static void* NAME(nullalloc, malloc)(nullalloc* DEBUG_UNUSED(self), size_t UNUSED(size)) {
    DEBUG_ASSERT(self);
    return NULL;
}

static void* NAME(nullalloc, realloc)(nullalloc* DEBUG_UNUSED(self), void* DEBUG_UNUSED(ptr),
                                      size_t UNUSED(size)) {
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(ptr);
    return NULL;
}

static void* NAME(nullalloc, calloc)(nullalloc* DEBUG_UNUSED(self), size_t UNUSED(count),
                                     size_t UNUSED(size)) {
    DEBUG_ASSERT(self);
    return NULL;
}

static void NAME(nullalloc, free)(nullalloc* DEBUG_UNUSED(self), void* UNUSED(ptr)) {
    DEBUG_ASSERT(self);
    PANIC("Not possible to free memory from the null allocator, as it allocates nothing")
}
