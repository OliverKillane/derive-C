#pragma once

#include <derive-c/core/prelude.h>

#define DC_TRAIT_VECTOR(SELF)                                                                         \
    DC_REQUIRE_TYPE(SELF, item_t);                                                                    \
    DC_REQUIRE_TYPE(SELF, index_t);                                                                   \
    DC_REQUIRE_METHOD(NS(SELF, item_t) const*, SELF, read, (SELF const*, NS(SELF, index_t)));         \
    DC_REQUIRE_METHOD(NS(SELF, item_t) const*, SELF, try_read, (SELF const*, NS(SELF, index_t)));     \
    DC_REQUIRE_METHOD(NS(SELF, item_t)*, SELF, write, (SELF*, NS(SELF, index_t)));                    \
    DC_REQUIRE_METHOD(NS(SELF, item_t)*, SELF, try_write, (SELF*, NS(SELF, index_t)));                \
    DC_REQUIRE_METHOD(NS(SELF, item_t)*, SELF, try_insert_at,                                         \
                   (SELF*, NS(SELF, index_t), NS(SELF, item_t) const*, NS(SELF, index_t)));        \
    DC_REQUIRE_METHOD(void, SELF, remove_at, (SELF*, NS(SELF, index_t), NS(SELF, index_t)));          \
    DC_REQUIRE_METHOD(bool, SELF, try_pop, (SELF*, NS(SELF, item_t)*));                               \
    DC_REQUIRE_METHOD(NS(SELF, item_t), SELF, pop, (SELF*));                                          \
    DC_REQUIRE_METHOD(NS(SELF, item_t)*, SELF, push, (SELF*, NS(SELF, item_t)));                      \
    DC_REQUIRE_METHOD(NS(SELF, index_t), SELF, size, (SELF const*));                                  \
    DC_REQUIRE_METHOD(size_t, SELF, max_size, (void));                                                \
    DC_TRAIT_ITERABLE(SELF);                                                                          \
    DC_TRAIT_DELETABLE(SELF);                                                                         \
    DC_TRAIT_CLONEABLE(SELF);                                                                         \
    DC_TRAIT_DEBUGABLE(SELF);
