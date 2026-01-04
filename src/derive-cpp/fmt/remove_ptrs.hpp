#pragma once

#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

#define DC_PTR_REPLACE "<ptr>"

namespace derivecpp::fmt {

namespace {

bool is_hex_digit(unsigned char c) noexcept { return std::isxdigit(c) != 0; }

// Replaces `%p`-style pointers (glibc-style) like 0x728cc5bdfd10 with DC_PTR_REPLACE.
// Matches: 0x + 1..(sizeof(void*)*2) hex digits.
std::string pointer_replace(std::string_view input) {
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
