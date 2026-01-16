#pragma once

#include <derive-c/core/prelude.h>

#define DC_TRAIT_QUEUE(SELF)                                                                       \
    DC_REQUIRE_TYPE(SELF, item_t);                                                                 \
    DC_REQUIRE_METHOD(bool, SELF, empty, (SELF const*));                                           \
    DC_REQUIRE_METHOD(size_t, SELF, size, (SELF const*));                                          \
    DC_REQUIRE_METHOD(bool, SELF, empty, (SELF const*));                                           \
    DC_REQUIRE_METHOD(void, SELF, push_front, (SELF*, NS(SELF, item_t)));                          \
    DC_REQUIRE_METHOD(void, SELF, push_back, (SELF*, NS(SELF, item_t)));                           \
    DC_REQUIRE_METHOD(NS(SELF, item_t) const*, SELF, try_read_from_back, (SELF const*, size_t));   \
    DC_REQUIRE_METHOD(NS(SELF, item_t) const*, SELF, try_read_from_front, (SELF const*, size_t));  \
    DC_REQUIRE_METHOD(NS(SELF, item_t)*, SELF, try_write_from_front, (SELF*, size_t));             \
    DC_REQUIRE_METHOD(NS(SELF, item_t)*, SELF, try_write_from_back, (SELF*, size_t));              \
    DC_REQUIRE_METHOD(NS(SELF, item_t), SELF, pop_front, (SELF*));                                 \
    DC_REQUIRE_METHOD(NS(SELF, item_t), SELF, pop_back, (SELF*));                                  \
    DC_TRAIT_ITERABLE(SELF);                                                                       \
    DC_TRAIT_CONST_ITERABLE(SELF);                                                                 \
    DC_TRAIT_DELETABLE(SELF);                                                                      \
    DC_TRAIT_CLONEABLE(SELF);                                                                      \
    DC_TRAIT_DEBUGABLE(SELF)
