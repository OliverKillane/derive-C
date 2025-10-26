#pragma once

#define DERIVE_STRUCT_MEMBER(MEMBER_TYPE, MEMBER_NAME) MEMBER_TYPE MEMBER_NAME;

// Given you have define a reflect for an ID, the struct can be defined.
// ```c
// #define Foo_REFLECT(F, ...) \
//     F(int, x, __VA_ARGS__)  \
//     F(int, y, __VA_ARGS__)
//
// DERIVE_STRUCT(Foo)
// ```
#define DERIVE_STRUCT(TYPE, ...)                                                                   \
    typedef struct {                                                                               \
        NS(TYPE, REFLECT)(DERIVE_STRUCT_MEMBER)                                                    \
    }(TYPE)__VA_ARGS__;
