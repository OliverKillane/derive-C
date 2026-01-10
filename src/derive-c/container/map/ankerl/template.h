/// @brief A simple swiss table implementation.
/// See the abseil docs for swss table [here](https://abseil.io/about/design/swisstables)

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined KEY
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("No KEY")
    #endif
    #define KEY map_key_t
typedef int KEY;
#endif

#if !defined KEY_HASH
    #if !defined DC_PLACEHOLDERS
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
typedef ALLOC NS(SELF, alloc_t);

// JUSTIFY: Using name instead of SELF
// - We need to use SLOT from within the vector template, so need to avoid using
//   SELF (which is SLOT_VECTOR in that context)
#define SLOT NS(NAME, slot_t)
typedef struct {
    KEY key;
    VALUE value;
} SLOT;

static SLOT NS(SLOT, clone)(SLOT const* slot) {
    return (SLOT){
        .key = KEY_CLONE(&slot->key),
        .value = VALUE_CLONE(&slot->value),
    };
}

static void NS(SLOT, debug)(SLOT const* slot, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, DC_EXPAND_STRING(SLOT) "@%p {\n", slot);
    fmt = dc_debug_fmt_scope_begin(fmt);

    dc_debug_fmt_print(fmt, stream, "key: ");
    KEY_DEBUG(&slot->key, fmt, stream);
    fprintf(stream, ",\n");

    dc_debug_fmt_print(fmt, stream, "value: ");
    VALUE_DEBUG(&slot->value, fmt, stream);
    fprintf(stream, ",\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

static void NS(SLOT, delete)(SLOT* slot) {
    KEY_DELETE(&slot->key);
    VALUE_DELETE(&slot->value);
}

#define SLOT_VECTOR NS(NAME, item_vectors)

#pragma push_macro("ALLOC")

// ITEM is already defined
#define ITEM SLOT                  // [DERIVE-C] for template
#define ITEM_CLONE NS(SLOT, clone) // [DERIVE-C] for template
#define ITEM_DEBUG NS(SLOT, debug) // [DERIVE-C] for template
#define ITEM_CLONE NS(SLOT, clone) // [DERIVE-C] for template
#define INTERNAL_NAME SLOT_VECTOR  // [DERIVE-C] for template
#include <derive-c/container/vector/dynamic/template.h>

#pragma pop_macro("ALLOC")

#define BUCKET NS(SELF, bucket)

#if defined SMALL_BUCKETS
    #define INDEX_KIND dc_ankerl_index_small

typedef struct {
    dc_ankerl_mdata mdata;
    uint16_t index;
} BUCKET;

DC_STATIC_CONSTANT size_t NS(SELF, max_capacity) = (size_t)UINT16_MAX;

static BUCKET PRIV(NS(BUCKET, new))(dc_ankerl_mdata mdata, size_t index) {
    DC_ASSUME(index <= NS(SELF, max_capacity));
    return (BUCKET){
        .mdata = mdata,
        .index = (uint16_t)(index),
    };
}

static size_t PRIV(NS(BUCKET, get_index))(BUCKET const* bucket) { return (size_t)bucket->index; }

DC_STATIC_ASSERT(sizeof(BUCKET) == 4);
    #undef SMALL_BUCKETS // [DERIVE-C] for input arg
#else
    #define INDEX_KIND dc_ankerl_index_large

typedef struct {
    dc_ankerl_mdata mdata;
    uint16_t index_hi;
    uint32_t index_lo;
} BUCKET;

DC_STATIC_CONSTANT size_t NS(SELF, max_capacity) = (size_t)UINT32_MAX + ((size_t)UINT16_MAX << 32);

static BUCKET PRIV(NS(BUCKET, new))(dc_ankerl_mdata mdata, size_t index) {
    DC_ASSUME(index <= NS(SELF, max_capacity));
    return (BUCKET){
        .mdata = mdata,
        .index_hi = (uint16_t)(index >> 32),
        .index_lo = (uint32_t)index,
    };
}

static size_t PRIV(NS(BUCKET, get_index))(BUCKET const* bucket) {
    return (size_t)bucket->index_lo + ((size_t)bucket->index_hi << 32);
}

DC_STATIC_ASSERT(sizeof(BUCKET) == 8);
#endif

typedef struct {
    size_t buckets_capacity;
    BUCKET* buckets;
    SLOT_VECTOR slots;

    NS(ALLOC, ref) alloc_ref;
    dc_gdb_marker derive_c_hashmap;
    // JUSTIFY: No iteration invalidator
    // - Iteration is dependent on the vector, which already tracks this.
} SELF;

#define INVARIANT_CHECK(self)                                                                      \
    DC_ASSUME(self);                                                                               \
    DC_ASSUME(DC_MATH_IS_POWER_OF_2((self)->buckets_capacity));                                    \
    DC_ASSUME((self)->buckets);

static SELF PRIV(NS(SELF, new_with_exact_capacity))(size_t capacity, NS(ALLOC, ref) alloc_ref) {
    DC_ASSUME(capacity > 0);
    DC_ASSUME(DC_MATH_IS_POWER_OF_2(capacity));

    BUCKET* buckets = (BUCKET*)NS(ALLOC, allocate_zeroed)(alloc_ref, capacity * sizeof(BUCKET));

    return (SELF){
        .buckets_capacity = capacity,
        .buckets = buckets,
        .slots = NS(SLOT_VECTOR, new_with_capacity)(capacity, alloc_ref),
        .alloc_ref = alloc_ref,
    };
}

static SELF NS(SELF, new_with_capacity_for)(size_t for_items, NS(ALLOC, ref) alloc_ref) {
    DC_ASSERT(for_items > 0);
    return PRIV(NS(SELF, new_with_exact_capacity))(dc_ankerl_buckets_capacity(for_items),
                                                   alloc_ref);
}

static SELF NS(SELF, new)(NS(ALLOC, ref) alloc_ref) {
    return PRIV(NS(SELF, new_with_exact_capacity))(dc_ankerl_initial_items, alloc_ref);
}

static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);

    BUCKET* new_buckets = (BUCKET*)NS(ALLOC, allocate_uninit)(
        self->alloc_ref, self->buckets_capacity * sizeof(BUCKET));
    memcpy(new_buckets, self->buckets, self->buckets_capacity * sizeof(BUCKET));

    return (SELF){
        .buckets_capacity = self->buckets_capacity,
        .buckets = new_buckets,
        .slots = NS(SLOT_VECTOR, clone)(&self->slots),
        .alloc_ref = self->alloc_ref,
        .derive_c_hashmap = dc_gdb_marker_new(),
    };
}

