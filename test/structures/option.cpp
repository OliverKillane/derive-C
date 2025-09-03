#include <gtest/gtest.h>

namespace option {

void free_int(int** ptr) { delete *ptr; }

extern "C" {
#define NAME optional_int
#define T int*
#define T_DELETE free_int
#include <derive-c/structures/option/template.h>
}

TEST(Option, EmptyNotPresent) {
    optional_int opt = optional_int_empty();
    ASSERT_FALSE(optional_int_is_present(&opt));
    ASSERT_EQ(optional_int_get(&opt), nullptr);
    ASSERT_EQ(optional_int_get_const(&opt), nullptr);
    optional_int_delete(&opt);
}

TEST(Option, Present) {
    optional_int opt = optional_int_from(new int(42));
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(**optional_int_get(&opt), 42);
    ASSERT_EQ(**optional_int_get_const(&opt), 42);
    optional_int_delete(&opt);
}

TEST(Option, From) {
    optional_int opt = optional_int_from(new int(100));
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(**optional_int_get(&opt), 100);
    ASSERT_EQ(**optional_int_get_const(&opt), 100);
    optional_int_delete(&opt);
}

TEST(Option, Replace) {
    optional_int opt = optional_int_from(new int(10));
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(**optional_int_get(&opt), 10);

    optional_int_replace(&opt, new int(100));
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(**optional_int_get(&opt), 100);

    optional_int_delete(&opt);
}

TEST(Option, ReplaceEmpty) {
    optional_int opt = optional_int_empty();
    ASSERT_FALSE(optional_int_is_present(&opt));

    optional_int_replace(&opt, new int(100));
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(**optional_int_get(&opt), 100);

    optional_int_delete(&opt);
}

} // namespace option