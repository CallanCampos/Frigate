#ifndef POKEBW2_MSL_RUNTIME_TRK_STUBS_H
#define POKEBW2_MSL_RUNTIME_TRK_STUBS_H

// The debugger hooks of console_io.c (trk_stubs.c), which a debugger library overrides and which do nothing in the
// game. __write_console hands them a string: the bytes of a write but the last as one, then the last byte as another.
void __TRK_write_console(unsigned char *text);
void WriteUARTN(unsigned char *text);

// The byte that the debugger's console reads: 0 here
int ReadUARTN(void);

#endif // POKEBW2_MSL_RUNTIME_TRK_STUBS_H
