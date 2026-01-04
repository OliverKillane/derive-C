
#include <gtest/gtest.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/utils/debug.h>

#define ITEM int
#define NAME Sut
#include <derive-c/container/queue/deque/template.h>

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

#define ITEM const char*
#define NAME str_queue
#include <derive-c/container/queue/deque/template.h>

TEST(DequeTests, Debug) {
    DC_SCOPED(str_queue) q = str_queue_new(stdalloc_get());

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
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
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
        str_queue_debug(&q, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "str_queue@" DC_PTR_REPLACE " {\n"
            "  size: 11,\n"
            "  front: str_queue_item_vectors@" DC_PTR_REPLACE " {\n"
            "    size: 5,\n"
            "    capacity: 5,\n"
            "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "    items: @" DC_PTR_REPLACE " [\n"
            "      char*@" DC_PTR_REPLACE " \"e\",\n"
            "      char*@" DC_PTR_REPLACE " \"d\",\n"
            "      char*@" DC_PTR_REPLACE " \"c\",\n"
            "      char*@" DC_PTR_REPLACE " \"b\",\n"
            "      char*@" DC_PTR_REPLACE " \"a\",\n"
            "    ],\n"
            "  },\n"
            "  back: str_queue_item_vectors@" DC_PTR_REPLACE " {\n"
            "    size: 6,\n"
            "    capacity: 8,\n"
            "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "    items: @" DC_PTR_REPLACE " [\n"
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
