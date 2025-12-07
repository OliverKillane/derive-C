#include <derive-cpp/test/rapidcheck_panic.hpp>
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/map/staticlinear/includes.h>

struct SutSmall {
#define EXPAND_IN_STRUCT
#define CAPACITY 16
#define KEY size_t
#define KEY_EQ(key_1, key_2) (*key_1 == *key_2)
#define VALUE size_t
#define NAME Sut
#include <derive-c/container/map/staticlinear/template.h>
};

struct SutMedium {
#define EXPAND_IN_STRUCT
#define CAPACITY 256
#define KEY size_t
#define KEY_EQ(key_1, key_2) (*key_1 == *key_2)
#define VALUE size_t
#define NAME Sut
#include <derive-c/container/map/staticlinear/template.h>
};

namespace containers::map::staticlinear {

template <typename SutNS> struct InsertIfCapacity : Insert<SutNS> {
    using Base = Insert<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    void checkPreconditions(const Model& s) const override {
        Insert<SutNS>::checkPreconditions(s);
        RC_PRE(s.size() < SutNS::Sut_capacity());
    }
};

RC_GTEST_PROP(StaticLinearSmallTests, Fuzz, ()) {
    using SutNS = SutSmall;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new());
    SutModel<SutNS> model;

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<InsertIfCapacity<SutNS>, InsertIfCapacity<SutNS>,
                                          InsertIfCapacity<SutNS>, InsertIfCapacity<SutNS>,
                                          Write<SutNS>, Remove<SutNS>, DeleteEntry<SutNS>,
                                          DuplicateInsert<SutNS>>());
}

RC_GTEST_PROP(StaticLinearMediumTests, Fuzz, ()) {
    using SutNS = SutMedium;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new());
    SutModel<SutNS> model;

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<InsertIfCapacity<SutNS>, InsertIfCapacity<SutNS>,
                                          InsertIfCapacity<SutNS>, InsertIfCapacity<SutNS>,
                                          Write<SutNS>, Remove<SutNS>, DeleteEntry<SutNS>,
                                          DuplicateInsert<SutNS>>());
}
} // namespace containers::map::staticlinear
