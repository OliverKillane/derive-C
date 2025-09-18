#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include <derive-cpp/test/strongint.h>
#include <unordered_map>

using Value = std::size_t;
using ModelIndex = derivecpp::StrongInteger<struct ModelIndexTag, Value>;

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
#include <derive-c/alloc/std.h>

#define NAME Sut
#define VALUE Value
#define INDEX_BITS 8
#include <derive-c/container/arena/basic/template.h>
}

inline bool operator==(const Sut_index& a, const Sut_index& b) { return a.index == b.index; }
inline bool operator==(const Sut_iv& a, const Sut_iv& b) {
    return a.index == b.index && a.value == b.value;
}
inline bool operator==(const Sut_iv_const& a, const Sut_iv_const& b) {
    return a.index == b.index && a.value == b.value;
}

namespace std {
template <> struct hash<Sut_index> {
    std::size_t operator()(const Sut_index& s) const noexcept {
        return std::hash<uint8_t>{}(s.index);
    }
};
} // namespace std

struct SutWrapper {
    // JUSTIFY: 3 items
    //          - Want to ensure tests have plenty of reallocations / extensions
    //          - odd number for hitting edge cases with capacity (set to power of 2)
    SutWrapper() : sut(Sut_new_with_capacity_for(1, stdalloc_get())) {}
    SutWrapper(const SutWrapper& other) : sut(Sut_clone(other.getConst())) {}
    ~SutWrapper() { Sut_delete(&sut); }

    [[nodiscard]] Sut* get() { return &sut; }
    [[nodiscard]] Sut const* getConst() const { return &sut; }

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

        {
            RC_ASSERT(m.map.size() == Sut_size(s.getConst()));
            SutWrapper wrapperCopy = s;
            for (const auto& [key, value] : m.map) {
                Sut_index index = s.indexToSutIndex.at(key);
                RC_ASSERT(*Sut_read(s.getConst(), index) == value);
                RC_ASSERT(*Sut_write(wrapperCopy.get(), index) == value);
            }
        }

        {
            SutWrapper wrapperCopy = s;
            Sut_iter_const iter_const = Sut_get_iter_const(s.getConst());
            Sut_iter iter = Sut_get_iter(wrapperCopy.get());
            while (!Sut_iter_const_empty(&iter_const)) {
                RC_ASSERT(!Sut_iter_empty(&iter));
                Sut_iv_const const* item_const = Sut_iter_const_next(&iter_const);
                Sut_iv const* item = Sut_iter_next(&iter);
                RC_ASSERT(item_const->index == item->index);
                ModelIndex modelIndex = s.SutIndexToIndex.at(item_const->index);
                RC_ASSERT(m.map.at(modelIndex) == *item_const->value);
                RC_ASSERT(*item->value == *item_const->value);
            }
            RC_ASSERT(Sut_iter_empty(&iter));
            RC_ASSERT(Sut_iter_next(&iter) == nullptr);
            RC_ASSERT(Sut_iter_const_next(&iter_const) == nullptr);
        }

        // JUSTIFY:
        //  - We need to check the failure paths additionally
        {
            SutWrapper wrapperCopy = s;
            Sut_index invalid_index{0};

            while (s.SutIndexToIndex.contains(invalid_index)) {
                invalid_index.index++;
            }

            RC_ASSERT(Sut_try_read(s.getConst(), invalid_index) == nullptr);
            RC_ASSERT(Sut_try_write(wrapperCopy.get(), invalid_index) == nullptr);
            Value removed;
            RC_ASSERT(!Sut_try_remove(wrapperCopy.get(), invalid_index, &removed));
            RC_ASSERT(!Sut_delete_entry(wrapperCopy.get(), invalid_index));
        }
    }
};

struct Insert : Command {
    Value value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Value>::max());

    void checkPreconditions(const Model& s) const override {
        RC_ASSERT(s.map.size() < Sut_max_capacity);
    }

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

        Value entry = Sut_remove(s.get(), sut_index);
        RC_ASSERT(entry == m.map.at(index.value()));
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
    rc::state::check(model, sutWrapper,
                     rc::state::gen::execOneOfWithArgs<Insert, Insert, Write, Remove, Delete>());
}

TEST(ArenaTests, Full) {
    Sut sut = Sut_new_with_capacity_for(1, stdalloc_get());
    ASSERT_FALSE(Sut_full(&sut));
    ASSERT_EQ(Sut_max_capacity, 256);

    for (size_t i = 0; i < Sut_max_capacity; ++i) {
        Sut_insert(&sut, i);
    }
    ASSERT_TRUE(Sut_full(&sut));

    Sut_delete(&sut);
}
