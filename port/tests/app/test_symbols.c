// Tests of the program's symbol lookup (port/app/symbols.h) on this test program's own symbols: on macOS through
// dlsym, on Linux through the program's ELF symbol table, with no symbol exported in either
#include "app/symbols.h"
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
    Expect("a global object", port_symbol_address("port_test_symbols_global") == &port_test_symbols_global);
    Expect("a global function",
           port_symbol_address("port_test_symbols_function") == (void *)port_test_symbols_function);
    Expect("no such global", port_symbol_address("port_test_symbols_none") == NULL);
    Expect("a static isn't visible", port_symbol_address("sHidden") == NULL && sHidden == 7);
    printf("%d checks, %d failed\n", sChecks, sFailures);
    return sFailures != 0;
}
