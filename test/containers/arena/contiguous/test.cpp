
#include <gtest/gtest.h>

extern "C" {
#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>

#define NAME Sut
#define VALUE size_t
#define INDEX_BITS 8
#include <derive-c/container/arena/contiguous/template.h>
}

TEST(ArenaTests, Full) {
    Sut sut = Sut_new_with_capacity_for(1, stdalloc_get());
    ASSERT_FALSE(Sut_full(&sut));
    ASSERT_EQ(Sut_max_entries, 254);

    for (size_t i = 0; i < Sut_max_entries; ++i) {
        Sut_insert(&sut, i);
    }
    ASSERT_TRUE(Sut_full(&sut));

    Sut_delete(&sut);
}

TEST(ArenaTests, Empty) {
    Sut sut = Sut_new_with_capacity_for(1, stdalloc_get());
    Sut_delete(&sut);
}

TEST(ArenaTests, Single) {
    Sut sut = Sut_new_with_capacity_for(1, stdalloc_get());
    Sut_insert(&sut, 1);
    Sut_delete(&sut);
}
