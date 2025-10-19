#include <derive-cpp/test/gtest_panic.hpp>
#include <gtest/gtest.h>

extern "C" {
#define ITEM int
#define NAME Sut
#include <derive-c/container/queue/deque/template.h>
}

TEST(DequeTests, CreateWithCapacity) {
    Sut sut = Sut_new_with_capacity(64, stdalloc_get());
    ASSERT_EQ(Sut_size(&sut), 0);
    Sut_delete(&sut);
}

TEST(DequeTests, CreateWithZeroSize) {
    Sut sut = Sut_new(stdalloc_get());
    ASSERT_EQ(Sut_size(&sut), 0);
    Sut_delete(&sut);
}
