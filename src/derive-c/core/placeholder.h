#pragma once

#if defined __clang_analyzer__
    #define PLACEHOLDERS
#endif

#if defined __clang_daemon__
    #define PLACEHOLDERS
#endif
