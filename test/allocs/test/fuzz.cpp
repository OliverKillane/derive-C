#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-c/alloc/std.h>

#include <derive-c/alloc/test/includes.h>

struct SutNS {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#include <derive-c/alloc/test/template.h>
};

namespace {
RC_GTEST_PROP(AllocTest, Fuzz, ()) {
    SutModel model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(stdalloc_get_ref()));
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<Malloc<SutNS>, Calloc<SutNS>, ReallocLarger<SutNS>,
                                          ReallocSmaller<SutNS>, Free<SutNS>>());
}
} // namespace
