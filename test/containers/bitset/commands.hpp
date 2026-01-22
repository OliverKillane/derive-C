#pragma once

#include <limits>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>
#include <unordered_set>

using SutModel = std::unordered_set<size_t>;

template <typename SutNS> struct SutWrapper {
    SutWrapper(SutNS::Sut s) : mSut(s) {}
    SutWrapper(const SutWrapper& other) : mSut(SutNS::Sut_clone(&other.mSut)) {}
    ~SutWrapper() { SutNS::Sut_delete(&mSut); }

    [[nodiscard]] SutNS::Sut* get() { return &mSut; }
    [[nodiscard]] SutNS::Sut const* getConst() const { return &mSut; }

    // System Under Test
    SutNS::Sut mSut;
};

template <typename SutNS> struct Command : rc::state::Command<SutModel, SutWrapper<SutNS>> {
    using Model = SutModel;
    using Wrapper = SutWrapper<SutNS>;

    virtual void runCommand(const Model& m, Wrapper& w) const = 0;
    virtual void AdditionalChecks(const Model& m, const Wrapper& w) const {}

    void CheckValues(const Model& m, const Wrapper& w) const {
        RC_ASSERT(m.size() == SutNS::Sut_size(w.getConst()));
        for (const auto& index : m) {
            RC_ASSERT(SutNS::Sut_get(w.getConst(), index));
        }
    }

    void CheckIterators(const Model& m, const Wrapper& w) const {
        Wrapper wrapperCopy = w;

        typename SutNS::Sut_iter_const iter_const = SutNS::Sut_get_iter_const(w.getConst());
        typename SutNS::Sut_iter iter = SutNS::Sut_get_iter(wrapperCopy.get());

        while (!SutNS::Sut_iter_const_empty(&iter_const)) {
            RC_ASSERT(!SutNS::Sut_iter_empty(&iter));

            typename SutNS::Sut_iter_const_item item_const =
                SutNS::Sut_iter_const_next(&iter_const);
            typename SutNS::Sut_iter_item item = SutNS::Sut_iter_next(&iter);

            RC_ASSERT(!SutNS::Sut_iter_const_empty_item(&item_const));
            RC_ASSERT(!SutNS::Sut_iter_empty_item(&item));
            RC_ASSERT(m.contains(item_const));
        }

        RC_ASSERT(SutNS::Sut_iter_empty(&iter));
        RC_ASSERT(SutNS::Sut_iter_const_empty(&iter_const));

        typename SutNS::Sut_iter_item item = SutNS::Sut_iter_next(&iter);
        typename SutNS::Sut_iter_const_item item_const = SutNS::Sut_iter_const_next(&iter_const);
        RC_ASSERT(SutNS::Sut_iter_empty_item(&item));
        RC_ASSERT(SutNS::Sut_iter_const_empty_item(&item_const));
    }

    void CheckFailedAccess(const Model& m, const Wrapper& w) const {
        const auto it = std::max_element(m.begin(), m.end());
        if (it != m.end()) {
            const auto& max_value = *it;

            for (size_t i = 0; i <= max_value; i++) {
                if (!m.contains(i)) {
                    RC_ASSERT(!SutNS::Sut_get(w.getConst(), static_cast<SutNS::Sut_index_t>(i)));
                }
            }
        }
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

template <typename SutNS> struct SetOutOfBounds : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    bool const mValue = *rc::gen::arbitrary<bool>();
    typename SutNS::Sut_index_t mIndex =
        *rc::gen::oneOf(rc::gen::inRange(std::numeric_limits<typename SutNS::Sut_index_t>::min(),
                                         SutNS::Sut_min_index),
                        rc::gen::inRange(SutNS::Sut_max_index,
                                         std::numeric_limits<typename SutNS::Sut_index_t>::max()));

    void checkPreconditions(const Model& /*m*/) const override {
        // JUSTIFY: Additional check after generation
        // - As the max bound includes the max index, and we cannot +1 as this overflows to zero
        // - Easier to just re-check ranges here.
        RC_PRE(mIndex > SutNS::Sut_max_index || mIndex < SutNS::Sut_min_index);
    }
    void apply(Model& /*m*/) const override {}
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        RC_ASSERT(!SutNS::Sut_try_set(w.get(), mIndex, true));
    }

    void show(std::ostream& os) const override {
        os << "SetOutOfBounds(" << static_cast<size_t>(mIndex) << " = true)";
    }
};

template <typename SutNS> struct SetTrue : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    typename SutNS::Sut_index_t mIndex =
        *rc::gen::inRange(SutNS::Sut_min_index, SutNS::Sut_max_index);

    void checkPreconditions(const Model& m) const override {}
    void apply(Model& m) const override { m.insert((size_t)mIndex); }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        RC_ASSERT(SutNS::Sut_try_set(w.get(), mIndex, true));
    }

    void show(std::ostream& os) const override {
        os << "Set(" << static_cast<size_t>(mIndex) << " = true)";
    }
};

template <typename SutNS> struct SetFalse : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    typename SutNS::Sut_index_t mIndex =
        *rc::gen::inRange(SutNS::Sut_min_index, SutNS::Sut_max_index);

    void checkPreconditions(const Model& m) const override {}
    void apply(Model& m) const override { m.erase((size_t)mIndex); }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        RC_ASSERT(SutNS::Sut_try_set(w.get(), mIndex, false));
    }

    void show(std::ostream& os) const override {
        os << "Set(" << static_cast<size_t>(mIndex) << " = false)";
    }
};