static VALUE* PRIV(NS(SELF, try_insert_no_extend_capacity))(SELF* self, KEY key, VALUE value) {
    INVARIANT_CHECK(self);
    DC_ASSUME(NS(SLOT_VECTOR, size)(&self->slots) < self->buckets_capacity);
    const size_t mask = self->buckets_capacity - 1;

    const size_t hash = KEY_HASH(&key);
    const uint8_t fp = dc_ankerl_fingerprint_from_hash(hash);
    const size_t desired = hash & mask;

    {
        dc_ankerl_dfd dfd = dc_ankerl_dfd_new(0);
        for (size_t pos = desired;; pos = (pos + 1) & mask) {
            BUCKET const* b = &self->buckets[pos];

            if (!dc_ankerl_mdata_present(&b->mdata)) {
                break;
            }

            // JUSTIFY: Checking for the maximum DFD
            //  - dfd (distance-from-desired) uses saturating arithmetic.
            //  - Once dfd reaches dc_ankerl_dfd_max it no longer encodes a strict ordering,
            //    so the usual Robin Hood early-out (b->dfd < dfd) is only valid while dfd
            //    is not saturated. After saturation we must continue probing until EMPTY.
            if (dfd != dc_ankerl_dfd_max && b->mdata.dfd < dfd) {
                break;
            }

            if (b->mdata.fingerprint == fp) {
                const size_t di = PRIV(NS(BUCKET, get_index))(b);
                SLOT const* slot = NS(SLOT_VECTOR, read)(&self->slots, di);
                if (KEY_EQ(&slot->key, &key)) {
                    return NULL;
                }
            }

            dfd = dc_ankerl_dfd_increment(dfd);
        }
    }

    const size_t dense_index = NS(SLOT_VECTOR, size)(&self->slots);
    NS(SLOT_VECTOR, push)(&self->slots, (SLOT){
                                            .key = key,
                                            .value = value,
                                        });
    BUCKET cur = PRIV(NS(BUCKET, new))(
        (dc_ankerl_mdata){
            .fingerprint = fp,
            .dfd = dc_ankerl_dfd_new(0),
        },
        dense_index);

    for (size_t pos = desired;; pos = (pos + 1) & mask) {
        BUCKET* b = &self->buckets[pos];

        if (!dc_ankerl_mdata_present(&b->mdata)) {
            *b = cur;
            return &NS(SLOT_VECTOR, write)(&self->slots, dense_index)->value;
        }

        if (b->mdata.dfd < cur.mdata.dfd) {
            BUCKET tmp = *b;
            *b = cur;
            cur = tmp;
        }

        cur.mdata.dfd = dc_ankerl_dfd_increment(cur.mdata.dfd);
    }
}

