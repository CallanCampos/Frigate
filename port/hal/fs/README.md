# Native file-system adapters

The game uses NitroSDK's archive/file commands. These adapters preserve their APIs on a 64-bit host; game files
still come from the player's ROM. The feature map is in [ARCHITECTURE.md](../../docs/ARCHITECTURE.md).

| File | Responsibility |
| --- | --- |
| [fs_command_default_host.c](fs_command_default_host.c) | Includes the original SDK commands and supplies native archive loading/defaults. FAT/FNT caches use the caller's full `load_mem` pointer while positions remain archive offsets. |
| [port_fs_tables.h](port_fs_tables.h) | Typed cached-table resolver called by the small `PLATFORM_PC` branch in the SDK's `FSi_ReadTable`. Complete spans are bounded against the FAT/FNT ranges. |
| [fs_memfile_host.c](fs_memfile_host.c) | Files over caller-owned host memory, without reducing its address to the SDK's 32-bit archive base. |
| [fs_overlay.c](fs_overlay.c) | Overlay load/unload on the native program's linked game C. |
| [ovsec.c](ovsec.c), [ovsec.h](ovsec.h) | Reset an overlay's native static storage from generated section records when it loads. |
| [replaced.txt](replaced.txt) | Original SDK translation units replaced by the adapters. |

`FS_LoadArchiveTables` copies through the actual SDK file commands into the caller's aligned allocation.
`FS_UnloadArchiveTables` retains its original implementation: it suspends access, restores the original offsets and
returns the original allocation pointer. A null size query preserves an existing cache; a non-null reload unloads
it first, including an insufficient-buffer reload. Data-file reads continue through the archive's read callback.

[The real SDK lifecycle regression](../../tests/fs/README.md) covers these contracts without a ROM. This cache
correction adds no virtual elapsed time and does not establish trainer-AI timing parity; see
[ADR 0026](../../docs/adr/0026-native-archive-table-caches.md).
