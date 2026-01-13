/// @brief A tagged union containing an integer index type
//  - Used the the arena data structures

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif
#include <derive-c/core/self/def.h>

#if !defined SLOT_INDEX_TYPE
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("A SLOT_INDEX_TYPE needs to be specified for a slot")
    #endif
    #define SLOT_INDEX_TYPE int
#endif

#if !defined SLOT_VALUE
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("A SLOT_VALUE must be defined for a slot")
    #endif
    #define SLOT_VALUE slot_value_t
typedef struct {
    int x;
} SLOT_VALUE;
    #define SLOT_VALUE_DELETE slot_value_delete
static void SLOT_VALUE_DELETE(slot_value_t* self);
    #define SLOT_VALUE_CLONE slot_value_clone
static slot_value_t SLOT_VALUE_CLONE(slot_value_t const* self);
    #define SLOT_VALUE_DEBUG slot_value_debug
static void SLOT_VALUE_DEBUG(SLOT_VALUE const*, dc_debug_fmt, FILE* stream);
#endif

#if !defined SLOT_VALUE_DELETE
    #define SLOT_VALUE_DELETE DC_NO_DELETE
#endif

#if !defined SLOT_VALUE_CLONE
    #define SLOT_VALUE_CLONE DC_COPY_CLONE
#endif

#if !defined SLOT_VALUE_DEBUG
    #define SLOT_VALUE_DEBUG DC_DEFAULT_DEBUG
#endif

typedef struct {
// JUSTIFY: Union for release, struct for debug
// In order to allow for easy msan / uninitialised checks with debug
//  - To allow derive-c's own tests to just check the value* goes to uninitialised
//  - To allow for a throw on any access through user's pointers accessing the value
// In release, we save memory by using a union.
#if defined NDEBUG
    union {
        SLOT_VALUE value;
        SLOT_INDEX_TYPE next_free;
    };
#else
    SLOT_VALUE value;
    SLOT_INDEX_TYPE next_free;
#endif
    bool present;
} SELF;

static void NS(SELF, memory_tracker_empty)(SELF const* slot) {
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_CONTAINER, DC_MEMORY_TRACKER_CAP_NONE, &slot->value,
                          sizeof(SLOT_VALUE));
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_CONTAINER, DC_MEMORY_TRACKER_CAP_WRITE,
                          &slot->next_free, sizeof(SLOT_INDEX_TYPE));
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_CONTAINER, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                          &slot->present, sizeof(bool));
}

static void NS(SELF, set_empty)(SELF* slot, SLOT_INDEX_TYPE next_free) {
    NS(SELF, memory_tracker_empty)(slot);
    slot->present = false;
    slot->next_free = next_free;
}

static void NS(SELF, memory_tracker_present)(SELF const* slot) {
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_CONTAINER, DC_MEMORY_TRACKER_CAP_NONE,
                          &slot->next_free, sizeof(SLOT_INDEX_TYPE));
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_CONTAINER, DC_MEMORY_TRACKER_CAP_WRITE,
                          &slot->value, sizeof(SLOT_VALUE));
    dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_CONTAINER, DC_MEMORY_TRACKER_CAP_READ_WRITE,
                          &slot->present, sizeof(bool));
}

static void NS(SELF, fill)(SELF* slot, SLOT_VALUE value) {
    NS(SELF, memory_tracker_present)(slot);
    slot->present = true;
    slot->value = value;
}

static void NS(SELF, clone_from)(SELF const* from_slot, SELF* to_slot) {
    to_slot->present = from_slot->present;
    if (from_slot->present) {
        NS(SELF, fill)(to_slot, SLOT_VALUE_CLONE(&from_slot->value));
    } else {
        NS(SELF, set_empty)(to_slot, from_slot->next_free);
    }
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    if (self->present) {
        fprintf(stream, "[empty] { next_free: %lu}", (size_t)self->next_free);
    } else {
        fprintf(stream, "[full] ");
        SLOT_VALUE_DEBUG(&self->value, fmt, stream);
    }
}

static void NS(SELF, delete)(SELF* self) {
    if (self->present) {
        SLOT_VALUE_DELETE(&self->value);
    }
}

#undef SLOT_VALUE_DEBUG
#undef SLOT_VALUE_CLONE
#undef SLOT_VALUE_DELETE
#undef SLOT_VALUE
#undef SLOT_INDEX_TYPE

#include <derive-c/core/self/undef.h>
#include <derive-c/core/includes/undef.h>
