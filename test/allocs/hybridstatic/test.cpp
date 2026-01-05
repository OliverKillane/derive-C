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
    FIXTURE_MOCK(HybridStaticEmptyTests, void*, mock_alloc_allocate_uninit,
                 (mock_alloc * self, size_t size), ());
    FIXTURE_MOCK(HybridStaticEmptyTests, void*, mock_alloc_allocate_zeroed,
                 (mock_alloc * self, size_t size), ());
    FIXTURE_MOCK(HybridStaticEmptyTests, void*, mock_alloc_reallocate,
                 (mock_alloc * self, void* ptr, size_t old_size, size_t new_size), ());
    FIXTURE_MOCK(HybridStaticEmptyTests, void, mock_alloc_deallocate,
                 (mock_alloc * self, void* ptr, size_t size), ());
};

TEST_F(HybridStaticEmptyTests, EmptyAllocator) {
    DC_SCOPED(mock_alloc) mock = mock_alloc_new(stdalloc_get_ref());
    hybridstaticempty_buffer buf;
    DC_SCOPED(hybridstaticempty) alloc = hybridstaticempty_new(&buf, &mock);

    char mocked_allocation_storage[100] = {};
    void* mocked_allocation = mocked_allocation_storage;
    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 100)).WillOnce(Return(mocked_allocation));
    void* ptr1 = hybridstaticempty_allocate_uninit(&alloc, 100);
    EXPECT_EQ(ptr1, mocked_allocation);
}

#define ALLOC stdalloc
#define CAPACITY 30
#define NAME hybridstatic
#include <derive-c/alloc/hybridstatic/template.h>

TEST(HybridStaticTests, Debug) {
    hybridstatic_buffer buf;
    DC_SCOPED(hybridstatic) alloc = hybridstatic_new(&buf, stdalloc_get_ref());

    void* ptr1 = hybridstatic_allocate_uninit(&alloc, 10);
    void* ptr2 = hybridstatic_allocate_zeroed(&alloc, 5);
    void* ptr3 = hybridstatic_allocate_uninit(&alloc, 15);

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

    hybridstatic_deallocate(&alloc, ptr1, 10);
    hybridstatic_deallocate(&alloc, ptr2, 5);
    hybridstatic_deallocate(&alloc, ptr3, 15);
}
