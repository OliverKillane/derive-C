/// @brief A simple open-addressed hashmap using robin-hood hashing.

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined KEY
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("No KEY")
    #endif
    #define KEY map_key_t
typedef int KEY;
#endif

#if !defined KEY_HASH
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("No KEY_HASH")
    #endif

    #define KEY_HASH key_hash
static size_t KEY_HASH(KEY const* key);
#endif

#if !defined KEY_EQ
    #define KEY_EQ MEM_EQ
#endif

#if !defined KEY_DELETE
    #define KEY_DELETE NO_DELETE
#endif

#if !defined KEY_CLONE
    #define KEY_CLONE COPY_CLONE
#endif

#if !defined KEY_DEBUG
    #define KEY_DEBUG DEFAULT_DEBUG
#endif

#if !defined VALUE
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("No VALUE")
    #endif
typedef struct {
    int x;
} value_t;
    #define VALUE value_t
#endif

#if !defined VALUE_DELETE
    #define VALUE_DELETE NO_DELETE
#endif

#if !defined VALUE_CLONE
    #define VALUE_CLONE COPY_CLONE
#endif

#if !defined VALUE_DEBUG
    #define VALUE_DEBUG DEFAULT_DEBUG
#endif

typedef KEY NS(SELF, key_t);
typedef VALUE NS(SELF, value_t);
typedef ALLOC NS(SELF, alloc_t);

#define KEY_ENTRY NS(SELF, key_entry)
typedef struct {
    bool present;
    uint16_t distance_from_desired;
    KEY key;
} KEY_ENTRY;

typedef struct {
    size_t capacity; // INVARIANT: A power of 2
    size_t items;
    KEY_ENTRY* keys;
    VALUE* values;
    ALLOC* alloc;
    gdb_marker derive_c_hashmap;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

#define INVARIANT_CHECK(self)                                                                      \
    ASSUME(self);                                                                                  \
    ASSUME(is_power_of_2((self)->capacity));                                                       \
    ASSUME((self)->keys);                                                                          \
    ASSUME((self)->values);                                                                        \
    ASSUME((self)->alloc);

static SELF NS(SELF, new_with_capacity_for)(size_t capacity, ALLOC* alloc) {
    ASSERT(capacity > 0);
    size_t const real_capacity = apply_capacity_policy(capacity);
    ASSERT(real_capacity > 0);
    // JUSTIFY: calloc of keys
    //  - A cheap way to get all precense flags as zeroed (os & allocater supported get zeroed page)
    //  - for the values, we do not need this (no precense checks are done on values)
    KEY_ENTRY* keys = (KEY_ENTRY*)NS(ALLOC, calloc)(alloc, sizeof(KEY_ENTRY), real_capacity);
    VALUE* values = (VALUE*)NS(ALLOC, malloc)(alloc, sizeof(VALUE) * real_capacity);
    ASSERT(keys && values);

    // JUSTIFY: no access for values & but keys are fine
    // - Keys are calloced/zeroed as we use this for item lookup, therefore it is valid to read
    // them.
    // - Values are only accessed when the corresponding key is present, so we can mark them as
    // deleted.
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE, values,
                       sizeof(VALUE) * real_capacity);

    return (SELF){
        .capacity = real_capacity,
        .items = 0,
        .keys = keys,
        .values = values,
        .alloc = alloc,
        .derive_c_hashmap = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static SELF NS(SELF, new)(ALLOC* alloc) {
    return NS(SELF, new_with_capacity_for)(INITIAL_CAPACITY, alloc);
}

static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);

    // JUSTIFY: Naive copy
    //           - We could resize (potentially a smaller map) and rehash
    //           - Not confident it would be any better than just a copy.
    // JUSTIFY: Individually copy keys
    //           - Many entries are zeroed, no need to copy uninit data

    KEY_ENTRY* keys = (KEY_ENTRY*)NS(ALLOC, calloc)(self->alloc, sizeof(KEY_ENTRY), self->capacity);
    VALUE* values = (VALUE*)NS(ALLOC, malloc)(self->alloc, sizeof(VALUE) * self->capacity);
    ASSERT(keys && values);

    for (size_t i = 0; i < self->capacity; i++) {
        if (self->keys[i].present) {
            KEY_ENTRY const* old_entry = &self->keys[i];
            keys[i] = (KEY_ENTRY){
                .present = true,
                .distance_from_desired = old_entry->distance_from_desired,
                .key = KEY_CLONE(&old_entry->key),
            };
            values[i] = VALUE_CLONE(&self->values[i]);
        } else {
            memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE, &values[i],
                               sizeof(VALUE));
        }
    }

    return (SELF){
        .capacity = self->capacity,
        .items = self->items,
        .keys = keys,
        .values = values,
        .alloc = self->alloc,
        .derive_c_hashmap = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static VALUE* PRIV(NS(SELF, try_insert_no_extend_capacity))(SELF* self, KEY key, VALUE value) {
    uint16_t distance_from_desired = 0;
    size_t const hash = KEY_HASH(&key);
    size_t index = modulus_power_of_2_capacity(hash, self->capacity);

    VALUE* inserted_to_entry = NULL;

    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];
        ASSUME(distance_from_desired < self->capacity);

        if (entry->present) {
            if (KEY_EQ(&entry->key, &key)) {
                return NULL;
            }

            if (entry->distance_from_desired < distance_from_desired) {
                KEY const switch_key = entry->key;
                uint16_t const switch_distance_from_desired = entry->distance_from_desired;
                VALUE const switch_value = self->values[index];

                entry->key = key;
                entry->distance_from_desired = distance_from_desired;
                self->values[index] = value;

                key = switch_key;
                distance_from_desired = switch_distance_from_desired;
                value = switch_value;

                if (!inserted_to_entry) {
                    inserted_to_entry = &self->values[index];
                }
            }

            distance_from_desired++;
            index = modulus_power_of_2_capacity(index + 1, self->capacity);
        } else {
            entry->present = true;
            entry->distance_from_desired = distance_from_desired;
            entry->key = key;

            memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_WRITE,
                               &self->values[index], sizeof(VALUE));

            self->values[index] = value;

            if (!inserted_to_entry) {
                inserted_to_entry = &self->values[index];
            }

            self->items++;
            return inserted_to_entry;
        }
    }
}

