#pragma once

#include <derive-c/core/prelude.h>

typedef enum {
    DC_TRACE = 1,
    DC_DEBUG,
    DC_INFO,
    DC_WARN,
    DC_ERROR,
} dc_log_level;

DC_PUBLIC static char const* dc_log_level_to_string(dc_log_level level) {
    switch (level) {
    case DC_TRACE:
        return "TRACE";
    case DC_DEBUG:
        return "DEBUG";
    case DC_INFO:
        return "INFO";
    case DC_WARN:
        return "WARN";
    case DC_ERROR:
        return "ERROR";
    }
    return "UNKNOWN";
}

typedef struct {
    char const* file;
    int line;
} dc_log_location;

#define DC_LOCATION                                                                                \
    (dc_log_location) { __FILE__, __LINE__ }

#define DC_LOG_ID_LENGTH 32

typedef struct {
    char name[DC_LOG_ID_LENGTH];
} dc_log_id;

#define DC_TRAIT_LOGGER(SELF)                                                                      \
    DC_REQUIRE_TYPE(SELF, global_config);                                                          \
    DC_REQUIRE_METHOD(SELF, SELF, new_global, (NS(SELF, global_config), dc_log_id id));            \
    DC_REQUIRE_METHOD(SELF, SELF, from_parent, (SELF * parent, dc_log_id id));                     \
    DC_REQUIRE_METHOD(void, SELF, log,                                                             \
                      (SELF*, dc_log_location, dc_log_level, const char* const, ...));             \
    DC_TRAIT_DELETABLE(SELF);                                                                      \
    DC_TRAIT_DEBUGABLE(SELF)
