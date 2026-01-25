#pragma once
#include <deque>
#include <queue>
#include <type_traits>

#include <derive-cpp/meta/labels.hpp>

#include <derive-c/container/queue/circular/includes.h>
#include <derive-c/container/queue/deque/includes.h>

template <typename T>
concept QueueCase = requires {
    typename T::Self;
    typename T::Self_item_t;
    { T::impl_name } -> std::convertible_to<const char*>;
};

// Circular queue wrapper
template <typename Item> struct Circular {
    LABEL_ADD(derive_c_circular);
    static constexpr const char* impl_name = "derive-c/circular";
#define EXPAND_IN_STRUCT
#define ITEM Item
#define CAPACITY 65536
#define NAME Self
#include <derive-c/container/queue/circular/template.h>
};

// Deque wrapper
template <typename Item> struct Deque {
    LABEL_ADD(derive_c_deque);
    static constexpr const char* impl_name = "derive-c/deque";
#define EXPAND_IN_STRUCT
#define ITEM Item
#define NAME Self
#include <derive-c/container/queue/deque/template.h>
};

// std::deque wrapper
template <typename Item> struct StdDeque {
    LABEL_ADD(stl_deque);
    static constexpr const char* impl_name = "std/deque";

    using Self_item_t = Item;
    using Self = std::deque<Item>;
};

// std::queue wrapper (uses std::deque as container)
template <typename Item> struct StdQueue {
    LABEL_ADD(stl_queue);
    static constexpr const char* impl_name = "std/queue";

    using Self_item_t = Item;
    using Self = std::queue<Item>;
};
