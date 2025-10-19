#include <stdexcept>

#include <derive-cpp/fmt/c_style.hpp>

#define PANIC(...) throw std::runtime_error(derivecpp::fmt::c_style(__VA_ARGS__))
