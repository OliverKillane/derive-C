#include <cstdint>

#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-cpp/test/rapidcheck_fuzz.hpp>

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/vector/dynamic/includes.h>

template <ObjectType Item> struct SutObject {
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
template <typename SutNS> void Test() {
    SutModel<SutNS> model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new_with_capacity(4, stdalloc_get()));
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<Push<SutNS>, Push<SutNS>, Push<SutNS>, Write<SutNS>,
                                          TryInsertAt<SutNS>, RemoveAt<SutNS>, Pop<SutNS>,
                                          TryPushOverCapacity<SutNS>,
                                          TryInsertAtOverCapacity<SutNS>>());
}
} // namespace

// clang-format off
FUZZ(Small,   SutObject<Primitive<uint8_t>>)
FUZZ(Empty,   SutObject<Empty             >)
FUZZ(Complex, SutObject<Complex           >)
// clang-format on
} // namespace
