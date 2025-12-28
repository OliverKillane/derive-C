#pragma once

#include <cstdint>
#include <concepts>
#include <cstddef>
#include <compare>
#include <functional>
#include <memory>
#include <ostream>
#include <type_traits>

#include <rapidcheck.h>

// TODO(oliverkillane): Work out a nice way to do raii here
//  - We store these types in both cpp data structures (i.e. the models), and C for the SUT.
//  - Would be nice to get a test guarentee on no leaks, by using an object with owned ptr.
//  - However needs to work for both the model, and for the SUT, with copying and deletion.

/// The interface for types expected by commands.
///  - Keeping a single interface for accessing methods simplifies test code
template <typename T>
concept ObjectType = std::is_object_v<T> && requires {
    { T::clone_(std::declval<T const*>()) } -> std::same_as<T>;
    { T::delete_(std::declval<T*>()) } -> std::same_as<void>;
    { T::equality_(std::declval<T const*>(), std::declval<T const*>()) } -> std::same_as<bool>;
    { T::hash_(std::declval<T const*>()) } -> std::convertible_to<std::size_t>;
} && requires(T const& a, T const& b, std::ostream& os) {
    { a == b } -> std::convertible_to<bool>;
    { os << a } -> std::same_as<std::ostream&>;
} && requires(T const& x) {
    { std::hash<T>{}(x) } -> std::convertible_to<std::size_t>;
} && requires {
    { rc::Arbitrary<T>::arbitrary() } -> std::same_as<rc::Gen<T>>;
};

template <typename Integer> struct Primitive {
    static_assert(std::is_integral_v<Integer>);
    static_assert(std::is_trivially_copyable_v<Integer>);

    Integer v{};

    friend bool operator==(Primitive const& lhs, Primitive const& rhs) { return lhs.v == rhs.v; }

    auto operator<=>(Primitive const&) const = default;

    static Primitive clone_(Primitive const* self) { return Primitive{self ? self->v : Integer{}}; }

    static void delete_(Primitive* self) { (void)self; }

    static bool equality_(Primitive const* lhs, Primitive const* rhs) {
        if (lhs == nullptr || rhs == nullptr)
            return lhs == rhs;
        return lhs->v == rhs->v;
    }

    static std::size_t hash_(Primitive const* self) noexcept {
        // JUSTIFY: A bad hash to increase collisions inside tests
        constexpr size_t SMALL_MOD = 1000;
        if (self->v % 2 == 0) {
            return SMALL_MOD + (self->v % SMALL_MOD);
        }
        return 0;
    }

    friend std::ostream& operator<<(std::ostream& os, Primitive const& p) {
        if constexpr (std::is_same_v<Integer, std::uint8_t> ||
                      std::is_same_v<Integer, std::int8_t> || std::is_same_v<Integer, std::byte>) {
            return os << "Primitive{" << +p.v << "}";
        } else {
            return os << "Primitive{" << p.v << "}";
        }
    }
};

namespace rc {

template <typename Integer> struct Arbitrary<Primitive<Integer>> {
    static Gen<Primitive<Integer>> arbitrary() {
        return gen::map(gen::arbitrary<Integer>(), [](Integer x) { return Primitive<Integer>{x}; });
    }
};

} // namespace rc

namespace std {

template <typename Integer> struct hash<Primitive<Integer>> {
    size_t operator()(Primitive<Integer> const& self) const noexcept {
        return Primitive<Integer>::hash_(&self);
    }
};

} // namespace std

struct Empty {
    auto operator<=>(const Empty&) const = default;

    static Empty clone_(Empty const* self) {
        (void)self;
        return Empty{};
    }

    static void delete_(Empty* self) { (void)self; }

    static bool equality_(Empty const* lhs, Empty const* rhs) { return *lhs == *rhs; }

    static std::size_t hash_(Empty const* self) noexcept {
        (void)self;
        return 0;
    }

    friend std::ostream& operator<<(std::ostream& os, Empty const& self) {
        (void)self;
        return os << "Empty{}";
    }
};

namespace rc {

template <> struct Arbitrary<Empty> {
    static Gen<Empty> arbitrary() { return gen::just(Empty{}); }
};

} // namespace rc

namespace std {

template <> struct hash<Empty> {
    size_t operator()(Empty const& self) const noexcept { return Empty::hash_(&self); }
};

} // namespace std

struct Complex {
    uint8_t a = 0;
    size_t b = 0;
    uint8_t c = 0;

    static bool equality_(Complex const* lhs, Complex const* rhs) { return *lhs == *rhs; }

    auto operator<=>(Complex const&) const = default;

    static Complex clone_(Complex const* self) { return *self; }

    static void delete_(Complex* self) { (void)self; }

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
        os << "Complex{a=" << +v.a << ", b=" << v.b << ", c=" << v.c << "}";
        return os;
    }
};

static_assert(sizeof(Complex) > sizeof(uint8_t) + sizeof(size_t) + sizeof(uint8_t));

namespace rc {

template <> struct Arbitrary<Complex> {
    static Gen<Complex> arbitrary() {
        return rc::gen::build<Complex>(
            rc::gen::set(&Complex::a, rc::gen::arbitrary<std::uint8_t>()),
            rc::gen::set(&Complex::b, rc::gen::arbitrary<std::size_t>()),
            rc::gen::set(&Complex::c, rc::gen::arbitrary<std::uint8_t>()));
    }
};

} // namespace rc

namespace std {

template <> struct hash<Complex> {
    size_t operator()(Complex const& self) const noexcept { return Complex::hash_(&self); }
};

} // namespace std
