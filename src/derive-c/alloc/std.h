/// An allocator for the standard allocator, which only has global state,
// and just uses malloc/calloc/free.

#pragma once

#include "derive-c/core/panic.h"
#include <stdio.h>
#include <stdlib.h>

#include <derive-c/alloc/trait.h>
#include <derive-c/core/prelude.h>

DC_ZERO_SIZED(stdalloc);
static stdalloc stdalloc_instance = {};
DC_TRAIT_REFERENCABLE_SINGLETON(stdalloc, stdalloc_instance);

static stdalloc* NS(stdalloc, get)() { return &stdalloc_instance; }

static void* NS(stdalloc, malloc)(stdalloc* self, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0, "Cannot allocate zero sized");
    void* alloc = malloc(size);
    DC_ASSERT(alloc != NULL, "Standard allocator failed to malloc");
    return alloc;
}

static void* NS(stdalloc, realloc)(stdalloc* self, void* ptr, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0, "Cannot allocate zero sized");
    if (ptr) {
        void* new_ptr = realloc(ptr, size);
        DC_ASSERT(new_ptr != NULL, "Standard allocator failed to realloc");
        return new_ptr;
    }
    return NS(stdalloc, malloc)(self, size);
}

static void* NS(stdalloc, calloc)(stdalloc* self, size_t count, size_t size) {
    DC_ASSUME(self);
    DC_ASSERT(size > 0, "Cannot allocate zero sized");
    void* alloc = calloc(count, size);
    DC_ASSERT(alloc != NULL, "Standard allocator failed to calloc");
    return alloc;
}

static void NS(stdalloc, free)(stdalloc* self, void* ptr) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);
    free(ptr);
}

static void NS(stdalloc, debug)(stdalloc const* self, dc_debug_fmt fmt, FILE* stream) {
    DC_ASSUME(self);
    (void)fmt;
    fprintf(stream, "stdalloc@%p { }", self);
}

static void NS(stdalloc, delete)(stdalloc* self) { DC_ASSUME(self); }

DC_TRAIT_ALLOC(stdalloc);
