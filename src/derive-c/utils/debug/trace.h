#pragma once

#include <stdio.h>

#define DC_DEBUG_TRACE                                                                             \
    fprintf(stdout, "[%s@%s:%d] entering function\n", __func__, __FILE__, __LINE__)
