/// @file
/// @example container/vector/static.c
/// @brief Static vectors as fixed-capacity containers.

#include <stdio.h>

#include <derive-c/prelude.h>
#include <derive-c/utils/for.h>

#define ITEM char
#define CAPACITY 16
#define NAME vec
#include <derive-c/container/vector/static/template.h>

static void basic_usage() {
    printf("=== Basic Static Vector ===\n");
    DC_SCOPED(vec) v = vec_new();

    for (char c = 'A'; c <= 'J'; c++) {
        vec_push(&v, c);
    }

    printf("Size: %u/%u\n", vec_size(&v), (uint8_t)16);

    printf("Contents: ");
    for (uint8_t i = 0; i < vec_size(&v); i++) {
        printf("%c", *vec_read(&v, i));
    }
    printf("\n");

    char popped = vec_pop(&v);
    printf("Popped: '%c'\n", popped);

    vec_debug(&v, dc_debug_fmt_new(), stdout);
}

#define ITEM int
#define CAPACITY 8
#define NAME stack
#include <derive-c/container/vector/static/template.h>

static void stack_operations() {
    printf("\n=== Stack (LIFO) ===\n");
    DC_SCOPED(stack) stk = stack_new();

    for (int i = 1; i <= 5; i++) {
        stack_push(&stk, i * 10);
    }

    printf("Top: %d\n", *stack_read(&stk, stack_size(&stk) - 1));

    printf("Popping:");
    while (stack_size(&stk) > 0) {
        printf(" %d", stack_pop(&stk));
    }
    printf("\n");
}

#define ITEM double
#define CAPACITY 4
#define NAME ring
#include <derive-c/container/vector/static/template.h>

static void ring_buffer() {
    printf("\n=== Ring Buffer ===\n");
    DC_SCOPED(ring) buf = ring_new();

    double readings[] = {23.5, 24.1, 23.8, 25.0, 24.5};

    for (size_t i = 0; i < sizeof(readings) / sizeof(readings[0]); i++) {
        if (ring_size(&buf) >= 4) {
            double discarded = *ring_read(&buf, 0);
            for (uint8_t j = 0; j < ring_size(&buf) - 1; j++) {
                *ring_write(&buf, j) = *ring_read(&buf, j + 1);
            }
            ring_remove_at(&buf, ring_size(&buf) - 1, 1);
            printf("Discarded %.1f, ", discarded);
        }

        ring_push(&buf, readings[i]);
        printf("buffer:");
        for (uint8_t j = 0; j < ring_size(&buf); j++) {
            printf(" %.1f", *ring_read(&buf, j));
        }
        printf("\n");
    }

    double sum = 0.0;
    for (uint8_t i = 0; i < ring_size(&buf); i++) {
        sum += *ring_read(&buf, i);
    }
    printf("Average: %.2f\n", sum / ring_size(&buf));
}

int main() {
    basic_usage();
    stack_operations();
    ring_buffer();
    return 0;
}
