#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <derive-c/core/attributes.h>
#include <derive-c/core/panic.h>
#include <derive-c/core/compiler.h>

// JUSTIFY: Macro rather than function
// - So this can be used in static asserts
#define DC_MATH_IS_POWER_OF_2(x) ((x) != 0 && ((x) & ((x) - 1)) == 0)

#if defined DC_GENERIC_KEYWORD_SUPPORTED
    #define DC_MATH_MSB_INDEX(x)                                                                   \
        ((x) == 0 ? 0                                                                              \
                  : _Generic((x),                                                                  \
                 uint8_t: (7u - (uint32_t)__builtin_clz((uint32_t)((x)) << 24u)),                  \
                 uint16_t: (15u - (uint32_t)__builtin_clz((uint32_t)((x)) << 16u)),                \
                 uint32_t: (31u - (uint32_t)__builtin_clz((uint32_t)(x))),                         \
                 uint64_t: (63u - (uint32_t)__builtin_clzll((uint64_t)(x)))))
#else
    #include <bit>
    #include <type_traits>

namespace dc::math {

template <typename T> constexpr unsigned msb_index(T x) noexcept {
    static_assert(std::is_unsigned_v<T>, "DC_MATH_MSB_INDEX requires an unsigned integer type");

    if (x == 0) {
        return 0U;
    }

    constexpr unsigned bits = sizeof(T) * 8U;
    return bits - 1U - std::countl_zero(x);
}

} // namespace dc::math

    #define DC_MATH_MSB_INDEX(x) ::dc::math::msb_index(x)
#endif

static DC_INLINE DC_CONST size_t dc_math_next_power_of_2(size_t x) {
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

static size_t DC_INLINE DC_CONST dc_math_modulus_power_of_2_capacity(size_t index,
                                                                     size_t capacity) {
    DC_ASSUME(DC_MATH_IS_POWER_OF_2(capacity));
    return index & (capacity - 1);
}

static bool DC_INLINE DC_CONST dc_math_is_aligned_pow2(const void* ptr, unsigned alignment) {
    DC_ASSUME(DC_MATH_IS_POWER_OF_2(alignment));
    return ((uintptr_t)ptr & (alignment - 1)) == 0;
}
