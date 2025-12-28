#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/map/staticlinear/includes.h>

template <ObjectType Key, ObjectType Value> struct SutObjects {
#define CAPACITY 16
#define KEY Key
#define KEY_EQ Key::equality_
#define KEY_DELETE Key::delete_
#define KEY_CLONE Key::clone_
#define VALUE Value
#define VALUE_CLONE Value::clone_
#define VALUE_DELETE Value::delete_
#define NAME Sut
#include <derive-c/container/map/staticlinear/template.h>
};

namespace {

template <typename SutNS> struct InsertIfCapacity : Insert<SutNS> {
    using Base = Insert<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    void checkPreconditions(const Model& s) const override {
        Insert<SutNS>::checkPreconditions(s);
        RC_PRE(s.size() < SutNS::Sut_capacity());
    }
};

template <typename SutNS> void Test() {
    SutModel<SutNS> model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new());
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<InsertIfCapacity<SutNS>, InsertIfCapacity<SutNS>,
                                          InsertIfCapacity<SutNS>, InsertIfCapacity<SutNS>,
                                          Write<SutNS>, Remove<SutNS>, DeleteEntry<SutNS>,
                                          DuplicateInsert<SutNS>>());
}

RC_GTEST_PROP(StaticLinearSmall, Fuzz, ()) {
    Test<SutObjects<Primitive<uint8_t>, Primitive<uint8_t>>>();
}
RC_GTEST_PROP(StaticLinearMediumMedium, Fuzz, ()) {
    Test<SutObjects<Primitive<size_t>, Primitive<size_t>>>();
}
RC_GTEST_PROP(StaticLinearComplexEmpty, Fuzz, ()) { Test<SutObjects<Complex, Empty>>(); }
RC_GTEST_PROP(StaticLinearComplexComplex, Fuzz, ()) { Test<SutObjects<Complex, Complex>>(); }

} // namespace
