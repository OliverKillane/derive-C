/// @file mixed.hpp
/// @brief Mixed operations on a map
///
/// Checking Regressions For:
/// - Mixed insert/remove/lookup performance
/// - Hash table behavior under varying load
/// - Deletion and re-insertion patterns
/// - Lookup performance for present vs missing keys
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
#include "../../../utils/object.hpp"
#include "../../../utils/label.hpp"
#include "../../../utils/range.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>
#include <derive-c/algorithm/hash/id.h>

#include <derive-cpp/meta/labels.hpp>
#include <derive-cpp/meta/unreachable.hpp>

enum class Action : std::uint8_t {
    Insert,
    Remove,
    LookupPresent,
    LookupMissing,
};

// Action weights: Insert=10, Remove=5, LookupPresent=20, LookupMissing=20
static constexpr std::size_t ACTION_WEIGHT_INSERT = 10;
static constexpr std::size_t ACTION_WEIGHT_REMOVE = 5;
static constexpr std::size_t ACTION_WEIGHT_LOOKUP_PRESENT = 20;
static constexpr std::size_t ACTION_WEIGHT_LOOKUP_MISSING = 20;
static constexpr std::size_t ACTION_WEIGHT_TOTAL =
    ACTION_WEIGHT_INSERT + ACTION_WEIGHT_REMOVE + ACTION_WEIGHT_LOOKUP_PRESENT +
    ACTION_WEIGHT_LOOKUP_MISSING;

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
        if (choice < ACTION_WEIGHT_LOOKUP_PRESENT) {
            return Action::LookupPresent;
        }
        // choice >= ACTION_WEIGHT_LOOKUP_PRESENT
        return Action::LookupMissing;
    }

  private:
    std::uint32_t mState;
};

static_assert(Generator<ActionGen>);

/// Generate a key for lookup operations
/// For present keys: returns index in range [0, map_size)
/// For missing keys: returns index >= map_size
template <typename KeyType>
KeyType generate_key(std::size_t index, std::size_t map_size, bool present) {
    if (present) {
        // Key in range [0, map_size)
        return static_cast<KeyType>(index % map_size);
    }
    // Key >= map_size (guaranteed missing)
    return static_cast<KeyType>(map_size + index);
}

template <MapCase NS, typename KeyGen>
void mixed_case_derive_c(benchmark::State& /* state */, size_t max_n, KeyGen& key_gen,
                          ActionGen& action_gen) {
    typename NS::Self m = NS::Self_new(stdalloc_get_ref());
    std::size_t map_size = 0;

    for (size_t i = 0; i < max_n; i++) {
        Action action = action_gen.next();
        auto key = key_gen.next();

        switch (action) {
        case Action::Insert: {
            typename NS::Self_value_t* placed = nullptr;
            if constexpr (std::is_same_v<typename NS::Self_key_t, typename NS::Self_value_t>) {
                placed = NS::Self_try_insert(&m, key, key);
            } else {
                typename NS::Self_value_t value{};
                placed = NS::Self_try_insert(&m, key, value);
            }
            if (placed != nullptr) {
                map_size++;
            }
            break;
        }
        case Action::Remove: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename NS::Self_key_t>(i, map_size, true);
                auto* value = NS::Self_try_read(&m, lookup_key);
                if (value != nullptr) {
                    NS::Self_remove(&m, lookup_key);
                    map_size--;
                }
            }
            break;
        }
        case Action::LookupPresent: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename NS::Self_key_t>(i, map_size, true);
                auto* value = NS::Self_try_read(&m, lookup_key);
                benchmark::DoNotOptimize(value);
            }
            break;
        }
        case Action::LookupMissing: {
            auto lookup_key = generate_key<typename NS::Self_key_t>(i, map_size, false);
            auto* value = NS::Self_try_read(&m, lookup_key);
            benchmark::DoNotOptimize(value);
            break;
        }
        }
    }

    NS::Self_delete(&m);
}

