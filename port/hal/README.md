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
| [os/](os/port_os.h) | NitroSDK OS (`lib/nitro/src/os`): contexts, the CPSR, the ITCM interrupt handler, timers, ticks, alarms, arenas, the allocator's 64-bit edges, waits, terminate and reset | Runs the game's `main()` and every SDK thread on fibers of one host thread, under the decomp's own scheduler; the dispatcher, the virtual clock that drives VBlank, HBlank, the timers and other modules' timed work (the sound driver's alarms), and the frame and scanline hooks. Also the program's options (`port_app.h`, `app_options.c`), which any module can read | Everything: the main loop's frame wait (`OS_WaitIrq` in `src/system/main.c`), the heaps' arenas (`src/gfl/heap.c`, `src/system/gfl_use.c`), the sound loader thread (`src/system/snd_sys.c`), the sound streams' alarms (NitroSystem's stream player, `lib/nnsys/src/snd/snd_stream.c`), soft reset (`src/gfl/ui.c`), the screen wipes that cover lines one by one, whose HBlank function checks DISPSTAT's HBlank flag (`src/system/wipe_sub.c`) | `os_*`, `integrate_os_alloc`, `integrate_main_arena`, `integrate_unknown_option` |
| [hw/](hw/hw.h) | The DS's memory map and the registers whose accesses act (no SDK module; [ADR 0004](../docs/adr/0004-ds-addresses-by-header-translation.md)) | Ten 16 MiB windows of host memory that `PORT_HW_ADDR()` turns DS addresses into; the accessors of VCOUNT, the divider and square root, the geometry ports and GXFIFO (logged for the geometry engine), and the 3D readbacks (the card slot's SPI registers have theirs in [card/card_spi.c](card/card_spi.c)) | Every register and fixed address the SDK headers name; the divider through `lib/nitro/src/fx/fx_cp.c`; the 3D commands of everything that draws in 3D | `os_display_registers`, `g3d_hal`, `hal_mi`, `hal_fx` |
| [gx/](gx/vram.h) | NitroSDK GX's VRAM and load code: `gx_vramcnt.c`, `gx_state.c`, `mi_wram.c` (wrapped), `gx_load3d.c`, part of `gx_load2d.c`, `gxasm.c` | The nine VRAM banks as views of host memory mapped by page aliasing (or copying), the VRAMCNT writes that move them, texture and palette loads, the geometry FIFO's assembly, and which engine feeds which screen (`gx_display.c`) | Every picture: backgrounds, sprites, textures; `GX_SetBankFor*` in many game files (`src/gfl/clact.c`, `src/gfl/double3Ddisp.c`, the field's transitions) | `gx_*` (layout, the four backends, every SDK bank setting, loads, FIFO), `gx_display` |
| [dma/](dma/dma.c) | NitroSDK MI's DMA: `mi_dma.c`, `mi_dma_itcm.c`, `mi_dma_card.c`, `mi_dma_gxcommand.c` | DMA done at once by the CPU, unit by unit; asynchronous transfers raise their interrupt; GX command DMA goes into the geometry log | VRAM and palette uploads (`gx.h`'s load functions), NitroSystem G3D's display lists (`lib/nnsys/src/g3d/gecom.c`) | `integrate_dma_copies`, `integrate_dma_async`, `integrate_dma_gx_command` |
| [mi/](mi/mi_memory.c) | NitroSDK MI's assembly: `mi_memory.c`, `mi_swap.c`, `mi_uncompress.c` | CPU copies, fills and compares in the original's units and order; `MI_SwapWord`; the LZ77 decompressor | Nearly every file (`MI_CpuFill8`, `MI_CpuCopy8`: `src/gfl/bg_sys.c`, `src/gfl/backup_system.c`); compressed graphics (`src/gfl/arc_util.c`) | `hal_mi`, `asm_oracle` |
| [fx/](fx/fx_mtx33.c) | NitroSDK FX's matrix assembly in `fx_mtx22.c`, `fx_mtx33.c`, `fx_mtx43.c`, `fx_mtx44.c` (behind `#ifndef PLATFORM_PC` there) | Identity, copy, rotation and scale helpers of the fixed-point matrices | 3D cameras and models (`MTX_Identity33_` in `src/gfl/billboard.c`, `src/gfl/calctool.c` and others), Pokémon and trainer sprites (`src/system/mcss.c`), NitroSystem G3D, particles | `hal_fx`, `fx_libfx`, `asm_oracle` |
| [math/](math/math.c) | NitroSDK MATH's assembly: `math.c`, `math_pop.c`, `sha1_block.c` | `MATH_QSort` with the original's comparison order, count leading zeros, the SHA-1 block | The bag's sorting (`src/ov142/bag_item.c`), the random seed (`buildSeed`, `src/gfl/gf_standard.c`) | `hal_fx`, `asm_oracle` |
| [crypto/](crypto/rc4.c) | NitroSDK CRYPTO's `CRYPTO_RC4Encrypt` (assembly) | RC4 in the original's memory order | Wi-Fi's WDS code (`lib/dwc/src/wds/wds.c`) | `hal_fx`, `asm_oracle` |
| [rom/](rom/port_rom.h) | The game card's ROM and the DS's boot loader (no SDK module) | Finds the player's ROM (`--rom`, `FRIGATE_ROM`, the checkout's `orig/`), maps it read-only, checks it against the decomp's hashes, and places what the DS's boot leaves in main memory | Every file the game reads | `rom_*`, `rom_game_*` |
| [card/](card/port_card.h) | NitroSDK CARD's ROM reads (`card_rom.c`) and backup commands (`card_backup.c`), the ARM7's card service, and the card slot's SPI bus with the cartridge's infrared chip (`AUXSPICNT`, `AUXSPIDATA`; no SDK module, [ADR 0011](../docs/adr/0011-cartridge-infrared-chip-as-a-register-model.md)) | ROM reads from the mapped dump; the save chip, a 4 Mbit flash kept as `IREO.sav` or `IRDO.sav`, written whole a frame after the game's last write; the SPI registers and the infrared chip as melonDS has them, which answer the game's genuine-card check as a genuine chip does | Saving and loading (`src/gfl/backup_card.c`, `src/system/save_control.c`), every file read under `FS_ReadFile`; the genuine-card check at every CONTINUE and NEW GAME and when a Pokémon faints in battle (`src/ov338/ir_check.c`; on failure the C-Gear's new-beacon marks are hidden for good and battles give no experience), the infrared library (`lib/irc`) | `card_backup_*`, `rom_card_reads`, `rom_game_backup*`, `rom_game_ir_genuine_card`, `hal_card_spi_*`, `replay_same_save`, `oracle_port_save`, `oracle_continue` |
| [fs/](fs/ovsec.h) | NitroSDK FS: `fs_overlay.c`, `fs_command_default.c`, `fs_memfile.c` | Overlay loading without images (every overlay is linked in): each load sets the overlay's writable statics back from a snapshot, as the DS reloads `.data` and clears `.bss`; archive tables left in the ROM; files over memory with whole pointers | Every overlay (`src/gfl/gf_overlay.c`); NARC members opened as files (`src/gfl/arc_tool.c`) | `rom_game_overlays`, `rom_game_file_system`, `rom_game_memory_files`, `overlay_sections_*` |
| [pxi/](pxi/pxi_bus.h) | NitroSDK PXI: `pxi_fifo.c`, `pxi_init.c` | The FIFO between the two processors: each word goes straight to the host service of its tag; the host ARM7's boot and its work at each VBlank (`arm7.c`); 64-bit pointers recovered from 32-bit words (`pointer.c`) | Everything the ARM7 serves: sound, saves, touch, the clock, power, the firmware settings | `pxi_bus_*`, and each service's `pxi_*` |
| [pm/](pm/pm7.c) | The ARM7's power management service, and NitroSDK's `pm.c` compiled through `pm_host.c` | Battery, backlights, screen power, the sound amplifier, sleep and power-off requests | Backlight and sleep on closing the lid (`src/gfl/ui.c`), the amplifier (`src/system/machine_use.c`) | `pxi_pm_*` |
| [tp/](tp/tp_host.h) | The ARM7's touch panel service (the ARM9's `lib/nitro/src/spi/tp.c` compiles unchanged) | Calibrated samples of where the player touches the bottom screen, one at a time or sampled automatically each frame | The touch screen, which the game reads through `src/gfl/touchpanel.c` | `pxi_tp_*` (no replay touches the screen yet) |
| [nvram/](nvram/nvram_settings.h) | The ARM7's NVRAM service and the firmware's user settings, and NitroSDK's `nvram.c` compiled through `nvram_host.c` | The console's owner settings (nickname, birthday, colour, language), MAC address and touch calibration in the shared memory, and a 256 KiB firmware flash image | `OS_GetOwnerInfoEx` (the console owner's birthday on the trainer card, `src/ov012/scrcmd_game_state.c`), `OS_GetMacAddress` (wireless and Wi-Fi play, such as `src/ov012/bsubway_comm.c`), the RTC offset in the save | `pxi_nvram_*` |
| [rtc/](rtc/rtc_host.h) | NitroSDK RTC's `rtc_api.c` (its assembly wait) and the ARM7's RTC service | The clock chip from the host's local time, or `--fixed-time`, which runs with the virtual clock or, pinned by a replay's input file, stays at its time | Day and night, seasons, daily events, play time (`RTC_*` in many game files: `src/gfl/rtc_cache.c`, `src/ov012/ev_time.c`) | `pxi_rtc_*` |
| [reset/](reset/reset7.c) | The ARM7's OS service | The reset and terminate commands | Soft reset and returning to the title (`OS_ResetSystem`, which ends the program with status 74) | `pxi_reset_command` |
| [ctrdg/](ctrdg/ctrdg7.c) | The ARM7's cartridge slot service, and the BIOS's `SVC_CpuSet` | An empty GBA slot | The cartridge check at boot (`CTRDG_Init`, `src/system/machine_use.c`) | `pxi_ctrdg_empty` |
| [mic/](mic/mic.c) | NitroSDK's `lib/nitro/src/spi/mic.c` (its assembly wait) and the ARM7's MIC service | The microphone API; the microphone hears silence | The title screen's microphone test (`src/ov253/mictest.c`), the Xtransceiver (`src/ov257/ctvt_mic.c`) | `pxi_mic_silence` |
| [snd7/](snd7/snd7_internal.h) | The ARM7's sound driver and the DS's sound unit; NitroSDK SND's `snd_bank.c`, `snd_command.c` and `snd_interface.c` on the ARM9's side | The driver, written from the ARM7's code: command packets, the sequencer (16 players, 32 tracks), channels with envelopes, sweep and LFO, the alarms (as the OS core's timed work), and the 16 hardware channels (PCM8, PCM16, IMA-ADPCM, PSG, noise) with melonDS's start delays and FIFO, mixed to stereo at GBATEK's widths ([ADR 0006](../docs/adr/0006-sound-driver-as-built.md)); its state after each run, in the DS's layout, for the sound trace that holds it to melonDS ([../tools/oracle](../tools/oracle/README.md#sound)) | Music, sound effects and cries: everything `src/system/snd_sys.c` plays through NitroSystem's sound players | `snd_*`, `pxi_snd_*`, `oracle_sound_*` |
| [input/](input/port_input.h) | KEYINPUT and the ARM7's XY word (hardware; the SDK's `PAD_Read` reads them) | Writes the buttons the program reports where the game reads them, active low, with X, Y and the lid in the shared memory | Every button press (`src/gfl/key.c`), the random seed's entropy | `integrate_input` |
| [nnsys/](nnsys/reloc.h) | NitroSystem's G2D file loaders (`lib/nnsys/src/g2d/*_load.c`) and FND heaps' frees (wrapped) | Builds host-layout structs from G2D files (characters, palettes, cells, multi-cells, animations, fonts) instead of relocating them in place, and frees them with the heap memory they came from ([ADR 0007](../docs/adr/0007-nitrosystem-files-in-host-structs.md)) | Every sprite and cell animation (`src/gfl/clact.c`, `src/gfl/arc_util.c`), the GFL heaps (`src/gfl/heapsys.c`), the sound heap | `nnsys_*`, `nnsys_g2d_rom_equivalence`, `nnsys_ds_layout` |
| [gfl/](gfl/heap_policy.c) | Hooks into Game Freak's library (`src/gfl`), and `src/system/gfl_use.c` and `src/gfl/gf_standard.c` compiled through `gfl_use_host.c` and `gf_standard_host.c` | The host's heap sizes (`port_heap_size`) and the memory of the root heaps the game creates in its own (`port_heap_root_memory`, [ADR 0021](../docs/adr/0021-root-heaps-in-the-games-own-memory-from-host-memory.md)), the particle systems' larger work ([ADR 0008](../docs/adr/0008-particle-work-from-the-platform-layer.md)), GFL's frame count for replays, the random seeds an input file gives in place of the ones `buildSeed` builds, and the handles through which `--run-dump` reads GFL's random state | Every heap (`src/gfl/heapsys.c`), particles (`src/gfl/particle.c`), input replays, the random numbers of the field and of every battle | `hal_heap_policy`, `hal_random_seed`; whole-program tests (`replay_*`, `oracle_*`) |
| [msl/](msl/fp_fastI.c) | MSL's runtime routine `_u32_div_f` (`lib/msl/src/fp/fp_fastI.s`): not a game function but the C runtime MWCC links into the ROM, which the host's C library replaces everywhere else (`replaced.txt`) | The unsigned 32-bit division as the ARM routine returns it, the quotient in the low half of a `u64` and the remainder in the high half, and the DS's result for a divisor of 0: the dividend, remainder 0. MSL's string extensions are in `stubs/` | The two game files that call the routine by name: the field's lens flare data at every map change (`src/ov036/field_goout_effect_data.c`), the Unity Tower's visitors (`src/ov033/unity_visitor.c`) | `hal_msl`, `asm_oracle` |
| [stubs/](stubs/dsprot.c) | DS Protect (`lib/dsprot`, overlays 165 and 337) | The checks pass as on a genuine cartridge | The checks in `src/system/game_system.c`, `src/ov012/event_battle.c`, `src/ov035/event_mapchange.c`, `src/ov165/pokelist.c` | whole-program tests |
| [stubs/gbucket.c](stubs/gbucket.c) | GameSpy's three bucket-set declarations (`lib/dwc/include/dwc/gbucket.h`), with no implementation in the ROM | Unconditional function-specific "not in ROM" diagnostic and exit 70; no substitute behavior | References from DS-dead-stripped `lib/dwc/src/gstats/gstats.c` code retained by host links | `hal_gbucket_*` |
| [host/](host/host.h) | No SDK module: the host operating system's services that the platform layer, the program and the tests share where macOS, Linux and Windows differ ([ADR 0013](../docs/adr/0013-windows-build-with-msys2-clang64.md); the modules that keep their own branches are listed [below](#names)) | Pages and their protection, faults (a trap placeholder touched, by the game's code or the C library's, a fiber's stack overflowing into its guard, a crash, on a fiber too: a memory fault, a division by zero, an illegal instruction or a breakpoint, each reported as `port: crash: ...` with a backtrace), reported on a stack of their own, but for the exceptions off the fibers that Windows' vectored handler leaves to the unhandled-exception filter (a division by zero, an illegal instruction, a breakpoint, a memory fault in another module's code), reported on the faulting stack, backtraces and the program's names, writes to stderr that take no lock, the watchdog, the monotonic clock, the renderer's threads and the signals they wait on, directories, the environment and the program's static data; on Windows, binary stdout and stderr, no error dialogs and both exception handlers before `main()` | None directly: the trap stubs, the fibers' guards, the PXI pointers into static data, `--run-dump`, `--frame-times`, the 3D renderer's workers and the watchdog stand on it | `host_*`, `os_guard_*`, `app_symbols`, `pxi_bus_arena_overlap`, `integrate_overlay_sections`, the CI's macOS, Linux and Windows jobs |

