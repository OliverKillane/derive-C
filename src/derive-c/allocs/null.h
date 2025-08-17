/// An allocator for the standard allocator, which only has global state,
// and just uses malloc/calloc/free.

#ifndef NULLALLOC_H
#define NULLALLOC_H

#include <derive-c/core.h>
#include <derive-c/panic.h>
#include <stdlib.h>

#ifdef __cplusplus
struct nullalloc {
    char UNUSED(_dummy_cpp_object_size_compatibility);
};
#else
typedef struct {
} nullalloc;
#endif

static nullalloc NAME(nullalloc, get)() { return (nullalloc){}; }

static void* NAME(nullalloc, malloc)(nullalloc* self, size_t size) {
    DEBUG_ASSERT(self);
    return NULL;
}

static void* NAME(nullalloc, realloc)(nullalloc* self, void* ptr, size_t size) {
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(ptr);
    return NULL;
}

static void* NAME(nullalloc, calloc)(nullalloc* self, size_t count, size_t size) {
    DEBUG_ASSERT(self);
    return NULL;
}

static void NAME(nullalloc, free)(nullalloc* self, void* ptr) {
    DEBUG_ASSERT(self);
    PANIC("Not possible to free memory from the null allocator, as it allocates nothing")
}

#endif