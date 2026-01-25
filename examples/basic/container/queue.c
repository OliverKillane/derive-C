/// @file
/// @example container/queue.c
/// @brief Examples for using queue containers.

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITEM int
#define NAME int_queue
#include <derive-c/container/queue/circular/template.h>

static void example_queue() {
    DC_DEBUG_TRACE;
    DC_SCOPED(int_queue) queue = int_queue_new(stdalloc_get_ref());

    for (int i = 1; i <= 5; i++) {
        int_queue_push_back(&queue, i);
    }

    int_queue_debug(&queue, dc_debug_fmt_new(), stdout);

    while (int_queue_size(&queue) > 0) {
        int value = int_queue_pop_front(&queue);
        printf("Popped: %d\n", value);
    }

    int_queue_debug(&queue, dc_debug_fmt_new(), stdout);
}

#define ITEM int
#define NAME int_deque
#include <derive-c/container/queue/deque/template.h>

static void example_deque() {
    DC_DEBUG_TRACE;
    DC_SCOPED(int_deque) deque = int_deque_new(stdalloc_get_ref());

    int_deque_push_back(&deque, 10);
    int_deque_push_back(&deque, 20);
    int_deque_pop_front(&deque);
}

struct message {
    char* data;
    int length;
};

static void message_delete(struct message* self) { free(self->data); }

static struct message message_clone(struct message const* self) {
    char* cloned_data = (char*)malloc((size_t)self->length);
    memcpy(cloned_data, self->data, (size_t)self->length);
    return (struct message){.data = cloned_data, .length = self->length};
}

static void message_debug(struct message const* self, dc_debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "message@%p { data: \"%.*s\", length: %d }", (void*)self, self->length,
            self->data, self->length);
}

#define ITEM struct message
#define ITEM_DELETE message_delete
#define ITEM_CLONE message_clone
#define ITEM_DEBUG message_debug
#define NAME message_queue
#include <derive-c/container/queue/circular/template.h>

static void example_custom() {
    DC_DEBUG_TRACE;
    DC_SCOPED(message_queue) queue = message_queue_new(stdalloc_get_ref());

    char* data1 = (char*)malloc(5);
    memcpy(data1, "hello", 5); // NOLINT(bugprone-not-null-terminated-result)
    struct message msg1 = {.data = data1, .length = 5};
    message_queue_push_back(&queue, msg1);

    char* data2 = (char*)malloc(5);
    memcpy(data2, "world", 5); // NOLINT(bugprone-not-null-terminated-result)
    struct message msg2 = {.data = data2, .length = 5};
    message_queue_push_back(&queue, msg2);

    message_queue_debug(&queue, dc_debug_fmt_new(), stdout);
}

int main() {
    example_queue();
    example_deque();
    example_custom();
    return 0;
}
