#pragma once
#include <derive-c/core/object/trait.h>
#include <derive-c/core/prelude.h>
#include <derive-c/utils/iterator/trait.h>

#define TRAIT_VECTOR(SELF)                                                                         \
    REQUIRE_TYPE(SELF, item_t);                                                                    \
    REQUIRE_TYPE(SELF, index_t);                                                                   \
    REQUIRE_METHOD(NS(SELF, item_t) const*, SELF, read, (SELF const*, NS(SELF, index_t)));         \
    REQUIRE_METHOD(NS(SELF, item_t) const*, SELF, try_read, (SELF const*, NS(SELF, index_t)));     \
    REQUIRE_METHOD(NS(SELF, item_t)*, SELF, write, (SELF*, NS(SELF, index_t)));                    \
    REQUIRE_METHOD(NS(SELF, item_t)*, SELF, try_write, (SELF*, NS(SELF, index_t)));                \
    REQUIRE_METHOD(void, SELF, remove_at, (SELF*, NS(SELF, index_t), NS(SELF, index_t)));          \
    REQUIRE_METHOD(bool, SELF, try_pop, (SELF*, NS(SELF, item_t)*));                               \
    REQUIRE_METHOD(NS(SELF, item_t), SELF, pop, (SELF*));                                          \
    REQUIRE_METHOD(NS(SELF, item_t)*, SELF, push, (SELF*, NS(SELF, item_t)));                      \
    REQUIRE_METHOD(NS(SELF, index_t), SELF, size, (SELF const*));                                  \
    TRAIT_ITERABLE(SELF);                                                                          \
    TRAIT_DELETABLE(SELF);                                                                         \
    TRAIT_CLONEABLE(SELF);
