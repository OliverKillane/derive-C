#pragma once

#define FOR_INNER(TYPE, INSTANCE, ITEM, ACCESS)                                                    \
    NS(TYPE, iter) NS(INSTANCE, iter_instance) = NS(TYPE, ACCESS)(INSTANCE);                       \
    for (NS(NS(TYPE, iter), item) ITEM; (ITEM = NS(NS(TYPE, iter), next)(ITEM));)

#define FOR(TYPE, INSTANCE, ITEM) FOR_INNER(TYPE, INSTANCE, ITEM, get_iter)
#define FOR_CONST(TYPE, INSTANCE, ITEM) FOR_INNER(TYPE, INSTANCE, ITEM, get_iter_const)
