#pragma once

#include <derive-c/core/namespace.h>
#include <derive-c/core/require.h>
#include <derive-c/core/std/reflect.h>

#define DC_TRAIT_CLONEABLE(SELF) DC_REQUIRE_METHOD(SELF, SELF, clone, (SELF const*));

#define COPY_CLONE(SELF) (*(SELF))

#define _DERIVE_CLONE_MEMBER(MEMBER_TYPE, MEMBER_NAME)                                             \
    .MEMBER_NAME = NS(MEMBER_TYPE, clone)(&self->MEMBER_NAME),

#define DERIVE_CLONE(TYPE)                                                                         \
    static TYPE NS(TYPE, clone)(TYPE const* self) {                                                \
        return (TYPE){NS(TYPE, REFLECT)(_DERIVE_CLONE_MEMBER)};                                    \
    }

#define _DERIVE_STD_CLONE(TYPE, ...)                                                               \
    static TYPE NS(TYPE, clone)(TYPE const* self) { return *self; }

STD_REFLECT(_DERIVE_STD_CLONE)
