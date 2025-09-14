/**
 * @file
 * @example complex/prime_sieve.c
 * @brief Using a vector in implementing a basic prime sieve.
 */
// #include <errno.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_UP_TO 100000

// to store values, plus room for realloc
#define CAPACITY 300000
#define NAME bump_alloc
#include <derive-c/allocs/staticbump/template.h>

#define ITEM bool
#define ALLOC bump_alloc
#define NAME sieve_vec
#include <derive-c/structures/vector/template.h>

size_t sqrt_size_t(size_t n) {
    if (n == 0 || n == 1) {
        return n;
    }
    size_t left = 1;
    size_t right = n;
    size_t mid;
    size_t result = 0;
    while (left <= right) {
        mid = left + (right - left) / 2;
        size_t square = mid * mid;

        if (square == n) {
            return mid;
        }

        if (square < n) {
            result = mid; // Store the last valid result
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return result;
}

void display(sieve_vec const* sieve) {
    sieve_vec_iter_const iter = sieve_vec_get_iter_const(sieve);

    sieve_vec_iter_const_next(&iter); // skip 0
    sieve_vec_iter_const_next(&iter); // skip 1
    size_t index = 2;

    bool const* is_not_prime;
    while ((is_not_prime = sieve_vec_iter_const_next(&iter))) {
        if (!*is_not_prime) {
            printf("%zu is prime\n", index);
        }
        index++;
    }
}

void compute(sieve_vec* sieve) {
    size_t size = sieve_vec_size(sieve);
    size_t sqrt = sqrt_size_t(size);
    printf("Sieve size: %zu, sqrt: %zu\n", size, sqrt);
    for (size_t factor = 2; factor <= sqrt; factor++) {
        for (size_t index = factor * 2; index < size; index += factor) {
            printf("Marking %zu as not prime (factor: %zu)\n", index, factor);
            *sieve_vec_write(sieve, index) = true;
        }
    }
}

int main() {
    size_t up_to = 28;
    assert(up_to < MAX_UP_TO);
    printf("Listing primes up to: %zu\n", up_to);
    bump_alloc alloc = bump_alloc_new();
    sieve_vec values = sieve_vec_new_with_defaults(up_to, false, &alloc);
    compute(&values);
    display(&values);
    sieve_vec_delete(&values);
}
