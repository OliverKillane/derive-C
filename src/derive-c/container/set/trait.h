#pragma once

#include <derive-c/core/prelude.h>

#define DC_TRAIT_SET(SELF)                                                                         \
    DC_REQUIRE_TYPE(SELF, item_t);                                                                 \
    DC_REQUIRE_CONSTANT(SELF, max_capacity, size_t);                                               \
    DC_REQUIRE_METHOD(bool, SELF, try_add, (SELF*, NS(SELF, item_t)));                             \
    DC_REQUIRE_METHOD(void, SELF, add, (SELF*, NS(SELF, item_t)));                                 \
    DC_REQUIRE_METHOD(bool, SELF, contains, (SELF const*, NS(SELF, item_t)));                      \
    DC_REQUIRE_METHOD(bool, SELF, try_remove, (SELF*, NS(SELF, item_t)));                          \
    DC_REQUIRE_METHOD(void, SELF, remove, (SELF*, NS(SELF, item_t)));                              \
    DC_REQUIRE_METHOD(size_t, SELF, size, (SELF const*));                                          \
    DC_TRAIT_CONST_ITERABLE(SELF);                                                                 \
    DC_TRAIT_CLONEABLE(SELF);                                                                      \
    DC_TRAIT_DELETABLE(SELF);                                                                      \
    DC_TRAIT_DEBUGABLE(SELF)
