#pragma once

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>
#include <vector>

template <typename SutNS> using SutModel = std::vector<typename SutNS::Sut_item_t>;

template <typename SutNS> struct SutWrapper {
    SutWrapper(SutNS::Sut s) : mSut(s) {}
    SutWrapper(const SutWrapper& other) : mSut(SutNS::Sut_clone(&other.mSut)) {}
    ~SutWrapper() { SutNS::Sut_delete(&mSut); }

    [[nodiscard]] SutNS::Sut* get() { return &mSut; }
    [[nodiscard]] SutNS::Sut const* getConst() const { return &mSut; }

    // System Under Test
    SutNS::Sut mSut;
};

template <typename SutNS> struct Command : rc::state::Command<SutModel<SutNS>, SutWrapper<SutNS>> {
    using Model = SutModel<SutNS>;
    using Wrapper = SutWrapper<SutNS>;

    virtual void runCommand(const Model& m, Wrapper& w) const = 0;
    virtual void AdditionalChecks(const Model& m, const Wrapper& w) const {}

    void CheckContents(const Model& m, const Wrapper& w) const {
        RC_ASSERT(m.size() == SutNS::Sut_size(w.getConst()));
        Wrapper wrapperCopy = w;
        for (size_t i = 0; i < m.size(); ++i) {
            RC_ASSERT(m[i] == *SutNS::Sut_read(w.getConst(), i));
            RC_ASSERT(m[i] == *SutNS::Sut_write(wrapperCopy.get(), i));
        }
    }

    void CheckIterators(const Model& m, const Wrapper& w) const {
        Wrapper wrapperCopy = w;
        typename SutNS::Sut_iter_const iter_const = SutNS::Sut_get_iter_const(w.getConst());
        typename SutNS::Sut_iter iter = SutNS::Sut_get_iter(wrapperCopy.get());
        while (!SutNS::Sut_iter_const_empty(&iter_const)) {
            RC_ASSERT(!SutNS::Sut_iter_empty(&iter));
            size_t pos_const = SutNS::Sut_iter_const_position(&iter_const);
            size_t pos = SutNS::Sut_iter_position(&iter);
            typename SutNS::Sut_item_t const* item_const = SutNS::Sut_iter_const_next(&iter_const);
            typename SutNS::Sut_item_t* item = SutNS::Sut_iter_next(&iter);
            RC_ASSERT(!SutNS::Sut_iter_const_empty_item(&item_const));
            RC_ASSERT(!SutNS::Sut_iter_empty_item(&item));
            RC_ASSERT(m[pos] == *item_const);
            RC_ASSERT(m[pos] == *item);
        }
        RC_ASSERT(SutNS::Sut_iter_empty(&iter));
        RC_ASSERT(SutNS::Sut_iter_const_empty(&iter_const));
        RC_ASSERT(SutNS::Sut_iter_next(&iter) == nullptr);
        RC_ASSERT(SutNS::Sut_iter_const_next(&iter_const) == nullptr);
    }

    void CheckInvalidAccess(const Model& m, const Wrapper& w) const {
        Wrapper wrapperCopy = w;
        RC_ASSERT(SutNS::Sut_try_read(w.getConst(), m.size()) == nullptr);
        RC_ASSERT(SutNS::Sut_try_write(wrapperCopy.get(), m.size()) == nullptr);
    }

    void CheckEmpty(const Model& m, const Wrapper& w) const {
        if (m.empty()) {
            Wrapper wrapperCopy = w;
            typename SutNS::Sut_item_t destination;
            RC_ASSERT(!SutNS::Sut_try_pop(wrapperCopy.get(), &destination));
        }
    }

    void run(const Model& m, Wrapper& w) const override {
        runCommand(m, w);

        Model next = this->nextState(m);

        CheckContents(next, w);
        CheckIterators(next, w);
        CheckInvalidAccess(next, w);
        CheckEmpty(next, w);
        AdditionalChecks(next, w);
    }
};

template <typename SutNS> struct TryPushOverCapacity : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    typename SutNS::Sut_item_t mValue = *rc::gen::arbitrary<typename SutNS::Sut_item_t>();

    void checkPreconditions(const Model& s) const override {
        RC_PRE(s.size() >= SutNS::Sut_max_size);
    }

    void apply(Model& /*m*/) const override {}
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        RC_ASSERT(SutNS::Sut_try_push(w.get(), mValue) == nullptr);
    }
    void show(std::ostream& os) const override { os << "TryPushOverCapacity(" << mValue << ")"; }
};

template <typename SutNS> struct Push : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    typename SutNS::Sut_item_t mValue = *rc::gen::arbitrary<typename SutNS::Sut_item_t>();

    void checkPreconditions(const Model& s) const override {
        RC_PRE(s.size() < SutNS::Sut_max_size);
    }

    void apply(Model& m) const override { m.push_back(mValue); }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        SutNS::Sut_push(w.get(), mValue);
    }
    void show(std::ostream& os) const override { os << "Push(" << mValue << ")"; }
};

