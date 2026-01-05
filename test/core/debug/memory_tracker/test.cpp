
#include <gtest/gtest.h>
#include <initializer_list>

#include <derive-c/utils/debug.h>

// JUSTIFY: No customd defines for the checker implementation or global level.
//  - Set and check should be usable regardless msan or asan usage
//  - matrix build in CI covers configurations
#include <derive-c/core/debug/memory_tracker.h>

namespace {
void set_and_check(dc_memory_tracker_level level, dc_memory_tracker_capability cap,
                   const void* addr, size_t size) {
    dc_memory_tracker_set(level, cap, addr, size);
    dc_memory_tracker_check(level, cap, addr, size);
}
} // namespace

TEST(MemoryTrackerTest, BasicChecks) {
    char buf[8];

    for (dc_memory_tracker_level lvl : std::initializer_list<dc_memory_tracker_level>{
             DC_MEMORY_TRACKER_LVL_NONE,
             DC_MEMORY_TRACKER_LVL_CONTAINER,
             DC_MEMORY_TRACKER_LVL_ALLOC,
         }) {
        for (size_t i = 0; i < sizeof(buf); i++) {
            for (dc_memory_tracker_capability cap :
                 std::initializer_list<dc_memory_tracker_capability>{
                     DC_MEMORY_TRACKER_CAP_NONE,
                     DC_MEMORY_TRACKER_CAP_WRITE,
                     DC_MEMORY_TRACKER_CAP_READ_WRITE,
                 }) {

                // JUSTIFY: Check bytes
                // - For asan 8 bytes map to 1, so additional logic is present in the check.
                set_and_check(lvl, cap, buf + i, 1);
            }
        }
        for (dc_memory_tracker_capability cap : std::initializer_list<dc_memory_tracker_capability>{
                 DC_MEMORY_TRACKER_CAP_NONE,
                 DC_MEMORY_TRACKER_CAP_WRITE,
                 DC_MEMORY_TRACKER_CAP_READ_WRITE,
             }) {
            set_and_check(lvl, cap, buf, sizeof(buf));
        }
    }
}

TEST(MemoryTrackerTest, DebugOutput) {
    // JUSTIFY: Tests use level alloc
    //  - To avoid confusion, we dont run this test when the custom memory tracking is off
    if (dc_memory_tracker_global_level >= DC_MEMORY_TRACKER_LVL_ALLOC) {
        uint8_t buf[32] = {};

        for (size_t i = 0; i < sizeof(buf); i++) {
            buf[i] = i;
        }

        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_ALLOC, DC_MEMORY_TRACKER_CAP_NONE, &buf[2], 3);

        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        dc_memory_tracker_debug(dc_debug_string_builder_stream(&sb), buf, 7);

#if defined ASAN_ON
        EXPECT_EQ(std::string(std::format(
                      "memory tracker debug (7 bytes) at {:p} [ASAN]:"
                      "\ndisplaying each 8 byte grandule (asan tracks poisoning as 0-8 bytes from "
                      "the end)"
                      "\n"
                      "\n                    0      1      2      3      4      5      6      7  "
                      "\n{:p}: [U|00] [U|01] [U|02] [U|03] [U|04] [U|05] [U|06] |U|07| "
                      "\n"
                      "\n",
                      static_cast<const void*>(buf), static_cast<const void*>(buf))),
                  std::string(dc_debug_string_builder_string(&sb)));
#endif

#if defined MSAN_ON
        EXPECT_EQ(derivecpp::fmt::c_style(
                      // clang-format off
                "memory tracker debug (7 bytes) at %p [MSAN]:\n"
                "%p: I [00]\n"
                "%p: I [01]\n"
                "%p: U [02]\n"
                "%p: U [03]\n"
                "%p: U [04]\n"
                "%p: I [05]\n"
                "%p: I [06]\n"
                      // clang-format on
                      ,
                      &buf[0], &buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5], &buf[6],
                      &buf[7]),
                  std::string(dc_debug_string_builder_string(&sb)));
#endif
    }
}