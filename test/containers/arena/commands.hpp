#pragma once

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <derive-c/core/debug/memory_tracker.h>

using ModelIndex = size_t;

// JUSTIFY: Sequential Index Generator
//  - We apply commands to the model and sut separately, so we need a was to generate a sequence of
//    model indexes, that we can map in the sut's wrapper, and add to the model's map.
struct ModelIndexGen {
    ModelIndex next() { return mIndex++; }
    ModelIndex mIndex{0};
};

template <typename SutNS> struct SutModel {
    ModelIndexGen mModelIndexGen;
    std::unordered_map<ModelIndex, typename SutNS::Sut_value_t> mValues;
};

template <typename SutNS> struct SutWrapper {
    SutWrapper(SutNS::Sut s) : mSut(s) {}
    SutWrapper(const SutWrapper& other) : mSut(SutNS::Sut_clone(&other.mSut)) {}
    ~SutWrapper() { SutNS::Sut_delete(&mSut); }

    [[nodiscard]] SutNS::Sut* get() { return &mSut; }
    [[nodiscard]] SutNS::Sut const* getConst() const { return &mSut; }

    // System Under Test
    SutNS::Sut mSut;

    // Index Mapping
    using SutIndex = typename SutNS::Sut_index_t;
    ModelIndexGen mIndexGen;
    std::unordered_map<ModelIndex, SutIndex> mModelToSut;
    std::unordered_map<SutIndex, ModelIndex> mSutToModel;
};

template <typename SutNS> struct Command : rc::state::Command<SutModel<SutNS>, SutWrapper<SutNS>> {
    using Model = SutModel<SutNS>;
    using Wrapper = SutWrapper<SutNS>;

    // Match old runAndCheck signature: runCommand receives the current model so it can
    // perform the same assertions against it.
    virtual void runCommand(const Model& m, Wrapper& w) const = 0;
    virtual void AdditionalChecks(const Model& m, const Wrapper& w) const {}

    void CheckValues(const Model& m, const Wrapper& w) const {
        RC_ASSERT(m.mValues.size() == SutNS::Sut_size(w.getConst()));
        Wrapper wMut = w;
        for (const auto& [key, value] : m.mValues) {
            typename SutNS::Sut_index_t index = w.mModelToSut.at(key);
            RC_ASSERT(*SutNS::Sut_read(w.getConst(), index) == value);
            RC_ASSERT(*SutNS::Sut_write(wMut.get(), index) == value);
        }
    }

    void CheckIterators(const Model& m, const Wrapper& w) const {
        Wrapper wMut = w;

        typename SutNS::Sut_iter_const iter_const = SutNS::Sut_get_iter_const(w.getConst());
        typename SutNS::Sut_iter iter = SutNS::Sut_get_iter(wMut.get());

        typename SutNS::Sut_iter_const_item item_const = SutNS::Sut_iter_const_next(&iter_const);
        typename SutNS::Sut_iter_item item = SutNS::Sut_iter_next(&iter);

        while (!SutNS::Sut_iter_const_empty_item(&item_const)) {
            RC_ASSERT(item_const.index == item.index);
            ModelIndex modelIndex = w.mSutToModel.at(item_const.index);
            RC_ASSERT(m.mValues.at(modelIndex) == *item_const.value);
            RC_ASSERT(*item.value == *item_const.value);
            item_const = SutNS::Sut_iter_const_next(&iter_const);
            item = SutNS::Sut_iter_next(&iter);
        }

        item = SutNS::Sut_iter_next(&iter);
        item_const = SutNS::Sut_iter_const_next(&iter_const);
        RC_ASSERT(SutNS::Sut_iter_empty_item(&item));
        RC_ASSERT(SutNS::Sut_iter_const_empty_item(&item_const));
    }

    void CheckFailedAccess(const Model& m, const Wrapper& w) const {
        (void)m;
        Wrapper wMut = w;
        // Default-construct an index (strong-int typically provides a .index field).
        // Increment the underlying representation until we find an index not present in the map.
        typename SutNS::Sut_index_t invalid_index{};
        while (w.mSutToModel.contains(invalid_index)) {
            invalid_index.index++;
        }
        auto const* x = SutNS::Sut_try_read(w.getConst(), invalid_index);
        RC_ASSERT(x == nullptr);

        RC_ASSERT(SutNS::Sut_try_write(wMut.get(), invalid_index) == nullptr);

        typename SutNS::Sut_value_t removed;
        RC_ASSERT(!SutNS::Sut_try_remove(wMut.get(), invalid_index, &removed));
    }

    void run(const Model& m, Wrapper& w) const override {
        // pass the current model through to runCommand so behavior matches the old
        // runAndCheck which used the model parameter.
        runCommand(m, w);

        Model next = this->nextState(m);
        CheckValues(next, w);
        CheckIterators(next, w);
        CheckFailedAccess(next, w);
        AdditionalChecks(next, w);
    }
};

