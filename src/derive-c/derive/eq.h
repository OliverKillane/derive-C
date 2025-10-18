#include <derive-c/core/prelude.h>
#include <stdbool.h>

#define DERIVE_EQ_MEMBER(t, n) &&NS(t, eq)(&self_1->n, &self_2->n)

#define DERIVE_EQ(ID)                                                                              \
    bool NS(ID, eq)(ID const* self_1, ID const* self_2) {                                          \
        return true NS(ID, REFLECT)(DERIVE_EQ_MEMBER);                                             \
    }
