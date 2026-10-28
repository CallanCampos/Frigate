// The real host resolver (hal/host/host_posix.c or host_windows.c) must find globals used by --run-dump without
// exporting game names to shared libraries. Shared by test_host.c and tiny ROM-free platform probes. ELF fixtures
// deliberately model LLD's localized hidden globals and two translation units with the same hidden-local name.
#ifndef PORT_TEST_HOST_GLOBAL_SYMBOLS_H
#define PORT_TEST_HOST_GLOBAL_SYMBOLS_H
#include "hal/host/host.h"
#include <stdio.h>

int port_host_test_visible_global = 17;
#if defined(__linux__)
static int port_host_test_hidden_unique __attribute__((used)) = 23;
static int port_host_test_hidden_duplicate __attribute__((used)) = 29;
static int port_host_test_ordinary_local __attribute__((used)) = 31;
__asm__(".hidden port_host_test_hidden_unique\n.hidden port_host_test_hidden_duplicate");
extern int port_host_test_weak_alias __attribute__((weak, alias("port_host_test_visible_global")));
void *port_host_test_hidden_peer(void);
#endif

static int TestGlobalSymbols(void) {
    int failed = port_host_global_address("port_host_test_visible_global") != &port_host_test_visible_global;
#if defined(__linux__)
    failed |= port_host_global_address("port_host_test_weak_alias") != &port_host_test_visible_global;
    failed |= port_host_global_address("port_host_test_hidden_unique") != &port_host_test_hidden_unique;
    failed |= port_host_global_address("port_host_test_ordinary_local") != NULL;
    failed |= port_host_test_hidden_peer() == &port_host_test_hidden_duplicate;
    failed |= port_host_global_address("port_host_test_hidden_duplicate") != NULL;
#endif
    printf("global_symbols: %s\n", failed ? "FAIL" : "ok");
    return failed;
}
#endif
