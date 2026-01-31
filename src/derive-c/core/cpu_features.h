#pragma once

#include <stdbool.h>
#include <stdio.h>

#include <derive-c/core/debug/fmt.h>
#include <derive-c/core/traits/debug.h>

#define CPU_FEATURES(F)                                                                            \
    F(SSE, "sse")                                                                                  \
    F(SSE2, "sse2")                                                                                \
    F(SSE3, "sse3")                                                                                \
    F(SSSE3, "ssse3")                                                                              \
    F(SSE4_1, "sse4.1")                                                                            \
    F(SSE4_2, "sse4.2")                                                                            \
    F(AVX, "avx")                                                                                  \
    F(AVX2, "avx2")                                                                                \
    F(FMA, "fma")                                                                                  \
    F(BMI, "bmi")                                                                                  \
    F(BMI2, "bmi2")                                                                                \
    F(AES, "aes")                                                                                  \
    F(PCLMUL, "pclmul")                                                                            \
    F(POPCNT, "popcnt")                                                                            \
    F(LZCNT, "lzcnt")

typedef struct {
    char const* name;
    bool const compiled_with;
    bool const runtime_supported;
} dc_cpu_feature;

typedef struct {
#define FEATURE_DECLARE(name, _) dc_cpu_feature name;
    CPU_FEATURES(FEATURE_DECLARE)
#undef FEATURE_DECLARE
} dc_cpu_features;

static dc_cpu_features dc_cpu_features_get() {
    return (dc_cpu_features){
#define FEATURE_DETECT(feature_name, runtime_name)                                                 \
    .feature_name = {.name = runtime_name,                                                         \
                     .compiled_with = DC_IS_DEFINED(__##feature_name##__),                         \
                     .runtime_supported = __builtin_cpu_supports(runtime_name)},
        CPU_FEATURES(FEATURE_DETECT)
#undef FEATURE_DETECT
    };
}

DC_PUBLIC static void dc_cpu_features_debug(dc_cpu_features const* self, dc_debug_fmt fmt,
                                            FILE* stream) {
    (void)fmt;
    fprintf(stream, "| %-12s | %-8s | %-8s |\n", "feature", "compiler", "runtime");
    fprintf(stream, "| %-12s | %-8s | %-8s |\n", "------------", "--------", "--------");
#define FEATURE_ROW(feature, _)                                                                    \
    fprintf(stream, "| %-12s | %-8s | %-8s |\n", self->feature.name,                               \
            self->feature.compiled_with ? "yes" : "no",                                            \
            self->feature.runtime_supported ? "yes" : "no");
    CPU_FEATURES(FEATURE_ROW)
#undef FEATURE_ROW
    fprintf(stream, "\n");
}

DC_PUBLIC static void dc_cpu_features_dump(FILE* stream) {
    dc_cpu_features features = dc_cpu_features_get();
    dc_cpu_features_debug(&features, dc_debug_fmt_new(), stream);
}
