#include <gtest/gtest.h>
#include <limits>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include <vector>

using Data = size_t;
using Model = std::vector<Data>;

extern "C" {
#define PANIC abort()
#define SELF Sut
#define T Data
#include <derive-c/structures/vector.template.h>
}

struct SutWrapper {
    SutWrapper() : sut(Sut_new()) {}
    ~SutWrapper() { Sut_delete(&sut); }
    SutWrapper(const Sut& sut) : sut(Sut_clone(&sut)) {}
    SutWrapper& operator=(const SutWrapper& other) {
        if (this != &other) {
            Sut_delete(&sut);
            sut = Sut_clone(&other.sut);
        }
        return *this;
    }

    Sut* get() { return &sut; }
    Sut const* getConst() const { return &sut; }

    Sut sut;
};

struct Command : rc::state::Command<Model, SutWrapper> {
    virtual void runAndCheck(const Model& s0, SutWrapper& sut) const = 0;

    void run(const Model& s0, SutWrapper& sut) const override {
        runAndCheck(s0, sut);
        checkInvariants(s0, sut);
    }

    void checkInvariants(const Model& oldModel, const SutWrapper& sut) const {
        Model model = nextState(oldModel);
        RC_ASSERT(model.size() == Sut_size(sut.getConst()));
        for (size_t i = 0; i < model.size(); ++i) {
            RC_ASSERT(model[i] == *Sut_read(sut.getConst(), i));
        }

        // JUSTIFY: Separately checking the iterator implementations
        //           - Different access to SELF_read
        Sut_iter_const iter = Sut_get_iter_const(&sut.sut);
        while (!Sut_iter_const_empty(&iter)) {
            size_t pos = Sut_iter_const_position(&iter);
            Data item = *Sut_iter_const_next(&iter);
            RC_ASSERT(model[pos] == item);
        }
    }
};

struct Push : Command {
    Data value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Data>::max());

    void checkPreconditions(const Model& s0) const override {
        RC_PRE(s0.size() < std::numeric_limits<size_t>::max());
    }
    void apply(Model& s0) const override { s0.push_back(value); }
    void runAndCheck(const Model& s0, SutWrapper& sut) const override {
        Sut_push(sut.get(), value);
    }
    void show(std::ostream& os) const override { os << "Push(" << value << ")"; }
};

struct Write : Command {
    explicit Write(const Model& s0)
        : index(*rc::gen::inRange(static_cast<size_t>(0), s0.size() - 1)) {}

    size_t index;
    Data value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Data>::max());

    void checkPreconditions(const Model& s0) const override { RC_PRE(!s0.empty()); }
    void apply(Model& s0) const override { s0[index] = value; }
    void runAndCheck(const Model& s0, SutWrapper& sut) const override {
        Data* at = Sut_write(sut.get(), index);
        *at = value;
    }
    void show(std::ostream& os) const override { os << "Write(" << index << " = " << value << ")"; }
};


struct Pop : Command {

    void checkPreconditions(const Model& s0) const override { RC_PRE(!s0.empty()); }
    void apply(Model& s0) const override { s0.pop_back(); }
    void runAndCheck(const Model& s0, SutWrapper& sut) const override {
        Data value;
        RC_ASSERT(Sut_pop(sut.get(), &value));
    }
    void show(std::ostream& os) const override { os << "Pop()"; }
};

RC_GTEST_PROP(VectorTests, General, ()) {
    Model model;
    SutWrapper sut;
    rc::state::check(model, sut, rc::state::gen::execOneOfWithArgs<Push, Write, Pop>());
}
