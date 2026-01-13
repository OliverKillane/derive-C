
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-cpp/test/rapidcheck_fuzz.hpp>

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/bitset/static/includes.h>

struct SutSmall {
#define EXPAND_IN_STRUCT
#define EXCLUSIVE_END_INDEX 16
#define NAME Sut
#include <derive-c/container/bitset/static/template.h>
};

struct SutMedium {
#define EXPAND_IN_STRUCT
#define EXCLUSIVE_END_INDEX 256
#define NAME Sut
#include <derive-c/container/bitset/static/template.h>
};

namespace {

template <typename SutNS> void Test() {
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new());
    SutModel model;

    rc::state::check(model, sutWrapper,
                     rc::state::gen::execOneOfWithArgs<SetTrue<SutNS>, SetTrue<SutNS>,
                                                       SetFalse<SutNS>, SetOutOfBounds<SutNS>>());
}

// clang-format off
FUZZ(Small,  SutSmall)
FUZZ(Medium, SutMedium)
// clang-format on

} // namespace
