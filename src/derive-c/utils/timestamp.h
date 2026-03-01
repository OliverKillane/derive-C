#pragma once

#include <derive-c/core/prelude.h>
#include <derive-c/core/debug/fmt.h>
#include <derive-c/test/mock.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define DC_NS_PER_SEC 1000000000ULL

typedef struct {
    uint64_t ns;
} dc_timestamp;

typedef enum {
    DC_TZ_UTC,
} dc_timezone;

typedef struct {
    struct tm tm;
    uint32_t nanos;
    dc_timezone tz;
} dc_datetime;

DC_MOCKABLE(dc_timestamp, dc_timestamp_now, (void)) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (dc_timestamp){.ns = (uint64_t)ts.tv_sec * DC_NS_PER_SEC + (uint64_t)ts.tv_nsec};
}

DC_PUBLIC static dc_datetime dc_datetime_from_timestamp(dc_timestamp ts, dc_timezone tz) {
    time_t secs = (time_t)(ts.ns / DC_NS_PER_SEC);
    uint32_t nanos = (uint32_t)(ts.ns % DC_NS_PER_SEC);

    dc_datetime dt;
    dt.nanos = nanos;
    dt.tz = tz;

    switch (tz) {
    case DC_TZ_UTC:
        gmtime_r(&secs, &dt.tm);
        break;
    }

    return dt;
}

DC_PUBLIC static dc_datetime dc_datetime_now_utc(void) {
    return dc_datetime_from_timestamp(dc_timestamp_now(), DC_TZ_UTC);
}

DC_PUBLIC static dc_timestamp dc_datetime_to_timestamp(dc_datetime const* dt) {
    struct tm tm_copy = dt->tm;
    time_t secs;

    switch (dt->tz) {
    case DC_TZ_UTC:
        secs = timegm(&tm_copy);
        break;
    }

    return (dc_timestamp){.ns = (uint64_t)secs * DC_NS_PER_SEC + (uint64_t)dt->nanos};
}

DC_PUBLIC static void dc_datetime_format(dc_datetime const* dt, FILE* stream) {
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &dt->tm);

    char const* tz_suffix;
    switch (dt->tz) {
    case DC_TZ_UTC:
        tz_suffix = "Z";
        break;
    }

    fprintf(stream, "%s.%09u%s", buf, dt->nanos, tz_suffix);
}

DC_PUBLIC static void dc_datetime_debug(dc_datetime const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, "dc_datetime {\n");
    fmt = dc_debug_fmt_scope_begin(fmt);

    dc_debug_fmt_print(fmt, stream, "year: %d,\n", self->tm.tm_year + 1900);
    dc_debug_fmt_print(fmt, stream, "month: %d,\n", self->tm.tm_mon + 1);
    dc_debug_fmt_print(fmt, stream, "day: %d,\n", self->tm.tm_mday);
    dc_debug_fmt_print(fmt, stream, "hour: %d,\n", self->tm.tm_hour);
    dc_debug_fmt_print(fmt, stream, "min: %d,\n", self->tm.tm_min);
    dc_debug_fmt_print(fmt, stream, "sec: %d,\n", self->tm.tm_sec);
    dc_debug_fmt_print(fmt, stream, "nanos: %u,\n", self->nanos);
    dc_debug_fmt_print(fmt, stream, "tz: UTC,\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}
