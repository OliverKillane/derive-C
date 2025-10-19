#pragma once
#include <derive-c/core/prelude/macros.h>

#if defined __cplusplus
    #include <type_traits> // NOLINT(misc-include-cleaner)
    #define REQUIRE_METHOD_EXPR(ret, obj, name, args)                                              \
        std::is_same_v<decltype(&NS(obj, name)), ret(*) args>
    #define REQUIRE_CONSTANT_TYPE_EXPR(obj, name, Type)                                            \
        std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(NS(obj, name))>>, Type>
#else
    #define REQUIRE_METHOD_EXPR(ret, obj, name, args)                                              \
        _Generic(&NS(obj, name), ret(*) args: 1, default: 0)
    #define REQUIRE_CONSTANT_TYPE_EXPR(obj, name, Type)                                            \
        _Generic((NS(obj, name)), Type: 1, default: 0)
#endif

#define REQUIRE_METHOD(ret, obj, name, args)                                                       \
    STATIC_ASSERT(REQUIRE_METHOD_EXPR(ret, obj, name, args),                                       \
                  "Method " #obj "." #name " must exist with type " #ret " (*)" #args)

#define REQUIRE_TYPE(obj, name)                                                                    \
    STATIC_ASSERT(sizeof(NS(obj, name)), "Type " #obj "." #name " must exist")

#define REQUIRE_CONSTANT_TYPE(obj, name, type)                                                     \
    STATIC_ASSERT(REQUIRE_CONSTANT_TYPE_EXPR(obj, name, type),                                     \
                  "Method " #obj "." #name " must exist and be of type " #type)

#define REQUIRE_CONSTANT(obj, name)                                                                \
    STATIC_ASSERT(sizeof(NS(obj, name)), "Constant " #obj "." #name " must exist")
