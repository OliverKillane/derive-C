#pragma once

#include <cstdint>
#include <cstddef>
#include <compare>
#include <functional>
#include <memory>

#include <rapidcheck.h>

struct Empty {
    auto operator<=>(const Empty&) const = default;

    static Empty clone_(Empty const* self) { (void)self; return Empty{}; }

    static void delete_(Empty* self) { (void)self; }

    static bool equality_(Empty const* lhs, Empty const* rhs) {
        return *lhs == *rhs;
    }

    static std::size_t hash_(Empty const* self) noexcept {
        (void)self;
        return 0;
    }

    friend std::ostream& operator<<(std::ostream& os, Empty const&) {
        return os << "Empty{}";
    }
};

namespace rc {

template <>
struct Arbitrary<Empty> {
    static Gen<Empty> arbitrary() {
        return gen::just(Empty{});
    }
};

} // namespace rc

namespace std {

template <>
struct hash<Empty> {
    size_t operator()(Empty const& self) const noexcept {
        return Empty::hash_(&self);
    }
};

} // namespace std

struct Complex {
    uint8_t a{};
    size_t  b{};
    uint8_t c{};
    Empty*  ptr{};

    friend bool operator==(Complex const& lhs, Complex const& rhs) {
        if (lhs.a != rhs.a || lhs.b != rhs.b || lhs.c != rhs.c)
            return false;

        if (lhs.ptr == nullptr || rhs.ptr == nullptr)
            return lhs.ptr == rhs.ptr;

        return *lhs.ptr == *rhs.ptr;
    }

    static bool equality_(Complex const* lhs, Complex const* rhs) {
        return *lhs == *rhs;
    }

    auto operator<=>(Complex const&) const = default;

    static Complex clone_(Complex const* self) {
        Complex out = *self;
        out.ptr = new Empty(*self->ptr);
        return out;
    }

    static void delete_(Complex* self) {
        delete self->ptr;
        self->ptr = nullptr;
    }

    static std::size_t hash_(Complex const* self) noexcept {
        std::size_t seed = 0;
        auto combine = [&](std::size_t h) noexcept {
            seed ^= h + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        };
        combine(std::hash<std::uint8_t>{}(self->a));
        combine(std::hash<std::size_t>{}(self->b));
        combine(std::hash<std::uint8_t>{}(self->c));
        return seed;
    }

    friend std::ostream& operator<<(std::ostream& os, Complex const& v) {
        os << "Complex{a=" << +v.a
           << ", b=" << v.b
           << ", c=" << +v.c
           << ", ptr=";
        if (v.ptr) os << *v.ptr;
        else os << "null";
        return os << "}";
    }
};


namespace rc {

template <>
struct Arbitrary<Complex> {
    static Gen<Complex> arbitrary() {
        return rc::gen::build<Complex>(
            rc::gen::set(&Complex::a, rc::gen::arbitrary<std::uint8_t>()),
            rc::gen::set(&Complex::b, rc::gen::arbitrary<std::size_t>()),
            rc::gen::set(&Complex::c, rc::gen::arbitrary<std::uint8_t>()),
            rc::gen::set(&Complex::ptr,
                         rc::gen::map(rc::gen::arbitrary<Empty>(),
                                     [](Empty const& e) { return new Empty(e); }))
        );
    }
};

} // namespace rc

namespace std {

template <>
struct hash<Complex> {
    size_t operator()(Complex const& self) const noexcept {
        return Complex::hash_(&self);
    }
};

} // namespace std
