# Host service tests

The real [host API](../../hal/host/README.md) underlies crash reports, fibers, PXI static ranges, saves and state dumps.
Tests run without a ROM; [../../cmake/host.cmake](../../cmake/host.cmake) registers `host_*` scenarios.

| File | Purpose |
| --- | --- |
| [test_host.c](test_host.c) | Scenario runner: faults/traps on host/fiber stacks, watchdog, files, UTF-8, static ranges and real global-symbol lookup |
| [global_symbols.h](global_symbols.h) | Shared assertions/fixtures for `host_global_symbols` and tiny actual-host-source probes: exact global/weak address, unique hidden local, ordinary local rejection and duplicate hidden-name rejection |
| [socket_namespace.c](socket_namespace.c), [socket_peer.c](socket_peer.c) | Linux executable and real shared-library fixture: direct game-like socket stays private while the DSO opens/closes a real unconnected AF_UNIX socket; both targets inherit the production visibility and sanitizer flags (`host_socket_namespace`) |
| [elf_symbol_peer.c](elf_symbol_peer.c) | Second Linux translation unit with a distinct hidden-local object sharing a fixture name; lookup must not select either arbitrarily |

Run `ctest --test-dir <native-build> -R '^host_global_symbols$' --output-on-failure`. Linux fixtures use explicit
assembler visibility directives to cover both linker representations regardless of which linker builds the test.
No dynamic export or fake symbol resolver is used. The other hosts check their real global lookup path too.

`host_socket_namespace` needs neither SDL nor a display server or ROM. Its public DSO entry is explicitly exported;
its socket import is not redirected by the test. A standalone default-visibility build fails this same test, proving
it detects executable interposition. Volatile typed function pointers prevent an inlined sentinel from pretending to
prove dynamic binding. The local unconnected socket is closed immediately; there is no external network traffic.
