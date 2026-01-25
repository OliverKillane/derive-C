#pragma once

#include <concepts>

#include "object.hpp"

template <typename G>
concept Generator = requires(G& gen, std::size_t seed) {
    typename G::Value;
    { G(seed) } -> std::same_as<G>;
    { gen.next() } -> std::same_as<typename G::Value>;
};

/// Generates a sequence of bytes incrementing in size.
template <std::size_t size> struct BytesSeqGen {
    using Value = Bytes<size>;

    explicit BytesSeqGen(std::size_t seed) noexcept {
        // Repeat the seed's bytes across the entire array (little-endian pattern).
        for (std::size_t i = 0; i < size; ++i) {
            const std::size_t shift = 8U * (i % sizeof(std::size_t));
            mState.data[i] = static_cast<std::uint8_t>((seed >> shift) & 0xFFU);
        }
    }

    Value next() noexcept {
        Value out = mState;
        increment_le();
        return out;
    }

  private:
    Value mState{};

    void increment_le() noexcept {
        for (std::size_t i = 0; i < size; ++i) {
            mState.data[i] = static_cast<std::uint8_t>(mState.data[i] + 1U);
            if (mState.data[i] != 0U)
                return;
        }
    }
};

static_assert(Generator<BytesSeqGen<0>>);

template <Bytes bytes> struct BytesConstGen {
    using Value = decltype(bytes);
    BytesConstGen(size_t /* seed */) noexcept {}
    Value next() const noexcept { return bytes; }
};

static_assert(Generator<BytesConstGen<Bytes<1>{.data = {0}}>>);

/// Sequential integer generator for any integral type
template <std::integral T>
struct SeqGen {
    using Value = T;
    explicit SeqGen(std::size_t seed) noexcept : mState(static_cast<T>(seed)) {}
    Value next() noexcept { return mState++; }

  private:
    T mState;
};

static_assert(Generator<SeqGen<std::uint32_t>>);
static_assert(Generator<SeqGen<std::uint8_t>>);

// Type aliases for convenience
using U32SeqGen = SeqGen<std::uint32_t>;
using U8SeqGen = SeqGen<std::uint8_t>;

/// (More random) 32 bit indices
struct U32XORShiftGen {
    using Value = std::uint32_t;

    // JUSTIFY: Cannot be zero
    //  - Otherwise all subsequent values will be zero.
    explicit U32XORShiftGen(std::size_t seed) noexcept
        : state(seed == 0 ? 0x6d2b79f5U : static_cast<std::uint32_t>(seed)) {}
    Value next() noexcept {
        std::uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

  private:
    std::uint32_t state;
};

static_assert(Generator<U32XORShiftGen>);
