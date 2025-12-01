#include <derive-cpp/test/rapidcheck_panic.hpp>
#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include "../commands.hpp"

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

namespace containers::map::decomposed {

template <typename SutNS> struct ExtendCapacity : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    static const size_t MAX_FACTOR = 5;
    size_t oldCapacity;
    size_t newCapacity;

    explicit ExtendCapacity(const Model& m)
        : oldCapacity(m.size()),
          newCapacity(*rc::gen::inRange(oldCapacity, oldCapacity * MAX_FACTOR)) {}
    
    void checkPreconditions(const Model& s) const override {}
void apply(Model& m) const override {
        // No-op, just extending the capacity
    }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        SutNS::Sut_extend_capacity_for(w.get(), newCapacity);
    }

    void show(std::ostream& os) const override {
        os << "ExtendCapacity(" << oldCapacity << " -> " << newCapacity << ")";
    }

};
    
RC_GTEST_PROP(DecomposedTests, Fuzz, ()) {
    using SutNS = SutIntegers<size_t, size_t>;
    SutWrapper<SutNS> sutWrapper(SutNS::Sut_new(stdalloc_get()));
    SutModel<SutNS> model;
    
    rc::state::check(
        model, sutWrapper,
        rc::state::gen::execOneOfWithArgs<
            Insert<SutNS>, Insert<SutNS>, Insert<SutNS>, Insert<SutNS>, 
            ExtendCapacity<SutNS>, 
            Write<SutNS>,
            Remove<SutNS>, DeleteEntry<SutNS>, DuplicateInsert<SutNS>>());


}
}
