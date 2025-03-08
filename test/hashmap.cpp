#include <gtest/gtest.h>

extern "C" {
#define PANIC abort()

bool int_equality(uint32_t const* key_1, uint32_t const* key_2) { return key_1 == key_2; }

size_t int_hash(uint32_t const* key) { return *key; }

#define K uint32_t
#define V char
#define EQ int_equality
#define HASH int_hash
#define SELF test_hashmap
#include <derive-c/structures/hashmap.template.h>
}

TEST(HashmapTests, InitiallyEmpty) {
    auto x = test_hashmap_new();
    EXPECT_EQ(test_hashmap_size(&x), 0);
}
