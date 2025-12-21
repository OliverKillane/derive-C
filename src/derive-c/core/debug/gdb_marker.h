#pragma once

#include <derive-c/core/zerosized.h>

DC_ZERO_SIZED(dc_gdb_marker);

static dc_gdb_marker dc_gdb_marker_new() { return (dc_gdb_marker){}; }
