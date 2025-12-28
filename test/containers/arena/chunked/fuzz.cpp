
#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../macros.hpp"
#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/arena/chunked/includes.h>

template <ObjectType Object> struct SutObject_8_2 {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE Object
#define VALUE_CLONE Object::clone_
#define VALUE_DELETE Object::delete_
#define INDEX_BITS 8
#define BLOCK_INDEX_BITS 2
#include <derive-c/container/arena/chunked/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

using Sut_8_2_PrimitiveSmall = SutObject_8_2<Primitive<uint8_t>>;
INDEX_ITEMS_EQ_HASH(Sut_8_2_PrimitiveSmall);

using Sut_8_2_PrimitiveMedium = SutObject_8_2<Primitive<size_t>>;
INDEX_ITEMS_EQ_HASH(Sut_8_2_PrimitiveMedium);

template <ObjectType Object> struct SutObject_16_8 {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE Object
#define VALUE_CLONE Object::clone_
#define VALUE_DELETE Object::delete_
#define INDEX_BITS 16
#define BLOCK_INDEX_BITS 8
#include <derive-c/container/arena/chunked/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

using Sut_16_8_PrimitiveSmall = SutObject_16_8<Primitive<uint8_t>>;
INDEX_ITEMS_EQ_HASH(Sut_16_8_PrimitiveSmall);

using Sut_16_8_PrimitiveMedium = SutObject_16_8<Primitive<size_t>>;
INDEX_ITEMS_EQ_HASH(Sut_16_8_PrimitiveMedium);

template <ObjectType Object> struct SutObject_16_5 {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE Object
#define VALUE_CLONE Object::clone_
#define VALUE_DELETE Object::delete_
#define INDEX_BITS 16
#define BLOCK_INDEX_BITS 5
#include <derive-c/container/arena/chunked/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

using Sut_16_5_Empty = SutObject_16_5<Empty>;
INDEX_ITEMS_EQ_HASH(Sut_16_5_Empty);

using Sut_16_5_Complex = SutObject_16_5<Complex>;
INDEX_ITEMS_EQ_HASH(Sut_16_5_Complex);

namespace {
template <typename SutNS> void Test() {
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(stdalloc_get()));
    SutModel<SutNS> model;
    rc::state::check(model, sutWrapper,
                     rc::state::gen::execOneOfWithArgs<Insert<SutNS>, Insert<SutNS>, Insert<SutNS>,
                                                       Write<SutNS>, Remove<SutNS>>());
}

RC_GTEST_PROP(ContiguousArenaFuzz, Sut_8_2_PrimitiveSmall, ()) { Test<Sut_8_2_PrimitiveSmall>(); }
RC_GTEST_PROP(ContiguousArenaFuzz, Sut_8_2_PrimitiveMedium, ()) { Test<Sut_8_2_PrimitiveMedium>(); }
RC_GTEST_PROP(ContiguousArenaFuzz, Sut_16_8_PrimitiveSmall, ()) { Test<Sut_16_8_PrimitiveSmall>(); }
RC_GTEST_PROP(ContiguousArenaFuzz, Sut_16_8_PrimitiveMedium, ()) {
    Test<Sut_16_8_PrimitiveMedium>();
}
RC_GTEST_PROP(ContiguousArenaFuzz, Sut_16_5_Empty, ()) { Test<Sut_16_5_Empty>(); }
RC_GTEST_PROP(ContiguousArenaFuzz, Sut_16_5_Complex, ()) { Test<Sut_16_5_Complex>(); }

} // namespace
