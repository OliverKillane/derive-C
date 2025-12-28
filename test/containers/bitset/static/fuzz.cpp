
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/bitset/static/includes.h>

struct SutSmall {
#define EXPAND_IN_STRUCT
#define EXCLUSIVE_END_INDEX 16
#define NAME Sut
#include <derive-c/container/bitset/static/template.h>
};

struct SutPrimitive_3_16 {
#define EXPAND_IN_STRUCT
#define EXCLUSIVE_END_INDEX 256
#define NAME Sut
#include <derive-c/container/bitset/static/template.h>
};

namespace containers::bitset::staticbitset {

RC_GTEST_PROP(StaticSmallTests, Fuzz, ()) {
    using SutNS = SutSmall;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new());
    SutModel model;

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<SetTrue<SutNS>, SetTrue<SutNS>, SetFalse<SutNS>>());
}

RC_GTEST_PROP(StaticMediumTests, Fuzz, ()) {
    using SutNS = SutPrimitive_3_16;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new());
    SutModel model;

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<SetTrue<SutNS>, SetTrue<SutNS>, SetFalse<SutNS>>());
}

} // namespace containers::bitset::staticbitset