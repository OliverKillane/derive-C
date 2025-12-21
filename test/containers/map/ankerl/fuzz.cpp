
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#include <derive-c/container/map/ankerl/includes.h>

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
#include <derive-c/container/map/ankerl/template.h>
};

namespace {

RC_GTEST_PROP(Ankerl, Fuzz, ()) {
    using SutNS = SutIntegers<size_t, size_t>;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(stdalloc_get()));
    SutModel<SutNS> model;

    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<
            Insert<SutNS>, Insert<SutNS>, Insert<SutNS>, Insert<SutNS>, ExtendCapacity<SutNS>,
            Write<SutNS>, Remove<SutNS>, DeleteEntry<SutNS>, DuplicateInsert<SutNS>>());
}
} // namespace