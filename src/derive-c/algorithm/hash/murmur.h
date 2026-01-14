/// @brief MurmurHash3 implementation, copied (with love) from [Austin Appleby's
/// implementation](https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.h)
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <derive-c/core/prelude.h>
#include <derive-c/core/std/reflect.h>

DC_INLINE uint32_t PRIV(murmur_rotl32)(uint32_t x, int8_t r) { return (x << r) | (x >> (32 - r)); }

DC_INLINE uint64_t PRIV(murmur_rotl64)(uint64_t x, int8_t r) { return (x << r) | (x >> (64 - r)); }

DC_INLINE uint32_t PRIV(murmur_getblock32)(const uint8_t* p, int32_t i) {
    uint32_t out;
    memcpy(&out, p + ((ptrdiff_t)i * 4), sizeof(out));
    return out;
}

DC_INLINE uint64_t PRIV(murmur_getblock64)(const uint8_t* p, int32_t i) {
    uint64_t out;
    memcpy(&out, p + ((ptrdiff_t)i * 8), sizeof(out));
    return out;
}

DC_INLINE uint32_t PRIV(murmur_fmix32)(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

DC_INLINE uint64_t PRIV(murmur_fmix64)(uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdLLU;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53LLU;
    k ^= k >> 33;

    return k;
}

PUBLIC static inline void dc_MurmurHash3_x86_32(const void* key, int32_t len, uint32_t seed,
                                                void* out) {
    const uint8_t* data = (const uint8_t*)key;
    const int32_t nblocks = len / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    // body

    const uint8_t* blocks = data + (ptrdiff_t)(nblocks * 4);

    for (int i = -nblocks; i; i++) {
        uint32_t k1 = PRIV(murmur_getblock32)(blocks, i);

        k1 *= c1;
        k1 = PRIV(murmur_rotl32)(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = PRIV(murmur_rotl32)(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    // tail

    const uint8_t* tail = (const uint8_t*)(data + (ptrdiff_t)(nblocks * 4));

    uint32_t k1 = 0;

    switch (len & 3) {
    case 3:
        k1 ^= ((uint32_t)tail[2]) << 16;
        [[fallthrough]];
    case 2:
        k1 ^= ((uint32_t)tail[1]) << 8;
        [[fallthrough]];
    case 1:
        k1 ^= ((uint32_t)tail[0]);
        k1 *= c1;
        k1 = PRIV(murmur_rotl32)(k1, 15);
        k1 *= c2;
        h1 ^= k1;
        [[fallthrough]];
    default:
    };

    // finalization

    h1 ^= (uint32_t)len;

    h1 = PRIV(murmur_fmix32)(h1);

    *(uint32_t*)out = h1;
}

PUBLIC static inline void dc_MurmurHash3_x86_128(const void* key, const int32_t len, uint32_t seed,
                                                 void* out) {
    const uint8_t* data = (const uint8_t*)key;
    const int32_t nblocks = len / 16;

    uint32_t h1 = seed;
    uint32_t h2 = seed;
    uint32_t h3 = seed;
    uint32_t h4 = seed;

    const uint32_t c1 = 0x239b961b;
    const uint32_t c2 = 0xab0e9789;
    const uint32_t c3 = 0x38b34ae5;
    const uint32_t c4 = 0xa1e38b93;

    // body

    const uint8_t* blocks = data + (ptrdiff_t)(nblocks * 16);

    for (int i = -nblocks; i; i++) {
        uint32_t k1 = PRIV(murmur_getblock32)(blocks, (i * 4) + 0);
        uint32_t k2 = PRIV(murmur_getblock32)(blocks, (i * 4) + 1);
        uint32_t k3 = PRIV(murmur_getblock32)(blocks, (i * 4) + 2);
        uint32_t k4 = PRIV(murmur_getblock32)(blocks, (i * 4) + 3);

        k1 *= c1;
        k1 = PRIV(murmur_rotl32)(k1, 15);
        k1 *= c2;
        h1 ^= k1;

        h1 = PRIV(murmur_rotl32)(h1, 19);
        h1 += h2;
        h1 = h1 * 5 + 0x561ccd1b;

        k2 *= c2;
        k2 = PRIV(murmur_rotl32)(k2, 16);
        k2 *= c3;
        h2 ^= k2;

        h2 = PRIV(murmur_rotl32)(h2, 17);
        h2 += h3;
        h2 = h2 * 5 + 0x0bcaa747;

        k3 *= c3;
        k3 = PRIV(murmur_rotl32)(k3, 17);
        k3 *= c4;
        h3 ^= k3;

        h3 = PRIV(murmur_rotl32)(h3, 15);
        h3 += h4;
        h3 = h3 * 5 + 0x96cd1c35;

        k4 *= c4;
        k4 = PRIV(murmur_rotl32)(k4, 18);
        k4 *= c1;
        h4 ^= k4;

        h4 = PRIV(murmur_rotl32)(h4, 13);
        h4 += h1;
        h4 = h4 * 5 + 0x32ac3b17;
    }

    // tail

    const uint8_t* tail = (const uint8_t*)(data + (ptrdiff_t)(nblocks * 16));

    uint32_t k1 = 0;
    uint32_t k2 = 0;
    uint32_t k3 = 0;
    uint32_t k4 = 0;

    switch (len & 15) {
    case 15:
        k4 ^= ((uint32_t)tail[14]) << 16;
        [[fallthrough]];
    case 14:
        k4 ^= ((uint32_t)tail[13]) << 8;
        [[fallthrough]];
    case 13:
        k4 ^= ((uint32_t)tail[12]) << 0;
        k4 *= c4;
        k4 = PRIV(murmur_rotl32)(k4, 18);
        k4 *= c1;
        h4 ^= k4;

        [[fallthrough]];
    case 12:
        k3 ^= ((uint32_t)tail[11]) << 24;
        [[fallthrough]];
    case 11:
        k3 ^= ((uint32_t)tail[10]) << 16;
        [[fallthrough]];
    case 10:
        k3 ^= ((uint32_t)tail[9]) << 8;
        [[fallthrough]];
    case 9:
        k3 ^= ((uint32_t)tail[8]) << 0;
        k3 *= c3;
        k3 = PRIV(murmur_rotl32)(k3, 17);
        k3 *= c4;
        h3 ^= k3;

        [[fallthrough]];
    case 8:
        k2 ^= ((uint32_t)tail[7]) << 24;
        [[fallthrough]];
    case 7:
        k2 ^= ((uint32_t)tail[6]) << 16;
        [[fallthrough]];
    case 6:
        k2 ^= ((uint32_t)tail[5]) << 8;
        [[fallthrough]];
    case 5:
        k2 ^= ((uint32_t)tail[4]) << 0;
        k2 *= c2;
        k2 = PRIV(murmur_rotl32)(k2, 16);
        k2 *= c3;
        h2 ^= k2;

        [[fallthrough]];
    case 4:
        k1 ^= ((uint32_t)tail[3]) << 24;
        [[fallthrough]];
    case 3:
        k1 ^= ((uint32_t)tail[2]) << 16;
        [[fallthrough]];
    case 2:
        k1 ^= ((uint32_t)tail[1]) << 8;
        [[fallthrough]];
    case 1:
        k1 ^= ((uint32_t)tail[0]) << 0;
        k1 *= c1;
        k1 = PRIV(murmur_rotl32)(k1, 15);
        k1 *= c2;
        h1 ^= k1;
        [[fallthrough]];
    default:
    };

    // finalization

    h1 ^= (uint32_t)len;
    h2 ^= (uint32_t)len;
    h3 ^= (uint32_t)len;
    h4 ^= (uint32_t)len;

    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;

    h1 = PRIV(murmur_fmix32)(h1);
    h2 = PRIV(murmur_fmix32)(h2);
    h3 = PRIV(murmur_fmix32)(h3);
    h4 = PRIV(murmur_fmix32)(h4);

    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;

    ((uint32_t*)out)[0] = h1;
    ((uint32_t*)out)[1] = h2;
    ((uint32_t*)out)[2] = h3;
    ((uint32_t*)out)[3] = h4;
}

PUBLIC static inline void dc_MurmurHash3_x64_128(const void* key, const int32_t len,
                                                 const uint32_t seed, void* out) {
    const uint8_t* data = (const uint8_t*)key;
    const int32_t nblocks = len / 16;

    uint64_t h1 = seed;
    uint64_t h2 = seed;

    const uint64_t c1 = 0x87c37b91114253d5LLU;
    const uint64_t c2 = 0x4cf5ad432745937fLLU;

    // body

    const uint8_t* blocks = data;

    for (int32_t i = 0; i < nblocks; i++) {
        uint64_t k1 = PRIV(murmur_getblock64)(blocks, (i * 2) + 0);
        uint64_t k2 = PRIV(murmur_getblock64)(blocks, (i * 2) + 1);

        k1 *= c1;
        k1 = PRIV(murmur_rotl64)(k1, 31);
        k1 *= c2;
        h1 ^= k1;

        h1 = PRIV(murmur_rotl64)(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = PRIV(murmur_rotl64)(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        h2 = PRIV(murmur_rotl64)(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    // tail

    const uint8_t* tail = (const uint8_t*)(data + (ptrdiff_t)(nblocks * 16));

    uint64_t k1 = 0;
    uint64_t k2 = 0;

    switch (len & 15) {
    case 15:
        k2 ^= ((uint64_t)tail[14]) << 48;
        [[fallthrough]];
    case 14:
        k2 ^= ((uint64_t)tail[13]) << 40;
        [[fallthrough]];
    case 13:
        k2 ^= ((uint64_t)tail[12]) << 32;
        [[fallthrough]];
    case 12:
        k2 ^= ((uint64_t)tail[11]) << 24;
        [[fallthrough]];
    case 11:
        k2 ^= ((uint64_t)tail[10]) << 16;
        [[fallthrough]];
    case 10:
        k2 ^= ((uint64_t)tail[9]) << 8;
        [[fallthrough]];
    case 9:
        k2 ^= ((uint64_t)tail[8]) << 0;
        k2 *= c2;
        k2 = PRIV(murmur_rotl64)(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        [[fallthrough]];
    case 8:
        k1 ^= ((uint64_t)tail[7]) << 56;
        [[fallthrough]];
    case 7:
        k1 ^= ((uint64_t)tail[6]) << 48;
        [[fallthrough]];
    case 6:
        k1 ^= ((uint64_t)tail[5]) << 40;
        [[fallthrough]];
    case 5:
        k1 ^= ((uint64_t)tail[4]) << 32;
        [[fallthrough]];
    case 4:
        k1 ^= ((uint64_t)tail[3]) << 24;
        [[fallthrough]];
    case 3:
        k1 ^= ((uint64_t)tail[2]) << 16;
        [[fallthrough]];
    case 2:
        k1 ^= ((uint64_t)tail[1]) << 8;
        [[fallthrough]];
    case 1:
        k1 ^= ((uint64_t)tail[0]) << 0;
        k1 *= c1;
        k1 = PRIV(murmur_rotl64)(k1, 31);
        k1 *= c2;
        h1 ^= k1;
        [[fallthrough]];
    default:
    };

    // finalization

    h1 ^= (uint64_t)len;
    h2 ^= (uint64_t)len;

    h1 += h2;
    h2 += h1;

    h1 = PRIV(murmur_fmix64)(h1);
    h2 = PRIV(murmur_fmix64)(h2);

    h1 += h2;
    h2 += h1;

    ((uint64_t*)out)[0] = h1;
    ((uint64_t*)out)[1] = h2;
}

PUBLIC static inline size_t dc_murmurhash(const void* key, int32_t len, uint32_t seed) {
#if SIZE_MAX == UINT64_MAX
    // 64-bit platform: use the x64 128-bit variant, return lower 64 bits
    uint64_t out128[2];
    dc_MurmurHash3_x64_128(key, len, seed, out128);
    return (size_t)out128[0];
#elif SIZE_MAX == UINT32_MAX
    // 32-bit platform: use the x86 32-bit variant
    uint32_t out32;
    dc_MurmurHash3_x86_32(key, len, seed, &out32);
    return (size_t)out32;
#else
    #error "Unsupported size_t width"
#endif
}

/// Using [MurmurHash3's
/// finalizer](https://github.com/aappleby/smhasher/blob/0ff96f7835817a27d0487325b6c16033e2992eb5/src/MurmurHash3.cpp#L81)
/// for integer hashing.
// JUSTIFY: Casting signed to unsigned. This is just a hash, and signed->unsigned is not UB.
#define MURMURHASH_3_FMIx64(type, ...)                                                             \
    PUBLIC static size_t type##_hash_murmurhash3(type const* key) {                                \
        DC_STATIC_ASSERT(sizeof(type) <= sizeof(uint64_t),                                         \
                         "MurmurHash3 only supports up to 64-bit integers");                       \
        return (size_t)PRIV(murmur_fmix64)((uint64_t)(*key));                                      \
    }

DC_INT_REFLECT(MURMURHASH_3_FMIx64)

#undef MURMURHASH_3_FMIx64

// clang-format off
#define STRING_SIZES(_apply) \
    _apply(1)                \
    _apply(2)                \
    _apply(3)                \
    _apply(4)                \
    _apply(5)                \
    _apply(6)                \
    _apply(7)                \
    _apply(8)
// clang-format on

#define MURMURHASH_DEFAULT_SEED 0x9747b28c

#define MURMURHASH_STRING_FIXED_SIZE(size)                                                         \
    PUBLIC static size_t dc_murmur_hash_string_##size(const char str[size]) {                      \
        return dc_murmurhash(str, size, MURMURHASH_DEFAULT_SEED);                                  \
    }

STRING_SIZES(MURMURHASH_STRING_FIXED_SIZE)

PUBLIC static inline size_t dc_murmur_hash_string(const char* str) {
    return dc_murmurhash(str, (int)strlen(str), MURMURHASH_DEFAULT_SEED);
}

#undef MURMURHASH_STRING_FIXED_SIZE
#undef MURMURHASH_DEFAULT_SEED
#undef STRING_SIZES
