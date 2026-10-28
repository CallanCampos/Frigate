# Host system services

These modules provide the OS services used by traps, fibers, PXI static-pointer checks, screenshots/state dumps,
renderer workers and saves. They implement no game logic and replace no Nintendo SDK source.

| File | Purpose |
| --- | --- |
| [host.h](host.h) | Shared pages, crash/backtrace, file, threading, clock and program-symbol API |
| [host_posix.c](host_posix.c) | macOS and Linux implementation; Linux resolves program globals from the unstripped ELF symbol table, without exporting game symbols to shared libraries |
| [host_windows.c](host_windows.c) | Windows implementation, including COFF symbol lookup, exception reporting and page/thread APIs |

Linux lookup keeps ordinary local symbols private. It also recognizes `STB_LOCAL` plus `STV_HIDDEN`, because LLD
can localize intentionally hidden program globals; GNU ld may keep those as `STB_GLOBAL HIDDEN`. Multiple eligible
same-name symbols at different addresses return NULL. Identical-address entries are harmless. This rule does not
export anything, change shared-library symbol binding, or let ordinary C statics override a program global.
A deliberately hidden local is indistinguishable from a localized global in the final ELF table; ambiguous names
therefore fail closed. The binary must retain its symbol table for `--run-dump` lookup.

[Host tests](../../tests/host/README.md) exercise real lookup without a ROM on GNU ld, LLD and native builds.
