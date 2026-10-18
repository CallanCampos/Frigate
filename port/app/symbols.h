// The program's globals by name, for --run-dump (port/app/run_dir.h): their addresses in this process, from the
// program's own symbols, without exporting them to the dynamic linker. Plain C types only.
#ifndef PORT_APP_SYMBOLS_H
#define PORT_APP_SYMBOLS_H

// The address of the program's global object or function called name (its C name), or NULL when the program has no
// such global (statics aren't visible) or when it was stripped
void *port_symbol_address(const char *name);

#endif // PORT_APP_SYMBOLS_H
