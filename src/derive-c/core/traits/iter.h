#pragma once

#include <derive-c/core/namespace.h>
#include <derive-c/core/require.h>

#define DC_TRAIT_ITER(SELF)                                                                        \
    DC_REQUIRE_TYPE(SELF, item);                                                                   \
    DC_REQUIRE_METHOD(NS(SELF, item), SELF, next, (SELF*));                                        \
    DC_REQUIRE_METHOD(bool, SELF, empty_item, (NS(SELF, item) const*))

#define DC_TRAIT_CONST_ITERABLE(SELF)                                                              \
    DC_REQUIRE_TYPE(SELF, iter_const);                                                             \
    DC_REQUIRE_METHOD(NS(SELF, iter_const), SELF, get_iter_const, (SELF const*));                  \
    DC_TRAIT_ITER(NS(SELF, iter_const))

#define DC_TRAIT_ITERABLE(SELF)                                                                    \
    DC_REQUIRE_TYPE(SELF, iter);                                                                   \
    DC_REQUIRE_METHOD(NS(SELF, iter), SELF, get_iter, (SELF*));                                    \
    DC_TRAIT_ITER(NS(SELF, iter))
