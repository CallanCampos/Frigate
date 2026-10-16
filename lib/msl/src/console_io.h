#ifndef POKEBW2_MSL_CONSOLE_IO_H
#define POKEBW2_MSL_CONSOLE_IO_H

#include "ansi_files.h"

// The procedures of the three console streams (console_io.c), which the __files table points to: read and write bytes
// through the debugger's console, and close, which does nothing. They are __io_proc and __close_proc
int __read_console(__file_handle handle, unsigned char *buff, size_t *count, __idle_proc idle_proc);
int __write_console(__file_handle handle, unsigned char *buff, size_t *count, __idle_proc idle_proc);
int __close_console(__file_handle handle);

#endif // POKEBW2_MSL_CONSOLE_IO_H
