#pragma once

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>
#include <deque>

namespace containers::queue {

template <typename SutNS> using SutModel = std::deque<typename SutNS::Sut_item_t>;

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

    void CheckSize(const Model& m, const Wrapper& w) const {
        RC_ASSERT(m.size() == SutNS::Sut_size(w.getConst()));
    }

    void CheckIterators(const Model& m, const Wrapper& w) const {
        Wrapper wrapperCopy = w;
        typename SutNS::Sut_iter iter = SutNS::Sut_get_iter(wrapperCopy.get());
        typename SutNS::Sut_iter_const iter_const = SutNS::Sut_get_iter_const(w.getConst());

        for (const auto& item : m) {
            RC_ASSERT(!SutNS::Sut_iter_const_empty(&iter_const));
            RC_ASSERT(!SutNS::Sut_iter_empty(&iter));
            const auto* sut_item_const = SutNS::Sut_iter_const_next(&iter_const);
            auto* sut_item = SutNS::Sut_iter_next(&iter);
            RC_ASSERT(*sut_item_const == item);
            RC_ASSERT(*sut_item == item);
        }
        RC_ASSERT(SutNS::Sut_iter_const_empty(&iter_const));
        RC_ASSERT(SutNS::Sut_iter_empty(&iter));

        RC_ASSERT(SutNS::Sut_iter_const_next(&iter_const) == nullptr);
        RC_ASSERT(SutNS::Sut_iter_next(&iter) == nullptr);
    }

    void CheckEmpty(const Model& m, const Wrapper& w) const {
        Wrapper wrapperCopy = w;
        if (!m.empty()) {
            RC_ASSERT(!SutNS::Sut_empty(w.getConst()));
            RC_ASSERT(!SutNS::Sut_empty(wrapperCopy.getConst()));

            for (size_t index = 0; index < m.size(); ++index) {
                RC_ASSERT(*SutNS::Sut_try_read_from_back(w.getConst(), index) ==
                          m[m.size() - 1 - index]);
                RC_ASSERT(*SutNS::Sut_try_write_from_back(wrapperCopy.get(), index) ==
                          m[m.size() - 1 - index]);
                RC_ASSERT(*SutNS::Sut_try_read_from_front(w.getConst(), index) == m[index]);
                RC_ASSERT(*SutNS::Sut_try_write_from_front(wrapperCopy.get(), index) == m[index]);
            }
        } else {
            RC_ASSERT(SutNS::Sut_empty(w.getConst()));
            RC_ASSERT(SutNS::Sut_empty(wrapperCopy.getConst()));

            RC_ASSERT(SutNS::Sut_try_read_from_back(w.getConst(), 0) == nullptr);
            RC_ASSERT(SutNS::Sut_try_write_from_back(wrapperCopy.get(), 0) == nullptr);
            RC_ASSERT(SutNS::Sut_try_read_from_front(w.getConst(), 0) == nullptr);
            RC_ASSERT(SutNS::Sut_try_write_from_front(wrapperCopy.get(), 0) == nullptr);
        }
    }

    void run(const Model& m, Wrapper& w) const override {
        // pass the current model through to runCommand so behavior matches the old
        // runAndCheck which used the model parameter.
        runCommand(m, w);

        Model next = this->nextState(m);
        CheckSize(next, w);
        CheckIterators(next, w);
        CheckEmpty(next, w);
        AdditionalChecks(next, w);
    }
};

template <typename SutNS> struct PushFront : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    SutNS::Sut_item_t mValue = *rc::gen::arbitrary<typename SutNS::Sut_item_t>();

    void checkPreconditions(const Model& m) const override {}
    void apply(Model& m) const override { m.push_front(mValue); }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        SutNS::Sut_push_front(w.get(), mValue);
    }
    void show(std::ostream& os) const override { os << "PushFront(" << mValue << ")"; }
};

template <typename SutNS> struct PushBack : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    SutNS::Sut_item_t mValue = *rc::gen::arbitrary<typename SutNS::Sut_item_t>();

    void checkPreconditions(const Model& m) const override {}
    void apply(Model& m) const override { m.push_back(mValue); }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        SutNS::Sut_push_back(w.get(), mValue);
    }
    void show(std::ostream& os) const override { os << "PushFront(" << mValue << ")"; }
};

template <typename SutNS> struct PopFront : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    void checkPreconditions(const Model& m) const override { RC_PRE(!m.empty()); }
    void apply(Model& m) const override { m.pop_front(); }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        SutNS::Sut_pop_front(w.get());
    }
    void show(std::ostream& os) const override { os << "PopFront()"; }
};

template <typename SutNS> struct PopBack : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    void checkPreconditions(const Model& m) const override { RC_PRE(!m.empty()); }
    void apply(Model& m) const override { m.pop_back(); }
    void runCommand(const Model& /*m*/, Wrapper& w) const override { SutNS::Sut_pop_back(w.get()); }
    void show(std::ostream& os) const override { os << "PopBack()"; }
};

} // namespace containers::queue
