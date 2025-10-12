#pragma once

#include <derive-c/core/helpers.h>
#include <derive-c/core/object/trait.h>
#include <derive-c/core/require.h>
#include <derive-c/utils/iterator/trait.h>

#define TRAIT_ARENA(SELF)                                                                          \
    REQUIRE_TYPE(SELF, index_t);                                                                   \
    REQUIRE_TYPE(SELF, value_t);                                                                   \
    REQUIRE_METHOD(NS(SELF, index_t), SELF, insert, (SELF*, NS(SELF, value_t)));                   \
    REQUIRE_METHOD(NS(SELF, value_t)*, SELF, write, (SELF*, NS(SELF, index_t)));                   \
    REQUIRE_METHOD(NS(SELF, value_t)*, SELF, try_write, (SELF*, NS(SELF, index_t)));               \
    REQUIRE_METHOD(NS(SELF, value_t) const*, SELF, read, (SELF const*, NS(SELF, index_t)));        \
    REQUIRE_METHOD(NS(SELF, value_t) const*, SELF, try_read, (SELF const*, NS(SELF, index_t)));    \
    REQUIRE_METHOD(bool, SELF, try_remove, (SELF*, NS(SELF, index_t), NS(SELF, value_t)*));        \
    REQUIRE_METHOD(NS(SELF, value_t), SELF, remove, (SELF*, NS(SELF, index_t)));                   \
    TRAIT_ITERABLE(SELF);                                                                          \
    TRAIT_CLONEABLE(SELF);                                                                         \
    TRAIT_DELETABLE(SELF);
