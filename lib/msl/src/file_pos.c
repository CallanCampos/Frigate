// MSL's stream positioning: where a stream is in its file (ftell), moving it (fseek and rewind), and the two workers
// underneath, which the public functions call inside the stream's critical region. Overlay 189's HTTP code is the
// game's user of all three. The workers are static, since only these functions call them.
#include "ansi_files.h"
#include "buffer_io.h"
#include "critical_regions.h"
#include <errno.h>

// The position of the next byte: the file's own when the buffer is empty, otherwise where the buffer starts plus how
// far into it the stream is, less the characters that ungetc pushed back. Fails with EFPOS on a stream that is
// closed, in error or a string
static long _ftell(FILE *file) {
    long position;
    unsigned char kind = file->mode.file_kind;

    if (!(kind == __disk_file || kind == __console_file) || file->state.error) {
        errno = EFPOS;
        return -1;
    }
    if (file->state.io_state == __neutral) {
        return file->position;
    }
    position = (file->buffer_ptr - file->buffer) + file->buffer_pos;
    if (file->state.io_state >= __rereading) {
        position -= file->state.io_state - __rereading + 1;
    }
    return position;
}

// The region of a stream: the console streams have one each, all the other files share one
static inline int __stream_region(FILE *file) {
    if (file == stdin) {
        return stdin_access;
    }
    if (file == stdout) {
        return stdout_access;
    }
    if (file == stderr) {
        return stderr_access;
    }
    return files_access;
}

long ftell(FILE *file) {
    long retval;
    int region = __stream_region(file);

    __begin_critical_region(region);
    retval = _ftell(file);
    __end_critical_region(region);
    return retval;
}

// Moves within the buffer when the stream is being read and the position is in it, and otherwise through the
// position procedure, which drops the buffer. Only a disk file can be moved: a console has no position to set
static int _fseek(FILE *file, unsigned long position, int whence) {
    unsigned char kind = file->mode.file_kind;

    if (kind != __disk_file || file->state.error) {
        errno = EFPOS;
        return -1;
    }
    if (file->state.io_state == __writing && __flush_buffer(file, NULL) != __no_io_error) {
        file->state.error = 1;
        file->buffer_len = 0;
        errno = EFPOS;
        return -1;
    }
    if (whence == SEEK_CUR) {
        whence = SEEK_SET;
        position += _ftell(file);
    }
    if (whence != SEEK_END && file->mode.io_mode != __read_write &&
        (file->state.io_state == __reading || file->state.io_state == __rereading)) {
        if (position >= file->position || position < file->buffer_pos) {
            file->state.io_state = __neutral;
        } else {
            file->buffer_ptr = file->buffer + (position - file->buffer_pos);
            file->buffer_len = file->position - position;
            file->state.io_state = __reading;
        }
    } else {
        file->state.io_state = __neutral;
    }
    if (file->state.io_state == __neutral) {
        if (file->position_proc != NULL &&
            file->position_proc(file->handle, &position, whence, file->idle_proc) != __no_io_error) {
            file->state.error = 1;
            file->buffer_len = 0;
            errno = EFPOS;
            return -1;
        }
        file->state.eof = 0;
        file->position = position;
        file->buffer_len = 0;
    }
    return 0;
}

int fseek(FILE *file, long offset, int whence) {
    int retval;
    int region = __stream_region(file);

    __begin_critical_region(region);
    retval = _fseek(file, offset, whence);
    __end_critical_region(region);
    return retval;
}

void rewind(FILE *file) {
    file->state.error = 0;
    fseek(file, 0, SEEK_SET);
    file->state.error = 0;
}
