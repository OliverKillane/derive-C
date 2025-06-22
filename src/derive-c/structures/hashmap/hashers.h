/// Interesting literature:
/// - [](https://burtleburtle.net/bob/hash/doobs.html)

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <derive-c/structures/hashmap/murmurhash.h>

/// The worst possible hash, for testing purposes.
#define ALWAYS_COLLIDE(type)                                                                       \
    static size_t hash_always_collide_##type(type const* key) { return 0; }

/// No hashing, just returns the integer value.
/// For most circumstances with a key as a single integer, this is a good option.
#define ID(type)                                                                                   \
    static size_t hash_id_##type(type const* key) {                                                \
        _Static_assert(sizeof(type) <= sizeof(size_t),                                             \
                       "ID hashing only supports up to size_t integers");                          \
        return (size_t)(*key);                                                                     \
    }

/// Using [MurmurHash3's
/// finalizer](https://github.com/aappleby/smhasher/blob/0ff96f7835817a27d0487325b6c16033e2992eb5/src/MurmurHash3.cpp#L81)
/// for integer hashing.
// JUSTIFY: Casting signed to unsigned. This is just a hash, and signed->unsigned is not UB.
#define MURMURHASH_3_FMIx64(type)                                                                  \
    static size_t hash_murmurhash3_##type(type const* key) {                                       \
        _Static_assert(sizeof(type) <= sizeof(uint64_t),                                           \
                       "MurmurHash3 only supports up to 64-bit integers");                         \
        return (size_t)derive_c_fmix64((uint64_t)(*key));                                          \
    }

// clang-format off
#define INT_HASHERS(_apply) \
    _apply(int8_t  )        \
    _apply(int16_t )        \
    _apply(int32_t )        \
    _apply(int64_t )        \
    _apply(uint8_t )        \
    _apply(uint16_t)        \
    _apply(uint32_t)        \
    _apply(uint64_t)
// clang-format on

INT_HASHERS(ALWAYS_COLLIDE)
INT_HASHERS(ID)
INT_HASHERS(MURMURHASH_3_FMIx64)

#undef INT_HASHERS
#undef ALWAYS_COLLIDE
#undef ID
#undef MURMURHASH_3_FMIx64

#define MURMURHASH_DEFAULT_SEED 0x9747b28c

size_t hash_murmurhash_string(const char* str) {
    return derive_c_murmurhash(str, (int)strlen(str), MURMURHASH_DEFAULT_SEED);
}

// clang-format off
#define STRING_SIZES(_apply) \
    _apply(1)                \
    _apply(2)                \
    _apply(3)                \
    _apply(4)                \
    _apply(5)                \
    _apply(6)                \
    _apply(7)                \
    _apply(8) // clang-format on

#define MURMURHASH_STRING_FIXED_SIZE(size)                                                         \
    static size_t hash_murmurhash_string_##size(const char str[size]) {                            \
        return derive_c_murmurhash(str, size, MURMURHASH_DEFAULT_SEED);                            \
    }

STRING_SIZES(MURMURHASH_STRING_FIXED_SIZE)

#undef MURMURHASH_STRING_FIXED_SIZE
#undef MURMURHASH_DEFAULT_SEED

static inline size_t hash_combine(size_t seed, size_t h) {
    // 0x9e3779b97f4a7c15 is 64-bit fractional part of the golden ratio;
    // “+ (seed<<6) + (seed>>2)” mixes seed’s bits
    return seed ^ (h + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}
