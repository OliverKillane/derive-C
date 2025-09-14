#include <gtest/gtest.h>

#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/state.h>

#include <limits>
#include <vector>

using Data = std::size_t;
using Model = std::vector<Data>;

extern "C" {
#define ITEM Data
#define INPLACE_CAPACITY 10
#define NAME Sut
#include <derive-c/structures/staticvec/template.h>
}

struct SutWrapper {

    SutWrapper() : sut(Sut_new()) {}
    SutWrapper(const SutWrapper& other) : sut(Sut_clone(other.getConst())) {}
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

        {
            RC_ASSERT(m.size() == Sut_size(s.getConst()));
            SutWrapper wrapperCopy = s;
            for (size_t i = 0; i < m.size(); ++i) {
                RC_ASSERT(m[i] == *Sut_read(s.getConst(), i));
                RC_ASSERT(m[i] == *Sut_write(wrapperCopy.get(), i));
            }
        }

        // JUSTIFY: Separately checking the iterator implementations
        //           - Different access to SELF_read
        {
            SutWrapper wrapperCopy = s;
            Sut_iter_const iter_const = Sut_get_iter_const(&s.sut);
            Sut_iter iter = Sut_get_iter(wrapperCopy.get());
            while (!Sut_iter_const_empty(&iter_const)) {
                RC_ASSERT(!Sut_iter_empty(&iter));
                size_t pos_const = Sut_iter_const_position(&iter_const);
                size_t pos = Sut_iter_position(&iter);
                Data const* item_const = Sut_iter_const_next(&iter_const);
                Data* item = Sut_iter_next(&iter);
                RC_ASSERT(m[pos] == *item_const);
                RC_ASSERT(m[pos] == *item);
            }
            RC_ASSERT(Sut_iter_empty(&iter));
            RC_ASSERT(Sut_iter_next(&iter) == nullptr);
            RC_ASSERT(Sut_iter_const_next(&iter_const) == nullptr);
        }

        {
            SutWrapper wrapperCopy = s;
            RC_ASSERT(Sut_try_read(s.getConst(), m.size()) == nullptr);
            RC_ASSERT(Sut_try_write(wrapperCopy.get(), m.size()) == nullptr);
        }

        {
            if (m.empty()) {
                SutWrapper wrapperCopy = s;
                Data destination;
                RC_ASSERT(!Sut_try_pop(wrapperCopy.get(), &destination));
            }
        }
    }
};

struct TryPush : Command {
    Data value =
        *rc::gen::inRange(std::numeric_limits<size_t>::min(), std::numeric_limits<Data>::max());

    void checkPreconditions(const Model& m) const override {}

    void apply(Model& m) const override {
        if (m.size() < Sut_max_size) {
            m.push_back(value);
        }
    }
    void runAndCheck(const Model& m, SutWrapper& s) const override {
        if (m.size() < Sut_max_size) {
            Sut_push(s.get(), value);
        } else {
            RC_ASSERT(!Sut_try_push(s.get(), value));
        }
    }
    void show(std::ostream& os) const override { os << "TryPush(" << value << ")"; }
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
        RC_PRE(m.size() < Sut_max_size);
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

struct InsertAt : Command {
    std::vector<Data> values;
    size_t fromIndex;

    explicit InsertAt(const Model& m)
        : values(*rc::gen::container<std::vector<Data>>(
              *rc::gen::inRange(static_cast<size_t>(0), static_cast<size_t>(10)),
              rc::gen::inRange(std::numeric_limits<Data>::min(),
                               std::numeric_limits<Data>::max()))),
          fromIndex(*rc::gen::inRange(static_cast<size_t>(0), m.size())) {}

    void checkPreconditions(const Model& m) const override {}

    void apply(Model& m) const override {
        if (values.size() > 0 && m.size() + values.size() <= Sut_max_size) {
            m.insert(m.begin() + fromIndex, values.begin(), values.end());
        }
    }

    void runAndCheck(const Model& m, SutWrapper& s) const override {
        bool expectedResult = m.size() + values.size() <= Sut_max_size;
        if (values.empty()) {
            Data dummy;
            RC_ASSERT(Sut_try_insert_at(s.get(), fromIndex, &dummy, 0) == expectedResult);
        } else {
            RC_ASSERT(Sut_try_insert_at(s.get(), fromIndex, values.data(), values.size()) ==
                      expectedResult);
        }
    }

    void show(std::ostream& os) const override {
        os << "InsertAt(index=" << fromIndex << ", values.size()=" << values.size() << ")";
    }
};

struct RemoveAt : Command {
    size_t fromIndex;
    size_t count;

    explicit RemoveAt(const Model& m)
        : fromIndex(*rc::gen::inRange(static_cast<size_t>(0), m.size())),
          count(*rc::gen::inRange(static_cast<size_t>(0), m.size() - fromIndex + 1)) {}

    void checkPreconditions(const Model& m) const override {
        RC_PRE(fromIndex + count <= m.size());
    }

    void apply(Model& m) const override {
        m.erase(m.begin() + fromIndex, m.begin() + fromIndex + count);
    }

    void runAndCheck(const Model& m, SutWrapper& s) const override {
        Sut_remove_at(s.get(), fromIndex, count);
    }

    void show(std::ostream& os) const override {
        os << "RemoveAt(index=" << fromIndex << ", count=" << count << ")";
    }
};

struct Pop : Command {
    void checkPreconditions(const Model& m) const override { RC_PRE(!m.empty()); }
    void apply(Model& m) const override { m.pop_back(); }
    void runAndCheck(const Model& m, SutWrapper& sut) const override { Sut_pop(sut.get()); }
    void show(std::ostream& os) const override { os << "Pop()"; }
};

RC_GTEST_PROP(StaticVecTests, General, ()) {
    Model model;
    SutWrapper sut;
    rc::state::check(model, sut,
                     rc::state::gen::execOneOfWithArgs<TryPush, TryPush, TryPush, Write, InsertAt,
                                                       RemoveAt, Pop>());
}
