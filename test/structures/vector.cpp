#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include <limits>
#include <vector>

namespace vector {

using Data = size_t;
using Model = std::vector<Data>;

extern "C" {
#include <derive-c/allocs/std.h>

#define SELF Sut
#define T Data
#include <derive-c/structures/vector/template.h>
}

static const int MAX_SIZE = 100;
Sut newSut(size_t size) {
    if (size == 0) {
        return Sut_new(stdalloc_get());
    }
    return Sut_new_with_capacity(size, stdalloc_get());
}

struct SutWrapper {
    SutWrapper() : sut(newSut(*rc::gen::inRange(0, MAX_SIZE))) {}
    ~SutWrapper() { Sut_delete(&sut); }
    SutWrapper(const Sut& sut) : sut(Sut_shallow_clone(&sut)) {}
    SutWrapper& operator=(const SutWrapper& other) {
        if (this != &other) {
            Sut_delete(&sut);
            sut = Sut_shallow_clone(&other.sut);
        }
        return *this;
    }

    [[nodiscard]] Sut* get() { return &sut; }
    [[nodiscard]] Sut const* getConst() const { return &sut; }

    Sut sut;
};

struct Command : rc::state::Command<Model, SutWrapper> {
    virtual void runAndCheck(const Model& m, SutWrapper& sut) const = 0;

    void run(const Model& m, SutWrapper& sut) const override {
        runAndCheck(m, sut);
        checkInvariants(m, sut);
    }

    void checkInvariants(const Model& oldModel, const SutWrapper& s) const {
        Model m = nextState(oldModel);
        RC_ASSERT(m.size() == Sut_size(s.getConst()));
        for (size_t i = 0; i < m.size(); ++i) {
            RC_ASSERT(m[i] == *Sut_read(s.getConst(), i));
        }

        // JUSTIFY: Separately checking the iterator implementations
        //           - Different access to SELF_read
        Sut_iter_const iter = Sut_get_iter_const(&s.sut);
        while (!Sut_iter_const_empty(&iter)) {
            size_t pos = Sut_iter_const_position(&iter);
            Data item = *Sut_iter_const_next(&iter);
            RC_ASSERT(m[pos] == item);
        }
    }
};

struct Push : Command {
    Data value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Data>::max());

    void checkPreconditions(const Model& m) const override {
        RC_PRE(m.size() < std::numeric_limits<size_t>::max());
    }
    void apply(Model& m) const override { m.push_back(value); }
    void runAndCheck(const Model& m, SutWrapper& s) const override { Sut_push(s.get(), value); }
    void show(std::ostream& os) const override { os << "Push(" << value << ")"; }
};

struct Write : Command {
    std::optional<size_t> index = std::nullopt;
    Data value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Data>::max());

    explicit Write(const Model& m) {
        if (!m.empty()) {
            index = *rc::gen::inRange(static_cast<size_t>(0), m.size() - 1);
        }
    }

    void checkPreconditions(const Model& m) const override {
        RC_PRE(index.has_value());
        RC_PRE(index.value() < m.size());
    }
    void apply(Model& m) const override { m[index.value()] = value; }
    void runAndCheck(const Model& m, SutWrapper& s) const override {
        Data* at = Sut_write(s.get(), index.value());
        *at = value;
    }
    void show(std::ostream& os) const override {
        os << "Write(" << index.value() << " = " << value << ")";
    }
};

struct Pop : Command {

    void checkPreconditions(const Model& m) const override { RC_PRE(!m.empty()); }
    void apply(Model& m) const override { m.pop_back(); }
    void runAndCheck(const Model& m, SutWrapper& sut) const override { Sut_pop(sut.get()); }
    void show(std::ostream& os) const override { os << "Pop()"; }
};

RC_GTEST_PROP(VectorTests, General, ()) {
    Model model;
    SutWrapper sut;
    rc::state::check(model, sut, rc::state::gen::execOneOfWithArgs<Push, Push, Push, Write, Pop>());
}

TEST(VectorTests, CreateWithDefaults) {
    Sut sut = Sut_new_with_defaults(128, 3, stdalloc_get());
    ASSERT_EQ(Sut_size(&sut), 128);

    for (size_t i = 0; i < Sut_size(&sut); ++i) {
        ASSERT_EQ(*Sut_read(&sut, i), 3);
    }

    Sut_delete(&sut);
}

TEST(VectorTests, CreateWithZeroSize) {
    Sut sut_1 = Sut_new(stdalloc_get());
    ASSERT_EQ(Sut_size(&sut_1), 0);
    Sut_delete(&sut_1);

    Sut sut_2 = Sut_new(stdalloc_get());
    ASSERT_EQ(Sut_size(&sut_2), 0);
    Sut_delete(&sut_2);
}

TEST(VectorTests, CreateWithCapacity) {
    Sut sut = Sut_new_with_capacity(64, stdalloc_get());
    ASSERT_EQ(Sut_size(&sut), 0);
    Sut_delete(&sut);
}

TEST(VectorTests, FailedAccesses) {
    Sut sut = Sut_new_with_capacity(64, stdalloc_get());
    ASSERT_EQ(Sut_size(&sut), 0);

    ASSERT_EQ(Sut_try_read(&sut, 0), nullptr);
    ASSERT_EQ(Sut_try_read(&sut, 63), nullptr);
    ASSERT_EQ(Sut_try_write(&sut, 0), nullptr);

    Sut_delete(&sut);
}

TEST(VectorTests, FailedPop) {
    Sut sut = Sut_new_with_capacity(64, stdalloc_get());
    ASSERT_EQ(Sut_size(&sut), 0);

    Data value;
    ASSERT_FALSE(Sut_try_pop(&sut, &value));
    ASSERT_EQ(Sut_size(&sut), 0);

    Sut_delete(&sut);
}

TEST(VectorTests, IteratorEdgeCases) {
    Sut sut = Sut_new(stdalloc_get());

    const size_t upto = 100;

    for (size_t i = 0; i < upto; ++i) {
        Sut_push(&sut, i);
    }

    Sut_iter_const iter_const = Sut_get_iter_const(&sut);
    while (!Sut_iter_const_empty(&iter_const)) {
        size_t pos = Sut_iter_const_position(&iter_const);
        const Data* data = Sut_iter_const_next(&iter_const);
        ASSERT_EQ(pos, *data);
    }
    ASSERT_EQ(Sut_iter_const_next(&iter_const), nullptr);
    ASSERT_EQ(Sut_iter_const_position(&iter_const), upto);

    Sut_iter iter = Sut_get_iter(&sut);
    while (!Sut_iter_empty(&iter)) {
        size_t pos = Sut_iter_position(&iter);
        Data* data = Sut_iter_next(&iter);
        ASSERT_EQ(pos, *data);
    }
    ASSERT_EQ(Sut_iter_next(&iter), nullptr);
    ASSERT_EQ(Sut_iter_position(&iter), upto);
    Sut_delete(&sut);
}

TEST(VectorTests, ShallowClone) {
    Sut sut = Sut_new_with_defaults(100, 3, stdalloc_get());
    ASSERT_EQ(Sut_size(&sut), 100);

    Sut cloned_sut = Sut_shallow_clone(&sut);
    ASSERT_EQ(Sut_size(&cloned_sut), 100);

    for (size_t i = 0; i < Sut_size(&cloned_sut); ++i) {
        ASSERT_EQ(*Sut_read(&cloned_sut, i), *Sut_read(&sut, i));
    }

    Sut_delete(&sut);
    Sut_delete(&cloned_sut);
}

} // namespace vector