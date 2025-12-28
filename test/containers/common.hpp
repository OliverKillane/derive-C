#pragma once

#include <cstdint>
#include <cstddef>
#include <compare>
#include <functional>

#include <rapidcheck.h>

using Small = uint8_t;
using Medium = size_t;

struct Complex {    
    uint8_t a;
    size_t b;
    uint8_t c;

    auto operator<=>(const Complex&) const = default;

};

namespace rc {

template <>
struct Arbitrary<Complex> {
    static Gen<Complex> arbitrary() {
        return gen::build<Complex>(
            gen::set(&Complex::a, gen::arbitrary<uint8_t>()),
            gen::set(&Complex::b, gen::arbitrary<size_t>()),
            gen::set(&Complex::c, gen::arbitrary<uint8_t>())
        );
    }
};

} // namespace rc

namespace std {

template <>
struct hash<Complex> {
    size_t operator()(Complex const& p) const noexcept {
        size_t seed = 0;

        auto combine = [&](size_t h) noexcept {
            seed ^= h + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        };

        combine(std::hash<std::uint8_t>{}(p.a));
        combine(std::hash<std::size_t>{}(p.b));
        combine(std::hash<std::uint8_t>{}(p.c));

        return seed;
    }
};

} // namespace std
