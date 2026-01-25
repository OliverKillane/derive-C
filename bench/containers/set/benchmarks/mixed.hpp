/// @file mixed_sequential.hpp
/// @brief Mixed operations on a set
///
/// Checking Regressions For:
/// - Mixed insert/remove/contains performance
/// - Hash table behavior under varying load
/// - Deletion and re-insertion patterns
/// - Contains performance for present vs missing keys
/// - Steady-state behavior with weighted operation ratios
///
/// Representative:
/// Not production representative. Weighted action distribution creates
/// specific load patterns that may not reflect typical usage.

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "../instances.hpp"
#include "../../../utils/seed.hpp"
#include "../../../utils/generator.hpp"
#include "../../../utils/label.hpp"
#include "../../../utils/range.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>
#include <derive-cpp/meta/unreachable.hpp>

#include <derive-c/algorithm/hash/id.h>

enum class Action : std::uint8_t {
    Insert,
    Remove,
    ContainsPresent,
    ContainsMissing,
};

// Action weights: Insert=10, Remove=5, ContainsPresent=20, ContainsMissing=20
static constexpr std::size_t ACTION_WEIGHT_INSERT = 10;
static constexpr std::size_t ACTION_WEIGHT_REMOVE = 5;
static constexpr std::size_t ACTION_WEIGHT_CONTAINS_PRESENT = 20;
static constexpr std::size_t ACTION_WEIGHT_CONTAINS_MISSING = 20;
static constexpr std::size_t ACTION_WEIGHT_TOTAL =
    ACTION_WEIGHT_INSERT + ACTION_WEIGHT_REMOVE + ACTION_WEIGHT_CONTAINS_PRESENT +
    ACTION_WEIGHT_CONTAINS_MISSING;

/// Generator for weighted actions using deterministic RNG
struct ActionGen {
    using Value = Action;

    explicit ActionGen(std::size_t seed) noexcept
        : mState(seed == 0 ? 0x6d2b79f5U : static_cast<std::uint32_t>(seed)) {}

    Value next() noexcept {
        // Use XOR-shift to generate next random value
        std::uint32_t x = mState;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        mState = x;

        // Weighted selection
        std::uint32_t choice = x % ACTION_WEIGHT_TOTAL;
        if (choice < ACTION_WEIGHT_INSERT) {
            return Action::Insert;
        }
        choice -= ACTION_WEIGHT_INSERT;
        if (choice < ACTION_WEIGHT_REMOVE) {
            return Action::Remove;
        }
        choice -= ACTION_WEIGHT_REMOVE;
        if (choice < ACTION_WEIGHT_CONTAINS_PRESENT) {
            return Action::ContainsPresent;
        }
        // choice >= ACTION_WEIGHT_CONTAINS_PRESENT
        return Action::ContainsMissing;
    }

  private:
    std::uint32_t mState;
};

static_assert(Generator<ActionGen>);

/// Generate a key for contains operations
/// For present keys: returns index in range [0, set_size)
/// For missing keys: returns index >= set_size
template <typename KeyType>
KeyType generate_key(std::size_t index, std::size_t set_size, bool present) {
    if (present) {
        // Key in range [0, set_size)
        return static_cast<KeyType>(index % set_size);
    }
    // Key >= set_size (guaranteed missing)
    return static_cast<KeyType>(set_size + index);
}

template <SetCase NS, typename KeyGen>
void mixed_case_derive_c_swiss(benchmark::State& /* state */, size_t max_n, KeyGen& key_gen,
                                ActionGen& action_gen) {
    typename NS::Self s = NS::Self_new(stdalloc_get_ref());
    std::size_t set_size = 0;

    for (size_t i = 0; i < max_n; i++) {
        Action action = action_gen.next();
        auto key = key_gen.next();

        switch (action) {
        case Action::Insert: {
            NS::Self_add(&s, key);
            set_size++;
            break;
        }
        case Action::Remove: {
            if (set_size > 0) {
                auto lookup_key = generate_key<typename NS::Self_item_t>(i, set_size, true);
                if (NS::Self_contains(&s, lookup_key)) {
                    NS::Self_remove(&s, lookup_key);
                    set_size--;
                }
            }
            break;
        }
        case Action::ContainsPresent: {
            if (set_size > 0) {
                auto lookup_key = generate_key<typename NS::Self_item_t>(i, set_size, true);
                bool found = NS::Self_contains(&s, lookup_key);
                benchmark::DoNotOptimize(found);
            }
            break;
        }
        case Action::ContainsMissing: {
            auto lookup_key = generate_key<typename NS::Self_item_t>(i, set_size, false);
            bool found = NS::Self_contains(&s, lookup_key);
            benchmark::DoNotOptimize(found);
            break;
        }
        }
    }

    NS::Self_delete(&s);
}

template <SetCase Std, typename KeyGen>
void mixed_case_stl_unordered_set(benchmark::State& /* state */, size_t max_n, KeyGen& key_gen,
                                    ActionGen& action_gen) {
    typename Std::Self s;
    std::size_t set_size = 0;

    for (size_t i = 0; i < max_n; i++) {
        Action action = action_gen.next();
        auto key = key_gen.next();

        switch (action) {
        case Action::Insert: {
            auto result = s.insert(key);
            if (result.second) {
                set_size++;
            }
            break;
        }
        case Action::Remove: {
            if (set_size > 0) {
                auto lookup_key = generate_key<typename Std::Self_item_t>(i, set_size, true);
                auto it = s.find(lookup_key);
                if (it != s.end()) {
                    s.erase(it);
                    set_size--;
                }
            }
            break;
        }
        case Action::ContainsPresent: {
            if (set_size > 0) {
                auto lookup_key = generate_key<typename Std::Self_item_t>(i, set_size, true);
                bool found = s.find(lookup_key) != s.end();
                benchmark::DoNotOptimize(found);
            }
            break;
        }
        case Action::ContainsMissing: {
            auto lookup_key = generate_key<typename Std::Self_item_t>(i, set_size, false);
            bool found = s.find(lookup_key) != s.end();
            benchmark::DoNotOptimize(found);
            break;
        }
        }
    }
}

