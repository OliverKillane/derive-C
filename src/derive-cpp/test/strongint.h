#pragma once
#include <cstddef>
#include <functional>

template <typename Tag, typename T> struct StrongInteger {
    T value;

    bool operator==(const StrongInteger& other) const { return value == other.value; }
    bool operator<(const StrongInteger& other) const { return value < other.value; }
};

namespace std {
template <typename Tag, typename T> struct hash<StrongInteger<Tag, T>> {
    std::size_t operator()(const StrongInteger<Tag, T>& id) const noexcept {
        return std::hash<T>{}(id.value);
    }
};
} // namespace std
