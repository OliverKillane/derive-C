#pragma once
#include <derive-c/core/zerosized.h>

/// RAII in C. Call the destructor when the variable goes out of scope.
#define DC_SCOPED(type, ...) __attribute__((cleanup(type##_delete))) type __VA_ARGS__

/// Simple zero sized value for `DC_DEFER`
DC_ZERO_SIZED(sentinel);

/// Call the function with a null 'sentinel' value.
///  - Useful for cleanup that does not take arguments
#define DC_DEFER(fcn) __attribute__((cleanup(fcn))) sentinel PRIV(NS(sentinel, __COUNTER__)) = {}
