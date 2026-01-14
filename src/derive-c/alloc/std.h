/// An allocator for the standard allocator, which only has global state,
// and just uses malloc/calloc/free.

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <derive-c/alloc/trait.h>
#include <derive-c/core/prelude.h>
#include <derive-c/core/debug/memory_tracker.h>

DC_ZERO_SIZED(stdalloc);
static stdalloc stdalloc_instance = {};
DC_TRAIT_REFERENCABLE_SINGLETON(stdalloc, stdalloc_instance);

PUBLIC static void* NS(stdalloc, allocate_uninit)(stdalloc_ref /* ref */, size_t size) {
    DC_ASSERT(size > 0, "Cannot allocate zero sized");
    void* alloc = malloc(size);

    // JUSTIFY: Setting memory capabilities
    //  - Flakiness observed for tests running under msan with uninstrumented glibc
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, alloc, size);
    DC_ASSERT(alloc != NULL, "Standard allocator failed to malloc");
    return alloc;
}

PUBLIC static void* NS(stdalloc, allocate_zeroed)(stdalloc_ref /* ref */, size_t size) {
    DC_ASSERT(size > 0, "Cannot allocate zero sized");
    void* alloc = calloc(size, 1);

    // JUSTIFY: Setting memory capabilities
    //  - Flakiness observed for tests running under msan with uninstrumented glibc
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE, alloc,
                          size);

    DC_ASSERT(alloc != NULL, "Standard allocator failed to calloc");
    return alloc;
}

PUBLIC static void* NS(stdalloc, reallocate)(stdalloc_ref /* ref */, void* ptr, size_t old_size,
                                             size_t new_size) {
    DC_ASSERT(new_size > 0, "Cannot allocate zero sized");
    DC_ASSERT(ptr, "Cannot reallocate a null pointer");
    DC_ASSUME(old_size > 0, "Could never have allocated zero sized");

    if (new_size < old_size) {
        // JUSTIFY: We poison the memory we will no longer use.
        //  - When reallocating by reducing allocation size, msan does not automatically poison the
        //    bytes that are no longer part of the allocation
        //  - We need this in order to pass alloc fuzz tests: we want the strong property that
        //  deallocate memory is poisoned.
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                              (char const*)ptr + new_size, old_size - new_size);
    }

    void* new_ptr = realloc(ptr, new_size);
    DC_ASSERT(new_ptr != NULL, "Standard allocator failed to realloc");

    if (new_size > old_size) {
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                              (char*)new_ptr + old_size, new_size - old_size);
    }

    return new_ptr;
}

PUBLIC static void NS(stdalloc, deallocate)(stdalloc_ref /* ref */, void* ptr, size_t size) {
    DC_ASSUME(ptr);

    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, ptr, size);

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

PUBLIC static void NS(stdalloc, debug)(stdalloc const* self, dc_debug_fmt fmt, FILE* stream) {
    DC_ASSUME(self);
    (void)fmt;
    fprintf(stream, "stdalloc@%p { }", (void*)self);
}

PUBLIC static void NS(stdalloc, delete)(stdalloc* self) { DC_ASSUME(self); }

DC_TRAIT_ALLOC(stdalloc);
