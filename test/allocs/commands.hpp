#pragma once

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>
#include <unordered_map>
#include <vector>
#include <memory>

#include <derive-c/core/debug/memory_tracker.h>

using ModelIndex = size_t;

struct ModelIndexGen {
    ModelIndex next() { return mIndex++; }
    ModelIndex mIndex{0};
};

struct SutModel {
    ModelIndexGen mModelIndexGen;
    std::unordered_map<ModelIndex, size_t> mSizes;
};

struct IAlloc {
    virtual ~IAlloc();
};

template <typename SutNS> struct AllocWrapper : IAlloc {
    SutNS::Sut mSut;
    ~AllocWrapper() override { SutNS::Sut_delete(&mSut); }
};

template <typename SutNS> struct SutWrapper {
    SutWrapper(SutNS::Sut sut) : mSut(sut) {}
    SutWrapper(const SutWrapper& other) = delete;
    ~SutWrapper() {
        for (auto& [_, alloc] : mAllocations) {
            SutNS::Sut_free(&mSut, alloc.ptr);
        }
        SutNS::Sut_delete(&mSut);
    }

    [[nodiscard]] SutNS::Sut* get() { return &mSut; }
    [[nodiscard]] SutNS::Sut const* getConst() const { return &mSut; }

    SutNS::Sut mSut;

    struct Data {
        void* ptr;
        char fill_byte;
        size_t size;
    };
    ModelIndexGen mIndexGen;
    std::unordered_map<ModelIndex, Data> mAllocations;
};

static char GetFill(size_t num_allocations) {
    return static_cast<char>(static_cast<unsigned char>(num_allocations % 256));
}

template <typename SutNS> struct Command : rc::state::Command<SutModel, SutWrapper<SutNS>> {
    using Model = SutModel;
    using Wrapper = SutWrapper<SutNS>;

    virtual void runCommand(const Model& m, Wrapper& w) const = 0;
    virtual void AdditionalChecks(const Model& m, const Wrapper& w) const {}

    void run(const Model& m, Wrapper& w) const override {
        runCommand(m, w);

        Model next = this->nextState(m);
        for (const auto& [idx, data] : w.mAllocations) {
            bool validPtr = data.ptr != nullptr;
            RC_ASSERT(validPtr);
            char* char_ptr = static_cast<char*>(data.ptr);
            for (size_t i = 0; i < data.size; ++i) {
                RC_ASSERT(char_ptr[i] == data.fill_byte);
            }
        }
    }
};

template <typename SutNS> struct Malloc : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    size_t mSize = *rc::gen::inRange<size_t>(1, 1000);

    void checkPreconditions(const Model& m) const override {}
    void apply(Model& m) const override {
        ModelIndex model_index = m.mModelIndexGen.next();
        m.mSizes[model_index] = mSize;
    }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        char fill = GetFill(w.mAllocations.size());
        void* ptr = SutNS::Sut_malloc(w.get(), mSize);
        if (ptr != nullptr) {
            char* char_ptr = static_cast<char*>(ptr);
            for (size_t i = 0; i < mSize; ++i) {
                char_ptr[i] = fill;
            }
            dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE, ptr,
                                    mSize);
            ModelIndex alloc_index = w.mIndexGen.next();
            w.mAllocations[alloc_index] = {ptr, fill, mSize};
        }
    }
    void show(std::ostream& os) const override { os << "Malloc(" << mSize << ")"; }
};

template <typename SutNS> struct Free : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<ModelIndex> mIndex = std::nullopt;

    explicit Free(const Model& m) {
        if (!m.mSizes.empty()) {
            std::vector<ModelIndex> indices;
            indices.reserve(m.mSizes.size());
            for (const auto& [k, _] : m.mSizes) {
                indices.push_back(k);
            }
            mIndex = *rc::gen::elementOf(indices);
        }
    }

    void checkPreconditions(const Model& /*m*/) const override { RC_PRE(mIndex.has_value()); }
    void apply(Model& m) const override { m.mSizes.erase(mIndex.value()); }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        const auto& data = w.mAllocations.at(mIndex.value());
        void* ptr = data.ptr;
        size_t size = data.size;
        SutNS::Sut_free(w.get(), ptr);
        dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, ptr, size);
        w.mAllocations.erase(mIndex.value());
    }
    void show(std::ostream& os) const override { os << "Free()"; }
};

template <typename SutNS> struct Calloc : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    size_t mNmemb = *rc::gen::inRange<size_t>(1, 100);
    size_t mSize = *rc::gen::inRange<size_t>(1, 10);

    void checkPreconditions(const Model& m) const override {}
    void apply(Model& m) const override {
        ModelIndex model_index = m.mModelIndexGen.next();
        size_t total_size = mNmemb * mSize;
        m.mSizes[model_index] = total_size;
    }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        void* ptr = SutNS::Sut_calloc(w.get(), mNmemb, mSize);
        if (ptr != nullptr) {
            char* char_ptr = static_cast<char*>(ptr);
            size_t total_size = mNmemb * mSize;
            for (size_t i = 0; i < total_size; ++i) {
                RC_ASSERT(char_ptr[i] == 0);
            }
            char fill = GetFill(w.mAllocations.size());
            for (size_t i = 0; i < total_size; ++i) {
                char_ptr[i] = fill;
            }
            dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                                    ptr, total_size);
            ModelIndex alloc_index = w.mIndexGen.next();
            w.mAllocations[alloc_index] = {ptr, fill, total_size};
        }
    }
    void show(std::ostream& os) const override {
        os << "Calloc(" << mNmemb << ", " << mSize << ")";
    }
};

