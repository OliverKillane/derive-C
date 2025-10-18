#include <derive-c/core/prelude.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define DERIVE_STD_SIMPLE(t)                                                                       \
    bool NS(t, eq)(t const* self_1, t const* self_2) { return *self_1 == *self_2; }                \
    t NS(t, clone)(t const* self) { return *self; }                                                \
    bool NS(t, gt)(t const* self_1, t const* self_2) { return *self_1 > *self_2; }                 \
    bool NS(t, lt)(t const* self_1, t const* self_2) { return *self_1 < *self_2; }

DERIVE_STD_SIMPLE(bool)
DERIVE_STD_SIMPLE(char)
DERIVE_STD_SIMPLE(int8_t)
DERIVE_STD_SIMPLE(uint8_t)
DERIVE_STD_SIMPLE(int16_t)
DERIVE_STD_SIMPLE(uint16_t)
DERIVE_STD_SIMPLE(int32_t)
DERIVE_STD_SIMPLE(uint32_t)
DERIVE_STD_SIMPLE(int64_t)
DERIVE_STD_SIMPLE(uint64_t)
DERIVE_STD_SIMPLE(size_t)
DERIVE_STD_SIMPLE(float)
DERIVE_STD_SIMPLE(double)
