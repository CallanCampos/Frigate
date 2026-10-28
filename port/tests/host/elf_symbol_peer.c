// Second translation unit for the Linux --run-dump resolver ambiguity test (global_symbols.h): two distinct local
// hidden objects share a name. No game data or emulator; selecting either by name must fail closed.
#if defined(__linux__)
static int port_host_test_hidden_duplicate __attribute__((used)) = 37;
__asm__(".hidden port_host_test_hidden_duplicate");
void *port_host_test_hidden_peer(void) {
    return &port_host_test_hidden_duplicate;
}
#endif
