#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/utils/null_stream.h>

#include <derive-c/alloc/debug/includes.h>

struct SutNS {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#include <derive-c/alloc/debug/template.h>
};

namespace {
RC_GTEST_PROP(AllocTest, Fuzz, ()) {
    FILE* stream = dc_null_stream();
    SutModel model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new("test", stream, stdalloc_get_ref()));
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<AllocateUninit<SutNS>, AllocateZeroed<SutNS>, ReallocateLarger<SutNS>,
                                          ReallocateSmaller<SutNS>, Deallocate<SutNS>>());
    fclose(stream);
}
} // namespace
