#include <derive-c/core.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef PANIC
#error "PANIC must be defined (used for unrecoverable failures)"
#define PANIC abort() // Allows independent debugging
#endif

#ifndef K
#error "K (key type) must be defined to for a hashmap template"
typedef struct {
    int x;
} placeholder_key;
#define K placeholder_key // Allows independent debugging
#endif

#ifndef V
#error "V (value type) must be defined to for a hashmap template"
typedef struct {
    int x;
} placeholder_value;
#define V placeholder_value // Allows independent debugging
#endif

#ifndef HASH
#error "HASH (the hash function on K) must be defined"
inline size_t placeholder_hash(placeholder_key const* key);
#define HASH placeholder_hash
#endif

#ifndef EQ
#error "HASH (the equality function on K) must be defined"
inline bool placeholder_eq(placeholder_key const* key_1, placeholder_key const* key_2);
#define EQ placeholder_eq
#endif

#ifndef SELF
#ifndef MODULE
#error                                                                                             \
    "MODULE must be defined to use a template (it is prepended to the start of all methods, and the type)"
#endif
#define SELF NAME(MODULE, NAME(hashmap, NAME(K, NAME(V, NAME(HASH, EQ)))))
#endif

#ifndef HASHMAP_INTERNAL
#define HASHMAP_INTERNAL

size_t apply_overallocate_factor(size_t capacity) { return capacity * 3 / 2; }

static size_t const PROBE_DISTANCE = 4;
static size_t const INITIAL_CAPACITY = 32;
#endif

#define KEY_ENTRY NAME(SELF, key_entry)
typedef struct {
    bool present;
    uint16_t distance_from_desired;
    K key;
} KEY_ENTRY;

typedef struct {
    size_t capacity;
    size_t items;
    // Split keys & values in an old hashmap I used, cannot remember why (many collisions better
    // decomposed), should probably use 1 buffer.
    KEY_ENTRY* keys;
    V* values;
} SELF;

SELF NAME(SELF, new_with_capacity)(size_t capacity) {
    ASSERT(capacity > 0);
    size_t overallocated_capacity = apply_overallocate_factor(capacity);
    ASSERT(overallocated_capacity > 0);
    // JUSTIFY: calloc of keys
    //  - A cheap way to get all precense flags as zeroed (os & allocater supported get zeroed page)
    //  - for the values, we do not need this (no precense checks are done on values)
    KEY_ENTRY* keys = (KEY_ENTRY*)calloc(sizeof(KEY_ENTRY), overallocated_capacity);
    V* values = (V*)malloc(sizeof(V) * overallocated_capacity);
    if (!keys || !values)
        PANIC;
    return (SELF){
        .capacity = overallocated_capacity,
        .items = 0,
        .keys = keys,
        .values = values,
    };
}

SELF NAME(SELF, new)() { return NAME(SELF, new_with_capacity)(INITIAL_CAPACITY); }

void NAME(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    free(self->keys);
    free(self->values);
}

MAYBE_NULL(V) NAME(SELF, insert)(SELF* self, K key, V value);

void NAME(SELF, extend_capacity_for)(SELF* self, size_t expected_items) {
    DEBUG_ASSERT(self);
    size_t target_capacity = apply_overallocate_factor(expected_items);
    if (target_capacity > self->capacity) {
        SELF new_map = NAME(SELF, new_with_capacity)(expected_items);
        for (size_t index = 0; index < self->capacity; index++) {
            KEY_ENTRY* entry = &self->keys[index];
            if (entry->present) {
                if (!NAME(SELF, insert)(&new_map, entry->key, self->values[index]))
                    PANIC;
            }
        }
        NAME(SELF, delete)(self);
        *self = new_map;
    }
}

MAYBE_NULL(V) NAME(SELF, insert)(SELF* self, K key, V value) {
    DEBUG_ASSERT(self);
    if (apply_overallocate_factor(self->items) > self->capacity / 2) {
        NAME(SELF, extend_capacity_for)(self, self->items * 2);
    }

    uint16_t distance_from_desired = 0;
    size_t hash = HASH(&key);
    size_t index = hash & (self->capacity - 1);
    V* placed_entry = NULL;
    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];

        if (entry->present) {
            if (EQ(&entry->key, &key)) {
                return NULL;
            }

            if (entry->distance_from_desired < distance_from_desired) {
                K switch_key = entry->key;
                uint16_t switch_distance_from_desired = entry->distance_from_desired;
                V switch_value = self->values[index];

                entry->key = key;
                entry->distance_from_desired = distance_from_desired;
                self->values[index] = value;

                key = switch_key;
                distance_from_desired = switch_distance_from_desired;
                value = switch_value;

                if (!placed_entry) {
                    placed_entry = &self->values[index];
                }
            }

            distance_from_desired++;
            index = (index + PROBE_DISTANCE) & (self->capacity - 1);
        } else {
            entry->present = true;
            entry->distance_from_desired = distance_from_desired;
            entry->key = key;
            self->values[index] = value;
            if (!placed_entry) {
                placed_entry = &self->values[index];
            }
            self->items++;
            return placed_entry;
        }
    }
}

