#pragma once
#include <derive-c/core/helpers.h>

#define REQUIRE_METHOD(ret, obj, name, args)                                                       \
    _Static_assert(_Generic(&NS(obj, name), ret(*) args: 1, default: 0),                           \
                   "Method " #obj "." #name " must exist with type " #ret " (*)" #args)

#define REQUIRE_TYPE(obj, name)                                                                    \
    _Static_assert(sizeof(NS(obj, name)), "Type " #obj "." #name " must exist")

#define REQUIRE_CONSTANT_TYPE(obj, name, Type)                                                     \
    _Static_assert(_Generic((NS(obj, name)), Type: 1, default: 0),                                 \
                   "Method " #obj "." #name " must exist and be of type " #Type)

#define REQUIRE_CONSTANT(obj, name)                                                                \
    _Static_assert(sizeof(NS(obj, name)), "Constant " #obj "." #name " must exist")
