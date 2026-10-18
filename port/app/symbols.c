// The program's globals by name (port/app/symbols.h), so that --run-dump can read any of the game's globals by its
// decomp name. Each host finds them its own way, in port/hal/host: through dlsym on macOS, through the program's own
// ELF symbol table on Linux, and through the COFF symbol table that lld keeps in a MinGW program on Windows.
#include "app/symbols.h"
#include "hal/host/host.h"

void *port_symbol_address(const char *name) {
    return port_host_global_address(name);
}
