#pragma once

#include <derive-c/core/prelude.h>
#include <derive-c/utils/log/trait.h>

DC_ZERO_SIZED(dc_log_null);
DC_ZERO_SIZED(dc_log_null_global_config);

DC_PUBLIC static dc_log_null dc_log_null_new_global(dc_log_null_global_config config,
                                                    dc_log_id id) {
    (void)config;
    (void)id;
    dc_log_null self = {};
    return self;
}

DC_PUBLIC static dc_log_null dc_log_null_from_parent(dc_log_null* parent, dc_log_id id) {
    (void)parent;
    (void)id;
    dc_log_null self = {};
    return self;
}

DC_PUBLIC static void dc_log_null_log(dc_log_null* self, dc_log_location location,
                                      dc_log_level level, const char* const message, ...) {
    (void)self;
    (void)location;
    (void)level;
    (void)message;
}

DC_PUBLIC static void dc_log_null_delete(dc_log_null* self) { (void)self; }

DC_PUBLIC static void dc_log_null_debug(dc_log_null const* self, dc_debug_fmt fmt, FILE* stream) {
    (void)self;
    (void)fmt;
    fprintf(stream, "dc_log_null { }");
}

DC_TRAIT_LOGGER(dc_log_null);

#if !defined DC_LOGGER
    #define DC_LOGGER dc_log_null
#endif
