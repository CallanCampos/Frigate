# port/hal/

The platform layer: the parts of the DS that the game's C reaches through Nintendo's SDK and that a host doesn't have,
written as host C. The game, the Game Freak library and most of NitroSDK and NitroSystem compile unchanged from the
decomp's `src/` and `lib/`; what is left is the SDK code that drives the hardware (in assembly, or through registers a
host can't give the same meaning), and the ARM7, the DS's second processor, which the decomp doesn't have at all. Each
directory here replaces one SDK module, or serves one of the ARM7's services. The layers around it are in
[../docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md), and the words in [../docs/GLOSSARY.md](../docs/GLOSSARY.md).

The build compiles every C file under `hal/` against the decomp's headers (the `port_hal` target,
[../CMakeLists.txt](../CMakeLists.txt)); the per-file options of the decomp files a module compiles through itself
are in the module's fragment in [../cmake/](../cmake/README.md). A module's tests are in
[../tests/](../tests/README.md), under the names in the last column (`ctest -R NAME` runs them).

## The modules

| Directory | What it replaces | What it does | Game functionality that depends on it | Tests |
| --- | --- | --- | --- | --- |
| [os/](os/port_os.h) | NitroSDK OS (`lib/nitro/src/os`): contexts, the CPSR, the ITCM interrupt handler, timers, ticks, alarms, arenas, the allocator's 64-bit edges, waits, terminate and reset | Runs the game's `main()` and every SDK thread on fibers of one host thread, under the decomp's own scheduler; the dispatcher, the virtual clock that drives VBlank, HBlank and the timers, and the frame and scanline hooks. Also the program's options (`port_app.h`, `app_options.c`), which any module can read | Everything: the main loop's frame wait (`OS_WaitIrq` in `src/system/main.c`), the heaps' arenas (`src/gfl/heap.c`, `src/system/gfl_use.c`), the sound loader thread (`src/system/snd_sys.c`), soft reset (`src/gfl/ui.c`) | `os_*`, `integrate_os_alloc`, `integrate_main_arena`, `integrate_unknown_option` |
| [hw/](hw/hw.h) | The DS's memory map and the registers whose accesses act (no SDK module; [ADR 0004](../docs/adr/0004-ds-addresses-by-header-translation.md)) | Ten 16 MiB windows of host memory that `PORT_HW_ADDR()` turns DS addresses into; the accessors of VCOUNT, the divider and square root, the geometry ports and GXFIFO (logged for the geometry engine), and the 3D readbacks | Every register and fixed address the SDK headers name; the divider through `lib/nitro/src/fx/fx_cp.c`; the 3D commands of everything that draws in 3D | `os_display_registers`, `g3d_hal`, `hal_mi`, `hal_fx` |
| [gx/](gx/vram.h) | NitroSDK GX's VRAM and load code: `gx_vramcnt.c`, `gx_state.c`, `mi_wram.c` (wrapped), `gx_load3d.c`, part of `gx_load2d.c`, `gxasm.c` | The nine VRAM banks as views of host memory mapped by page aliasing (or copying), the VRAMCNT writes that move them, texture and palette loads, the geometry FIFO's assembly, and which engine feeds which screen (`gx_display.c`) | Every picture: backgrounds, sprites, textures; `GX_SetBankFor*` in 18 game files (`src/gfl/clact.c`, `src/gfl/double3Ddisp.c`, the field's transitions) | `gx_*` (layout, the four backends, every SDK bank setting, loads, FIFO), `gx_display` |
| [dma/](dma/dma.c) | NitroSDK MI's DMA: `mi_dma.c`, `mi_dma_itcm.c`, `mi_dma_card.c`, `mi_dma_gxcommand.c` | DMA done at once by the CPU, unit by unit; asynchronous transfers raise their interrupt; GX command DMA goes into the geometry log | VRAM and palette uploads (`gx.h`'s load functions), NitroSystem G3D's display lists (`lib/nnsys/src/g3d/gecom.c`) | `integrate_dma_copies`, `integrate_dma_async`, `integrate_dma_gx_command` |
| [mi/](mi/mi_memory.c) | NitroSDK MI's assembly: `mi_memory.c`, `mi_swap.c`, `mi_uncompress.c` | CPU copies, fills and compares in the original's units and order; `MI_SwapWord`; the LZ77 decompressor | Nearly every file (`MI_CpuFill8` in 505 game files); compressed graphics (`src/gfl/arc_util.c`) | `hal_mi`, `asm_oracle` |
| [fx/](fx/fx_mtx33.c) | NitroSDK FX's matrix assembly in `fx_mtx22.c`, `fx_mtx33.c`, `fx_mtx43.c`, `fx_mtx44.c` (behind `#ifndef PLATFORM_PC` there) | Identity, copy, rotation and scale helpers of the fixed-point matrices | 3D cameras and models (`MTX_Identity33_` in 31 game files), Pokémon and trainer sprites (`src/system/mcss.c`), NitroSystem G3D, particles | `hal_fx`, `fx_libfx`, `asm_oracle` |
| [math/](math/math.c) | NitroSDK MATH's assembly: `math.c`, `math_pop.c`, `sha1_block.c` | `MATH_QSort` with the original's comparison order, count leading zeros, the SHA-1 block | The bag's sorting (`src/ov142/bag_item.c`), the random seed (`buildSeed`, `src/gfl/gf_standard.c`) | `hal_fx`, `asm_oracle` |
| [crypto/](crypto/rc4.c) | NitroSDK CRYPTO's `CRYPTO_RC4Encrypt` (assembly) | RC4 in the original's memory order | Wi-Fi's WDS code (`lib/dwc/src/wds/wds.c`) | `hal_fx`, `asm_oracle` |
| [rom/](rom/port_rom.h) | The game card's ROM and the DS's boot loader (no SDK module) | Finds the player's ROM (`--rom`, `FRIGATE_ROM`, the checkout's `orig/`), maps it read-only, checks it against the decomp's hashes, and places what the DS's boot leaves in main memory | Every file the game reads | `rom_*`, `rom_game_*` |
| [card/](card/port_card.h) | NitroSDK CARD's ROM reads (`card_rom.c`) and backup commands (`card_backup.c`), and the ARM7's card service | ROM reads from the mapped dump; the save chip, a 4 Mbit flash kept as `IREO.sav` or `IRDO.sav`, written whole a frame after the game's last write | Saving and loading (`src/gfl/backup_card.c`, `src/system/save_control.c`), every file read under `FS_ReadFile` | `card_backup_*`, `rom_card_reads`, `rom_game_backup*`, `replay_same_save`, `oracle_port_save`, `oracle_continue` |
| [fs/](fs/ovsec.h) | NitroSDK FS: `fs_overlay.c`, `fs_command_default.c`, `fs_memfile.c` | Overlay loading without images (every overlay is linked in): each load sets the overlay's writable statics back from a snapshot, as the DS reloads `.data` and clears `.bss`; archive tables left in the ROM; files over memory with whole pointers | Every overlay (`src/gfl/gf_overlay.c`); NARC members opened as files (`src/gfl/arc_tool.c`) | `rom_game_overlays`, `rom_game_file_system`, `rom_game_memory_files`, `overlay_sections_*` |
| [pxi/](pxi/pxi_bus.h) | NitroSDK PXI: `pxi_fifo.c`, `pxi_init.c` | The FIFO between the two processors: each word goes straight to the host service of its tag; the host ARM7's boot and its work at each VBlank (`arm7.c`); 64-bit pointers recovered from 32-bit words (`pointer.c`) | Everything the ARM7 serves: sound, saves, touch, the clock, power, the firmware settings | `pxi_bus_*`, and each service's `pxi_*` |
| [pm/](pm/pm7.c) | The ARM7's power management service, and NitroSDK's `pm.c` compiled through `pm_host.c` | Battery, backlights, screen power, the sound amplifier, sleep and power-off requests | Backlight and sleep on closing the lid (`src/gfl/ui.c`), the amplifier (`src/system/machine_use.c`) | `pxi_pm_*` |
| [tp/](tp/tp_host.h) | The ARM7's touch panel service (the ARM9's `lib/nitro/src/spi/tp.c` compiles unchanged) | Calibrated samples of where the player touches the bottom screen, one at a time or sampled automatically each frame | The touch screen, which the game reads through `src/gfl/touchpanel.c` | `pxi_tp_*`, `integrate_input` |
| [nvram/](nvram/nvram_settings.h) | The ARM7's NVRAM service and the firmware's user settings, and NitroSDK's `nvram.c` compiled through `nvram_host.c` | The console's owner settings (nickname, birthday, colour, language), MAC address and touch calibration in the shared memory, and a 256 KiB firmware flash image | `OS_GetOwnerInfo` (9 game files), `OS_GetMacAddress` (wireless and Wi-Fi play, such as `src/ov012/bsubway_comm.c`), the RTC offset in the save | `pxi_nvram_*` |
| [rtc/](rtc/rtc_host.h) | NitroSDK RTC's `rtc_api.c` (its assembly wait) and the ARM7's RTC service | The clock chip from the host's local time, or `--fixed-time`, which runs with the virtual clock | Day and night, seasons, daily events, play time (`RTC_*` in 44 game files, `src/gfl/rtc_cache.c`) | `pxi_rtc_*` |
| [reset/](reset/reset7.c) | The ARM7's OS service | The reset and terminate commands | Soft reset and returning to the title (`OS_ResetSystem`, which ends the program with status 74) | `pxi_reset_command` |
| [ctrdg/](ctrdg/ctrdg7.c) | The ARM7's cartridge slot service, and the BIOS's `SVC_CpuSet` | An empty GBA slot | The cartridge check at boot (`CTRDG_Init`, `src/system/machine_use.c`) | `pxi_ctrdg_empty` |
| [mic/](mic/mic.c) | NitroSDK's `lib/nitro/src/spi/mic.c` (its assembly wait) and the ARM7's MIC service | The microphone API; the microphone hears silence | The title screen's microphone test (`src/ov253/mictest.c`), the Xtransceiver (`src/ov257/ctvt_mic.c`) | `pxi_mic_silence` |
| [snd7/](snd7/snd7_internal.h) | The ARM7's sound driver and the DS's sound unit; NitroSDK SND's `snd_bank.c`, `snd_command.c` and `snd_interface.c` on the ARM9's side | The driver, written from the ARM7's code: command packets, the sequencer (16 players, 32 tracks), channels with envelopes, sweep and LFO, and the 16 hardware channels (PCM8, PCM16, IMA-ADPCM, PSG, noise) mixed to stereo ([ADR 0006](../docs/adr/0006-sound-driver-as-built.md)) | Music, sound effects and cries: everything `src/system/snd_sys.c` plays through NitroSystem's sound players | `snd_*`, `pxi_snd_*` |
| [input/](input/port_input.h) | KEYINPUT and the ARM7's XY word (hardware; the SDK's `PAD_Read` reads them) | Writes the buttons the program reports where the game reads them, active low, with X, Y and the lid in the shared memory | Every button press (`src/gfl/key.c`), the random seed's entropy | `integrate_input` |
| [nnsys/](nnsys/reloc.h) | NitroSystem's G2D file loaders (`lib/nnsys/src/g2d/*_load.c`) and FND heaps' frees (wrapped) | Builds host-layout structs from G2D files (characters, palettes, cells, multi-cells, animations, fonts) instead of relocating them in place, and frees them with the heap memory they came from ([ADR 0007](../docs/adr/0007-nitrosystem-files-in-host-structs.md)) | Every sprite and cell animation (`src/gfl/clact.c`, `src/gfl/arc_util.c`), the GFL heaps (`src/gfl/heapsys.c`), the sound heap | `nnsys_*`, `nnsys_g2d_rom_equivalence`, `nnsys_ds_layout` |
| [gfl/](gfl/heap_policy.c) | Hooks into Game Freak's library (`src/gfl`), and `src/system/gfl_use.c` compiled through `gfl_use_host.c` | The host's heap sizes (`port_heap_size`), the particle systems' larger work ([ADR 0008](../docs/adr/0008-particle-work-from-the-platform-layer.md)), and GFL's frame count for replays | Every heap (`src/gfl/heapsys.c`), particles (`src/gfl/particle.c`), input replays | whole-program tests (`replay_*`, `oracle_*`) |
| [dwc/](dwc/ov011_pending.c) | Overlay 11 functions with no C yet that DWC's initialization reaches | Their offline effect | The boot, which calls `DWC_InitForProduction` (`src/ov011/dwc_rapcommon.c`) | whole-program tests |
| [stubs/](stubs/dsprot.c) | DS Protect (`lib/dsprot`, overlays 165 and 337), and a call the decomp names by placeholder | The checks pass as on a genuine cartridge; the placeholder call has no effect | The checks in `src/system/game_system.c`, `src/ov012/event_battle.c`, `src/ov035/event_mapchange.c`, `src/ov165/pokelist.c`; `src/system/resh_zek_savedata.c` | whole-program tests |

