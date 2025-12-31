#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-c/alloc/null.h>

#include <derive-c/alloc/wrap/includes.h>

struct SutNS {
#define EXPAND_IN_STRUCT
#define ALLOC nullalloc
#define NAME Sut
#include <derive-c/alloc/wrap/template.h>
};

namespace {
RC_GTEST_PROP(AllocNull, Fuzz, ()) {
    SutModel model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(nullalloc_get()));
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<Malloc<SutNS>, Calloc<SutNS>, ReallocLarger<SutNS>,
                                          ReallocSmaller<SutNS>, Free<SutNS>>());
}
} // namespace
