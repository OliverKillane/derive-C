#include <stdexcept>

#include <derive-cpp/fmt/c_style.hpp>

#if defined PANIC
    #error "Define panic as gtest first so all includes use this"
#endif

#define PANIC(...) throw std::runtime_error(derivecpp::fmt::c_style(__VA_ARGS__))