The root's files:

| File | What it does | Tests |
| --- | --- | --- |
| [trap.c](trap.c), [trap.h](trap.h) | Trap stubs: a DS function or data object that has no C yet stops the program with its name, its address and its caller (status 70). [../tools/gen_stubs.py](../tools/gen_stubs.py) writes a stub for each at every build; data placeholders sit on pages with no access, and a fault handler names the one touched | `smoke_reaches_game_main`, `oracle_continue` (which stops at the field's first trap) |
| [replaced.txt](replaced.txt) | The decomp files the host build leaves out, whole modules and asm-only files, each with its reason. A module's own `replaced.txt` lists the rest; [../tools/gen_host_tree.py](../tools/gen_host_tree.py) reads them all | the build |

## How a decomp file is adapted

Four patterns, which every module uses:

| Pattern | What it looks like | Examples |
| --- | --- | --- |
| Left out and rewritten | The decomp file is in a `replaced.txt`; the module defines the same functions | `os/os_tick.c`, `pxi/pxi_fifo.c`, `card/card_rom.c`, `mi/mi_memory.c`, `gx/gx_load3d.c` |
| Compiled through a host file | `*_host.c` renames the functions that can't run on a 64-bit host (`#define F F_ds`), includes the decomp's file, and writes them again with access to its statics | `os/os_thread_host.c`, `card/card_backup_host.c`, `fs/fs_memfile_host.c`, `pm/pm_host.c`, `snd7/snd_interface_host.c`, `gfl/gfl_use_host.c` |
| Compiled in place, functions renamed | A CMake fragment renames a file's exported functions to `port_sdk_<name>`, and the module defines the original name as a wrapper | `gx/vramcnt.c` ([../cmake/gx.cmake](../cmake/gx.cmake)), `nnsys/fnd_hooks.c` and `nnsys/g2d_load.c` ([../cmake/nnsys.cmake](../cmake/nnsys.cmake)) |
| Assembly behind `#ifndef PLATFORM_PC` | The SDK file keeps its MWCC `asm` function for the ROM build; the module has the C version | `fx/fx_mtx*.c`, `crypto/rc4.c`, `math/math_pop.c`, `gx/gxasm.c` |

Every change these patterns need outside `port/` (the `#ifndef PLATFORM_PC` guards) has a row in
[../docs/game-changes.md](../docs/game-changes.md).

## Names

- `*7.c` is the host ARM7's side of a PXI service (`pm7.c`, `tp7.c`, `nvram7.c`, `rtc7.c`, `reset7.c`, `ctrdg7.c`,
  `snd7*.c`), started by `pxi/arm7.c`. `*_host.c` compiles a decomp file through, as above. `*_internal.h` is shared
  inside one module only.
