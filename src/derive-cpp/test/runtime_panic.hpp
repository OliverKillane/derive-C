#pragma once

#ifdef __cplusplus
extern "C++" {
#endif

#include <stdexcept>
#include <derive-cpp/fmt/c_style.hpp>

#define DC_PANIC(str, ...)                                                                         \
    throw std::runtime_error(                                                                      \
        derivecpp::fmt::c_style("[%s:%d] " str, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__))

#ifdef __cplusplus
} // extern "C++"
#endif
