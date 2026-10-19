# Architecture

How Frigate is put together as it stands, for anyone who reads or changes its code: the layers, how one frame runs,
the rules at the boundaries, and a map from each part of the game to the code that serves it and the tests that cover
it. [PLAN.md](PLAN.md) is the plan of record, with the parts not built yet; this document describes what exists. The
words it uses are in the [glossary](GLOSSARY.md).

Frigate is the decomp's C compiled for the host. The game's code (`src/`), Game Freak's library and Nintendo's
libraries (`lib/`) compile unchanged with the host's clang; a platform layer in `port/` gives them the parts of the DS
that a host doesn't have, and the program around it opens a window, plays sound and reads the controls with SDL3.
Nothing runs or translates the ROM's ARM code: the ROM is only where the game's files come from.

## The layers

```
 the player
 ── port/app ──────── the program: options, the window, keyboard, mouse and gamepads, presenting and pacing,
 │                    replays, the oracle's run directory and sound trace, the test launcher, the macOS app
 ── port/gfx ──────── renderers: the 2D engines line by line, the geometry engine, the 3D rasterizer, display capture
 ── port/audio ────── the sound device and the WAV dump
 ══ the decomp's C ══ src/ (src/system, src/gfl, src/ovNNN): the game, compiled unchanged
 ══                ══ lib/: NitroSDK and TwlSDK, NitroSystem, SPL, DWC and the rest, compiled unchanged
 ── the SDK surface ─ the SDK functions, registers and fixed addresses the platform layer takes over
 ── port/hal ──────── the platform layer: threads and time, memory map, VRAM, DMA, files and overlays, the ROM and
 │                    saves, the ARM7's services (sound, touch, clock, power, firmware), input, stubs, and
 │                    hal/host, the operating system's services where macOS, Linux and Windows differ
 ── SDL3 ──────────── window, renderer, input devices, audio stream
```

| Layer | Where | What it is | It depends on |
| --- | --- | --- | --- |
| The decomp's C | `src/`, `lib/`, `include/` (upstream's) | The game and its libraries, as upstream writes them; the host build compiles every C file, matching or not, except the ones the platform layer replaces ([../hal/replaced.txt](../hal/replaced.txt)). The game's `main()` is renamed `NitroMain` | the SDK surface; and, in the host build only, the test launcher's seam, a `PLATFORM_PC` block of `src/ov035/event_mapchange.c` that calls into `port_launcher` ([../include/port_host.h](../include/port_host.h)) |
| The SDK surface | the boundary between `lib/` and `port/hal` | What the game's C reaches that a host doesn't have: NitroSDK's assembly, its hardware-bound files (CPU state, interrupts, timers, DMA, card), the DS's fixed addresses and registers, and the ARM7 behind the PXI FIFO. Everything else of the SDK stays the decomp's C ([ADR 0005](adr/0005-platform-boundary-as-built.md)) | — |
| The platform layer | [port/hal](../hal/README.md) | Host C for that surface, one directory per SDK module or ARM7 service, compiled against the decomp's headers | the decomp's headers, the host's C library, and the operating system through [port/hal/host](../hal/host/host.h) (pages, faults, backtraces, the watchdog), one file a host |
| The renderers and sound output | [port/gfx](../gfx/README.md), [port/audio](../audio/README.md) | What the display hardware and the speakers make of the state the game leaves; plain C, without the decomp's headers | the platform layer's plain-C interfaces |
| The program | [port/app](../app/README.md) | `main()`, the options, the window, the controls, presentation and pacing, replays, the oracle's run directory and the sound driver's trace, and the test launcher (`--launch`), which opens the field menu's apps for the tests through the game's own app call and so includes the decomp's headers | everything above, SDL3 |
| SDL3 | an installed SDL3 3.4, or the pinned release | Window, renderer, input devices, audio stream | — |

The build ([../CMakeLists.txt](../CMakeLists.txt), [../cmake/](../cmake/README.md)) makes five object libraries and
links them into one program, `frigate-black2` or `frigate-white2`: `pokebw2_game` (the decomp's C), `port_hal`,
`port_gfx`, `port_app` (with `port/audio` and the oracle's input file reader) and `port_launcher` (the test launcher,
compiled against the decomp's headers, which every program that links the game links too, since the game's seam in
`src/ov035/event_mapchange.c` calls it). At each configure it generates, in `build/port/gen`
([../tools/gen_host_tree.py](../tools/gen_host_tree.py)), the source list, an `include/` mirror without the decomp's C
library headers, and host variants of the SDK headers in which every DS address goes through `PORT_HW_ADDR()`
([ADR 0004](adr/0004-ds-addresses-by-header-translation.md)); at each build, a trap stub for every DS function or data
object the game references that has no C yet ([../tools/gen_stubs.py](../tools/gen_stubs.py)). Every decomp file gets
[../include/port_host.h](../include/port_host.h) force-included, and is compiled with MWCC's language model: signed
`char`, int-sized enums, wrapping arithmetic, no strict aliasing, zeroed locals (PLAN.md §4.4).

## How a frame runs

### Boot

1. **`main()`** ([../app/main.c](../app/main.c)) records the options for every module (`port_app_set_arguments`,
   [../hal/os/port_app.h](../hal/os/port_app.h)), checks them, and opens the replay and the run directory when asked.
2. It calls `port_hw_init` ([../hal/hw/hw.h](../hal/hw/hw.h); the DS's memory windows are static arrays, so it has
   nothing to do yet), makes the trap placeholders inaccessible (`port_trap_protect_data`,
   [../hal/trap.h](../hal/trap.h)), opens the window and the sound device (unless `--headless`), sets the input at rest,
   attaches the scanline hook ([../gfx/screens.c](../gfx/screens.c)) and the geometry engine
   ([../gfx/ge/ge_hal.c](../gfx/ge/ge_hal.c)), and adds its frame hooks: the 3D swap, then its own `AppFrame`.
3. It finds, maps and checks the player's ROM (`port_rom_boot`, [../hal/rom](../hal/rom/port_rom.h)) and places what
   the DS's boot leaves in main memory.
4. **`port_os_run(NitroMain)`** ([../hal/os/core.c](../hal/os/core.c)) resets the OS layer, sets the registers as the
   DS enters `main()`, and starts the dispatcher, which runs the game's `main()` on a fiber as the DS's launcher thread.
5. The game's `main()` (`src/system/main.c`) runs as on the DS: `SystemInit()` (`src/system/machine_use.c`) calls
   `OS_Init`, whose `PXI_Init` boots the host ARM7 ([../hal/pxi/arm7.c](../hal/pxi/arm7.c), which adds the ARM7's
   frame hook) and whose `CARD_Init` starts the save chip's service ([../hal/card/backup.c](../hal/card/backup.c),
   which adds the save's frame hook); then the tick and alarms, the clock chip, GX, the file system, NitroSystem's VRAM
   transfers and the sound amplifier; `GFLAppInit()` (`src/system/gfl_use.c`: heaps, archives, input, overlays,
   processes, tasks, fades, backup, text, random numbers); the interrupt handlers; and `GameInit()`, which loads
   overlays 12 and 27, queues the title's process, opens the save and starts sound and cries.

### One frame

The game's loop (`src/system/main.c`) runs `GFLAppUpdate()` (input, then the current screen's process), `GameUpdate()`
(brightness, wipes, sound), asks for the 3D buffers' swap, and waits for the VBlank in `OS_WaitIrq`. What happens
while it waits is the platform layer's:

