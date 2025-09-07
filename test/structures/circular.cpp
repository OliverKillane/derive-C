#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include <deque>

using Data = int;
using Model = std::deque<Data>;

extern "C" {
#define NAME Sut
#define D Data
#include <derive-c/structures/circular/template.h>
}

struct SutWrapper {
    static const int MAX_SIZE = 100;

    SutWrapper() : sut(Sut_new_with_capacity_for(*rc::gen::inRange(0, MAX_SIZE), stdalloc_get())) {}
    SutWrapper(const SutWrapper& other) : sut(Sut_clone(&other.sut)) {}
    ~SutWrapper() { Sut_delete(&sut); }

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

        SutWrapper wrapperCopy = s;
        Sut_iter iter = Sut_get_iter(wrapperCopy.get());
        Sut_iter_const iter_const = Sut_get_iter_const(s.getConst());

        size_t index = 0;
        for (const auto& item : m) {
            RC_ASSERT(!Sut_iter_const_empty(&iter_const));
            RC_ASSERT(!Sut_iter_empty(&iter));
            const auto* sut_item_const = Sut_iter_const_next(&iter_const);
            auto* sut_item = Sut_iter_next(&iter);
            RC_ASSERT(*sut_item_const == item);
            RC_ASSERT(*sut_item == item);
            RC_ASSERT(*Sut_try_read_from_front(s.getConst(), index) == item);
            RC_ASSERT(*Sut_try_write_from_front(wrapperCopy.get(), index) == item);
            index++;
        }
        RC_ASSERT(Sut_iter_const_empty(&iter_const));
        RC_ASSERT(Sut_iter_empty(&iter));

        if (!m.empty()) {
            RC_ASSERT(!Sut_empty(s.getConst()));
        } else {
            RC_ASSERT(Sut_empty(s.getConst()));
        }
    }
};

struct PushFront : Command {
    Data value = *rc::gen::arbitrary<Data>();

    void checkPreconditions(const Model& m) const override {}
    void apply(Model& m) const override { m.push_front(value); }
    void runAndCheck(const Model& m, SutWrapper& s) const override {
        Sut_push_front(s.get(), value);
    }
    void show(std::ostream& os) const override { os << "PushFront(" << value << ")"; }
};

struct PushBack : Command {
    Data value = *rc::gen::arbitrary<Data>();

    void checkPreconditions(const Model& m) const override {}
    void apply(Model& m) const override { m.push_back(value); }
    void runAndCheck(const Model& m, SutWrapper& s) const override {
        Sut_push_back(s.get(), value);
    }
    void show(std::ostream& os) const override { os << "PushBack(" << value << ")"; }
};

struct PopFront : Command {
    void checkPreconditions(const Model& m) const override { RC_PRE(!m.empty()); }
    void apply(Model& m) const override { m.pop_front(); }
    void runAndCheck(const Model& m, SutWrapper& sut) const override { Sut_pop_front(sut.get()); }
    void show(std::ostream& os) const override { os << "PopFront()"; }
};

struct PopBack : Command {
    void checkPreconditions(const Model& m) const override { RC_PRE(!m.empty()); }
    void apply(Model& m) const override { m.pop_back(); }
    void runAndCheck(const Model& m, SutWrapper& sut) const override { Sut_pop_back(sut.get()); }
    void show(std::ostream& os) const override { os << "PopBack()"; }
};

RC_GTEST_PROP(CircularTests, General, ()) {
    Model model;
    SutWrapper sut;
    rc::state::check(model, sut,
                     rc::state::gen::execOneOfWithArgs<PushFront, PushFront, PushBack, PushBack,
                                                       PopFront, PopBack>());
}
