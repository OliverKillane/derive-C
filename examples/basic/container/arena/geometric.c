/// @file
/// @example container/arena/geometric.c
/// @brief Geometric growth arena for efficient reallocation.

#include <stdio.h>

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>
#include <derive-c/utils/for.h>

#define INDEX_BITS 16
#define INITIAL_BLOCK_INDEX_BITS 2
#define VALUE double
#define NAME double_arena
#include <derive-c/container/arena/geometric/template.h>

static void geometric_growth() {
    printf("=== Geometric Growth ===\n");
    DC_SCOPED(double_arena) arena = double_arena_new(stdalloc_get_ref());

    double_arena_index_t indices[20];
    for (size_t i = 0; i < 20; i++) {
        indices[i] = double_arena_insert(&arena, (double)i * 1.5);

        if (i == 1 || i == 3 || i == 7 || i == 15) {
            printf("After %zu insertions: size = %zu\n", i + 1, double_arena_size(&arena));
        }
    }

    printf("\nFinal size: %zu\n", double_arena_size(&arena));

    printf("\nSampled values:\n");
    for (size_t i = 0; i < 20; i += 5) {
        printf("  [%u] = %.1f\n", indices[i].index, *double_arena_read(&arena, indices[i]));
    }
}

struct point {
    int x;
    int y;
};

static void point_debug(struct point const* p, dc_debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "(%d,%d)", p->x, p->y);
}

#define INDEX_BITS 8
#define INITIAL_BLOCK_INDEX_BITS 3
#define VALUE struct point
#define VALUE_DEBUG point_debug
#define NAME point_arena
#include <derive-c/container/arena/geometric/template.h>

static void point_grid() {
    printf("\n=== Point Grid ===\n");
    DC_SCOPED(point_arena) arena = point_arena_new(stdalloc_get_ref());

    point_arena_index_t grid[4][4];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            grid[y][x] = point_arena_insert(&arena, (struct point){x, y});
        }
    }

    printf("Grid size: %zu\n", point_arena_size(&arena));

    struct point* center = point_arena_write(&arena, grid[1][1]);
    center->x = 99;
    center->y = 99;

    printf("\nAll points:\n");
    size_t count = 0;
    DC_FOR_CONST(point_arena, &arena, iter, entry) {
        if (count % 4 == 0)
            printf("  ");
        printf("(%d,%d) ", entry.value->x, entry.value->y);
        if (++count % 4 == 0)
            printf("\n");
    }
}

static void sparse_with_removals() {
    printf("\n=== Sparse Arena (Removals) ===\n");
    DC_SCOPED(double_arena) arena = double_arena_new(stdalloc_get_ref());

    double_arena_index_t indices[20];
    for (size_t i = 0; i < 20; i++) {
        indices[i] = double_arena_insert(&arena, (double)i);
    }

    printf("Initial size: %zu\n", double_arena_size(&arena));

    for (size_t i = 0; i < 20; i += 3) {
        double removed = double_arena_remove(&arena, indices[i]);
        printf("Removed [%u] = %.0f\n", indices[i].index, removed);
    }

    printf("After removals: size = %zu\n", double_arena_size(&arena));

    printf("\nReinserting (reuses slots):\n");
    for (int i = 0; i < 3; i++) {
        double_arena_index_t idx = double_arena_insert(&arena, 100.0 + i);
        printf("  Inserted %.0f at [%u]\n", *double_arena_read(&arena, idx), idx.index);
    }

    printf("Final size: %zu\n", double_arena_size(&arena));
}

int main() {
    geometric_growth();
    point_grid();
    sparse_with_removals();
    return 0;
}
