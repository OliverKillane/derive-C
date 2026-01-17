#pragma once
#include <unordered_set>
#include <set>

#include <derive-cpp/meta/labels.hpp>

#include <derive-c/container/set/swiss/includes.h>

template <typename Item, size_t(*item_hash)(Item const*)> struct Swiss {
    LABEL_ADD(derive_c_swiss);
#define EXPAND_IN_STRUCT
#define ITEM Item
#define ITEM_HASH item_hash
#define NAME Self
#include <derive-c/container/set/swiss/template.h>
};

template <typename Item, size_t(*item_hash)(Item const*)> struct StdUnorderedSet {
    LABEL_ADD(stl_unordered_set);

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

    using Self_item_t = Item;
    using Self = std::set<Item>;
};
