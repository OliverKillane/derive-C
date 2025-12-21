
#include <gtest/gtest.h>

#include <derive-c/utils/for.h>

#define CAPACITY 256
#define KEY size_t
#define KEY_EQ(key_1, key_2) (*key_1 == *key_2)
#define VALUE size_t
#define NAME Sut
#include <derive-c/container/map/staticlinear/template.h>

TEST(StaticLinearMap, InsertAndRead) {
    Sut map = Sut_new();

    for (size_t i = 0; i < Sut_capacity(); i++) {
        size_t* placed = Sut_insert(&map, i, i * 10);
        ASSERT_NE(placed, nullptr);
        ASSERT_EQ(*placed, i * 10);
    }

    FOR(Sut, &map, iter, item) {
        size_t key = *item.key;
        size_t value = *item.value;
        ASSERT_EQ(value, key * 10);
    }

    Sut_delete(&map);
}
