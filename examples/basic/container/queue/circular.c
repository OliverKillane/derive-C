
#include <derive-c/core/prelude.h>

#define CAPACITY 300000
#define NAME bump_alloc
#include <derive-c/alloc/staticbump/template.h>

#define ITEM int
#define ALLOC bump_alloc
#define NAME int_queue
#include <derive-c/container/queue/circular/template.h>

void basic_example() {
    bump_alloc_buffer buffer = {};
    bump_alloc alloc = bump_alloc_new(&buffer);

    int_queue q = int_queue_new_with_capacity_for(8, &alloc);

    int_queue_push_back(&q, 1);
    int_queue_push_back(&q, 2);
    int_queue_push_back(&q, 3);

    ASSERT(int_queue_size(&q) == 3);

    int_queue_debug(&q, debug_fmt_new(), stdout);

    ASSERT(int_queue_pop_front(&q) == 1);

    ASSERT(int_queue_pop_back(&q) == 3);

    int_queue_debug(&q, debug_fmt_new(), stdout);

    int_queue_delete(&q);
}

int main() { basic_example(); }
