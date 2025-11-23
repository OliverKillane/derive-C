#include <derive-cpp/test/rapidcheck_panic.hpp>
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/arena/chunked/includes.h>

struct SutSmall {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE size_t
#define INDEX_BITS 8
#define BLOCK_INDEX_BITS 2
#include <derive-c/container/arena/chunked/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

inline bool operator==(const SutSmall::Sut_index_t& a, const SutSmall::Sut_index_t& b) {
    return a.index == b.index;
}
inline bool operator==(const SutSmall::Sut_iter_item& a, const SutSmall::Sut_iter_item& b) {
    return a.index == b.index && *a.value == *b.value;
}
inline bool operator==(const SutSmall::Sut_iter_const_item& a,
                       const SutSmall::Sut_iter_const_item& b) {
    return a.index == b.index && *a.value == *b.value;
}

namespace std {
template <> struct hash<SutSmall::Sut_index_t> {
    std::size_t operator()(const SutSmall::Sut_index_t& s) const noexcept {
        return std::hash<uint8_t>{}(s.index);
    }
};
} // namespace std

struct SutMedium {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE uint8_t
#define INDEX_BITS 16
#define BLOCK_INDEX_BITS 8
#include <derive-c/container/arena/chunked/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

inline bool operator==(const SutMedium::Sut_index_t& a, const SutMedium::Sut_index_t& b) {
    return a.index == b.index;
}
inline bool operator==(const SutMedium::Sut_iter_item& a, const SutMedium::Sut_iter_item& b) {
    return a.index == b.index && *a.value == *b.value;
}
inline bool operator==(const SutMedium::Sut_iter_const_item& a,
                       const SutMedium::Sut_iter_const_item& b) {
    return a.index == b.index && *a.value == *b.value;
}

namespace std {
template <> struct hash<SutMedium::Sut_index_t> {
    std::size_t operator()(const SutMedium::Sut_index_t& s) const noexcept {
        return std::hash<uint16_t>{}(s.index);
    }
};
} // namespace std

namespace containers::arena::contiguous {

RC_GTEST_PROP(ChunkedArena, FuzzSmall, ()) {
    SutWrapper<SutSmall> sutWrapper(SutSmall::Sut_new(stdalloc_get()));
    SutModel<SutSmall> sutModel;

    rc::state::check(sutModel, sutWrapper,
                     rc::state::gen::execOneOfWithArgs<Insert<SutSmall>, Insert<SutSmall>,
                                                       Write<SutSmall>, Remove<SutSmall>>());
}

RC_GTEST_PROP(ChunkedArena, FuzzMedium, ()) {
    SutWrapper<SutMedium> sutWrapper(SutMedium::Sut_new(stdalloc_get()));
    SutModel<SutMedium> sutModel;

    rc::state::check(sutModel, sutWrapper,
                     rc::state::gen::execOneOfWithArgs<Insert<SutMedium>, Insert<SutMedium>,
                                                       Write<SutMedium>, Remove<SutMedium>>());
}

} // namespace containers::arena::contiguous
