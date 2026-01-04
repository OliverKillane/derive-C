
#if !defined NDEBUG

    #include <gtest/gtest.h>

    #include <derive-c/alloc/std.h>
    #include <derive-c/utils/debug.h>

    #define ALLOC stdalloc
    #define ENTRIES allocs
    #define NAME testalloc
    #include <derive-c/alloc/test/template.h>

namespace {
void allocate_and_throw(testalloc* alloc) {
    void* a = testalloc_malloc(alloc, 10000000);
    ((int*)a)[12] = 42;
    DC_PANIC("problem!");
}
} // namespace

TEST(TestAlloc, BasicAllocation) {
    testalloc alloc = testalloc_new(stdalloc_get_ref());
    EXPECT_ANY_THROW(allocate_and_throw(&alloc));
    testalloc_unleak_and_delete(&alloc);
}

TEST(TestAlloc, DebugAllocations) {
    DC_SCOPED(testalloc) alloc = testalloc_new(stdalloc_get_ref());

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        testalloc_debug(&alloc, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        std::string const debug_string_expected = derivecpp::fmt::c_style(
            // clang-format off
            "testalloc @%p {\n"
            "  base: stdalloc@%p,\n"
            "  entries: testalloc_entries@%p {\n"
            "    size: 0,\n"
            "    capacity: 0,\n"
            "    alloc: stdalloc@%p { },\n"
            "    items: @(nil) [\n"
            "    ],\n"
            "  }\n"
            "}",
            // clang-format on
            &alloc, &stdalloc_instance, &alloc.entries, &stdalloc_instance);
        std::string const debug_string = dc_debug_string_builder_string(&sb);
        EXPECT_EQ(debug_string, debug_string_expected);
    }

    void* ptr1 = testalloc_malloc(&alloc, 10);
    void* ptr2 = testalloc_calloc(&alloc, 10, 30);
    void* ptr3 = testalloc_malloc(&alloc, 1);

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        testalloc_debug(&alloc, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        std::string const debug_string_expected = derivecpp::fmt::c_style(
            // clang-format off
            "testalloc @%p {\n"
            "  base: stdalloc@%p,\n"
            "  entries: testalloc_entries@%p {\n"
            "    size: 3,\n"
            "    capacity: 8,\n"
            "    alloc: stdalloc@%p { },\n"
            "    items: @%p [\n"
            "      { ptr: %p, state: alive },\n"
            "      { ptr: %p, state: alive },\n"
            "      { ptr: %p, state: alive },\n"
            "    ],\n"
            "  }\n"
            "}",
            // clang-format on
            &alloc, &stdalloc_instance, &alloc.entries, &stdalloc_instance, alloc.entries.data,
            ptr1, ptr2, ptr3);
        std::string const debug_string = dc_debug_string_builder_string(&sb);
        EXPECT_EQ(debug_string, debug_string_expected);
    }

    testalloc_free(&alloc, ptr1);
    testalloc_free(&alloc, ptr2);
    testalloc_free(&alloc, ptr3);

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        testalloc_debug(&alloc, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        std::string const debug_string_expected = derivecpp::fmt::c_style(
            // clang-format off
            "testalloc @%p {\n"
            "  base: stdalloc@%p,\n"
            "  entries: testalloc_entries@%p {\n"
            "    size: 3,\n"
            "    capacity: 8,\n"
            "    alloc: stdalloc@%p { },\n"
            "    items: @%p [\n"
            "      { ptr: %p, state: freed },\n"
            "      { ptr: %p, state: freed },\n"
            "      { ptr: %p, state: freed },\n"
            "    ],\n"
            "  }\n"
            "}",
            // clang-format on
            &alloc, &stdalloc_instance, &alloc.entries, &stdalloc_instance, alloc.entries.data,
            ptr1, ptr2, ptr3);
        std::string const debug_string = dc_debug_string_builder_string(&sb);
        EXPECT_EQ(debug_string, debug_string_expected);
    }
}

#endif
