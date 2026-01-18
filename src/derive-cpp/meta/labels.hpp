#include <concepts>

// JUSTIFY: Checking associated types
//  - Can be used to mark derive-c struct expansions for benchmarks
#define LABEL_ADD(LABEL) using LABEL = void
#define LABEL_CHECK(T, LABEL)                                                                      \
    ([] {                                                                                          \
        if constexpr (requires { typename T::LABEL; }) {                                           \
            return std::same_as<typename T::LABEL, void>;                                          \
        } else {                                                                                   \
            return false;                                                                          \
        }                                                                                          \
    }())
