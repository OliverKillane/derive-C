
#if !defined NDEBUG
    #include <gmock/gmock.h>
    #include <gtest/gtest.h>

    #include <derive-cpp/test/gtest_mock.hpp>
    #include <derive-cpp/fmt/remove_ptrs.hpp>

    #include <derive-c/alloc/std.h>
    #include <derive-c/utils/debug.h>

    #define ALLOC stdalloc
    #define NAME testalloc
    #include <derive-c/alloc/test/template.h>

namespace {
void allocate_and_throw(testalloc* alloc) {
    void* a = testalloc_allocate_uninit(alloc, 10000000);
    ((int*)a)[12] = 42;
    DC_PANIC("problem!");
}
} // namespace

using namespace testing;

TEST(TestAlloc, BasicAllocation) {
    DC_SCOPED(testalloc) alloc = testalloc_new(stdalloc_get_ref());
    EXPECT_ANY_THROW(allocate_and_throw(&alloc));
    testalloc_unleak(&alloc);
}

    #define ALLOC stdalloc
    #define NAME mock_alloc
    #include <derive-c/alloc/wrap/template.h>

    #define ALLOC mock_alloc
    #define NAME test_with_mock_alloc
    #include <derive-c/alloc/test/template.h>

struct TestAllocWithMock : Test {
    FIXTURE_MOCK(TestAllocWithMock, void*, mock_alloc_allocate_uninit,
                 (mock_alloc * self, size_t size), ());
    FIXTURE_MOCK(TestAllocWithMock, void*, mock_alloc_allocate_zeroed,
                 (mock_alloc * self, size_t size), ());
    FIXTURE_MOCK(TestAllocWithMock, void*, mock_alloc_reallocate,
                 (mock_alloc * self, void* ptr, size_t old_size, size_t new_size), ());
    FIXTURE_MOCK(TestAllocWithMock, void, mock_alloc_deallocate,
                 (mock_alloc * self, void* ptr, size_t size), ());
};

TEST_F(TestAllocWithMock, DebugAllocations) {
    DC_SCOPED(mock_alloc) mocked_alloc = mock_alloc_new(stdalloc_get_ref());
    DC_SCOPED(test_with_mock_alloc) alloc = test_with_mock_alloc_new(&mocked_alloc);

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_with_mock_alloc_debug(&alloc, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        std::string const debug_string = dc_debug_string_builder_string(&sb);
        EXPECT_EQ(
            // clang-format off
            "test_with_mock_alloc @" DC_PTR_REPLACE " {\n"
            "  base: mock_alloc@" DC_PTR_REPLACE ",\n"
            "  allocations: test_with_mock_alloc_allocations@" DC_PTR_REPLACE " {\n"
            "    capacity: 256,\n"
            "    tombstones: 0,\n"
            "    count: 0,\n"
            "    ctrl: @" DC_PTR_REPLACE "[256 + simd probe size additional 16],\n"
            "    slots: @" DC_PTR_REPLACE "[256],\n"
            "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "    entries: [\n"
            "    ]\n"
            "  }\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(debug_string));
    }

    // JUSTIFY: Only debug printing with 1 allocation
    //  - We can get the expected ordering knowing the values of the pointers & their hashes
    //  - However this would require taking the addresses, hashing, then dynamically constructing
    //  the
    //    expected debug string with that ordering. This is unecessary complexity - we already have
    //    tests for the swisstable expected debug output.
    char ptr1_storage[10] = {};
    void* expected_ptr1 = &ptr1_storage[0];

    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 10)).WillOnce(Return(expected_ptr1));
    void* ptr1 = test_with_mock_alloc_allocate_uninit(&alloc, 10);

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_with_mock_alloc_debug(&alloc, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        std::string const debug_string = dc_debug_string_builder_string(&sb);

        EXPECT_EQ(
            // clang-format off
            "test_with_mock_alloc @" DC_PTR_REPLACE " {\n"
            "  base: mock_alloc@" DC_PTR_REPLACE ",\n"
            "  allocations: test_with_mock_alloc_allocations@" DC_PTR_REPLACE " {\n"
            "    capacity: 256,\n"
            "    tombstones: 0,\n"
            "    count: 1,\n"
            "    ctrl: @" DC_PTR_REPLACE "[256 + simd probe size additional 16],\n"
            "    slots: @" DC_PTR_REPLACE "[256],\n"
            "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "    entries: [\n"
            "      {\n"
            "        key: void*@" DC_PTR_REPLACE ",\n"
            "        value: 10,\n"
            "      },\n"
            "    ]\n"
            "  }\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(debug_string));
    }

    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, _, 10));
    test_with_mock_alloc_deallocate(&alloc, ptr1, 10);
    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_with_mock_alloc_debug(&alloc, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        std::string const debug_string = dc_debug_string_builder_string(&sb);
        EXPECT_EQ(
            // clang-format off
            "test_with_mock_alloc @" DC_PTR_REPLACE " {\n"
            "  base: mock_alloc@" DC_PTR_REPLACE ",\n"
            "  allocations: test_with_mock_alloc_allocations@" DC_PTR_REPLACE " {\n"
            "    capacity: 256,\n"
            "    tombstones: 1,\n"
            "    count: 0,\n"
            "    ctrl: @" DC_PTR_REPLACE "[256 + simd probe size additional 16],\n"
            "    slots: @" DC_PTR_REPLACE "[256],\n"
            "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "    entries: [\n"
            "    ]\n"
            "  }\n"
            "}"
            ,
            // clang-format on
            derivecpp::fmt::pointer_replace(debug_string));
    }
}

TEST_F(TestAllocWithMock, ReallocateSamePointer) {
    DC_SCOPED(mock_alloc) mocked_alloc = mock_alloc_new(stdalloc_get_ref());
    DC_SCOPED(test_with_mock_alloc) alloc = test_with_mock_alloc_new(&mocked_alloc);

    char ptr1_storage[10] = {};

    void* expected_ptr1 = ptr1_storage;

    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 10)).WillOnce(Return(expected_ptr1));
    void* ptr1 = test_with_mock_alloc_allocate_uninit(&alloc, 10);
    EXPECT_EQ(ptr1, expected_ptr1);

    EXPECT_CALL(*this, mock_alloc_reallocate_mock(_, ptr1, 10, 5)).WillOnce(Return(expected_ptr1));
    void* ptr1_reallocated = test_with_mock_alloc_reallocate(&alloc, ptr1, 10, 5);
    EXPECT_EQ(ptr1, ptr1_reallocated);

    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, _, _));
    test_with_mock_alloc_deallocate(&alloc, ptr1_reallocated, 5);
}