template <MapCase NS, typename KeyGen>
void mixed_case_derive_c_staticlinear(benchmark::State& /* state */, size_t max_n,
                                       KeyGen& key_gen, ActionGen& action_gen) {
    typename NS::Self m = NS::Self_new();
    std::size_t map_size = 0;

    for (size_t i = 0; i < max_n; i++) {
        Action action = action_gen.next();
        auto key = key_gen.next();

        switch (action) {
        case Action::Insert: {
            typename NS::Self_value_t* placed = nullptr;
            if constexpr (std::is_same_v<typename NS::Self_key_t, typename NS::Self_value_t>) {
                placed = NS::Self_try_insert(&m, key, key);
            } else {
                typename NS::Self_value_t value{};
                placed = NS::Self_try_insert(&m, key, value);
            }
            if (placed != nullptr) {
                map_size++;
            }
            break;
        }
        case Action::Remove: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename NS::Self_key_t>(i, map_size, true);
                auto* value = NS::Self_try_read(&m, lookup_key);
                if (value != nullptr) {
                    NS::Self_remove(&m, lookup_key);
                    map_size--;
                }
            }
            break;
        }
        case Action::LookupPresent: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename NS::Self_key_t>(i, map_size, true);
                auto* value = NS::Self_try_read(&m, lookup_key);
                benchmark::DoNotOptimize(value);
            }
            break;
        }
        case Action::LookupMissing: {
            auto lookup_key = generate_key<typename NS::Self_key_t>(i, map_size, false);
            auto* value = NS::Self_try_read(&m, lookup_key);
            benchmark::DoNotOptimize(value);
            break;
        }
        }
    }

    NS::Self_delete(&m);
}

template <MapCase Std, typename KeyGen>
void mixed_case_stl_unordered_map(benchmark::State& /* state */, size_t max_n, KeyGen& key_gen,
                                    ActionGen& action_gen) {
    typename Std::Self m;
    std::size_t map_size = 0;

    for (size_t i = 0; i < max_n; i++) {
        Action action = action_gen.next();
        auto key = key_gen.next();

        switch (action) {
        case Action::Insert: {
            bool inserted = false;
            if constexpr (std::is_same_v<typename Std::Self_key_t, typename Std::Self_value_t>) {
                auto result = m.insert({key, key});
                inserted = result.second;
            } else {
                typename Std::Self_value_t value{};
                auto result = m.insert({key, value});
                inserted = result.second;
            }
            if (inserted) {
                map_size++;
            }
            break;
        }
        case Action::Remove: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename Std::Self_key_t>(i, map_size, true);
                auto it = m.find(lookup_key);
                if (it != m.end()) {
                    m.erase(it);
                    map_size--;
                }
            }
            break;
        }
        case Action::LookupPresent: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename Std::Self_key_t>(i, map_size, true);
                auto it = m.find(lookup_key);
                benchmark::DoNotOptimize(&it);
            }
            break;
        }
        case Action::LookupMissing: {
            auto lookup_key = generate_key<typename Std::Self_key_t>(i, map_size, false);
            auto it = m.find(lookup_key);
            benchmark::DoNotOptimize(&it);
            break;
        }
        }
    }
}

template <MapCase Std, typename KeyGen>
void mixed_case_stl_map(benchmark::State& /* state */, size_t max_n, KeyGen& key_gen,
                         ActionGen& action_gen) {
    typename Std::Self m;
    std::size_t map_size = 0;

    for (size_t i = 0; i < max_n; i++) {
        Action action = action_gen.next();
        auto key = key_gen.next();

        switch (action) {
        case Action::Insert: {
            bool inserted = false;
            if constexpr (std::is_same_v<typename Std::Self_key_t, typename Std::Self_value_t>) {
                auto result = m.insert({key, key});
                inserted = result.second;
            } else {
                typename Std::Self_value_t value{};
                auto result = m.insert({key, value});
                inserted = result.second;
            }
            if (inserted) {
                map_size++;
            }
            break;
        }
        case Action::Remove: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename Std::Self_key_t>(i, map_size, true);
                auto it = m.find(lookup_key);
                if (it != m.end()) {
                    m.erase(it);
                    map_size--;
                }
            }
            break;
        }
        case Action::LookupPresent: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename Std::Self_key_t>(i, map_size, true);
                auto it = m.find(lookup_key);
                benchmark::DoNotOptimize(&it);
            }
            break;
        }
        case Action::LookupMissing: {
            auto lookup_key = generate_key<typename Std::Self_key_t>(i, map_size, false);
            auto it = m.find(lookup_key);
            benchmark::DoNotOptimize(&it);
            break;
        }
        }
    }
}

