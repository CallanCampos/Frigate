// The hooks that console_io.c writes and reads the debugger's console through: the TRK (Metrowerks' debugger kernel)
// and its UART. They are in the runtime as empty functions, which a debugger library replaces, so in the game the
// console's output goes nowhere and a read returns 0.
#include "trk_stubs.h"

void __TRK_write_console(unsigned char *text) {
}

void WriteUARTN(unsigned char *text) {
}

int ReadUARTN(void) {
    return 0;
}
