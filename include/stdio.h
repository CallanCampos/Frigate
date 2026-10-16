#ifndef POKEBW2_STDIO_H
#define POKEBW2_STDIO_H

#include <stdarg.h>
#include <stddef.h>

// The C library's stdio.h, from CodeWarrior's MSL. ARM9 main links only the console streams: stdin, stdout and stderr
// are the library's own (lib/msl/src/ansi_files.h, which has the layout of FILE)

typedef struct _FILE FILE;

#define EOF (-1)

// How a stream is buffered: not at all, a line at a time or completely
#define _IONBF 0
#define _IOLBF 1
#define _IOFBF 2

// Where fseek counts its offset from: the start of the file, the current position or the end
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

size_t fread(void *ptr, size_t memb_size, size_t num_memb, FILE *file);
int fclose(FILE *file);
int fflush(FILE *file);
long ftell(FILE *file);
int fseek(FILE *file, long offset, int whence);
void rewind(FILE *file);

// The formatted output to a string (printf.c): the characters that fit in n, with a terminator, and the number that the
// whole text has; sprintf has no limit
int vsnprintf(char *buffer, size_t n, const char *format, va_list arg);
int sprintf(char *buffer, const char *format, ...);

#endif // POKEBW2_STDIO_H
