
#if defined PANIC
    #error "Define panic as gtest first so all includes use this"
#endif

// Format strings not supported.
#define PANIC(fmt, ...) RC_FAIL(fmt)
