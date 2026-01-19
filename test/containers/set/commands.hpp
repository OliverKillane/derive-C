#pragma once

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>
#include <unordered_set>

template <typename SutNS> using SutModel = std::unordered_set<typename SutNS::Sut_item_t>;

template <typename SutNS> struct SutWrapper {
    SutWrapper(SutNS::Sut s) : mSut(s) {}
    SutWrapper(const SutWrapper& other) : mSut(SutNS::Sut_clone(&other.mSut)) {}
    ~SutWrapper() { SutNS::Sut_delete(&mSut); }

    [[nodiscard]] SutNS::Sut* get() { return &mSut; }
    [[nodiscard]] SutNS::Sut const* getConst() const { return &mSut; }

    SutNS::Sut mSut;
};

template <typename SutNS> struct Command : rc::state::Command<SutModel<SutNS>, SutWrapper<SutNS>> {
    using Model = SutModel<SutNS>;
    using Wrapper = SutWrapper<SutNS>;

    virtual void runCommand(const Model& m, Wrapper& w) const = 0;
    virtual void AdditionalChecks(const Model& m, const Wrapper& w) const {}

    void CheckValues(const Model& m, const Wrapper& w) const {
        RC_ASSERT(m.size() == SutNS::Sut_size(w.getConst()));
        for (const auto& key : m) {
            RC_ASSERT(SutNS::Sut_contains(w.getConst(), key));
        }
    }

    void CheckIterators(const Model& /* m */, const Wrapper& w) const {
        Wrapper wrapperCopy = w;

        typename SutNS::Sut_iter_const iter_const = SutNS::Sut_get_iter_const(w.getConst());

        while (!SutNS::Sut_iter_const_empty(&iter_const)) {
            typename SutNS::Sut_iter_const_item item_const =
                SutNS::Sut_iter_const_next(&iter_const);
            RC_ASSERT(!SutNS::Sut_iter_const_empty_item(&item_const));
            RC_ASSERT(item_const != nullptr);
        }

        RC_ASSERT(SutNS::Sut_iter_const_empty(&iter_const));
        typename SutNS::Sut_iter_const_item item_const = SutNS::Sut_iter_const_next(&iter_const);
        RC_ASSERT(SutNS::Sut_iter_const_empty_item(&item_const));
    }

    void CheckFailedAccess(const Model& m, const Wrapper& w) const {
        Wrapper wrapperCopy = w;
        typename SutNS::Sut_item_t const invalid_key =
            *rc::gen::suchThat(rc::gen::arbitrary<typename SutNS::Sut_item_t>(),
                               [=](auto const& k) { return m.find(k) == m.end(); });
        RC_ASSERT(!SutNS::Sut_contains(w.getConst(), invalid_key));
        RC_ASSERT(!SutNS::Sut_try_remove(wrapperCopy.get(), invalid_key));
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

/*

Add
Remove

*/

template <typename SutNS> struct Add : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    typename SutNS::Sut_item_t mItem = *rc::gen::arbitrary<typename SutNS::Sut_item_t>();

    void checkPreconditions(const Model& m) const override {
        RC_PRE(m.size() < SutNS::Sut_max_capacity);
        RC_PRE(m.find(mItem) == m.end());
    }
    void apply(Model& s) const override { s.insert(mItem); }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        SutNS::Sut_add(w.get(), mItem);
    }
    void show(std::ostream& os) const override { os << "Add(" << mItem << ")"; }
};

template <typename SutNS> struct AddOverMaxSize : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    typename SutNS::Sut_item_t mItem = *rc::gen::arbitrary<typename SutNS::Sut_item_t>();

    void checkPreconditions(const Model& m) const override {
        RC_PRE(m.size() >= SutNS::Sut_max_capacity);

        // JUSTIFY: Also checking key uniqueness
        //  - This command should only check against max size
        RC_PRE(m.find(mItem) == m.end());
    }
    void apply(Model& /*m*/) const override {}
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        bool const inserted = SutNS::Sut_try_add(w.get(), mItem);
        RC_ASSERT(!inserted);
    }
    void show(std::ostream& os) const override { os << "InsertOverMaxSize(" << mItem << ")"; }
};

template <typename SutNS> struct DuplicateInsert : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<typename SutNS::Sut_item_t> mItem = std::nullopt;

    explicit DuplicateInsert(const Model& m) {
        if (!m.empty()) {
            std::vector<typename SutNS::Sut_item_t> keys;
            keys.reserve(m.size());
            for (const auto& k : m) {
                keys.push_back(k);
            }
            mItem = *rc::gen::elementOf(keys);
        }
    }

    void checkPreconditions(const Model& m) const override {
        // To avoid tripping other assertions on capacity
        RC_PRE(m.size() < SutNS::Sut_max_capacity);

        RC_PRE(mItem.has_value());
        RC_PRE(m.find(mItem.value()) != m.end());
    }

    void apply(Model& m) const override {
        // No-op, our hashmap does not replace on duplicate insert
    }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        RC_ASSERT(!SutNS::Sut_try_add(w.get(), mItem.value()));
    }

    void show(std::ostream& os) const override { os << "DuplicateInsert(" << mItem.value() << ")"; }
};

template <typename SutNS> struct Remove : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<typename SutNS::Sut_item_t> mItem = std::nullopt;

    explicit Remove(const Model& m) {
        if (!m.empty()) {
            std::vector<typename SutNS::Sut_item_t> keys;
            keys.reserve(m.size());
            for (const auto& k : m) {
                keys.push_back(k);
            }
            mItem = *rc::gen::elementOf(keys);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(mItem.has_value());
        RC_PRE(m.find(mItem.value()) != m.end());
    }

    void apply(Model& m) const override { RC_ASSERT(m.erase(mItem.value())); }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        SutNS::Sut_remove(w.get(), mItem.value());
    }

    void show(std::ostream& os) const override { os << "Remove(" << mItem.value() << ")"; }
};

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