The root's files:

| File | What it does | Tests |
| --- | --- | --- |
| [trap.c](trap.c), [trap.h](trap.h) | Trap stubs: a DS function or data object that has no C yet stops the program with its name, its address and its caller (status 70). [../tools/gen_stubs.py](../tools/gen_stubs.py) writes a stub for each at every build; data placeholders sit on pages with no access (readable again at exit in the sanitized build, for LeakSanitizer's scan), and a fault handler names the one touched; any other memory fault it reports as a crash (`port: crash: ...`, with a backtrace), as [host/](host/host.h) reports the crashes that aren't memory faults | `smoke_reaches_game_main`, `host_*` (a placeholder touched, on the host's stack and on a fiber) |
| [replaced.txt](replaced.txt) | The decomp files the host build leaves out, whole modules and asm-only files, each with its reason. A module's own `replaced.txt` lists the rest; [../tools/gen_host_tree.py](../tools/gen_host_tree.py) reads them all | the build |

## Files of each module

Every file opens with what it does, the part of the game it serves and the decomp file it replaces; this is the short
version, to find the file. `replaced.txt` in a module lists the decomp files that module takes over.

**[os/](os/port_os.h)** — threads, interrupts and time

| File | What it does |
| --- | --- |
| [port_os.h](os/port_os.h) | The OS core's interface for the program, the tests and the other modules: running the game, the frame and scanline hooks, the virtual clock, and timed work at a cycle of it (`port_os_set_timed_work`) |
| [core.c](os/core.c) | The OS core: the dispatcher, the CPU state (contexts, the CPSR), interrupts at defined points, the halt, the virtual clock's lines, VBlank, the HBlank interrupt as DISPSTAT's HBlank flag sets, and timers, other modules' timed work (the sound driver's next alarm), and the frame and scanline hooks |
| [fiber.c](os/fiber.c), [fiber.h](os/fiber.h) | Fibers over minicoro, one per SDK thread, switched on the one host thread; it tells AddressSanitizer about every switch, frees the fake stack of a fiber destroyed while suspended, and gives LeakSanitizer the live frames of suspended fibers ([../docs/sanitizers.md](../docs/sanitizers.md)) |
| [mco_impl.c](os/mco_impl.c), [mco_impl.h](os/mco_impl.h) | minicoro's implementation, in a file of its own so that a sanitizer build leaves it uninstrumented, and a suspended coroutine's saved stack pointer for fiber.c, declared in the header |
| [sanitizers.c](os/sanitizers.c) | The sanitized build's runtime hooks: UndefinedBehaviorSanitizer's reports with AddressSanitizer's stack trace, which follows the fibers; the runtime's defaults, a report aborting the program on every host (`abort_on_error=1`) and, with `PORT_SANITIZE_RECOVER`, ASan going on after a report. Empty in other builds ([../docs/sanitizers.md](../docs/sanitizers.md)) |
| [os_thread_host.c](os/os_thread_host.c) | The decomp's `os_thread.c` (the scheduler) compiled through, with the context switch given to the fibers |
| [os_init.c](os/os_init.c) | `OS_Init`: the SDK's boot sequence (PXI, the card, the tick, alarms) as the decomp's file has it, on the host |
| [os_interrupt.c](os/os_interrupt.c) | Interrupt handlers and masks (`OS_SetIrqFunction`, IE, IME), with the effects the hardware has; a DMA channel's or timer's handler is called with its own type, where the SDK casts it to a callback |
| [os_irqTable.c](os/os_irqTable.c) | The interrupt table and the handlers of the interrupts that have callbacks, DTCM data with no C in the decomp |
| [os_system.c](os/os_system.c) | Waits: `OS_WaitIrq`, the busy loops (`OS_SpinWait`, `SVC_WaitByLoop`) as moves of the virtual clock |
| [os_tick.c](os/os_tick.c) | The tick counter, on the virtual clock's timer 0 |
| [os_alarm.c](os/os_alarm.c) | Alarms, on the virtual clock's timer 1 |
| [os_valarm_host.c](os/os_valarm_host.c) | The decomp's V-count alarms compiled through, so that each run starts them afresh |
| [os_arena.c](os/os_arena.c) | The memory arenas over host memory (the 16 MiB main arena, `--main-arena-size`), and whether a span lies in the main arena's block (`port_os_main_arena_holds`) |
| [os_alloc_host.c](os/os_alloc_host.c) | The decomp's heap allocator (`os_alloc.c`) compiled through, with its 64-bit edges fixed |
| [memory_map.c](os/memory_map.c) | The linker symbols of the DS memory map that the SDK's C names, and the host's DTCM |
| [os_cpu.c](os/os_cpu.c) | What only drives the ARM9's coprocessor or memory map: the caches, the protection unit, the TCM address, the exception vector; nothing to do on a host |
| [os_emulator.c](os/os_emulator.c) | The console type: a retail DS, not a DSi or a development unit (`OS_IsRunOnTwl` is FALSE) |
| [os_spinLock.c](os/os_spinLock.c) | Spin locks and the lock-ID pool, without the handshake with the ARM7 |
| [os_vramExclusive.c](os/os_vramExclusive.c) | The exclusive use of VRAM banks, with its count-leading-zeros helper in C |
| [os_reset_host.c](os/os_reset_host.c) | The decomp's `os_reset.c` compiled through, so that each run starts it afresh |
| [os_terminate.c](os/os_terminate.c) | How the game stops: `OS_Terminate` (status 71) and `OS_ResetSystem`'s end (status 74) |
| [os_internal.h](os/os_internal.h) | What the files of `os/` share; only they and their tests include it |
| [port_app.h](os/port_app.h), [app_options.c](os/app_options.c) | The product's name, description and disclaimer (which the build reads too); the program's options and hooks, which any module reads |
| [frame_time.c](os/frame_time.c), [frame_time.h](os/frame_time.h) | `--frame-times`: the host time each frame of a run takes, by part (the 3D frame, the geometry engine, the 2D lines, capture, the ARM7, the window, the pacing wait), timed where each part's work is, kept a row a frame and written when the run ends, with the median, 95th percentile and most on stderr; [../tools/frame_times.py](../tools/frame_times.py) reads the file. It reads only the host's clock, so the frames are the same with it |

**[hw/](hw/hw.h)** — the DS's memory map

| File | What it does |
| --- | --- |
| [hw.h](hw/hw.h), [hw.c](hw/hw.c) | The windows of host memory that `PORT_HW_ADDR()` turns DS addresses into, the accessors of the registers whose accesses act (VCOUNT, the divider, the geometry ports), the geometry command log and its readbacks; an access to POLYGON_ATTR, TEXIMAGE_PARAM or PLTT_BASE right after one to the same port shares its entry, so that SPL's read of POLYGON_ATTR after each particle's write sends no command of 0 (which drew no particle at all), and a send of MI's to a shared entry keeps it ([ADR 0004](../docs/adr/0004-ds-addresses-by-header-translation.md)'s note) |

