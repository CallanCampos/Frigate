# Glossary

The words Frigate's code and documents use, from the DS's hardware, Nintendo's and Game Freak's libraries, the decomp
and Frigate itself. [ARCHITECTURE.md](ARCHITECTURE.md) shows how the parts fit.

## The project

| Term | Meaning |
| --- | --- |
| **Frigate** | The native PC port of Pokémon Black 2 and White 2 built from the pokebw2 decompilation, in `port/` of this repository; its programs are `frigate-black2` and `frigate-white2` |
| **The port, the layer** | Frigate's code, as opposed to the decomp's. Its internal names keep the word: the `port/` directory, the `port_` C prefix, the `PORT_*` CMake variables, the `port:` prefix of the program's messages |
| **The decomp, upstream** | pokebw2 (`fuddlesworth/pokebw2`), the decompilation this repository follows, merged every hour ([SYNC.md](SYNC.md)): everything outside `port/` is its, except Frigate's three files there, `.github/README.md` and the two `.github/workflows/port-*.yml` |
| **ROM build, host build** | The ROM build (`configure.py` and `ninja`, with MWCC) rebuilds both ROMs from the decomp and checks their SHA1s; the host build (CMake in `port/`, with clang) makes Frigate. Both build from the same tree |
| **MWCC, matching** | Metrowerks CodeWarrior, the DS's C compiler. A matching function is C that MWCC compiles to the ROM's exact bytes; matching is upstream's work |
| **Complete, incomplete file** | A decomp file whose every function matches is marked complete in its `delinks.txt` and links into the ROM; an incomplete one still links the original code, so the ROM build proves nothing about its C. Frigate compiles both |
| **`PLATFORM_PC`** | Defined by the host build only (`port/CMakeLists.txt`). An `#ifdef PLATFORM_PC` block in a decomp file is one of Frigate's game changes ([game-changes.md](game-changes.md)) |
| **`BUGFIX`** | The decomp's switch for its `// BUG:` fixes. Off in Frigate, so the game plays as the original; memory-safety bugs are fixed under `PLATFORM_PC` instead |
| **swan** | The community's names for BW2's symbols, which the decomp uses first |
| **GBATEK** | The DS's hardware reference, which the renderers and the platform layer cite for each fact |
| **ADR** | An architecture decision record, in [adr/](adr/README.md) |
| **T0 to T7** | The test levels: the ROM build's SHA1s, the host build, unit tests, replays, the oracle, sanitizers, saves and ROM checks, playtests ([../CLAUDE.md](../CLAUDE.md#testing-planmd-6)) |

## The DS

