#include <gtest/gtest.h>

namespace option {

extern "C" {
#define SELF optional_int
#define T int
#include <derive-c/structures/option/template.h>
}

TEST(Option, EmptyNotPresent) {
    optional_int opt = optional_int_empty();
    ASSERT_FALSE(optional_int_is_present(&opt));
    ASSERT_EQ(optional_int_get(&opt), nullptr);
    ASSERT_EQ(optional_int_get_const(&opt), nullptr);
}

TEST(Option, Present) {
    optional_int opt = optional_int_from(42);
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(*optional_int_get(&opt), 42);
    ASSERT_EQ(*optional_int_get_const(&opt), 42);
}

TEST(Option, From) {
    optional_int opt = optional_int_from(100);
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(*optional_int_get(&opt), 100);
    ASSERT_EQ(*optional_int_get_const(&opt), 100);
}

} // namespace option