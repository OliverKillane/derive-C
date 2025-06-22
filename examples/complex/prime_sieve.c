/**
 * @file
 * @example complex/prime_sieve.c
 * @brief Using a vector in implementing a basic prime sieve.
 */
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <derive-c/macros/iterators.h>

#define SELF sieve_vec
#define T bool
#include <derive-c/structures/vector/template.h>

size_t sqrt_size_t(size_t n) {
    if (n == 0 || n == 1) {
        return n;
    }
    size_t left = 1, right = n, mid, result = 0;
    while (left <= right) {
        mid = left + (right - left) / 2;
        size_t square = mid * mid;

        if (square == n) {
            return mid;
        } else if (square < n) {
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

    ITER_ENUMERATE_LOOP(sieve_vec_iter_const, iter, bool const*, is_not_prime, size_t, index) {
        if (!*is_not_prime) {
            printf("%zu is prime\n", sieve_vec_iter_const_position(&iter) - 1);
        }
    }
}

void compute(sieve_vec* sieve) {
    size_t size = sieve_vec_size(sieve);
    size_t sqrt = sqrt_size_t(size);

    for (size_t factor = 2; factor < sqrt; factor++) {
        for (size_t index = factor * 2; index < size; index += factor) {
            *sieve_vec_write(sieve, index) = true;
        }
    }
}

int main() {
    size_t up_to = 23;
    printf("Listing primes up to: %zu\n", up_to);

    sieve_vec values = sieve_vec_new_with_defaults(up_to, false);
    compute(&values);
    display(&values);
    sieve_vec_delete(&values);

    return 0;
}
