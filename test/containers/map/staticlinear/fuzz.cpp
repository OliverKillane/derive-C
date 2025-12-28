#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/map/staticlinear/includes.h>

template <typename Key, typename Value> struct SutPrimitive {
#define EXPAND_IN_STRUCT
#define CAPACITY 16
#define KEY Key
#define VALUE Value
#define NAME Sut
#include <derive-c/container/map/staticlinear/template.h>
};

template <typename Key, typename Value> struct SutObjects {
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

template <typename SutNS> void Test(SutWrapper<SutNS> sutWrapper) {
    SutModel<SutNS> model;
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<InsertIfCapacity<SutNS>, InsertIfCapacity<SutNS>,
                                          InsertIfCapacity<SutNS>, InsertIfCapacity<SutNS>,
                                          Write<SutNS>, Remove<SutNS>, DeleteEntry<SutNS>,
                                          DuplicateInsert<SutNS>>());
}

RC_GTEST_PROP(StaticLinearSmall, Fuzz, ()) {
    using SutNS = SutPrimitive<uint32_t, uint8_t>;
    Test(SutWrapper<SutNS>(SutNS::Sut_new()));
}

RC_GTEST_PROP(StaticLinearMedium, Fuzz, ()) {
    using SutNS = SutPrimitive<size_t, size_t>;
    Test(SutWrapper<SutNS>(SutNS::Sut_new()));
}

RC_GTEST_PROP(StaticLinearComplexEmpty, Fuzz, ()) {
    using SutNS = SutObjects<Complex, Empty>;
    Test(SutWrapper<SutNS>(SutNS::Sut_new()));
}

RC_GTEST_PROP(StaticLinearComplexComplex, Fuzz, ()) {
    using SutNS = SutObjects<Complex, Complex>;
    Test(SutWrapper<SutNS>(SutNS::Sut_new()));
}

} // namespace
