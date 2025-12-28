#include <cstdint>

#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/queue/deque/includes.h>

template <ObjectType Item> struct SutObject {
#define EXPAND_IN_STRUCT
#define ITEM_CLONE Item::clone_
#define ITEM_DELETE Item::delete_
#define ITEM Item
#define NAME Sut
#include <derive-c/container/queue/deque/template.h>
};

namespace {

namespace {
template <typename SutNS> void Test() {
    SutModel<SutNS> model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new_with_capacity(4, stdalloc_get()));
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<PushFront<SutNS>, PushFront<SutNS>, PushBack<SutNS>,
                                          PushBack<SutNS>, PopFront<SutNS>, PopBack<SutNS>>());
}
} // namespace

RC_GTEST_PROP(QueueDequeueSmall, Fuzz, ()) { Test<SutObject<Primitive<uint8_t>>>(); }
RC_GTEST_PROP(QueueDequeueMedium, Fuzz, ()) { Test<SutObject<Primitive<size_t>>>(); }
RC_GTEST_PROP(QueueDequeueEmpty, Fuzz, ()) { Test<SutObject<Empty>>(); }
RC_GTEST_PROP(QueueDequeueComplex, Fuzz, ()) { Test<SutObject<Complex>>(); }

} // namespace
