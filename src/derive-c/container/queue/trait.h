#pragma once

#include <derive-c/core/prelude.h>

#define TRAIT_QUEUE(SELF)                                                                          \
    REQUIRE_TYPE(SELF, item_t);                                                                    \
    REQUIRE_METHOD(bool, SELF, empty, (SELF const*));                                              \
    REQUIRE_METHOD(size_t, SELF, size, (SELF const*));                                             \
    REQUIRE_METHOD(bool, SELF, empty, (SELF const*));                                              \
    REQUIRE_METHOD(void, SELF, push_front, (SELF*, NS(SELF, item_t)));                             \
    REQUIRE_METHOD(void, SELF, push_back, (SELF*, NS(SELF, item_t)));                              \
    REQUIRE_METHOD(NS(SELF, item_t) const*, SELF, try_read_from_back, (SELF const*, size_t));          \
    REQUIRE_METHOD(NS(SELF, item_t) const*, SELF, try_read_from_front, (SELF const*, size_t));         \
    REQUIRE_METHOD(NS(SELF, item_t)*, SELF, try_write_from_front, (SELF*, size_t));                    \
    REQUIRE_METHOD(NS(SELF, item_t)*, SELF, try_write_from_back, (SELF*, size_t));                     \
    REQUIRE_METHOD(NS(SELF, item_t), SELF, pop_front, (SELF*));                                    \
    REQUIRE_METHOD(NS(SELF, item_t), SELF, pop_back, (SELF*));                                     \
    TRAIT_ITERABLE(SELF);                                                                          \
    TRAIT_DELETABLE(SELF);                                                                         \
    TRAIT_CLONEABLE(SELF);                                                                         \
    TRAIT_DEBUGABLE(SELF);
