
#include <gtest/gtest.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/utils/debug.h>

#define ITEM int
#define NAME Sut
#include <derive-c/container/queue/deque/template.h>

TEST(DequeTests, CreateWithCapacity) {
    Sut sut = Sut_new_with_capacity(64, stdalloc_get_ref());
    ASSERT_EQ(Sut_size(&sut), 0);
    Sut_delete(&sut);
}

TEST(DequeTests, CreateWithZeroSize) {
    Sut sut = Sut_new(stdalloc_get_ref());
    ASSERT_EQ(Sut_size(&sut), 0);
    Sut_delete(&sut);
}

TEST(DequeTests, IteratorEmpty) {
    Sut sut = Sut_new(stdalloc_get_ref());

    Sut_iter_const iter = Sut_get_iter_const(&sut);
    ASSERT_TRUE(Sut_iter_const_empty(&iter));
    ASSERT_EQ(Sut_iter_const_next(&iter), nullptr);

    Sut_delete(&sut);
}

TEST(DequeTests, IteratorSimple) {
    Sut sut = Sut_new(stdalloc_get_ref());

    Sut_push_back(&sut, 1);
    Sut_push_back(&sut, 2);
    Sut_push_back(&sut, 3);

    ASSERT_EQ(Sut_size(&sut), 3);

    Sut_iter_const iter = Sut_get_iter_const(&sut);
    ASSERT_FALSE(Sut_iter_const_empty(&iter));

    int const* val1 = Sut_iter_const_next(&iter);
    ASSERT_NE(val1, nullptr);
    ASSERT_EQ(*val1, 1);

    int const* val2 = Sut_iter_const_next(&iter);
    ASSERT_NE(val2, nullptr);
    ASSERT_EQ(*val2, 2);

    int const* val3 = Sut_iter_const_next(&iter);
    ASSERT_NE(val3, nullptr);
    ASSERT_EQ(*val3, 3);

    ASSERT_TRUE(Sut_iter_const_empty(&iter));
    ASSERT_EQ(Sut_iter_const_next(&iter), nullptr);

    Sut_delete(&sut);
}

TEST(DequeTests, IteratorMixedOps) {
    Sut sut = Sut_new(stdalloc_get_ref());

    Sut_push_back(&sut, 2);
    Sut_push_front(&sut, 1);
    Sut_push_back(&sut, 3);
    Sut_push_front(&sut, 0);

    ASSERT_EQ(Sut_size(&sut), 4);

    Sut_iter_const iter = Sut_get_iter_const(&sut);

    int expected[] = {0, 1, 2, 3};
    for (int i = 0; i < 4; i++) {
        ASSERT_FALSE(Sut_iter_const_empty(&iter));
        int const* val = Sut_iter_const_next(&iter);
        ASSERT_NE(val, nullptr) << "Iterator returned NULL at position " << i;
        ASSERT_EQ(*val, expected[i]) << "Wrong value at position " << i;
    }

    ASSERT_TRUE(Sut_iter_const_empty(&iter));
    ASSERT_EQ(Sut_iter_const_next(&iter), nullptr);

    Sut_delete(&sut);
}

TEST(DequeTests, IteratorLikeFuzzTest) {
    // Replicate the fuzz test's checking logic
    Sut sut = Sut_new_with_capacity(4, stdalloc_get_ref());

    // Model
    std::vector<int> model;

    // Do some operations
    Sut_push_back(&sut, 5);
    model.push_back(5);

    Sut_push_front(&sut, 3);
    model.insert(model.begin(), 3);

    // Check iterator like fuzz test does
    Sut_iter_const iter_const = Sut_get_iter_const(&sut);

    for (const auto& expected_item : model) {
        ASSERT_FALSE(Sut_iter_const_empty(&iter_const))
            << "Iterator claims empty but model has items";

        int const* sut_item = Sut_iter_const_next(&iter_const);
        ASSERT_NE(sut_item, nullptr) << "Iterator returned NULL unexpectedly";
        ASSERT_EQ(*sut_item, expected_item) << "Item mismatch";
    }

    ASSERT_TRUE(Sut_iter_const_empty(&iter_const))
        << "Iterator should be empty after consuming all items";
    ASSERT_EQ(Sut_iter_const_next(&iter_const), nullptr)
        << "Iterator should return NULL when empty";

    Sut_delete(&sut);
}

