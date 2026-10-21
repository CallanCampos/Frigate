// MSL's console I/O for the three console streams of ansi_files.c (stdin, stdout and stderr), here through the TRK
// debugger's UART. The hooks are empty functions in the game (trk_stubs.c), so a write goes nowhere and a read gets 0
// characters, and the three procedures here are the ones the stream table points to. The original was built with
// C++ exception handling on, as the exception index in the ROM has entries for the read and the write procedure (and
// for none of the others, which have no stack frame).
#include "../console_io.h"
#include "trk_stubs.h"

int __read_console(__file_handle handle, unsigned char *buff, size_t *count, __idle_proc idle_proc) {
    size_t i;
    size_t size = *count;

    for (i = 0; i < size; i++) {
        buff[i] = ReadUARTN();
        if (buff[i] == '\r' || buff[i] == '\n') {
            *count = i + 1;
            break;
        }
    }
    return 0;
}

int __write_console(__file_handle handle, unsigned char *buff, size_t *count, __idle_proc idle_proc) {
    unsigned char last;

    if (*count > 1) {
        // The debugger gets the text but its last character as one string and the last character as another
        last = buff[*count - 1];
        buff[*count - 1] = '\0';
        WriteUARTN(buff);
        __TRK_write_console(&last);
    } else {
        __TRK_write_console(buff);
    }
    return 0;
}

int __close_console(__file_handle handle) {
    return 0;
}
