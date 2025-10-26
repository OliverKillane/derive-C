/// An allocator for the standard allocator, which only has global state,
// and just uses malloc/calloc/free.

#pragma once

#include "derive-c/core/debug/fmt.h"
#include <stdio.h>
#include <stdlib.h>

#include <derive-c/alloc/trait.h>
#include <derive-c/core/prelude.h>

ZERO_SIZED(stdalloc);

static stdalloc* NS(stdalloc, get)() {
    static stdalloc instance = {};
    return &instance;
}

static void* NS(stdalloc, malloc)(stdalloc* self, size_t size) {
    ASSUME(self);
    return malloc(size);
}

static void* NS(stdalloc, realloc)(stdalloc* self, void* ptr, size_t size) {
    ASSUME(self);
    if (ptr) {
        return realloc(ptr, size);
    }
    return NS(stdalloc, malloc)(self, size);
}

static void* NS(stdalloc, calloc)(stdalloc* self, size_t count, size_t size) {
    ASSUME(self);
    return calloc(count, size);
}

static void NS(stdalloc, free)(stdalloc* self, void* ptr) {
    ASSUME(self);
    ASSUME(ptr);
    free(ptr);
}

static void NS(stdalloc, debug)(stdalloc const* self, debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "stdalloc@%p { }", self);
}

TRAIT_ALLOC(stdalloc);
