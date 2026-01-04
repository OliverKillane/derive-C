#pragma once

#include <cstring>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>
#include <unordered_map>
#include <vector>
#include <memory>

#include <derive-c/core/debug/memory_tracker.h>

#define NAME debug_builder
#include <derive-c/utils/string_builder/template.h>

using ModelIndex = size_t;

struct ModelIndexGen {
    ModelIndex increment() { return mIndex++; }
    ModelIndex next() const { return mIndex; }
    ModelIndex mIndex{0};
};

struct SutModel {
    ModelIndexGen mModelIndexGen;

    struct Allocation {
        char mFill;
        size_t mSize;
    };

    std::unordered_map<ModelIndex, Allocation> mAllocations;
};

template <typename SutNS> struct SutWrapper {
    SutWrapper(SutNS::Sut sut) : mSut(sut) {}
    SutWrapper(const SutWrapper& other) = delete;
    ~SutWrapper() {
        for (auto& [_, ptr] : mAllocations) {
            SutNS::Sut_free(&mSut, ptr);
        }
        SutNS::Sut_delete(&mSut);
    }

    [[nodiscard]] SutNS::Sut* get() { return &mSut; }
    [[nodiscard]] SutNS::Sut const* getConst() const { return &mSut; }

    SutNS::Sut mSut;
    std::unordered_map<ModelIndex, void*> mAllocations;
};

static char GetFill(size_t num_allocations) {
    return static_cast<char>(static_cast<unsigned char>(num_allocations % 256));
}

template <typename SutNS> struct Command : rc::state::Command<SutModel, SutWrapper<SutNS>> {
    using Model = SutModel;
    using Wrapper = SutWrapper<SutNS>;

    static std::optional<ModelIndex> GetModelIndex(const Model& m) {
        if (!m.mAllocations.empty()) {
            std::vector<ModelIndex> indices;
            indices.reserve(m.mAllocations.size());
            for (const auto& [k, _] : m.mAllocations) {
                indices.push_back(k);
            }
            return *rc::gen::elementOf(indices);
        }
        return std::nullopt;
    }

    virtual void runCommand(const Model& m, Wrapper& w) const = 0;
    virtual void AdditionalChecks(const Model& m, const Wrapper& w) const {}

    void run(const Model& m, Wrapper& w) const override {
        runCommand(m, w);

        Model next = this->nextState(m);
        for (const auto& [idx, alloc] : next.mAllocations) {
            void* ptr = w.mAllocations.at(idx);
            char* char_ptr = static_cast<char*>(ptr);
            for (size_t i = 0; i < alloc.mSize; ++i) {
                RC_ASSERT(char_ptr[i] == alloc.mFill);
            }
        }
    }
};

template <typename SutNS> struct Malloc : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    size_t mSize;
    ModelIndex mIndex;
    char mFill;

    Malloc(const Model& m)
        : mSize(*rc::gen::inRange<size_t>(1, 1000)), mIndex(m.mModelIndexGen.next()),
          mFill(GetFill(mIndex)) {}

    void checkPreconditions(const Model& m) const override {}

    void apply(Model& m) const override {
        ModelIndex index = m.mModelIndexGen.increment();
        RC_ASSERT(index == mIndex);
        m.mAllocations[index] = {
            .mFill = mFill,
            .mSize = mSize,
        };
    }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        void* ptr = SutNS::Sut_malloc(w.get(), mSize);
        bool const success = ptr != nullptr;
        RC_ASSERT(success);

        // Check for correct msan/asan poisoning
        dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, ptr,
                                mSize);

        // Fill with data (for checking overwrites)
        std::memset(ptr, mFill, mSize);

        // Sync with the model
        w.mAllocations[mIndex] = ptr;
    }
    void show(std::ostream& os) const override {
        os << "Malloc(index=" << mIndex << ", size=" << mSize << ", fill=" << mFill << ")";
    }
};

template <typename SutNS> struct Calloc : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    size_t mItems;
    size_t mItemSize;
    ModelIndex mIndex;
    char mFill;

    Calloc(const Model& m)
        : mItems(*rc::gen::inRange<size_t>(1, 100)), mItemSize(*rc::gen::inRange<size_t>(1, 10)),
          mIndex(m.mModelIndexGen.next()), mFill(GetFill(mIndex)) {}

    void checkPreconditions(const Model& m) const override {}

    void apply(Model& m) const override {
        ModelIndex index = m.mModelIndexGen.increment();
        RC_ASSERT(index == mIndex);
        m.mAllocations[index] = {
            .mFill = mFill,
            .mSize = mItems * mItemSize,
        };
    }

    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        void* ptr = SutNS::Sut_calloc(w.get(), mItems, mItemSize);
        bool const success = ptr != nullptr;
        RC_ASSERT(success);

        // Check for correct msan/asan poisoning
        dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE, ptr,
                                mItems * mItemSize);

        //  Check all bytes zeroed
        char const* char_ptr = static_cast<char const*>(ptr);
        for (size_t i = 0; i < mItems * mItemSize; ++i) {
            RC_ASSERT(char_ptr[i] == 0);
        }

        // Fill with data (for checking overwrites)
        std::memset(ptr, mFill, mItems * mItemSize);

        // Sync with the model
        w.mAllocations[mIndex] = ptr;
    }
    void show(std::ostream& os) const override {
        os << "Calloc(index=" << mIndex << ", items=" << mItems << ", itemSize=" << mItemSize
           << ", fill=" << mFill << ")";
    }
};

