# port/include/

What every decomp C file needs to compile for a host. The host build force-includes the header here into each file of
`src/` and `lib/` (`-include`, in `port_decomp_env` in [../CMakeLists.txt](../CMakeLists.txt)), so the decomp's files
stay as upstream wrote them.

| File | What it does | Game functionality that depends on it | Tests |
| --- | --- | --- | --- |
| [port_host.h](port_host.h) | MWCC's built-ins under the names the decomp uses (`__memcpy`); `PORT_HW_ADDR()`, which turns a DS fixed address into the host memory of [../hal/hw](../hal/README.md#the-modules) ([ADR 0004](../docs/adr/0004-ds-addresses-by-header-translation.md)), with the prototypes of the register accessors; and the declarations of the platform-layer hooks that a few decomp files call under `PLATFORM_PC`: the heap sizes, the particle work, the sound handles, and the DS's address of `OS_GetTick`, which DWC adds to the seed of the console's Wi-Fi ID | All of the game's C; the hooks are called from `src/gfl/heapsys.c`, `src/gfl/particle.c`, `lib/nnsys/src/snd/snd_arc.c` and `lib/dwc/src/bm/util_wifiidtool.c` ([../docs/game-changes.md](../docs/game-changes.md)) | every test that links the game's code |

The host variants of the SDK headers that name DS addresses aren't here: the build makes them at every configure from
upstream's headers, in `build/port/gen/hwinclude`
([../tools/gen_host_tree.py](../tools/gen_host_tree.py)).