TEST(DequeTests, IteratorAfterRebalancing) {
    Sut sut = Sut_new(stdalloc_get_ref());

    // Push many to back to fill back vector
    for (int i = 0; i < 10; i++) {
        Sut_push_back(&sut, i);
    }

    // This should trigger rebalancing
    ASSERT_EQ(Sut_size(&sut), 10);

    // Now iterate
    Sut_iter_const iter = Sut_get_iter_const(&sut);

    for (int i = 0; i < 10; i++) {
        ASSERT_FALSE(Sut_iter_const_empty(&iter)) << "Empty at " << i;
        int const* val = Sut_iter_const_next(&iter);
        ASSERT_NE(val, nullptr) << "NULL at " << i;
        ASSERT_EQ(*val, i) << "Wrong value at " << i;
    }

    ASSERT_TRUE(Sut_iter_const_empty(&iter));

    Sut_delete(&sut);
}

#define ITEM const char*
#define NAME str_queue
#include <derive-c/container/queue/deque/template.h>

TEST(DequeTests, Debug) {
    DC_SCOPED(str_queue) q = str_queue_new(stdalloc_get_ref());

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        str_queue_debug(&q, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "str_queue@" DC_PTR_REPLACE " {\n"
            "  size: 0,\n"
            "  front: str_queue_item_vectors@" DC_PTR_REPLACE " {\n"
            "    size: 0,\n"
            "    capacity: 0,\n"
            "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "    items: @(nil) [\n"
            "    ],\n"
            "  },\n"
            "  back: str_queue_item_vectors@" DC_PTR_REPLACE " {\n"
            "    size: 0,\n"
            "    capacity: 0,\n"
            "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "    items: @(nil) [\n"
            "    ],\n"
            "  },\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }

    str_queue_push_back(&q, "a");
    str_queue_push_back(&q, "b");
    str_queue_push_back(&q, "c");
    str_queue_push_back(&q, "d");
    str_queue_push_back(&q, "e");
    str_queue_push_back(&q, "f");
    str_queue_push_back(&q, "h");
    str_queue_push_back(&q, "i");
    str_queue_push_back(&q, "j");
    str_queue_push_back(&q, "k");
    str_queue_push_back(&q, "l");

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        str_queue_debug(&q, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "str_queue@" DC_PTR_REPLACE " {\n"
            "  size: 11,\n"
            "  front: str_queue_item_vectors@" DC_PTR_REPLACE " {\n"
            "    size: 0,\n"
            "    capacity: 0,\n"
            "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "    items: @(nil) [\n"
            "    ],\n"
            "  },\n"
            "  back: str_queue_item_vectors@" DC_PTR_REPLACE " {\n"
            "    size: 11,\n"
            "    capacity: 16,\n"
            "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "    items: @" DC_PTR_REPLACE " [\n"
            "      char*@" DC_PTR_REPLACE " \"a\",\n"
            "      char*@" DC_PTR_REPLACE " \"b\",\n"
            "      char*@" DC_PTR_REPLACE " \"c\",\n"
            "      char*@" DC_PTR_REPLACE " \"d\",\n"
            "      char*@" DC_PTR_REPLACE " \"e\",\n"
            "      char*@" DC_PTR_REPLACE " \"f\",\n"
            "      char*@" DC_PTR_REPLACE " \"h\",\n"
            "      char*@" DC_PTR_REPLACE " \"i\",\n"
            "      char*@" DC_PTR_REPLACE " \"j\",\n"
            "      char*@" DC_PTR_REPLACE " \"k\",\n"
            "      char*@" DC_PTR_REPLACE " \"l\",\n"
            "    ],\n"
            "  },\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}