- A header that says "Plain C types only" can be included by [../app](../app/README.md) and [../gfx](../gfx/README.md),
  which don't see the decomp's headers.
- Public functions are `port_<module>_*`. Inside the OS core, `port_osi_*` (after the SDK's `OSi_`) is shared between
  the files of `os/` only (`os_internal.h`); inside the sound driver, `snd7_*` is shared between the files of `snd7/`.
- `vram_os.c` in `gx/` holds the host operating system's virtual memory calls; it has nothing to do with `os/`.

Some headers are named for the interface rather than the file that implements it:

| Header | Implemented in |
| --- | --- |
| [os/port_app.h](os/port_app.h) (the program's options and hooks) | `os/app_options.c` |
| [os/port_os.h](os/port_os.h) (the OS core) | `os/core.c`, and `os/os_arena.c` (`port_os_set_main_arena_size`), `os/memory_map.c` (the IRQ check word) |
| [card/port_card.h](card/port_card.h) (the save chip) | `card/backup.c` |
| [input/port_input.h](input/port_input.h) | `input/input.c` |
| [rtc/rtc_host.h](rtc/rtc_host.h) | `rtc/rtc7.c` |
| [tp/tp_host.h](tp/tp_host.h) | `tp/tp7.c` |
| [pxi/arm7.h](pxi/arm7.h) (what the ARM7's services share) | `pxi/arm7.c` and each service |
| [snd7/snd_handle.h](snd7/snd_handle.h) | `snd7/snd_bank_host.c` |
| [gfl/gfl_frame.h](gfl/gfl_frame.h) | `gfl/gfl_use_host.c` |
| [../include/port_host.h](../include/port_host.h)'s `port_heap_size` and particle hooks | `gfl/heap_policy.c`, `gfl/particle_work.c` |

## Adding a module

A new SDK module gets a directory named after it, its C files (which the build's glob picks up), a `replaced.txt` for
the decomp files it takes over, a CMake fragment in [../cmake/](../cmake/README.md) for its tests and per-file options,
its tests in [../tests/](../tests/README.md), a row in the table above, and a row in the feature map of
[../docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md#feature-map) for the game functionality it serves
([../CLAUDE.md](../CLAUDE.md#code-that-modders-can-read)).
