#pragma once

#include <derive-c/utils/log/trait.h>
#include <derive-c/core/require.h>
#include <derive-c/utils/log/macros.h>

#if defined DC_LOG_HEADER
    #include DC_LOG_HEADER
#else
    #include <derive-c/utils/log/loggers/null.h>
#endif

#if !defined(DC_LOGGER)
    #error                                                                                         \
        "The provided logging implementation does not define DC_LOGGER, which derive-c needs to call"
#endif

DC_TRAIT_LOGGER(DC_LOGGER);

#define DC_LOG(...) DC_LOGGER_LOG(DC_LOGGER, __VA_ARGS__)
#define DC_LOGGER_NEW(...) DC_LOGGER_FROM_PARENT(DC_LOGGER, __VA_ARGS__)
