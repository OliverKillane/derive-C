#include <derive-c/core.h>

#define DERIVE_CLONE_MEMBER(t, n) .n = NAME(t, clone)(&self->n),

// Given you have define a reflect for an ID, the struct can be defined.
// ```c
// #define Foo_REFLECT(F) \
//     F(int, x) \
//     F(int, y)
//
// DERIVE_STRUCT(Foo)
// ```
#define DERIVE_CLONE(ID) \
ID NAME(ID, clone)(ID const* self) { \
    return (ID){ \ 
        NAME(ID, REFLECT)(DERIVE_CLONE_MEMBER) \
    }; \
}