template <typename SutNS> struct Insert : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    typename SutNS::Sut_value_t mValue = *rc::gen::arbitrary<typename SutNS::Sut_value_t>();

    void checkPreconditions(const Model& s) const override {
        RC_ASSERT(s.mValues.size() < SutNS::max_size());
    }

    void apply(Model& s) const override {
        ModelIndex model_index = s.mModelIndexGen.next();
        s.mValues[model_index] = mValue;
    }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        // The model parameter is unused here (same as old Insert::runAndCheck).
        typename SutNS::Sut_index_t sutIndex = SutNS::Sut_insert(w.get(), mValue);

        ModelIndex modelIndex = w.mIndexGen.next();
        w.mModelToSut[modelIndex] = sutIndex;
        w.mSutToModel[sutIndex] = modelIndex;
    }

    void show(std::ostream& os) const override { os << "Insert(" << mValue << ")"; }
};

template <typename SutNS> struct Write : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<ModelIndex> mIndex = std::nullopt;
    typename SutNS::Sut_value_t mValue = *rc::gen::arbitrary<typename SutNS::Sut_value_t>();

    explicit Write(const Model& m) {
        if (!m.mValues.empty()) {
            std::vector<ModelIndex> indices;
            indices.reserve(m.mValues.size());
            for (const auto& [k, _] : m.mValues) {
                indices.push_back(k);
            }
            mIndex = *rc::gen::elementOf(indices);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(mIndex.has_value());
        RC_PRE(m.mValues.find(mIndex.value()) != m.mValues.end());
    }

    void apply(Model& m) const override { m.mValues.at(mIndex.value()) = mValue; }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        // Same behaviour as old Write::runAndCheck
        typename SutNS::Sut_index_t sut_index = w.mModelToSut.at(mIndex.value());
        typename SutNS::Sut_value_t* p = SutNS::Sut_write(w.get(), sut_index);
        *p = mValue;
    }

    void show(std::ostream& os) const override {
        os << "Write(" << (mIndex.has_value() ? std::to_string(mIndex.value()) : "none") << " = "
           << mValue << ")";
    }
};

template <typename SutNS> struct Remove : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<ModelIndex> mIndex = std::nullopt;

    explicit Remove(const Model& m) {
        if (!m.mValues.empty()) {
            std::vector<ModelIndex> indices;
            indices.reserve(m.mValues.size());
            for (const auto& [k, _] : m.mValues) {
                indices.push_back(k);
            }
            mIndex = *rc::gen::elementOf(indices);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(mIndex.has_value());
        RC_PRE(m.mValues.find(mIndex.value()) != m.mValues.end());
    }

    void apply(Model& m) const override { m.mValues.erase(mIndex.value()); }

    void runCommand(const Model& m, Wrapper& w) const override {
        typename SutNS::Sut_index_t sut_index = w.mModelToSut.at(mIndex.value());

        w.mModelToSut.erase(mIndex.value());
        w.mSutToModel.erase(sut_index);

        typename SutNS::Sut_value_t const* entry_ptr = SutNS::Sut_read(w.getConst(), sut_index);
        typename SutNS::Sut_value_t entry = SutNS::Sut_remove(w.get(), sut_index);
        dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_CONTAINER, DC_MEMORY_TRACKER_CAP_NONE,
                                entry_ptr, sizeof(typename SutNS::Sut_value_t));

        RC_ASSERT(entry == m.mValues.at(mIndex.value()));

        SutNS::Sut_value_t::delete_(&entry);
    }

    void show(std::ostream& os) const override {
        os << "Remove(" << (mIndex.has_value() ? std::to_string(mIndex.value()) : "none") << ")";
    }
};
