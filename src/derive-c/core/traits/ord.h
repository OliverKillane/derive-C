#pragma once

#include <stdbool.h>

#include <derive-c/core/namespace.h>
#include <derive-c/core/std/reflect.h>

#define DC_TRAIT_ORDABLE(SELF)                                                                     \
    DC_REQUIRE_METHOD(bool, SELF, lt, (SELF const*, SELF const*));                                 \
    DC_REQUIRE_METHOD(bool, SELF, gt, (SELF const*, SELF const*))

#define DC_TRAIT_ORDABLE_INVARIANTS(SELF, a, b, c)                                                 \
    DC_TRAIT_ORDABLE(SELF);                                                                        \
    DC_ASSUME(DC_WHEN(NS(SELF, lt)(&a, &b) && NS(SELF, lt)(&b, &c), NS(SELF, lt)(&a, &c)));        \
    DC_ASSUME(DC_WHEN(NS(SELF, gt)(&a, &b) && NS(SELF, gt)(&b, &c), NS(SELF, gt)(&a, &c)));        \
    DC_ASSUME(DC_WHEN(NS(SELF, lt)(&a, &b), NS(SELF, gt)(&b, &a)));                                \
    DC_ASSUME(DC_WHEN(NS(SELF, gt)(&a, &b), NS(SELF, lt)(&b, &a)));                                \
    DC_ASSUME(!NS(SELF, lt)(&a, &a));                                                              \
    DC_ASSUME(!NS(SELF, gt)(&a, &a))

#define _DC_DERIVE_ORD_MEMBER_GT(MEMBER_TYPE, MEMBER_NAME)                                         \
    || NS(MEMBER_TYPE, gt)(&self_1->MEMBER_NAME, &self_2->MEMBER_NAME)
#define _DC_DERIVE_ORD_MEMBER_LT(MEMBER_TYPE, MEMBER_NAME)                                         \
    || NS(MEMBER_TYPE, lt)(&self_1->MEMBER_NAME, &self_2->MEMBER_NAME)

#define DC_DERIVE_ORD(TYPE)                                                                        \
    DC_PUBLIC static bool NS(TYPE, gt)(TYPE const* self_1, TYPE const* self_2) {                   \
        return false NS(TYPE, REFLECT)(_DC_DERIVE_ORD_MEMBER_GT);                                  \
    }                                                                                              \
    DC_PUBLIC static bool NS(TYPE, lt)(TYPE const* self_1, TYPE const* self_2) {                   \
        return false NS(TYPE, REFLECT)(_DC_DERIVE_ORD_MEMBER_LT);                                  \
    }

#define _DC_DERIVE_STD_ORD(TYPE, ...)                                                              \
    DC_PUBLIC static bool NS(TYPE, gt)(TYPE const* self_1, TYPE const* self_2) {                   \
        return *self_1 > *self_2;                                                                  \
    }                                                                                              \
    DC_PUBLIC static bool NS(TYPE, lt)(TYPE const* self_1, TYPE const* self_2) {                   \
        return *self_1 < *self_2;                                                                  \
    }

DC_STD_REFLECT(_DC_DERIVE_STD_ORD)
DC_FLOAT_REFLECT(_DC_DERIVE_STD_ORD)
