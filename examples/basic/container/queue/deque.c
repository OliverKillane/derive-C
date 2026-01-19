/// @file
/// @example container/queue/deque.c
/// @brief Examples for using double-ended queues.

#include <stdio.h>

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>
#include <derive-c/utils/for.h>

// Basic integer deque
#define ITEM int
#define NAME int_deque
#include <derive-c/container/queue/deque/template.h>

static void basic_deque_operations() {
    printf("=== Basic Deque Operations ===\n");
    int_deque dq = int_deque_new_with_capacity(8, stdalloc_get_ref());

    // Push to back
    printf("Pushing to back: 10, 20, 30\n");
    int_deque_push_back(&dq, 10);
    int_deque_push_back(&dq, 20);
    int_deque_push_back(&dq, 30);

    // Push to front
    printf("Pushing to front: 5, 3, 1\n");
    int_deque_push_front(&dq, 5);
    int_deque_push_front(&dq, 3);
    int_deque_push_front(&dq, 1);

    printf("Deque size: %zu\n", int_deque_size(&dq));
    printf("Contents (front to back): ");
    DC_FOR_CONST(int_deque, &dq, iter1, item) { printf("%d ", *item); }
    printf("\n");

    // Access elements at both ends
    int_deque_iter_const peek_iter = int_deque_get_iter_const(&dq);
    int const* first = int_deque_iter_const_next(&peek_iter);
    printf("Front element: %d\n", *first);

    // Get last element by iterating to end
    int const* last = NULL;
    int_deque_iter_const last_iter = int_deque_get_iter_const(&dq);
    while (!int_deque_iter_const_empty(&last_iter)) {
        last = int_deque_iter_const_next(&last_iter);
    }
    printf("Back element: %d\n", *(last - 1)); // Last item we saw

    // Pop from both ends
    printf("\nPopping from front: %d\n", int_deque_pop_front(&dq));
    printf("Popping from back: %d\n", int_deque_pop_back(&dq));

    printf("After popping, contents: ");
    DC_FOR_CONST(int_deque, &dq, iter2, item) { printf("%d ", *item); }
    printf("\n");

    int_deque_debug(&dq, dc_debug_fmt_new(), stdout);
    printf("\n");

    int_deque_delete(&dq);
}

// Simulate a work queue where tasks can be added from both ends
struct task {
    int priority;
    char name[32];
};

static void task_debug(struct task const* t, dc_debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "Task{pri:%d, name:\"%s\"}", t->priority, t->name);
}

#define ITEM struct task
#define ITEM_DEBUG task_debug
#define NAME task_queue
#include <derive-c/container/queue/deque/template.h>

static void priority_queue_example() {
    printf("\n=== Priority Queue Simulation ===\n");
    task_queue tq = task_queue_new(stdalloc_get_ref());

    // Normal tasks go to the back
    printf("Adding normal priority tasks to back:\n");
    task_queue_push_back(&tq, (struct task){.priority = 1, .name = "Process data"});
    task_queue_push_back(&tq, (struct task){.priority = 1, .name = "Update logs"});
    task_queue_push_back(&tq, (struct task){.priority = 1, .name = "Send report"});

    // High priority tasks go to the front
    printf("Adding high priority task to front:\n");
    task_queue_push_front(&tq, (struct task){.priority = 9, .name = "URGENT FIX"});

    printf("\nTask queue (%zu tasks):\n", task_queue_size(&tq));
    DC_FOR_CONST(task_queue, &tq, iter, task) {
        printf("  [pri:%d] %s\n", task->priority, task->name);
    }

    // Process tasks from front
    printf("\nProcessing tasks:\n");
    while (task_queue_size(&tq) > 0) {
        struct task t = task_queue_pop_front(&tq);
        printf("  Executing [pri:%d]: %s\n", t.priority, t.name);
    }

    printf("Queue empty: %s\n\n", task_queue_size(&tq) == 0 ? "yes" : "no");

    task_queue_delete(&tq);
}

// Palindrome checker using deque
#define ITEM char
#define NAME char_deque
#include <derive-c/container/queue/deque/template.h>

static bool is_palindrome(const char* str) {
    char_deque dq = char_deque_new(stdalloc_get_ref());

    // Load string into deque
    for (const char* c = str; *c != '\0'; c++) {
        if (*c != ' ') { // Ignore spaces
            char_deque_push_back(&dq, *c);
        }
    }

    // Check palindrome by comparing front and back
    bool is_pal = true;
    while (char_deque_size(&dq) > 1) {
        char front = char_deque_pop_front(&dq);
        char back = char_deque_pop_back(&dq);
        if (front != back) {
            is_pal = false;
            break;
        }
    }

    char_deque_delete(&dq);
    return is_pal;
}

static void palindrome_example() {
    printf("\n=== Palindrome Checker ===\n");
    const char* tests[] = {"racecar", "hello", "a man a plan a canal panama",
                           "deque",   "noon",  "abcba"};

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        printf("\"%s\" is %sa palindrome\n", tests[i], is_palindrome(tests[i]) ? "" : "not ");
    }
}

// Sliding window with deque
static void sliding_window_example() {
    printf("\n=== Sliding Window Maximum ===\n");
    int_deque window = int_deque_new(stdalloc_get_ref());

    int values[] = {4, 2, 7, 1, 9, 3, 8, 5};
    size_t window_size = 3;
    size_t n = sizeof(values) / sizeof(values[0]);

    printf("Finding maximum in sliding window of size %zu:\n", window_size);
    printf("Values: ");
    for (size_t i = 0; i < n; i++) {
        printf("%d ", values[i]);
    }
    printf("\n\n");

    for (size_t i = 0; i < n; i++) {
        // Add new element
        int_deque_push_back(&window, values[i]);

        // Remove old elements if window is too large
        if (int_deque_size(&window) > window_size) {
            int_deque_pop_front(&window);
        }

        // Find maximum in current window
        if (int_deque_size(&window) == window_size) {
            int_deque_iter_const max_iter = int_deque_get_iter_const(&window);
            int const* first_val = int_deque_iter_const_next(&max_iter);
            int max = *first_val;

            DC_FOR_CONST(int_deque, &window, iter_val, val) {
                if (*val > max)
                    max = *val;
            }

            printf("Window [");
            DC_FOR_CONST(int_deque, &window, iter_print, val) { printf("%d ", *val); }
            printf("] -> max: %d\n", max);
        }
    }

    int_deque_delete(&window);
}

int main() {
    basic_deque_operations();
    priority_queue_example();
    palindrome_example();
    sliding_window_example();
    return 0;
}
