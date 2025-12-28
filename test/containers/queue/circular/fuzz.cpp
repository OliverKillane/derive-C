#include <cstdint>

#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/queue/circular/includes.h>

template <typename Int> struct SutPrimitive_3_8 {
#define EXPAND_IN_STRUCT
#define ITEM Int
#define NAME Sut
#include <derive-c/container/queue/circular/template.h>
};

template <typename Item> struct SutObject {
#define EXPAND_IN_STRUCT
#define ITEM_CLONE Item::clone_
#define ITEM_DELETE Item::delete_
#define ITEM Item
#define NAME Sut
#include <derive-c/container/queue/circular/template.h>
};

namespace {

namespace {
template <typename SutNS> void TestDequeue(SutWrapper<SutNS> sutWrapper) {
    SutModel<SutNS> model;

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<PushFront<SutNS>, PushFront<SutNS>, PushBack<SutNS>,
                                          PushBack<SutNS>, PopFront<SutNS>, PopBack<SutNS>>());
}
} // namespace

RC_GTEST_PROP(QueueCircularSmall, Fuzz, ()) {
    using SutNS = SutPrimitive_3_8<uint8_t>;
    TestDequeue(SutWrapper<SutNS>(SutNS::Sut_new_with_capacity_for(4, stdalloc_get())));
}

RC_GTEST_PROP(QueueCircularMedium, Fuzz, ()) {
    using SutNS = SutPrimitive_3_8<size_t>;
    TestDequeue(SutWrapper<SutNS>(SutNS::Sut_new_with_capacity_for(1, stdalloc_get())));
}

RC_GTEST_PROP(QueueCircularEmpty, Fuzz, ()) {
    using SutNS = SutObject<Empty>;
    TestDequeue(SutWrapper<SutNS>(SutNS::Sut_new_with_capacity_for(1, stdalloc_get())));
}

RC_GTEST_PROP(QueueCircularComplex, Fuzz, ()) {
    using SutNS = SutObject<Complex>;
    TestDequeue(SutWrapper<SutNS>(SutNS::Sut_new_with_capacity_for(1, stdalloc_get())));
}

} // namespace