| Term | Meaning |
| --- | --- |
| **ARM9** | The DS's main processor (an ARM946E-S), which runs the game. Everything in `src/` and `lib/` is ARM9 code; Frigate compiles it for the host |
| **ARM7** | The DS's second processor, which runs Nintendo's own code for sound, touch, the clock, power, the firmware and wireless. The decomp doesn't have it; Frigate replaces it with host services behind PXI ([../hal](../hal/README.md)) |
| **PXI** | The FIFO between the two processors: 32-bit words, each with a tag for its service. The platform layer delivers each word to the host service of its tag at once |
| **ITCM, DTCM** | The ARM9's tightly coupled memories: ITCM, 32 KiB of fast code (the interrupt handler, hot renderers), and DTCM, 16 KiB of fast data (the interrupt table, stacks, a small heap), at 0x02fe0000 in BW2. On the host DTCM is a 16 KiB block ([../hal/os/memory_map.c](../hal/os/memory_map.c)) |
| **Main memory, arena** | The DS's 4 MiB of RAM; the SDK divides what the program leaves free into arenas, from which the game carves its heaps. Frigate's main arena is 16 MiB of host memory |
| **Overlay** | A block of ARM9 code and data loaded on demand into a region it shares with others (344 in each version). The decomp's `src/ovNNN/` holds overlay NNN's files. Frigate links all of them into the program, and loading one sets its writable statics back as the DS does ([../hal/fs/ovsec.h](../hal/fs/ovsec.h)) |
| **2D engines A and B** | The main (A) and sub (B) 2D graphics engines, one for each screen; POWCNT1 decides which feeds the top screen and which the bottom, and engine A can show the 3D picture as its background 0 |
| **BG** | A background layer: four per engine, each text (tiled), affine, extended or bitmap |
| **OAM, OBJ** | Object attribute memory: 128 sprite entries per engine. OBJ is the hardware's word for a sprite |
| **VRAM banks, VRAMCNT, LCDC** | Nine banks of video memory, A to I, that VRAMCNT maps into backgrounds, sprites, textures, palettes, or LCDC, where the CPU reads and writes them directly (0x06800000). [../hal/gx/vram.c](../hal/gx/vram.c) maps them by page aliasing |
| **DISPCNT, POWCNT1** | The display control register of each engine (mode, layers shown), and the power control register (which engine feeds which screen) |
| **VBlank, HBlank, VCOUNT, DISPSTAT** | The vertical blank (lines 192 to 262), when the game swaps buffers and uploads to VRAM; the horizontal blank after each line's 256 dots; the line being drawn. Frigate's virtual clock drives all three. DISPSTAT holds the flags the ARM9 reads of them: its HBlank flag sets 1606 cycles into each line, and Frigate takes the HBlank interrupt at the same moment, as melonDS does, so that a handler sees the flag. The screen wipes that cover or uncover the screens line by line write their windows from an HBlank handler only while the flag is set (`src/system/wipe_sub.c`) |
| **Geometry engine, GXFIFO** | The DS's 3D command processor, fed through command ports and the GXFIFO; it transforms, lights and clips polygons for the rendering engine. [../gfx/ge](../gfx/README.md) models it |
| **Display capture** | Hardware that writes a composed picture into a VRAM bank, used for motion blur and 3D on both screens ([../gfx/capture.c](../gfx/capture.c)) |
| **Backup, flash** | The cartridge's save chip, a 4 Mbit (512 KiB) flash in BW2. Frigate keeps it as `IREO.sav` (Black 2) or `IRDO.sav` (White 2), named after the ROM's game code |
| **NitroFS, FAT, FNT** | The cartridge's file system, with its allocation and name tables; every file the game reads is in it |
| **NARC** | Nitro archive: one NitroFS file that holds many numbered members, read by `src/gfl/arc_tool.c` |
| **SDAT, SSEQ, SBNK, SWAR, STRM** | The sound archive and what it holds: sequences, instrument banks, wave archives, streams |
| **G2D files: NCGR, NCLR, NCER, NMCR, NANR, NFTR** | NitroSystem's 2D graphics files: characters (tiles), palettes, cells, multi-cells, animations, fonts. The DS relocates them in place; Frigate rebuilds them as host structs ([ADR 0007](adr/0007-nitrosystem-files-in-host-structs.md)) |
| **Infrared chip, card SPI bus** | BW2's cartridges carry an infrared chip on the card slot's SPI bus, which also reaches the save's flash; the ARM9 drives the bus through `AUXSPICNT` and `AUXSPIDATA`. Frigate models both as melonDS has them ([../hal/card/card_spi.c](../hal/card/card_spi.c), [ADR 0011](adr/0011-cartridge-infrared-chip-as-a-register-model.md)) |
| **DSi, TWL** | The DSi and its SDK's code name. BW2 runs on both; Frigate is a DS, so every DSi path is skipped |

## Nintendo's and Game Freak's libraries

