#include <cstdint>
#include <derive-cpp/test/rapidcheck_panic.hpp>
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/queue/circular/includes.h>

template <typename Int> struct SutSmall {
#define EXPAND_IN_STRUCT
#define ITEM Int
#define NAME Sut
#include <derive-c/container/queue/circular/template.h>
};

namespace containers::queue::circular {

namespace {
template <typename SutNS> void TestDequeue(SutWrapper<SutNS> sutWrapper) {
    SutModel<SutNS> model;

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<PushFront<SutNS>, PushFront<SutNS>, PushBack<SutNS>,
                                          PushBack<SutNS>, PopFront<SutNS>, PopBack<SutNS>>());
}
} // namespace

RC_GTEST_PROP(DequeueSmallCase1, Fuzz, ()) {
    using SutNS = SutSmall<size_t>;
    TestDequeue(SutWrapper<SutNS>(SutNS::Sut_new_with_capacity_for(4, stdalloc_get())));
}

RC_GTEST_PROP(DequeueSmallCase2, Fuzz, ()) {
    using SutNS = SutSmall<uint8_t>;
    TestDequeue(SutWrapper<SutNS>(SutNS::Sut_new_with_capacity_for(1, stdalloc_get())));
}

} // namespace containers::queue::circular