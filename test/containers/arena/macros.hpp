#pragma once

#define INDEX_ITEMS_EQ_HASH(SUT)                                                                   \
    inline bool operator==(const SUT::Sut_index_t& a, const SUT::Sut_index_t& b) {                 \
        return a.index == b.index;                                                                 \
    }                                                                                              \
    inline bool operator==(const SUT::Sut_iter_item& a, const SUT::Sut_iter_item& b) {             \
        return a.index == b.index && *a.value == *b.value;                                         \
    }                                                                                              \
    inline bool operator==(const SUT::Sut_iter_const_item& a, const SUT::Sut_iter_const_item& b) { \
        return a.index == b.index && *a.value == *b.value;                                         \
    }                                                                                              \
    namespace std {                                                                                \
    template <> struct hash<SUT::Sut_index_t> {                                                    \
        std::size_t operator()(const SUT::Sut_index_t& s) const noexcept {                         \
            return std::hash<uint8_t>{}(s.index);                                                  \
        }                                                                                          \
    };                                                                                             \
    } // namespace std