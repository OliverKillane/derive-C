#include <gtest/gtest.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/algorithm/hash/default.h>
#include <derive-c/utils/debug.h>

#define ITEM int32_t
#define ITEM_HASH DC_DEFAULT_HASH
#define NAME test_set
#include <derive-c/container/set/swiss/template.h>

TEST(SwissTest, Debug) {
    DC_SCOPED(test_set) map = test_set_new(stdalloc_get_ref());

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_set_debug(&map, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_set@" DC_PTR_REPLACE " {\n"
            "  map: __private_test_set_inner_map@<ptr> {\n"
            "    capacity: 256,\n"
            "    tombstones: 0,\n"
            "    count: 0,\n"
            "    ctrl: @<ptr>[256 + simd probe size additional 16],\n"
            "    slots: @<ptr>[256],\n"
            "    alloc: stdalloc@<ptr> { },\n"
            "    entries: [\n"
            "    ]\n"
            "  },\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}
