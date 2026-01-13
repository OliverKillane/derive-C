/// @file
/// @example container/map/decomposed.c
/// @brief Examples for inserting, iterating, and deleting hashmaps.

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <derive-c/algorithm/hash/default.h>
#include <derive-c/algorithm/hash/murmur.h>
#include <derive-c/algorithm/hash/combine.h>

#include <derive-c/alloc/std.h>
#include <derive-c/core/prelude.h>
#include <derive-c/utils/for.h>

#define KEY uint32_t
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE char const*
#define NAME id_to_name
#include <derive-c/container/map/decomposed/template.h>

static void print_map(id_to_name const* map) {
    printf("Map has items:\n");
    size_t pos = 0;
    DC_FOR_CONST(id_to_name, map, iter, entry) {
        printf("position: %zu key: %" PRIu32 " string: %s\n", pos, *entry.key, *entry.value);
        pos++;
    }
}

static void id_to_name_example() {
    printf("Id to Name Map Example:\n");
    id_to_name map = id_to_name_new(stdalloc_get_ref());

    id_to_name_insert(&map, 23, "hello");
    id_to_name_insert(&map, 10, "bob");
    id_to_name_insert(&map, 42, "meaning");
    DC_ASSERT(strcmp(*id_to_name_read(&map, 42), "meaning") == 0);

    print_map(&map);

    char const** entry = id_to_name_write(&map, 23);
    DC_ASSERT(entry);
    *entry = "a different string!";

    print_map(&map);

    id_to_name_debug(&map, dc_debug_fmt_new(), stdout);

    id_to_name_delete(&map);
}

struct report_id {
    char* name;
    uint32_t section;
};

static void report_id_debug(struct report_id const* self, dc_debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, " report_id@%p { name: \"%s\", section: %d}", self, self->name, self->section);
}

static bool report_id_equality(struct report_id const* report_1, struct report_id const* report_2) {
    return strcmp(report_1->name, report_2->name) == 0 && report_1->section == report_2->section;
}

static size_t report_id_hash(struct report_id const* report_id) {
    return dc_hash_combine(dc_murmur_hash_string(report_id->name),
                           uint32_t_hash_id(&report_id->section));
}

static void report_id_delete(struct report_id* self) { free(self->name); }

struct report {
    char* description;
    int value;
};

static void report_debug(struct report const* self, dc_debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, " report@%p { description: \"%s\", value: %d}", self, self->description,
            self->value);
}

static void report_delete(struct report* self) { free(self->description); }

#define KEY struct report_id
#define KEY_EQ report_id_equality
#define KEY_HASH report_id_hash
#define KEY_DELETE report_id_delete
#define KEY_DEBUG report_id_debug
#define VALUE struct report
#define VALUE_DELETE report_delete
#define VALUE_DEBUG report_debug
#define NAME report_map
#include <derive-c/container/map/decomposed/template.h>

static void report_map_example() {
    printf("Report Map Example:\n");
    report_map map = report_map_new(stdalloc_get_ref());

    struct report_id id1 = {.name = strdup("Report A"), .section = 1};
    struct report_id id2 = {.name = strdup("Report B"), .section = 2};

    report_map_insert(&map, id1,
                      (struct report){.description = strdup("Description A"), .value = 100});
    report_map_insert(&map, id2,
                      (struct report){.description = strdup("Description B"), .value = 200});

    DC_ASSERT(strcmp(report_map_read(&map, id1)->description, "Description A") == 0);

    {
        size_t pos = 0;
        DC_FOR_CONST(report_map, &map, iter, entry) {
            printf("Position: %zu Key: %s Section: %u Value: %d\n", pos, entry.key->name,
                   entry.key->section, entry.value->value);
            pos++;
        }
    }

    report_map_debug(&map, dc_debug_fmt_new(), stdout);

    struct report entry = report_map_remove(&map, id1);
    report_delete(&entry);

    report_map_debug(&map, dc_debug_fmt_new(), stdout);

    report_map_delete(&map);
}

struct fixed_string {
    char value[4];
};

static void fixed_string_debug(struct fixed_string const* self, dc_debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "fixed_string@%p { value: \"%.*s\" }", self, 4, self->value);
}

static bool fixed_string_eq(struct fixed_string const* str1, struct fixed_string const* str2) {
    return memcmp(str1->value, str2->value, sizeof(str1->value)) == 0;
}

static size_t fixed_string_hash(struct fixed_string const* str) {
    return dc_murmur_hash_string_4(str->value);
}

#define KEY struct fixed_string
#define KEY_EQ fixed_string_eq
#define KEY_HASH fixed_string_hash
#define KEY_DEBUG fixed_string_debug
#define VALUE uint32_t
#define NAME fixed_string_map
#include <derive-c/container/map/decomposed/template.h>

static void fixed_string_example() {
    printf("Fixed Strings Example:\n");
    fixed_string_map map = fixed_string_map_new(stdalloc_get_ref());

    struct fixed_string key1 = {.value = "abc"};
    struct fixed_string key2 = {.value = "def"};
    struct fixed_string key3 = {.value = "ghi"};

    fixed_string_map_insert(&map, key1, 123);
    fixed_string_map_insert(&map, key2, 456);
    fixed_string_map_insert(&map, key3, 789);

    DC_ASSERT(*fixed_string_map_read(&map, key1) == 123);
    DC_ASSERT(*fixed_string_map_read(&map, key2) == 456);
    DC_ASSERT(*fixed_string_map_read(&map, key3) == 789);

    size_t pos = 0;
    DC_FOR_CONST(fixed_string_map, &map, iter, entry) {
        printf("Position: %zu Key: %.3s Value: %u\n", pos, entry.key->value, *entry.value);
        pos++;
    }

    fixed_string_map_debug(&map, dc_debug_fmt_new(), stdout);

    fixed_string_map_delete(&map);
}

int main() {
    id_to_name_example();
    report_map_example();
    fixed_string_example();
}
