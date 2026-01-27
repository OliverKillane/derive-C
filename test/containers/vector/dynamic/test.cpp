
#include <gtest/gtest.h>

#include <derive-c/utils/debug/string.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#define NAME sut
#define ITEM size_t
#include <derive-c/container/vector/dynamic/template.h>

TEST(VectorTests, CreateWithDefaults) {
    sut sut = sut_new_with_defaults(128, 3, stdalloc_get_ref());
    ASSERT_EQ(sut_size(&sut), 128);

    for (size_t i = 0; i < sut_size(&sut); ++i) {
        ASSERT_EQ(*sut_read(&sut, i), 3);
    }

    sut_delete(&sut);
}

TEST(VectorTests, CreateWithZeroSize) {
    sut sut_1 = sut_new(stdalloc_get_ref());
    ASSERT_EQ(sut_size(&sut_1), 0);
    sut_delete(&sut_1);

    sut sut_2 = sut_new(stdalloc_get_ref());
    ASSERT_EQ(sut_size(&sut_2), 0);
    sut_delete(&sut_2);
}

TEST(VectorTests, CreateWithCapacity) {
    sut sut = sut_new_with_capacity(64, stdalloc_get_ref());
    ASSERT_EQ(sut_size(&sut), 0);
    sut_delete(&sut);
}

TEST(VectorTests, CreateWithCapacity2) {
    sut sut = sut_new_with_capacity(64, stdalloc_get_ref());
    sut_push(&sut, 1);
    const auto* a = sut_read(&sut, 0);
    dc_memory_tracker_check(DC_MEMORY_TRACKER_LVL_CONTAINER, DC_MEMORY_TRACKER_CAP_READ_WRITE, a,
                            sizeof(*a));
    sut_delete(&sut);
}

#define NAME test_vec
#define ITEM char const*
#include <derive-c/container/vector/dynamic/template.h>

TEST(VectorTests, Debug) {
    DC_SCOPED(test_vec) v = test_vec_new(stdalloc_get_ref());

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_vec_debug(&v, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_vec@" DC_PTR_REPLACE " {\n"
            "  size: 0,\n"
            "  capacity: 0,\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  items: @(nil) [\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }

    test_vec_push(&v, "foo");
    test_vec_push(&v, "bar");
    test_vec_push(&v, "bing");

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_vec_debug(&v, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_vec@" DC_PTR_REPLACE " {\n"
            "  size: 3,\n"
            "  capacity: 8,\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  items: @" DC_PTR_REPLACE " [\n"
            "    char*@" DC_PTR_REPLACE " \"foo\",\n"
            "    char*@" DC_PTR_REPLACE " \"bar\",\n"
            "    char*@" DC_PTR_REPLACE " \"bing\",\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}
