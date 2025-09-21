#pragma once
#include <derive-c/core/helpers.h>
#include <derive-c/core/require.h>

#define TRAIT_ITER(SELF)                                                                           \
    REQUIRE_TYPE(SELF, item);                                                                      \
    REQUIRE_METHOD(NS(SELF, item), SELF, next, (SELF*));                                           \
    REQUIRE_METHOD(bool, SELF, empty, (SELF const*));                                              \
    REQUIRE_METHOD(bool, SELF, empty_item, (NS(SELF, item) const*));

#define TRAIT_ITERABLE(SELF)                                                                       \
    REQUIRE_TYPE(SELF, iter);                                                                      \
    REQUIRE_TYPE(SELF, iter_const);                                                                \
    REQUIRE_METHOD(NS(SELF, iter), SELF, get_iter, (SELF*));                                       \
    REQUIRE_METHOD(NS(SELF, iter_const), SELF, get_iter_const, (SELF const*));                     \
    TRAIT_ITER(NS(SELF, iter));                                                                    \
    TRAIT_ITER(NS(SELF, iter_const));
