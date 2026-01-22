#pragma once

#include <cstddef>
#include <cstdint>

// JUSTIFY: Testing with different sized objects
//  - To test differences in behaviour udner different sizes. e.g. Hashmaps storing keys 
//    and values separately, versus adjacent.
template<size_t size>
struct Bytes {
    uint8_t data[size];

    static Bytes clone(Bytes const* self) {
        return *self;
    }

    static void _delete(Bytes* self) {
        (void)self;
    }

    static bool _eq(Bytes const* lhs, Bytes const* rhs) {
        return memcmp(lhs->data, rhs->data, size) == 0;
    }

    // An extremely simple hash function

    static size_t _cast_hash(Bytes const* self) {
        if constexpr (size >= 8) {
            return self->data[0] | (self->data[1] << 8) | (self->data[2] << 16) | (self->data[3] << 24) | (self->data[4] << 32) | (self->data[5] << 40) | (self->data[6] << 48) | (self->data[7] << 56);
        } else if constexpr (size >= 4) {
            return self->data[0] | (self->data[1] << 8) | (self->data[2] << 16) | (self->data[3] << 24);
        } else if constexpr (size >= 2) {
            return self->data[0] | (self->data[1] << 8);
        } else if constexpr (size == 1) {
            return self->data[0];
        } else {
            return 0;
        }
    }
};
