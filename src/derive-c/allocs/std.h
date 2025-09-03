/// An allocator for the standard allocator, which only has global state,
// and just uses malloc/calloc/free.

#pragma once

#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>
#include <stdlib.h>

#if defined __cplusplus
struct stdalloc {
    char UNUSED(_dummy_cpp_object_size_compatibility);
};
#else
typedef struct {
} stdalloc;
#endif

static stdalloc* NS(stdalloc, get)() {
    static stdalloc instance = {};
    return &instance;
}

static void* NS(stdalloc, malloc)(stdalloc* DEBUG_UNUSED(self), size_t size) {
    DEBUG_ASSERT(self);
    return malloc(size);
}

static void* NS(stdalloc, realloc)(stdalloc* DEBUG_UNUSED(self), void* ptr, size_t size) {
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(ptr);
    return realloc(ptr, size);
}

static void* NS(stdalloc, calloc)(stdalloc* DEBUG_UNUSED(self), size_t count, size_t size) {
    DEBUG_ASSERT(self);
    return calloc(count, size);
}

static void NS(stdalloc, free)(stdalloc* DEBUG_UNUSED(self), void* ptr) {
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(ptr);
    free(ptr);
}
