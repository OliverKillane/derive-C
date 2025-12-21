#pragma once

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>
#include <unordered_map>

template <typename SutNS>
using SutModel = std::unordered_map<typename SutNS::Sut_key_t, typename SutNS::Sut_value_t>;

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

    void CheckValues(const Model& m, const Wrapper& w) const {
        RC_ASSERT(m.size() == SutNS::Sut_size(w.getConst()));
        for (const auto& [key, value] : m) {
            typename SutNS::Sut_value_t const* val = SutNS::Sut_read(w.getConst(), key);
            RC_ASSERT(*val == value);
        }
    }

    void CheckIterators(const Model& m, const Wrapper& w) const {
        Wrapper wrapperCopy = w;

        typename SutNS::Sut_iter_const iter_const = SutNS::Sut_get_iter_const(w.getConst());
        typename SutNS::Sut_iter iter = SutNS::Sut_get_iter(wrapperCopy.get());

        typename SutNS::Sut_iter_const_item item_const = SutNS::Sut_iter_const_next(&iter_const);
        typename SutNS::Sut_iter_item item = SutNS::Sut_iter_next(&iter);

        while (!SutNS::Sut_iter_const_empty_item(&item_const)) {
            RC_ASSERT(item_const.key != nullptr && item_const.value != nullptr);
            RC_ASSERT(item.key != nullptr && item.value != nullptr);

            RC_ASSERT(m.at(*item_const.key) == *item_const.value);
            RC_ASSERT(m.at(*item.key) == *item.value);

            item_const = SutNS::Sut_iter_const_next(&iter_const);
            item = SutNS::Sut_iter_next(&iter);
        }

        item = SutNS::Sut_iter_next(&iter);
        item_const = SutNS::Sut_iter_const_next(&iter_const);
        RC_ASSERT(SutNS::Sut_iter_empty_item(&item));
        RC_ASSERT(SutNS::Sut_iter_const_empty_item(&item_const));
    }

    void CheckFailedAccess(const Model& m, const Wrapper& w) const {
        Wrapper wrapperCopy = w;
        typename SutNS::Sut_key_t invalid_key = 0;

        while (m.find(invalid_key) != m.end()) {
            invalid_key++;
        }
        RC_ASSERT(SutNS::Sut_try_read(w.getConst(), invalid_key) == nullptr);
        RC_ASSERT(SutNS::Sut_try_write(wrapperCopy.get(), invalid_key) == nullptr);
        typename SutNS::Sut_value_t removed;
        RC_ASSERT(!SutNS::Sut_try_remove(wrapperCopy.get(), invalid_key, &removed));
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

    typename SutNS::Sut_value_t mKey =
        *rc::gen::inRange(std::numeric_limits<typename SutNS::Sut_key_t>::min(),
                          std::numeric_limits<typename SutNS::Sut_key_t>::max());

    typename SutNS::Sut_value_t mValue =
        *rc::gen::inRange(std::numeric_limits<typename SutNS::Sut_value_t>::min(),
                          std::numeric_limits<typename SutNS::Sut_value_t>::max());

    void checkPreconditions(const Model& s) const override { RC_PRE(s.find(mKey) == s.end()); }
    void apply(Model& s) const override { s[mKey] = mValue; }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        typename SutNS::Sut_value_t* foundValue = SutNS::Sut_insert(w.get(), mKey, mValue);
        RC_ASSERT(foundValue != nullptr && *foundValue == mValue);
    }
    void show(std::ostream& os) const override { os << "Insert(" << mKey << ", " << mValue << ")"; }
};

template <typename SutNS> struct Write : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<typename SutNS::Sut_key_t> mKey = std::nullopt;
    typename SutNS::Sut_value_t mValue =
        *rc::gen::inRange(std::numeric_limits<typename SutNS::Sut_value_t>::min(),
                          std::numeric_limits<typename SutNS::Sut_value_t>::max());

    explicit Write(const Model& m) {
        if (!m.empty()) {
            std::vector<typename SutNS::Sut_key_t> keys;
            keys.reserve(m.size());
            for (const auto& [k, _] : m) {
                keys.push_back(k);
            }
            mKey = *rc::gen::elementOf(keys);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(mKey.has_value());
        RC_PRE(m.find(mKey.value()) != m.end());
    }

    void apply(Model& m) const override { m[mKey.value()] = mValue; }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        typename SutNS::Sut_value_t* foundValue = SutNS::Sut_write(w.get(), mKey.value());
        *foundValue = mValue;
    }

    void show(std::ostream& os) const override {
        os << "Write(" << mKey.value() << " = " << mValue << ")";
    }
};

template <typename SutNS> struct DuplicateInsert : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<typename SutNS::Sut_key_t> mKey = std::nullopt;
    typename SutNS::Sut_value_t mValue =
        *rc::gen::inRange(std::numeric_limits<typename SutNS::Sut_value_t>::min(),
                          std::numeric_limits<typename SutNS::Sut_value_t>::max());

    explicit DuplicateInsert(const Model& m) {
        if (!m.empty()) {
            std::vector<typename SutNS::Sut_key_t> keys;
            keys.reserve(m.size());
            for (const auto& [k, _] : m) {
                keys.push_back(k);
            }
            mKey = *rc::gen::elementOf(keys);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(mKey.has_value());
        RC_PRE(m.find(mKey.value()) != m.end());
    }

    void apply(Model& m) const override {
        // No-op, our hashmap does not replace on duplicate insert
    }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        RC_ASSERT(!SutNS::Sut_try_insert(w.get(), mKey.value(), mValue));
    }

    void show(std::ostream& os) const override {
        os << "DuplicateInsert(" << mKey.value() << ", " << mValue << ")";
    }
};

template <typename SutNS> struct Remove : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<typename SutNS::Sut_key_t> mKey = std::nullopt;

    explicit Remove(const Model& m) {
        if (!m.empty()) {
            std::vector<typename SutNS::Sut_key_t> keys;
            keys.reserve(m.size());
            for (const auto& [k, _] : m) {
                keys.push_back(k);
            }
            mKey = *rc::gen::elementOf(keys);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(mKey.has_value());
        RC_PRE(m.find(mKey.value()) != m.end());
    }

    void apply(Model& m) const override { RC_ASSERT(m.erase(mKey.value())); }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        SutNS::Sut_remove(w.get(), mKey.value());
    }

    void show(std::ostream& os) const override { os << "Remove(" << mKey.value() << ")"; }
};

template <typename SutNS> struct DeleteEntry : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<typename SutNS::Sut_key_t> mKey = std::nullopt;

    explicit DeleteEntry(const Model& m) {
        if (!m.empty()) {
            std::vector<typename SutNS::Sut_key_t> keys;
            keys.reserve(m.size());
            for (const auto& [k, _] : m) {
                keys.push_back(k);
            }
            mKey = *rc::gen::elementOf(keys);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(mKey.has_value());
        RC_PRE(m.find(mKey.value()) != m.end());
    }

    void apply(Model& m) const override {
        if (mKey.has_value()) {
            RC_ASSERT(m.erase(mKey.value()));
        }
    }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        if (mKey.has_value()) {
            SutNS::Sut_delete_entry(w.get(), mKey.value());
        }
    }

    void show(std::ostream& os) const override { os << "DeleteEntry(" << mKey.value() << ")"; }
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
