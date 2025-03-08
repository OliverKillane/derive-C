#include <derive-c/core.h>
#include <stdbool.h>

#define DERIVE_EQ_MEMBER(t, n) && NAME(t, eq)(&self_1->n, &self_2->n)

#define DERIVE_EQ(ID) \
bool NAME(ID, eq)(ID const* self_1, ID const* self_2) { \
    return true NAME(ID, REFLECT)(DERIVE_EQ_MEMBER); \
}
