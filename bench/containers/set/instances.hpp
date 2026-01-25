#pragma once
#include <unordered_set>
#include <set>
#include <type_traits>

#include <derive-cpp/meta/labels.hpp>

#include <derive-c/container/set/swiss/includes.h>

#include <boost/unordered/unordered_flat_set.hpp>

template <typename T>
concept SetCase = requires {
    typename T::Self;
    typename T::Self_item_t;
    { T::impl_name } -> std::convertible_to<const char*>;
};

template <typename Item, size_t(*item_hash)(Item const*)> struct Swiss {
    LABEL_ADD(derive_c_swiss);
    static constexpr const char* impl_name = "derive-c/swiss";
#define EXPAND_IN_STRUCT
#define ITEM Item
#define ITEM_HASH item_hash
#define NAME Self
#include <derive-c/container/set/swiss/template.h>
};

template <typename Item, size_t(*item_hash)(Item const*)> struct StdUnorderedSet {
    LABEL_ADD(stl_unordered_set);
    static constexpr const char* impl_name = "std/unordered_set";

    struct ItemHasher {
        size_t operator()(Item const& item) const {
            return item_hash(&item);
        }
    };

    using Self_item_t = Item;
    using Self = std::unordered_set<Item, ItemHasher>;
};

template <typename Item> struct StdSet {
    LABEL_ADD(stl_set);
    static constexpr const char* impl_name = "std/set";

    using Self_item_t = Item;
    using Self = std::set<Item>;
};

template <typename Item, size_t(*item_hash)(Item const*)> struct BoostFlat {
    LABEL_ADD(boost_flat);
    static constexpr const char* impl_name = "boost/unordered_flat_set";

    struct ItemHasher {
        size_t operator()(Item const& item) const {
            return item_hash(&item);
        }
    };

    using Self_item_t = Item;
    using Self = boost::unordered_flat_set<Item, ItemHasher>;
};
