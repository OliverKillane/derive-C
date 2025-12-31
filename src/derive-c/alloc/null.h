/// An allocator for the standard allocator, which only has global state,
// and just uses malloc/calloc/free.

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <derive-c/alloc/trait.h>
#include <derive-c/core/prelude.h>

DC_ZERO_SIZED(nullalloc);

static nullalloc dc_nullalloc_instance = {};
static nullalloc* NS(nullalloc, get)() { return &dc_nullalloc_instance; }
DC_TRAIT_REFERENCABLE_SINGLETON(nullalloc, dc_nullalloc_instance);

static void* NS(nullalloc, malloc)(nullalloc* self, size_t size) {
    (void)size;
    DC_ASSUME(self);
    return NULL;
}

static void* NS(nullalloc, realloc)(nullalloc* self, void* ptr, size_t size) {
    (void)size;
    DC_ASSUME(self);
    DC_ASSUME(
        !ptr,
        "Got a non-null pointer to realloc, but this allocator can only return null pointers");
    return NULL;
}

static void* NS(nullalloc, calloc)(nullalloc* self, size_t count, size_t size) {
    (void)count;
    (void)size;
    DC_ASSUME(self);
    return NULL;
}

static void NS(nullalloc, free)(nullalloc* self, void* ptr) {
    (void)ptr;
    DC_ASSUME(self);
    DC_PANIC("Not possible to free memory from the null allocator, as it allocates nothing");
}

static void NS(nullalloc, debug)(nullalloc const* self, dc_debug_fmt fmt, FILE* stream) {
    (void)fmt;
    DC_ASSUME(self);
    fprintf(stream, "nullalloc@%p { }", self);
}

static void NS(nullalloc, delete)(nullalloc* self) { DC_ASSUME(self); }

DC_TRAIT_ALLOC(nullalloc);
