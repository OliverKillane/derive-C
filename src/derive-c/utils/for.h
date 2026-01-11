#pragma once

// Need to work out a better way here - do not use instance name, use counter!
#define _DC_FOR_INNER(TYPE, INSTANCE, ITER, ITEM, ITER_TYPE, ITER_GET)                             \
    NS(TYPE, ITER_TYPE) ITER = NS(TYPE, ITER_GET)(INSTANCE);                                       \
    for (NS(NS(TYPE, ITER_TYPE), item) ITEM = NS(NS(TYPE, ITER_TYPE), next)(&ITER);                \
         !NS(NS(TYPE, ITER_TYPE), empty_item)(&ITEM); ITEM = NS(NS(TYPE, ITER_TYPE), next)(&ITER))

#define DC_FOR(TYPE, INSTANCE, ITER, ITEM) _DC_FOR_INNER(TYPE, INSTANCE, ITER, ITEM, iter, get_iter)
#define DC_FOR_CONST(TYPE, INSTANCE, ITER, ITEM)                                                   \
    _DC_FOR_INNER(TYPE, INSTANCE, ITER, ITEM, iter_const, get_iter_const)
