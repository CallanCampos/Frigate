// MSL's ANSI C streams: the table of the three console streams, stdin, stdout and stderr, with their buffers, and the
// functions that flush all the streams or the line buffered ones.
#include "ansi_files.h"
#include "console_io.h"

static unsigned char stdin_buff[__console_buffer_size];
static unsigned char stdout_buff[__console_buffer_size];
static unsigned char stderr_buff[__console_buffer_size];

// stdin and stdout are line buffered, stderr isn't buffered. Each stream starts neutral, reads or writes its buffer
// through the console procedures, and has no position procedure
FILE __files[__files_count] = {
    {
        .handle = 0,
        .mode = { .open_mode = __must_exist, .io_mode = __read, .buffer_mode = _IOLBF, .file_kind = __console_file },
        .buffer = stdin_buff,
        .buffer_size = __console_buffer_size,
        .buffer_ptr = stdin_buff,
        .read_proc = __read_console,
        .write_proc = __write_console,
        .close_proc = __close_console,
    },
    {
        .handle = 1,
        .mode = { .open_mode = __must_exist, .io_mode = __write, .buffer_mode = _IOLBF, .file_kind = __console_file },
        .buffer = stdout_buff,
        .buffer_size = __console_buffer_size,
        .buffer_ptr = stdout_buff,
        .read_proc = __read_console,
        .write_proc = __write_console,
        .close_proc = __close_console,
    },
    {
        .handle = 2,
        .mode = { .open_mode = __must_exist, .io_mode = __write, .buffer_mode = _IONBF, .file_kind = __console_file },
        .buffer = stderr_buff,
        .buffer_size = __console_buffer_size,
        .buffer_ptr = stderr_buff,
        .read_proc = __read_console,
        .write_proc = __write_console,
        .close_proc = __close_console,
    },
};

// Flushes the line buffered streams that are being written (before a read from the console, so that a prompt is
// shown), and returns EOF if one of them failed
int __flush_line_buffered_output_files(void) {
    FILE *file = &__files[0];
    int result = 0;
    int i = 1;

    do {
        if (file->mode.file_kind != __closed_file && (file->mode.buffer_mode & _IOLBF) &&
            file->state.io_state == __writing) {
            if (fflush(file)) {
                result = EOF;
            }
        }
        file = (i < __files_count) ? &__files[i++] : NULL;
    } while (file != NULL);
    return result;
}

// Flushes all the open streams, and returns EOF if one of them failed
int __flush_all(void) {
    FILE *file = &__files[0];
    int result = 0;
    int i = 1;

    do {
        if (file->mode.file_kind != __closed_file) {
            if (fflush(file)) {
                result = EOF;
            }
        }
        file = (i < __files_count) ? &__files[i++] : NULL;
    } while (file != NULL);
    return result;
}
