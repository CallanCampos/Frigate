#ifndef POKEBW2_MSL_BUFFER_IO_H
#define POKEBW2_MSL_BUFFER_IO_H

#include "ansi_files.h"

// What the stream functions share to fill and empty a FILE's buffer

// Convert the end of line characters of a buffer between the system's and the C library's; both do nothing here, where
// text and binary files are alike
void __convert_from_newlines(unsigned char *buf, size_t *n);
void __convert_to_newlines(unsigned char *buf, size_t *n);

// Points buffer_ptr at the start of the buffer, with the room for the rest of the block that the file position is in
void __prep_buffer(FILE *file);
// Reads into the buffer through read_proc, whole (1) or up to the next block, and gives the number of bytes read in
// *bytes_loaded if it isn't NULL. Returns an __io_results value
int __load_buffer(FILE *file, size_t *bytes_loaded, int whole);
// Writes what the buffer holds through write_proc, and gives the number of bytes written in *bytes_flushed if it isn't
// NULL. Returns an __io_results value
int __flush_buffer(FILE *file, size_t *bytes_flushed);

#endif // POKEBW2_MSL_BUFFER_IO_H
