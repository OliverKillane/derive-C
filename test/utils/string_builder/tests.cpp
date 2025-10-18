#include <gtest/gtest.h>

extern "C" {
#include <derive-c/alloc/std.h>

#define ALLOC stdalloc
#define NAME string_builder
#include <derive-c/utils/string_builder/template.h>

#define CAPACITY 128
#define NAME alloc_128
#include <derive-c/alloc/staticbump/template.h>

#define ALLOC alloc_128
#define NAME string_builder_static
#include <derive-c/utils/string_builder/template.h>
}

TEST(StringBuilder, Basic) {
    string_builder sb = string_builder_new(stdalloc_get());
    std::string hello_world = "hello world";
    fprintf(string_builder_stream(&sb), "%s", hello_world.c_str());
    EXPECT_EQ(hello_world, string_builder_string(&sb));
    string_builder_delete(&sb);
}

TEST(StringBuilder, BasicStatic) {
    alloc_128 alloc = alloc_128_new();
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
