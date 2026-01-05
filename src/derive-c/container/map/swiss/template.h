/// @brief A simple swiss table implementation.
/// See the abseil docs for swss table [here](https://abseil.io/about/design/swisstables)

#include "derive-c/container/map/swiss/utils.h"
#include "derive-c/core/math.h"
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
typedef ALLOC NS(SELF, alloc_t);

#define SLOT NS(SELF, slot_t)
typedef struct {
    VALUE value;
    KEY key;
} SLOT;

typedef struct {
    size_t capacity;
    size_t count;
    size_t tombstones;

    dc_swiss_ctrl* ctrl;
    SLOT* slots;

    NS(ALLOC, ref) alloc_ref;
    dc_gdb_marker derive_c_hashmap;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

DC_STATIC_CONSTANT size_t NS(SELF, max_capacity) = dc_swiss_index_capacity;

#define INVARIANT_CHECK(self)                                                                      \
    DC_ASSUME(self);                                                                               \
    DC_ASSUME(DC_MATH_IS_POWER_OF_2((self)->capacity));                                            \
    DC_ASSUME((self)->slots);                                                                      \
    DC_ASSUME((self)->ctrl);                                                                       \
    DC_ASSUME((self)->count + (self)->tombstones <= (self)->capacity);

static SELF PRIV(NS(SELF, new_with_exact_capacity))(size_t capacity, NS(ALLOC, ref) alloc_ref) {
    DC_ASSUME(capacity > DC_SWISS_SIMD_PROBE_SIZE);
    DC_ASSUME(DC_MATH_IS_POWER_OF_2(capacity));
    size_t ctrl_capacity = capacity + DC_SWISS_SIMD_PROBE_SIZE;

    dc_swiss_ctrl* ctrl = (dc_swiss_ctrl*)NS(ALLOC, allocate_zeroed)(
        alloc_ref, sizeof(dc_swiss_ctrl) * ctrl_capacity);
    SLOT* slots = (SLOT*)NS(ALLOC, allocate_uninit)(alloc_ref, sizeof(SLOT) * capacity);

    for (size_t i = 0; i < capacity; i++) {
        ctrl[i] = DC_SWISS_VAL_EMPTY;
    }
    ctrl[capacity] = DC_SWISS_VAL_SENTINEL;
    for (size_t i = 1; i < DC_SWISS_SIMD_PROBE_SIZE; i++) {
        ctrl[capacity + i] = ctrl[i - 1]; // currently empty
    }

    return (SELF){
        .capacity = capacity,
        .count = 0,
        .tombstones = 0,
        .ctrl = ctrl,
        .slots = slots,
        .alloc_ref = alloc_ref,
        .derive_c_hashmap = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static SELF NS(SELF, new_with_capacity_for)(size_t for_items, NS(ALLOC, ref) alloc_ref) {
    DC_ASSERT(for_items > 0);

    return PRIV(NS(SELF, new_with_exact_capacity))(dc_swiss_capacity(for_items), alloc_ref);
}

static SELF NS(SELF, new)(NS(ALLOC, ref) alloc_ref) {
    return NS(SELF, new_with_capacity_for)(DC_SWISS_INITIAL_CAPACITY, alloc_ref);
}

static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);

    size_t ctrl_capacity = self->capacity + DC_SWISS_SIMD_PROBE_SIZE;

    dc_swiss_ctrl* ctrl = (dc_swiss_ctrl*)NS(ALLOC, allocate_uninit)(
        self->alloc_ref, sizeof(dc_swiss_ctrl) * ctrl_capacity);
    SLOT* slots = (SLOT*)NS(ALLOC, allocate_uninit)(self->alloc_ref, sizeof(SLOT) * self->capacity);

    memcpy(ctrl, self->ctrl, sizeof(dc_swiss_ctrl) * ctrl_capacity);

    for (size_t i = 0; i < self->capacity; i++) {
        if (dc_swiss_is_present(self->ctrl[i])) {
            slots[i].key = KEY_CLONE(&self->slots[i].key);
            slots[i].value = VALUE_CLONE(&self->slots[i].value);
        }
    }

    return (SELF){
        .capacity = self->capacity,
        .count = self->count,
        .tombstones = self->tombstones,
        .ctrl = ctrl,
        .slots = slots,
        .alloc_ref = self->alloc_ref,
        .derive_c_hashmap = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static VALUE* PRIV(NS(SELF, try_insert_no_extend_capacity))(SELF* self, KEY key, VALUE value) {
    INVARIANT_CHECK(self);
    DC_ASSUME(self->count + self->tombstones < self->capacity);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    const size_t mask = self->capacity - 1;

    const size_t hash = KEY_HASH(&key);
    const dc_swiss_id id = dc_swiss_id_from_hash(hash);

    dc_swiss_optional_index first_deleted = DC_SWISS_NO_INDEX;

    const size_t start = hash & mask;

    for (size_t step = 0;; step += DC_SWISS_SIMD_PROBE_SIZE) {
        const size_t group = (start + step) & mask;

        // Scan the group once:
        //  - detect duplicates
        //  - remember first DELETED
        //  - stop at first EMPTY and insert (EMPTY terminates the probe)
        for (size_t j = 0; j < DC_SWISS_SIMD_PROBE_SIZE; j++) {
            const size_t i = (group + j) & mask;
            const dc_swiss_ctrl c = self->ctrl[i];

            switch (c) {
            case DC_SWISS_VAL_DELETED:
                if (first_deleted == DC_SWISS_NO_INDEX) {
                    first_deleted = i;
                }
                break;

            case DC_SWISS_VAL_EMPTY: {
                bool const has_deleted = first_deleted != DC_SWISS_NO_INDEX;
                const size_t ins = has_deleted ? first_deleted : i;
                if (has_deleted) {
                    self->tombstones--;
                }

                self->slots[ins].key = key;
                self->slots[ins].value = value;

                dc_swiss_ctrl_set_at(self->ctrl, self->capacity, ins, (dc_swiss_ctrl)id);

                self->count++;
                return &self->slots[ins].value;
            }

            case DC_SWISS_VAL_SENTINEL:
                // Treat as not usable; keep scanning.
                break;

            default:
                // Present slot: only a possible duplicate if the 7-bit id matches.
                if (dc_swiss_ctrl_get_id(c) == id && KEY_EQ(&self->slots[i].key, &key)) {
                    return NULL; // duplicate exists
                }
                break;
            }
        }
    }
}

static void PRIV(NS(SELF, rehash))(SELF* self, size_t new_capacity) {
    INVARIANT_CHECK(self);

    // NOTE: This code also works for shrinking the hashmap
    //  - we never expect to do this, so are defensive.
    //  - We do expect to rehash with the same capacity (tombstone cleanup)
    DC_ASSUME(new_capacity >= self->capacity);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    SELF new_map = PRIV(NS(SELF, new_with_exact_capacity))(new_capacity, self->alloc_ref);

    for (size_t i = 0; i < self->capacity; i++) {
        if (dc_swiss_is_present(self->ctrl[i])) {
            (void)PRIV(NS(SELF, try_insert_no_extend_capacity))(&new_map, self->slots[i].key,
                                                                self->slots[i].value);
        }
    }

    new_map.iterator_invalidation_tracker = self->iterator_invalidation_tracker;

    size_t ctrl_capacity = self->capacity + DC_SWISS_SIMD_PROBE_SIZE;
    NS(ALLOC, deallocate)(self->alloc_ref, self->ctrl, sizeof(dc_swiss_ctrl) * ctrl_capacity);
    NS(ALLOC, deallocate)(self->alloc_ref, self->slots, sizeof(SLOT) * self->capacity);

    *self = new_map;
}

static void NS(SELF, extend_capacity_for)(SELF* self, size_t expected_items) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    size_t new_capacity = dc_swiss_capacity(expected_items);
    if (new_capacity > self->capacity) {
        PRIV(NS(SELF, rehash))(self, new_capacity);
    }
}

static VALUE* NS(SELF, try_insert)(SELF* self, KEY key, VALUE value) {
    INVARIANT_CHECK(self);

    if (self->count >= NS(SELF, max_capacity)) {
        return NULL;
    }

    switch (dc_swiss_heuristic_should_extend(self->tombstones, self->count, self->capacity)) {
    case DC_SWISS_DOUBLE_CAPACITY:
        PRIV(NS(SELF, rehash))(self, self->capacity * 2);
        break;
    case DC_SWISS_CLEANUP_TOMBSONES:
        PRIV(NS(SELF, rehash))(self, self->capacity);
        break;
    case DC_SWISS_DO_NOTHING:
        break;
    }

    return PRIV(NS(SELF, try_insert_no_extend_capacity))(self, key, value);
}

static VALUE* NS(SELF, insert)(SELF* self, KEY key, VALUE value) {
    VALUE* value_ptr = NS(SELF, try_insert)(self, key, value);
    DC_ASSERT(value_ptr);
    return value_ptr;
}

static VALUE const* NS(SELF, try_read)(SELF const* self, KEY key) {
    const size_t mask = self->capacity - 1;

    const size_t hash = KEY_HASH(&key);
    const dc_swiss_id id = dc_swiss_id_from_hash(hash);

    const size_t start = hash & mask;

    for (size_t step = 0;; step += DC_SWISS_SIMD_PROBE_SIZE) {
        const size_t group = (start + step) & mask;

        for (size_t j = 0; j < DC_SWISS_SIMD_PROBE_SIZE; j++) {
            const size_t i = (group + j) & mask;
            const dc_swiss_ctrl c = self->ctrl[i];

            switch (c) {
            case DC_SWISS_VAL_EMPTY:
                // EMPTY terminates the probe: key does not exist
                return NULL;

            case DC_SWISS_VAL_DELETED:
            case DC_SWISS_VAL_SENTINEL:
                // Keep probing
                break;

            default:
                // Present slot: check fingerprint, then full key
                if (dc_swiss_ctrl_get_id(c) == id && KEY_EQ(&self->slots[i].key, &key)) {
                    return &self->slots[i].value;
                }
                break;
            }
        }
    }
}

static VALUE const* NS(SELF, read)(SELF const* self, KEY key) {
    VALUE const* value = NS(SELF, try_read)(self, key);
    DC_ASSERT(value);
    return value;
}

static VALUE* NS(SELF, try_write)(SELF* self, KEY key) {
    INVARIANT_CHECK(self);
    return (VALUE*)NS(SELF, try_read)((SELF const*)self, key);
}

static VALUE* NS(SELF, write)(SELF* self, KEY key) {
    VALUE* value = NS(SELF, try_write)(self, key);
    DC_ASSERT(value);
    return value;
}

static bool NS(SELF, try_remove)(SELF* self, KEY key, VALUE* destination) {
    INVARIANT_CHECK(self);

    const size_t mask = self->capacity - 1;

    const size_t hash = KEY_HASH(&key);
    const dc_swiss_id id = dc_swiss_id_from_hash(hash);

    const size_t start = hash & mask;

    for (size_t step = 0;; step += DC_SWISS_SIMD_PROBE_SIZE) {
        const size_t group = (start + step) & mask;

        for (size_t j = 0; j < DC_SWISS_SIMD_PROBE_SIZE; j++) {
            const size_t i = (group + j) & mask;
            const dc_swiss_ctrl c = self->ctrl[i];

            switch (c) {
            case DC_SWISS_VAL_EMPTY:
                // EMPTY terminates probe: key not present
                return false;

            case DC_SWISS_VAL_DELETED:
            case DC_SWISS_VAL_SENTINEL:
                // keep probing
                break;

            default:
                // Present slot: check fingerprint, then full key
                if (dc_swiss_ctrl_get_id(c) != id) {
                    break;
                }
                if (!KEY_EQ(&self->slots[i].key, &key)) {
                    break;
                }

                // Found
                if (destination != NULL) {
                    *destination = self->slots[i].value;
                }

                // Mark tombstone (must NOT become EMPTY)
                dc_swiss_ctrl_set_at(self->ctrl, self->capacity, i, DC_SWISS_VAL_DELETED);

                self->count--;
                self->tombstones++;
                return true;
            }
        }
    }
}

static VALUE NS(SELF, remove)(SELF* self, KEY key) {
    VALUE value;
    DC_ASSERT(NS(SELF, try_remove)(self, key, &value));
    return value;
}

static void NS(SELF, delete_entry)(SELF* self, KEY key) {
    VALUE value = NS(SELF, remove)(self, key);
    VALUE_DELETE(&value);
}

static size_t NS(SELF, size)(SELF const* self) {
    INVARIANT_CHECK(self);
    return self->count;
}

static void PRIV(NS(SELF, next_populated_index))(SELF const* self, dc_swiss_optional_index* index) {
    size_t i = *index;
    while (i < self->capacity && !dc_swiss_is_present(self->ctrl[i])) {
        ++i;
    }
    *index = (i == self->capacity) ? DC_SWISS_NO_INDEX : i;
}

static void NS(SELF, delete)(SELF* self) {
    for (size_t i = 0; i < self->capacity; i++) {
        if (dc_swiss_is_present(self->ctrl[i])) {
            KEY_DELETE(&self->slots[i].key);
            VALUE_DELETE(&self->slots[i].value);
        }
    }

    size_t ctrl_capacity = self->capacity + DC_SWISS_SIMD_PROBE_SIZE;
    NS(ALLOC, deallocate)(self->alloc_ref, self->ctrl, sizeof(dc_swiss_ctrl) * ctrl_capacity);
    NS(ALLOC, deallocate)(self->alloc_ref, self->slots, sizeof(SLOT) * self->capacity);
}

#define ITER_CONST NS(SELF, iter_const)
#define KV_PAIR_CONST NS(ITER_CONST, item)

typedef struct {
    SELF const* map;
    dc_swiss_optional_index next_index;
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

    dc_swiss_optional_index index = iter->next_index;
    if (index == DC_SWISS_NO_INDEX) {
        return (KV_PAIR_CONST){
            .key = NULL,
            .value = NULL,
        };
    }

    iter->next_index++;
    PRIV(NS(SELF, next_populated_index))(iter->map, &iter->next_index);
    return (KV_PAIR_CONST){
        .key = &iter->map->slots[index].key,
        .value = &iter->map->slots[index].value,
    };
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    INVARIANT_CHECK(self);

    dc_swiss_optional_index index = 0;
    PRIV(NS(SELF, next_populated_index))(self, &index);

    return (ITER_CONST){
        .map = self,
        .next_index = index,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);

    dc_debug_fmt_print(fmt, stream, "capacity: %lu,\n", self->capacity);
    dc_debug_fmt_print(fmt, stream, "tombstones: %lu,\n", self->tombstones);
    dc_debug_fmt_print(fmt, stream, "count: %lu,\n", self->count);

    dc_debug_fmt_print(fmt, stream, "ctrl: @%p[%lu + simd probe size additional %lu],\n",
                       self->ctrl, self->capacity, DC_SWISS_SIMD_PROBE_SIZE);
    dc_debug_fmt_print(fmt, stream, "slots: @%p[%lu],\n", self->slots, self->capacity);

    dc_debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(NS(NS(ALLOC, ref), deref)(self->alloc_ref), fmt, stream);
    fprintf(stream, ",\n");

    dc_debug_fmt_print(fmt, stream, "entries: [\n");
    fmt = dc_debug_fmt_scope_begin(fmt);

    ITER_CONST iter = NS(SELF, get_iter_const)(self);
    KV_PAIR_CONST item;

    for (KV_PAIR_CONST item = NS(ITER_CONST, next)(&iter); !NS(ITER_CONST, empty_item)(&item);
         item = NS(ITER_CONST, next)(&iter)) {
        dc_debug_fmt_print(fmt, stream, "{\n");
        fmt = dc_debug_fmt_scope_begin(fmt);

        dc_debug_fmt_print(fmt, stream, "key: ");
        KEY_DEBUG(item.key, fmt, stream);
        fprintf(stream, ",\n");

        dc_debug_fmt_print(fmt, stream, "value: ");
        VALUE_DEBUG(item.value, fmt, stream);
        fprintf(stream, ",\n");

        fmt = dc_debug_fmt_scope_end(fmt);
        dc_debug_fmt_print(fmt, stream, "},\n");
    }

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "]\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#undef KV_PAIR_CONST
#undef ITER_CONST

#define ITER NS(SELF, iter)
#define KV_PAIR NS(ITER, item)

typedef struct {
    SELF const* map;
    dc_swiss_optional_index next_index;
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

    dc_swiss_optional_index index = iter->next_index;
    if (index == DC_SWISS_NO_INDEX) {
        return (KV_PAIR){
            .key = NULL,
            .value = NULL,
        };
    }

    iter->next_index++;
    PRIV(NS(SELF, next_populated_index))(iter->map, &iter->next_index);
    return (KV_PAIR){
        .key = &iter->map->slots[index].key,
        .value = &iter->map->slots[index].value,
    };
}

static ITER NS(SELF, get_iter)(SELF* self) {
    INVARIANT_CHECK(self);

    dc_swiss_optional_index index = 0;
    PRIV(NS(SELF, next_populated_index))(self, &index);

    return (ITER){
        .map = self,
        .next_index = index,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef KV_PAIR
#undef ITER

#undef INVARIANT_CHECK

#undef SLOT

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

DC_TRAIT_MAP(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>