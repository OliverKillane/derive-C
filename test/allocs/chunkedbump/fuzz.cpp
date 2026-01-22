#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-cpp/test/rapidcheck_fuzz.hpp>

#include <derive-c/alloc/std.h>

#include <derive-c/alloc/chunkedbump/includes.h>

struct SutSmall {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#define BLOCK_SIZE 256
#include <derive-c/alloc/chunkedbump/template.h>
};

struct SutMedium {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#define BLOCK_SIZE 4096
#include <derive-c/alloc/chunkedbump/template.h>
};

struct SutLarge {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#define BLOCK_SIZE 65536
#include <derive-c/alloc/chunkedbump/template.h>
};

namespace {

template <typename SutNS> void Test() {
    SutModel model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(stdalloc_get_ref()));

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<AllocateUninit<SutNS>, AllocateZeroed<SutNS>,
                                          ReallocateLarger<SutNS>, ReallocateSmaller<SutNS>,
                                          Deallocate<SutNS>, Reset<SutNS>>());
}

// clang-format off
FUZZ(SutSmall,  SutSmall )
FUZZ(SutMedium, SutMedium)
FUZZ(SutLarge,  SutLarge )
// clang-format on
} // namespace
