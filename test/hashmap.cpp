#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>
#include <optional>
#include <unordered_map>

using Value = size_t;
using Key = size_t;
using Model = std::unordered_map<Key, Value>;

extern "C" {
#define PANIC abort()
bool key_equality(Key const* key_1, Key const* key_2) { return *key_1 == *key_2; }
Key key_hash(Key const* key) { return *key; }

#define K Key
#define V Value
#define EQ key_equality
#define HASH key_hash
#define SELF Sut
#include <derive-c/structures/hashmap.template.h>
}

struct SutWrapper {
    SutWrapper() : sut(Sut_new()) {}
    ~SutWrapper() { Sut_delete(&sut); }

    SutWrapper(const Sut& sut) : sut(Sut_clone(&sut)) {}
    SutWrapper& operator=(const SutWrapper& other) {
        if (this != &other) {
            Sut_delete(&sut);
            sut = Sut_clone(&other.sut);
        }
        return *this;
    }

    Sut* get() { return &sut; }
    Sut const* getConst() const { return &sut; }

    Sut sut;
};

struct Command : rc::state::Command<Model, SutWrapper> {
    virtual void runAndCheck(const Model& s0, SutWrapper& sut) const = 0;

    void run(const Model& s0, SutWrapper& sut) const override {
        runAndCheck(s0, sut);
        checkInvariants(s0, sut);
    }

    void checkInvariants(const Model& oldModel, const SutWrapper& sut) const {
        Model model = nextState(oldModel);

        RC_ASSERT(model.size() == Sut_size(sut.getConst()));
        
        for (const auto& [key, value] : model) {
            RC_ASSERT(value == *Sut_read(sut.getConst(), key));
        }

        // JUSTIFY: Separately checking the iterator implementations
        //           - Different access to SELF_read
        //           - iterating on sut, against map will find elements 
        //             present in sut but not in model (not checked by previous loop)

        Sut_iter_const iter = Sut_get_iter_const(sut.getConst());
        while (!Sut_iter_const_empty(&iter)) {
            Sut_kv_const item = Sut_iter_const_next(&iter);
            RC_ASSERT(item.key != nullptr && item.value != nullptr);
            RC_ASSERT(model[*item.key] == *item.value);
        }
    }
};

struct Insert : Command {
    Key key =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Value>::max());
    Value value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Value>::max());

    void checkPreconditions(const Model& s0) { RC_PRE(s0.find(key) == s0.end()); }
    void apply(Model& s0) const override { s0[key] = value; }
    void runAndCheck(const Model& s0, SutWrapper& sut) const override {
        Value* val = Sut_insert(sut.get(), key, value);
        RC_ASSERT(val != nullptr && *val == value);
    }
    void show(std::ostream& os) const override { os << "Insert(" << key << ", " << value << ")"; }
};

struct Write : Command {
    explicit Write(const Model& s0) {
        if (s0.empty()) {
            key = std::nullopt;
        } else {
            std::vector<Key> keys;
            keys.reserve(s0.size());
            for (const auto& [k, _] : s0) {
                keys.push_back(k);
            }
            key = *rc::gen::elementOf(keys);
        }
    }

    std::optional<Key> key;
    Value value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Value>::max());

    void checkPreconditions(const Model& s0) { RC_PRE(key.has_value()); }
    void apply(Model& s0) const override { s0[key.value()] = value; }
    void runAndCheck(const Model& s0, SutWrapper& sut) const override {
        Value* val = Sut_write(sut.get(), key.value());
        // RC_ASSERT(val);
        *val = value;
    }
    void show(std::ostream& os) const override { 
        
        if (key.has_value()) {
            os << "Write(" << key.value() << " = " << value << ")";
        } else {
            os << "Default Write (no key, cannot pass preconditions)";
        }
    }
};

struct Remove : Command {
    explicit Remove(const Model& s0) {
        if (s0.empty()) {
            key = std::nullopt;
        } else {
            // key = std::nullopt;
            std::vector<Key> keys;
            keys.reserve(s0.size());
            for (const auto& [k, _] : s0) {
                keys.push_back(k);
            }
            key = *rc::gen::elementOf(keys);
        }
    }

    std::optional<Key> key;
    void checkPreconditions(const Model& s0) { RC_PRE(key.has_value()); }
    void apply(Model& s0) const override { 
        if (key.has_value()) {
            RC_ASSERT(s0.erase(key.value())); 
        }
    }
    void runAndCheck(const Model& s0, SutWrapper& sut) const override {
        if (key.has_value()) {
            Value* val = Sut_remove(sut.get(), key.value());
            if (val == nullptr) {
                RC_ASSERT(false);
            } else {
                // *val = 0; // NOTE: Simulate access to delete
            }
        }
    }
    void show(std::ostream& os) const override { 
        if (key.has_value()) {
            os << "Remove(" << key.value() << ")"; 
        } else {
             os << "Default Remove (no key, cannot pass preconditions)";
        }
    }
};

RC_GTEST_PROP(HashMapTests, General, ()) {
    Model model;
    SutWrapper sut;
    rc::state::check(model, sut, rc::state::gen::execOneOfWithArgs<Insert, Remove>());
}


// void intercept () {}

// TEST(HashMapTests, Custom) {
//     intercept();
//     SutWrapper sut;

//     Sut_insert(sut.get(), 0, 0);
//     Sut_insert(sut.get(), 893503477868103032, 843757779030619246);
//     Sut_remove(sut.get(), 893503477868103032); // success remove
//     Sut_insert(sut.get(), 0, 0); // fail insert

//     ASSERT_EQ(Sut_size(sut.get()), 1);
// }
