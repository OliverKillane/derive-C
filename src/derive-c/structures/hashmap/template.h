/// @brief A simple open-addressed hashmap using robin-hood hashing.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <derive-c/core.h>
#include <derive-c/panic.h>
#include <derive-c/self.h>
#include <derive-c/structures/hashmap/utils.h>

#ifndef K
#ifndef __clang_analyzer__
#error "Key type must be defined to for a hashmap template"
#endif
typedef struct {
    int x;
} derive_c_parameter_key;
#define K derive_c_parameter_key
static void derive_c_parameter_key_delete(derive_c_parameter_key* key __attribute__((unused))) {}
#define K_DELETE derive_c_parameter_key_delete
#endif

#ifndef V
#ifndef __clang_analyzer__
#error "Value type must be defined to for a hashmap template"
#endif
typedef struct {
    int x;
} derive_c_parameter_value;
#define V derive_c_parameter_value
static void derive_c_parameter_value_delete(derive_c_parameter_value* key __attribute__((unused))) {
}
#define V_DELETE derive_c_parameter_value_delete
#endif

#ifndef HASH
#ifndef __clang_analyzer__
#error "The hash function for K must be defined"
#endif
static size_t derive_c_parameter_hash(derive_c_parameter_key const* key);
#define HASH derive_c_parameter_hash
#endif

#ifndef EQ
#ifndef __clang_analyzer__
#error "The equality function for K must be defined"
#endif
static bool derive_c_parameter_eq(derive_c_parameter_key const* key_1,
                                  derive_c_parameter_key const* key_2);
#define EQ derive_c_parameter_eq
#endif

#ifndef K_DELETE
#define K_DELETE(key)
#endif

#ifndef V_DELETE
#define V_DELETE(value)
#endif

#define KEY_ENTRY NAME(SELF, key_entry)
typedef struct {
    bool present;
    uint16_t distance_from_desired;
    K key;
} KEY_ENTRY;

typedef struct {
    size_t capacity; // INV: A power of 2
    size_t items;
    // Split keys & values in an old hashmap I used, cannot remember why (many collisions better
    // decomposed), should probably use 1 buffer.
    KEY_ENTRY* keys;
    V* values;
    gdb_marker derive_c_hashmap;
} SELF;

static SELF NAME(SELF, new_with_capacity_for)(size_t capacity) {
    ASSERT(capacity > 0);
    size_t real_capacity = apply_capacity_policy(capacity);
    ASSERT(real_capacity > 0);
    // JUSTIFY: calloc of keys
    //  - A cheap way to get all precense flags as zeroed (os & allocater supported get zeroed page)
    //  - for the values, we do not need this (no precense checks are done on values)
    KEY_ENTRY* keys = (KEY_ENTRY*)calloc(sizeof(KEY_ENTRY), real_capacity);
    V* values = (V*)malloc(sizeof(V) * real_capacity);
    ASSERT(keys && values);
    return (SELF){
        .capacity = real_capacity,
        .items = 0,
        .keys = keys,
        .values = values,
    };
}

static SELF NAME(SELF, new)() { return NAME(SELF, new_with_capacity_for)(INITIAL_CAPACITY); }

static SELF NAME(SELF, shallow_clone)(SELF const* self) {
    DEBUG_ASSERT(self);

    // JUSTIFY: Naive copy
    //           - We could resize (potentially a smaller map) and rehash
    //           - Not confident it would be any better than just a copy.
    // JUSTIFY: Individually copy keys
    //           - Many entries are zeroed, no need to copy uninit data

    KEY_ENTRY* keys = (KEY_ENTRY*)calloc(sizeof(KEY_ENTRY), self->capacity);
    V* values = (V*)malloc(sizeof(V) * self->capacity);
    ASSERT(keys && values);

    for (size_t i = 0; i < self->capacity; i++) {
        if (self->keys[i].present) {
            keys[i] = self->keys[i];
            values[i] = self->values[i];
        }
    }

    return (SELF){.capacity = self->capacity, .items = self->items, .keys = keys, .values = values};
}