**[gx/](gx/vram.h)** — VRAM, loads and the screens

| File | What it does |
| --- | --- |
| [vram.h](gx/vram.h), [vram.c](gx/vram.c) | The nine VRAM banks as views of host memory, and applying a new VRAMCNT layout to them; the 2D engines' sources at every line, kept from line to line until the layout changes |
| [vram_layout.c](gx/vram_layout.c) | From VRAMCNT to the slots of every view, as GBATEK and melonDS map them |
| [vram_os.h](gx/vram_os.h), [vram_os.c](gx/vram_os.c) | The operating system's virtual memory calls that alias the banks' pages |
| [vramcnt.c](gx/vramcnt.c) | The SDK's `GX_SetBankFor*` functions, wrapped so that the views follow each VRAMCNT write |
| [gx_load3d.c](gx/gx_load3d.c) | Loads into the 3D engine's memory: textures, texture palettes, the rear plane image |
| [gx_load2d_extpltt.c](gx/gx_load2d_extpltt.c) | Loads of the main engine's extended palettes |
| [gx_load_copy.c](gx/gx_load_copy.c), [gx_internal.h](gx/gx_internal.h) | The copy behind those loads, shared by the GX files only |
| [gxasm.c](gx/gxasm.c) | The geometry FIFO's stores that the SDK wrote in assembly |
| [gx_display.h](gx/gx_display.h), [gx_display.c](gx/gx_display.c) | Which 2D engine feeds which screen, and their power, from POWCNT1 |

