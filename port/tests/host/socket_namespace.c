// ELF executable/DSO isolation for the GameSpy BSD names in lib/dwc/src/gscommon/gsPlatformSocket.c: a shared SDL3's
// display libraries must reach libc, while the game's direct calls retain its own implementation. No ROM, display
// server or external network: the peer opens and closes one unconnected local AF_UNIX socket.
#include <stdio.h>
#include <sys/socket.h>

#define SOCKET_SENTINEL (-317)
__attribute__((visibility("default"))) int port_host_test_shared_socket(void);

int socket(int domain, int type, int protocol) {
    (void)domain;
    (void)type;
    (void)protocol;
    return SOCKET_SENTINEL;
}

int main(void) {
    int (*volatile ownSocket)(int, int, int) = socket;
    int own = ownSocket(AF_UNIX, SOCK_STREAM, 0);
    int shared = port_host_test_shared_socket();

    printf("socket_namespace: direct=%d shared_libc=%d\n", own == SOCKET_SENTINEL, shared);
    return own == SOCKET_SENTINEL && shared ? 0 : 1;
}
