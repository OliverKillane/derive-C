#include <derive-c/core/helpers.h>
#include <stdbool.h>

#define DERIVE_ORD_MEMBER_GT(t, n) || NS(t, gt)(&self_1->n, &self_2->n)
#define DERIVE_ORD_MEMBER_LT(t, n) || NS(t, lt)(&self_1->n, &self_2->n)

#define DERIVE_ORD(ID)                                                                             \
    bool NS(ID, gt)(ID const* self_1, ID const* self_2) {                                          \
        return false NS(ID, REFLECT)(DERIVE_ORD_MEMBER_GT);                                        \
    }                                                                                              \
    bool NS(ID, lt)(ID const* self_1, ID const* self_2) {                                          \
        return false NS(ID, REFLECT)(DERIVE_ORD_MEMBER_LT);                                        \
    }
