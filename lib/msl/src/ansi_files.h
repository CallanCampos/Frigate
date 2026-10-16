#ifndef POKEBW2_MSL_ANSI_FILES_H
#define POKEBW2_MSL_ANSI_FILES_H

#include "types.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

// The C library's FILE, from CodeWarrior's MSL, which ARM9 main links. It has the three console streams and nothing
// else: no fopen, so no files on a disk. The names are MSL's own (the library's private header, ansi_files.h)

typedef unsigned long __file_handle;

// What a file may be opened for, how it is buffered, what it is and whether it has been used as bytes or as wide
// characters (fwide's orientation). _IONBF, _IOLBF and _IOFBF are the buffer_mode values
enum __open_modes {
    __must_exist,
    __create_or_truncate,
    __create_if_necessary,
};

enum __io_modes {
    __read = 1,
    __write = 2,
    __read_write = 3,
    __append = 4,
};

enum __file_kinds {
    __closed_file,
    __disk_file,
    __console_file,
    __string_file,
};

enum __file_orientation {
    __unoriented,
    __char_oriented,
    __wide_oriented,
};

typedef struct __file_modes {
    unsigned int open_mode : 2;
    unsigned int io_mode : 3;
    unsigned int buffer_mode : 2;
    unsigned int file_kind : 3;
    unsigned int file_orientation : 2;
    unsigned int binary_io : 1;
} __file_modes;

// What a stream's buffer holds: nothing, bytes to write or bytes read. From __rereading on it also has characters that
// ungetc pushed back (io_state - __rereading + 1 of them, in ungetc_buffer or ungetc_wide_buffer), which fread hands
// out first
enum __io_states {
    __neutral,
    __writing,
    __reading,
    __rereading,
};

typedef struct __file_state {
    unsigned int io_state : 3;
    unsigned int free_buffer : 1;
    unsigned char eof;
    unsigned char error;
} __file_state;

// The results of a read, write or position procedure
enum __io_results {
    __no_io_error,
    __io_error,
    __io_EOF,
};

#define __ungetc_buffer_size 2

typedef void (*__idle_proc)(void);
typedef int (*__pos_proc)(__file_handle file, unsigned long *position, int mode, __idle_proc idle_proc);
typedef int (*__io_proc)(__file_handle file, unsigned char *buff, size_t *count, __idle_proc idle_proc);
typedef int (*__close_proc)(__file_handle file);

// The buffer is buffer_size bytes at buffer, and buffer_ptr walks it: buffer_len bytes remain to read (or are left to
// fill) from it, and buffer_pos is the file position of its first byte
struct _FILE {
    __file_handle handle;
    __file_modes mode;
    __file_state state;
    unsigned char is_dynamically_allocated;
    unsigned char char_buffer;
    unsigned char ungetc_buffer[__ungetc_buffer_size];
    wchar_t ungetc_wide_buffer[__ungetc_buffer_size];
    unsigned long position;
    unsigned char *buffer;
    unsigned long buffer_size;
    unsigned char *buffer_ptr;
    unsigned long buffer_len;
    unsigned long buffer_alignment;
    unsigned long saved_buffer_len;
    unsigned long buffer_pos;
    __pos_proc position_proc;
    __io_proc read_proc;
    __io_proc write_proc;
    __close_proc close_proc;
    __idle_proc idle_proc;
};

#define __files_count 3

// stdin, stdout and stderr, in that order
extern FILE __files[__files_count];

#define stdin (&__files[0])
#define stdout (&__files[1])
#define stderr (&__files[2])

// The size of the console streams' buffers
#define __console_buffer_size 0x100

// Flushes the console streams that are line buffered and being written, and all of the streams (fflush(NULL)'s work).
// Both return EOF if a flush failed
int __flush_line_buffered_output_files(void);
int __flush_all(void);

#endif // POKEBW2_MSL_ANSI_FILES_H
