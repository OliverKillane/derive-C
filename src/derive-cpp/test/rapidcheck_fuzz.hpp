
/// A helper for writing RC tests
#define FUZZ(NAME, ...)                                                                            \
    RC_GTEST_PROP(Fuzz, NAME, ()) { Test<__VA_ARGS__>(); }
