
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"
#include "../../objects.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/map/decomposed/includes.h>

/// A test case for maps from (int -> int)
template <typename Key, typename Value> struct SutIntegers {
    static bool equality(Key const* key_1, Key const* key_2) { return *key_1 == *key_2; }
    static size_t awful_hash(Key const* key) {
        // Bad hash exposes more collisions in the map
        constexpr size_t SMALL_MOD = 1000;
        if (*key % 2 == 0) {
            return SMALL_MOD + (*key % SMALL_MOD);
        }
        return 0;
    }

#define EXPAND_IN_STRUCT
#define KEY Key
#define KEY_EQ equality
#define KEY_HASH awful_hash
#define VALUE Value
#define NAME Sut
#include <derive-c/container/map/decomposed/template.h>
};

template <typename Key, typename Value> struct SutObjects {
#define EXPAND_IN_STRUCT
#define KEY Key
#define KEY_EQ Key::equality_
#define KEY_HASH Key::hash_
#define KEY_DELETE Key::delete_
#define KEY_CLONE Key::clone_
#define VALUE Value
#define VALUE_CLONE Value::clone_
#define VALUE_DELETE Value::delete_
#define NAME Sut
#include <derive-c/container/map/decomposed/template.h>
};

namespace {
template <typename SutNS> void Test(SutWrapper<SutNS> sutWrapper) {
    SutModel<SutNS> model;
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<
            Insert<SutNS>, Insert<SutNS>, Insert<SutNS>, Insert<SutNS>, ExtendCapacity<SutNS>,
            Write<SutNS>, Remove<SutNS>, DeleteEntry<SutNS>, DuplicateInsert<SutNS>>());
}

RC_GTEST_PROP(DecomposedSmall, Fuzz, ()) {
    using SutNS = SutIntegers<uint32_t, uint8_t>;
    Test(SutWrapper<SutNS>(SutNS::Sut_new(stdalloc_get())));
}

RC_GTEST_PROP(DecomposedMedium, Fuzz, ()) {
    using SutNS = SutIntegers<size_t, size_t>;
    Test(SutWrapper<SutNS>(SutNS::Sut_new(stdalloc_get())));
}

RC_GTEST_PROP(DecomposedComplexEmpty, Fuzz, ()) {
    using SutNS = SutObjects<Complex, Empty>;
    Test(SutWrapper<SutNS>(SutNS::Sut_new(stdalloc_get())));
}

RC_GTEST_PROP(DecomposedComplexComplex, Fuzz, ()) {
    using SutNS = SutObjects<Complex, Complex>;
    Test(SutWrapper<SutNS>(SutNS::Sut_new(stdalloc_get())));
}

} // namespace
