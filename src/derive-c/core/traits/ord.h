#pragma once

#include <stdbool.h>

#include <derive-c/core/namespace.h>
#include <derive-c/core/std/reflect.h>

#define TRAIT_ORDABLE(SELF)                                                                        \
    REQUIRE_METHOD(bool, SELF, lt, (SELF const*, SELF const*));                                    \
    REQUIRE_METHOD(bool, SELF, gt, (SELF const*, SELF const*));

#define _DERIVE_ORD_MEMBER_GT(MEMBER_TYPE, MEMBER_NAME)                                            \
    || NS(MEMBER_TYPE, gt)(&self_1->MEMBER_NAME, &self_2->MEMBER_NAME)
#define _DERIVE_ORD_MEMBER_LT(MEMBER_TYPE, MEMBER_NAME)                                            \
    || NS(MEMBER_TYPE, lt)(&self_1->MEMBER_NAME, &self_2->MEMBER_NAME)

#define DERIVE_ORD(TYPE)                                                                           \
    static bool NS(TYPE, gt)(TYPE const* self_1, TYPE const* self_2) {                             \
        return false NS(TYPE, REFLECT)(_DERIVE_ORD_MEMBER_GT);                                     \
    }                                                                                              \
    static bool NS(TYPE, lt)(TYPE const* self_1, TYPE const* self_2) {                             \
        return false NS(TYPE, REFLECT)(_DERIVE_ORD_MEMBER_LT);                                     \
    }

#define _DERIVE_STD_ORD(TYPE, ...)                                                                 \
    static bool NS(TYPE, gt)(TYPE const* self_1, TYPE const* self_2) { return *self_1 > *self_2; } \
    static bool NS(TYPE, lt)(TYPE const* self_1, TYPE const* self_2) { return *self_1 < *self_2; }

STD_REFLECT(_DERIVE_STD_ORD)
