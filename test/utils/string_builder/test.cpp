
#include <gtest/gtest.h>

#include <cerrno>
#include <cstdio>

#include <derive-c/alloc/std.h>

#define ALLOC stdalloc
#define NAME string_builder
#include <derive-c/utils/string_builder/template.h>

#define ALLOC stdalloc
#define CAPACITY 128
#define NAME alloc_128
#include <derive-c/alloc/hybridstatic/template.h>

#define ALLOC alloc_128
#define NAME string_builder_static
#include <derive-c/utils/string_builder/template.h>

TEST(StringBuilder, Basic) {
    DC_SCOPED(string_builder) sb = string_builder_new(stdalloc_get_ref());
    std::string hello_world = "hello world";
    fprintf(string_builder_stream(&sb), "%s", hello_world.c_str());
    EXPECT_EQ(hello_world, string_builder_string(&sb));
}

TEST(StringBuilder, BasicStatic) {
    alloc_128_buffer buf;
    DC_SCOPED(alloc_128) alloc = alloc_128_new(&buf, stdalloc_get_ref());
    DC_SCOPED(string_builder_static) sb = string_builder_static_new(&alloc);
    std::string hello_world = "hello world";
    fprintf(string_builder_static_stream(&sb), "%s", hello_world.c_str());
    EXPECT_EQ(hello_world, string_builder_static_string(&sb));
}

TEST(StringBuilder, NoOps) {
    DC_SCOPED(string_builder) sb = string_builder_new(stdalloc_get_ref());
}

TEST(StringBuilder, Release) {
    DC_SCOPED(string_builder) sb = string_builder_new(stdalloc_get_ref());
    std::string hello_world = "hello world";
    fprintf(string_builder_stream(&sb), "%s", hello_world.c_str());

    char* hello_world_new = string_builder_release_string(&sb);

    EXPECT_EQ(string_builder_string_size(&sb), 0);
    EXPECT_EQ(std::string(""), string_builder_string(&sb));

    // JUSTIFY: naked free of stdalloc created pointer
    //  - If allocating from stdalloc, we can just use the normal functions
    free(hello_world_new);
}

TEST(StringBuilder, VeryLargeString) {
    const auto* const repeat = "foooo!!!";
    DC_SCOPED(string_builder) sb = string_builder_new(stdalloc_get_ref());
    std::string expected;
    for (auto i = 0; i < 1024; i++) {
        fprintf(string_builder_stream(&sb), "%s", repeat);
        expected.append(repeat);
    }

    EXPECT_EQ(expected, string_builder_string(&sb));
}

TEST(StringBuilder, Reset) {
    DC_SCOPED(string_builder) sb = string_builder_new(stdalloc_get_ref());
    std::string hello_world = "hello world";

    fprintf(string_builder_stream(&sb), "%s", hello_world.c_str());

    string_builder_reset(&sb);
    EXPECT_EQ(string_builder_string_size(&sb), 0);
    EXPECT_EQ(std::string(""), string_builder_string(&sb));

    fprintf(string_builder_stream(&sb), "%s", hello_world.c_str());
    EXPECT_EQ(hello_world, string_builder_string(&sb));
}

TEST(StringBuilder, UnsupportedRead) {
    DC_SCOPED(string_builder) sb = string_builder_new(stdalloc_get_ref());

    char buf[8];
    errno = 0;

    size_t n = fread(buf, 1, sizeof(buf), string_builder_stream(&sb));

    EXPECT_EQ(n, 0);
    // JUSTIFY: EBADF even though we panic on read.
    //  - hence this errors before our read is ever called.
    EXPECT_EQ(errno, EBADF);
}

TEST(StringBuilder, UnsupportedSeek) {
    DC_SCOPED(string_builder) sb = string_builder_new(stdalloc_get_ref());

    errno = 0;
    int ret = fseek(string_builder_stream(&sb), 0, SEEK_SET);

    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EPERM);
}
