#include <gtest/gtest.h>
#include <derive-c/alloc/std.h>

#define ITEM int
#define NAME TestDeque
#include <derive-c/container/queue/deque/template.h>

TEST(DequeIteratorDebug, SimpleCase) {
    TestDeque deque = TestDeque_new(stdalloc_get_ref());

    // Add some elements
    TestDeque_push_back(&deque, 1);
    TestDeque_push_back(&deque, 2);
    TestDeque_push_back(&deque, 3);

    printf("Deque size: %zu\n", TestDeque_size(&deque));

    // Get iterator
    TestDeque_iter_const iter = TestDeque_get_iter_const(&deque);
    printf("Iterator front_size: %zu, total_size: %zu\n", iter.front_size, iter.total_size);

    // Iterate and print
    int expected[] = {1, 2, 3};
    int idx = 0;
    while (!TestDeque_iter_const_empty(&iter)) {
        int const* item = TestDeque_iter_const_next(&iter);
        printf("pos=%zu, item=%p", iter.pos - 1, (void*)item);
        if (item) {
            printf(", value=%d (expected %d)\n", *item, expected[idx]);
            ASSERT_EQ(*item, expected[idx]);
        } else {
            printf(", NULL (should not happen!)\n");
            FAIL() << "Iterator returned NULL unexpectedly";
        }
        idx++;
    }

    ASSERT_EQ(idx, 3);
    TestDeque_delete(&deque);
}

TEST(DequeIteratorDebug, WithRebalancing) {
    TestDeque deque = TestDeque_new(stdalloc_get_ref());

    // Push to back multiple times to trigger potential rebalancing
    for (int i = 0; i < 10; i++) {
        TestDeque_push_back(&deque, i);
    }

    printf("After push_back: size=%zu\n", TestDeque_size(&deque));

    // Get iterator
    TestDeque_iter_const iter = TestDeque_get_iter_const(&deque);
    printf("Iterator front_size: %zu, total_size: %zu\n", iter.front_size, iter.total_size);

    // Iterate
    int idx = 0;
    while (!TestDeque_iter_const_empty(&iter)) {
        int const* item = TestDeque_iter_const_next(&iter);
        printf("pos=%zu, item=%p", iter.pos - 1, (void*)item);
        if (item) {
            printf(", value=%d (expected %d)\n", *item, idx);
            ASSERT_EQ(*item, idx);
        } else {
            printf(", NULL at pos %zu\n", iter.pos - 1);
            FAIL() << "Iterator returned NULL at position " << (iter.pos - 1);
        }
        idx++;
    }

    ASSERT_EQ(idx, 10);
    TestDeque_delete(&deque);
}

TEST(DequeIteratorDebug, MixedOperations) {
    TestDeque deque = TestDeque_new(stdalloc_get_ref());

    // Mix of front and back operations
    TestDeque_push_back(&deque, 1);
    TestDeque_push_front(&deque, 0);
    TestDeque_push_back(&deque, 2);
    TestDeque_push_front(&deque, -1);

    printf("After mixed ops: size=%zu\n", TestDeque_size(&deque));

    // Get iterator
    TestDeque_iter_const iter = TestDeque_get_iter_const(&deque);
    printf("Iterator front_size: %zu, total_size: %zu\n", iter.front_size, iter.total_size);

    // Expected: -1, 0, 1, 2
    int expected[] = {-1, 0, 1, 2};
    int idx = 0;
    while (!TestDeque_iter_const_empty(&iter)) {
        int const* item = TestDeque_iter_const_next(&iter);
        printf("pos=%zu, item=%p", iter.pos - 1, (void*)item);
        if (item) {
            printf(", value=%d (expected %d)\n", *item, expected[idx]);
            ASSERT_EQ(*item, expected[idx]);
        } else {
            printf(", NULL\n");
            FAIL() << "Iterator returned NULL at position " << (iter.pos - 1);
        }
        idx++;
    }

    ASSERT_EQ(idx, 4);
    TestDeque_delete(&deque);
}
