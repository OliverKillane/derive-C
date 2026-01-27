#include <gtest/gtest.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/algorithm/hash/default.h>
#include <derive-c/utils/debug/string.h>

#define KEY int32_t
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE const char*
#define NAME test_map
#include <derive-c/container/map/swiss/template.h>

TEST(SwissTest, Debug) {
    DC_SCOPED(test_map) map = test_map_new(stdalloc_get_ref());

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_map_debug(&map, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_map@" DC_PTR_REPLACE " {\n"
            "  capacity: 256,\n"
            "  tombstones: 0,\n"
            "  count: 0,\n"
            "  ctrl: @" DC_PTR_REPLACE "[256 + simd probe size additional 16],\n"
            "  slots: @" DC_PTR_REPLACE "[256],\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  entries: [\n"
            "  ]\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }

    test_map_insert(&map, 3, "foo");
    test_map_insert(&map, 4, "bar");
    test_map_insert(&map, 5, "bing");
    test_map_insert(&map, 6, "zing");

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_map_debug(&map, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_map@" DC_PTR_REPLACE " {\n"
            "  capacity: 256,\n"
            "  tombstones: 0,\n"
            "  count: 4,\n"
            "  ctrl: @" DC_PTR_REPLACE "[256 + simd probe size additional 16],\n"
            "  slots: @" DC_PTR_REPLACE "[256],\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  entries: [\n"
            "    {\n"
            "      key: 3,\n"
            "      value: char*@" DC_PTR_REPLACE " \"foo\",\n"
            "    },\n"
            "    {\n"
            "      key: 4,\n"
            "      value: char*@" DC_PTR_REPLACE " \"bar\",\n"
            "    },\n"
            "    {\n"
            "      key: 5,\n"
            "      value: char*@" DC_PTR_REPLACE " \"bing\",\n"
            "    },\n"
            "    {\n"
            "      key: 6,\n"
            "      value: char*@" DC_PTR_REPLACE " \"zing\",\n"
            "    },\n"
            "  ]\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}

TEST(SwissUtils, ExtendHeuristic) {
    // capacity = 1 → max_load = 1 - (1 / 8) = 1

    // No entries and no tombstones: below max load, nothing to do
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 0, 1), DC_SWISS_DO_NOTHING);

    // Table exactly full with live entries and no tombstones: must grow
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 1, 1), DC_SWISS_DOUBLE_CAPACITY);

    // Table full but all space is tombstones: clean up instead of growing
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(1, 0, 1), DC_SWISS_CLEANUP_TOMBSONES);

    // Overfull (live + tombstones) with tombstones present: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(1, 1, 1), DC_SWISS_CLEANUP_TOMBSONES);

    // capacity = 7 → max_load = 7 - (7 / 8) = 7

    // Still room before reaching max load
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 6, 7), DC_SWISS_DO_NOTHING);

    // Exactly at max load with no tombstones: grow capacity
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 7, 7), DC_SWISS_DOUBLE_CAPACITY);

    // Exactly at max load with tombstones present: clean up instead of growing
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(1, 6, 7), DC_SWISS_CLEANUP_TOMBSONES);

    // Below max load, but tombstones dominate live entries: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(3, 3, 7), DC_SWISS_CLEANUP_TOMBSONES);

    // capacity = 8 → max_load = 8 - (8 / 8) = 7

    // Empty table: nothing to do
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 0, 8), DC_SWISS_DO_NOTHING);

    // Still below max load
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 6, 8), DC_SWISS_DO_NOTHING);

    // Exactly at max load without tombstones: grow
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 7, 8), DC_SWISS_DOUBLE_CAPACITY);

    // Exactly at max load with tombstones: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(1, 6, 8), DC_SWISS_CLEANUP_TOMBSONES);

    // Below max load and tombstones do not dominate: do nothing
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(1, 5, 8), DC_SWISS_DO_NOTHING);

    // Over max load and tombstones present: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(3, 5, 8), DC_SWISS_CLEANUP_TOMBSONES);

    // Below max load, but tombstones are more than half of live entries: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(3, 2, 8), DC_SWISS_CLEANUP_TOMBSONES);

    // Tombstones equal to half of live entries is not enough to trigger cleanup
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(2, 4, 8), DC_SWISS_DO_NOTHING);

    // capacity = 9 → max_load = 9 - (9 / 8) = 8

    // Below max load: do nothing
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 7, 9), DC_SWISS_DO_NOTHING);

    // Exactly at max load with no tombstones: grow
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 8, 9), DC_SWISS_DOUBLE_CAPACITY);

    // Exactly at max load with tombstones: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(1, 7, 9), DC_SWISS_CLEANUP_TOMBSONES);

    // capacity = 16 → max_load = 16 - (16 / 8) = 14

    // Still below max load
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 13, 16), DC_SWISS_DO_NOTHING);

    // Exactly at max load with no tombstones: grow
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 14, 16), DC_SWISS_DOUBLE_CAPACITY);

    // Exactly at max load with tombstones: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(1, 13, 16), DC_SWISS_CLEANUP_TOMBSONES);

    // Well over max load and tombstones present: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(6, 11, 16), DC_SWISS_CLEANUP_TOMBSONES);

    // capacity = 32 → max_load = 32 - (32 / 8) = 28

    // Plenty of space, no tombstones: do nothing
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 10, 32), DC_SWISS_DO_NOTHING);

    // Below max load, but tombstones dominate live entries: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(6, 10, 32), DC_SWISS_CLEANUP_TOMBSONES);

    // Tombstones exactly half of live entries is not enough to clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(5, 10, 32), DC_SWISS_DO_NOTHING);

    // Any tombstones when count is 1 dominate live entries: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(1, 1, 32), DC_SWISS_CLEANUP_TOMBSONES);

    // No tombstones and small count: do nothing
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 1, 32), DC_SWISS_DO_NOTHING);

    // Exactly at max load with no tombstones: grow
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 28, 32), DC_SWISS_DOUBLE_CAPACITY);

    // Exactly at max load with tombstones: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(1, 27, 32), DC_SWISS_CLEANUP_TOMBSONES);

    // Even when tombstones dominate, being below max load is required for that rule
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(19, 8, 32), DC_SWISS_CLEANUP_TOMBSONES);

    // No live entries but tombstones exist: clean up
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(1, 0, 32), DC_SWISS_CLEANUP_TOMBSONES);

    // Completely empty table: nothing to do
    EXPECT_EQ(_dc_swiss_heuristic_should_extend(0, 0, 32), DC_SWISS_DO_NOTHING);
}