static void NAME(SELF, delete)(SELF* self);

static V* NAME(SELF, insert)(SELF* self, K key, V value);

static void NAME(SELF, extend_capacity_for)(SELF* self, size_t expected_items) {
    DEBUG_ASSERT(self);
    size_t target_capacity = apply_capacity_policy(expected_items);
    if (target_capacity > self->capacity) {
        SELF new_map = NAME(SELF, new_with_capacity_for)(expected_items);
        for (size_t index = 0; index < self->capacity; index++) {
            KEY_ENTRY* entry = &self->keys[index];
            if (entry->present) {
                NAME(SELF, insert)(&new_map, entry->key, self->values[index]);
            }
        }
        free((void*)self->keys);
        free((void*)self->values);
        *self = new_map;
    }
}

static V* NAME(SELF, try_insert)(SELF* self, K key, V value) {
    DEBUG_ASSERT(self);
    if (apply_capacity_policy(self->items) > self->capacity / 2) {
        NAME(SELF, extend_capacity_for)(self, self->items * 2);
    }

    uint16_t distance_from_desired = 0;
    size_t hash = HASH(&key);
    size_t index = modulus_capacity(hash, self->capacity);
    V* placed_entry = NULL;
    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];
        DEBUG_ASSERT(distance_from_desired < self->capacity);

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
            index = modulus_capacity(index + 1, self->capacity);
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

static V* NAME(SELF, insert)(SELF* self, K key, V value) {
    V* value_ptr = NAME(SELF, try_insert)(self, key, value);
    ASSERT(value_ptr);
    return value_ptr;
}

static V* NAME(SELF, try_write)(SELF* self, K key) {
    DEBUG_ASSERT(self);
    size_t hash = HASH(&key);
    size_t index = modulus_capacity(hash, self->capacity);

    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];
        if (entry->present) {
            if (EQ(&entry->key, &key)) {
                return &self->values[index];
            }
            index = modulus_capacity(index + 1, self->capacity);
        } else {
            return NULL;
        }
    }
}

static V* NAME(SELF, write)(SELF* self, K key) {
    V* value = NAME(SELF, try_write)(self, key);
    ASSERT(value);
    return value;
}

static V const* NAME(SELF, try_read)(SELF const* self, K key) {
    DEBUG_ASSERT(self);
    size_t hash = HASH(&key);
    size_t index = modulus_capacity(hash, self->capacity);

    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];
        if (entry->present) {
            if (EQ(&entry->key, &key)) {
                return &self->values[index];
            }
            index = modulus_capacity(index + 1, self->capacity);
        } else {
            return NULL;
        }
    }
}

static V const* NAME(SELF, read)(SELF const* self, K key) {
    V const* value = NAME(SELF, try_read)(self, key);
    ASSERT(value);
    return value;
}

static bool NAME(SELF, try_remove)(SELF* self, K key, V* destination) {
    DEBUG_ASSERT(self);
    size_t hash = HASH(&key);
    size_t index = modulus_capacity(hash, self->capacity);

    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];
        if (entry->present) {
            if (EQ(&entry->key, &key)) {
                self->items--;

                *destination = self->values[index];
                K_DELETE(&entry->key);

                // NOTE: For robin hood hashing, we need probe chains to be unbroken
                //        - Need to find the entries that might use this chain (
                //          distance_to_desired > 0 until the next not-present or
                //          distance_to_desired=0 slot)
                //        hence we shift entries the left (being careful with modulus index)

                size_t free_index = index;
                KEY_ENTRY* free_entry = entry;

                size_t check_index = modulus_capacity(free_index + 1, self->capacity);
                KEY_ENTRY* check_entry = &self->keys[check_index];

                while (check_entry->present && (check_entry->distance_from_desired > 0)) {
                    free_entry->key = check_entry->key;
                    free_entry->distance_from_desired = check_entry->distance_from_desired - 1;
                    self->values[free_index] = self->values[check_index];

                    free_index = check_index;
                    free_entry = check_entry;

                    check_index = modulus_capacity(free_index + 1, self->capacity);
                    check_entry = &self->keys[check_index];
                }

                // JUSTIFY: Only setting free entry to false
                //           - We remove, then shift down an index
                //           - The removed entry already has the flag set
                //           - the free entry was the last one removed/moved down an index, so it
                //             should be false.

                free_entry->present = false;

                return true;
            }
            index = modulus_capacity(index + 1, self->capacity);
        } else {
            return false;
        }
    }
}