template <typename SutNS> struct ReallocLarger : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<ModelIndex> mIndex = std::nullopt;
    size_t mNewSize = 0;

    explicit ReallocLarger(const Model& m) {
        if (!m.mSizes.empty()) {
            std::vector<ModelIndex> indices;
            indices.reserve(m.mSizes.size());
            for (const auto& [k, _] : m.mSizes) {
                indices.push_back(k);
            }
            mIndex = *rc::gen::elementOf(indices);
            if (mIndex.has_value()) {
                size_t old_size = m.mSizes.at(mIndex.value());
                mNewSize = *rc::gen::inRange<size_t>(old_size + 1, old_size + 1000);
            }
        }
    }

    void checkPreconditions(const Model& /*m*/) const override { RC_PRE(mIndex.has_value()); }
    void apply(Model& m) const override { m.mSizes.at(mIndex.value()) = mNewSize; }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        const auto& data = w.mAllocations.at(mIndex.value());
        // Ensure old data is correct before realloc
        for (size_t i = 0; i < data.size; ++i) {
            RC_ASSERT(static_cast<char*>(data.ptr)[i] == data.fill_byte);
        }
        void* old_ptr = data.ptr;
        char fill = data.fill_byte;
        void* new_ptr = SutNS::Sut_realloc(w.get(), old_ptr, mNewSize);
        if (new_ptr == nullptr) {
            // If realloc fails, check that the original data is unchanged
            for (size_t i = 0; i < data.size; ++i) {
                RC_ASSERT(static_cast<char*>(data.ptr)[i] == data.fill_byte);
            }
        } else {
            char* char_ptr = static_cast<char*>(new_ptr);
            for (size_t i = 0; i < data.size; ++i) {
                RC_ASSERT(char_ptr[i] == fill);
            }
            for (size_t i = data.size; i < mNewSize; ++i) {
                char_ptr[i] = fill;
            }
            dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                                    new_ptr, data.size);
            dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_WRITE,
                                    static_cast<char*>(new_ptr) + data.size, mNewSize - data.size);
            w.mAllocations[mIndex.value()] = {new_ptr, fill, mNewSize};
        }
    }
    void show(std::ostream& os) const override { os << "ReallocLarger()"; }
};

template <typename SutNS> struct ReallocSmaller : Command<SutNS> {
    using Base = Command<SutNS>;
    using typename Base::Model;
    using typename Base::Wrapper;

    std::optional<ModelIndex> mIndex = std::nullopt;
    size_t mNewSize = 0;

    explicit ReallocSmaller(const Model& m) {
        if (!m.mSizes.empty()) {
            std::vector<ModelIndex> indices;
            indices.reserve(m.mSizes.size());
            for (const auto& [k, _] : m.mSizes) {
                indices.push_back(k);
            }
            mIndex = *rc::gen::elementOf(indices);
            if (mIndex.has_value()) {
                size_t old_size = m.mSizes.at(mIndex.value());
                mNewSize = *rc::gen::inRange<size_t>(1, old_size);
            }
        }
    }

    void checkPreconditions(const Model& /*m*/) const override { RC_PRE(mIndex.has_value()); }
    void apply(Model& m) const override { m.mSizes.at(mIndex.value()) = mNewSize; }
    void runCommand(const Model& /*m*/, Wrapper& w) const override {
        const auto& data = w.mAllocations.at(mIndex.value());
        // Ensure old data is correct before realloc
        for (size_t i = 0; i < data.size; ++i) {
            RC_ASSERT(static_cast<char*>(data.ptr)[i] == data.fill_byte);
        }
        void* old_ptr = data.ptr;
        char fill = data.fill_byte;
        void* new_ptr = SutNS::Sut_realloc(w.get(), old_ptr, mNewSize);
        if (new_ptr == nullptr) {
            // If realloc fails, check that the original data is unchanged
            for (size_t i = 0; i < data.size; ++i) {
                RC_ASSERT(static_cast<char*>(data.ptr)[i] == data.fill_byte);
            }
        } else {
            char* char_ptr = static_cast<char*>(new_ptr);
            for (size_t i = 0; i < mNewSize; ++i) {
                RC_ASSERT(char_ptr[i] == fill);
            }
            dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                                    new_ptr, mNewSize);
            dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE,
                                    static_cast<char*>(new_ptr) + mNewSize, data.size - mNewSize);
            w.mAllocations[mIndex.value()] = {new_ptr, fill, mNewSize};
        }
    }
    void show(std::ostream& os) const override { os << "ReallocSmaller()"; }
};