static void NS(SELF, extend_capacity_for)(SELF* self, size_t expected_items) {
    INVARIANT_CHECK(self);

    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    size_t const target_capacity = apply_capacity_policy(expected_items);
    if (target_capacity > self->capacity) {
        SELF new_map = NS(SELF, new_with_capacity_for)(expected_items, self->alloc);
        for (size_t index = 0; index < self->capacity; index++) {
            KEY_ENTRY* entry = &self->keys[index];
            if (entry->present) {
                PRIV(NS(SELF, try_insert_no_extend_capacity))
                (&new_map, entry->key, self->values[index]);
            }
        }
        NS(ALLOC, free)(self->alloc, (void*)self->keys);
        NS(ALLOC, free)(self->alloc, (void*)self->values);

        INVARIANT_CHECK(&new_map);
        *self = new_map;
    }
}

static VALUE* NS(SELF, try_insert)(SELF* self, KEY key, VALUE value) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    if (apply_capacity_policy(self->items) > self->capacity / 2) {
        NS(SELF, extend_capacity_for)(self, self->items * 2);
    }

    return PRIV(NS(SELF, try_insert_no_extend_capacity))(self, key, value);
}

static VALUE* NS(SELF, insert)(SELF* self, KEY key, VALUE value) {
    VALUE* value_ptr = NS(SELF, try_insert)(self, key, value);
    ASSERT(value_ptr);
    return value_ptr;
}

static VALUE* NS(SELF, try_write)(SELF* self, KEY key) {
    INVARIANT_CHECK(self);
    size_t const hash = KEY_HASH(&key);
    size_t index = modulus_power_of_2_capacity(hash, self->capacity);

    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];
        if (entry->present) {
            if (KEY_EQ(&entry->key, &key)) {
                return &self->values[index];
            }
            index = modulus_power_of_2_capacity(index + 1, self->capacity);
        } else {
            return NULL;
        }
    }
}

static VALUE* NS(SELF, write)(SELF* self, KEY key) {
    VALUE* value = NS(SELF, try_write)(self, key);
    ASSERT(value);
    return value;
}