**[dma/](dma/dma.c)**, **[mi/](mi/mi_memory.c)**, **[fx/](fx/fx_mtx33.c)**, **[math/](math/math.c)**,
**[crypto/](crypto/rc4.c)** — the SDK's assembly in C

| File | What it does |
| --- | --- |
| [dma/dma.c](dma/dma.c) | DMA done at once by the CPU; asynchronous transfers raise their interrupt; GX command DMA goes to the geometry log |
| [mi/mi_memory.c](mi/mi_memory.c) | CPU copies, fills and compares, in the original's units and order |
| [mi/mi_swap.c](mi/mi_swap.c) | `MI_SwapWord`, for the SDK's spin locks |
| [mi/mi_uncompress.c](mi/mi_uncompress.c) | `MI_UncompressLZ8`, the LZ77 decompressor |
| [fx/fx_mtx22.c](fx/fx_mtx22.c), [fx_mtx33.c](fx/fx_mtx33.c), [fx_mtx43.c](fx/fx_mtx43.c), [fx_mtx44.c](fx/fx_mtx44.c) | The matrix helpers (identity, copy, rotation, scale) that the SDK wrote in assembly |
| [math/math.c](math/math.c) | `MATH_QSort`, with the original's order of comparisons |
| [math/math_pop.c](math/math_pop.c) | `MATH_CountLeadingZerosFunc` |
| [math/sha1_block.c](math/sha1_block.c) | The SHA-1 block function of `MATH_CalcSHA1`, which the random seed uses |
| [crypto/rc4.c](crypto/rc4.c) | `CRYPTO_RC4Encrypt`, for DWC's WDS code |

