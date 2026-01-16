#pragma once

#include <derive-c/core/prelude.h>

#define DC_TRAIT_MAP(SELF)                                                                         \
    DC_REQUIRE_TYPE(SELF, key_t);                                                                  \
    DC_REQUIRE_TYPE(SELF, value_t);                                                                \
    DC_REQUIRE_CONSTANT(SELF, max_capacity, size_t);                                               \
    DC_REQUIRE_METHOD(NS(SELF, value_t)*, SELF, insert,                                            \
                      (SELF*, NS(SELF, key_t), NS(SELF, value_t)));                                \
    DC_REQUIRE_METHOD(NS(SELF, value_t)*, SELF, try_insert,                                        \
                      (SELF*, NS(SELF, key_t), NS(SELF, value_t)));                                \
    DC_REQUIRE_METHOD(NS(SELF, value_t)*, SELF, write, (SELF*, NS(SELF, key_t)));                  \
    DC_REQUIRE_METHOD(NS(SELF, value_t)*, SELF, try_write, (SELF*, NS(SELF, key_t)));              \
    DC_REQUIRE_METHOD(NS(SELF, value_t) const*, SELF, read, (SELF const*, NS(SELF, key_t)));       \
    DC_REQUIRE_METHOD(NS(SELF, value_t) const*, SELF, try_read, (SELF const*, NS(SELF, key_t)));   \
    DC_REQUIRE_METHOD(bool, SELF, try_remove, (SELF*, NS(SELF, key_t), NS(SELF, value_t)*));       \
    DC_REQUIRE_METHOD(NS(SELF, value_t), SELF, remove, (SELF*, NS(SELF, key_t)));                  \
    DC_REQUIRE_METHOD(void, SELF, delete_entry, (SELF*, NS(SELF, key_t)));                         \
    DC_REQUIRE_METHOD(size_t, SELF, size, (SELF const*));                                          \
    DC_TRAIT_ITERABLE(SELF);                                                                       \
    DC_TRAIT_CONST_ITERABLE(SELF);                                                                 \
    DC_TRAIT_CLONEABLE(SELF);                                                                      \
    DC_TRAIT_DELETABLE(SELF);                                                                      \
    DC_TRAIT_DEBUGABLE(SELF)
