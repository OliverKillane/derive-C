#pragma once

#include <derive-c/core/prelude.h>
#include <stdio.h>

typedef struct {
    FILE* handle;
} dc_file_handle;

DC_PUBLIC static dc_file_handle dc_file_handle_new(char const* path, char const* mode) {
    FILE* handle = fopen(path, mode);
    DC_ASSERT(handle != NULL, "failed to open file: %s", path);
    return (dc_file_handle){.handle = handle};
}

DC_PUBLIC static FILE* dc_file_handle_get(dc_file_handle* self) { return self->handle; }

DC_PUBLIC static void dc_file_handle_delete(dc_file_handle* self) {
    DC_ASSERT(self->handle != NULL, "file handle is null");
    fclose(self->handle);
    self->handle = NULL;
}
