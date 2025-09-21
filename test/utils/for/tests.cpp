#include "derive-c/alloc/std.h"
#include <gtest/gtest.h>

extern "C" {

#define ITEM int
#define NAME int_vec
#include <derive-c/container/vector/dynamic/template.h>

#include <derive-c/utils/iterator/for.h>
}

TEST(For, empty_iterator) {
    int_vec v = int_vec_new(stdalloc_get());

    FOR(int_vec, v, item) { FAIL() << "Iterator should be empty"; }

    int_vec_delete(&v);
}

TEST(For, single_item) {
    int_vec v = int_vec_new(stdalloc_get());
    int_vec_push(&v, 42);
    size_t count = 0;
    FOR(int_vec, v, item) {
        count++;
        EXPECT_EQ(*item, 42);
    }
    EXPECT_EQ(count, 1);

    int_vec_delete(&v);
}

TEST(For, multiple_items) {
    int_vec v = int_vec_new(stdalloc_get());
    for (int i = 0; i < 10; i++) {
        int_vec_push(&v, i);
    }

    size_t count = 0;
    FOR(int_vec, v, item) {
        EXPECT_EQ(*item, count);
        count++;
    }
    EXPECT_EQ(count, 10);

    int_vec_delete(&v);
}
