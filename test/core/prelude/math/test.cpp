#include <derive-cpp/test/gtest_panic.hpp>
#include <gtest/gtest.h>

#include <derive-c/core/prelude.h>

TEST(CoreTests, NextPowerOf2) {
    ASSERT_EQ(next_power_of_2(0), 1);
    ASSERT_EQ(next_power_of_2(1), 1);
    ASSERT_EQ(next_power_of_2(2), 2);
    ASSERT_EQ(next_power_of_2(3), 4);
}
