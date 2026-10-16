// MSL's buffered stream I/O: filling a stream's buffer from its read procedure, emptying it through its write
// procedure, and the newline conversions, which do nothing on this platform.
#include "buffer_io.h"

void __convert_from_newlines(unsigned char *buf, size_t *n) {
}

void __convert_to_newlines(unsigned char *buf, size_t *n) {
}

void __prep_buffer(FILE *file) {
    file->buffer_ptr = file->buffer;
    file->buffer_len = file->buffer_size - (file->position & file->buffer_alignment);
    file->buffer_pos = file->position;
}

int __load_buffer(FILE *file, size_t *bytes_loaded, int whole) {
    int ioresult;

    __prep_buffer(file);
    if (whole == 1) {
        file->buffer_len = file->buffer_size;
    }
    ioresult = file->read_proc(file->handle, file->buffer, &file->buffer_len, file->idle_proc);
    if (ioresult == __io_EOF) {
        file->buffer_len = 0;
    }
    if (bytes_loaded != NULL) {
        *bytes_loaded = file->buffer_len;
    }
    if (ioresult != __no_io_error) {
        return ioresult;
    }
    file->position += file->buffer_len;
    if (!file->mode.binary_io) {
        __convert_to_newlines(file->buffer, &file->buffer_len);
    }
    return __no_io_error;
}

int __flush_buffer(FILE *file, size_t *bytes_flushed) {
    size_t buffer_len;
    int ioresult;

    buffer_len = file->buffer_ptr - file->buffer;
    if (buffer_len != 0) {
        file->buffer_len = buffer_len;
        if (!file->mode.binary_io) {
            __convert_from_newlines(file->buffer, &file->buffer_len);
        }
        ioresult = file->write_proc(file->handle, file->buffer, &file->buffer_len, file->idle_proc);
        if (bytes_flushed != NULL) {
            *bytes_flushed = file->buffer_len;
        }
        if (ioresult != __no_io_error) {
            return ioresult;
        }
        file->position += file->buffer_len;
    }
    __prep_buffer(file);
    return __no_io_error;
}
