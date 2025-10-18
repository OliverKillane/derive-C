/// An allocator for the standard allocator, which only has global state,
// and just uses malloc/calloc/free.

#pragma once

#include <stdlib.h>

#include <derive-c/alloc/trait.h>
#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>
#include <derive-c/core/zerosized.h>

ZERO_SIZED(stdalloc);

static stdalloc* NS(stdalloc, get)() {
    static stdalloc instance = {};
    return &instance;
}

static void* NS(stdalloc, malloc)(stdalloc* self, size_t size) {
    DEBUG_ASSERT(self);
    return malloc(size);
}

static void* NS(stdalloc, realloc)(stdalloc* self, void* ptr, size_t size) {
    DEBUG_ASSERT(self);
    if (ptr) {
        return realloc(ptr, size);
    }
    return NS(stdalloc, malloc)(self, size);
}

static void* NS(stdalloc, calloc)(stdalloc* self, size_t count, size_t size) {
    DEBUG_ASSERT(self);
    return calloc(count, size);
}

static void NS(stdalloc, free)(stdalloc* self, void* ptr) {
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(ptr);
    free(ptr);
}

TRAIT_ALLOC(stdalloc);