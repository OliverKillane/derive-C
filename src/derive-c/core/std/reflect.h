#pragma once

#include <inttypes.h>  // NOLINT(misc-include-cleaner)
#include <stddef.h>    // NOLINT(misc-include-cleaner)
#include <stdint.h>    // NOLINT(misc-include-cleaner)
#include <sys/types.h> // NOLINT(misc-include-cleaner)

/// Reflection on integer types
/// `F(type, print_fmt)`
// clang-format off
#define DC_INT_REFLECT(F, ...)            \
    F(int8_t,   "%" PRId8  , __VA_ARGS__) \
    F(uint8_t,  "%" PRIu8  , __VA_ARGS__) \
    F(int16_t,  "%" PRId16 , __VA_ARGS__) \
    F(uint16_t, "%" PRIu16 , __VA_ARGS__) \
    F(int32_t,  "%" PRId32 , __VA_ARGS__) \
    F(uint32_t, "%" PRIu32 , __VA_ARGS__) \
    F(int64_t,  "%" PRId64 , __VA_ARGS__) \
    F(uint64_t, "%" PRIu64 , __VA_ARGS__)
// clang-format on

/// Reflection on std types
/// `F(type, print_fmt)`
// clang-format off
#define DC_FLOAT_REFLECT(F, ...)              \
    /* floating point types */                \
    F(float,              "%f" , __VA_ARGS__) \
    F(double,             "%lf", __VA_ARGS__)
// clang-format on

// clang-format off
#define DC_STD_REFLECT(F, ...)                \
    /* character types */                     \
    F(char,               "%c" , __VA_ARGS__) \
    DC_INT_REFLECT(F, __VA_ARGS__)
// clang-format on
