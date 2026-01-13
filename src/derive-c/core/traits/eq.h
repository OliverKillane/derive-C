/// @brief The equality trait
/// For semantic equality on objects.
#pragma once

#include <stdbool.h>
#include <string.h>

#include <derive-c/core/attributes.h>
#include <derive-c/core/namespace.h>
#include <derive-c/core/require.h>
#include <derive-c/core/std/reflect.h>

#define DC_TRAIT_EQABLE(SELF) DC_REQUIRE_METHOD(bool, SELF, eq, (SELF const*, SELF const*))

#define DC_TRAIT_EQABLE_INVARIANTS(SELF, a, b)                                                     \
    DC_TRAIT_EQABLE(SELF);                                                                         \
    DC_ASSUME(DC_WHEN(NS(SELF, eq)(&a, &b) && NS(SELF, eq)(&b, &c), NS(SELF, eq)(&a, &c)));        \
    DC_ASSUME(DC_WHEN(NS(SELF, eq)(&a, &b), NS(SELF, eq)(&b, &a)));                                \
    DC_ASSUME(NS(SELF, eq)(&a, &a))

#define DC_MEM_EQ(SELF_1, SELF_2) (*(SELF_1) == *(SELF_2))

#define _DC_DERIVE_EQ_MEMBER(MEMBER_TYPE, MEMBER_NAME)                                             \
    &&NS(MEMBER_TYPE, eq)(&self_1->MEMBER_NAME, &self_2->MEMBER_NAME)

#define DC_DERIVE_EQ(TYPE)                                                                         \
    static bool NS(TYPE, eq)(TYPE const* self_1, TYPE const* self_2) {                             \
        return true NS(TYPE, REFLECT)(_DC_DERIVE_EQ_MEMBER);                                       \
    }

#define _DC_DERIVE_STD_EQ(TYPE, ...)                                                               \
    static DC_UNUSED bool NS(TYPE, eq)(TYPE const* self_1, TYPE const* self_2) {                   \
        return (*self_1 == *self_2);                                                               \
    }

DC_STD_REFLECT(_DC_DERIVE_STD_EQ)

static bool dc_str_eq(char* const* self_1, char* const* self_2) {
    return strcmp(*self_1, *self_2) == 0;
}

static bool dc_str_const_eq(const char* const* self_1, const char* const* self_2) {
    return strcmp(*self_1, *self_2) == 0;
}
