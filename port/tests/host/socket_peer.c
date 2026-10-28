// The shared-library side of socket_namespace.c, representing a shared SDL3/Wayland/X11 consumer. The public test
// entry is intentionally exported; socket itself must resolve to the host libc, not the executable's GameSpy-like
// sentinel. Opening an unconnected AF_UNIX socket requires no server, path, ROM or external network.
#include <sys/socket.h>
#include <unistd.h>

__attribute__((visibility("default"))) int port_host_test_shared_socket(void) {
    int (*volatile hostSocket)(int, int, int) = socket;
    int descriptor = hostSocket(AF_UNIX, SOCK_STREAM, 0);

    if (descriptor < 0) {
        return 0;
    }
    return close(descriptor) == 0;
}
