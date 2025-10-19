#include <cstdarg>
#include <cstdio>
#include <string>

namespace derivecpp::fmt {
namespace {
std::string c_style(const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int len = std::vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    if (len < 0) {
        return {};
    }

    std::string result(len, '\0');

    va_start(args, fmt);
    std::vsnprintf(result.data(), result.size() + 1, fmt, args);
    va_end(args);

    return result;
}
} // namespace
} // namespace derivecpp::fmt