TEST_F(TestAllocWithMock, ReallocateDifferentPointer) {
    DC_SCOPED(mock_alloc) mocked_alloc = mock_alloc_new(stdalloc_get_ref());
    DC_SCOPED(test_with_mock_alloc) alloc = test_with_mock_alloc_new(&mocked_alloc);

    char ptr1_storage[10] = {};
    char ptr2_storage[5] = {};

    void* expected_ptr1 = ptr1_storage;
    void* expected_ptr2 = ptr2_storage;

    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 10)).WillOnce(Return(expected_ptr1));
    void* ptr1 = test_with_mock_alloc_allocate_uninit(&alloc, 10);
    EXPECT_EQ(ptr1, expected_ptr1);

    EXPECT_CALL(*this, mock_alloc_reallocate_mock(_, ptr1, 10, 5)).WillOnce(Return(expected_ptr2));
    void* ptr1_reallocated = test_with_mock_alloc_reallocate(&alloc, ptr1, 10, 5);
    EXPECT_EQ(expected_ptr2, ptr1_reallocated);

    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, _, _));
    test_with_mock_alloc_deallocate(&alloc, ptr1_reallocated, 5);
}

TEST_F(TestAllocWithMock, ReuseFreedAllocation) {
    DC_SCOPED(mock_alloc) mocked_alloc = mock_alloc_new(stdalloc_get_ref());
    DC_SCOPED(test_with_mock_alloc) alloc = test_with_mock_alloc_new(&mocked_alloc);

    char ptr1_storage[10] = {};

    void* expected_ptr1 = ptr1_storage;
    void* expected_ptr2 = ptr1_storage;

    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 10)).WillOnce(Return(expected_ptr1));
    void* ptr1 = test_with_mock_alloc_allocate_uninit(&alloc, 10);
    EXPECT_EQ(ptr1, expected_ptr1);

    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, _, _));
    test_with_mock_alloc_deallocate(&alloc, ptr1, 10);

    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 10)).WillOnce(Return(expected_ptr2));
    void* ptr2 = test_with_mock_alloc_allocate_uninit(&alloc, 10);
    EXPECT_EQ(ptr2, expected_ptr2);

    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, _, _));
    test_with_mock_alloc_deallocate(&alloc, ptr2, 10);
}

TEST_F(TestAllocWithMock, ReuseReallocedAllocation) {
    DC_SCOPED(mock_alloc) mocked_alloc = mock_alloc_new(stdalloc_get_ref());
    DC_SCOPED(test_with_mock_alloc) alloc = test_with_mock_alloc_new(&mocked_alloc);

    char ptr1_storage[10] = {};
    char ptr2_storage[5] = {};

    void* expected_ptr1 = ptr1_storage;
    void* expected_ptr2 = ptr2_storage;
    void* expected_ptr3 = ptr1_storage;

    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 10)).WillOnce(Return(expected_ptr1));
    void* ptr1 = test_with_mock_alloc_allocate_uninit(&alloc, 10);
    EXPECT_EQ(ptr1, expected_ptr1);

    EXPECT_CALL(*this, mock_alloc_reallocate_mock(_, ptr1, 10, 5)).WillOnce(Return(expected_ptr2));
    void* ptr1_reallocated = test_with_mock_alloc_reallocate(&alloc, ptr1, 10, 5);
    EXPECT_EQ(expected_ptr2, ptr1_reallocated);

    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 10)).WillOnce(Return(expected_ptr3));
    void* ptr3 = test_with_mock_alloc_allocate_uninit(&alloc, 10);
    EXPECT_EQ(ptr3, expected_ptr3);

    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, _, _));
    test_with_mock_alloc_deallocate(&alloc, ptr1_reallocated, 5);
    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, _, _));
    test_with_mock_alloc_deallocate(&alloc, ptr3, 10);
}

#endif