static void PRIV(NS(SELF, rehash))(SELF* self, size_t new_capacity) {
    INVARIANT_CHECK(self);
    DC_ASSUME(DC_MATH_IS_POWER_OF_2(new_capacity));

    BUCKET* new_buckets =
        (BUCKET*)NS(ALLOC, allocate_zeroed)(self->alloc_ref, new_capacity * sizeof(BUCKET));

    const size_t new_mask = new_capacity - 1;
    const size_t n = NS(SLOT_VECTOR, size)(&self->slots);

    for (size_t dense_index = 0; dense_index < n; ++dense_index) {
        SLOT const* slot = NS(SLOT_VECTOR, read)(&self->slots, dense_index);

        const size_t hash = KEY_HASH(&slot->key);
        const uint8_t fp = dc_ankerl_fingerprint_from_hash(hash);
        const size_t desired = hash & new_mask;

        BUCKET cur = PRIV(NS(BUCKET, new))(
            (dc_ankerl_mdata){
                .fingerprint = fp,
                .dfd = dc_ankerl_dfd_new(0),
            },
            dense_index);

        for (size_t pos = desired;; pos = (pos + 1) & new_mask) {
            BUCKET* b = &new_buckets[pos];

            if (!dc_ankerl_mdata_present(&b->mdata)) {
                *b = cur;
                break;
            }

            if (b->mdata.dfd < cur.mdata.dfd) {
                BUCKET tmp = *b;
                *b = cur;
                cur = tmp;
            }

            cur.mdata.dfd = dc_ankerl_dfd_increment(cur.mdata.dfd);
        }
    }

    NS(ALLOC, deallocate)(self->alloc_ref, self->buckets, self->buckets_capacity * sizeof(BUCKET));
    self->buckets = new_buckets;
    self->buckets_capacity = new_capacity;
}

static void NS(SELF, extend_capacity_for)(SELF* self, size_t expected_items) {
    INVARIANT_CHECK(self);

    const size_t required = dc_ankerl_buckets_capacity(expected_items);
    if (required <= self->buckets_capacity) {
        return;
    }

    PRIV(NS(SELF, rehash))(self, required);
}

static VALUE* NS(SELF, try_insert)(SELF* self, KEY key, VALUE value) {
    INVARIANT_CHECK(self);

    if (NS(SLOT_VECTOR, size)(&self->slots) >= NS(SELF, max_capacity)) {
        return NULL;
    }

    const size_t size = NS(SLOT_VECTOR, size)(&self->slots);
    if (size >= self->buckets_capacity) {
        NS(SELF, extend_capacity_for)(self, size * 2);
    }

    return PRIV(NS(SELF, try_insert_no_extend_capacity))(self, key, value);
}

