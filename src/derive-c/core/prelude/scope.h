#pragma once
#include <derive-c/core/prelude/zerosized.h>

/// RAII in C. Call the destructor when the variable goes out of scope.
#define SCOPED(type) __attribute__((cleanup(type##_delete))) type

/// Simple zero sized value for `DEFER`
ZERO_SIZED(sentinel);

/// Call the function with a null 'sentinel' value.
///  - Useful for cleanup that does not take arguments
#define DEFER(fcn) __attribute__((cleanup(fcn))) sentinel PRIV(NS(sentinel, __COUNTER__)) = {}
