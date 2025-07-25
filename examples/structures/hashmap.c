/// @file
/// @example structures/hashmap.c
/// @brief Examples for inserting, iterating, and deleting hashmaps.

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <derive-c/derives/std.h>
#include <derive-c/macros/iterators.h>
#include <derive-c/structures/hashmap/hashers.h>

#define K uint32_t
#define V char const*
#define EQ uint32_t_eq
#define HASH hash_id_uint32_t
#define SELF id_to_name
#include <derive-c/structures/hashmap/template.h>

void print_map(id_to_name const* map) {
    printf("Map has items:\n");
    id_to_name_iter_const iter = id_to_name_get_iter_const(map);
    ITER_ENUMERATE_LOOP(id_to_name_iter_const, iter, id_to_name_kv_const, entry, size_t, pos) {
        printf("position: %zu key: %" PRIu32 " string: %s\n", pos, *entry.key, *entry.value);
    }
}

void id_to_name_example() {
    printf("Id to Name Map Example:\n");
    id_to_name map = id_to_name_new();

    id_to_name_insert(&map, 23, "hello");
    id_to_name_insert(&map, 10, "bob");
    id_to_name_insert(&map, 42, "meaning");
    ASSERT(strcmp(*id_to_name_read(&map, 42), "meaning") == 0);

    print_map(&map);

    char const** entry;
    ASSERT((entry = id_to_name_write(&map, 23)));
    *entry = "a different string!";

    print_map(&map);

    id_to_name_delete(&map);
}

struct report_id {
    char* name;
    uint32_t section;
};

bool report_id_equality(struct report_id const* report_1, struct report_id const* report_2) {
    return strcmp(report_1->name, report_2->name) == 0 && report_1->section == report_2->section;
}

size_t report_id_hash(struct report_id const* report_id) {
    return hash_combine(hash_murmurhash_string(report_id->name),
                        hash_id_uint32_t(&report_id->section));
}

void report_id_delete(struct report_id* self) { free(self->name); }

struct report {
    char* description;
    int value;
};

void report_delete(struct report* self) { free(self->description); }

#define K struct report_id
#define V struct report
#define EQ report_id_equality
#define HASH report_id_hash
#define K_DELETE report_id_delete
#define V_DELETE report_delete
#define SELF report_map
#include <derive-c/structures/hashmap/template.h>

void report_map_example() {
    printf("Report Map Example:\n");
    report_map map = report_map_new();

    struct report_id id1 = {.name = strdup("Report A"), .section = 1};
    struct report_id id2 = {.name = strdup("Report B"), .section = 2};

    report_map_insert(&map, id1,
                      (struct report){.description = strdup("Description A"), .value = 100});
    report_map_insert(&map, id2,
                      (struct report){.description = strdup("Description B"), .value = 200});

    assert(strcmp(report_map_read(&map, id1)->description, "Description A") == 0);

    report_map_iter_const iter = report_map_get_iter_const(&map);
    ITER_ENUMERATE_LOOP(report_map_iter_const, iter, report_map_kv_const, entry, size_t, pos) {
        printf("Position: %zu Key: %s Section: %u Value: %d\n", pos, entry.key->name,
               entry.key->section, entry.value->value);
    }

    struct report entry = report_map_remove(&map, id1);
    report_delete(&entry);

    report_map_delete(&map);
}

struct fixed_string {
    char value[4];
};

bool fixed_string_eq(struct fixed_string const* str1, struct fixed_string const* str2) {
    return memcmp(str1->value, str2->value, sizeof(str1->value)) == 0;
}

size_t fixed_string_hash(struct fixed_string const* str) {
    return hash_murmurhash_string_4(str->value);
}

#define K struct fixed_string
#define V uint32_t
#define EQ fixed_string_eq
#define HASH fixed_string_hash
#define SELF fixed_string_map
#include <derive-c/structures/hashmap/template.h>

void fixed_string_example() {
    printf("Fixed Strings Example:\n");
    fixed_string_map map = fixed_string_map_new();

    struct fixed_string key1 = {.value = "abc"};
    struct fixed_string key2 = {.value = "def"};
    struct fixed_string key3 = {.value = "ghi"};

    fixed_string_map_insert(&map, key1, 123);
    fixed_string_map_insert(&map, key2, 456);
    fixed_string_map_insert(&map, key3, 789);

    assert(*fixed_string_map_read(&map, key1) == 123);
    assert(*fixed_string_map_read(&map, key2) == 456);
    assert(*fixed_string_map_read(&map, key3) == 789);

    fixed_string_map_iter_const iter = fixed_string_map_get_iter_const(&map);
    ITER_ENUMERATE_LOOP(fixed_string_map_iter_const, iter, fixed_string_map_kv_const, entry, size_t,
                        pos) {
        printf("Position: %zu Key: %.3s Value: %u\n", pos, entry.key->value, *entry.value);
    }

    fixed_string_map_delete(&map);
}

int main() {
    id_to_name_example();
    report_map_example();
    fixed_string_example();
}
