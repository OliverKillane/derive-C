/// @brief The delete trait
/// For deleting an owned object.
#pragma once

#include <derive-c/core/namespace.h>
#include <derive-c/core/require.h>
#include <derive-c/core/std/reflect.h>

#define DC_TRAIT_DELETABLE(SELF) DC_REQUIRE_METHOD(void, SELF, delete, (SELF*));

#define DC_NO_DELETE(SELF) (void)(SELF)

#define _DC_DERIVE_DELETE_MEMBER(MEMBER_TYPE, MEMBER_NAME)                                         \
    NS(MEMBER_TYPE, delete)(&self->MEMBER_NAME),

#define DC_DERIVE_DELETE(TYPE)                                                                     \
    static void NS(TYPE, delete)(TYPE * self) { NS(TYPE, REFLECT)(_DC_DERIVE_DELETE_MEMBER); }

#define _DC_DERIVE_STD_DELETE(TYPE, ...)                                                           \
    static void NS(TYPE, delete)(TYPE * self /* NOLINT(readability-non-const-parameter) */) {      \
        (void)self;                                                                                \
    }

DC_STD_REFLECT(_DC_DERIVE_STD_DELETE)
