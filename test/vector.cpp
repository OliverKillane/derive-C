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
    void runAndCheck(const Model& m, SutWrapper& sut) const override {
        Sut_popped_entry entry = Sut_pop(sut.get());
        RC_ASSERT(entry.present);
    }
    void show(std::ostream& os) const override { os << "Pop()"; }
};

RC_GTEST_PROP(VectorTests, General, ()) {
    Model model;
    SutWrapper sut;
    rc::state::check(model, sut, rc::state::gen::execOneOfWithArgs<Push>());
}
} // namespace vector