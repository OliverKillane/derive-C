#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-c/utils/log/loggers/null.h>
#include <derive-c/utils/log/loggers/file.h>
#include <derive-c/utils/log/macros.h>
#include <derive-c/utils/debug/string.h>

#include <derive-cpp/test/trampoline.hpp>
#include <derive-cpp/test/gtest_mock.hpp>
#include <derive-cpp/fmt/remove_ptrs.hpp>

using namespace testing;

TEST(NullLoggerTests, CreateFromGlobalConfig) {
    DC_SCOPED(dc_log_null) logger = dc_log_null_new_global({}, {"test"});
}

TEST(NullLoggerTests, CreateFromParent) {
    DC_SCOPED(dc_log_null) parent = dc_log_null_new_global({}, {"parent"});
    DC_SCOPED(dc_log_null) child = DC_LOGGER_FROM_PARENT(dc_log_null, &parent, "%s", "child");
}

TEST(NullLoggerTests, LogDoesNothing) {
    DC_SCOPED(dc_log_null) logger = dc_log_null_new_global({}, {"test"});

    DC_LOGGER_LOG(dc_log_null, logger, DC_TRACE, "trace message %d", 1);
    DC_LOGGER_LOG(dc_log_null, logger, DC_DEBUG, "debug message %d", 2);
    DC_LOGGER_LOG(dc_log_null, logger, DC_INFO, "info message %d", 3);
    DC_LOGGER_LOG(dc_log_null, logger, DC_WARN, "warn message %d", 4);
    DC_LOGGER_LOG(dc_log_null, logger, DC_ERROR, "error message %d", 5);
}

TEST(NullLoggerTests, Debug) {
    DC_SCOPED(dc_log_null) logger = dc_log_null_new_global({}, {"test"});
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());

    dc_log_null_debug(&logger, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

    EXPECT_STREQ("dc_log_null { }", dc_debug_string_builder_string(&sb));
}

constexpr uint64_t TEST_TIMESTAMP_NS = 1704067200000000000ULL;

struct FileLoggerTests : Test {
    FIXTURE_MOCK(FileLoggerTests, dc_timestamp, dc_timestamp_now, (), ());

    void SetUp() override {
        EXPECT_CALL(*this, dc_timestamp_now_mock())
            .Times(AnyNumber())
            .WillRepeatedly(Return(dc_timestamp{TEST_TIMESTAMP_NS}));
    }
};

TEST_F(FileLoggerTests, CreateAndDelete) {
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    FILE* stream = dc_debug_string_builder_stream(&sb);

    {
        DC_SCOPED(dc_log_file)
        logger = dc_log_file_new_global({.stream = stream, .ansi_colours = false}, {"root"});
    }

    std::string output = dc_debug_string_builder_string(&sb);
    EXPECT_THAT(output, HasSubstr("[INFO]"));
    EXPECT_THAT(output, HasSubstr("[root]"));
    EXPECT_THAT(output, HasSubstr("Logger created"));
    EXPECT_THAT(output, HasSubstr("Logger deleted"));
}

TEST_F(FileLoggerTests, CreateChildModules) {
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    FILE* stream = dc_debug_string_builder_stream(&sb);

    {
        DC_SCOPED(dc_log_file)
        parent = dc_log_file_new_global({.stream = stream, .ansi_colours = false}, {"parent"});
        DC_SCOPED(dc_log_file) child = dc_log_file_from_parent(&parent, {"child"});
        DC_SCOPED(dc_log_file) grandchild = dc_log_file_from_parent(&child, {"grandchild"});
    }

    std::string output = dc_debug_string_builder_string(&sb);
    EXPECT_THAT(output, HasSubstr("[parent]"));
    EXPECT_THAT(output, HasSubstr("[parent/child]"));
    EXPECT_THAT(output, HasSubstr("[parent/child/grandchild]"));
}

TEST_F(FileLoggerTests, FilterIgnoresLowerLevels) {
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    FILE* stream = dc_debug_string_builder_stream(&sb);

    {
        DC_SCOPED(dc_log_file)
        logger = dc_log_file_new_global({.stream = stream, .ansi_colours = false}, {"test"});

        DC_LOGGER_LOG(dc_log_file, logger, DC_DEBUG, "debug message");
        DC_LOGGER_LOG(dc_log_file, logger, DC_TRACE, "trace message");
    }

    std::string output = dc_debug_string_builder_string(&sb);
    EXPECT_THAT(output, Not(HasSubstr("debug message")));
    EXPECT_THAT(output, Not(HasSubstr("trace message")));
    EXPECT_THAT(output, HasSubstr("Logger created"));
}

TEST_F(FileLoggerTests, ParentFilterBlocksChild) {
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    FILE* stream = dc_debug_string_builder_stream(&sb);

    {
        DC_SCOPED(dc_log_file)
        parent = dc_log_file_new_global({.stream = stream, .ansi_colours = false}, {"parent"});
        dc_log_file_set_filter(&parent, DC_ERROR);

        DC_SCOPED(dc_log_file) child = dc_log_file_from_parent(&parent, {"child"});
        dc_log_file_set_filter(&child, DC_DEBUG);

        DC_LOGGER_LOG(dc_log_file, child, DC_INFO, "should be filtered by parent");
        DC_LOGGER_LOG(dc_log_file, child, DC_ERROR, "should pass");
    }

    std::string output = dc_debug_string_builder_string(&sb);
    EXPECT_THAT(output, Not(HasSubstr("should be filtered by parent")));
    EXPECT_THAT(output, HasSubstr("should pass"));
}

TEST_F(FileLoggerTests, LogFormatIncludesTimestamp) {
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    FILE* stream = dc_debug_string_builder_stream(&sb);

    {
        DC_SCOPED(dc_log_file)
        logger = dc_log_file_new_global({.stream = stream, .ansi_colours = false}, {"test"});
    }

    std::string output = dc_debug_string_builder_string(&sb);
    EXPECT_THAT(output, HasSubstr("[2024-01-01T00:00:00.000000000Z]"));
}

TEST_F(FileLoggerTests, LocationIncludedForDebugNotInfo) {
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    FILE* stream = dc_debug_string_builder_stream(&sb);

    {
        DC_SCOPED(dc_log_file)
        logger = dc_log_file_new_global({.stream = stream, .ansi_colours = false}, {"test"});
        dc_log_file_set_filter(&logger, DC_TRACE);

        DC_LOGGER_LOG(dc_log_file, logger, DC_DEBUG, "debug message");
        DC_LOGGER_LOG(dc_log_file, logger, DC_INFO, "info message");
    }

    std::string output = dc_debug_string_builder_string(&sb);

    // Debug messages should include location
    EXPECT_THAT(output, HasSubstr("debug message ["));
    EXPECT_THAT(output, ContainsRegex(R"(debug message \[.*:\d+\])"));

    // Info messages should not include location
    EXPECT_THAT(output, HasSubstr("info message"));
    EXPECT_THAT(output, Not(ContainsRegex(R"(info message \[.*:\d+\])")));
}