template <MapCase Ext, typename KeyGen>
void mixed_case_ankerl_unordered_dense(benchmark::State& /* state */, size_t max_n,
                                         KeyGen& key_gen, ActionGen& action_gen) {
    typename Ext::Self m;
    std::size_t map_size = 0;

    for (size_t i = 0; i < max_n; i++) {
        Action action = action_gen.next();
        auto key = key_gen.next();

        switch (action) {
        case Action::Insert: {
            if constexpr (std::is_same_v<typename Ext::Self_key_t, typename Ext::Self_value_t>) {
                m.insert({key, key});
            } else {
                typename Ext::Self_value_t value{};
                m.insert({key, value});
            }
            map_size++;
            break;
        }
        case Action::Remove: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename Ext::Self_key_t>(i, map_size, true);
                auto it = m.find(lookup_key);
                if (it != m.end()) {
                    m.erase(it);
                    map_size--;
                }
            }
            break;
        }
        case Action::LookupPresent: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename Ext::Self_key_t>(i, map_size, true);
                auto it = m.find(lookup_key);
                benchmark::DoNotOptimize(&it);
            }
            break;
        }
        case Action::LookupMissing: {
            auto lookup_key = generate_key<typename Ext::Self_key_t>(i, map_size, false);
            auto it = m.find(lookup_key);
            benchmark::DoNotOptimize(&it);
            break;
        }
        }
    }
}

template <MapCase Ext, typename KeyGen>
void mixed_case_abseil_swiss(benchmark::State& /* state */, size_t max_n, KeyGen& key_gen,
                               ActionGen& action_gen) {
    typename Ext::Self m;
    std::size_t map_size = 0;

    for (size_t i = 0; i < max_n; i++) {
        Action action = action_gen.next();
        auto key = key_gen.next();

        switch (action) {
        case Action::Insert: {
            if constexpr (std::is_same_v<typename Ext::Self_key_t, typename Ext::Self_value_t>) {
                m.insert({key, key});
            } else {
                typename Ext::Self_value_t value{};
                m.insert({key, value});
            }
            map_size++;
            break;
        }
        case Action::Remove: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename Ext::Self_key_t>(i, map_size, true);
                auto it = m.find(lookup_key);
                if (it != m.end()) {
                    m.erase(it);
                    map_size--;
                }
            }
            break;
        }
        case Action::LookupPresent: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename Ext::Self_key_t>(i, map_size, true);
                auto it = m.find(lookup_key);
                benchmark::DoNotOptimize(&it);
            }
            break;
        }
        case Action::LookupMissing: {
            auto lookup_key = generate_key<typename Ext::Self_key_t>(i, map_size, false);
            auto it = m.find(lookup_key);
            benchmark::DoNotOptimize(&it);
            break;
        }
        }
    }
}

