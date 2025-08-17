/// An allocator for the standard allocator, which only has global state,
// and just uses malloc/calloc/free.

#ifndef STDALLOC_H
#define STDALLOC_H

#include <derive-c/core.h>
#include <derive-c/panic.h>
#include <stdlib.h>

#ifdef __cplusplus
struct stdalloc {
    char UNUSED(_dummy_cpp_object_size_compatibility);
};
#else
typedef struct {
} stdalloc;
#endif

static stdalloc* NAME(stdalloc, get)() {
    static stdalloc instance = {};
    return &instance;
}

static void* NAME(stdalloc, malloc)(stdalloc* DEBUG_UNUSED(self), size_t size) {
    DEBUG_ASSERT(self);
    return malloc(size);
}

static void* NAME(stdalloc, realloc)(stdalloc* DEBUG_UNUSED(self), void* ptr, size_t size) {
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(ptr);
    return realloc(ptr, size);
}

static void* NAME(stdalloc, calloc)(stdalloc* DEBUG_UNUSED(self), size_t count, size_t size) {
    DEBUG_ASSERT(self);
    return calloc(count, size);
}

static void NAME(stdalloc, free)(stdalloc* DEBUG_UNUSED(self), void* ptr) {
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(ptr);
    free(ptr);
}

#endif