#include <gtest/gtest.h>

extern "C" {
#define MODULE test
#define PANIC abort()

#define T uint32_t
#include <derive-c/structures/vector.template.h>

#define T char
#include <derive-c/structures/vector.template.h>

typedef struct {
} empty; // zero-sized type
#define T empty
#include <derive-c/structures/vector.template.h>
}

TEST(VectorTests, InitiallyEmpty) {
    auto x = test_vector_empty_new();
    EXPECT_EQ(test_vector_empty_size(&x), 0);
}
