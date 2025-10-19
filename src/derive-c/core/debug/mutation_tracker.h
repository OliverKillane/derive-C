/// @brief tracks a specific version of a value, so that this can be compared later to check
/// modification For example, checking iterator invalidation in debug mode.

#pragma once
#include <derive-c/core/prelude.h>
#include <stddef.h>

#if defined NDEBUG

ZERO_SIZED(mutation_tracker);
ZERO_SIZED(mutation_version);

static mutation_tracker mutation_tracker_new() { return (mutation_tracker){}; }

static void mutation_tracker_mutate(mutation_tracker* self) { (void)self; }

static mutation_version mutation_tracker_get(mutation_tracker const* self) {
    (void)self;
    return (mutation_version){};
}

static void mutation_version_check(mutation_version const* self) { (void)self; }

#else

typedef struct {
    size_t count;
} mutation_tracker;

typedef struct {
    mutation_tracker const* tracker;
    size_t const count;
} mutation_version;

static mutation_tracker mutation_tracker_new() { return (mutation_tracker){.count = 0}; }

static void mutation_tracker_mutate(mutation_tracker* self) {
    ASSERT(self);
    self->count++;
}

static mutation_version mutation_tracker_get(mutation_tracker const* self) {
    ASSERT(self);
    return (mutation_version){.tracker = self, .count = self->count};
}

/// Throw on the tracker version not matching.
/// For example an iterator over a vector may store the version from it's creation, so that on
/// access it can check it was not invalidated by mutation to the vector.
static void mutation_version_check(mutation_version const* self) {
    ASSERT(self->count == self->tracker->count, "No mutations to the tracker's data structure were "
                                                "expected... however it has been mutated");
}
#endif