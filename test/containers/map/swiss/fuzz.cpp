
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"
#include "../../objects.hpp"

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
        rc::state::gen::execOneOfWithArgs<
            Insert<SutNS>, Insert<SutNS>, Insert<SutNS>, Insert<SutNS>, ExtendCapacity<SutNS>,
            Write<SutNS>, Remove<SutNS>, DeleteEntry<SutNS>, DuplicateInsert<SutNS>>());
}

RC_GTEST_PROP(SwissSmall, Fuzz, ()) { Test<SutObjects<Primitive<uint8_t>, Primitive<uint8_t>>>(); }
RC_GTEST_PROP(SwissMediumMedium, Fuzz, ()) {
    Test<SutObjects<Primitive<size_t>, Primitive<size_t>>>();
}
RC_GTEST_PROP(SwissComplexEmpty, Fuzz, ()) { Test<SutObjects<Complex, Empty>>(); }
RC_GTEST_PROP(SwissComplexComplex, Fuzz, ()) { Test<SutObjects<Complex, Complex>>(); }

} // namespace