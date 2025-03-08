#include <derive-c/core.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define DERIVE_EQ_FOR_STD_SIMPLE(t) \
bool NAME(t, eq)(t const* self_1, t const* self_2) { \
    return self_1 == self_2; \
} \ 
t NAME(t, clone)(t const* self) { \
    return *self; \
} \
bool NAME(t, gt)(t const* self_1, t const* self_2) { \
    return *self_1 > *self_2; \
} \
bool NAME(t, lt)(t const* self_1, t const* self_2) { \
    return *self_1 < *self_2; \
}

DERIVE_EQ_FOR_STD_SIMPLE(bool)
DERIVE_EQ_FOR_STD_SIMPLE(char)
DERIVE_EQ_FOR_STD_SIMPLE(int8_t)
DERIVE_EQ_FOR_STD_SIMPLE(uint8_t)
DERIVE_EQ_FOR_STD_SIMPLE(int16_t)
DERIVE_EQ_FOR_STD_SIMPLE(uint16_t)
DERIVE_EQ_FOR_STD_SIMPLE(int32_t)
DERIVE_EQ_FOR_STD_SIMPLE(uint32_t)
DERIVE_EQ_FOR_STD_SIMPLE(int64_t)
DERIVE_EQ_FOR_STD_SIMPLE(uint64_t)
DERIVE_EQ_FOR_STD_SIMPLE(size_t)
DERIVE_EQ_FOR_STD_SIMPLE(float)
DERIVE_EQ_FOR_STD_SIMPLE(double)