| Term | Meaning |
| --- | --- |
| **NitroSDK, TwlSDK** | Nintendo's DS and DSi SDKs (`lib/nitro`, `lib/twl`): OS, GX, FS, CARD, SND, PXI, MI, FX and the rest |
| **NitroSystem, NNS** | Nintendo's middleware above the SDK (`lib/nnsys`): FND heaps, G2D, G3D, GFD VRAM transfers, the sound players |
| **SPL** | Nintendo's particle library (`lib/spl`) |
| **DWC** | Nintendo Wi-Fi Connection (`lib/dwc`), the online library |
| **DS Protect** | Nintendo's anti-piracy library (`lib/dsprot`); Frigate's stub passes its checks as on a genuine cartridge |
| **GFL** | Game Freak's library (`src/gfl`): heaps, overlays, archives, processes, tasks, backgrounds, cell actors, input, text |
| **Process (proc)** | A GFL process: a screen or mode with its init, main and exit functions and its overlay (`src/gfl/procsys.c`) |
| **TCB** | A GFL task (task control block) in a priority list run each frame, each VBlank or each HBlank (`src/gfl/tcb.c`) |
| **Cell actor (ClAct)** | GFL's sprite system (`src/gfl/clact.c`), which draws NitroSystem cells and animations through OAM |
| **Field menu, app call** | The menu X opens in the field (overlay 36's `field_menu.c`): POKéMON, POKéDEX, BAG, the trainer card, SAVE and OPTIONS, fewer without a Pokémon or the Pokédex. Each item opens an app, a process of its own overlay, through overlay 12's app call (`EventFieldAppCall`, `src/ov012/event_field_proclink.c`), which closes the field, runs the app and opens the next one it asks for or the field again |
| **Game event, event type** | A game event (`GameEvent`, `src/system/game_event.c`) is a step of the game's flow with a state machine of its own, run each pass of the main loop until it ends; it can chain a child event that runs in its place until that one ends (`GameEvent_ChainNext`). CONTINUE's event chains `EventFieldOpen` to open the field; Frigate's test launcher chains its own event there instead. An event type is what an app's result asks of the field menu once the app call closes the app (`EventFieldAppCall_ConvAppResultToEventType`): 0 shows the menu again, 1 closes it, and 2 (a field move), 3 (a field event, such as an item's use) and 5 (a script) act in the field |
| **Genuine-card check** | The game's check of the cartridge's infrared chip (`src/ov338/ir_check.c`), at every CONTINUE and NEW GAME and when a Pokémon faints in battle: a chip that doesn't answer its ID hides the C-Gear's new-beacon marks for good and stops battles giving experience. Frigate's model of the chip passes it |
| **MCSS** | The system that draws Pokémon and trainer sprites from multi-cell animations, in 3D, for battles and other screens (`src/system/mcss.c`) |

## Frigate's own terms

| Term | Meaning |
| --- | --- |
| **Platform layer, HAL** | [port/hal](../hal/README.md): host C for the parts of the SDK and the ARM7 a host doesn't have |
| **SDK surface** | What the game's C reaches that the platform layer takes over: the SDK's assembly, its hardware-bound files, the DS's addresses and registers, the ARM7 |
| **Virtual clock** | Frigate's count of the ARM9 bus's cycles (33.514 MHz), which moves only through halts, modelled busy waits and `OS_GetTick`, so that runs are deterministic ([../hal/os/core.c](../hal/os/core.c)) |
| **Dispatcher, fiber** | The loop on the host thread's own stack that resumes the SDK's threads, each a fiber (a stack and saved registers), and runs interrupts and host work between them |
| **Frame hook, scanline hook** | Host functions run at each VBlank, and at the start of each visible line, on the dispatcher's stack ([../hal/os/port_os.h](../hal/os/port_os.h)) |
| **Timed work** | Host work that a module sets for a cycle of the virtual clock (`port_os_set_timed_work`) and the OS core runs on the dispatcher's stack when the clock reaches it: the sound driver's next alarm ([../hal/os/port_os.h](../hal/os/port_os.h)) |
| **Trap stub, data placeholder** | A generated stand-in for a DS function or data object that has no C yet: reaching it stops the program with its name and status 70 ([../hal/trap.c](../hal/trap.c), [../tools/gen_stubs.py](../tools/gen_stubs.py)) |
| **`replaced.txt`, `*_host.c`, `port_sdk_*`** | The three ways the platform layer takes over a decomp file: leaving it out, compiling it through a host file, or renaming its functions ([../hal/README.md](../hal/README.md#how-a-decomp-file-is-adapted)) |
| **`PORT_HW_ADDR`, hwinclude** | The build's translation of the SDK headers' DS addresses into host memory windows ([ADR 0004](adr/0004-ds-addresses-by-header-translation.md)) |
| **Plain C types only** | What a header of the platform layer says when it includes none of the decomp's headers, so that `port/app`, `port/gfx` and `port/audio`, which never include them, can include it |
| **Oracle** | melonDS running the player's ROM headless (`pbw-oracle`, [../tools/oracle](../tools/oracle/README.md)): the reference Frigate is compared with, a test tool only. The asm oracle ([../tests/asm_oracle](../tests/asm_oracle/README.md)) is the same idea for single functions, with Unicorn |
| **pbwinput, run directory** | The input file format that the oracle and Frigate's replays share, and the directory a run writes for comparison (`frames.tsv`, screens, memory dumps) |
| **Periodic run, play** | A run of the ARM7 sound driver's thread is periodic when its alarm wakes it, every 2728 ticks of 64 cycles (about 192 a second), the only runs that move the sequences on; a play is a player's sequence from its start to its stop. The sound comparison pairs the two programs' plays, and their runs by the count of periodic runs since the play's start ([../tools/oracle/README.md](../tools/oracle/README.md#sound)) |
| **Sound trace** | The ARM7 sound driver's state after each run of its thread, in the DS's layout (`pbwsnd 1`, [../tools/oracle/sndtrace.h](../tools/oracle/sndtrace.h)), which the oracle and Frigate write with `--sound-trace` for `sound.py` to compare |
| **Game frame, video frame** | A pass of the game's main loop (GFL's frame count) and a VBlank. The field runs at 30 passes a second, so the two differ |
| **Replay** | A run of the program driven by an input file (`--input`) instead of the player |
| **Test launcher** | Frigate's `--launch` ([../app/launcher.c](../app/launcher.c), [ADR 0012](adr/0012-test-launcher-for-the-field-menus-apps.md)): when a continued game asks for the field, which has no C yet, it opens the field menu's apps in the field's place, each at a count of GFL's frames, so that the tests can hold them to the oracle's route through the field menu. Test-only and off by default; it steps aside once the field's process table has C |
| **Saves made on the oracle** | Saves that melonDS makes by playing an input file (`oracle.py save`, `SAVES`), the first from a blank save and each later one from the save before it, each checked against the SHA-1 `oracle.py` keeps. The tests start from the first save and the starter save (a Pokémon and the Pokédex), which ctest's `oracle_saves` makes into the build tree, where later runs reuse them; the Route 19 save, for battles, is made only on request (`oracle.py save route19_save`), and no test uses it yet |
| **Game change** | A change Frigate makes outside `port/`, of one of three allowed kinds, each in [game-changes.md](game-changes.md) |
| **Universal program, slice** | A macOS program with code for more than one processor, each a slice: the distribution build's has an arm64 slice for Apple Silicon and an x86_64 slice for Intel Macs, and macOS runs the one for its processor. `lipo -archs` lists them; `otool` reads one, so the tools read every slice (`-arch all`) |
| **Rosetta** | Apple's translator that runs an x86_64 program on Apple Silicon; here it runs the universal program's x86_64 slice for its tests (`arch -x86_64`). Apple keeps it whole through macOS 27 |
| **Deployment target, minos** | The oldest macOS a build is for (`CMAKE_OSX_DEPLOYMENT_TARGET`, `-mmacosx-version-min`), which the linker writes as each slice's `minos` in its `LC_BUILD_VERSION` load command and the app as `LSMinimumSystemVersion`. A development build sets none and gets the SDK's |
