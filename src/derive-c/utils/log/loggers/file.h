#pragma once

#include <derive-c/core/prelude.h>
#include <derive-c/utils/log/trait.h>
#include <derive-c/utils/timestamp.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    FILE* stream;
    bool ansi_colours;
} dc_log_file_global_config;

typedef struct _dc_log_file {
    struct _dc_log_file* parent;
    FILE* stream;
    dc_log_id id;
    dc_log_level filter;
    bool ansi_colours;
} dc_log_file;

#define DC_LOG_ANSI_RESET "\033[0m"
#define DC_LOG_ANSI_PURPLE "\033[35m"
#define DC_LOG_ANSI_BLUE "\033[34m"
#define DC_LOG_ANSI_GREEN "\033[32m"
#define DC_LOG_ANSI_ORANGE "\033[33m"
#define DC_LOG_ANSI_RED "\033[31m"

DC_INTERNAL static char const* dc_log_level_ansi_colour(dc_log_level level) {
    switch (level) {
    case DC_TRACE:
        return DC_LOG_ANSI_PURPLE;
    case DC_DEBUG:
        return DC_LOG_ANSI_BLUE;
    case DC_INFO:
        return DC_LOG_ANSI_GREEN;
    case DC_WARN:
        return DC_LOG_ANSI_ORANGE;
    case DC_ERROR:
        return DC_LOG_ANSI_RED;
    }
    return DC_LOG_ANSI_RESET;
}

DC_PUBLIC static bool dc_log_file_should_log(dc_log_file const* self, dc_log_level level) {
    if (level < self->filter) {
        return false;
    }
    if (self->parent != NULL) {
        return dc_log_file_should_log(self->parent, level);
    }
    return true;
}

DC_PUBLIC static dc_log_level dc_log_file_get_filter(dc_log_file const* self) {
    return self->filter;
}

DC_INTERNAL static void dc_log_file_print_descriptor(dc_log_file const* self, FILE* stream) {
    if (self->parent != NULL) {
        dc_log_file_print_descriptor(self->parent, stream);
        fprintf(stream, "/");
    }
    fprintf(stream, "%s", self->id.name);
}

DC_PUBLIC static void dc_log_file_log(dc_log_file* self, dc_log_location location,
                                      dc_log_level level, const char* const message, ...)
#if defined(__clang__) || defined(__GNUC__)
    __attribute__((format(printf, 4, 5)))
#endif
    ;

DC_PUBLIC static void dc_log_file_log(dc_log_file* self, dc_log_location location,
                                      dc_log_level level, const char* const message, ...) {
    if (!dc_log_file_should_log(self, level)) {
        return;
    }

    if (self->ansi_colours) {
        fprintf(self->stream, "%s", dc_log_level_ansi_colour(level));
    }

    dc_datetime dt = dc_datetime_now_utc();
    fprintf(self->stream, "[");
    dc_datetime_format(&dt, self->stream);
    fprintf(self->stream, "] [%s] [", dc_log_level_to_string(level));
    dc_log_file_print_descriptor(self, self->stream);
    fprintf(self->stream, "] ");

    va_list args;
    va_start(args, message);
    vfprintf(self->stream, message, args);
    va_end(args);

    if (level <= DC_DEBUG) {
        fprintf(self->stream, " [%s:%d]", location.file, location.line);
    }

    if (self->ansi_colours) {
        fprintf(self->stream, "%s", DC_LOG_ANSI_RESET);
    }

    fprintf(self->stream, "\n");
    fflush(self->stream);
}

DC_PUBLIC static void dc_log_file_set_filter(dc_log_file* self, dc_log_level level) {
    dc_log_level old_level = self->filter;
    self->filter = level;
    dc_log_file_log(self, DC_LOCATION, DC_INFO, "Log level changed from %s to %s",
                    dc_log_level_to_string(old_level), dc_log_level_to_string(level));
}

DC_PUBLIC static dc_log_file dc_log_file_new_global(dc_log_file_global_config config,
                                                    dc_log_id id) {
    dc_log_file self = {
        .parent = NULL,
        .stream = config.stream,
        .id = id,
        .filter = DC_INFO,
        .ansi_colours = config.ansi_colours,
    };
    dc_log_file_log(&self, DC_LOCATION, DC_INFO, "Logger created");
    return self;
}

DC_PUBLIC static dc_log_file dc_log_file_from_parent(dc_log_file* parent, dc_log_id id) {
    dc_log_file self = {
        .parent = parent,
        .stream = parent->stream,
        .id = id,
        .filter = parent->filter,
        .ansi_colours = parent->ansi_colours,
    };
    dc_log_file_log(&self, DC_LOCATION, DC_INFO, "Logger created");
    return self;
}

DC_PUBLIC static void dc_log_file_delete(dc_log_file* self) {
    dc_log_file_log(self, DC_LOCATION, DC_INFO, "Logger deleted");
}

DC_PUBLIC static void dc_log_file_debug(dc_log_file const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, "dc_log_file@%p {\n", (void const*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);

    dc_debug_fmt_print(fmt, stream, "parent: ");
    if (self->parent != NULL) {
        dc_log_file_debug(self->parent, fmt, stream);
    } else {
        fprintf(stream, "NULL");
    }
    fprintf(stream, ",\n");

    dc_debug_fmt_print(fmt, stream, "stream: %p,\n", (void const*)self->stream);
    dc_debug_fmt_print(fmt, stream, "id: \"%s\",\n", self->id.name);
    dc_debug_fmt_print(fmt, stream, "filter: %s,\n", dc_log_level_to_string(self->filter));
    dc_debug_fmt_print(fmt, stream, "ansi_colours: %s,\n", self->ansi_colours ? "true" : "false");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

DC_TRAIT_LOGGER(dc_log_file);

#if !defined DC_LOGGER
    #define DC_LOGGER dc_log_file
#endif
