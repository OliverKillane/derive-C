#pragma once

#include <derive-c/core/prelude.h>

/// The trait for derive-c allocators.
/// Behaviour is more restrictive that the standard allocator
///  - Throws on allocating zero sized
///  - Never returns null pointers from realloc, malloc, calloc
///  - Throws on freeing a nullptr
#define DC_TRAIT_ALLOC(SELF)                                                                       \
    DC_REQUIRE_METHOD(void*, SELF, malloc, (SELF*, size_t));                                       \
    DC_REQUIRE_METHOD(void, SELF, free, (SELF*, void*));                                           \
    DC_REQUIRE_METHOD(void*, SELF, realloc, (SELF*, void*, size_t));                               \
    DC_REQUIRE_METHOD(void*, SELF, calloc, (SELF*, size_t, size_t));                               \
    DC_TRAIT_REFERENCABLE(SELF);                                                                   \
    DC_TRAIT_DELETABLE(SELF);                                                                      \
    DC_TRAIT_DEBUGABLE(SELF);
