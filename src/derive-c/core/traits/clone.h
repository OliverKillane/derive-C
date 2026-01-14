/// @brief The clone trait.
/// For cloning an object into another owned object.

#pragma once

#include <derive-c/core/attributes.h>
#include <derive-c/core/namespace.h>
#include <derive-c/core/require.h>
#include <derive-c/core/std/reflect.h>

#define DC_TRAIT_CLONEABLE(SELF) DC_REQUIRE_METHOD(SELF, SELF, clone, (SELF const*))

#define DC_COPY_CLONE(SELF) (*(SELF))

#define _DC_DERIVE_CLONE_MEMBER(MEMBER_TYPE, MEMBER_NAME)                                          \
    .MEMBER_NAME = NS(MEMBER_TYPE, clone)(&self->MEMBER_NAME),

#define DC_DERIVE_CLONE(TYPE)                                                                      \
    DC_PUBLIC static TYPE NS(TYPE, clone)(TYPE const* self) {                                      \
        return (TYPE){NS(TYPE, REFLECT)(_DC_DERIVE_CLONE_MEMBER)};                                 \
    }

#define _DC_DERIVE_STD_CLONE(TYPE, ...)                                                            \
    DC_PUBLIC static TYPE NS(TYPE, clone)(TYPE const* self) { return *self; }

DC_STD_REFLECT(_DC_DERIVE_STD_CLONE)
DC_FLOAT_REFLECT(_DC_DERIVE_STD_CLONE)