**[rom/](rom/port_rom.h)**, **[card/](card/port_card.h)**, **[fs/](fs/ovsec.h)** — the ROM, saves, files and overlays

| File | What it does |
| --- | --- |
| [rom/port_rom.h](rom/port_rom.h), [rom/rom.c](rom/rom.c) | Finding the player's ROM (`--rom`, `FRIGATE_ROM`, `orig/`), mapping it, checking it, and what the DS's boot leaves in memory |
| [rom/rom_manifest.h](rom/rom_manifest.h) | What the build knows of the ROM it was built for: the game code, the name, the hashes of the dump and of every file |
| [rom/sha1.h](rom/sha1.h), [rom/sha1.c](rom/sha1.c) | SHA-1 for those checks and the run directory's screen hashes: the ARMv8 SHA instructions where the compiler targets them (every Apple Silicon Mac), the C rounds elsewhere |
| [card/port_card.h](card/port_card.h), [card/backup.c](card/backup.c) | The ARM7's side of the save chip: the 512 KiB flash kept as `IREO.sav` or `IRDO.sav`, written a frame after the game's last write |
| [card/card_rom.c](card/card_rom.c) | `CARDi_ReadRom` and the card's ROM reads, as copies from the mapped dump |
| [card/card_backup_host.c](card/card_backup_host.c), [card/card_backup_calls.h](card/card_backup_calls.h) | The decomp's backup commands compiled through, with the game's buffers kept as whole pointers |
| [card/card_internal.h](card/card_internal.h) | The page buffer that every save read and write passes through, shared by the card files |
| [card/card_spi.c](card/card_spi.c), [card/card_spi.h](card/card_spi.h) | The card slot's SPI bus (`AUXSPICNT`, `AUXSPIDATA`, through the accessors the host headers call) and the cartridge's infrared chip on it, as melonDS 1.1 has them: the chip answers its ID, 0xaa, so the game's genuine-card check passes ([ADR 0011](../docs/adr/0011-cartridge-infrared-chip-as-a-register-model.md)) |
| [fs/fs_overlay.c](fs/fs_overlay.c) | Overlay loading with every overlay linked in: a load sets the overlay's statics back, and zeroes the host memory that stands in for a static the game made a root heap in (`gfl/heap_policy.h`) |
| [fs/ovsec.h](fs/ovsec.h), [fs/ovsec.c](fs/ovsec.c) | Each overlay's writable sections: finding them in the program, the snapshot and the reset |
| [fs/fs_command_default_host.c](fs/fs_command_default_host.c) | The archives' default commands compiled through, with the ROM archive's tables left in the ROM |
| [fs/fs_memfile_host.c](fs/fs_memfile_host.c) | Files over memory (`FS_CreateFileFromMemory`) with whole pointers |

