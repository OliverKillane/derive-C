
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-cpp/test/rapidcheck_fuzz.hpp>

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/set/swiss/includes.h>

template <ObjectType Item> struct SutObjects {
#define EXPAND_IN_STRUCT
#define ITEM Item
#define ITEM_EQ Item::equality_
#define ITEM_HASH Item::hash_
#define ITEM_DELETE Item::delete_
#define ITEM_CLONE Item::clone_
#define NAME Sut
#include <derive-c/container/set/swiss/template.h>
};

namespace {
template <typename SutNS> void Test() {
    SutModel<SutNS> model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(stdalloc_get_ref()));
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<Add<SutNS>, Add<SutNS>, Add<SutNS>, Add<SutNS>,
                                          ExtendCapacity<SutNS>, Remove<SutNS>, Remove<SutNS>,
                                          DuplicateInsert<SutNS>, AddOverMaxSize<SutNS>>());
}

// clang-format off
FUZZ(ByteByte,       SutObjects<Primitive<uint8_t>>)
FUZZ(ComplexComplex, SutObjects<Complex           >)
// clang-format on

} // namespace
