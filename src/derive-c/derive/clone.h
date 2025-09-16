#include <derive-c/core/helpers.h>

#define DERIVE_CLONE_MEMBER(t, n) .n = NS(t, clone)(&self->n),

// Given you have define a reflect for an ID, the struct can be defined.
// ```c
// #define Foo_REFLECT(F) \
//     F(int, x) \
//     F(int, y)
//
// DERIVE_STRUCT(Foo)
// ```
#define DERIVE_CLONE(ID)                                                                           \
    ID NS(ID, clone)(ID const* self) { return (ID){NS(ID, REFLECT)(DERIVE_CLONE_MEMBER)}; }