**[pxi/](pxi/pxi_bus.h)** and the ARM7's services

| File | What it does |
| --- | --- |
| [pxi/pxi_bus.h](pxi/pxi_bus.h), [pxi/pxi_fifo.c](pxi/pxi_fifo.c) | The FIFO between the ARM9 and the host's ARM7: each word goes straight to its tag's service |
| [pxi/arm7.h](pxi/arm7.h), [pxi/arm7.c](pxi/arm7.c) | The host ARM7: its boot, and its work at each frame (touch, sound, the microphone) |
| [pxi/pointer.c](pxi/pointer.c) | ARM9 pointers that a protocol carries in 32 bits, recovered whole |
| [pm/pm7.c](pm/pm7.c), [pm/pm_host.c](pm/pm_host.c) | Power: the battery, backlights, screens, the sound amplifier, sleep; the decomp's `pm.c` compiled through |
| [tp/tp_host.h](tp/tp_host.h), [tp/tp7.c](tp/tp7.c) | The touch panel: where the program says the player touches, sampled as the ARM7 does |
| [nvram/nvram_settings.h](nvram/nvram_settings.h), [nvram/nvram7.c](nvram/nvram7.c), [nvram/nvram_host.c](nvram/nvram_host.c) | The firmware's user settings, the MAC address, the touch calibration and the firmware flash; the decomp's `nvram.c` compiled through |
| [rtc/rtc_host.h](rtc/rtc_host.h), [rtc/rtc7.c](rtc/rtc7.c), [rtc/rtc_api.c](rtc/rtc_api.c) | The clock chip: the host's local time or `--fixed-time`, running or pinned; the SDK's RTC API without its assembly wait |
| [reset/reset7.c](reset/reset7.c) | The reset and terminate commands |
| [ctrdg/ctrdg7.c](ctrdg/ctrdg7.c), [ctrdg/svc_cpuset.c](ctrdg/svc_cpuset.c) | An empty GBA slot, and the BIOS's `SVC_CpuSet` its library uses |
| [mic/mic.c](mic/mic.c) | The microphone, which hears silence |
| [snd7/snd7_internal.h](snd7/snd7_internal.h) | What the sound driver's files share, and the list of the ARM7's functions each is written from |
| [snd7/snd7.c](snd7/snd7.c) | The sound driver's thread, written from the ARM7's: the ARM9's commands, its periodic work caught up to the virtual clock, its alarms as the OS core's timed work, and the trace's record of the driver's work in the DS's layout (`port_snd7_trace_record`) |
| [snd7/snd7_seq.c](snd7/snd7_seq.c) | The sequencer: 16 players and 32 tracks reading sequences |
| [snd7/snd7_exchannel.c](snd7/snd7_exchannel.c) | The driver's channels: allocation, envelopes, sweep and LFO; invalidation cancels matching PCM voices' queued starts before their waves are released (`snd_invalidate_wave`) |
| [snd7/snd7_hw.c](snd7/snd7_hw.c), [snd7/snd7_output.h](snd7/snd7_output.h) | The sound unit: 16 hardware channels (their timers, start delays and FIFO as the hardware and melonDS have them) mixed to stereo at GBATEK's widths; its output for `port/audio`, and the driver's trace for `port/app/sound_trace.c` |
| [snd7/snd7_tables.c](snd7/snd7_tables.c) | The decibel and pitch tables |
| [snd7/snd_bank_host.c](snd7/snd_bank_host.c), [snd7/snd_handle.h](snd7/snd_handle.h) | Banks and wave archives on the ARM9's side, with 32-bit handles for host pointers |
| [snd7/snd_command_host.c](snd7/snd_command_host.c), [snd7/snd_interface_host.c](snd7/snd_interface_host.c) | The decomp's command queue and driver interface compiled through |

**[input/](input/port_input.h)**, **[nnsys/](nnsys/reloc.h)**, **[gfl/](gfl/heap_policy.c)**,
**[msl/](msl/fp_fastI.c)**, **[stubs/](stubs/dsprot.c)**

