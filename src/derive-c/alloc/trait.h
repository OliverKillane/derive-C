#pragma once

#include <derive-c/core/prelude.h>

#define DC_TRAIT_ALLOC(SELF)                                                                       \
    DC_REQUIRE_METHOD(void*, SELF, malloc, (SELF*, size_t));                                       \
    DC_REQUIRE_METHOD(void, SELF, free, (SELF*, void*));                                           \
    DC_REQUIRE_METHOD(void*, SELF, realloc, (SELF*, void*, size_t));                               \
    DC_REQUIRE_METHOD(void*, SELF, calloc, (SELF*, size_t, size_t));                               \
    DC_TRAIT_DEBUGABLE(SELF)
