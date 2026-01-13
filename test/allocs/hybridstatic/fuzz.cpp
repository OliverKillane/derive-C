#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-cpp/test/rapidcheck_fuzz.hpp>

#include <derive-c/alloc/std.h>

#include <derive-c/alloc/hybridstatic/includes.h>

struct SutUint8Size {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#define CAPACITY 256
#include <derive-c/alloc/hybridstatic/template.h>
};

struct SutUint8Larger {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#define CAPACITY 257
#include <derive-c/alloc/hybridstatic/template.h>
};

struct SutMedium {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#define CAPACITY 4096
#include <derive-c/alloc/hybridstatic/template.h>
};

namespace {

template <typename SutNS> void Test() {
    typename SutNS::Sut_buffer buffer = {};
    SutModel model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(&buffer, stdalloc_get_ref()));

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<AllocateUninit<SutNS>, AllocateZeroed<SutNS>,
                                          ReallocateLarger<SutNS>, ReallocateSmaller<SutNS>,
                                          Deallocate<SutNS>>());
}

// clang-format off
FUZZ(SutUint8Size,   SutUint8Size  )
FUZZ(SutUint8Larger, SutUint8Larger)
FUZZ(SutMedium,      SutMedium     )
// clang-format on
} // namespace
