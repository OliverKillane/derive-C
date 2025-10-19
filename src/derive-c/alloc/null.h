/// An allocator for the standard allocator, which only has global state,
// and just uses malloc/calloc/free.

#pragma once

#include <stdlib.h>

#include <derive-c/alloc/trait.h>
#include <derive-c/core/prelude.h>

ZERO_SIZED(nullalloc);

static nullalloc NS(nullalloc, get)() { return (nullalloc){}; }

static void* NS(nullalloc, malloc)(nullalloc* self, size_t size) {
    (void)size;
    ASSUME(self);
    return NULL;
}

static void* NS(nullalloc, realloc)(nullalloc* self, void* ptr, size_t size) {
    (void)size;
    ASSUME(self);
    ASSUME(ptr);
    return NULL;
}

static void* NS(nullalloc, calloc)(nullalloc* self, size_t count, size_t size) {
    (void)count;
    (void)size;
    ASSUME(self);
    return NULL;
}

static void NS(nullalloc, free)(nullalloc* self, void* ptr) {
    (void)ptr;
    ASSUME(self);
    PANIC("Not possible to free memory from the null allocator, as it allocates nothing")
}

TRAIT_ALLOC(nullalloc);
