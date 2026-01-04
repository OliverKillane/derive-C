/// @brief A statically allocated map that linearly looks up keys

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/self/def.h>

#if !defined CAPACITY
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("No CAPACITY")
    #endif
    #define CAPACITY 32
#endif

DC_STATIC_ASSERT(CAPACITY > 0, EXPAND_STRING(SELF) " CAPACITY cannot be empty");

#if !defined KEY
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("No KEY")
    #endif
    #define KEY map_key_t
typedef int KEY;
#endif

#if !defined KEY_EQ
    #define KEY_EQ DC_MEM_EQ
#endif

#if !defined KEY_DELETE
    #define KEY_DELETE DC_NO_DELETE
#endif

#if !defined KEY_CLONE
    #define KEY_CLONE DC_COPY_CLONE
#endif

#if !defined KEY_DEBUG
    #define KEY_DEBUG DC_DEFAULT_DEBUG
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
    #define VALUE_DELETE DC_NO_DELETE
#endif

#if !defined VALUE_CLONE
    #define VALUE_CLONE DC_COPY_CLONE
#endif

#if !defined VALUE_DEBUG
    #define VALUE_DEBUG DC_DEFAULT_DEBUG
#endif

typedef KEY NS(SELF, key_t);
typedef VALUE NS(SELF, value_t);

#define BITSET NS(NAME, bitset)

#define EXCLUSIVE_END_INDEX CAPACITY // [DERIVE-C] for template
#define INTERNAL_NAME BITSET         // [DERIVE-C] for template
#include <derive-c/container/bitset/static/template.h>

typedef struct {
    BITSET presence;
    KEY keys[CAPACITY];
    VALUE values[CAPACITY];

    dc_gdb_marker derive_c_map_staticlinear;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

DC_STATIC_CONSTANT size_t NS(SELF, max_capacity) = (size_t)CAPACITY;

#define INVARIANT_CHECK(self) DC_ASSUME(self);

static SELF NS(SELF, new)() {
    return (SELF){
        .presence = NS(BITSET, new)(),
        .keys = {},
        .values = {},
        .derive_c_map_staticlinear = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static VALUE const* NS(SELF, try_read)(SELF const* self, KEY key) {
    INVARIANT_CHECK(self);
    for (size_t index = 0; index < CAPACITY; index++) {
        if (NS(BITSET, get)(&self->presence, index) && KEY_EQ(&self->keys[index], &key)) {
            return &self->values[index];
        }
    }
    return NULL;
}

static VALUE const* NS(SELF, read)(SELF const* self, KEY key) {
    VALUE const* value = NS(SELF, try_read)(self, key);
    DC_ASSERT(value);
    return value;
}

static VALUE* NS(SELF, try_write)(SELF* self, KEY key) {
    return (VALUE*)(NS(SELF, try_read)(self, key));
}

static VALUE* NS(SELF, write)(SELF* self, KEY key) {
    VALUE* value = NS(SELF, try_write)(self, key);
    DC_ASSERT(value);
    return value;
}

static VALUE* NS(SELF, try_insert)(SELF* self, KEY key, VALUE value) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (NS(BITSET, size)(&self->presence) >= CAPACITY) {
        return NULL;
    }

    VALUE const* existing_value = NS(SELF, try_read)(self, key);
    if (existing_value) {
        return NULL;
    }

    for (size_t index = 0; index < CAPACITY; index++) {
        if (!NS(BITSET, get)(&self->presence, index)) {
            NS(BITSET, set)(&self->presence, index, true);
            self->keys[index] = key;
            self->values[index] = value;
            return &self->values[index];
        }
    }

    DC_UNREACHABLE("A space must exist for insert");
}

static VALUE* NS(SELF, insert)(SELF* self, KEY key, VALUE value) {
    VALUE* placed = NS(SELF, try_insert)(self, key, value);
    DC_ASSERT(placed);
    return placed;
}

static bool NS(SELF, try_remove)(SELF* self, KEY key, VALUE* dest) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    for (size_t index = 0; index < CAPACITY; index++) {
        if (NS(BITSET, get)(&self->presence, index) && KEY_EQ(&self->keys[index], &key)) {
            NS(BITSET, set)(&self->presence, index, false);
            KEY_DELETE(&self->keys[index]);
            *dest = self->values[index];
            return true;
        }
    }
    return false;
}

static VALUE NS(SELF, remove)(SELF* self, KEY key) {
    VALUE dest;
    DC_ASSERT(NS(SELF, try_remove)(self, key, &dest));
    return dest;
}

static void NS(SELF, delete_entry)(SELF* self, KEY key) {
    VALUE val = NS(SELF, remove)(self, key);
    VALUE_DELETE(&val);
}

static size_t NS(SELF, size)(SELF const* self) { return NS(BITSET, size)(&self->presence); }

static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);

    SELF new_self = (SELF){
        .presence = NS(BITSET, clone)(&self->presence),
        .derive_c_map_staticlinear = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
    for (size_t index = 0; index < CAPACITY; index++) {
        if (NS(BITSET, get)(&self->presence, index)) {
            new_self.keys[index] = KEY_CLONE(&self->keys[index]);
            new_self.values[index] = VALUE_CLONE(&self->values[index]);
        }
    }
    return new_self;
}

