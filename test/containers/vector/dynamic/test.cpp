#include <derive-cpp/test/gtest_panic.hpp>
#include <gtest/gtest.h>

extern "C" {
#define NAME Sut
#define ITEM size_t
#include <derive-c/container/vector/dynamic/template.h>
}

TEST(VectorTests, CreateWithDefaults) {
    Sut sut = Sut_new_with_defaults(128, 3, stdalloc_get());
    ASSERT_EQ(Sut_size(&sut), 128);

    for (size_t i = 0; i < Sut_size(&sut); ++i) {
        ASSERT_EQ(*Sut_read(&sut, i), 3);
    }

    Sut_delete(&sut);
}

TEST(VectorTests, CreateWithZeroSize) {
    Sut sut_1 = Sut_new(stdalloc_get());
    ASSERT_EQ(Sut_size(&sut_1), 0);
    Sut_delete(&sut_1);

    Sut sut_2 = Sut_new(stdalloc_get());
    ASSERT_EQ(Sut_size(&sut_2), 0);
    Sut_delete(&sut_2);
}

TEST(VectorTests, CreateWithCapacity) {
    Sut sut = Sut_new_with_capacity(64, stdalloc_get());
    ASSERT_EQ(Sut_size(&sut), 0);
    Sut_delete(&sut);
}