MAYBE_NULL(V) NAME(SELF, write)(SELF* self, K key) {
    DEBUG_ASSERT(self);
    size_t hash = HASH(&key);
    size_t index = hash & (self->capacity - 1);

    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];
        if (entry->present) {
            if (EQ(&entry->key, &key)) {
                return &self->values[index];
            } else {
                index = (index + PROBE_DISTANCE) & (self->capacity - 1);
            }
        } else {
            return NULL;
        }
    }
}

MAYBE_NULL(V const) NAME(SELF, read)(SELF const* self, K key) {
    DEBUG_ASSERT(self);
    size_t hash = HASH(&key);
    size_t index = hash & (self->capacity - 1);

    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];
        if (entry->present) {
            if (EQ(&entry->key, &key)) {
                return &self->values[index];
            } else {
                index = (index + PROBE_DISTANCE) & (self->capacity - 1);
            }
        } else {
            return NULL;
        }
    }
}

MAYBE_NULL(V) NAME(SELF, remove)(SELF* self, K key) {
    DEBUG_ASSERT(self);
    size_t hash = HASH(&key);
    size_t index = hash & (self->capacity - 1);

    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];
        if (entry->present) {
            if (EQ(&entry->key, &key)) {
                entry->present = false;
                return &self->values[index];
            } else {
                index = (index + PROBE_DISTANCE) & (self->capacity - 1);
            }
        } else {
            return NULL;
        }
    }
}

size_t NAME(SELF, size)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->items;
}

#define KV_PAIR NAME(SELF, kv)

typedef struct {
    K const* key;
    V* value;
} KV_PAIR;

#define ITER NAME(SELF, iter)

typedef struct {
    SELF* map;
    size_t index;
    size_t pos;
} ITER;

KV_PAIR NAME(ITER_CONST, next)(ITER* iter) {
    DEBUG_ASSERT(iter);
    if (iter->index < iter->map->capacity) {
        KV_PAIR ret_val = {.key = &iter->map->keys[iter->index].key,
                           .value = &iter->map->values[iter->index]};
        iter->pos++;
        iter->index++;
        while (iter->index < iter->map->capacity && !iter->map->keys[iter->index].present) {
            iter->index++;
        }

        return ret_val;
    } else {
        return (KV_PAIR){.key = NULL, .value = NULL};
    }
}

size_t NAME(ITER_CONST, position)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

bool NAME(ITER_CONST, empty)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->index >= iter->map->capacity;
}

ITER NAME(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
    size_t first_index = 0;
    while (first_index < self->capacity && !self->keys[first_index].present) {
        first_index++;
    }
    return (ITER){
        .map = self,
        .index = first_index,
        .pos = 0,
    };
}

#define KV_PAIR_CONST NAME(SELF, kv_const)

typedef struct {
    K const* key;
    V const* value;
} KV_PAIR_CONST;

#define ITER_CONST NAME(SELF, iter_const)

typedef struct {
    SELF const* map;
    size_t index;
    size_t pos;
} ITER_CONST;

KV_PAIR_CONST NAME(ITER_CONST, next)(ITER_CONST* iter) {
    DEBUG_ASSERT(iter);
    if (iter->index < iter->map->capacity) {
        KV_PAIR_CONST ret_val = {.key = &iter->map->keys[iter->index].key,
                                 .value = &iter->map->values[iter->index]};
        iter->pos++;
        iter->index++;
        while (iter->index < iter->map->capacity && !iter->map->keys[iter->index].present) {
            iter->index++;
        }

        return ret_val;
    } else {
        return (KV_PAIR_CONST){.key = NULL, .value = NULL};
    }
}

size_t NAME(ITER_CONST, position)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

bool NAME(ITER_CONST, empty)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->index >= iter->map->capacity;
}

ITER_CONST NAME(SELF, get_iter_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    size_t first_index = 0;
    while (first_index < self->capacity && !self->keys[first_index].present) {
        first_index++;
    }
    return (ITER_CONST){
        .map = self,
        .index = first_index,
        .pos = 0,
    };
}

#undef K
#undef V
#undef HASH
#undef EQ
#undef SELF
#undef KEY_ENTRY
#undef ITER
#undef ITER_CONST
