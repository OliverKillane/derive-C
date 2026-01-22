#pragma once
#include <unordered_map>
#include <map>

#include <derive-cpp/meta/labels.hpp>

#include <derive-c/container/map/swiss/includes.h>
#include <derive-c/container/map/ankerl/includes.h>
#include <derive-c/container/map/decomposed/includes.h>
#include <derive-c/container/map/staticlinear/includes.h>

#include <ankerl/unordered_dense.h>
#include <absl/container/flat_hash_map.h>
#include <boost/unordered/unordered_flat_map.hpp>

template <typename Key, typename Value, size_t(*key_hash)(Key const*)> struct Swiss {
    LABEL_ADD(derive_c_swiss);
    static constexpr const char* impl_name = "derive-c/swiss";
#define EXPAND_IN_STRUCT
#define KEY Key
#define KEY_HASH key_hash
#define VALUE Value
#define NAME Self
#include <derive-c/container/map/swiss/template.h>
};

template <typename Key, typename Value, size_t(*key_hash)(Key const*)> struct Ankerl {
    LABEL_ADD(derive_c_ankerl);
    static constexpr const char* impl_name = "derive-c/ankerl";
#define EXPAND_IN_STRUCT
#define KEY Key
#define KEY_HASH key_hash
#define VALUE Value
#define NAME Self
#include <derive-c/container/map/ankerl/template.h>
};

template <typename Key, typename Value, size_t(*key_hash)(Key const*)> struct Decomposed {
    LABEL_ADD(derive_c_decomposed);
    static constexpr const char* impl_name = "derive-c/decomposed";
#define EXPAND_IN_STRUCT
#define KEY Key
#define KEY_HASH key_hash
#define VALUE Value
#define NAME Self
#include <derive-c/container/map/decomposed/template.h>
};

template <typename Key, typename Value> struct StaticLinear {
    LABEL_ADD(derive_c_staticlinear);
    static constexpr const char* impl_name = "derive-c/staticlinear";
#define EXPAND_IN_STRUCT
#define KEY Key
#define VALUE Value
#define CAPACITY 1024
#define NAME Self
#include <derive-c/container/map/staticlinear/template.h>
};

template <typename Key, typename Value, size_t(*key_hash)(Key const*)> struct StdUnorderedMap {
    LABEL_ADD(stl_unordered_map);
    static constexpr const char* impl_name = "std/unordered_map";

    struct KeyHasher {
        size_t operator()(Key const& key) const {
            return key_hash(&key);
        }
    };

    using Self_key_t = Key;
    using Self_value_t = Value;
    using Self = std::unordered_map<Key, Value, KeyHasher>;
};

template <typename Key, typename Value> struct StdMap {
    LABEL_ADD(stl_map);
    static constexpr const char* impl_name = "std/map";

    using Self_key_t = Key;
    using Self_value_t = Value;
    using Self = std::map<Key, Value>;
};

template <typename Key, typename Value, size_t(*key_hash)(Key const*)> struct AnkerlUnorderedDense {
    LABEL_ADD(ankerl_unordered_dense);
    static constexpr const char* impl_name = "ankerl/unordered_dense";

    struct KeyHasher {
        size_t operator()(Key const& key) const {
            return key_hash(&key);
        }
    };

    using Self_key_t = Key;
    using Self_value_t = Value;
    using Self = ankerl::unordered_dense::map<Key, Value, KeyHasher>;
};

template <typename Key, typename Value, size_t(*key_hash)(Key const*)> struct AbseilSwiss {
    LABEL_ADD(abseil_swiss);
    static constexpr const char* impl_name = "abseil/flat_hash_map";

    struct KeyHasher {
        size_t operator()(Key const& key) const {
            return key_hash(&key);
        }
    };

    using Self_key_t = Key;
    using Self_value_t = Value;
    using Self = absl::flat_hash_map<Key, Value, KeyHasher>;
};

template <typename Key, typename Value, size_t(*key_hash)(Key const*)> struct BoostFlat {
    LABEL_ADD(boost_flat);
    static constexpr const char* impl_name = "boost/unordered_flat_map";

    struct KeyHasher {
        size_t operator()(Key const& key) const {
            return key_hash(&key);
        }
    };

    using Self_key_t = Key;
    using Self_value_t = Value;
    using Self = boost::unordered_flat_map<Key, Value, KeyHasher>;
};