static VALUE const* NS(SELF, try_read)(SELF const* self, KEY key) {
    INVARIANT_CHECK(self);
    size_t const hash = KEY_HASH(&key);
    size_t index = modulus_power_of_2_capacity(hash, self->capacity);

    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];
        if (entry->present) {
            if (KEY_EQ(&entry->key, &key)) {
                return &self->values[index];
            }
            index = modulus_power_of_2_capacity(index + 1, self->capacity);
        } else {
            return NULL;
        }
    }
}

static VALUE const* NS(SELF, read)(SELF const* self, KEY key) {
    VALUE const* value = NS(SELF, try_read)(self, key);
    ASSERT(value);
    return value;
}

static bool NS(SELF, try_remove)(SELF* self, KEY key, VALUE* destination) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    size_t const hash = KEY_HASH(&key);
    size_t index = modulus_power_of_2_capacity(hash, self->capacity);

    for (;;) {
        KEY_ENTRY* entry = &self->keys[index];
        if (entry->present) {
            if (KEY_EQ(&entry->key, &key)) {
                self->items--;

                *destination = self->values[index];
                KEY_DELETE(&entry->key);

                // NOTE: For robin hood hashing, we need probe chains to be unbroken
                //        - Need to find the entries that might use this chain (
                //          distance_to_desired > 0 until the next not-present or
                //          distance_to_desired=0 slot)
                //        hence we shift entries the left (being careful with modulus index)

                size_t free_index = index;
                KEY_ENTRY* free_entry = entry;

                size_t check_index = modulus_power_of_2_capacity(free_index + 1, self->capacity);
                KEY_ENTRY* check_entry = &self->keys[check_index];

                while (check_entry->present && (check_entry->distance_from_desired > 0)) {
                    free_entry->key = check_entry->key;
                    free_entry->distance_from_desired = check_entry->distance_from_desired - 1;
                    self->values[free_index] = self->values[check_index];

                    free_index = check_index;
                    free_entry = check_entry;

                    check_index = modulus_power_of_2_capacity(free_index + 1, self->capacity);
                    check_entry = &self->keys[check_index];
                }

                // JUSTIFY: Only setting free entry to false
                //           - We remove, then shift down an index
                //           - The removed entry already has the flag set
                //           - the free entry was the last one removed/moved down an index, so it
                //             should be false.

                free_entry->present = false;
                memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE,
                                   &self->values[free_index], sizeof(VALUE));

                return true;
            }
            index = modulus_power_of_2_capacity(index + 1, self->capacity);
        } else {
            return false;
        }
    }
}

static VALUE NS(SELF, remove)(SELF* self, KEY key) {
    VALUE value;
    ASSERT(NS(SELF, try_remove)(self, key, &value));
    return value;
}

static void NS(SELF, delete_entry)(SELF* self, KEY key) {
    VALUE value = NS(SELF, remove)(self, key);
    VALUE_DELETE(&value);
}

static size_t NS(SELF, size)(SELF const* self) {
    INVARIANT_CHECK(self);
    return self->items;
}

#define KV_PAIR NS(SELF, kv)

typedef struct {
    KEY const* key;
    VALUE* value;
} KV_PAIR;

#define ITER NS(SELF, iter)
typedef KV_PAIR NS(ITER, item);

static bool NS(ITER, empty_item)(KV_PAIR const* item) {
    return item->key == NULL && item->value == NULL;
}

typedef struct {
    SELF* map;
    size_t index;
    KV_PAIR curr;
    mutation_version version;
} ITER;

static KV_PAIR NS(ITER, next)(ITER* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    if (iter->index < iter->map->capacity) {
        iter->curr = (KV_PAIR){.key = &iter->map->keys[iter->index].key,
                               .value = &iter->map->values[iter->index]};
        iter->index++;
        while (iter->index < iter->map->capacity && !iter->map->keys[iter->index].present) {
            iter->index++;
        }
        return iter->curr;
    }
    return (KV_PAIR){.key = NULL, .value = NULL};
}

static bool NS(ITER, empty)(ITER const* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->index >= iter->map->capacity;
}

