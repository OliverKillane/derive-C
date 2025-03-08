#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define PANIC abort()

bool int_equality(uint32_t const* key_1, uint32_t const* key_2) { return key_1 == key_2; }

size_t int_hash(uint32_t const* key) { return *key; }

typedef struct {
    char const* str; 
} string;

#define K uint32_t
#define V string
#define EQ int_equality
#define HASH int_hash
#define SELF id_to_name
#include <derive-c/structures/hashmap.template.h>

int main() {
    id_to_name map = id_to_name_new();

    id_to_name_insert(&map, 23, (string){.str = "hello"});
    id_to_name_insert(&map, 10, (string){.str = "bob"});
    id_to_name_insert(&map, 42, (string){.str = "meaning"});

    ASSERT(strcmp(id_to_name_read(&map, 42)->str, "meaning") == 0);

    string* entry;
    ASSERT((entry = id_to_name_write(&map, 23)));
    entry->str = "a different string!";
}

