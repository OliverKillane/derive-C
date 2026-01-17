#pragma once
#include <vector>

#include <derive-cpp/meta/labels.hpp>
#include <derive-c/container/vector/dynamic/includes.h>

template <typename Item> struct Dynamic {
    LABEL_ADD(derive_c_dynamic);
#define EXPAND_IN_STRUCT
#define ITEM Item
#define NAME Self
#include <derive-c/container/vector/dynamic/template.h>
};

template <typename Item> struct Hybrid {
    LABEL_ADD(derive_c_hybrid);
#define EXPAND_IN_STRUCT
#define CAPACITY 1024
#define NAME HybridAlloc
#include <derive-c/alloc/hybridstatic/template.h>

#define EXPAND_IN_STRUCT
#define ALLOC HybridAlloc
#define ITEM Item
#define NAME Self
#include <derive-c/container/vector/dynamic/template.h>
};

template <typename Item> struct Static {
    LABEL_ADD(derive_c_static_1024);
#define EXPAND_IN_STRUCT
#define ITEM Item
#define CAPACITY 8096
#define NAME Self
#include <derive-c/container/vector/static/template.h>
};

template <typename Item> struct Std {
    LABEL_ADD(stl_vector);
    using Self_item_t = Item;
    using Self = std::vector<Item>;
};
