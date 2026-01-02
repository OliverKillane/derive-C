
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
    string_builder sb = string_builder_new(stdalloc_get());
    std::string hello_world = "hello world";
    fprintf(string_builder_stream(&sb), "%s", hello_world.c_str());
    EXPECT_EQ(hello_world, string_builder_string(&sb));
    string_builder_delete(&sb);
}

TEST(StringBuilder, BasicStatic) {
    alloc_128_buffer buf;
    alloc_128 alloc = alloc_128_new(&buf, stdalloc_get_ref());
    string_builder_static sb = string_builder_static_new(&alloc);
    std::string hello_world = "hello world";
    fprintf(string_builder_static_stream(&sb), "%s", hello_world.c_str());
    EXPECT_EQ(hello_world, string_builder_static_string(&sb));
    string_builder_static_delete(&sb);
}

TEST(StringBuilder, NoOps) {
    string_builder sb = string_builder_new(stdalloc_get());
    string_builder_delete(&sb);
}

TEST(StringBuilder, Release) {
    string_builder sb = string_builder_new(stdalloc_get());
    std::string hello_world = "hello world";
    fprintf(string_builder_stream(&sb), "%s", hello_world.c_str());

    char* hello_world_new = string_builder_release_string(&sb);

    EXPECT_EQ(string_builder_string_size(&sb), 0);
    EXPECT_EQ(std::string(""), string_builder_string(&sb));

    free(hello_world_new);
    string_builder_delete(&sb);
}

TEST(StringBuilder, VeryLargeString) {
    const auto* const repeat = "foooo!!!";
    string_builder sb = string_builder_new(stdalloc_get());
    std::string expected;
    for (auto i = 0; i < 1024; i++) {
        fprintf(string_builder_stream(&sb), "%s", repeat);
        expected.append(repeat);
    }

    EXPECT_EQ(expected, string_builder_string(&sb));
    string_builder_delete(&sb);
}

TEST(StringBuilder, Reset) {
    string_builder sb = string_builder_new(stdalloc_get());
    std::string hello_world = "hello world";

    fprintf(string_builder_stream(&sb), "%s", hello_world.c_str());

    string_builder_reset(&sb);
    EXPECT_EQ(string_builder_string_size(&sb), 0);
    EXPECT_EQ(std::string(""), string_builder_string(&sb));

    fprintf(string_builder_stream(&sb), "%s", hello_world.c_str());
    EXPECT_EQ(hello_world, string_builder_string(&sb));

    string_builder_delete(&sb);
}

TEST(StringBuilder, UnsupportedRead) {
    string_builder sb = string_builder_new(stdalloc_get());

    char buf[8];
    errno = 0;

    size_t n = fread(buf, 1, sizeof(buf), string_builder_stream(&sb));

    EXPECT_EQ(n, 0);
    // JUSTIFY: EBADF even though we panic on read.
    //  - hence this errors before our read is ever called.
    EXPECT_EQ(errno, EBADF);

    string_builder_delete(&sb);
}

TEST(StringBuilder, UnsupportedSeek) {
    string_builder sb = string_builder_new(stdalloc_get());

    errno = 0;
    int ret = fseek(string_builder_stream(&sb), 0, SEEK_SET);

    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EPERM);

    string_builder_delete(&sb);
}
