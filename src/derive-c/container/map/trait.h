#pragma once

#include <derive-c/core/helpers.h>
#include <derive-c/core/object/trait.h>
#include <derive-c/core/require.h>
#include <derive-c/utils/iterator/trait.h>

#define TRAIT_MAP(SELF)                                                                            \
    REQUIRE_TYPE(SELF, key_t);                                                                     \
    REQUIRE_TYPE(SELF, value_t);                                                                   \
    REQUIRE_METHOD(NS(SELF, value_t)*, SELF, insert, (SELF*, NS(SELF, key_t), NS(SELF, value_t))); \
    REQUIRE_METHOD(NS(SELF, value_t)*, SELF, try_insert,                                           \
                   (SELF*, NS(SELF, key_t), NS(SELF, value_t)));                                   \
    REQUIRE_METHOD(NS(SELF, value_t)*, SELF, write, (SELF*, NS(SELF, key_t)));                     \
    REQUIRE_METHOD(NS(SELF, value_t)*, SELF, try_write, (SELF*, NS(SELF, key_t)));                 \
    REQUIRE_METHOD(NS(SELF, value_t) const*, SELF, read, (SELF const*, NS(SELF, key_t)));          \
    REQUIRE_METHOD(NS(SELF, value_t) const*, SELF, try_read, (SELF const*, NS(SELF, key_t)));      \
    REQUIRE_METHOD(bool, SELF, try_remove, (SELF*, NS(SELF, key_t), NS(SELF, value_t)*));          \
    REQUIRE_METHOD(NS(SELF, value_t), SELF, remove, (SELF*, NS(SELF, key_t)));                     \
    REQUIRE_METHOD(void, SELF, delete_entry, (SELF*, NS(SELF, key_t)));                            \
    REQUIRE_METHOD(size_t, SELF, size, (SELF const*));                                             \
    TRAIT_ITERABLE(SELF);                                                                          \
    TRAIT_CLONEABLE(SELF);                                                                         \
    TRAIT_DELETABLE(SELF);
