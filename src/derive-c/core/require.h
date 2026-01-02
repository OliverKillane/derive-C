#pragma once
#include <derive-c/core/namespace.h>

#if defined __cplusplus
    #include <type_traits> // NOLINT(misc-include-cleaner)
    #define DC_REQUIRE_METHOD_EXPR(ret, obj, name, args)                                           \
        std::is_same_v<decltype(+NS(obj, name)), ret(*) args>
    #define DC_REQUIRE_CONSTANT_TYPE_EXPR(obj, name, Type)                                         \
        std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(NS(obj, name))>>, Type>
#else
    #define DC_REQUIRE_METHOD_EXPR(ret, obj, name, args)                                           \
        _Generic(NS(obj, name), ret(*) args: 1, default: 0)
    #define DC_REQUIRE_CONSTANT_TYPE_EXPR(obj, name, Type)                                         \
        _Generic((NS(obj, name)), Type: 1, default: 0)
#endif

#define DC_REQUIRE_METHOD(ret, obj, name, args)                                                    \
    DC_STATIC_ASSERT(DC_REQUIRE_METHOD_EXPR(ret, obj, name, args),                                 \
                     "Method " #obj "." #name " must exist with type " #ret " (*)" #args)

// JUSTIFY: +1 on sizeof
//  - We use the sizeof to enforce the type exists
//  - For zero sized types, we need the expression to not be zero.
#define DC_REQUIRE_TYPE(obj, name) DC_STATIC_ASSERT(sizeof(NS(obj, name)) + 1)

#define DC_REQUIRE_CONSTANT_TYPE(obj, name, type)                                                  \
    DC_STATIC_ASSERT(DC_REQUIRE_CONSTANT_TYPE_EXPR(obj, name, type),                               \
                     "Method " #obj "." #name " must exist and be of type " #type)

#define DC_REQUIRE_CONSTANT(obj, name)                                                             \
    DC_STATIC_ASSERT(sizeof(NS(obj, name)), "Constant " #obj "." #name " must exist")