template <MapCase Ext, typename KeyGen>
void mixed_case_boost_flat(benchmark::State& /* state */, size_t max_n, KeyGen& key_gen,
                            ActionGen& action_gen) {
    typename Ext::Self m;
    std::size_t map_size = 0;

    for (size_t i = 0; i < max_n; i++) {
        Action action = action_gen.next();
        auto key = key_gen.next();

        switch (action) {
        case Action::Insert: {
            if constexpr (std::is_same_v<typename Ext::Self_key_t, typename Ext::Self_value_t>) {
                m.insert({key, key});
            } else {
                typename Ext::Self_value_t value{};
                m.insert({key, value});
            }
            map_size++;
            break;
        }
        case Action::Remove: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename Ext::Self_key_t>(i, map_size, true);
                auto it = m.find(lookup_key);
                if (it != m.end()) {
                    m.erase(it);
                    map_size--;
                }
            }
            break;
        }
        case Action::LookupPresent: {
            if (map_size > 0) {
                auto lookup_key = generate_key<typename Ext::Self_key_t>(i, map_size, true);
                auto it = m.find(lookup_key);
                benchmark::DoNotOptimize(&it);
            }
            break;
        }
        case Action::LookupMissing: {
            auto lookup_key = generate_key<typename Ext::Self_key_t>(i, map_size, false);
            auto it = m.find(lookup_key);
            benchmark::DoNotOptimize(&it);
            break;
        }
        }
    }
}

template <MapCase Impl> void mixed(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    set_impl_label_with_key_value<Impl>(state);

    // Only support uint32_t keys
    static_assert(std::is_same_v<typename Impl::Self_key_t, std::uint32_t>,
                  "Key type must be std::uint32_t");
    for (auto _ : state) {
        SeqGen<std::uint32_t> key_gen(SEED);
        ActionGen action_gen(SEED);
        if constexpr (LABEL_CHECK(Impl, derive_c_swiss) || LABEL_CHECK(Impl, derive_c_ankerl) ||
                      LABEL_CHECK(Impl, derive_c_ankerl_small) ||
                      LABEL_CHECK(Impl, derive_c_decomposed)) {
            mixed_case_derive_c<Impl>(state, max_n, key_gen, action_gen);
        } else if constexpr (LABEL_CHECK(Impl, derive_c_staticlinear)) {
            mixed_case_derive_c_staticlinear<Impl>(state, max_n, key_gen, action_gen);
        } else if constexpr (LABEL_CHECK(Impl, stl_unordered_map)) {
            mixed_case_stl_unordered_map<Impl>(state, max_n, key_gen, action_gen);
        } else if constexpr (LABEL_CHECK(Impl, stl_map)) {
            mixed_case_stl_map<Impl>(state, max_n, key_gen, action_gen);
        } else if constexpr (LABEL_CHECK(Impl, ankerl_unordered_dense)) {
            mixed_case_ankerl_unordered_dense<Impl>(state, max_n, key_gen, action_gen);
        } else if constexpr (LABEL_CHECK(Impl, abseil_swiss)) {
            mixed_case_abseil_swiss<Impl>(state, max_n, key_gen, action_gen);
        } else if constexpr (LABEL_CHECK(Impl, boost_flat)) {
            mixed_case_boost_flat<Impl>(state, max_n, key_gen, action_gen);
        } else {
            static_assert_unreachable<Impl>();
        }
    }

    // Report items processed (each action counts as one item)
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(max_n));
}

// Small sizes for StaticLinear (CAPACITY 1024)
#define BENCH_SMALL(...)                                                                          \
    BENCHMARK_TEMPLATE(mixed, __VA_ARGS__)->Apply(range::exponential<1024>)

// Large sizes for dynamic implementations
#define BENCH_LARGE(...)                                                                          \
    BENCHMARK_TEMPLATE(mixed, __VA_ARGS__)->Apply(range::exponential<65536>)

BENCH_LARGE(Swiss<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(Ankerl<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(AnkerlSmall<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(Decomposed<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_SMALL(StaticLinear<std::uint32_t, Bytes<16>>);
BENCH_LARGE(StdUnorderedMap<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(StdMap<std::uint32_t, Bytes<16>>);
BENCH_LARGE(AnkerlUnorderedDense<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(AbseilSwiss<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(BoostFlat<std::uint32_t, Bytes<16>, uint32_t_hash_id>);

#undef BENCH_SMALL
#undef BENCH_LARGE
