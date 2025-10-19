#pragma once

#include <derive-c/core/prelude/zerosized.h>

ZERO_SIZED(gdb_marker);

static gdb_marker gdb_marker_new() { return (gdb_marker){}; }
