#pragma once

/// @brief Utility to trigger a compile-time error when an unreachable branch is taken
///
/// This should be used in if constexpr chains where all expected types are handled,
/// and any other type reaching this point indicates a programming error that should
/// be caught at compile time.
///
/// Usage:
///   if constexpr (LABEL_CHECK(Impl, label1)) {
///       // handle label1
///   } else if constexpr (LABEL_CHECK(Impl, label2)) {
///       // handle label2
///   } else {
///       static_assert_unreachable<Impl>();
///   }
template <typename T> consteval bool always_false() { return false; }

template <typename T> void static_assert_unreachable() {
    static_assert(always_false<T>(), "Unreachable: unknown or unhandled implementation type");
}