template <typename SutNS> struct Free : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<ModelIndex> mIndex;

    explicit Free(const Model& m) : mIndex(Base::GetModelIndex(m)) {}

    void checkPreconditions(const Model& /*m*/) const override { RC_PRE(mIndex.has_value()); }

    void apply(Model& m) const override { m.mAllocations.erase(mIndex.value()); }
    void runCommand(const Model& m, Wrapper& w) const override {
        void* ptr = w.mAllocations.at(mIndex.value());
        size_t const size = m.mAllocations.at(mIndex.value()).mSize;

        SutNS::Sut_free(w.get(), ptr);

        dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, ptr, size);
        w.mAllocations.erase(mIndex.value());
    }
    void show(std::ostream& os) const override { os << "Free(index=" << mIndex.value() << ")"; }
};

template <typename SutNS> struct ReallocLarger : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<ModelIndex> mIndex;
    size_t mIncreaseBy;

    explicit ReallocLarger(const Model& m)
        : mIndex(Base::GetModelIndex(m)), mIncreaseBy(*rc::gen::inRange<size_t>(0, 100)) {}

    void checkPreconditions(const Model& /*m*/) const override { RC_PRE(mIndex.has_value()); }

    void apply(Model& m) const override { m.mAllocations.at(mIndex.value()).mSize += mIncreaseBy; }

    void runCommand(const Model& m, Wrapper& w) const override {
        void* original_ptr = w.mAllocations.at(mIndex.value());
        const auto& alloc = m.mAllocations.at(mIndex.value());
        void* new_ptr = SutNS::Sut_realloc(w.get(), original_ptr, alloc.mSize + mIncreaseBy);

        // Update tracking for index
        w.mAllocations[mIndex.value()] = new_ptr;

        if (new_ptr != original_ptr) {
            // Check the bytes for the old allocation are poisoned
            dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE,
                                    original_ptr, alloc.mSize);
        }

        // Check the original bytes are unchanged
        char const* original_start = static_cast<char const*>(new_ptr);
        dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                                original_start, alloc.mSize);
        for (size_t offset = 0; offset < alloc.mSize; offset++) {
            RC_ASSERT(original_start[offset] == alloc.mFill);
        }

        // Check the new bytes are correctly poisoned
        if (mIncreaseBy > 0) {
            char* start_of_extension = static_cast<char*>(new_ptr) + alloc.mSize;
            dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                                    static_cast<char*>(new_ptr) + alloc.mSize, mIncreaseBy);
            // Set new bytes
            std::memset(start_of_extension, alloc.mFill, mIncreaseBy);
        }
    }
    void show(std::ostream& os) const override {
        os << "ReallocLarger(index=" << mIndex.value() << ", increaseBy=" << mIncreaseBy << ")";
    }
};

template <typename SutNS> struct ReallocSmaller : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<ModelIndex> mIndex;
    size_t mNewSize;

    explicit ReallocSmaller(const Model& m)
        : mIndex(Base::GetModelIndex(m)),
          mNewSize(mIndex.has_value()
                       ? *rc::gen::inRange<size_t>(1, m.mAllocations.at(mIndex.value()).mSize)
                       : 0) {}

    void checkPreconditions(const Model& /*m*/) const override { RC_PRE(mIndex.has_value()); }
    void apply(Model& m) const override { m.mAllocations.at(mIndex.value()).mSize = mNewSize; }
    void runCommand(const Model& m, Wrapper& w) const override {
        const auto& alloc = m.mAllocations.at(mIndex.value());
        void* original_ptr = w.mAllocations.at(mIndex.value());

        void* new_ptr = SutNS::Sut_realloc(w.get(), original_ptr, mNewSize);
        w.mAllocations[mIndex.value()] = new_ptr;

        if (new_ptr == original_ptr) {
            // Need to check that the memory removed was poisoned
            char const* end = static_cast<char const*>(original_ptr) + mNewSize;
            size_t poisonedBytes = alloc.mSize - mNewSize;
            dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, end,
                                    poisonedBytes);
        } else {
            // Need to check the entire old allocation was poisoned
            dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE,
                                    original_ptr, alloc.mSize);
        }

        // Check the data is preserved
        char* char_ptr = static_cast<char*>(new_ptr);
        for (size_t i = 0; i < mNewSize; ++i) {
            RC_ASSERT(char_ptr[i] == alloc.mFill);
        }
    }
    void show(std::ostream& os) const override { os << "ReallocSmaller(index=" << mIndex.value() << ", size=" << mNewSize << ")"; }
};
