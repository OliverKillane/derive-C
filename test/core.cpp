#include <gtest/gtest.h>

#include <derive-c/core/helpers.h>

namespace core {

TEST(CoreTests, NextPowerOf2) {
    ASSERT_EQ(next_power_of_2(0), 1);
    ASSERT_EQ(next_power_of_2(1), 1);
    ASSERT_EQ(next_power_of_2(2), 2);
    ASSERT_EQ(next_power_of_2(3), 4);
}

} // namespace core
