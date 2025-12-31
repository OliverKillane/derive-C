#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-c/alloc/std.h>

#include <derive-c/alloc/staticbump/includes.h>

struct SutNone {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#define CAPACITY 0
#include <derive-c/alloc/staticbump/template.h>
};

struct SutUint8Size {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#define CAPACITY 256
#include <derive-c/alloc/staticbump/template.h>
};

struct SutUint8Larger {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#define CAPACITY 257
#include <derive-c/alloc/staticbump/template.h>
};

struct SutMedium {
#define EXPAND_IN_STRUCT
#define ALLOC stdalloc
#define NAME Sut
#define CAPACITY 4096
#include <derive-c/alloc/staticbump/template.h>
};

namespace {

template <typename SutNS> void Test() {
    char buffer[SutNS::Sut_capacity] = {};
    SutModel model;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(&buffer));

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<Malloc<SutNS>, Calloc<SutNS>, ReallocLarger<SutNS>,
                                          ReallocSmaller<SutNS>, Free<SutNS>>());
}

RC_GTEST_PROP(AllocSutNone, Fuzz, ()) { Test<SutNone>(); }
RC_GTEST_PROP(AllocSutUint8Size, Fuzz, ()) { Test<SutUint8Size>(); }
RC_GTEST_PROP(AllocSutUint8Larger, Fuzz, ()) { Test<SutUint8Larger>(); }
RC_GTEST_PROP(AllocSutMedium, Fuzz, ()) { Test<SutMedium>(); }

} // namespace