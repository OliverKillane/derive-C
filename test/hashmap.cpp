#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include <optional>
#include <unordered_map>

namespace hashmap {

using Value = size_t;
using Key = size_t;
using Model = std::unordered_map<Key, Value>;

extern "C" {
bool key_equality(Key const* key_1, Key const* key_2) { return *key_1 == *key_2; }
Key key_hash(Key const* key) {
    // Bad hash exposes more collisions in the map
    if (*key % 2 == 0) {
        return 1000 + (*key % 1000);
    } else {
        return 0;
    }
}

#define K Key
#define V Value
#define EQ key_equality
#define HASH key_hash
#define SELF Sut
#include <derive-c/structures/hashmap/template.h>
}

struct SutWrapper {
    SutWrapper() : sut(Sut_new()) {}
    ~SutWrapper() { Sut_delete(&sut); }

    SutWrapper(const Sut& sut) : sut(Sut_shallow_clone(&sut)) {}
    SutWrapper& operator=(const SutWrapper& other) {
        if (this != &other) {
            Sut_delete(&sut);
            sut = Sut_shallow_clone(&other.sut);
        }
        return *this;
    }

    Sut* get() { return &sut; }
    Sut const* getConst() const { return &sut; }

    Sut sut;
};

struct Command : rc::state::Command<Model, SutWrapper> {
    virtual void runAndCheck(const Model& s, SutWrapper& m) const = 0;

    void run(const Model& m, SutWrapper& s) const override {
        runAndCheck(m, s);
        checkInvariants(m, s);
    }

    void checkInvariants(const Model& oldModel, const SutWrapper& s) const {
        Model m = nextState(oldModel);
        RC_ASSERT(m.size() == Sut_size(s.getConst()));

        for (const auto& [key, value] : m) {
            Value const* val = Sut_read(s.getConst(), key);
            RC_ASSERT(*val == value);
        }

        // JUSTIFY: Separately checking the iterator implementations
        //           - Different access to SELF_read
        //           - iterating on sut, against map will find elements
        //             present in sut but not in model (not checked by previous loop)
        Sut_iter_const iter = Sut_get_iter_const(s.getConst());
        while (!Sut_iter_const_empty(&iter)) {
            Sut_kv_const item = Sut_iter_const_next(&iter);
            RC_ASSERT(item.key != nullptr && item.value != nullptr);
            RC_ASSERT(m[*item.key] == *item.value);
        }
    }
};

struct Insert : Command {
    Key key =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Value>::max());
    Value value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Value>::max());

    void checkPreconditions(const Model& s) const override { RC_PRE(s.find(key) == s.end()); }
    void apply(Model& s) const override { s[key] = value; }
    void runAndCheck(const Model& m, SutWrapper& s) const override {
        Value* foundValue = Sut_insert(s.get(), key, value);
        RC_ASSERT(foundValue != nullptr && *foundValue == value);
    }
    void show(std::ostream& os) const override { os << "Insert(" << key << ", " << value << ")"; }
};

struct Write : Command {
    std::optional<Key> key = std::nullopt;
    Value value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Value>::max());

    explicit Write(const Model& m) {
        if (!m.empty()) {
            std::vector<Key> keys;
            keys.reserve(m.size());
            for (const auto& [k, _] : m) {
                keys.push_back(k);
            }
            key = *rc::gen::elementOf(keys);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(key.has_value());
        RC_PRE(m.find(key.value()) != m.end());
    }
    void apply(Model& m) const override { m[key.value()] = value; }
    void runAndCheck(const Model& m, SutWrapper& s) const override {
        Value* foundValue = Sut_write(s.get(), key.value());
        *foundValue = value;
    }
    void show(std::ostream& os) const override {
        os << "Write(" << key.value() << " = " << value << ")";
    }
};

struct Remove : Command {
    std::optional<Key> key = std::nullopt;

    explicit Remove(const Model& m) {
        if (!m.empty()) {
            std::vector<Key> keys;
            keys.reserve(m.size());
            for (const auto& [k, _] : m) {
                keys.push_back(k);
            }
            key = *rc::gen::elementOf(keys);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(key.has_value());
        RC_PRE(m.find(key.value()) != m.end());
    }

    void apply(Model& m) const override {
        if (key.has_value()) {
            RC_ASSERT(m.erase(key.value()));
        }
    }
    void runAndCheck(const Model& m, SutWrapper& s) const override {
        if (key.has_value()) {
            Sut_remove(s.get(), key.value());
        }
    }
    void show(std::ostream& os) const override { os << "Remove(" << key.value() << ")"; }
};

RC_GTEST_PROP(HashMapTests, General, ()) {
    Model model;
    SutWrapper sutWrapper;
    rc::state::check(model, sutWrapper,
                     rc::state::gen::execOneOfWithArgs<Insert, Insert, Insert, Write, Remove>());
}
} // namespace hashmap
