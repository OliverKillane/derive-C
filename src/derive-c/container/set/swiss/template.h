/// @brief A swisstable based hashset

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined ITEM
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("No ITEM")
    #endif
    #define ITEM set_item_t
typedef size_t ITEM;
#endif

#if !defined ITEM_HASH
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("No ITEM_HASH")
    #endif

    #define ITEM_HASH item_hash
static size_t ITEM_HASH(ITEM const* item) { return *item; }
#endif

#if !defined ITEM_EQ
    #define ITEM_EQ DC_MEM_EQ
#endif

#if !defined ITEM_DELETE
    #define ITEM_DELETE DC_NO_DELETE
#endif

#if !defined ITEM_CLONE
    #define ITEM_CLONE DC_COPY_CLONE
#endif

#if !defined ITEM_DEBUG
    #define ITEM_DEBUG DC_DEFAULT_DEBUG
#endif

typedef ITEM NS(SELF, item_t);

#define MAP PRIV(NS(NAME, inner_map))

#pragma push_macro("ALLOC")

#define KEY ITEM                    // [DERIVE-C] for template
#define KEY_HASH ITEM_HASH          // [DERIVE-C] for template
#define KEY_EQ ITEM_EQ              // [DERIVE-C] for template
#define KEY_DELETE ITEM_DELETE      // [DERIVE-C] for template
#define KEY_CLONE ITEM_CLONE        // [DERIVE-C] for template
#define KEY_DEBUG ITEM_DEBUG        // [DERIVE-C] for template
#define VALUE dc_unit               // [DERIVE-C] for template
#define VALUE_DELETE dc_unit_delete // [DERIVE-C] for template
#define VALUE_CLONE dc_unit_clone   // [DERIVE-C] for template
#define VALUE_DEBUG dc_unit_debug   // [DERIVE-C] for template
#define INTERNAL_NAME MAP           // [DERIVE-C] for template
#include <derive-c/container/map/swiss/template.h>

#pragma pop_macro("ALLOC")

DC_STATIC_CONSTANT size_t NS(SELF, max_capacity) = NS(MAP, max_capacity);

typedef struct {
    MAP map;
} SELF;

DC_PUBLIC static SELF NS(SELF, new_with_capacity_for)(size_t for_items, NS(ALLOC, ref) alloc_ref) {
    return (SELF){
        .map = NS(MAP, new_with_capacity_for)(for_items, alloc_ref),
    };
}

DC_PUBLIC static SELF NS(SELF, new)(NS(ALLOC, ref) alloc_ref) {
    return (SELF){
        .map = NS(MAP, new)(alloc_ref),
    };
}

DC_PUBLIC static SELF NS(SELF, clone)(SELF const* self) {
    return (SELF){
        .map = NS(MAP, clone)(&self->map),
    };
}

DC_PUBLIC static void NS(SELF, extend_capacity_for)(SELF* self, size_t expected_items) {
    NS(MAP, extend_capacity_for)(&self->map, expected_items);
}

DC_PUBLIC static bool NS(SELF, try_add)(SELF* self, ITEM item) {
    return NS(MAP, try_insert(&self->map, item, dc_unit_new())) != NULL;
}

DC_PUBLIC static void NS(SELF, add)(SELF* self, ITEM item) {
    bool const inserted = NS(SELF, try_add)(self, item);
    DC_ASSERT(inserted, "Failed to insert item");
}

DC_PUBLIC static bool NS(SELF, contains)(SELF const* self, ITEM item) {
    return NS(MAP, try_read)(&self->map, item) != NULL;
}

DC_PUBLIC static bool NS(SELF, try_remove)(SELF* self, ITEM item) {
    dc_unit dest;
    return NS(MAP, try_remove)(&self->map, item, &dest);
}

DC_PUBLIC static void NS(SELF, remove)(SELF* self, ITEM item) {
    bool const removed = NS(SELF, try_remove)(self, item);
    DC_ASSERT(removed, "Failed to remove");
}

DC_PUBLIC static size_t NS(SELF, size)(SELF const* self) { return NS(MAP, size)(&self->map); }

#define ITER_CONST NS(SELF, iter_const)

typedef struct {
    NS(MAP, iter_const) map_iter;
} ITER_CONST;

typedef ITEM const* NS(ITER_CONST, item);

DC_PUBLIC static bool NS(ITER_CONST, empty_item)(ITEM const* const* item) { return *item == NULL; }

DC_PUBLIC static ITEM const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    return NS(MAP, NS(iter_const, next))(&iter->map_iter).key;
}

DC_PUBLIC static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    return (ITER_CONST){
        .map_iter = NS(MAP, get_iter_const)(&self->map),
    };
}

DC_PUBLIC static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);

    dc_debug_fmt_print(fmt, stream, "map: ");
    NS(MAP, debug)(&self->map, fmt, stream);
    fprintf(stream, ",\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#undef ITER_CONST

DC_PUBLIC static void NS(SELF, delete)(SELF* self) { NS(MAP, delete)(&self->map); }

#undef MAP

#undef ITEM_DEBUG
#undef ITEM_CLONE
#undef ITEM_DELETE
#undef ITEM_EQ
#undef ITEM_HASH
#undef ITEM

DC_TRAIT_SET(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>
