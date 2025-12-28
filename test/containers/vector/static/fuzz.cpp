#include <cstdint>

#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/vector/static/includes.h>

template <ObjectType Item> struct SutObject {
#define EXPAND_IN_STRUCT
#define ITEM Item
#define ITEM_CLONE Item::clone_
#define ITEM_DELETE Item::delete_
#define INPLACE_CAPACITY 10
#define NAME Sut
#include <derive-c/container/vector/static/template.h>
};

namespace {
template <typename SutNS> void Test() {
    SutModel<SutNS> model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new());
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<Push<SutNS>, Push<SutNS>, Push<SutNS>, Write<SutNS>,
                                          TryInsertAt<SutNS>, RemoveAt<SutNS>, Pop<SutNS>>());
}

RC_GTEST_PROP(VectorStaticSmall, Fuzz, ()) { Test<SutObject<Primitive<uint8_t>>>(); }
RC_GTEST_PROP(VectorStaticMedium, Fuzz, ()) { Test<SutObject<Primitive<size_t>>>(); }
RC_GTEST_PROP(VectorStaticEmpty, Fuzz, ()) { Test<SutObject<Empty>>(); }
RC_GTEST_PROP(VectorStaticComplex, Fuzz, ()) { Test<SutObject<Complex>>(); }

} // namespace
