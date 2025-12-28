#include <cstdint>

#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/vector/dynamic/includes.h>

template <typename Int> struct SutPrimitive_3_8 {
#define EXPAND_IN_STRUCT
#define ITEM Int
#define INPLACE_CAPACITY 10
#define NAME Sut
#include <derive-c/container/vector/dynamic/template.h>
};

template <typename Item> struct SutObject {
#define EXPAND_IN_STRUCT
#define ITEM_CLONE Item::clone_
#define ITEM_DELETE Item::delete_
#define ITEM Item
#define INPLACE_CAPACITY 10
#define NAME Sut
#include <derive-c/container/vector/dynamic/template.h>
};

namespace {

namespace {
template <typename SutNS> void TestVector(SutWrapper<SutNS> sutWrapper) {
    SutModel<SutNS> model;

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<Push<SutNS>, Push<SutNS>, Push<SutNS>, Write<SutNS>,
                                          TryInsertAt<SutNS>, RemoveAt<SutNS>, Pop<SutNS>>());
}
} // namespace

RC_GTEST_PROP(VectorDynamicSmall, Fuzz, ()) {
    using SutNS = SutPrimitive_3_8<uint8_t>;
    TestVector(SutWrapper<SutNS>(SutNS::Sut_new_with_capacity(4, stdalloc_get())));
}

RC_GTEST_PROP(VectorDynamicMedium, Fuzz, ()) {
    using SutNS = SutPrimitive_3_8<size_t>;
    TestVector(SutWrapper<SutNS>(SutNS::Sut_new_with_capacity(1, stdalloc_get())));
}

RC_GTEST_PROP(VectorDynamicEmpty, Fuzz, ()) {
    using SutNS = SutObject<Empty>;
    TestVector(SutWrapper<SutNS>(SutNS::Sut_new_with_capacity(1, stdalloc_get())));
}

RC_GTEST_PROP(VectorDynamicComplex, Fuzz, ()) {
    using SutNS = SutObject<Complex>;
    TestVector(SutWrapper<SutNS>(SutNS::Sut_new_with_capacity(1, stdalloc_get())));
}

} // namespace
