/// An allocator for the standard allocator, which only has global state,
// and just uses malloc/calloc/free.

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <derive-c/alloc/trait.h>
#include <derive-c/core/prelude.h>

DC_ZERO_SIZED(stdalloc);
static stdalloc stdalloc_instance = {};
DC_TRAIT_REFERENCABLE_SINGLETON(stdalloc, stdalloc_instance);

static void* NS(stdalloc, malloc)(stdalloc_ref /* ref */, size_t size) {
    DC_ASSERT(size > 0, "Cannot allocate zero sized");
    void* alloc = malloc(size);
    DC_ASSERT(alloc != NULL, "Standard allocator failed to malloc");
    return alloc;
}

static void* NS(stdalloc, realloc)(stdalloc_ref ref, void* ptr, size_t size) {
    DC_ASSERT(size > 0, "Cannot allocate zero sized");
    if (ptr) {
        void* new_ptr = realloc(ptr, size);
        DC_ASSERT(new_ptr != NULL, "Standard allocator failed to realloc");
        return new_ptr;
    }
    return NS(stdalloc, malloc)(ref, size);
}

static void* NS(stdalloc, calloc)(stdalloc_ref /* ref */, size_t count, size_t size) {
    DC_ASSERT(size > 0, "Cannot allocate zero sized");
    void* alloc = calloc(count, size);
    DC_ASSERT(alloc != NULL, "Standard allocator failed to calloc");
    return alloc;
}

static void NS(stdalloc, free)(stdalloc_ref /* ref */, void* ptr) {
    DC_ASSUME(ptr);

    // JUSTIFY: Ignoring malloc clang static analyser warnings in this branch
    // To make this safe we need to prove:
    //  1. the ptr was allocated by self
    //
    // Allocators such as the hybridstatic allocator make it harder it harder
    //  - provenance: clang static analyser cannot prove a pointer's origin
    //  - can be shown by wrapping allocator's returns, but this adds substantial
    //    complexity, and makes the returned ptr-types less easy to use
    //  - can also use special headers on allocations, but this adds complexity
    // So we settle for ignoring here:

    free(ptr); // NOLINT(clang-analyzer-unix.Malloc)
}

static void NS(stdalloc, debug)(stdalloc const* self, dc_debug_fmt fmt, FILE* stream) {
    DC_ASSUME(self);
    (void)fmt;
    fprintf(stream, "stdalloc@%p { }", self);
}

static void NS(stdalloc, delete)(stdalloc* self) { DC_ASSUME(self); }

DC_TRAIT_ALLOC(stdalloc);