static ITER NS(SELF, get_iter)(SELF* self) {
    ASSUME(self);
    size_t first_index = 0;
    while (first_index < self->capacity && !self->keys[first_index].present) {
        first_index++;
    }
    return (ITER){
        .map = self,
        .index = first_index,
        .curr = (KV_PAIR){.key = NULL, .value = NULL},
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

static void NS(SELF, delete)(SELF* self) {
    ASSUME(self);

    for (size_t i = 0; i < self->capacity; i++) {
        if (self->keys[i].present) {
            KEY_DELETE(&self->keys[i].key);
            VALUE_DELETE(&self->values[i]);
        }
    }

    NS(ALLOC, free)(self->alloc, (void*)self->keys);
    NS(ALLOC, free)(self->alloc, (void*)self->values);
}

#undef ITER
#undef KV_PAIR

#define KV_PAIR_CONST NS(SELF, kv_const)

typedef struct {
    KEY const* key;
    VALUE const* value;
} KV_PAIR_CONST;

#define ITER_CONST NS(SELF, iter_const)
typedef KV_PAIR_CONST NS(ITER_CONST, item);

static bool NS(ITER_CONST, empty_item)(KV_PAIR_CONST const* item) {
    return item->key == NULL && item->value == NULL;
}

typedef struct {
    SELF const* map;
    size_t index;
    KV_PAIR_CONST curr;
    mutation_version version;
} ITER_CONST;

static KV_PAIR_CONST NS(ITER_CONST, next)(ITER_CONST* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    if (iter->index < iter->map->capacity) {
        iter->curr = (KV_PAIR_CONST){.key = &iter->map->keys[iter->index].key,
                                     .value = &iter->map->values[iter->index]};
        iter->index++;
        while (iter->index < iter->map->capacity && !iter->map->keys[iter->index].present) {
            iter->index++;
        }
        return iter->curr;
    }
    return (KV_PAIR_CONST){.key = NULL, .value = NULL};
}

static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->index >= iter->map->capacity;
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    ASSUME(self);
    size_t first_index = 0;
    while (first_index < self->capacity && !self->keys[first_index].present) {
        first_index++;
    }
    return (ITER_CONST){
        .map = self,
        .index = first_index,
        .curr = (KV_PAIR_CONST){.key = NULL, .value = NULL},
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

static void NS(SELF, debug)(SELF const* self, debug_fmt fmt, FILE* stream) {
    fprintf(stream, EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = debug_fmt_scope_begin(fmt);

    debug_fmt_print(fmt, stream, "capacity: %lu,\n", self->capacity);
    debug_fmt_print(fmt, stream, "size: %lu,\n", NS(SELF, size)(self));

    debug_fmt_print(fmt, stream, "keys: @%p,\n", self->keys);
    debug_fmt_print(fmt, stream, "values: @%p,\n", self->values);

    debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(self->alloc, fmt, stream);
    fprintf(stream, ",\n");

    debug_fmt_print(fmt, stream, "items: [");
    fmt = debug_fmt_scope_begin(fmt);

    ITER_CONST iter = NS(SELF, get_iter_const)(self);
    KV_PAIR_CONST item;

    for (KV_PAIR_CONST item = NS(ITER_CONST, next)(&iter); !NS(ITER_CONST, empty_item)(&item);
         item = NS(ITER_CONST, next)(&iter)) {
        debug_fmt_print(fmt, stream, "{\n");
        fmt = debug_fmt_scope_begin(fmt);

        debug_fmt_print(fmt, stream, "key: ");
        KEY_DEBUG(item.key, fmt, stream);
        fprintf(stream, ",\n");

        debug_fmt_print(fmt, stream, "value: ");
        VALUE_DEBUG(item.value, fmt, stream);
        fprintf(stream, ",\n");

        fmt = debug_fmt_scope_end(fmt);
        debug_fmt_print(fmt, stream, "},\n");
    }

    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "],\n");
    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "}");
}

#undef ITER_CONST
#undef KV_PAIR_CONST

#undef INVARIANT_CHECK
#undef KEY_ENTRY

#undef VALUE_DEBUG
#undef VALUE_CLONE
#undef VALUE_DELETE
#undef VALUE

#undef KEY_DEBUG
#undef KEY_CLONE
#undef KEY_DELETE
#undef KEY_EQ
#undef KEY_HASH
#undef KEY

TRAIT_MAP(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>