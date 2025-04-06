#include <derive-c/core.h>
#include <stdbool.h>

#define DERIVE_ORD_MEMBER_GT(t, n) || NAME(t, gt)(&self_1->n, &self_2->n)
#define DERIVE_ORD_MEMBER_LT(t, n) || NAME(t, lt)(&self_1->n, &self_2->n)

#define DERIVE_ORD(ID)                                                                             \
    bool NAME(ID, gt)(ID const* self_1, ID const* self_2) {                                        \
        return false NAME(ID, REFLECT)(DERIVE_ORD_MEMBER_GT);                                      \
    }                                                                                              \
    bool NAME(ID, lt)(ID const* self_1, ID const* self_2) {                                        \
        return false NAME(ID, REFLECT)(DERIVE_ORD_MEMBER_LT);                                      \
    }
