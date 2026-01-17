#pragma once

// Optimized iterator loop using empty() check
// Uses while-for pattern to check iterator state first, then get item once
#define _DC_FOR_INNER(TYPE, INSTANCE, ITER, ITEM, ITER_TYPE, ITER_GET)                             \
    NS(TYPE, ITER_TYPE) ITER = NS(TYPE, ITER_GET)(INSTANCE);                                       \
    while (!NS(NS(TYPE, ITER_TYPE), empty)(&ITER))                                                 \
        for (int _dc_for_once = 1; _dc_for_once; _dc_for_once = 0)                                 \
            for (NS(NS(TYPE, ITER_TYPE), item) ITEM = NS(NS(TYPE, ITER_TYPE), next)(&ITER);        \
                 !NS(NS(TYPE, ITER_TYPE), empty_item)(&ITEM);)

#define DC_FOR(TYPE, INSTANCE, ITER, ITEM) _DC_FOR_INNER(TYPE, INSTANCE, ITER, ITEM, iter, get_iter)
#define DC_FOR_CONST(TYPE, INSTANCE, ITER, ITEM)                                                   \
    _DC_FOR_INNER(TYPE, INSTANCE, ITER, ITEM, iter_const, get_iter_const)
