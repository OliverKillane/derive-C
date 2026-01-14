#pragma once

#include <stdint.h>

#include <derive-c/core/prelude.h>
#include <derive-c/core/std/reflect.h>

/// FNV-1a 64-bit constants
#define DC_FNV1A_64_OFFSET 14695981039346656037ull
#define DC_FNV1A_64_PRIME 1099511628211ull

/// Hashes a null terminated string
PUBLIC static inline uint64_t dc_fnv1a_str_borrow(const char* s) {
    const unsigned char* p = (const unsigned char*)(s);
    uint64_t h = DC_FNV1A_64_OFFSET;

    for (unsigned char c = *p; c != 0; c = *++p) {
        h ^= (uint64_t)c;
        h *= DC_FNV1A_64_PRIME;
    }
    return h;
}

PUBLIC static inline uint64_t dc_fnv1a_str(char* const* s) { return dc_fnv1a_str_borrow(*s); }

PUBLIC static inline uint64_t dc_fnv1a_str_const(const char* const* s) {
    return dc_fnv1a_str_borrow(*s);
}

PUBLIC static inline uint64_t dc_fnv1a_u64(uint64_t const* v) {
    uint64_t h = DC_FNV1A_64_OFFSET;
    uint64_t x = *v;

    for (int i = 0; i < 8; ++i) {
        h ^= (uint64_t)(x & 0xffU);
        h *= DC_FNV1A_64_PRIME;
        x >>= 8;
    }
    return h;
}

PUBLIC static inline uint64_t dc_fnv1a_u32(uint32_t const* v) {
    uint64_t h = DC_FNV1A_64_OFFSET;
    uint32_t x = *v;

    for (int i = 0; i < 4; ++i) {
        h ^= (uint64_t)(x & 0xffU);
        h *= DC_FNV1A_64_PRIME;
        x >>= 8;
    }
    return h;
}

/// Applying the fnv1a hash for the size of the integer
#define FNV1A_INTEGER(type, ...)                                                                   \
    PUBLIC static size_t type##_hash_fnv1a(type const* key) {                                      \
        DC_STATIC_ASSERT(sizeof(type) <= sizeof(uint64_t),                                         \
                         "fnv integer hashing only supports up to size_t integers");               \
        if (sizeof(type) <= sizeof(uint32_t)) {                                                    \
            uint32_t value = (uint32_t)(*key);                                                     \
            return dc_fnv1a_u32(&value);                                                           \
        }                                                                                          \
        uint64_t value = (uint64_t)(*key);                                                         \
        return dc_fnv1a_u64(&value);                                                               \
    }

DC_INT_REFLECT(FNV1A_INTEGER)

#undef FNV1A_INTEGER
