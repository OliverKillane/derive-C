#pragma once

#include <derive-c/core/prelude.h>
#include <stdbool.h>

#define DC_TRAIT_BITSET(SELF)                                                                      \
    DC_REQUIRE_TYPE(SELF, index_t);                                                                \
    DC_REQUIRE_CONSTANT(SELF, max_index, NS(SELF, index_t));                                       \
    DC_REQUIRE_CONSTANT(SELF, min_index, NS(SELF, index_t));                                       \
    DC_REQUIRE_METHOD(bool, SELF, try_set, (SELF*, NS(SELF, index_t), bool));                      \
    DC_REQUIRE_METHOD(void, SELF, set, (SELF*, NS(SELF, index_t), bool));                          \
    DC_REQUIRE_METHOD(bool, SELF, get, (SELF const*, NS(SELF, index_t)));                          \
    DC_REQUIRE_METHOD(size_t, SELF, size, (SELF const*));                                          \
    DC_TRAIT_ITERABLE(SELF);                                                                       \
    DC_TRAIT_CLONEABLE(SELF);                                                                      \
    DC_TRAIT_DELETABLE(SELF);                                                                      \
    DC_TRAIT_DEBUGABLE(SELF);
