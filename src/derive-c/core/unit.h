#pragma once

#include <derive-c/core/debug/fmt.h>
#include <derive-c/core/zerosized.h>
#include <derive-c/core/namespace.h>

DC_ZERO_SIZED(dc_unit);

DC_PUBLIC static dc_unit dc_unit_new() { return (dc_unit){}; }

DC_PUBLIC static dc_unit dc_unit_clone(dc_unit const* self) { return *self; }

DC_PUBLIC static void dc_unit_debug(dc_unit const* /* self */, dc_debug_fmt /* fmt */,
                                    FILE* stream) {
    fprintf(stream, "[UNIT]");
}

DC_PUBLIC static void dc_unit_delete(dc_unit* /* self */) {}
