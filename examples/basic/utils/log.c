/// @file
/// @example utils/log.c
/// @brief Examples for using loggers.

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>
#include <derive-c/utils/debug/string.h>

#define ITEM dc_log_file
#define ITEM_DELETE dc_log_file_delete
#define NAME logger_vec
#include <derive-c/container/vector/dynamic/template.h>

static void example_1(dc_log_file* parent) {
    DC_SCOPED(dc_log_file) demo = DC_LOGGER_NEW(parent, "%s", "demo");

    DC_SCOPED(logger_vec) loggers = logger_vec_new(stdalloc_get_ref());

    for (int i = 1; i <= 5; i++) {
        dc_log_file logger = DC_LOGGER_NEW(&demo, "logger_%d", i);
        logger_vec_push(&loggers, logger);
    }

    DC_FOR(logger_vec, &loggers, iter, logger) { DC_LOG(*logger, DC_INFO, "hello world"); }
}

static void example_2(dc_log_file* parent) {
    DC_SCOPED(dc_log_file) foo = DC_LOGGER_NEW(parent, "%s", "foo");

    dc_log_file_set_filter(&foo, DC_DEBUG);
    DC_LOG(foo, DC_DEBUG, "this debug message is visible");

    dc_log_file_set_filter(&foo, DC_INFO);
    DC_LOG(foo, DC_DEBUG, "this debug message is filtered out");
    DC_LOG(foo, DC_INFO, "this info message is visible");
}

static void example_3(dc_log_file* parent) {
    DC_SCOPED(dc_log_file) level1 = DC_LOGGER_NEW(parent, "%s", "level1");
    DC_SCOPED(dc_log_file) level2 = DC_LOGGER_NEW(&level1, "%s", "level2");
    DC_SCOPED(dc_log_file) level3 = DC_LOGGER_NEW(&level2, "%s", "level3");
    DC_SCOPED(dc_log_file) level4 = DC_LOGGER_NEW(&level3, "%s", "level4");

    DC_LOG(level4, DC_INFO, "logger debug info:\n%s", DC_DEBUG(dc_log_file_debug, &level4));
}

static void example_4(dc_log_file* parent) {
    DC_SCOPED(dc_log_file) levels = DC_LOGGER_NEW(parent, "%s", "levels");

    dc_log_file_set_filter(&levels, DC_TRACE);

    DC_LOG(levels, DC_TRACE, "this is a trace message");
    DC_LOG(levels, DC_DEBUG, "this is a debug message");
    DC_LOG(levels, DC_INFO, "this is an info message");
    DC_LOG(levels, DC_WARN, "this is a warning message");
    DC_LOG(levels, DC_ERROR, "this is an error message");
}

int main() {
    DC_SCOPED(dc_log_file)
    root = NS(DC_LOGGER,
              new_global)((NS(DC_LOGGER, global_config)){.stream = stdout, .ansi_colours = true},
                          (dc_log_id){"root"});
    dc_log_file_set_filter(&root, DC_TRACE);

    example_1(&root);
    example_2(&root);
    example_3(&root);
    example_4(&root);
    return 0;
}