static VALUE* NS(SELF, insert)(SELF* self, KEY key, VALUE value) {
    VALUE* value_ptr = NS(SELF, try_insert)(self, key, value);
    DC_ASSERT(value_ptr);
    return value_ptr;
}

static bool PRIV(NS(SELF, try_find))(SELF const* self, KEY const* key, size_t* out_bucket_pos,
                                     size_t* out_dense_index) {
    INVARIANT_CHECK(self);
    DC_ASSUME(key);
    DC_ASSUME(out_bucket_pos);
    DC_ASSUME(out_dense_index);

    const size_t mask = self->buckets_capacity - 1;

    const size_t hash = KEY_HASH(key);
    const uint8_t fp = dc_ankerl_fingerprint_from_hash(hash);
    const size_t desired = hash & mask;

    dc_ankerl_dfd dfd = dc_ankerl_dfd_new(0);
    for (size_t pos = desired;; pos = (pos + 1) & mask) {
        BUCKET const* b = &self->buckets[pos];

        if (!dc_ankerl_mdata_present(&b->mdata)) {
            return false;
        }

        // NOTE: capped/saturating dfd: early-out only valid while dfd not saturated.
        if (dfd != dc_ankerl_dfd_max && b->mdata.dfd < dfd) {
            return false;
        }

        if (b->mdata.fingerprint == fp) {
            const size_t di = PRIV(NS(BUCKET, get_index))(b);
            SLOT const* slot = NS(SLOT_VECTOR, read)(&self->slots, di);
            if (KEY_EQ(&slot->key, key)) {
                *out_bucket_pos = pos;
                *out_dense_index = di;
                return true;
            }
        }

        dfd = dc_ankerl_dfd_increment(dfd);
    }
}

static VALUE const* NS(SELF, try_read)(SELF const* self, KEY key) {
    INVARIANT_CHECK(self);
    size_t pos;
    size_t di;
    if (!PRIV(NS(SELF, try_find))(self, &key, &pos, &di)) {
        return NULL;
    }

    return &NS(SLOT_VECTOR, read)(&self->slots, di)->value;
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

    size_t found_pos;
    size_t removed_dense_index;
    if (!PRIV(NS(SELF, try_find))((SELF const*)self, &key, &found_pos, &removed_dense_index)) {
        return false; // unchanged
    }

    const size_t mask = self->buckets_capacity - 1;

    // Move out value (or delete if destination is NULL) and delete key.
    {
        SLOT* slot = NS(SLOT_VECTOR, write)(&self->slots, removed_dense_index);

        if (destination) {
            *destination = slot->value;
        } else {
            VALUE_DELETE(&slot->value);
        }

        KEY_DELETE(&slot->key);
    }

    // Backshift delete from buckets starting at found_pos.
    {
        size_t hole = found_pos;

        for (;;) {
            const size_t next = (hole + 1) & mask;
            BUCKET* nb = &self->buckets[next];

            if (!dc_ankerl_mdata_present(&nb->mdata) || nb->mdata.dfd == dc_ankerl_dfd_new(0)) {
                self->buckets[hole].mdata.dfd = dc_ankerl_dfd_none;
                break;
            }

            self->buckets[hole] = *nb;
            self->buckets[hole].mdata.dfd =
                dc_ankerl_dfd_decrement_for_backshift(self->buckets[hole].mdata.dfd);
            hole = next;
        }
    }

    // Dense swap-remove + update moved element’s bucket (if swap occurred).
    {
        const size_t size = NS(SLOT_VECTOR, size)(&self->slots);
        const size_t last = size - 1;

        if (removed_dense_index != last) {
            SLOT* dst = NS(SLOT_VECTOR, write)(&self->slots, removed_dense_index);
            SLOT* src = NS(SLOT_VECTOR, write)(&self->slots, last);
            *dst = *src;

            // Find moved key's bucket and patch its dense index to removed_dense_index.
            // (One extra probe only when we swapped.)
            const size_t moved_hash = KEY_HASH(&dst->key);
            const uint8_t moved_fp = dc_ankerl_fingerprint_from_hash(moved_hash);
            const size_t desired = moved_hash & mask;

            dc_ankerl_dfd dfd = dc_ankerl_dfd_new(0);
            for (size_t pos = desired;; pos = (pos + 1) & mask) {
                BUCKET* b = &self->buckets[pos];

                DC_ASSERT(dc_ankerl_mdata_present(&b->mdata));

                if (dfd != dc_ankerl_dfd_max && b->mdata.dfd < dfd) {
                    DC_ASSERT(false);
                }

                if (b->mdata.fingerprint == moved_fp) {
                    const size_t di = PRIV(NS(BUCKET, get_index))(b);
                    SLOT const* slot = NS(SLOT_VECTOR, read)(&self->slots, di);
                    if (KEY_EQ(&slot->key, &dst->key)) {
                        *b = PRIV(NS(BUCKET, new))(b->mdata, removed_dense_index);
                        break;
                    }
                }

                dfd = dc_ankerl_dfd_increment(dfd);
            }
        }

        (void)NS(SLOT_VECTOR, pop)(&self->slots);
    }

    return true;
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
    return NS(SLOT_VECTOR, size)(&self->slots);
}

