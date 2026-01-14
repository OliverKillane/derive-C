#pragma once

#include <stddef.h>

#include <derive-c/core/namespace.h>

DC_PUBLIC static inline size_t dc_hash_combine(size_t seed, size_t h) {
    // 0x9e3779b97f4a7c15 is 64-bit fractional part of the golden ratio;
    // “+ (seed<<6) + (seed>>2)” mixes seed’s bits
    return seed ^ (h + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}
