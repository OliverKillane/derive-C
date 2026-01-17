/// @brief A statically allocated map that linearly looks up keys

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/self/def.h>

#if !defined CAPACITY
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("No CAPACITY")
    #endif
    #define CAPACITY 32
#endif

DC_STATIC_ASSERT(CAPACITY > 0, DC_EXPAND_STRING(SELF) " CAPACITY cannot be empty");

#if !defined KEY
    #if !defined DC_PLACEHOLDERS
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
    #if !defined DC_PLACEHOLDERS
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

#define ENTRY NS(SELF, entry_t)
typedef struct {
    KEY key;
    VALUE value;
} ENTRY;

typedef struct {
    size_t size;
    ENTRY entries[CAPACITY];

    dc_gdb_marker derive_c_map_staticlinear;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

DC_STATIC_CONSTANT size_t NS(SELF, max_capacity) = (size_t)CAPACITY;

#define INVARIANT_CHECK(self)                                                                      \
    DC_ASSUME(self);                                                                               \
    DC_ASSUME((self)->size <= CAPACITY);

DC_PUBLIC static SELF NS(SELF, new)() {
    return (SELF){
        .size = 0,
        .entries = {},
        .derive_c_map_staticlinear = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

DC_PUBLIC static VALUE const* NS(SELF, try_read)(SELF const* self, KEY key) {
    INVARIANT_CHECK(self);
    for (size_t index = 0; index < self->size; index++) {
        if (KEY_EQ(&self->entries[index].key, &key)) {
            return &self->entries[index].value;
        }
    }
    return NULL;
}

DC_PUBLIC static VALUE const* NS(SELF, read)(SELF const* self, KEY key) {
    VALUE const* value = NS(SELF, try_read)(self, key);
    DC_ASSERT(value);
    return value;
}

DC_PUBLIC static VALUE* NS(SELF, try_write)(SELF* self, KEY key) {
    return (VALUE*)(NS(SELF, try_read)(self, key));
}

DC_PUBLIC static VALUE* NS(SELF, write)(SELF* self, KEY key) {
    VALUE* value = NS(SELF, try_write)(self, key);
    DC_ASSERT(value);
    return value;
}

DC_PUBLIC static VALUE* NS(SELF, try_insert)(SELF* self, KEY key, VALUE value) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (self->size >= CAPACITY) {
        return NULL;
    }

    VALUE const* existing_value = NS(SELF, try_read)(self, key);
    if (existing_value) {
        return NULL;
    }

    self->entries[self->size].key = key;
    self->entries[self->size].value = value;
    self->size++;
    return &self->entries[self->size - 1].value;
}

DC_PUBLIC static VALUE* NS(SELF, insert)(SELF* self, KEY key, VALUE value) {
    VALUE* placed = NS(SELF, try_insert)(self, key, value);
    DC_ASSERT(placed);
    return placed;
}

DC_PUBLIC static bool NS(SELF, try_remove)(SELF* self, KEY key, VALUE* dest) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    for (size_t index = 0; index < self->size; index++) {
        if (KEY_EQ(&self->entries[index].key, &key)) {
            KEY_DELETE(&self->entries[index].key);
            *dest = self->entries[index].value;
            // Shift remaining entries down
            for (size_t i = index; i < self->size - 1; i++) {
                self->entries[i] = self->entries[i + 1];
            }
            self->size--;
            return true;
        }
    }
    return false;
}

DC_PUBLIC static VALUE NS(SELF, remove)(SELF* self, KEY key) {
    VALUE dest;
    DC_ASSERT(NS(SELF, try_remove)(self, key, &dest));
    return dest;
}

DC_PUBLIC static void NS(SELF, delete_entry)(SELF* self, KEY key) {
    VALUE val = NS(SELF, remove)(self, key);
    VALUE_DELETE(&val);
}

DC_PUBLIC static size_t NS(SELF, size)(SELF const* self) { return self->size; }

DC_PUBLIC static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);

    SELF new_self = (SELF){
        .size = self->size,
        .derive_c_map_staticlinear = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
    for (size_t index = 0; index < self->size; index++) {
        new_self.entries[index].key = KEY_CLONE(&self->entries[index].key);
        new_self.entries[index].value = VALUE_CLONE(&self->entries[index].value);
    }
    return new_self;
}

DC_PUBLIC static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);

    for (size_t index = 0; index < self->size; index++) {
        KEY_DELETE(&self->entries[index].key);
        VALUE_DELETE(&self->entries[index].value);
    }
}

DC_PUBLIC static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "capacity: %zu,\n", (size_t)CAPACITY);
    dc_debug_fmt_print(fmt, stream, "size: %zu,\n", self->size);
    dc_debug_fmt_print(fmt, stream, "entries: [\n");
    fmt = dc_debug_fmt_scope_begin(fmt);
    for (size_t index = 0; index < self->size; index++) {
        dc_debug_fmt_print(fmt, stream, "{index: %lu, key: ", index);
        KEY_DEBUG(&self->entries[index].key, fmt, stream);
        fprintf(stream, ", value: ");
        VALUE_DEBUG(&self->entries[index].value, fmt, stream);
        fprintf(stream, "},\n");
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
    size_t next_index;
    mutation_version version;
} ITER_CONST;

typedef struct {
    KEY const* key;
    VALUE const* value;
} KV_PAIR_CONST;

DC_PUBLIC static bool NS(ITER_CONST, empty_item)(KV_PAIR_CONST const* item) {
    return item->key == NULL && item->value == NULL;
}

DC_PUBLIC static KV_PAIR_CONST NS(ITER_CONST, next)(ITER_CONST* iter) {
    mutation_version_check(&iter->version);
    size_t const next_index = iter->next_index;

    if (next_index >= iter->map->size) {
        return (KV_PAIR_CONST){.key = NULL, .value = NULL};
    }

    iter->next_index++;

    return (KV_PAIR_CONST){
        .key = &iter->map->entries[next_index].key,
        .value = &iter->map->entries[next_index].value,
    };
}

DC_PUBLIC static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->next_index >= iter->map->size;
}

DC_PUBLIC static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    INVARIANT_CHECK(self);

    return (ITER_CONST){
        .map = self,
        .next_index = 0,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef KV_PAIR_CONST
#undef ITER_CONST

#define ITER NS(SELF, iter)
#define KV_PAIR NS(ITER, item)

typedef struct {
    SELF* map;
    size_t next_index;
    mutation_version version;
} ITER;

typedef struct {
    KEY const* key;
    VALUE const* value;
} KV_PAIR;

DC_PUBLIC static bool NS(ITER, empty_item)(KV_PAIR const* item) {
    return item->key == NULL && item->value == NULL;
}

DC_PUBLIC static KV_PAIR NS(ITER, next)(ITER* iter) {
    mutation_version_check(&iter->version);
    size_t const next_index = iter->next_index;

    if (next_index >= iter->map->size) {
        return (KV_PAIR){.key = NULL, .value = NULL};
    }

    iter->next_index++;

    return (KV_PAIR){
        .key = &iter->map->entries[next_index].key,
        .value = &iter->map->entries[next_index].value,
    };
}

DC_PUBLIC static bool NS(ITER, empty)(ITER const* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->next_index >= iter->map->size;
}

DC_PUBLIC static ITER NS(SELF, get_iter)(SELF* self) {
    INVARIANT_CHECK(self);

    return (ITER){
        .map = self,
        .next_index = 0,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef KV_PAIR
#undef ITER

#undef INVARIANT_CHECK
#undef ENTRY

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