static V NAME(SELF, remove)(SELF* self, K key) {
    V value;
    ASSERT(NAME(SELF, try_remove)(self, key, &value));
    return value;
}

static void NAME(SELF, delete_entry)(SELF* self, K key) {
    V value = NAME(SELF, remove)(self, key);
    V_DELETE(&value);
}

static size_t NAME(SELF, size)(SELF const* self) {
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
    KV_PAIR curr;
} ITER;

static KV_PAIR const* NAME(ITER, next)(ITER* iter) {
    DEBUG_ASSERT(iter);
    if (iter->index < iter->map->capacity) {
        iter->curr = (KV_PAIR){.key = &iter->map->keys[iter->index].key,
                               .value = &iter->map->values[iter->index]};
        iter->pos++;
        iter->index++;
        while (iter->index < iter->map->capacity && !iter->map->keys[iter->index].present) {
            iter->index++;
        }
        return &iter->curr;
    }
    return NULL;
}

static size_t NAME(ITER, position)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

static bool NAME(ITER, empty)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->index >= iter->map->capacity;
}

static ITER NAME(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
    size_t first_index = 0;
    while (first_index < self->capacity && !self->keys[first_index].present) {
        first_index++;
    }
    return (ITER){
        .map = self,
        .index = first_index,
        .pos = 0,
        .curr = (KV_PAIR){.key = NULL, .value = NULL},
    };
}

static void NAME(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    ITER iter = NAME(SELF, get_iter)(self);

    for (size_t i = 0; i < self->capacity; i++) {
        if (self->keys[i].present) {
            K_DELETE(&self->keys[i].key);
            V_DELETE(&self->values[i]);
        }
    }

    free((void*)self->keys);
    free((void*)self->values);
}

#undef ITER
#undef KV_PAIR

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
    KV_PAIR_CONST curr;
} ITER_CONST;

static KV_PAIR_CONST const* NAME(ITER_CONST, next)(ITER_CONST* iter) {
    DEBUG_ASSERT(iter);
    if (iter->index < iter->map->capacity) {
        iter->curr = (KV_PAIR_CONST){.key = &iter->map->keys[iter->index].key,
                                     .value = &iter->map->values[iter->index]};
        iter->pos++;
        iter->index++;
        while (iter->index < iter->map->capacity && !iter->map->keys[iter->index].present) {
            iter->index++;
        }
        return &iter->curr;
    }
    return NULL;
}

static size_t NAME(ITER_CONST, position)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

static bool NAME(ITER_CONST, empty)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->index >= iter->map->capacity;
}

static ITER_CONST NAME(SELF, get_iter_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    size_t first_index = 0;
    while (first_index < self->capacity && !self->keys[first_index].present) {
        first_index++;
    }
    return (ITER_CONST){
        .map = self,
        .index = first_index,
        .pos = 0,
        .curr = (KV_PAIR_CONST){.key = NULL, .value = NULL},
    };
}

#undef ITER_CONST
#undef KV_PAIR_CONST

#undef KEY_ENTRY

#undef K
#undef V
#undef HASH
#undef EQ
#undef SELF
#undef V_DELETE
#undef K_DELETE
