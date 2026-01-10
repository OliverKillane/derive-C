#pragma once

#if defined __clang_analyzer__
    #define DC_PLACEHOLDERS
#endif

#if defined __clang_daemon__
    #define DC_PLACEHOLDERS
#endif