static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);

    for (size_t index = 0; index < CAPACITY; index++) {
        if (NS(BITSET, get)(&self->presence, index)) {
            KEY_DELETE(&self->keys[index]);
            VALUE_DELETE(&self->values[index]);
        }
    }
    NS(BITSET, delete)(&self->presence);
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "capacity: %zu,\n", (size_t)CAPACITY);
    dc_debug_fmt_print(fmt, stream, "entries: [\n");
    fmt = dc_debug_fmt_scope_begin(fmt);
    for (size_t index = 0; index < CAPACITY; index++) {
        if (NS(BITSET, get)(&self->presence, index)) {
            dc_debug_fmt_print(fmt, stream, "{index: %lu, key: ", index);
            KEY_DEBUG(&self->keys[index], fmt, stream);
            fprintf(stream, ", value: ");
            VALUE_DEBUG(&self->values[index], fmt, stream);
            fprintf(stream, "},\n");
        }
    }
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "],\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#define ITER_CONST NS(SELF, iter_const)
#define KV_PAIR_CONST NS(ITER_CONST, item)

typedef struct {
    SELF const* map;
    NS(BITSET, NS(iter_const, item)) next_index;
    mutation_version version;
} ITER_CONST;

typedef struct {
    KEY const* key;
    VALUE const* value;
} KV_PAIR_CONST;

static bool NS(ITER_CONST, empty_item)(KV_PAIR_CONST const* item) {
    return item->key == NULL && item->value == NULL;
}

static KV_PAIR_CONST NS(ITER_CONST, next)(ITER_CONST* iter) {
    mutation_version_check(&iter->version);
    size_t const next_index = iter->next_index;

    if (next_index == CAPACITY) {
        return (KV_PAIR_CONST){.key = NULL, .value = NULL};
    }

    iter->next_index++;
    while (iter->next_index < CAPACITY &&
           !NS(BITSET, get)(&iter->map->presence, iter->next_index)) {
        iter->next_index++;
    }

    if (next_index == CAPACITY) {
        return (KV_PAIR_CONST){.key = NULL, .value = NULL};
    }
    return (KV_PAIR_CONST){
        .key = &iter->map->keys[next_index],
        .value = &iter->map->values[next_index],
    };
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    INVARIANT_CHECK(self);

    NS(BITSET, NS(iter_const, item)) next_index = 0;
    while (next_index < CAPACITY &&
           !NS(BITSET, get)(&self->presence, (NS(BITSET, index_t))next_index)) {
        next_index++;
    }

    return (ITER_CONST){
        .map = self,
        .next_index = next_index,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef KV_PAIR_CONST
#undef ITER_CONST

#define ITER NS(SELF, iter)
#define KV_PAIR NS(ITER, item)

typedef struct {
    SELF* map;
    NS(BITSET, NS(iter, item)) next_index;
    mutation_version version;
} ITER;

typedef struct {
    KEY const* key;
    VALUE const* value;
} KV_PAIR;

static bool NS(ITER, empty_item)(KV_PAIR const* item) {
    return item->key == NULL && item->value == NULL;
}

static KV_PAIR NS(ITER, next)(ITER* iter) {
    mutation_version_check(&iter->version);
    NS(BITSET, NS(iter, item)) const next_index = iter->next_index;

    if (next_index == CAPACITY) {
        return (KV_PAIR){.key = NULL, .value = NULL};
    }

    iter->next_index++;
    while (iter->next_index < CAPACITY &&
           !NS(BITSET, get)(&iter->map->presence, (NS(BITSET, index_t))iter->next_index)) {
        iter->next_index++;
    }

    if (next_index == CAPACITY) {
        return (KV_PAIR){.key = NULL, .value = NULL};
    }
    return (KV_PAIR){
        .key = &iter->map->keys[next_index],
        .value = &iter->map->values[next_index],
    };
}

static ITER NS(SELF, get_iter)(SELF* self) {
    INVARIANT_CHECK(self);

    NS(BITSET, NS(iter, item)) next_index = 0;
    while (next_index < CAPACITY &&
           !NS(BITSET, get)(&self->presence, (NS(BITSET, index_t))next_index)) {
        next_index++;
    }

    return (ITER){
        .map = self,
        .next_index = next_index,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef KV_PAIR
#undef ITER

#undef INVARIANT_CHECK
#undef BITSET

#undef VALUE_DEBUG
#undef VALUE_CLONE
#undef VALUE_DELETE
#undef VALUE

#undef KEY_DEBUG
#undef KEY_CLONE
#undef KEY_DELETE
#undef KEY_EQ
#undef KEY

#undef CAPACITY

DC_TRAIT_MAP(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/includes/undef.h>
