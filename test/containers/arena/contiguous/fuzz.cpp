
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../macros.hpp"
#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-cpp/test/rapidcheck_fuzz.hpp>

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>
#include <derive-c/container/arena/contiguous/includes.h>

template <ObjectType Object> struct SutObject_8 {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE Object
#define VALUE_CLONE Object::clone_
#define VALUE_DELETE Object::delete_
#define INDEX_BITS 8
#include <derive-c/container/arena/contiguous/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

using Sut_8_PrimitiveSmall = SutObject_8<Primitive<uint8_t>>;
INDEX_ITEMS_EQ_HASH(Sut_8_PrimitiveSmall);

using Sut_8_PrimitiveMedium = SutObject_8<Primitive<size_t>>;
INDEX_ITEMS_EQ_HASH(Sut_8_PrimitiveMedium);

template <ObjectType Object> struct SutObject_16 {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE Object
#define VALUE_CLONE Object::clone_
#define VALUE_DELETE Object::delete_
#define INDEX_BITS 16
#include <derive-c/container/arena/contiguous/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

using Sut_16_PrimitiveSmall = SutObject_16<Primitive<uint8_t>>;
INDEX_ITEMS_EQ_HASH(Sut_16_PrimitiveSmall);

using Sut_16_PrimitiveMedium = SutObject_16<Primitive<size_t>>;
INDEX_ITEMS_EQ_HASH(Sut_16_PrimitiveMedium);

template <ObjectType Object> struct SutObject_32 {
#define EXPAND_IN_STRUCT
#define NAME Sut
#define VALUE Object
#define VALUE_CLONE Object::clone_
#define VALUE_DELETE Object::delete_
#define INDEX_BITS 32
#include <derive-c/container/arena/contiguous/template.h>

    static size_t max_size() { return Sut_max_entries; }
};

using Sut_32_Empty = SutObject_32<Empty>;
INDEX_ITEMS_EQ_HASH(Sut_32_Empty);

using Sut_32_Complex = SutObject_32<Complex>;
INDEX_ITEMS_EQ_HASH(Sut_32_Complex);

namespace {
template <typename SutNS> void Test() {
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new_with_capacity_for(1, stdalloc_get_ref()));
    SutModel<SutNS> model;
    rc::state::check(model, sutWrapper,
                     rc::state::gen::execOneOfWithArgs<Insert<SutNS>, Insert<SutNS>, Insert<SutNS>,
                                                       Write<SutNS>, Remove<SutNS>>());
}

// clang-format off
FUZZ(Sut_8_PrimitiveSmall,   Sut_8_PrimitiveSmall  )
FUZZ(Sut_8_PrimitiveMedium,  Sut_8_PrimitiveMedium )
FUZZ(Sut_16_PrimitiveSmall,  Sut_16_PrimitiveSmall )
FUZZ(Sut_16_PrimitiveMedium, Sut_16_PrimitiveMedium)
FUZZ(Sut_32_Empty,           Sut_32_Empty          )
FUZZ(Sut_32_Complex,         Sut_32_Complex        )
// clang-format on

} // namespace
