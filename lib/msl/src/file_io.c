// MSL's stream control: closing and flushing a stream, and the case blind string comparison that the library keeps
// here.
#include "file_io.h"
#include "ansi_files.h"
#include "buffer_io.h"
#include <ctype.h>

int fclose(FILE *file) {
    int flush_result, close_result;

    if (file == NULL) {
        return EOF;
    }
    if (file->mode.file_kind == __closed_file) {
        return 0;
    }
    flush_result = fflush(file);
    close_result = file->close_proc(file->handle);
    file->mode.file_kind = __closed_file;
    file->handle = 0;
    // fopen and setvbuf aren't linked, so no stream has a buffer to free
    if (file->state.free_buffer) {
        return EOF;
    }
    return (flush_result || close_result) ? EOF : 0;
}

int fflush(FILE *file) {
    if (file == NULL) {
        return __flush_all();
    }
    if (file->state.error || file->mode.file_kind == __closed_file) {
        return EOF;
    }
    if (file->mode.io_mode == __read) {
        return 0;
    }
    if (file->state.io_state >= __rereading) {
        file->state.io_state = __reading;
    }
    if (file->state.io_state == __reading) {
        file->buffer_len = 0;
    }
    if (file->state.io_state != __writing) {
        file->state.io_state = __neutral;
        return 0;
    }
    if (__flush_buffer(file, NULL) != __no_io_error) {
        file->state.error = 1;
        file->buffer_len = 0;
        return EOF;
    }
    file->state.io_state = __neutral;
    file->position = 0;
    file->buffer_len = 0;
    return 0;
}

int __msl_strnicmp(const char *str1, const char *str2, int n) {
    int i;
    unsigned char c1, c2;

    for (i = 0; i < n; i++) {
        c1 = tolower(*str1++);
        c2 = tolower(*str2++);
        if (c1 < c2) {
            return -1;
        }
        if (c1 > c2) {
            return 1;
        }
        if (c1 == 0) {
            return 0;
        }
    }
    return 0;
}
