// MSL's block reads: fread, which holds the stream's critical region, and the reader itself. Black 2 doesn't link
// fwrite.
#include "buffer_io.h"
#include "critical_regions.h"
#include <wchar.h>

static size_t __fread(void *ptr, size_t memb_size, size_t num_memb, FILE *file);

size_t fread(void *ptr, size_t memb_size, size_t num_memb, FILE *file) {
    size_t retval;
    int region = (file == stdin) ? stdin_access : files_access;

    __begin_critical_region(region);
    retval = __fread(ptr, memb_size, num_memb, file);
    __end_critical_region(region);
    return retval;
}

// Reads what the stream has pushed back with ungetc first, then through the buffer, and last, for a binary stream that
// isn't fully buffered, straight into the caller's memory
static size_t __fread(void *ptr, size_t memb_size, size_t num_memb, FILE *file) {
    unsigned char *read_ptr;
    size_t bytes_to_go, bytes_read, num_bytes;
    int ioresult, always_buffer;

    if (fwide(file, 0) == 0) {
        fwide(file, -1);
    }
    bytes_to_go = memb_size * num_memb;
    if (bytes_to_go == 0 || file->state.error || file->mode.file_kind == __closed_file) {
        return 0;
    }
    always_buffer = !file->mode.binary_io || file->mode.buffer_mode == _IOFBF;

    if (file->state.io_state == __neutral) {
        if (file->mode.io_mode & __read) {
            file->state.io_state = __reading;
            file->buffer_len = 0;
        }
    }
    if (file->state.io_state < __reading) {
        file->state.error = 1;
        file->buffer_len = 0;
        return 0;
    }
    if (file->mode.buffer_mode & _IOLBF) {
        if (__flush_line_buffered_output_files()) {
            file->state.error = 1;
            file->buffer_len = 0;
            return 0;
        }
    }

    read_ptr = ptr;
    bytes_read = 0;

    if (bytes_to_go && file->state.io_state >= __rereading) {
        do {
            if (fwide(file, 0) == 1) {
                *(wchar_t *)read_ptr = file->ungetc_wide_buffer[file->state.io_state - __rereading];
                read_ptr += 2;
                bytes_read += 2;
                bytes_to_go -= 2;
            } else {
                *read_ptr++ = file->ungetc_buffer[file->state.io_state - __rereading];
                bytes_read++;
                bytes_to_go--;
            }
            file->state.io_state--;
        } while (bytes_to_go && file->state.io_state >= __rereading);

        if (file->state.io_state == __reading) {
            file->buffer_len = file->saved_buffer_len;
        }
    }

    if (bytes_to_go && (file->buffer_len || always_buffer)) {
        do {
            if (file->buffer_len == 0) {
                ioresult = __load_buffer(file, NULL, 0);
                if (ioresult != __no_io_error) {
                    // Each outcome empties the buffer; the compiler merges the two stores into one after the branches
                    if (ioresult == __io_error) {
                        file->state.error = 1;
                        file->buffer_len = 0;
                    } else {
                        file->state.io_state = __neutral;
                        file->state.eof = 1;
                        file->buffer_len = 0;
                    }
                    bytes_to_go = 0;
                    break;
                }
            }
            num_bytes = file->buffer_len;
            if (num_bytes > bytes_to_go) {
                num_bytes = bytes_to_go;
            }
            memcpy(read_ptr, file->buffer_ptr, num_bytes);
            read_ptr += num_bytes;
            bytes_read += num_bytes;
            bytes_to_go -= num_bytes;
            file->buffer_ptr += num_bytes;
            file->buffer_len -= num_bytes;
        } while (bytes_to_go && always_buffer);
    }

    // The rest of a binary stream that isn't fully buffered is read straight into the caller's memory, through the
    // stream's buffer pointing there for a moment
    if (bytes_to_go && !always_buffer) {
        unsigned char *saved_buffer = file->buffer;
        size_t saved_buffer_size = file->buffer_size;

        file->buffer = read_ptr;
        file->buffer_size = bytes_to_go;
        ioresult = __load_buffer(file, &num_bytes, 1);
        if (ioresult != __no_io_error) {
            if (ioresult == __io_error) {
                file->state.error = 1;
                file->buffer_len = 0;
            } else {
                file->buffer_len = 0;
                file->state.io_state = __neutral;
                file->state.eof = 1;
            }
        }
        bytes_read += num_bytes;
        file->buffer = saved_buffer;
        file->buffer_size = saved_buffer_size;
        __prep_buffer(file);
        file->buffer_len = 0;
    }

    return bytes_read / memb_size;
}
