#pragma once
#include <derive-c/core/helpers.h>
#include <derive-c/core/object/trait.h>
#include <derive-c/core/require.h>
#include <derive-c/utils/iterator/trait.h>

#define TRAIT_QUEUE(SELF)                                                                          \
    REQUIRE_TYPE(SELF, item_t);                                                                    \
    REQUIRE_METHOD(bool, SELF, empty, (SELF const*));                                              \
    REQUIRE_METHOD(size_t, SELF, size, (SELF const*));                                             \
    REQUIRE_METHOD(void, SELF, push_front, (SELF*, NS(SELF, item_t)));                             \
    REQUIRE_METHOD(void, SELF, push_back, (SELF*, NS(SELF, item_t)));                              \
    REQUIRE_METHOD(NS(SELF, item_t), SELF, pop_front, (SELF*));                                    \
    REQUIRE_METHOD(NS(SELF, item_t), SELF, pop_back, (SELF*));                                     \
    TRAIT_ITERABLE(SELF);                                                                          \
    TRAIT_DELETABLE(SELF);                                                                         \
    TRAIT_CLONEABLE(SELF);
