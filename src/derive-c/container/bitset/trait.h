#pragma once

#include <derive-c/core/prelude.h>
#include <stdbool.h>

#define TRAIT_BITSET(SELF)                                                                         \
    REQUIRE_TYPE(SELF, index_t);                                                                   \
    REQUIRE_METHOD(NS(SELF, index_t), SELF, max_index, (void));                                               \
    REQUIRE_METHOD(NS(SELF, index_t), SELF, min_index, (void));                                               \
    REQUIRE_METHOD(bool, SELF, try_set, (SELF*, NS(SELF, index_t), bool));                         \
    REQUIRE_METHOD(void, SELF, set, (SELF*, NS(SELF, index_t), bool));                             \
    REQUIRE_METHOD(bool, SELF, get, (SELF const*, NS(SELF, index_t)));                             \
    REQUIRE_METHOD(NS(SELF, index_t), SELF, size, (SELF const*));                                  \
    TRAIT_ITERABLE(SELF);                                                                          \
    TRAIT_CLONEABLE(SELF);                                                                         \
    TRAIT_DELETABLE(SELF);                                                                         \
    TRAIT_DEBUGABLE(SELF);
