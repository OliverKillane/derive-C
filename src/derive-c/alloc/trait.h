#pragma once

#include <derive-c/core/require.h>

/// The trait for derive-c allocators.
/// Behaviour is more restrictive that the standard allocator, and provides additional information.
///  - The user is trusted to store, or correctly infer (e.g. via types) the size of the allocation
///
/// Additiona poisoning is done.
///  - For msan, we set bytes removed by reallocating smaller. (not done by normal allocator, and
//     requires the old size)
///
/// The behaviour is stricter than the malloc/calloc/realloc/free:
///  - Throws on allocating zero sized
///  - Never returns null pointers from realloc, malloc, calloc
///  - Throws on freeing a nullptr
///  - Throws on reallocating to zero size
#define DC_TRAIT_ALLOC(SELF)                                                                       \
    DC_REQUIRE_METHOD(void*, SELF, allocate_uninit, (NS(SELF, ref), size_t size));                 \
    DC_REQUIRE_METHOD(void*, SELF, allocate_zeroed, (NS(SELF, ref), size_t size));                 \
    DC_REQUIRE_METHOD(void*, SELF, reallocate,                                                     \
                      (NS(SELF, ref), void* ptr, size_t old_size, size_t new_size));               \
    DC_REQUIRE_METHOD(void, SELF, deallocate, (NS(SELF, ref), void* ptr, size_t size));            \
    DC_TRAIT_REFERENCABLE(SELF);                                                                   \
    DC_TRAIT_DELETABLE(SELF);                                                                      \
    DC_TRAIT_DEBUGABLE(SELF)
