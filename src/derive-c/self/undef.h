#undef SELF

// If we are using the stack, pop the deepest name.
#if defined INTERNAL_NAME_4
#   undef INTERNAL_NAME_4
#elif defined INTERNAL_NAME_3
#   undef INTERNAL_NAME_3
#elif defined INTERNAL_NAME_2
#   undef INTERNAL_NAME_2
#elif defined INTERNAL_NAME_1
#   undef INTERNAL_NAME_1
#elif defined INTERNAL_NAME_0
#   undef INTERNAL_NAME_0
#endif
