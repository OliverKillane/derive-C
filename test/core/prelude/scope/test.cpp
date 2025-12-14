#include "derive-c/core/scope.h"
#include "gmock/gmock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-cpp/test/trampoline.hpp>

extern "C" {
#include <derive-c/core/prelude.h>
#include <derive-c/test/mock.h>
typedef struct {
    int inner;
} foo;

foo foo_new(int inner) {
    return (foo){
        .inner = inner,
    };
}
MOCKABLE(void, foo_on_delete, (foo*)) {}

// The cleanup attribute requires a function.
//  - MOCKABLE-s are global variables (function pointers) when compiling with mocking
//  - This does not work with the cleanup attribute
void foo_delete(foo* f) { foo_on_delete(f); }

MOCKABLE(void, on_some_fcn, ()) {}

void some_fcn(sentinel*) { on_some_fcn(); }
}

using namespace testing;

MATCHER_P(FooMatch, expectedInner, "") {
    *result_listener << "whose inner is " << arg->inner;
    return arg->inner == expectedInner;
}

struct ScopeTest : Test {
    MOCK_METHOD(void, foo_on_delete_mock, (foo*), ());
    derivecpp::Trampoline<&foo_on_delete, &ScopeTest::foo_on_delete_mock> foo_on_delete_tramp{this};

    MOCK_METHOD(void, on_some_fcn_mock, (), ());
    derivecpp::Trampoline<&on_some_fcn, &ScopeTest::on_some_fcn_mock> on_some_fcn_tramp{this};
};

TEST_F(ScopeTest, DeleterOrder) {
    InSequence s;

    {
        DC_SCOPED(foo) f = foo_new(0);
        EXPECT_CALL(*this, foo_on_delete_mock(FooMatch(0)));
    }

    {
        DC_SCOPED(foo) f = foo_new(1);
        DC_SCOPED(foo) ff = foo_new(2);

        EXPECT_CALL(*this, foo_on_delete_mock(FooMatch(2)));
        EXPECT_CALL(*this, foo_on_delete_mock(FooMatch(1)));
    }

    {
        DC_SCOPED(foo) f = foo_new(1);

        {
            DC_SCOPED(foo) ff = foo_new(2);
            EXPECT_CALL(*this, foo_on_delete_mock(FooMatch(2)));
        }

        EXPECT_CALL(*this, foo_on_delete_mock(FooMatch(1)));
    }
}

TEST_F(ScopeTest, DeferredCall) {
    InSequence s;

    {
        DC_DEFER(some_fcn);

        EXPECT_CALL(*this, on_some_fcn_mock());
    }

    {
        DC_DEFER(some_fcn);
        DC_DEFER(some_fcn);

        EXPECT_CALL(*this, on_some_fcn_mock());
        EXPECT_CALL(*this, on_some_fcn_mock());
    }
}
