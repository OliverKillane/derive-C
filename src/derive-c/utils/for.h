#pragma once

#define FOR_INNER(TYPE, INSTANCE, ITEM, ITER_TYPE, ITER_GET)                                       \
    NS(TYPE, ITER_TYPE) NS(INSTANCE, iter_instance) = NS(TYPE, ITER_GET)(&INSTANCE);               \
    for (NS(NS(TYPE, ITER_TYPE), item)                                                             \
             ITEM = NS(NS(TYPE, ITER_TYPE), next)(&NS(INSTANCE, iter_instance));                   \
         !NS(NS(TYPE, ITER_TYPE), empty_item)(&ITEM);                                              \
         ITEM = NS(NS(TYPE, ITER_TYPE), next)(&NS(INSTANCE, iter_instance)))

#define FOR(TYPE, INSTANCE, ITEM) FOR_INNER(TYPE, INSTANCE, ITEM, iter, get_iter)
#define FOR_CONST(TYPE, INSTANCE, ITEM) FOR_INNER(TYPE, INSTANCE, ITEM, iter_const, get_iter_const)
