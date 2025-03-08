#ifndef CORE
#define CORE

#define NAME_EXPANDED(pre, post) pre##_##post
#define NAME(pre, post) NAME_EXPANDED(pre, post)

#define LIKELY(x) __builtin_expect(!!(x), 1)

#define MAYBE_NULL(T) T*
#define NEVER_NULL(T) T*

#define OUT(ptr) ptr
#define IN(ptr) ptr
#define INOUT(ptr) ptr

#define EXPAND(...) __VA_ARGS__

#define ASSERT(expr)                                                                               \
    if (!(expr))                                                                                     \
        PANIC;

#endif
