
#define ITER_LOOP(ITER_TYPE, ITER_NAME, VALUE_TYPE, VALUE_NAME)                                    \
    for (VALUE_TYPE VALUE_NAME = ITER_TYPE##_next(&ITER_NAME); VALUE_NAME;                         \
         VALUE_NAME = ITER_TYPE##_next(&ITER_NAME))

#define ITER_ENUMERATE_LOOP(ITER_TYPE, ITER_NAME, VALUE_TYPE, VALUE_NAME, COUNTER_TYPE,            \
                            COUNTER_NAME)                                                          \
    COUNTER_TYPE COUNTER_NAME = 0;                                                                 \
    for (VALUE_TYPE VALUE_NAME = ITER_TYPE##_next(&ITER_NAME); VALUE_NAME;                         \
         VALUE_NAME = ITER_TYPE##_next(&ITER_NAME), COUNTER_NAME++)
