/// @brief An allocator that prints to stdout when it allocates or frees memory.
///  - Takes a specific instance, so we can define different printers for
//     different instances of data structures, only see the allocations we want
//     to.

#include <stdio.h>

#include <derive-c/core.h>
#include <derive-c/panic.h>

#include <derive-c/self/def.h>

#if !defined ALLOC
    #if !defined __clang_analyzer__
        #error "The allocator being debugged must be defined"
    #endif
    #include <derive-c/allocs/null.h>
    #define ALLOC nullalloc
#endif

typedef struct {
    char const* name;
    ALLOC* base;
} SELF;

static SELF NAME(SELF, new)(char const* name, ALLOC* alloc) {
    return (SELF){.name = name, .base = alloc};
}

static void* NAME(SELF, malloc)(SELF* self, size_t size) {
    DEBUG_ASSERT(self);
    void* ptr = NAME(ALLOC, malloc)(self->base, size);
    if (ptr) {
        printf("%s allocated %zu bytes at %p\n", self->name, size, ptr);
    } else {
        printf("%s failed to allocate %zu bytes\n", self->name, size);
    }
    return ptr;
}

static void* NAME(SELF, calloc)(SELF* self, size_t count, size_t size) {
    DEBUG_ASSERT(self);
    void* ptr = NAME(ALLOC, calloc)(self->base, count, size);
    if (ptr) {
        printf("%s allocated %zu bytes at %p\n", self->name, count * size, ptr);
    } else {
        printf("%s failed to allocate %zu bytes\n", self->name, count * size);
    }
    return ptr;
}

static void* NAME(SELF, realloc)(SELF* self, void* ptr, size_t size) {
    DEBUG_ASSERT(self);
    void* new_ptr = NAME(ALLOC, realloc)(self->base, ptr, size);
    if (new_ptr) {
        printf("%s reallocated memory at %p to %zu bytes\n", self->name, new_ptr, size);
    } else {
        printf("%s failed to reallocate memory at %p to %zu bytes\n", self->name, ptr, size);
    }
    return new_ptr;
}

static void NAME(SELF, free)(SELF* self, void* ptr) {
    DEBUG_ASSERT(self);
    printf("%s freeing memory at %p\n", self->name, ptr);
    NAME(ALLOC, free)(self->base, ptr);
}

#undef ALLOC

#include <derive-c/self/undef.h>
