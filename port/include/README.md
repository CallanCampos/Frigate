# port/include/

What the decomp's C files need to compile for a host. The host build force-includes `port_host.h` into each file of
`src/` and `lib/` (`-include`, in `port_decomp_env` in [../CMakeLists.txt](../CMakeLists.txt)), so the decomp's files
stay as upstream wrote them. The same target puts this directory on the include path ahead of the C library, where
`wchar.h` stands in for the C library's and MSL's, and where the few decomp files that need `port_wcm.h` find it under
`PLATFORM_PC`.

| File | What it does | Game functionality that depends on it | Tests |
| --- | --- | --- | --- |
| [port_host.h](port_host.h) | MWCC's built-ins under the names the decomp uses (`__memcpy`); `PORT_HW_ADDR()`, which turns a DS fixed address into the host memory of [../hal/hw](../hal/README.md#the-modules) ([ADR 0004](../docs/adr/0004-ds-addresses-by-header-translation.md)), with the prototypes of the register accessors (the card slot's SPI registers among them, [../hal/card/card_spi.c](../hal/card/card_spi.c)); and the declarations of the platform-layer hooks that a few decomp files call under `PLATFORM_PC`: the heap sizes, the particle work, the sound handles, the DS's address of `OS_GetTick`, which DWC adds to the seed of the console's Wi-Fi ID, and Frigate's test launcher's seam, `port_launcher_take_field_open` ([../app/launcher.c](../app/launcher.c)) | All of the game's C; the hooks are called from `src/gfl/heapsys.c`, `src/gfl/particle.c`, `lib/nnsys/src/snd/snd_arc.c`, `lib/dwc/src/bm/util_wifiidtool.c` and `src/ov035/event_mapchange.c` (CONTINUE's event) ([../docs/game-changes.md](../docs/game-changes.md)); the accessors by the SDK's card code that drives the infrared chip (`src/ov338/ir_check.c`, `lib/irc`) | every test that links the game's code |
| [wchar.h](wchar.h) | The DS wide-string boundary: only MSL scan and copy, under private names, with unsupported host wide APIs poisoned | DWC authentication names and profanity requests | `hal_wstring` |
| [port_wcm.h](port_wcm.h) | The native extent of WCM's DS workspace, shared by the SDK and DWC's two wrappers | Wireless initialization without overlapping access-point descriptors | `os_wcm_workspace` |

The host variants of the SDK headers that name DS addresses aren't here: the build makes them at every configure from
upstream's headers, in `build/port/gen/hwinclude`
([../tools/gen_host_tree.py](../tools/gen_host_tree.py)).