static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);

    NS(ALLOC, deallocate)(self->alloc_ref, self->buckets, self->buckets_capacity * sizeof(BUCKET));
    NS(SLOT_VECTOR, delete)(&self->slots);
}

#define ITER_CONST NS(SELF, iter_const)
#define KV_PAIR_CONST NS(ITER_CONST, item)

typedef struct {
    NS(SLOT_VECTOR, iter_const) iter;
} ITER_CONST;

typedef struct {
    KEY const* key;
    VALUE const* value;
} KV_PAIR_CONST;

static bool NS(ITER_CONST, empty_item)(KV_PAIR_CONST const* item) {
    return item->key == NULL && item->value == NULL;
}

static KV_PAIR_CONST NS(ITER_CONST, next)(ITER_CONST* iter) {
    SLOT const* next_item = NS(NS(SLOT_VECTOR, iter_const), next)(&iter->iter);
    if (!next_item) {
        return (KV_PAIR_CONST){
            .key = NULL,
            .value = NULL,
        };
    }
    return (KV_PAIR_CONST){
        .key = &next_item->key,
        .value = &next_item->value,
    };
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    INVARIANT_CHECK(self);
    return (ITER_CONST){
        .iter = NS(SLOT_VECTOR, get_iter_const)(&self->slots),
    };
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);

    dc_debug_fmt_print(fmt, stream, "bucket capacity: %lu,\n", self->buckets_capacity);

    dc_debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(NS(NS(ALLOC, ref), deref)(self->alloc_ref), fmt, stream);
    fprintf(stream, ",\n");

    dc_debug_fmt_print(fmt, stream, "slots: ");
    NS(SLOT_VECTOR, debug)(&self->slots, fmt, stream);
    fprintf(stream, ",\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#undef KV_PAIR_CONST
#undef ITER_CONST

#define ITER NS(SELF, iter)
#define KV_PAIR NS(ITER, item)

typedef struct {
    NS(SLOT_VECTOR, iter) iter;
} ITER;

typedef struct {
    KEY const* key;
    VALUE const* value;
} KV_PAIR;

static bool NS(ITER, empty_item)(KV_PAIR const* item) {
    return item->key == NULL && item->value == NULL;
}

static KV_PAIR NS(ITER, next)(ITER* iter) {
    SLOT* next_item = NS(NS(SLOT_VECTOR, iter), next)(&iter->iter);
    if (!next_item) {
        return (KV_PAIR){
            .key = NULL,
            .value = NULL,
        };
    }
    return (KV_PAIR){
        .key = &next_item->key,
        .value = &next_item->value,
    };
}

static ITER NS(SELF, get_iter)(SELF* self) {
    INVARIANT_CHECK(self);
    return (ITER){
        .iter = NS(SLOT_VECTOR, get_iter)(&self->slots),
    };
}

#undef KV_PAIR
#undef ITER

#undef INVARIANT_CHECK
#undef INDEX_KIND
#undef BUCKET
#undef SLOT_VECTOR
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