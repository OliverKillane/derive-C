#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-c/core/debug/memory_tracker.h>
#include <derive-c/alloc/std.h>

#include <derive-cpp/test/trampoline.hpp>
#include <derive-cpp/test/gtest_mock.hpp>

#define ALLOC stdalloc
#define NAME mock_alloc
#include <derive-c/alloc/wrap/template.h>

#define ALLOC mock_alloc
#define CAPACITY 0
#define NAME hybridstaticempty
#include <derive-c/alloc/hybridstatic/template.h>

using namespace testing;

struct HybridStaticTests : Test {
    FIXTURE_MOCK(HybridStaticTests, void*, mock_alloc_malloc, (mock_alloc * self, size_t size), ());
    FIXTURE_MOCK(HybridStaticTests, void*, mock_alloc_calloc,
                 (mock_alloc * self, size_t count, size_t size), ());
    FIXTURE_MOCK(HybridStaticTests, void*, mock_alloc_realloc,
                 (mock_alloc * self, void* ptr, size_t size), ());
    FIXTURE_MOCK(HybridStaticTests, void, mock_alloc_free, (mock_alloc * self, void* ptr), ());
};

TEST_F(HybridStaticTests, EmptyAllocator) {
    mock_alloc mock = mock_alloc_new(stdalloc_get_ref());
    hybridstaticempty_buffer buf;
    hybridstaticempty alloc = hybridstaticempty_new(&buf, &mock);

    char mocked_allocation[100] = {};
    EXPECT_CALL(*this, mock_alloc_malloc_mock(_, 100))
        .WillOnce(Return((void*)(&mocked_allocation[0])));
    void* ptr1 = hybridstaticempty_malloc(&alloc, 100);
    EXPECT_EQ(ptr1, mocked_allocation);

    hybridstaticempty_delete(&alloc);
}
