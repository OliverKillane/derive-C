#pragma once

#define FOR_INNER(TYPE, INSTANCE, ITEM, ACCESS)                                                    \
    NS(TYPE, iter) NS(INSTANCE, iter_instance) = NS(TYPE, ACCESS)(&INSTANCE);                      \
    for (NS(NS(TYPE, iter), item) ITEM = NS(NS(TYPE, iter), next)(&NS(INSTANCE, iter_instance));   \
         !NS(NS(TYPE, iter), empty_item)(&ITEM);                                                   \
         ITEM = NS(NS(TYPE, iter), next)(&NS(INSTANCE, iter_instance)))

#define FOR(TYPE, INSTANCE, ITEM) FOR_INNER(TYPE, INSTANCE, ITEM, get_iter)
#define FOR_CONST(TYPE, INSTANCE, ITEM) FOR_INNER(TYPE, INSTANCE, ITEM, get_iter_const)
