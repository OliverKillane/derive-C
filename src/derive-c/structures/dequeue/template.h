/// @brief A simple double ended queue

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined I
    #if !defined __clang_analyzer__
        #error "The contained type must be defined for a vector template"
    #endif

typedef struct {
    int x;
} derive_c_parameter_i;
    #define I derive_c_parameter_i

static void derive_c_parameter_t_delete(derive_c_parameter_i* UNUSED(i)) {}
    #define I_DELETE derive_c_parameter_t_delete
#endif

#if !defined I_DELETE
    #define I_DELETE(value)
#endif

#define ITEM_VECTORS NS(NAME, item_vectors)

#pragma push_macro("ALLOC")

#define T I
#define T_DELETE I_DELETE
#define INTERNAL_NAME ITEM_VECTORS
#include <derive-c/structures/vector/template.h>

#pragma pop_macro("ALLOC")

typedef struct {
    ITEM_VECTORS front;
    ITEM_VECTORS back;
    ALLOC* alloc;
    gdb_marker derive_c_dequeue;
} SELF;

// TODO(oliverkillane): build the rest of the structure

#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/self/undef.h>
