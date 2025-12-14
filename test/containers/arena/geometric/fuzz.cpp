#include <derive-cpp/test/rapidcheck_panic.hpp>
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../macros.hpp"
#include "../commands.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/arena/geometric/includes.h>

struct SutSmall {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE size_t
#define INITIAL_BLOCK_INDEX_BITS 3
#define INDEX_BITS 8
#include <derive-c/container/arena/geometric/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

INDEX_ITEMS_EQ_HASH(SutSmall);

struct SutMedium {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE uint8_t
#define INDEX_BITS 16
#include <derive-c/container/arena/geometric/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

INDEX_ITEMS_EQ_HASH(SutMedium);

namespace {

RC_GTEST_PROP(GeometricArena, FuzzSmall, ()) {
    SutWrapper<SutSmall> sutWrapper(SutSmall::Sut_new(stdalloc_get()));
    SutModel<SutSmall> sutModel;

    rc::state::check(sutModel, sutWrapper,
                     rc::state::gen::execOneOfWithArgs<Insert<SutSmall>, Insert<SutSmall>,
                                                       Write<SutSmall>, Remove<SutSmall>>());
}

RC_GTEST_PROP(GeometricArena, FuzzMedium, ()) {
    SutWrapper<SutMedium> sutWrapper(SutMedium::Sut_new(stdalloc_get()));
    SutModel<SutMedium> sutModel;

    rc::state::check(sutModel, sutWrapper,
                     rc::state::gen::execOneOfWithArgs<Insert<SutMedium>, Insert<SutMedium>,
                                                       Write<SutMedium>, Remove<SutMedium>>());
}

} // namespace
