#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-cpp/test/rapidcheck_fuzz.hpp>

#include <derive-c/alloc/std.h>

#include <derive-c/alloc/slab/includes.h>

struct SutSmall {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define BLOCK_SIZE 32
#define SLAB_SIZE 256
#define NAME Sut
#include <derive-c/alloc/slab/template.h>
};

struct SutMedium {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define BLOCK_SIZE 64
#define SLAB_SIZE 1024
#define NAME Sut
#include <derive-c/alloc/slab/template.h>
};

struct SutLarge {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define BLOCK_SIZE 128
#define SLAB_SIZE 4096
#define NAME Sut
#include <derive-c/alloc/slab/template.h>
};

namespace {

template <typename SutNS> void Test() {
    SutModel model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(stdalloc_get_ref()));

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<AllocateUninit<SutNS>, AllocateZeroed<SutNS>,
                                          ReallocateLarger<SutNS>, ReallocateSmaller<SutNS>,
                                          Deallocate<SutNS>>());
}

// clang-format off
FUZZ(SutSmall,  SutSmall )
FUZZ(SutMedium, SutMedium)
FUZZ(SutLarge,  SutLarge )
// clang-format on
} // namespace
