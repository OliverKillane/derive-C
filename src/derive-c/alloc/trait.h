#pragma once

#include <derive-c/core/prelude.h>

/// The trait for derive-c allocators.
/// Behaviour is more restrictive that the standard allocator
///  - Throws on allocating zero sized
///  - Never returns null pointers from realloc, malloc, calloc
///  - Throws on freeing a nullptr
#define DC_TRAIT_ALLOC(SELF)                                                                       \
    DC_REQUIRE_METHOD(void*, SELF, malloc, (NS(SELF, ref), size_t));                               \
    DC_REQUIRE_METHOD(void, SELF, free, (NS(SELF, ref), void*));                                   \
    DC_REQUIRE_METHOD(void*, SELF, realloc, (NS(SELF, ref), void*, size_t));                       \
    DC_REQUIRE_METHOD(void*, SELF, calloc, (NS(SELF, ref), size_t, size_t));                       \
    DC_TRAIT_REFERENCABLE(SELF);                                                                   \
    DC_TRAIT_DELETABLE(SELF);                                                                      \
    DC_TRAIT_DEBUGABLE(SELF);
