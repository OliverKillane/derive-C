#include <cstdint>
#include <derive-cpp/test/rapidcheck_panic.hpp>
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/vector/static/includes.h>

template <typename Int> struct SutSmall {
#define EXPAND_IN_STRUCT
#define ITEM Int
#define INPLACE_CAPACITY 10
#define NAME Sut
#include <derive-c/container/vector/static/template.h>
};

namespace containers::vector::staticcapacity {

namespace {
template <typename SutNS> void TestVector(SutWrapper<SutNS> sutWrapper) {
    SutModel<SutNS> model;

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<Push<SutNS>, Push<SutNS>, Push<SutNS>, Write<SutNS>,
                                          TryInsertAt<SutNS>, RemoveAt<SutNS>, Pop<SutNS>>());
}
} // namespace

RC_GTEST_PROP(DequeueSmallCase1, Fuzz, ()) {
    using SutNS = SutSmall<size_t>;
    TestVector(SutWrapper<SutNS>(SutNS::Sut_new()));
}

RC_GTEST_PROP(DequeueSmallCase2, Fuzz, ()) {
    using SutNS = SutSmall<uint8_t>;
    TestVector(SutWrapper<SutNS>(SutNS::Sut_new()));
}

} // namespace containers::vector::staticcapacity