template <SetCase Std, typename KeyGen>
void mixed_case_stl_set(benchmark::State& /* state */, size_t max_n, KeyGen& key_gen,
                         ActionGen& action_gen) {
    typename Std::Self s;
    std::size_t set_size = 0;

    for (size_t i = 0; i < max_n; i++) {
        Action action = action_gen.next();
        auto key = key_gen.next();

        switch (action) {
        case Action::Insert: {
            auto result = s.insert(key);
            if (result.second) {
                set_size++;
            }
            break;
        }
        case Action::Remove: {
            if (set_size > 0) {
                auto lookup_key = generate_key<typename Std::Self_item_t>(i, set_size, true);
                auto it = s.find(lookup_key);
                if (it != s.end()) {
                    s.erase(it);
                    set_size--;
                }
            }
            break;
        }
        case Action::ContainsPresent: {
            if (set_size > 0) {
                auto lookup_key = generate_key<typename Std::Self_item_t>(i, set_size, true);
                bool found = s.find(lookup_key) != s.end();
                benchmark::DoNotOptimize(found);
            }
            break;
        }
        case Action::ContainsMissing: {
            auto lookup_key = generate_key<typename Std::Self_item_t>(i, set_size, false);
            bool found = s.find(lookup_key) != s.end();
            benchmark::DoNotOptimize(found);
            break;
        }
        }
    }
}

template <SetCase Ext, typename KeyGen>
void mixed_case_boost_flat(benchmark::State& /* state */, size_t max_n, KeyGen& key_gen,
                            ActionGen& action_gen) {
    typename Ext::Self s;
    std::size_t set_size = 0;

    for (size_t i = 0; i < max_n; i++) {
        Action action = action_gen.next();
        auto key = key_gen.next();

        switch (action) {
        case Action::Insert: {
            auto result = s.insert(key);
            if (result.second) {
                set_size++;
            }
            break;
        }
        case Action::Remove: {
            if (set_size > 0) {
                auto lookup_key = generate_key<typename Ext::Self_item_t>(i, set_size, true);
                auto it = s.find(lookup_key);
                if (it != s.end()) {
                    s.erase(it);
                    set_size--;
                }
            }
            break;
        }
        case Action::ContainsPresent: {
            if (set_size > 0) {
                auto lookup_key = generate_key<typename Ext::Self_item_t>(i, set_size, true);
                bool found = s.find(lookup_key) != s.end();
                benchmark::DoNotOptimize(found);
            }
            break;
        }
        case Action::ContainsMissing: {
            auto lookup_key = generate_key<typename Ext::Self_item_t>(i, set_size, false);
            bool found = s.find(lookup_key) != s.end();
            benchmark::DoNotOptimize(found);
            break;
        }
        }
    }
}

template <SetCase Impl> void mixed_sequential(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    set_impl_label_with_item<Impl>(state);

    // Only support uint32_t keys
    static_assert(std::is_same_v<typename Impl::Self_item_t, std::uint32_t>,
                  "Item type must be std::uint32_t");
    for (auto _ : state) {
        SeqGen<std::uint32_t> key_gen(SEED);
        ActionGen action_gen(SEED);
        if constexpr (LABEL_CHECK(Impl, derive_c_swiss)) {
            mixed_case_derive_c_swiss<Impl>(state, max_n, key_gen, action_gen);
        } else if constexpr (LABEL_CHECK(Impl, stl_unordered_set)) {
            mixed_case_stl_unordered_set<Impl>(state, max_n, key_gen, action_gen);
        } else if constexpr (LABEL_CHECK(Impl, stl_set)) {
            mixed_case_stl_set<Impl>(state, max_n, key_gen, action_gen);
        } else if constexpr (LABEL_CHECK(Impl, boost_flat)) {
            mixed_case_boost_flat<Impl>(state, max_n, key_gen, action_gen);
        } else {
            static_assert_unreachable<Impl>();
        }
    }

    // Report items processed (each action counts as one item)
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(max_n));
}

// Small sizes for all implementations
#define BENCH_SMALL(...)                                                                          \
    BENCHMARK_TEMPLATE(mixed_sequential, __VA_ARGS__)->Apply(range::exponential<1024>)

// Large sizes for dynamic implementations
#define BENCH_LARGE(...)                                                                          \
    BENCHMARK_TEMPLATE(mixed_sequential, __VA_ARGS__)->Apply(range::exponential<65536>)

BENCH_SMALL(Swiss<std::uint32_t, uint32_t_hash_id>);
BENCH_LARGE(Swiss<std::uint32_t, uint32_t_hash_id>);

BENCH_SMALL(StdUnorderedSet<std::uint32_t, uint32_t_hash_id>);
BENCH_LARGE(StdUnorderedSet<std::uint32_t, uint32_t_hash_id>);

BENCH_SMALL(StdSet<std::uint32_t>);
BENCH_LARGE(StdSet<std::uint32_t>);

BENCH_SMALL(BoostFlat<std::uint32_t, uint32_t_hash_id>);
BENCH_LARGE(BoostFlat<std::uint32_t, uint32_t_hash_id>);

#undef BENCH_SMALL
#undef BENCH_LARGE
