// Tests of the lookup of the program's globals by name that --run-dump uses (port_host_global_address,
// port/hal/host/host.h), on this test program's own symbols: on macOS through dlsym, on Linux through the program's ELF
// symbol table, on Windows through its COFF symbol table, with no symbol exported on Linux or Windows
#include "hal/host/host.h"
#include <stdio.h>

int port_test_symbols_global = 42;
static int sHidden = 7;

void port_test_symbols_function(void) {
}

static int sChecks;
static int sFailures;

static void Expect(const char *what, int ok) {
    sChecks++;
    if (!ok) {
        sFailures++;
        printf("FAIL %s\n", what);
    }
}

int main(void) {
    Expect("a global object", port_host_global_address("port_test_symbols_global") == &port_test_symbols_global);
    Expect("a global function",
           port_host_global_address("port_test_symbols_function") == (void *)port_test_symbols_function);
    Expect("no such global", port_host_global_address("port_test_symbols_none") == NULL);
    Expect("a static isn't visible", port_host_global_address("sHidden") == NULL && sHidden == 7);
    printf("%d checks, %d failed\n", sChecks, sFailures);
    return sFailures != 0;
}