template <typename SutNS> struct Write : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<size_t> mIndex = std::nullopt;
    typename SutNS::Sut_item_t mValue = *rc::gen::arbitrary<typename SutNS::Sut_item_t>();

    explicit Write(const Model& m) {
        if (!m.empty()) {
            mIndex = *rc::gen::inRange(static_cast<size_t>(0), m.size() - 1);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(mIndex.has_value());
        RC_PRE(mIndex.value() < m.size());
    }

    void apply(Model& m) const override { m[mIndex.value()] = mValue; }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        typename SutNS::Sut_item_t* at = SutNS::Sut_write(w.get(), mIndex.value());
        *at = mValue;
    }
    void show(std::ostream& os) const override {
        os << "Write(" << mIndex.value() << " = " << mValue << ")";
    }
};

template <typename SutNS> struct TryInsertAt : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::vector<typename SutNS::Sut_item_t> mValues =
        *rc::gen::container<std::vector<typename SutNS::Sut_item_t>>(
            *rc::gen::inRange(static_cast<size_t>(0), static_cast<size_t>(10)),
            rc::gen::arbitrary<typename SutNS::Sut_item_t>());
    size_t mFromIndex;

    explicit TryInsertAt(const Model& m)
        : mFromIndex(*rc::gen::inRange(static_cast<size_t>(0), m.size())) {}

    void checkPreconditions(const Model& m) const override {
        RC_PRE(m.size() + mValues.size() < SutNS::Sut_max_size);
    }
    void apply(Model& m) const override {
        m.insert(m.begin() + mFromIndex, mValues.begin(), mValues.end());
    }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        if (mValues.empty()) {
            typename SutNS::Sut_item_t dummy;
            RC_ASSERT(SutNS::Sut_try_insert_at(w.get(), mFromIndex, &dummy, 0) == nullptr);
        } else {
            RC_ASSERT(SutNS::Sut_try_insert_at(w.get(), mFromIndex, mValues.data(),
                                               mValues.size()) != nullptr);
        }
    }
    void show(std::ostream& os) const override {
        os << "TryInsertAt(index=" << mFromIndex << ", values.size()=" << mValues.size() << ")";
    }
};

template <typename SutNS> struct TryInsertAtOverCapacity : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::vector<typename SutNS::Sut_item_t> mValues =
        *rc::gen::container<std::vector<typename SutNS::Sut_item_t>>(
            *rc::gen::inRange(static_cast<size_t>(0), static_cast<size_t>(10)),
            rc::gen::arbitrary<typename SutNS::Sut_item_t>());
    size_t mFromIndex;

    explicit TryInsertAtOverCapacity(const Model& m)
        : mFromIndex(*rc::gen::inRange(static_cast<size_t>(0), m.size())) {}

    void checkPreconditions(const Model& m) const override {
        RC_PRE(m.size() + mValues.size() >= SutNS::Sut_max_size);
    }
    void apply(Model& m) const override {}

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        typename SutNS::Sut_item_t dummy;
        RC_ASSERT(SutNS::Sut_try_insert_at(w.get(), mFromIndex, &dummy, 0) == nullptr);
    }
    void show(std::ostream& os) const override {
        os << "TryInsertAtOverCapacity(index=" << mFromIndex << ", values.size()=" << mValues.size()
           << ")";
    }
};

template <typename SutNS> struct RemoveAt : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    size_t mFromIndex;
    size_t mCount;

    explicit RemoveAt(const Model& m)
        : mFromIndex(*rc::gen::inRange(static_cast<size_t>(0), m.size())),
          mCount(*rc::gen::inRange(static_cast<size_t>(0), m.size() - mFromIndex + 1)) {}

    void checkPreconditions(const Model& m) const override {
        RC_PRE(mFromIndex + mCount <= m.size());
    }

    void apply(Model& m) const override {
        m.erase(m.begin() + mFromIndex, m.begin() + mFromIndex + mCount);
    }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        SutNS::Sut_remove_at(w.get(), mFromIndex, mCount);
    }

    void show(std::ostream& os) const override {
        os << "RemoveAt(index=" << mFromIndex << ", count=" << mCount << ")";
    }
};

template <typename SutNS> struct Pop : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    void checkPreconditions(const Model& m) const override { RC_PRE(!m.empty()); }
    void apply(Model& m) const override { m.pop_back(); }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        typename SutNS::Sut_item_t item = SutNS::Sut_pop(w.get());
        SutNS::Sut_item_t::delete_(&item);
    }
    void show(std::ostream& os) const override { os << "Pop()"; }
};
