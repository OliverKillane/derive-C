#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <derive-c/core/math.h>
#include <derive-c/core/attributes.h>
#include <derive-c/core/panic.h>

// JUSTIFY: Macro rather than function
// - So this can be used in static asserts
#define DC_MATH_IS_POWER_OF_2(x) ((x) != 0 && ((x) & ((x) - 1)) == 0)

#define DC_MATH_MSB_INDEX(x)                                                                               \
    (x == 0 ? 0                                                                                    \
            : _Generic((x),                                                                        \
             uint8_t: (7u - __builtin_clz((uint32_t)((x)) << 24)),                                 \
             uint16_t: (15u - __builtin_clz((uint32_t)((x)) << 16)),                               \
             uint32_t: (31u - __builtin_clz((uint32_t)(x))),                                       \
             uint64_t: (63u - __builtin_clzll((uint64_t)(x)))))

static INLINE CONST size_t dc_math_next_power_of_2(size_t x) {
    if (x == 0)
        return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
#if SIZE_MAX > 0xFFFFFFFF
    x |= x >> 32; // For 64-bit platforms
#endif
    return x + 1;
}

static size_t INLINE CONST dc_math_modulus_power_of_2_capacity(size_t index, size_t capacity) {
    ASSUME(DC_MATH_IS_POWER_OF_2(capacity));
    return index & (capacity - 1);
}

static bool INLINE CONST dc_math_is_aligned_pow2_exp(const void* ptr, unsigned exp) {
    uintptr_t const mask = (1U << exp) - 1;
    return (((uintptr_t)ptr) & mask) == 0;
}