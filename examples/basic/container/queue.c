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

static void example_queue(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(int_queue) queue = int_queue_new(stdalloc_get_ref());

    DC_LOG(log, DC_INFO, "pushing 1-5 to queue");
    for (int i = 1; i <= 5; i++) {
        int_queue_push_back(&queue, i);
    }

    DC_LOG(log, DC_INFO, "queue: %s", DC_DEBUG(int_queue_debug, &queue));

    while (int_queue_size(&queue) > 0) {
        int value = int_queue_pop_front(&queue);
        DC_LOG(log, DC_INFO, "popped: %d", value);
    }

    DC_LOG(log, DC_INFO, "queue after pops: %s", DC_DEBUG(int_queue_debug, &queue));
}

#define ITEM int
#define NAME int_deque
#include <derive-c/container/queue/deque/template.h>

static void example_deque(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(int_deque) deque = int_deque_new(stdalloc_get_ref());

    DC_LOG(log, DC_INFO, "push_back 10, push_back 20, pop_front");
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

static void example_custom(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(message_queue) queue = message_queue_new(stdalloc_get_ref());

    DC_LOG(log, DC_INFO, "pushing two messages");
    char* data1 = (char*)malloc(5);
    memcpy(data1, "hello", 5); // NOLINT(bugprone-not-null-terminated-result)
    struct message msg1 = {.data = data1, .length = 5};
    message_queue_push_back(&queue, msg1);

    char* data2 = (char*)malloc(5);
    memcpy(data2, "world", 5); // NOLINT(bugprone-not-null-terminated-result)
    struct message msg2 = {.data = data2, .length = 5};
    message_queue_push_back(&queue, msg2);

    DC_LOG(log, DC_INFO, "queue: %s", DC_DEBUG(message_queue_debug, &queue));
}

int main() {
    DC_SCOPED(DC_LOGGER)
    root = NS(DC_LOGGER,
              new_global)((NS(DC_LOGGER, global_config)){.stream = stdout, .ansi_colours = true},
                          (dc_log_id){"queue"});

    example_queue(&root);
    example_deque(&root);
    example_custom(&root);
    return 0;
}
