#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include <unordered_map>

#include "rapidcheck/Assertions.h"
#include "utils.hpp"

namespace arena {

using Value = size_t;
using ModelIndex = StrongInteger<struct ModelIndexTag, size_t>;

struct IndexHelper {
    ModelIndex next() {
        auto ret = index;
        index.value++;
        return ret;
    }
    ModelIndex index{0};
};

struct Model {
    std::unordered_map<ModelIndex, Value> map;
    IndexHelper indexHelper;
};

extern "C" {
#define SELF Sut
#define V Value
#define INDEX_BITS 8
#include <derive-c/structures/arena/template.h>
}

inline bool operator==(const Sut_index& a, const Sut_index& b) { return a.index == b.index; }
} // namespace arena

namespace std {
template <> struct hash<arena::Sut_index> {
    std::size_t operator()(const arena::Sut_index& s) const noexcept {
        return std::hash<uint8_t>{}(s.index);
    }
};
} // namespace std

namespace arena {

struct SutWrapper {
    // JUSTIFY: 3 items
    //          - Want to ensure tests have plenty of reallocations / extensions
    //          - odd number for hitting edge cases with capacity (set to power of 2)
    SutWrapper() : sut(Sut_new_with_capacity_for(1)) {}
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
    std::unordered_map<ModelIndex, Sut_index> indexToSutIndex;
    std::unordered_map<Sut_index, ModelIndex> SutIndexToIndex;
    IndexHelper indexHelper;
};

struct Command : rc::state::Command<Model, SutWrapper> {
    virtual void runAndCheck(const Model& s, SutWrapper& m) const = 0;

    void run(const Model& m, SutWrapper& s) const override {
        runAndCheck(m, s);
        checkInvariants(m, s);
    }

    void checkInvariants(const Model& oldModel, const SutWrapper& s) const {
        Model m = nextState(oldModel);
        RC_ASSERT(m.map.size() == Sut_size(s.getConst()));

        for (const auto& [key, value] : m.map) {
            Sut_index index = s.indexToSutIndex.at(key);
            Value const* val = Sut_read(s.getConst(), index);
            RC_ASSERT(*val == value);
        }

        Sut_iter_const iter = Sut_get_iter_const(s.getConst());
        while (!Sut_iter_const_empty(&iter)) {
            Sut_iv_const item = Sut_iter_const_next(&iter);
            RC_ASSERT(item.value != nullptr);
            ModelIndex modelIndex = s.SutIndexToIndex.at(item.index);
            RC_ASSERT(m.map.at(modelIndex) == *item.value);
        }
    }
};

struct Insert : Command {
    Value value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Value>::max());

    void checkPreconditions(const Model& s) const override { s.map.size() < Sut_max_capacity; }

    void apply(Model& s) const override {
        ModelIndex model_index = s.indexHelper.next();
        s.map[model_index] = value;
    }

    void runAndCheck(const Model& m, SutWrapper& s) const override {
        Sut_index sut_index = Sut_insert(s.get(), value);
        ModelIndex model_index = s.indexHelper.next();

        s.indexToSutIndex[model_index] = sut_index;
        s.SutIndexToIndex[sut_index] = model_index;
    }

    void show(std::ostream& os) const override { os << "Insert(" << value << ")"; }
};

struct Write : Command {
    std::optional<ModelIndex> index = std::nullopt;
    Value value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Value>::max());

    explicit Write(const Model& m) {
        if (!m.map.empty()) {
            std::vector<ModelIndex> indices;
            indices.reserve(m.map.size());
            for (const auto& [k, _] : m.map) {
                indices.push_back(k);
            }
            index = *rc::gen::elementOf(indices);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(index.has_value());
        RC_PRE(m.map.find(index.value()) != m.map.end());
    }

    void apply(Model& m) const override { m.map[index.value()] = value; }

    void runAndCheck(const Model& m, SutWrapper& s) const override {
        Sut_index sut_index = s.indexToSutIndex.at(index.value());
        Value* foundValue = Sut_write(s.get(), sut_index);
        *foundValue = value;
    }
    void show(std::ostream& os) const override {
        os << "Write(" << index.value().value << " = " << value << ")";
    }
};

struct Remove : Command {
    std::optional<ModelIndex> index = std::nullopt;

    explicit Remove(const Model& m) {
        if (!m.map.empty()) {
            std::vector<ModelIndex> indices;
            indices.reserve(m.map.size());
            for (const auto& [k, _] : m.map) {
                indices.push_back(k);
            }
            index = *rc::gen::elementOf(indices);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(index.has_value());
        RC_PRE(m.map.find(index.value()) != m.map.end());
    }

    void apply(Model& m) const override { m.map.erase(index.value()); }

    void runAndCheck(const Model& m, SutWrapper& s) const override {
        Sut_index sut_index = s.indexToSutIndex.at(index.value());
        s.indexToSutIndex.erase(index.value());
        s.SutIndexToIndex.erase(sut_index);

        Sut_removed_entry entry = Sut_remove(s.get(), sut_index);
        RC_ASSERT(entry.value == m.map.at(index.value()));
        RC_ASSERT(entry.present == true);
    }
};

struct Delete : Command {
    std::optional<ModelIndex> index = std::nullopt;

    explicit Delete(const Model& m) {
        if (!m.map.empty()) {
            std::vector<ModelIndex> indices;
            indices.reserve(m.map.size());
            for (const auto& [k, _] : m.map) {
                indices.push_back(k);
            }
            index = *rc::gen::elementOf(indices);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(index.has_value());
        RC_PRE(m.map.find(index.value()) != m.map.end());
    }

    void apply(Model& m) const override { m.map.erase(index.value()); }

    void runAndCheck(const Model& m, SutWrapper& s) const override {
        Sut_index sut_index = s.indexToSutIndex.at(index.value());
        s.indexToSutIndex.erase(index.value());
        s.SutIndexToIndex.erase(sut_index);

        bool was_removed = Sut_delete_entry(s.get(), sut_index);
        RC_ASSERT(was_removed);
    }
};

RC_GTEST_PROP(ArenaTests, General, ()) {
    Model model;
    SutWrapper sutWrapper;
    rc::state::check(model, sutWrapper, rc::state::gen::execOneOfWithArgs<Insert, Insert, Write, Remove, Delete>());
}

} // namespace arena
