/// @file
/// @example utils/string_builder.c
/// @brief Examples for using the string builder.

#include <derive-c/alloc/std.h>
#include <derive-c/algorithm/hash/default.h>
#include <derive-c/prelude.h>

#define ALLOC stdalloc
#define NAME string_builder
#include <derive-c/utils/string_builder/template.h>

static void example_basic(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(string_builder) sb = string_builder_new(stdalloc_get_ref());

    fprintf(string_builder_stream(&sb), "Hello, world!");

    DC_LOG(log, DC_INFO, "built string: %s", string_builder_string(&sb));
}

#define KEY uint32_t
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE char const*
#define NAME id_map
#include <derive-c/container/map/decomposed/template.h>

static void example_data_structure(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(id_map) map = id_map_new(stdalloc_get_ref());

    DC_LOG(log, DC_INFO, "inserting identities");
    id_map_insert(&map, 1, "Alice");
    id_map_insert(&map, 2, "Bob");
    id_map_insert(&map, 3, "Charlie");

    DC_SCOPED(string_builder) sb = string_builder_new(stdalloc_get_ref());

    fprintf(string_builder_stream(&sb), "the identities are: ");
    id_map_debug(&map, dc_debug_fmt_new(), string_builder_stream(&sb));

    DC_LOG(log, DC_INFO, "%s", string_builder_string(&sb));
}

#define NAME dbg
#include <derive-c/alloc/debug/template.h>

#define CAPACITY 256
#define ALLOC dbg
#define NAME hybrid
#include <derive-c/alloc/hybridstatic/template.h>

#define ALLOC hybrid
#define NAME string_builder_hybrid
#include <derive-c/utils/string_builder/template.h>

static void example_hybrid(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(dbg) debug_alloc = dbg_new("hybrid_example", stdout, stdalloc_get_ref());
    hybrid_buffer buf;
    DC_SCOPED(hybrid) hybrid_alloc = hybrid_new(&buf, &debug_alloc);
    DC_SCOPED(string_builder_hybrid) sb = string_builder_hybrid_new(&hybrid_alloc);

    fprintf(string_builder_hybrid_stream(&sb), "This is a small sentence.");

    DC_LOG(log, DC_INFO, "hybrid built string: %s", string_builder_hybrid_string(&sb));
}

int main() {
    DC_SCOPED(DC_LOGGER)
    root = NS(DC_LOGGER,
              new_global)((NS(DC_LOGGER, global_config)){.stream = stdout, .ansi_colours = true},
                          (dc_log_id){"string_builder"});

    example_basic(&root);
    example_data_structure(&root);
    example_hybrid(&root);
    return 0;
}
