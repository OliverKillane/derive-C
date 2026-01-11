#pragma once

#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

#define DC_PTR_REPLACE "<ptr>"

namespace derivecpp::fmt {

namespace {

bool is_hex_digit(unsigned char c) noexcept { return std::isxdigit(c) != 0; }

std::string pointer_replace(const std::string& input) {
    constexpr size_t kMaxHexDigits = sizeof(void*) * 2;

    std::string out;
    out.reserve(input.size());

    for (size_t i = 0; i < input.size();) {
        if (i + 3 <= input.size() && input[i] == '0' &&
            (input[i + 1] == 'x' || input[i + 1] == 'X') &&
            is_hex_digit(static_cast<unsigned char>(input[i + 2]))) {

            size_t j = i + 2;
            size_t digits = 0;
            while (j < input.size() && digits < kMaxHexDigits &&
                   is_hex_digit(static_cast<unsigned char>(input[j]))) {
                ++j;
                ++digits;
            }

            // Boundary check: avoid matching inside longer hex runs
            const bool right_ok =
                (j == input.size()) || !is_hex_digit(static_cast<unsigned char>(input[j]));

            if (right_ok) {
                out += DC_PTR_REPLACE;
                i = j;
                continue;
            }
        }

        out.push_back(input[i]);
        ++i;
    }

    return out;
}
} // namespace
} // namespace derivecpp::fmt
