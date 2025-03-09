#include <derive-c/core.h>

#define DERIVE_STRUCT_MEMBER(t, n) t n;

// Given you have define a reflect for an ID, the struct can be defined.
// ```c
// #define Foo_REFLECT(F) \
//     F(int, x) \
//     F(int, y)
//
// DERIVE_STRUCT(Foo)
// ```
#define DERIVE_STRUCT(ID)                                                                          \
    typedef struct {                                                                               \
        NAME(ID, REFLECT)(DERIVE_STRUCT_MEMBER)                                                    \
    } ID;
