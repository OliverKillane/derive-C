/// @brief An allocator that prints to stdout when it allocates or frees memory.
///  - Takes a specific instance, so we can define different printers for
//     different instances of data structures, only see the allocations we want
//     to.

#include <stdio.h>

#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

typedef struct {
    char const* name;
    ALLOC* base;
} SELF;

static SELF NS(SELF, new)(char const* name, ALLOC* alloc) {
    return (SELF){.name = name, .base = alloc};
}

static void* NS(SELF, malloc)(SELF* self, size_t size) {
    DEBUG_ASSERT(self);
    void* ptr = NS(ALLOC, malloc)(self->base, size);
    if (ptr) {
        printf("%s allocated %zu bytes at %p\n", self->name, size, ptr);
    } else {
        printf("%s failed to allocate %zu bytes\n", self->name, size);
    }
    return ptr;
}

static void* NS(SELF, calloc)(SELF* self, size_t count, size_t size) {
    DEBUG_ASSERT(self);
    void* ptr = NS(ALLOC, calloc)(self->base, count, size);
    if (ptr) {
        printf("%s allocated %zu bytes at %p\n", self->name, count * size, ptr);
    } else {
        printf("%s failed to allocate %zu bytes\n", self->name, count * size);
    }
    return ptr;
}

static void* NS(SELF, realloc)(SELF* self, void* ptr, size_t size) {
    DEBUG_ASSERT(self);
    void* new_ptr = NS(ALLOC, realloc)(self->base, ptr, size);
    if (new_ptr) {
        printf("%s reallocated memory at %p to %zu bytes\n", self->name, new_ptr, size);
    } else {
        printf("%s failed to reallocate memory at %p to %zu bytes\n", self->name, ptr, size);
    }
    return new_ptr;
}

static void NS(SELF, free)(SELF* self, void* ptr) {
    DEBUG_ASSERT(self);
    printf("%s freeing memory at %p\n", self->name, ptr);
    NS(ALLOC, free)(self->base, ptr);
}

#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/self/undef.h>