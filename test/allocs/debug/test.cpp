#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-c/alloc/std.h>
#include <derive-c/utils/null_stream.h>
#include <derive-c/utils/debug.h>

#include <derive-cpp/test/trampoline.hpp>
#include <derive-cpp/test/gtest_mock.hpp>
#include <derive-cpp/fmt/c_style.hpp>
#include <derive-cpp/fmt/remove_ptrs.hpp>

#define ALLOC stdalloc
#define NAME stddebugalloc
#include <derive-c/alloc/debug/template.h>

using namespace testing;

TEST(DebugAlloc, BasicAllocation) {
    FILE* stream = dc_null_stream();
    stddebugalloc alloc = stddebugalloc_new("Test Allocator", stream, stdalloc_get_ref());

    void* malloced = stddebugalloc_malloc(&alloc, 100);
    void* calloced = stddebugalloc_calloc(&alloc, 10, 10);
    stddebugalloc_free(&alloc, malloced);
    stddebugalloc_free(&alloc, calloced);
    stddebugalloc_delete(&alloc);
    fclose(stream);
}

#define ALLOC stdalloc
#define NAME mock_alloc
#include <derive-c/alloc/wrap/template.h>

#define ALLOC mock_alloc
#define NAME debug_alloc
#include <derive-c/alloc/debug/template.h>

struct DebugAllocMocked : Test {
    FIXTURE_MOCK(DebugAllocMocked, void*, mock_alloc_malloc, (mock_alloc * self, size_t size), ());
    FIXTURE_MOCK(DebugAllocMocked, void*, mock_alloc_calloc,
                 (mock_alloc * self, size_t count, size_t size), ());
    FIXTURE_MOCK(DebugAllocMocked, void*, mock_alloc_realloc,
                 (mock_alloc * self, void* ptr, size_t size), ());
    FIXTURE_MOCK(DebugAllocMocked, void, mock_alloc_free, (mock_alloc * self, void* ptr), ());
};

TEST_F(DebugAllocMocked, DebugAllocator) {
    DC_SCOPED(mock_alloc) mocked_alloc = mock_alloc_new(stdalloc_get_ref());
    DC_SCOPED(dc_debug_string_builder) sb_log = dc_debug_string_builder_new(stdalloc_get());
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
    DC_SCOPED(debug_alloc)
    alloc = debug_alloc_new("test", dc_debug_string_builder_stream(&sb_log), &mocked_alloc);

    debug_alloc_debug(&alloc, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

    EXPECT_EQ(
        // clang-format off
        "debug_alloc@" DC_PTR_REPLACE  " {\n"
        "  name: test,\n"
        "  alloc: mock_alloc@" DC_PTR_REPLACE " {\n"
        "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
        "    mocking: {\n"
        "      mock_alloc_malloc: enabled,\n"
        "      mock_alloc_calloc: enabled,\n"
        "      mock_alloc_realloc: enabled,\n"
        "      mock_alloc_free: enabled,\n"
        "    }\n"
        "  }\n"
        "}",
        // clang-format on
        derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
}

TEST_F(DebugAllocMocked, DebugLogging) {
    DC_SCOPED(mock_alloc) mocked_alloc = mock_alloc_new(stdalloc_get_ref());
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
    DC_SCOPED(debug_alloc)
    alloc = debug_alloc_new("test", dc_debug_string_builder_stream(&sb), &mocked_alloc);

    char expected_ptr1_storage[1] = {};
    char expected_ptr2_storage[1] = {};
    char expected_ptr3_storage[1] = {};

    void* expected_ptr1 = expected_ptr1_storage;
    void* expected_ptr2 = expected_ptr2_storage;
    void* expected_ptr3 = expected_ptr3_storage;

    EXPECT_CALL(*this, mock_alloc_malloc_mock(_, 10)).WillOnce(Return(expected_ptr1));
    void* ptr1 = debug_alloc_malloc(&alloc, 10);
    EXPECT_EQ(ptr1, expected_ptr1);

    EXPECT_CALL(*this, mock_alloc_calloc_mock(_, 1, 5)).WillOnce(Return(expected_ptr2));
    void* ptr2 = debug_alloc_calloc(&alloc, 1, 5);
    EXPECT_EQ(ptr2, expected_ptr2);

    EXPECT_CALL(*this, mock_alloc_realloc_mock(_, ptr1, 15)).WillOnce(Return(expected_ptr3));
    void* ptr3 = debug_alloc_realloc(&alloc, ptr1, 15);
    EXPECT_EQ(ptr3, expected_ptr3);

    EXPECT_CALL(*this, mock_alloc_free_mock(_, ptr2));
    debug_alloc_free(&alloc, ptr2);
    EXPECT_CALL(*this, mock_alloc_free_mock(_, ptr3));
    debug_alloc_free(&alloc, ptr3);

    std::string debug_log_expected = derivecpp::fmt::c_style(
        // clang-format off
            "[test] debug_alloc_new(alloc=mock_alloc@%p)\n"
            "[test] debug_alloc_malloc(size=10) -> %p\n"
            "[test] debug_alloc_calloc(count=1, size=5) -> %p\n"
            "[test] debug_alloc_realloc(%p, 15) -> %p\n"
            "[test] debug_alloc_free(%p)\n"
            "[test] debug_alloc_free(%p)\n",
        // clang-format on
        &mocked_alloc, ptr1, ptr2, ptr1, ptr3, ptr2, ptr3);
    std::string debug_log = dc_debug_string_builder_string(&sb);

    EXPECT_EQ(debug_log, debug_log_expected);
}
