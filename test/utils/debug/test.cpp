#include <gtest/gtest.h>

#include <derive-c/utils/debug/dump.h>

using namespace testing;

struct ConstantDebug {
    size_t size;
    char const content;

    static void debug(ConstantDebug const* self, dc_debug_fmt fmt, FILE* stream) {
        (void)fmt;
        for (size_t i = 0; i < self->size; i++) {
            fprintf(stream, "%c", self->content);
        }
    }
};

struct DebugDump : Test {
    void SetUp() override { dc_debug_dump_reset(); }
};

TEST_F(DebugDump, BasicPrint) {
    ConstantDebug debug_a = {5, 'a'};
    char const* result = DC_DEBUG(ConstantDebug::debug, &debug_a);
    EXPECT_STREQ(result, "aaaaa");
}

TEST_F(DebugDump, AdjacentPrint) {
    ConstantDebug debug_a = {3, 'a'};
    char const* result1 = DC_DEBUG(ConstantDebug::debug, &debug_a);
    EXPECT_STREQ(result1, "aaa");

    ConstantDebug debug_b = {4, 'b'};
    char const* result2 = DC_DEBUG(ConstantDebug::debug, &debug_b);
    EXPECT_STREQ(result2, "bbbb");
}

TEST_F(DebugDump, ExactMax) {
    ConstantDebug debug_a = {_DC_DEBUG_MAX_CAPACITY, 'a'};
    char const* result = DC_DEBUG(ConstantDebug::debug, &debug_a);
    EXPECT_EQ(strlen(result), _DC_DEBUG_MAX_CAPACITY);
    EXPECT_EQ(result[0], 'a');
    EXPECT_EQ(result[_DC_DEBUG_MAX_CAPACITY - 1], 'a');

    ConstantDebug debug_b = {1, 'b'};
    char const* result2 = DC_DEBUG(ConstantDebug::debug, &debug_b);
    EXPECT_STREQ(result2, "...");
}

TEST_F(DebugDump, OneOverCapacity) {
    ConstantDebug debug_a = {_DC_DEBUG_MAX_CAPACITY + 1, 'a'};
    char const* result = DC_DEBUG(ConstantDebug::debug, &debug_a);
    size_t len = strlen(result);
    size_t expected_content = _DC_DEBUG_MAX_CAPACITY - strlen("...");
    EXPECT_EQ(len, expected_content + 3);
    EXPECT_STREQ(result + len - 3, "...");

    ConstantDebug debug_b = {1, 'b'};
    char const* result2 = DC_DEBUG(ConstantDebug::debug, &debug_b);
    EXPECT_STREQ(result2, "...");
}
