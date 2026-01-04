#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-c/core/debug/memory_tracker.h>
#include <derive-c/alloc/std.h>
#include <derive-c/utils/debug.h>

#include <derive-cpp/test/trampoline.hpp>
#include <derive-cpp/test/gtest_mock.hpp>
#include <derive-cpp/fmt/remove_ptrs.hpp>

#define ALLOC stdalloc
#define NAME mock_alloc
#include <derive-c/alloc/wrap/template.h>

#define ALLOC mock_alloc
#define CAPACITY 0
#define NAME hybridstaticempty
#include <derive-c/alloc/hybridstatic/template.h>

using namespace testing;

struct HybridStaticEmptyTests : Test {
    FIXTURE_MOCK(HybridStaticEmptyTests, void*, mock_alloc_malloc, (mock_alloc * self, size_t size),
                 ());
    FIXTURE_MOCK(HybridStaticEmptyTests, void*, mock_alloc_calloc,
                 (mock_alloc * self, size_t count, size_t size), ());
    FIXTURE_MOCK(HybridStaticEmptyTests, void*, mock_alloc_realloc,
                 (mock_alloc * self, void* ptr, size_t size), ());
    FIXTURE_MOCK(HybridStaticEmptyTests, void, mock_alloc_free, (mock_alloc * self, void* ptr), ());
};

TEST_F(HybridStaticEmptyTests, EmptyAllocator) {
    DC_SCOPED(mock_alloc) mock = mock_alloc_new(stdalloc_get_ref());
    hybridstaticempty_buffer buf;
    DC_SCOPED(hybridstaticempty) alloc = hybridstaticempty_new(&buf, &mock);

    char mocked_allocation_storage[100] = {};
    void* mocked_allocation = mocked_allocation_storage;
    EXPECT_CALL(*this, mock_alloc_malloc_mock(_, 100)).WillOnce(Return(mocked_allocation));
    void* ptr1 = hybridstaticempty_malloc(&alloc, 100);
    EXPECT_EQ(ptr1, mocked_allocation);
}

#define ALLOC stdalloc
#define CAPACITY 30
#define NAME hybridstatic
#include <derive-c/alloc/hybridstatic/template.h>

TEST(HybridStaticTests, Debug) {
    hybridstatic_buffer buf;
    DC_SCOPED(hybridstatic) alloc = hybridstatic_new(&buf, stdalloc_get_ref());

    void* ptr1 = hybridstatic_malloc(&alloc, 10);
    void* ptr2 = hybridstatic_calloc(&alloc, 1, 5);
    void* ptr3 = hybridstatic_malloc(&alloc, 15);

    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    hybridstatic_debug(&alloc, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

    EXPECT_EQ(
        // clang-format off
        "hybridstatic@" DC_PTR_REPLACE " {\n"
        "  capacity: 30,\n"
        "  used: 17,\n"
        "  buffer: " DC_PTR_REPLACE ",\n"
        "  alloc: stdalloc@" DC_PTR_REPLACE " { }\n"
        "}",
        // clang-format on
        derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));

    hybridstatic_free(&alloc, ptr1);
    hybridstatic_free(&alloc, ptr2);
    hybridstatic_free(&alloc, ptr3);
}