1. **The thread sleeps.** The decomp's own scheduler (`lib/nitro/src/os/os_thread.c`) runs whatever thread is ready
   (the card's thread while a ROM or save read is in flight), and finally the idle thread, which calls `OS_Halt`. On
   the host that yields to the dispatcher. The game puts its sound loader (`g_GFLSndThread`, `src/system/snd_sys.c`)
   to sleep just before this wait and wakes it after `GameUpdateVBlank()` (`src/system/main.c`), so the loader never
   runs during the wait: at its priority, 17, below `main()`'s 16, it runs only when `main()` waits for something else
   during the frame.
2. **The virtual clock moves** (`Halt` in [../hal/os/core.c](../hal/os/core.c)), event by event: 2,130 cycles a line,
   263 lines a frame, 59.8261 frames a second. Each line updates VCOUNT and DISPSTAT; each HBlank raises its interrupt
   when DISPSTAT asks, as DISPSTAT's HBlank flag sets, so that a handler sees the flag; line 192 starts the VBlank. Work
   another module set for a cycle (`port_os_set_timed_work`) runs when the clock reaches it: the sound driver's next
   alarm (`SND_SetupAlarm`'s, which NitroSystem's stream player sets; [../hal/snd7/snd7.c](../hal/snd7/snd7.c)), so that
   its word reaches the ARM9 when it goes off, as on the DS. Nothing reads the host's time, so a run is the same every
   time.
3. **Each visible line is drawn** by the scanline hook ([../gfx/screens.c](../gfx/screens.c)): before line 0 the 3D
   frame swapped at the last VBlank is rasterized ([../gfx/ref/ds3d.c](../gfx/ref/ds3d.c), in bands of lines on the
   renderer's workers too), unless everything it is drawn from is what the last picture was drawn from
   ([../gfx/ge/ge_hal.c](../gfx/ge/ge_hal.c)); then for each 2D engine the registers, palettes, OAM and VRAM are
   gathered (`port_vram_fill_source`, [../hal/gx/vram.c](../hal/gx/vram.c)), the line is drawn
   ([../gfx/ref/ds2d.c](../gfx/ref/ds2d.c)), engine A's line feeds display capture
   ([../gfx/capture.c](../gfx/capture.c)), and each line goes to the screen POWCNT1 gives its engine. The game's HBlank
   tasks run between lines, so raster effects change registers as on the DS.
4. **At line 192 the frame hooks run**, on the dispatcher's stack, in the order they were added:
   - the 3D swap (`port_g3d_vblank`, [../gfx/ge/ge_hal.c](../gfx/ge/ge_hal.c)): the polygon list closed by
     SWAP_BUFFERS becomes next frame's;
   - the program's `AppFrame` ([../app/main.c](../app/main.c)): it polls the window and devices into the buttons and
     touch screen ([../app/present.c](../app/present.c), [../hal/input](../hal/input/port_input.h),
     [../hal/tp](../hal/tp/tp_host.h)), or plays a replay's frame instead ([../app/replay.c](../app/replay.c)),
     writes the run directory's row, presents both screens, stops at `--frames N`, paces to 59.8261 frames a second
     unless `--headless`, ends the frame's row of `--frame-times`, and kicks the watchdog;
   - the ARM7's frame ([../hal/pxi/arm7.c](../hal/pxi/arm7.c)): touch sampling, the sound driver catching up to the
     clock ([../hal/snd7/snd7.c](../hal/snd7/snd7.c)), whose samples go to [../audio](../audio/README.md), and the
     microphone;
   - the save's frame ([../hal/card/backup.c](../hal/card/backup.c)): the 512 KiB image is written a frame after the
     game's last write.
5. **The VBlank interrupt is taken**: the game's VBlank handler (`src/system/main.c`) runs, the threads waiting in
   `OS_WaitIrq` wake, and the game's `main()` goes on with `GameUpdateVBlank()` (VRAM transfers, GFL's frame count).

Interrupts are taken only at defined points: a halt, interrupts enabled again with one pending, a modelled busy wait
(`OS_GetTick`, `OS_SpinWaitSysCycles`, `SVC_WaitByLoop`), and `port_os_poll_irq`
([../hal/os/port_os.h](../hal/os/port_os.h)).

### Where a frame's time goes

The DS gives each frame 16.715 ms. Nearly all the host's time for one goes to the renderers, not to the game: the game's
own C on its fibers takes about 0.1 ms a frame outside loading. `--frame-times FILE` writes each frame's host time with
and without the pacing wait and its parts, timed where their work is ([../hal/os/frame_time.h](../hal/os/frame_time.h)),
and [../tools/frame_times.py](../tools/frame_times.py) reports a route scene by scene
([../README.md](../README.md#quick-start)); the `frame_times_*` tests hold the heaviest scenes to half a frame in an
optimized build. The hot paths, and their cost on an M2 MacBook Air (Black 2, headless, each frame the least of 3 runs,
medians in ms; the last row is the frame without the pacing wait, median and 95th percentile; before this work, on
2026-10-19, the five frames were 4.24 and 4.45, 2.53 and 3.95, 3.78 and 4.68, 0.82 and 1.23, 2.54 and 2.69 ms;
White 2's title, start menu, bag and card are within 0.1 ms of Black 2's, and it has no New Game route):

| Part | Where its time goes | Title's 3D | Start menu | New Game's 3D flight | Juniper's intro | Bag and card |
| --- | --- | --- | --- | --- | --- | --- |
| 3D frame | [../gfx/ref/ds3d.c](../gfx/ref/ds3d.c), before line 0, in bands of lines on the game's thread and the workers ([../gfx/workers.c](../gfx/workers.c), 4 bands on an M2): `DrawPixel`, `DrawSpan` and `PerspValue` for every pixel of every polygon (the title's Kyurem is about 1,160 polygons and 170,000 pixels a frame, 3.5 screens), `Texel` and `PaletteRead` for its textures; then edge marking, fog and anti-aliasing over the picture. The DS draws the last list every frame until the next swap; a frame whose list, latched registers and texture memory are all the last picture's keeps that picture instead ([../gfx/ge/ge_hal.c](../gfx/ge/ge_hal.c)), so a still 3D scene costs a comparison of its texture memory | 0.98 | 0 | 0.84 | 0.08 | 0 |
| Geometry engine | [../gfx/ge/ge.c](../gfx/ge/ge.c): the logged writes decoded and fed straight to the executor (`Decode`, `Feed`; queued only behind a SWAP_BUFFERS that waits for the VBlank), the matrices, lighting and the clipper (`RunCommand`, `Vertex`, `Submit`) | 0.43 | 0 | 0.59 | 0.01 | 0 |
| 2D lines | [../gfx/ref/ds2d.c](../gfx/ref/ds2d.c) (`DrawTextBg`, `DrawObjs`, `Compose`, whose colour effects run only on a line that can blend, in a loop of the line's effect) and each line's sources, [../hal/gx/vram.c](../hal/gx/vram.c)'s `port_vram_fill_source`, which keeps them until the VRAM layout changes | 0.36 | 0.49 | 0.14 | 0.39 | 0.41 |
| Capture | [../gfx/capture.c](../gfx/capture.c), every line while the game captures (the title's picture into VRAM D): a line in one pass of its source mode, written at once | 0.08 | 0 | 0 | 0 | 0 |
| ARM7 | [../hal/snd7](../hal/snd7/snd7_internal.h): the sound driver catching up and the mixer, at each VBlank and at its alarms | 0.02 | 0.02 | 0.13 | 0.09 | 0.10 |
| Window | [../app/present.c](../app/present.c): SDL's texture upload and present, about 0.2 ms with a window (none headless) | 0 | 0 | 0 | 0 | 0 |
| The rest | The game's C on its fibers, the dispatcher, the interrupt handlers, the other frame hooks | 0.08 | 0.04 | 0.09 | 0.05 | 0.04 |
| The frame | | 1.97, 2.04 | 0.56, 1.86 | 1.80, 2.08 | 0.63, 0.82 | 0.57, 0.70 |

Paced, the same frames cost more: the core slows down while the program sleeps through the rest of each frame, and the
shorter the frame's work, the more it is slowed. On SDL's dummy video driver, which presents in software, the title's
frame takes 5.2 ms paced, 1.4 of them presenting, where it took 6.7 before (4.0 drawing its 3D, on one thread); a 2D
scene's frame took as long as before, what it no longer does being given back to the slower clock. The x86_64 slice
under Rosetta took about 1.7 times the arm64 time before this work; it hasn't been measured since.

### Threads and stacks

Frigate runs the game on one host thread. Only the 3D renderer's workers run beside it, drawing their bands of the 3D
frame's lines while the game's thread draws its own ([ADR 0016](adr/0016-3d-frame-in-bands-on-worker-threads.md)). On
macOS every one of them runs at the user-interactive QoS that an app's main thread gets, however the program was
started (`port_host_thread_set_interactive`, [../hal/host](../hal/host/host.h)), so that a run from a shell or a test is
scheduled as the app is.

| What | Where it runs | Code |
| --- | --- | --- |
| The dispatcher, interrupt handlers, frame hooks, the scanline hook | the host thread's own stack | [../hal/os/core.c](../hal/os/core.c) |
| The game's `main()`, the DS's launcher thread | a fiber with an 8 MiB stack | `src/system/main.c` |
| Every SDK thread: the idle thread, the card's thread, the sound loader `g_GFLSndThread` (`src/system/snd_sys.c`), DWC's | fibers with 256 KiB stacks, switched by the decomp's own scheduler | [../hal/os/fiber.c](../hal/os/fiber.c) over minicoro |
| The ARM7's services | inside the ARM9's `PXI_SendWordByFifo` call, on the sender's fiber, and in the ARM7's frame hook | [../hal/pxi/pxi_fifo.c](../hal/pxi/pxi_fifo.c), [../hal/pxi/arm7.c](../hal/pxi/arm7.c) |
| The sound driver (the DS's ARM7 sound thread) | no thread: it catches up to the virtual clock at each VBlank, when the ARM9 sends it a command, and at its next alarm, which the OS core runs as timed work on the dispatcher's stack | [../hal/snd7/snd7.c](../hal/snd7/snd7.c) |
| The 3D renderer's workers | Up to 7 threads of their own (`--render-threads`; as many as the host has performance cores, the game's thread included, by default), started at boot at macOS's user-interactive QoS, with the asynchronous signals blocked; each draws its band of the frame's lines, then sleeps until the next frame. No game code, platform code or hook runs there | [../gfx/workers.c](../gfx/workers.c), [../gfx/ref/ds3d.c](../gfx/ref/ds3d.c), [../hal/host](../hal/host/host.h) (threads and signals) |
| SDL's audio device thread | SDL's own thread, which drains the stream that Frigate fills at each VBlank; no Frigate code runs there (the stream has no callback) | [../audio/audio.c](../audio/audio.c) |
| The watchdog, on Windows | A thread of its own that waits for each frame's kick and, when one doesn't come in time, reports where the main thread is and ends the program; no game code runs there (macOS and Linux use `SIGALRM`) | [../hal/host/host_windows.c](../hal/host/host_windows.c) |

### How a run ends

`port_os_run` returns when a frame hook stops it (`--frames`, the window closed, the test launcher once its launch ends,
a write to the run directory that failed), when the game's `main()` returns, or when every thread waits for good;
`main()` then writes the screenshot and closes the sound, the sound trace, the run directory, the replay and the window.
The exit status says why a run ended, as `--help` lists too (`sExitStatuses` in [../app/main.c](../app/main.c));
[MODDING.md](MODDING.md#when-the-game-stops) says how to read each and what to do about it:

| Status | Why | Where |
| --- | --- | --- |
| 0 | The run ended normally | [../app/main.c](../app/main.c) |
| 1 | The program couldn't go on, each cause with a `port:` line: the window didn't open; a write to the run directory failed during the run (a frame's row, a screen's PNG file or `memory.txt`, which stops the run at once); a write to the sound trace or of `--frame-times`' file failed; the host ran out of memory (an arena, the geometry engine or its command log, the files over memory) or a module found a build that can't work (no overlay sections, an archive or a file over memory read through an address cut to 32 bits); or `--launch`'s launch didn't finish (the run ended first, the game never asked for the field, the test launcher stepped aside because the field's process table has C, or an app wanted the field) | [../app/main.c](../app/main.c), [../app/launcher.h](../app/launcher.h), and the module named in the message |
| 2 | Bad options, among them a run directory, WAV dump or sound trace that can't be opened and a `--launch` list the test launcher can't read; and a `--launch` app that the save's field menu doesn't offer | [../app/main.c](../app/main.c), [../app/launcher.h](../app/launcher.h) |
| 65 | A save file Frigate can't use (the wrong size, a directory, another emulator's format) | [../hal/card](../hal/card/port_card.h) |
| 66 | No usable ROM | [../hal/rom](../hal/rom/port_rom.h) |
| 67 | The PXI bus's arena check | [../hal/pxi/pointer.c](../hal/pxi/pointer.c) |
| 70 | The game reached code or data that has no C yet (a trap stub) | [../hal/trap.c](../hal/trap.c) |
| 71 | The game stopped itself (`OS_Terminate`); also a DMA call that the host's card never makes (`MIi_CardDmaCopy32`) | [../hal/os/os_terminate.c](../hal/os/os_terminate.c), [../hal/dma/dma.c](../hal/dma/dma.c) |
| 72 | Every thread waits for an interrupt that nothing can raise | [../hal/os/core.c](../hal/os/core.c) |
| 73 | The watchdog: a frame took longer than `--watchdog` seconds of host time | [../app/main.c](../app/main.c) |
| 74 | The game reset the console (`OS_ResetSystem`), which Frigate can't do in place yet | [../hal/os/os_terminate.c](../hal/os/os_terminate.c) |
| a signal (`SIGABRT`, `SIGSEGV`, `SIGBUS`); on Windows 3 or an exception's status | A check of the platform layer failed (`port: os: ...`, `port: fiber: ...`, `port: PXI: ...` and the like, then `abort()`, which exits with 3 on Windows), or a real crash (`port: crash: signal N at address ...`, on Windows `port: crash: exception 0xc0000005 at address ...` and the exception's status, with a backtrace; Windows reports an illegal instruction or a division by zero too, and a fiber's guard ends it with a warning's status, 0x80000001) | the module named in the message; [../hal/trap.c](../hal/trap.c) reports crashes, through [../hal/host](../hal/host/host.h) |

## The rules at the boundaries

The decisions behind them are architecture decision records ([adr/](adr/README.md)); the rules for every change are in
[../CLAUDE.md](../CLAUDE.md).

- **The game's code stays the decomp's.** Outside `port/`, only three kinds of change are allowed (a fix that compiles
  to the same bytes under MWCC, a hook macro that compiles away, a rare `#ifdef PLATFORM_PC` block), each proven
  neutral for the ROM build and listed in [game-changes.md](game-changes.md). Both ROMs still build byte for byte from
  this tree. Decomp files are never copied into `port/` ([ADR 0001](adr/0001-repository-and-upstream-sync.md)).
- **Native means compiled from the C.** No ARM code from the ROM runs, and there is no CPU emulator. A function with
  no C yet is a trap stub that stops the game with its name; the C comes from the decomp, never from a stand-in in
  `port/`. The ROM is the player's, read at run time; nothing from it is committed (PLAN.md §1).
- **The SDK's own C stays wherever it can run on a host**; the platform layer replaces assembly, hardware-bound files
  and the ARM7 ([ADR 0005](adr/0005-platform-boundary-as-built.md)). The root
  [../hal/replaced.txt](../hal/replaced.txt) lists the whole modules and assembly-only files the host build leaves
  out, and a module's own `replaced.txt` the rest. The SDK's MI, FX, MATH and CRYPTO assembly that the platform layer
  rewrote is tested against the ROM's own code ([../tests/asm_oracle](../tests/asm_oracle/README.md)); the OS core's
  (contexts, the CPSR, the halt, the interrupt handler) is tested by scenario ([../tests/os](../tests/os)), and
  `gxasm.c`'s FIFO stores by the GX tests ([../tests/gx](../tests/gx)).
- **DS addresses go through the header translation** ([ADR 0004](adr/0004-ds-addresses-by-header-translation.md)):
  the build rewrites the SDK headers' fixed addresses into host memory windows, and the registers whose accesses act
  into accessor calls.
- **The ARM7 is replaced at the PXI protocol**, so the ARM9's libraries talk to it as to the DS's
  ([ADR 0005](adr/0005-platform-boundary-as-built.md)), and the sound driver is C written from the ARM7's code
  ([ADR 0006](adr/0006-sound-driver-as-built.md)).
- **Files the DS relocates in place get host structs**: NitroSystem's G2D files are rebuilt as host-layout trees
  ([ADR 0007](adr/0007-nitrosystem-files-in-host-structs.md)), and particle systems get work sized for 64-bit structs
  from the platform layer ([ADR 0008](adr/0008-particle-work-from-the-platform-layer.md)).
- **Determinism**: one host thread for the game, the decomp's scheduler, a virtual clock, interrupts at defined points,
  and `--fixed-time` for the clock, so a replay gives the same frames and saves every time. The 3D renderer's workers
  draw only the bands of its frame, which make the same picture however many draw it
  ([ADR 0016](adr/0016-3d-frame-in-bands-on-worker-threads.md)).
- **Plain-C interfaces**: `port/gfx`, `port/audio` and `port/app` never include the decomp's headers, except Frigate's
  test launcher (`port/app/launcher.c`, the `port_launcher` library), which is test-only and goes when overlay 36 has C
  ([ADR 0012](adr/0012-test-launcher-for-the-field-menus-apps.md)); a platform-layer header they use says
  "Plain C types only".
- **The controls**: gamepads by position, one pad at a time
  ([ADR 0009](adr/0009-gamepads-by-position-one-pad-at-a-time.md)).
- **The macOS app** is a signed copy of the program, ad hoc and never notarized
  ([ADR 0003](adr/0003-ad-hoc-signing-no-apple-developer-account.md),
  [ADR 0010](adr/0010-macos-app-as-a-signed-copy-of-the-program.md)).
- **melonDS is a test oracle only**, never linked into or shipped with Frigate
  ([../tools/oracle](../tools/oracle/README.md)).

## Feature map

Each part of the game, the decomp's code that implements it, the port's code that serves it, and the tests that cover
it. "Whole program" tests are the replays and oracle sequences, which run the game itself
([../tests/README.md](../tests/README.md)). A trap stub stops the game at the field today (overlay 36, the field
engine, is still being decompiled upstream), so the features past it haven't run yet, but for the field menu's apps,
which Frigate's test launcher opens in the field's place for the tests.

| Feature | Game code (decomp) | Port modules | Tests | Where it stands |
| --- | --- | --- | --- | --- |
| **Boot and the main loop** | `src/system/main.c` (the loop), `src/system/machine_use.c` (`SystemInit`), `src/system/gfl_use.c` (`GFLAppInit`, the VBlank tasks, the frame count), `src/gfl/procsys.c` (processes: a screen's init, main and exit), `src/gfl/tcb.c` (task lists) | [app/main.c](../app/main.c), [hal/os](../hal/os/port_os.h) (dispatcher, virtual clock), [hal/os/os_init.c](../hal/os/os_init.c), [hal/rom](../hal/rom/port_rom.h), [hal/pxi/arm7.c](../hal/pxi/arm7.c), [hal/ctrdg](../hal/ctrdg/ctrdg7.c) (an empty GBA slot at boot), [tools/gen_host_tree.py](../tools/gen_host_tree.py) (the source list and the host variants of the headers) | `smoke_reaches_game_main`, `os_*`, `replay_start_menu`, `oracle_opening` | Runs from power-on through the logos, the opening, the title screen and the start menu |
| **Boot screens, title, start menu** | `src/ov162/boot_screens.c`, `title.c`, `startmenu.c`, `game_start.c` | everything below | `replay_start_menu`, `oracle_opening(_hashes)`, `oracle_title(_hashes)` | Frame for frame against melonDS |
| **New Game's intro** | `src/ov294` (the intro), `src/ov162/game_start.c` | everything below | `replay_new_game` (Black 2) | Runs to just before the field's first script |
| **The field** | `src/ov036` (the field engine, being decompiled upstream), `src/ov012` (events and scripts), `src/ov035` (map changes) | the trap stubs ([hal/trap.c](../hal/trap.c), written at each build by [tools/gen_stubs.py](../tools/gen_stubs.py)) | `oracle_continue`, `oracle_continue_starter` (each stops at the field's first trap) | Stops at overlay 36's process table, `FIELDMAP_PROC_FUNCS`, data with no C yet (a trap placeholder), when CONTINUE asks for the field |
| **The field menu's apps** (the party list, the Pokédex, the bag, the trainer card, the summary) | `src/ov012/event_field_proclink.c` (`EventFieldAppCall`: the app call, its params and the next app an app asks for), `src/ov012/event_field_menu.c` (the menu's items), `src/ov036/field_menu.c` (the menu and its kinds, being decompiled upstream); the apps: `src/ov165` (the party list), `src/ov302` (the Pokédex), `src/ov142` (the bag), `src/ov186/trainer_card.c` and `src/ov012/trcard_sys.c` (the trainer card), `src/ov207` (the summary); `src/ov035/event_mapchange.c` (CONTINUE's event, with the launcher's seam) | Frigate's test launcher, [app/launcher.c](../app/launcher.c) and [app/launcher_spec.c](../app/launcher_spec.c) (`--launch`, [ADR 0012](adr/0012-test-launcher-for-the-field-menus-apps.md)): test-only, it opens the apps in the field's place while overlay 36 has no C, through the app call's own states, and steps aside once the field's process table has C ([hal/trap.c](../hal/trap.c)'s `port_trap_is_placeholder`) | `oracle_launch_field_menus`, `oracle_launch_party_menus` (each app against melonDS's route through the field menu, pass by pass), `launch_summary_standalone`, `launch_touch_then_party`, `launch_unfinished`, `launch_not_offered`, `app_launcher_spec_*`, `integrate_launch_*`, `integrate_launcher_off`, `layout_trainer_card_param` | Through the test launcher, the bag, the trainer card, the party list with its Pokémon's menu and the Pokédex with Tepig's entry are melonDS's pass for pass in both versions; the summary's background is a few pixels behind (it scrolls by VBlanks, which the DS spends loading it) |
| **Overlays** | `src/gfl/gf_overlay.c` (`GFL_OvlLoad`), `include/gfl/overlay.h` (`OVERLAY_ID` is the number under `PLATFORM_PC`), `config/<version>/arm9/overlays/*/delinks.txt` | [hal/fs/fs_overlay.c](../hal/fs/fs_overlay.c), [hal/fs/ovsec.c](../hal/fs/ovsec.c), [tools/gen_overlay_sections.py](../tools/gen_overlay_sections.py), [cmake/overlay_sections.cmake](../cmake/overlay_sections.cmake) | `rom_game_overlays`, `overlay_sections_*` | All linked in; a load sets the overlay's statics back as the DS does |
| **Heaps and memory** | `src/gfl/heap.c`, `heapsys.c` (the hook for host sizes), `heap_dtcm.c`; `lib/nnsys/src/fnd` (NitroSystem's heaps); `lib/nitro/src/os/os_alloc.c` | [hal/os/os_arena.c](../hal/os/os_arena.c) (16 MiB main arena), [os_alloc_host.c](../hal/os/os_alloc_host.c), [memory_map.c](../hal/os/memory_map.c) (DTCM), [hal/gfl/heap_policy.c](../hal/gfl/heap_policy.c), [hal/nnsys/fnd_hooks.c](../hal/nnsys/fnd_hooks.c), [hal/hw](../hal/hw/hw.h) | `os_arenas`, `integrate_os_alloc`, `integrate_main_arena`, `nnsys_heaps`; every route in the sanitized build, which sees each block of the expanded and frame heaps | Heaps four times (roots) and twice (children) their DS size for 64-bit structs; `--main-arena-size`, `--heap-scale-root`, `--heap-scale-child` |
| **Files and archives** | `src/gfl/arc_tool.c` (NARC archives), `src/gfl/arc_util.c` (load, decompress, unpack), `src/system/file_util.c`; `lib/nitro/src/fs` | The chain: `arc_tool.c` → `FS_OpenFile`, `FS_ReadFile` (`lib/nitro/src/fs`, unchanged) → the ROM archive (`fs_rom.c`, unchanged, with the default commands of [hal/fs/fs_command_default_host.c](../hal/fs/fs_command_default_host.c)) → `CARDi_ReadRom` ([hal/card/card_rom.c](../hal/card/card_rom.c)) → a copy from the mapped dump ([hal/rom](../hal/rom/port_rom.h), which checked it at boot); files over memory through [hal/fs/fs_memfile_host.c](../hal/fs/fs_memfile_host.c); [hal/mi/mi_uncompress.c](../hal/mi/mi_uncompress.c) for compressed files | `rom_*`, `rom_game_file_system`, `rom_game_card_read`, `rom_game_memory_files`, `hal_mi` | Read straight from the player's dump; the layered file system for mods is planned (phase 6) |
| **2D backgrounds and text boxes** | `src/gfl/bg_sys.c`, `src/gfl/gx_layers.c`, `src/gfl/arc_util.c`; `lib/nitro/src/gx` (compiled unchanged); NitroSystem's VRAM transfers (`lib/nnsys/src/gfd`) | [gfx/ref/ds2d.c](../gfx/ref/ds2d.c), [gfx/screens.c](../gfx/screens.c), [hal/gx](../hal/gx/vram.h), [hal/dma](../hal/dma/dma.c) | `gfx_ds2d`, `gx_*`, `integrate_screens`, `integrate_dma_*` | The CPU renderer, line by line, with exact raster effects |
| **HBlank effects and screen wipes** | `src/system/wipe.c`, `wipe_sub.c` (`WipeLineWnd_HBlank`, which writes the window planes line by line while `GX_IsHBlank()`), `wipe_wnd.c`; `src/system/gfl_use.c` (the HBlank tasks); `src/gfl/tcb.c` | [hal/os/core.c](../hal/os/core.c) (the HBlank interrupt taken as DISPSTAT's HBlank flag sets, 1606 cycles into the line, so that a handler sees the flag; the scanline hook), [gfx/screens.c](../gfx/screens.c) (each line drawn with the windows the handler wrote) | `os_hblank_flag_in_handler`, `os_hblank_lines`, `os_hblank_main_loop`, `os_dispstat_gates_blanks`, `oracle_launch_*` (the trainer card's opening wipes, pass by pass) | The wipes that cover or uncover the screens line by line are melonDS's pass for pass |
| **Sprites and cell animations** | `src/gfl/clact.c` (cell actors), `src/gfl/clact_itcm.c`, `src/system/bmp_oam.c`, `src/gfl/vman.c` (sprite VRAM); `lib/nnsys/src/g2d` (cells, multi-cells, animations) | [gfx/ref/ds2d.c](../gfx/ref/ds2d.c) (sprites from OAM), [hal/nnsys/g2d_load.c](../hal/nnsys/g2d_load.c), [reloc.c](../hal/nnsys/reloc.c), [hal/gx](../hal/gx/vram.h) | `gfx_ds2d`, `nnsys_*`, `nnsys_g2d_rom_equivalence` | Every G2D file of the ROM is checked against the DS's own unpacking |
| **Fonts and text** | `src/system/gf_font.c`, `src/system/printsys.c`, `src/gfl/textprint.c`, `src/gfl/msgdata.c`, `src/gfl/strbuf.c`, `src/system/wordset.c`, `src/gfl/bmp.c`, `src/gfl/bmp_win.c` | None of their own: `gf_font.c` reads the ROM's fonts (Nitro's NFTR format) itself, and the text is drawn as 4-bit tiles into the text windows' bitmaps (`bmp.c`, `bmp_win.c`), which the backgrounds show through [gfx/ref/ds2d.c](../gfx/ref/ds2d.c). NitroSystem's font loader in [hal/nnsys/g2d_load.c](../hal/nnsys/g2d_load.c) is there for the library, but no game file calls it | the whole-program tests (every text box on the replays' screens); `nnsys_font` and `nnsys_font_nodes` test only NitroSystem's unused loader | Drawn on every screen reached; frame for frame against melonDS on the start menu (`oracle_continue`, `oracle_port_save`) |
| **3D: models, cameras, lights** | `src/gfl/g3d_system.c`, `g3d_camera.c`, `g3d_light.c`, `g3d_util.c`; `lib/nnsys/src/g3d` (NitroSystem G3D); `src/system/mcss.c` (Pokémon and trainer sprites drawn in 3D); `src/gfl/billboard.c` | [gfx/ge/ge.c](../gfx/ge/ge.c) (geometry), [gfx/ref/ds3d.c](../gfx/ref/ds3d.c) (rasterizer, in bands of lines on [gfx/workers.c](../gfx/workers.c)'s threads, [ADR 0016](adr/0016-3d-frame-in-bands-on-worker-threads.md)), [gfx/ge/ge_hal.c](../gfx/ge/ge_hal.c) (the last picture kept while nothing it is drawn from changes), [hal/hw](../hal/hw/hw.h) (the command log, readbacks), [hal/gx/gx_load3d.c](../hal/gx/gx_load3d.c), [hal/gx/gxasm.c](../hal/gx/gxasm.c), [hal/dma](../hal/dma/dma.c), [hal/fx](../hal/fx/fx_mtx33.c) | `g3d_*` (`g3d_ds3d_bands`: the bands against one band), `fx_libfx`, `hal_fx`, `oracle_title` (3D within a tolerance) | The CPU reference; an SDL_GPU renderer at any resolution is planned (PLAN.md §4.6) |
| **Particles** | `src/gfl/particle.c`, `lib/spl` (Nintendo's SPL particle library) | [hal/gfl/particle_work.c](../hal/gfl/particle_work.c) ([ADR 0008](adr/0008-particle-work-from-the-platform-layer.md)), drawn through the 3D path | `replay_new_game` | Work sized for 64-bit structs |
| **Display capture and the window** | `src/gfl/double3Ddisp.c` (3D on both screens), `src/ov162/title.c`, `src/system/screentex.c`, `src/ov148` (encounter effects) | [gfx/capture.c](../gfx/capture.c), [gfx/screens.c](../gfx/screens.c), [hal/gx/gx_display.c](../hal/gx/gx_display.c), [app/present.c](../app/present.c) (the window: resizable and letterboxed, `WINDOW_SCALE` its first size, `SDL_SetTextureScaleMode` in `port_present_open` the filter, `port_present_frame` each frame, `BottomScreenPoint` the mouse on the bottom screen), [app/main.c](../app/main.c) (pacing), [app/window_icon.c](../app/window_icon.c) with [tools/gen_window_icon.c](../tools/gen_window_icon.c) (the window's icon on Linux and Windows) | `gfx_capture_*`, `gx_display`, `integrate_screens`, `oracle_title`, `app_window_icon_plain`, `app_window_icon_sanitized` | One window, screens stacked, nearest-neighbour scaling; other layouts and filters are planned. Widescreen (planned, [MODDING.md](MODDING.md#what-the-mod-platform-will-add) lists its files) |
| **Sound and music** | `src/system/snd_sys.c` (the sound archive, players, BGM stack, the loader thread), `snd_strm.c`, `pm_voice.c` (cries), `snd_bgm_stack.c`, `field_sound.c`, `event_sound.c`, `iss_*.c` (interactive music); `lib/nnsys/src/snd`; `lib/nitro/src/snd` (the ARM9's SND) | [hal/snd7](../hal/snd7/snd7_internal.h) (the ARM7's driver and sound unit, [ADR 0006](adr/0006-sound-driver-as-built.md)), [hal/pxi](../hal/pxi/pxi_bus.h), [hal/os/core.c](../hal/os/core.c) (the driver's alarms as timed work), [audio/audio.c](../audio/audio.c); held to melonDS by [app/sound_trace.c](../app/sound_trace.c) (`--sound-trace`, the driver's state after each run), [tools/oracle/sound.py](../tools/oracle/sound.py) and [sndtrace.h](../tools/oracle/sndtrace.h) ([the oracle's Sound](../tools/oracle/README.md#sound)) | `snd_*` (the driver and unit against values worked out by hand: `snd_timer_write`, `snd_fifo`, `snd_pxi_alarm_timing`, `snd_trace_layout` among them), `pxi_snd_*`, `snd_rom_every_sequence`, `os_timed_work`, `oracle_sound_tool`, `oracle_sound_opening(_hashes)`, `oracle_sound_title(_hashes)`, `oracle_boot` (the oracle's own trace and dump held to determinism) | Held to melonDS in the opening and the title, both versions: every run of the driver the comparison pairs equal field by field; what the ARM9 sees of it equal (or a run apart) at each frame on the game's count but the title's few left out, the DS's long load passes and Kyurem's cry ([the oracle's results](../tools/oracle/README.md#sound)); the title's stream and the cry melonDS's to the bit; the music by its envelope and levels, since its notes start 1 to 5 mixer samples earlier than on the DS. Deterministic, mixed on the game's thread as the virtual clock passes |
| **Buttons** | `src/gfl/key.c` (`PAD_Read`), `src/gfl/ui.c` (input, soft reset with L, R, Start and Select), `src/gfl/button_man.c` | [app/present.c](../app/present.c) (the keyboard), [app/gamepad.c](../app/gamepad.c) (`port_gamepad_default_map`, [ADR 0009](adr/0009-gamepads-by-position-one-pad-at-a-time.md)), [hal/input](../hal/input/port_input.h) | `integrate_input`, `app_gamepad_*` | Remapping from a settings file is not wired yet |
| **Touch screen** | `src/gfl/touchpanel.c`, over `lib/nitro/src/spi/tp.c` | [app/present.c](../app/present.c) (the mouse on the bottom screen, `BottomScreenPoint`), [app/replay.c](../app/replay.c) (an input file's touches), [hal/tp/tp7.c](../hal/tp/tp7.c), [hal/nvram/nvram7.c](../hal/nvram/nvram7.c) (calibration) | `pxi_tp_*` (the ARM7's service and the calibration, driven from the ARM9's `tp.c`), `launch_touch_then_party` (a touch from an input file closes the trainer card) | Works with the mouse. One input file touches the screen: `launch_touch_then_party` closes the trainer card by its return button, on the port alone, and the party list after it must show as the field menu's choice by a key leaves it, since the touch sets the UI's touch flag; no oracle sequence touches the screen yet, and the mouse's mapping has no test |
| **Saves** | `src/gfl/backup_card.c`, `src/gfl/backup_system.c`, `src/gfl/savedata.c`, `src/system/save_control.c`, `src/system/save_table.c`, the `*_savedata.c` blocks; `lib/nitro/src/card` | [hal/card/backup.c](../hal/card/backup.c), [card_backup_host.c](../hal/card/card_backup_host.c), [card_backup_calls.h](../hal/card/card_backup_calls.h), [hal/pxi/pointer.c](../hal/pxi/pointer.c); saves are made on melonDS, the first from a blank save and each later one from the save before it, by [tools/oracle/oracle.py](../tools/oracle/oracle.py) (`SAVES`): the first save and then the starter save with a Pokémon and the Pokédex, which the tests start from (`oracle_saves` makes them), and the Route 19 save after a trainer battle, made only on request for the battles, which no test uses yet | `card_backup_*`, `rom_game_backup*`, `replay_same_save`, `oracle_saves`, `oracle_continue`, `oracle_continue_starter`, `oracle_port_save` | Byte for byte melonDS's `.sav`; the sidecar for Frigate's own data is planned (D7). On Windows the saves are in `%APPDATA%\Frigate` and a save that is a symbolic link is replaced, not followed (`card_backup_linked_save` skips there) |
| **Clock** | `src/gfl/rtc_cache.c`, `src/system/rtc.c`, `src/system/playtime.c`, `src/system/season.c` | [hal/rtc](../hal/rtc/rtc_host.h) (`--fixed-time`) | `pxi_rtc_*` | The host's local time, or a fixed time that runs with the virtual clock |
| **Random numbers** | `src/gfl/gf_standard.c` (`GFL_RandomMT`, the seed), `src/system/gfl_use.c` (`GFL_RandomLC`); the seed's entropy in `lib/nitro/src/os/os_entropy.c` | everything the entropy reads: [hal/os/core.c](../hal/os/core.c) (VCOUNT, the tick), [hal/rtc](../hal/rtc/rtc_host.h), [hal/nvram](../hal/nvram/nvram_settings.h) (the MAC address), [hal/mic](../hal/mic/mic.c), [hal/tp](../hal/tp/tp_host.h), [hal/input](../hal/input/port_input.h), [hal/math/sha1_block.c](../hal/math/sha1_block.c) | `os_determinism`, `replay_same_save` | Deterministic with `--fixed-time` and `--input` |
| **Reset, power, lid** | `src/gfl/ui.c` (soft reset, sleep), `src/system/machine_use.c` (the amplifier) | [hal/reset/reset7.c](../hal/reset/reset7.c), [hal/os/os_terminate.c](../hal/os/os_terminate.c), [hal/pm/pm7.c](../hal/pm/pm7.c), [hal/input](../hal/input/port_input.h) (the lid) | `pxi_reset_command`, `pxi_pm_*` | A reset ends the program (status 74); restarting in place is to come |
| **Microphone** | `src/system/snd_mic.c`, `src/ov253/mictest.c`, `src/ov257/ctvt_mic.c` (the Xtransceiver) | [hal/mic/mic.c](../hal/mic/mic.c) | `pxi_mic_silence` | Silence; host input is planned |
| **Wireless, Wi-Fi, infrared, the genuine-card check** | `src/gfl/net*.c`, `src/gfl/wm_icon.c`; `lib/nitro/src/wm`, `lib/twl/src/nwm`, `lib/dwc`, `lib/nitrowifi`, `lib/poke_net`, `lib/irc` (the infrared library, overlay 31); overlays 11, 181, 182; `src/ov338/ir_check.c` (the genuine-card check, at every CONTINUE and NEW GAME and when a Pokémon faints in battle: on failure the C-Gear's new-beacon marks are hidden for good and battles give no experience) | [hal/pxi](../hal/pxi/pxi_bus.h) (the wireless tags answer with the error bit), [hal/dwc/ov011_pending.c](../hal/dwc/ov011_pending.c), [hal/crypto/rc4.c](../hal/crypto/rc4.c) (RC4 for DWC's WDS code), four DWC files left out ([hal/replaced.txt](../hal/replaced.txt)); [hal/card/card_spi.c](../hal/card/card_spi.c), the card slot's SPI bus and the cartridge's infrared chip ([ADR 0011](adr/0011-cartridge-infrared-chip-as-a-register-model.md)) | `asm_oracle` (RC4), `hal_card_spi_*`, `rom_game_ir_genuine_card`, whole program | The genuine-card check passes as on melonDS; the infrared chip has no partner, as on melonDS; wireless isn't modelled, multiplayer is phase 7 |
| **DS Protect, DSi features** | `lib/dsprot`; `src/system/dsi.c`, `lib/twl` | [hal/stubs/dsprot.c](../hal/stubs/dsprot.c) (a genuine cartridge), [hal/os/os_emulator.c](../hal/os/os_emulator.c) (`OS_IsRunOnTwl` is FALSE) | `os_console`, whole program | Every DSi path is skipped, as on a DS |
| **Frame times** (how fast the frames are made) | — | [hal/os/frame_time.c](../hal/os/frame_time.c) (`--frame-times`: each frame's host time and parts, timed in [gfx/screens.c](../gfx/screens.c), [gfx/ge/ge_hal.c](../gfx/ge/ge_hal.c), [hal/pxi/arm7.c](../hal/pxi/arm7.c), [hal/os/core.c](../hal/os/core.c) and [app/main.c](../app/main.c)), [hal/host](../hal/host/host.h) (the clock), [tools/frame_times.py](../tools/frame_times.py) (scenes, each frame the least of several runs; the budget's check) | `frame_times_tool`; `frame_times_title`, `frame_times_new_game` (Black 2), `frame_times_party_menus` (label `timing`, each scene's 95th percentile within `PORT_FRAME_TIME_BUDGET_MS`, half a frame) | Every route's 95th percentile within 2.1 ms on an M2, an eighth of the DS's frame (4.7 ms before the hot paths' work of 2026-10-19), both versions; the program's start 0.25 s ([Where a frame's time goes](#where-a-frames-time-goes)) |
| **Program options and messages** | — | [app/main.c](../app/main.c) (the options, `sOptions`, and the exit statuses, both in `--help`), [hal/os/app_options.c](../hal/os/app_options.c) (`port_app_option`), each module's reader; [app/message_log.c](../app/message_log.c) (the macOS app's log) | `integrate_unknown_option`, `integrate_launch_bad_option`, `app_about` (`--help` names Frigate as the build does and describes every option), `app_message_log` | No settings file yet; messages start with `port:` |
| **The sanitized build** (T5) | `src/system/gf_font.c`, `src/system/printsys.c`, `src/gfl/arc_util.c`'s dynamic load (the game's three memory bugs it found, fixed under `PLATFORM_PC`), `src/system/scroll_bar.c` (a division by a list's zero `max`, given the DS's result under `PLATFORM_PC`), `src/gfl/arc_util.c`'s static load (the copyright screen's read past a heap block, which the title screen shows, kept as the DS's and let through) | `PORT_SANITIZE` ([../CMakeLists.txt](../CMakeLists.txt)), [cmake/sanitizer_ignorelist.txt](../cmake/sanitizer_ignorelist.txt) (the checks left out, and why), [hal/os/sanitizers.c](../hal/os/sanitizers.c) (UBSan's reports with stacks that follow the fibers), [hal/os/fiber.c](../hal/os/fiber.c) (the switches, fake stacks, LeakSanitizer's roots), [hal/nnsys/fnd_hooks.c](../hal/nnsys/fnd_hooks.c) (the game's heaps poisoned block by block, and the DS's one read past a block let through, once it is checked to stay in the game's heaps or the main arena), [hal/trap.c](../hal/trap.c) (the placeholders readable at exit, through [hal/host](../hal/host/host.h)), [tests/app/check_exit.py](../tests/app/check_exit.py) (a report fails a run whatever ending it wants; no test has `WILL_FAIL`) | Every test in `build/port-san`; the routes are `ctest -L route`; `nnsys_ds_reads`, `nnsys_ds_read_outside`, `app_check_exit`; CI's `sanitize` job (macOS and Linux) | Every route clean in both versions, the test launcher's Pokédex and White 2's routes from its starter save included; the distribution build's options build sanitized too ([sanitizers.md](sanitizers.md)) |
| **Replays and the oracle** | — | [app/replay.c](../app/replay.c), [app/run_dir.c](../app/run_dir.c) (with the globals by name from [hal/host](../hal/host/host.h)), [tools/oracle](../tools/oracle/README.md), [hal/gfl/gfl_frame.h](../hal/gfl/gfl_frame.h) (GFL's frame count) | `app_run_dir_*`, `oracle_pbwinput_*`, `oracle_compare`, `oracle_*` | Inputs counted in the game's frames, compared with melonDS |
| **The macOS app** | — | [app/macos](../app/macos/make_app.sh) (the app, its icon, its bundle identifier in [bundle_id.cmake](../app/macos/bundle_id.cmake), and [check_app.sh](../app/macos/check_app.sh), what its binaries' load commands say), [cmake/macos_app.cmake](../cmake/macos_app.cmake), [tools/package_macos.sh](../tools/package_macos.sh), [assets/branding](../assets/branding/README.md); the distribution build, `PORT_MACOS_DISTRIBUTION` in [CMakeLists.txt](../CMakeLists.txt) ([ADR 0015](adr/0015-universal-macos-app-for-macos-11.md)), with the slices' link rules in [tools/gen_overlay_sections.py](../tools/gen_overlay_sections.py) and the x86_64 page alignment, `PORT_HOST_PAGE_ALIGNMENT` in [hal/host](../hal/host/host.h), which [hal/hw/hw.c](../hal/hw/hw.c) and [tools/gen_stubs.py](../tools/gen_stubs.py) take, and [tools/ctest_one_slice.sh](../tools/ctest_one_slice.sh), the whole suite as one slice | `app_make_app`, `app_bundle_id`, `app_package_macos_out`, `app_slices`, `app_universal_slices`, `overlay_sections_image_*`; CI's `macos-distribution` job (macOS 15 and 26, Apple Silicon and Intel) | Signed ad hoc, in a disk image. A development build's app is for the Mac that built it; the distribution build's is one universal program, arm64 and x86_64, built for macOS 11 and later, with SDL3 linked in, tested with the ROM on macOS 26 only (the x86_64 slice under Rosetta) and without it by CI on macOS 15 and 26, Apple Silicon and Intel |
| **macOS, Linux and Windows** | — | [hal/host](../hal/host/host.h) (what the platform layer, the program and the tests share of the system where the hosts differ: pages, faults, backtraces, the program's names, the watchdog, files; `host_posix.c` and `host_windows.c`, [ADR 0013](adr/0013-windows-build-with-msys2-clang64.md)); the modules whose feature is a host's, with their own branches ([hal/README.md](../hal/README.md#names)): [hal/card/backup.c](../hal/card/backup.c) (the saves' folder, `%APPDATA%\Frigate` on Windows, their atomic replacement, links followed on macOS and Linux only), [hal/gx/vram_os.c](../hal/gx/vram_os.c) (VRAM aliasing on macOS and Linux; Windows copies the banks), [hal/rom/rom.c](../hal/rom/rom.c) (the dump mapped, or read into memory on Windows), [hal/rtc/rtc7.c](../hal/rtc/rtc7.c) (the local time), [hal/fs/ovsec.c](../hal/fs/ovsec.c) (the overlays' sections on Mach-O, ELF and COFF); [app/windows/utf8.manifest](../app/windows/utf8.manifest) (UTF-8 as every Windows program's code page), [CMakeLists.txt](../CMakeLists.txt) (Windows' 8 MiB main stack, the manifest), [tools/gen_stubs.py](../tools/gen_stubs.py) (the C library each host provides), [../../.github/workflows/port-ci.yml](../../.github/workflows/port-ci.yml) | every test that needs no ROM, on each host in CI: `host_*` (traps and crash reports, on fibers and in the C library's code too, the watchdog, names that aren't ASCII), `os_guard_*` (a fiber's overflow reported), `app_symbols`, `integrate_overlay_sections`, `pxi_bus_arena_overlap` | macOS is where Frigate is developed and every test runs; Linux and Windows (MSYS2's CLANG64) build in CI and pass the tests that need no ROM. Windows copies the VRAM banks (no aliasing yet), and its saves follow no symbolic links |

The planned parts ([PLAN.md §4.3](PLAN.md#43-directory-structure)) have no directory yet: `gfx/gpu/` and
`gfx/shaders/` (the SDL_GPU renderer), `vfs/` (layered files and mods' overrides), `save/` (the sidecar), `mod/`,
`features/`, `net/`, `ui/` and `third_party/`.

## Glossary

The DS's hardware, Nintendo's libraries and Frigate's own terms, from ARM7 to VBlank: [GLOSSARY.md](GLOSSARY.md).
