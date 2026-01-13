#pragma once

#include <derive-c/core/prelude.h>

#define DC_TRAIT_ARENA(SELF)                                                                       \
    DC_REQUIRE_TYPE(SELF, index_t);                                                                \
    DC_REQUIRE_TYPE(SELF, value_t);                                                                \
    DC_REQUIRE_METHOD(NS(SELF, index_t), SELF, insert, (SELF*, NS(SELF, value_t)));                \
    DC_REQUIRE_METHOD(NS(SELF, value_t)*, SELF, write, (SELF*, NS(SELF, index_t)));                \
    DC_REQUIRE_METHOD(NS(SELF, value_t)*, SELF, try_write, (SELF*, NS(SELF, index_t)));            \
    DC_REQUIRE_METHOD(NS(SELF, value_t) const*, SELF, read, (SELF const*, NS(SELF, index_t)));     \
    DC_REQUIRE_METHOD(NS(SELF, value_t) const*, SELF, try_read, (SELF const*, NS(SELF, index_t))); \
    DC_REQUIRE_METHOD(bool, SELF, try_remove, (SELF*, NS(SELF, index_t), NS(SELF, value_t)*));     \
    DC_REQUIRE_METHOD(NS(SELF, value_t), SELF, remove, (SELF*, NS(SELF, index_t)));                \
    DC_TRAIT_ITERABLE(SELF);                                                                       \
    DC_TRAIT_CLONEABLE(SELF);                                                                      \
    DC_TRAIT_DELETABLE(SELF);                                                                      \
    DC_TRAIT_DEBUGABLE(SELF)
