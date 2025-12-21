
#ifdef __cplusplus
extern "C++" {
#endif

#include <stdexcept>
#include <derive-cpp/fmt/c_style.hpp>

#define DC_PANIC(...) throw std::runtime_error(derivecpp::fmt::c_style(__VA_ARGS__))

#ifdef __cplusplus
} // extern "C++"
#endif