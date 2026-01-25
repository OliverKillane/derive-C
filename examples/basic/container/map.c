/// @file
/// @example container/map.c
/// @brief Examples for using map containers.

#include <derive-c/alloc/std.h>
#include <derive-c/algorithm/hash/default.h>
#include <derive-c/algorithm/hash/combine.h>
#include <derive-c/algorithm/hash/fnv1a.h>
#include <derive-c/prelude.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct user_id {
    char const* username;
    uint64_t uuid;
};

static bool user_id_eq(struct user_id const* a, struct user_id const* b) {
    return strcmp(a->username, b->username) == 0 && a->uuid == b->uuid;
}

static size_t user_id_hash(struct user_id const* self) {
    const char* username_ptr = self->username;
    size_t hash = dc_fnv1a_str_const(&username_ptr);
    hash = dc_hash_combine(hash, DC_DEFAULT_HASH(&self->uuid));
    return hash;
}

static void user_id_debug(struct user_id const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, "user_id@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "username: %s,\n", self->username);
    dc_debug_fmt_print(fmt, stream, "uuid: %" PRIu64 ",\n", self->uuid);
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

struct userdata {
    int score;
    char hashed_password[16];
};

static void userdata_debug(struct userdata const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, "userdata@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "score: %d,\n", self->score);
    dc_debug_fmt_print(fmt, stream, "hashed_password: \"%.16s\",\n", self->hashed_password);
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#define KEY struct user_id
#define KEY_EQ user_id_eq
#define KEY_HASH user_id_hash
#define KEY_DEBUG user_id_debug
#define VALUE struct userdata
#define VALUE_DEBUG userdata_debug
#define NAME user_map
#include <derive-c/container/map/decomposed/template.h>

static void example_basic() {
    DC_DEBUG_TRACE;
    DC_SCOPED(user_map) map = user_map_new(stdalloc_get_ref());

    struct user_id user1 = {.username = "alice", .uuid = 1001};
    struct user_id user2 = {.username = "bob", .uuid = 1002};

    struct userdata data1 = {.score = 1500, .hashed_password = "hash1234567890"};
    struct userdata data2 = {.score = 2000, .hashed_password = "hash0987654321"};

    user_map_insert(&map, user1, data1);
    user_map_insert(&map, user2, data2);

    struct userdata const* found_data1 = user_map_read(&map, user1);
    DC_ASSERT(found_data1 != NULL);
    DC_ASSERT(found_data1->score == 1500);
    DC_ASSERT(strncmp(found_data1->hashed_password, "hash1234567890", 16) == 0);

    struct userdata const* found_data2 = user_map_read(&map, user2);
    DC_ASSERT(found_data2 != NULL);
    DC_ASSERT(found_data2->score == 2000);

    struct user_id invalid_user = {.username = "charlie", .uuid = 9999};
    struct userdata const* not_found = user_map_try_read(&map, invalid_user);
    DC_ASSERT(not_found == NULL);

    user_map_debug(&map, dc_debug_fmt_new(), stdout);
    fprintf(stdout, "\n");
}

struct largedata {
    char data[32];
};

static void largedata_debug(struct largedata const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, "largedata@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "data: \"%.32s\",\n", self->data);
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#define KEY uint64_t
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE uint16_t
#define NAME uuid_to_index_map
#include <derive-c/container/map/decomposed/template.h>

static void example_small() {
    DC_DEBUG_TRACE;
    DC_SCOPED(uuid_to_index_map) map = uuid_to_index_map_new(stdalloc_get_ref());

    uuid_to_index_map_insert(&map, 1001, 0);
    uuid_to_index_map_insert(&map, 1002, 1);
    uuid_to_index_map_insert(&map, 1003, 2);

    uuid_to_index_map_debug(&map, dc_debug_fmt_new(), stdout);
    fprintf(stdout, "\n");

    struct largedata large_data_array[3] = {
        {.data = "First large data"}, {.data = "Second large data"}, {.data = "Third large data"}};

    DC_FOR_CONST(uuid_to_index_map, &map, iter, entry) {
        uint16_t index = *entry.value;
        DC_ASSERT(index < 3);
        largedata_debug(&large_data_array[index], dc_debug_fmt_new(), stdout);
        fprintf(stdout, "\n");
    }
}

#define KEY uint64_t
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE struct largedata
#define VALUE_DEBUG largedata_debug
#define NAME ankerl_largedata_map
#include <derive-c/container/map/ankerl/template.h>

static void example_iteration() {
    DC_DEBUG_TRACE;
    DC_SCOPED(ankerl_largedata_map) map = ankerl_largedata_map_new(stdalloc_get_ref());

    struct largedata data1 = {.data = "Ankerl map entry number one"};
    struct largedata data2 = {.data = "Ankerl map entry number two"};
    struct largedata data3 = {.data = "Ankerl map entry number three"};

    ankerl_largedata_map_insert(&map, 2001, data1);
    ankerl_largedata_map_insert(&map, 2002, data2);
    ankerl_largedata_map_insert(&map, 2003, data3);

    size_t count = 0;
    DC_FOR_CONST(ankerl_largedata_map, &map, iter, entry) {
        printf("Entry %zu: uuid=%" PRIu64 " data=", count, *entry.key);
        largedata_debug(entry.value, dc_debug_fmt_new(), stdout);
        fprintf(stdout, "\n");
        count++;
    }
    DC_ASSERT(count == 3);

    ankerl_largedata_map_debug(&map, dc_debug_fmt_new(), stdout);
    fprintf(stdout, "\n");
}

int main() {
    example_basic();
    example_small();
    example_iteration();
    return 0;
}
