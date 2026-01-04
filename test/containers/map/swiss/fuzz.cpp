
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-cpp/test/rapidcheck_fuzz.hpp>

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/map/swiss/includes.h>

template <ObjectType Key, ObjectType Value> struct SutObjects {
#define EXPAND_IN_STRUCT
#define KEY Key
#define KEY_EQ Key::equality_
#define KEY_HASH Key::hash_
#define KEY_DELETE Key::delete_
#define KEY_CLONE Key::clone_
#define VALUE Value
#define VALUE_CLONE Value::clone_
#define VALUE_DELETE Value::delete_
#define NAME Sut
#include <derive-c/container/map/swiss/template.h>
};

namespace {
template <typename SutNS> void Test() {
    SutModel<SutNS> model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(stdalloc_get()));
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<Insert<SutNS>, Insert<SutNS>, Insert<SutNS>,
                                          Insert<SutNS>, ExtendCapacity<SutNS>, Write<SutNS>,
                                          Remove<SutNS>, DeleteEntry<SutNS>, DuplicateInsert<SutNS>,
                                          InsertOverMaxSize<SutNS>>());
}

// clang-format off
FUZZ(ByteByte,       SutObjects<Primitive<uint8_t>, Primitive<uint8_t>>)
FUZZ(ComplexComplex, SutObjects<Complex,            Complex           >)
FUZZ(ComplexEmpty,   SutObjects<Complex,            Empty             >)
// clang-format on

} // namespace