// Tests of the lookup of the program's globals by name that --run-dump uses (port_host_global_address,
// port/hal/host/host.h), on this test program's own symbols: on macOS through dlsym, on Linux through the program's ELF
// symbol table, on Windows through its COFF symbol table, with no symbol exported on Linux or Windows. On Linux, also
// a hidden global as the decomp's are there (port/CMakeLists.txt), which GNU ld keeps GLOBAL and LLD makes LOCAL: the
// test runs linked by each (app_symbols, app_symbols_lld; port/cmake/app.cmake)
#include "hal/host/host.h"
#include <stdio.h>

int port_test_symbols_global = 42;
#ifdef __linux__
__attribute__((visibility("hidden"))) int port_test_symbols_hidden_global = 43;
#endif
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
#ifdef __linux__
    Expect("a hidden global",
           port_host_global_address("port_test_symbols_hidden_global") == &port_test_symbols_hidden_global);
#endif
    Expect("no such global", port_host_global_address("port_test_symbols_none") == NULL);
    Expect("a static isn't visible", port_host_global_address("sHidden") == NULL && sHidden == 7);
    printf("%d checks, %d failed\n", sChecks, sFailures);
    return sFailures != 0;
}
