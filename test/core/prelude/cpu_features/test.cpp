
#include <gtest/gtest.h>

#include <derive-c/core/prelude.h>
#include <derive-c/utils/debug/string.h>

TEST(CoreTests, CpuFeaturesDebugToString) {
    dc_cpu_features features = {
        .SSE = {"sse", true, true},
        .SSE2 = {"sse2", true, false},
        .SSE3 = {"sse3", false, true},
        .SSSE3 = {"ssse3", false, false},
        .SSE4_1 = {"sse4.1", true, true},
        .SSE4_2 = {"sse4.2", true, false},
        .AVX = {"avx", false, true},
        .AVX2 = {"avx2", false, false},
        .FMA = {"fma", true, true},
        .BMI = {"bmi", true, false},
        .BMI2 = {"bmi2", false, true},
        .AES = {"aes", false, false},
        .PCLMUL = {"pclmul", true, true},
        .POPCNT = {"popcnt", true, false},
        .LZCNT = {"lzcnt", false, true},
    };

    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    dc_cpu_features_debug(&features, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

    char const* str = dc_debug_string_builder_string(&sb);
    ASSERT_NE(str, nullptr);

    char const* expected =
        // clang-format off
        "| feature      | compiler | runtime  |\n"
        "| ------------ | -------- | -------- |\n"
        "| sse          | yes      | yes      |\n"
        "| sse2         | yes      | no       |\n"
        "| sse3         | no       | yes      |\n"
        "| ssse3        | no       | no       |\n"
        "| sse4.1       | yes      | yes      |\n"
        "| sse4.2       | yes      | no       |\n"
        "| avx          | no       | yes      |\n"
        "| avx2         | no       | no       |\n"
        "| fma          | yes      | yes      |\n"
        "| bmi          | yes      | no       |\n"
        "| bmi2         | no       | yes      |\n"
        "| aes          | no       | no       |\n"
        "| pclmul       | yes      | yes      |\n"
        "| popcnt       | yes      | no       |\n"
        "| lzcnt        | no       | yes      |\n"
        "\n";
    // clang-format on
    EXPECT_STREQ(str, expected);
}
