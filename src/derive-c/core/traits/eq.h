#pragma once

#include <stdbool.h>

#include <derive-c/core/namespace.h>
#include <derive-c/core/require.h>
#include <derive-c/core/std/reflect.h>

#define TRAIT_EQABLE(SELF) REQUIRE_METHOD(bool, SELF, eq, (SELF const*, SELF const*));

#define MEM_EQ(SELF_1, SELF_2) (*(SELF_1) == *(SELF_2))

#define _DERIVE_EQ_MEMBER(MEMBER_TYPE, MEMBER_NAME)                                                \
    &&NS(MEMBER_TYPE, eq)(&self_1->MEMBER_NAME, &self_2->MEMBER_NAME)

#define DERIVE_EQ(TYPE)                                                                            \
    static bool NS(TYPE, eq)(TYPE const* self_1, TYPE const* self_2) {                             \
        return true NS(TYPE, REFLECT)(_DERIVE_EQ_MEMBER);                                          \
    }

#define _DERIVE_STD_EQ(TYPE, ...)                                                                  \
    static bool NS(TYPE, eq)(TYPE const* self_1, TYPE const* self_2) { return *self_1 == *self_2; }

STD_REFLECT(_DERIVE_STD_EQ)
