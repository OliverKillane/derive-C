/**
 * @file
 * @example complex/prime_sieve.c
 * @brief Using a vector in implementing a basic prime sieve.
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <derive-c/prelude.h>

#define MAX_UP_TO 100000

// to store values, plus room for realloc
#define CAPACITY 300000
#define NAME bump_alloc
#include <derive-c/alloc/hybridstatic/template.h>

#define ITEM bool
#define ALLOC bump_alloc
#define NAME sieve_vec
#include <derive-c/container/vector/dynamic/template.h>

static size_t sqrt_size_t(size_t n) {
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

static void display(sieve_vec const* sieve, DC_LOGGER* log) {
    sieve_vec_iter_const iter = sieve_vec_get_iter_const(sieve);

    sieve_vec_iter_const_next(&iter); // skip 0
    sieve_vec_iter_const_next(&iter); // skip 1
    size_t index = 2;

    bool const* is_not_prime;
    while ((is_not_prime = sieve_vec_iter_const_next(&iter))) {
        if (!*is_not_prime) {
            DC_LOG(*log, DC_INFO, "%zu is prime", index);
        }
        index++;
    }
}

static void compute(sieve_vec* sieve, DC_LOGGER* log) {
    size_t size = sieve_vec_size(sieve);
    size_t sqrt = sqrt_size_t(size);
    DC_LOG(*log, DC_INFO, "sieve size: %zu, sqrt: %zu", size, sqrt);
    for (size_t factor = 2; factor <= sqrt; factor++) {
        for (size_t index = factor * 2; index < size; index += factor) {
            DC_LOG(*log, DC_DEBUG, "marking %zu as not prime (factor: %zu)", index, factor);
            *sieve_vec_write(sieve, index) = true;
        }
    }
}

static void example_prime_sieve(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);

    dc_cpu_features features = dc_cpu_features_get();
    DC_LOG(log, DC_INFO, "cpu features: %s", DC_DEBUG(dc_cpu_features_debug, &features));

    size_t up_to = 28;
    DC_ASSERT(up_to < MAX_UP_TO);
    DC_LOG(log, DC_INFO, "listing primes up to: %zu", up_to);

    bump_alloc_buffer buf;
    bump_alloc alloc = bump_alloc_new(&buf, stdalloc_get_ref());
    sieve_vec values = sieve_vec_new_with_defaults(up_to, false, &alloc);

    compute(&values, &log);
    display(&values, &log);

    sieve_vec_delete(&values);
}

int main() {
    DC_SCOPED(DC_LOGGER)
    root = NS(DC_LOGGER,
              new_global)((NS(DC_LOGGER, global_config)){.stream = stdout, .ansi_colours = true},
                          (dc_log_id){"prime_sieve"});

    example_prime_sieve(&root);
    return 0;
}
