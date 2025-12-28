#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../macros.hpp"
#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/arena/geometric/includes.h>

template <typename Integer> struct SutPrimitive_3_8 {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE Integer
#define INITIAL_BLOCK_INDEX_BITS 3
#define INDEX_BITS 8
#include <derive-c/container/arena/geometric/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

using Sut_3_8_PrimitiveSmall = SutPrimitive_3_8<uint8_t>;
INDEX_ITEMS_EQ_HASH(Sut_3_8_PrimitiveSmall);

using Sut_3_8_PrimitiveMedium = SutPrimitive_3_8<size_t>;
INDEX_ITEMS_EQ_HASH(Sut_3_8_PrimitiveMedium);

template <typename Integer> struct SutPrimitive_3_16 {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE Integer
#define INITIAL_BLOCK_INDEX_BITS 3
#define INDEX_BITS 16
#include <derive-c/container/arena/geometric/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

using Sut_16_PrimitiveSmall = SutPrimitive_3_16<uint8_t>;
INDEX_ITEMS_EQ_HASH(Sut_16_PrimitiveSmall);

using Sut_16_PrimitiveMedium = SutPrimitive_3_16<size_t>;
INDEX_ITEMS_EQ_HASH(Sut_16_PrimitiveMedium);

template <typename Object> struct SutObject_5_32 {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE Object
#define VALUE_CLONE Object::clone_
#define VALUE_DELETE Object::delete_
#define INITIAL_BLOCK_INDEX_BITS 5
#define INDEX_BITS 32
#include <derive-c/container/arena/geometric/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

using Sut_32_Empty = SutObject_5_32<Empty>;
INDEX_ITEMS_EQ_HASH(Sut_32_Empty);

using Sut_32_Complex = SutObject_5_32<Complex>;
INDEX_ITEMS_EQ_HASH(Sut_32_Complex);

namespace {
template <typename SutNS> void Test() {
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(stdalloc_get()));
    SutModel<SutNS> model;
    rc::state::check(model, sutWrapper,
                     rc::state::gen::execOneOfWithArgs<Insert<SutNS>, Insert<SutNS>, Insert<SutNS>,
                                                       Write<SutNS>, Remove<SutNS>>());
}

RC_GTEST_PROP(GeometricArenaFuzz, Sut_3_8_PrimitiveSmall, ()) { Test<Sut_3_8_PrimitiveSmall>(); }
RC_GTEST_PROP(GeometricArenaFuzz, Sut_3_8_PrimitiveMedium, ()) { Test<Sut_3_8_PrimitiveMedium>(); }
RC_GTEST_PROP(GeometricArenaFuzz, Sut_3_16_PrimitiveSmall, ()) { Test<Sut_16_PrimitiveSmall>(); }
RC_GTEST_PROP(GeometricArenaFuzz, Sut_3_16_PrimitiveMedium, ()) { Test<Sut_16_PrimitiveMedium>(); }
RC_GTEST_PROP(GeometricArenaFuzz, Sut_5_32_Empty, ()) { Test<Sut_32_Empty>(); }
RC_GTEST_PROP(GeometricArenaFuzz, Sut_5_32_Complex, ()) { Test<Sut_32_Complex>(); }

} // namespace
