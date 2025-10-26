#pragma once

#include <derive-c/core/prelude.h>

#define TRAIT_ALLOC(SELF)                                                                          \
    REQUIRE_METHOD(void*, SELF, malloc, (SELF*, size_t));                                          \
    REQUIRE_METHOD(void, SELF, free, (SELF*, void*));                                              \
    REQUIRE_METHOD(void*, SELF, realloc, (SELF*, void*, size_t));                                  \
    REQUIRE_METHOD(void*, SELF, calloc, (SELF*, size_t, size_t));                                  \
    TRAIT_DEBUGABLE(SELF)