| File | What it does |
| --- | --- |
| [input/port_input.h](input/port_input.h), [input/input.c](input/input.c) | The buttons and the lid, written where the game's `PAD_Read` reads them |
| [nnsys/reloc.h](nnsys/reloc.h), [nnsys/reloc.c](nnsys/reloc.c) | The registry of host-layout trees built for NitroSystem files the DS relocates in place |
| [nnsys/g2d_load.h](nnsys/g2d_load.h), [nnsys/g2d_load.c](nnsys/g2d_load.c), [nnsys/g2d_file.h](nnsys/g2d_file.h) | The G2D file loaders (characters, palettes, cells, multi-cells, animations, fonts) building those trees from the files' DS layouts |
| [nnsys/fnd_hooks.c](nnsys/fnd_hooks.c) | NitroSystem's heap frees, wrapped so that a file's trees go with its memory; the heaps aligned for 64-bit structs; in the sanitized build, the expanded and frame heaps' blocks shown to AddressSanitizer, so that an access outside one is a report, and the one read the game makes past a block as the DS does, whose bytes it shows, let through (`port_heap_ds_read_begin`, `port_heap_ds_read_end`), in every build checked to stay in the game's heaps alive or the main arena ([../docs/sanitizers.md](../docs/sanitizers.md)); a destroyed heap's memory goes back to its owner, but for the host memory kept for a root heap in the game's own, which `gfl/heap_policy.c` poisons until the next heap there |
| [gfl/heap_policy.c](gfl/heap_policy.c), [gfl/heap_policy.h](gfl/heap_policy.h) | The host's heap sizes (`port_heap_size`, `--heap-scale-root`, `--heap-scale-child`), and host memory for the root heaps the game creates in its own (`port_heap_root_memory`): the field's, the trade screen's, GFL's extra heap, kept as the DS keeps the static, zeroed when the static's overlay loads again (`port_heap_root_memory_reset`) and poisoned for AddressSanitizer between two heaps (`port_heap_root_memory_release`) |
| [gfl/particle_work.c](gfl/particle_work.c) | The particle systems' work, sized for 64-bit structs |
| [gfl/gfl_frame.h](gfl/gfl_frame.h), [gfl/gfl_use_host.c](gfl/gfl_use_host.c) | GFL's frame count for replays; the decomp's `gfl_use.c` compiled through to read it |
| [gfl/random_seed.h](gfl/random_seed.h), [gfl/random_seed.c](gfl/random_seed.c), [gfl/gf_standard_host.c](gfl/gf_standard_host.c) | The seeds of the game's random numbers: GFL's at start-up and each battle's, which `buildSeed` hashes from the DS's timing and stack, which no host has. An input file's `seed` lines give the DS's, which the game takes in place of the ones it builds, in order; `run.txt` lists every seed a run used. The decomp's `gf_standard.c` is compiled through `gf_standard_host.c`, whose `buildSeed` runs the decomp's and then sets up the seed `random_seed.c` gives ([ADR 0022](../docs/adr/0022-the-ds-random-seeds-given-by-input-files.md)) |
| [gfl/random_state.h](gfl/random_state.h) | The handles of GFL's random state for `--run-dump` (`port/app/run_dir.c`): the addresses of `gfl_use.c`'s `sWork` (GFL's linear generator) and `gf_standard.c`'s `sGFLStd` (the Mersenne Twister), statics that point at memory GFL allocates, defined in `gfl_use_host.c` and `gf_standard_host.c` and read with `**`, so that the oracle's sequences compare the state with the DS's |
| [msl/fp_fastI.c](msl/fp_fastI.c) | MSL's runtime division `_u32_div_f`, which the game's C calls by name: the quotient and the remainder as the ARM routine returns them, the dividend for a divisor of 0 |
| [stubs/dsprot.c](stubs/dsprot.c) | DS Protect's checks, passing as on a genuine cartridge |
| [stubs/extras_string.c](stubs/extras_string.c) | MSL's ASCII-only case-blind comparison extensions for GameSpy's server browser |
| [stubs/wstring_host.c](stubs/wstring_host.c) | Original MSL string scan and copy, compiled with private names for DWC authentication's 16-bit code units |

**[host/](host/host.h)** — the host operating system

| File | What it does |
| --- | --- |
| [host/host.h](host/host.h) | What the platform layer, the program and the tests ask of the host's system, the same on every host |
| [host/host_posix.c](host/host_posix.c) | macOS and Linux: `mmap` and `mprotect`, SIGSEGV and SIGBUS, and SIGFPE, SIGILL and SIGTRAP reported at the instruction's address, with the function that holds it where `dladdr` names one, on an alternate stack, `backtrace` and `dladdr`, SIGALRM for the watchdog, the image's data and globals (Mach-O's `__DATA` and export trie, ELF's symbols), the monotonic clock (`CLOCK_UPTIME_RAW`, `CLOCK_MONOTONIC`), pthreads at the user-interactive QoS on macOS, a mutex and a condition variable for the signals |
| [host/host_windows.c](host/host_windows.c) | Windows: `VirtualAlloc` and `VirtualProtect`, a vectored exception handler that moves the report to a stack of its own (a stack overflow, a memory fault at the seam's inaccessible pages or in the program's own code, and on a fiber any exception that ends a program: a division by zero, an illegal instruction, a breakpoint), the unhandled-exception filter, which reports on the faulting stack what nobody handled off the fibers (a division by zero, an illegal instruction, a breakpoint, a memory fault in another module's code), unwinding by the image's unwind tables, the COFF symbol table, a watchdog thread, the PE image's writable sections, the performance counter as the clock, Win32 threads with a slim reader/writer lock and a condition variable for the signals; and, before `main()`, binary stdout and stderr and no error dialogs |

## How a decomp file is adapted

Four patterns, which every module uses:

| Pattern | What it looks like | Examples |
| --- | --- | --- |
| Left out and rewritten | The decomp file is in a `replaced.txt`; the module defines the same functions | `os/os_tick.c`, `pxi/pxi_fifo.c`, `card/card_rom.c`, `mi/mi_memory.c`, `gx/gx_load3d.c` |
| Compiled through a host file | `*_host.c` renames the functions that can't run on a 64-bit host (`#define F F_ds`), includes the decomp's file, and writes them again with access to its statics, or wraps the decomp's function (`gf_standard_host.c`'s `buildSeed`) | `os/os_thread_host.c`, `card/card_backup_host.c`, `fs/fs_memfile_host.c`, `pm/pm_host.c`, `snd7/snd_interface_host.c`, `gfl/gfl_use_host.c`, `gfl/gf_standard_host.c` |
| Compiled in place, functions renamed | A CMake fragment renames a file's exported functions to `port_sdk_<name>`, and the module defines the original name as a wrapper | `gx/vramcnt.c` ([../cmake/gx.cmake](../cmake/gx.cmake)), `nnsys/fnd_hooks.c` and `nnsys/g2d_load.c` ([../cmake/nnsys.cmake](../cmake/nnsys.cmake)) |
| Assembly behind `#ifndef PLATFORM_PC` | The SDK file keeps its MWCC `asm` function for the ROM build; the module has the C version | `fx/fx_mtx*.c`, `crypto/rc4.c`, `math/math_pop.c`, `gx/gxasm.c` |

Every change these patterns need outside `port/` (the `#ifndef PLATFORM_PC` guards) has a row in
[../docs/game-changes.md](../docs/game-changes.md).

## Names

- `*7.c` is the host ARM7's side of a PXI service (`pm7.c`, `tp7.c`, `nvram7.c`, `rtc7.c`, `reset7.c`, `ctrdg7.c`,
  `snd7*.c`), started by `pxi/arm7.c`. `*_host.c` compiles a decomp file through, as above. `*_internal.h` is shared
  inside one module only.
