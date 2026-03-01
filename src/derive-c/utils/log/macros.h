#pragma once

#define DC_LOGGER_LOG(LOGGER_TYPE, LOGGER, LEVEL, ...)                                             \
    NS(LOGGER_TYPE, log)(&LOGGER, DC_LOCATION, LEVEL, __VA_ARGS__);

#define DC_LOG_NAME_FMT(FMT_STRING, ...)                                                           \
    ({                                                                                             \
        dc_log_id id;                                                                              \
        snprintf(id.name, DC_LOG_ID_LENGTH, FMT_STRING, __VA_ARGS__);                              \
        id;                                                                                        \
    })

#define DC_LOGGER_FROM_PARENT(LOGGER_TYPE, PARENT_LOGGER_PTR, FMT_STRING, ...)                     \
    NS(LOGGER_TYPE, from_parent)(PARENT_LOGGER_PTR, DC_LOG_NAME_FMT(FMT_STRING, __VA_ARGS__))