- A header that says "Plain C types only" includes none of the decomp's headers, so [../app](../app/README.md),
  [../gfx](../gfx/README.md) and [../audio](../audio/README.md), which don't see them, can include it.
- Public functions are `port_<module>_*`. Inside the OS core, `port_osi_*` (after the SDK's `OSi_`) is shared between
  the files of `os/` only (`os_internal.h`); inside the sound driver, `snd7_*` is shared between the files of `snd7/`.
- `vram_os.c` in `gx/` holds the virtual memory calls of the VRAM's aliasing backends; it has nothing to do with `os/`.
- `host/` holds the operating system's services that the platform layer, the program and the tests share where macOS,
  Linux and Windows differ, one file a host (`host_posix.c`, `host_windows.c`), each compiled to nothing on the others'
  hosts: pages, faults, backtraces, names, the watchdog, the clock, threads, directories, the environment. Code that
  needs one calls it rather than an operating system's API. A module whose own feature is a host's keeps its `#ifdef`s,
  since the seam would only move them ([ADR 0013](../docs/adr/0013-windows-build-with-msys2-clang64.md)):
  - `card/backup.c`: the saves' folder (`%APPDATA%`, `~/Library/Application Support`, the XDG data folder) and the
    folders it makes (0755 on macOS and Linux), the atomic replacement (`_commit` and `MoveFileExA`, or `fsync` and
    `rename`), the listing of its folder for other saves that the game won't see by their names, a path's separators,
    and links followed on macOS and Linux only;
  - `gx/vram_os.c`: the aliasing backends, POSIX shared memory and Mach's on macOS, `memfd_create` on Linux, none yet
    on Windows (its page size is the seam's);
  - `rom/rom.c`: the dump mapped on macOS and Linux, read into memory on Windows;
  - `rtc/rtc7.c`: the reentrant local time, `localtime_r` or `localtime_s`;
  - `fs/ovsec.c`: the overlays' sections, by the object format (Mach-O, ELF, COFF) rather than by the system;
  - `os/fiber.c`: LeakSanitizer's calls, which Windows' ASan lacks;
  - `../app/message_log.c` and `../app/window_icon.c`: the macOS app's log, and the window icon that macOS leaves to
    the Dock.

Some headers are named for the interface rather than the file that implements it:

| Header | Implemented in |
| --- | --- |
| [os/port_app.h](os/port_app.h) (the program's options and hooks) | `os/app_options.c` |
| [os/port_os.h](os/port_os.h) (the OS core) | `os/core.c`, and `os/os_arena.c` (`port_os_set_main_arena_size`, `port_os_main_arena_holds`), `os/memory_map.c` (the IRQ check word) |
| [card/port_card.h](card/port_card.h) (the save chip) | `card/backup.c` |
| [../include/port_host.h](../include/port_host.h)'s `port_hw_auxspicnt` and `port_hw_auxspidata` (the card slot's SPI registers) | `card/card_spi.c` |
| [input/port_input.h](input/port_input.h) | `input/input.c` |
| [rtc/rtc_host.h](rtc/rtc_host.h) | `rtc/rtc7.c` |
| [tp/tp_host.h](tp/tp_host.h) | `tp/tp7.c` |
| [pxi/arm7.h](pxi/arm7.h) (what the ARM7's services share) | `pxi/arm7.c` and each service |
| [snd7/snd_handle.h](snd7/snd_handle.h) | `snd7/snd_bank_host.c` |
| [gfl/gfl_frame.h](gfl/gfl_frame.h) | `gfl/gfl_use_host.c` |
| [gfl/random_state.h](gfl/random_state.h) | `gfl/gfl_use_host.c`, `gfl/gf_standard_host.c` |
| [host/host.h](host/host.h) (the host's system) | `host/host_posix.c` on macOS and Linux, `host/host_windows.c` on Windows |
| [../include/port_host.h](../include/port_host.h)'s `port_heap_size`, `port_heap_root_memory` and particle hooks | `gfl/heap_policy.c`, `gfl/particle_work.c` |
| [../include/port_host.h](../include/port_host.h)'s `port_heap_ds_read_begin` and `port_heap_ds_read_end` (a read the DS makes past a heap block) | `nnsys/fnd_hooks.c` |

## Adding a module

A new SDK module gets a directory named after it, its C files (which the build's glob picks up), a `replaced.txt` for
the decomp files it takes over, a CMake fragment in [../cmake/](../cmake/README.md) for its tests and per-file options,
its tests in [../tests/](../tests/README.md), a row in the table above and one for each file in
[Files of each module](#files-of-each-module), and a row in the feature map of
[../docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md#feature-map) for the game functionality it serves
([../CLAUDE.md](../CLAUDE.md#code-that-modders-can-read)).

### GameSpy functions absent from the ROM

[stubs/gbucket.c](stubs/gbucket.c) provides only terminating diagnostics for `NewBucketSet`, `FreeBucketSet` and
`DumpBucketSet`, using the actual prototypes of `lib/dwc/include/dwc/gbucket.h`. The imported `gstats.c` references
these SDK functions from code discarded by the DS linker; no implementation or symbol address exists in the ROM.
Host links retain the references. Each call reports its own name and "not in ROM", then exits with status 70,
including `FreeBucketSet(NULL)`. No bucket-set behavior or DS address is invented. The `hal_gbucket_*` subprocess
tests check all three calls on every host; the ordinary generated ROM-symbol traps remain unchanged.
