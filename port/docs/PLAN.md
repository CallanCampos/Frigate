# Native port of Pokémon Black 2 and White 2: plan

How we build a native Mac port (Apple Silicon first) and then Windows and Linux ports of Black 2 and White 2 from the
pokebw2 decompilation, and how development runs, is tested and keeps up with the decomp. The findings behind each
decision are in [RESEARCH.md](RESEARCH.md). Written 2026-10-16 against `fuddlesworth/pokebw2` at `0c2bbee9d`; the
numbers are from that commit. The port repository, `CallanCampos/pokebw2-native`, was seeded from upstream `main` at
`462e90193` (§4.1).

## 1. Goal and what "native" means

The goal is Black 2 and White 2 running as a native application: fast, faithful, and easy to mod (widescreen, a
randomizer, multiplayer, and whatever modders build next).

- **All code runs as host machine code compiled from the decomp's C.** No ARM code from the ROM is executed,
  interpreted, recompiled or translated, and there is no CPU emulator.
- **The ROM is only a data source.** The player supplies their own dump; the port reads its files at runtime and ships
  nothing Nintendo owns.
- **Display and sound are host backends.** The game programs the DS's display hardware directly: tile memory,
  sprite tables, palettes, registers and 3D geometry commands. The port reimplements that contract as a renderer and
  a sound driver, the way libultraship's Fast3D turns N64 display lists into Metal or Direct3D calls. There is no
  clocked hardware, no bus and no cycle timing; the renderer reads the state the game set up once per frame (or per
  scanline for raster effects) and draws it on the GPU at any resolution.
- **An emulator is a test tool only.** melonDS running the real ROM serves as the reference the port is compared
  against during development. It is never linked into or shipped with the port.

## 2. Where we start

Full detail in [RESEARCH.md §1–4](RESEARCH.md#1-state-of-the-decomp).

- 91% of ARM9 code already exists as C. ARM9 main, the engine, is C except 2 KB of crt0, BIOS stubs and `exit`; its
  other gap is the four Game Freak sprite renderers in ITCM (3 KB).
- Game code with no C is about 122 KB: 111 KB in the field engine (overlay 36), which mariano54 is landing upstream
  now (issue #83), 8.5 KB of Download Play code in overlay 182 and the 3 KB of ITCM renderers. The Wi-Fi libraries
  (overlays 11 and 182) follow; the port doesn't need them until multiplayer.
- 747 KB of C exists but isn't linked into the ROM, so no hash proves it correct. The port will run it and needs its
  own equivalence tests.
- The ARM7 (sound driver, wireless, touch, clock) isn't decompiled; the port replaces it.
- The boot logos need no further decompilation; the title screen needs only the four ITCM renderers. The overworld
  needs overlay 36.

## 3. Decisions

| # | Decision | Status |
| --- | --- | --- |
| D1 | **Repository:** the public repository `CallanCampos/pokebw2-native`, which carries the decomp's full history and merges upstream `main` continuously, with all port code under `port/` | **Decided** 2026-10-16 (§4.1, [ADR 0001](adr/0001-repository-and-upstream-sync.md)) |
| D2 | **Our own platform layer**, built on pokebw2's own SDK C. libntr is a reference for patterns only | Recommended |
| D3 | **Hybrid boundary:** game, GFL, NitroSystem and portable SDK C (`gx` included) compile unchanged; 217 SDK entry points get host implementations (48 of them wireless stubs); display state is modelled for the renderers | Recommended |
| D4 | **Stack:** C for the port, C++ only inside the UI module (Dear ImGui); clang on every platform; SDL3 + SDL_GPU; CMake; Lua 5.4 for mods; ENet + libjuice for networking | Recommended |
| D5 | **64-bit native build** with source fixes, not a 32-bit or wasm32 build: macOS has no 32-bit mode and mods need real pointers | Recommended |
| D6 | **One executable for Black 2 and White 2**, choosing the version by the ROM's hash (two executables until then) | Recommended, from phase 5 |
| D7 | **Saves stay byte-compatible with the DS** (raw 512 KiB image, works with melonDS `.sav` and cartridge dumps); port and mod data go in a sidecar file | Recommended |
| D8 | **Sound:** keep the ARM9 sound libraries as C; replace the ARM7 driver with host C written from the ARM7's own asm (sequencer and channel logic) plus our own mixer | Recommended |
| D9 | **Mods first-class:** layered file system, data registries, hook macros that compile away in the ROM build, settings registry, Lua; built-in features use the same API | Recommended |
| D10 | **A solo project:** nothing about the port is posted on `fuddlesworth/pokebw2` (no issues, comments, pull requests or proposals); port changes stay in this repository | **Decided** 2026-10-17 (§9, [ADR 0002](adr/0002-solo-project-no-upstream-posts.md)) |
| D11 | **No Apple Developer account:** macOS builds are ad-hoc signed and never notarized | **Decided** 2026-10-17 (§7 phase 5, §10, [ADR 0003](adr/0003-ad-hoc-signing-no-apple-developer-account.md)) |

### Why not libntr

libntr is the obvious shortcut, and we should still read it closely. But it is the NitroSDK 4.2 decompilation (pokebw2
has its own TwlSDK 5.3-era SDK in C, much of it already matching) plus a register-level simulator. It lacks much of
what BW2 draws with: edge marking, display capture, per-scanline effects, translucency sorting and opening files by ID.
It doesn't build on macOS and needs OpenGL 4.2. Its MIT license covers decompiled code that was never licensed, and
parts closely match melonDS (GPLv3) without attribution. It depends on one developer's untagged branch, and every
serious mod so far has had to fork it. Its authors also refuse AI-made contributions, so we couldn't upstream fixes.
We take its patterns (the enet wireless tunnel, ROM extraction, ImGui and Tracy integration, the list of 64-bit fixes
in its resource loaders) and write our own.

## 4. Architecture

### 4.1 Repository

**Decided (2026-10-16):** the public repository `CallanCampos/pokebw2-native`, seeded with the full history of
`fuddlesworth/pokebw2` (upstream `main` at `462e90193` at creation). Its clones keep `upstream` pointing at
`fuddlesworth/pokebw2`. It is the same tree as the decomp plus a `port/` directory, the in-tree shape of the Sonic
Advance 2 port and sm64-port, so the ROM build keeps verifying both SHA1s in the same tree as the host build. Keeping
current is a plain `git merge` of upstream's `main`, as the Platinum port does with pret's. The decision and its
alternatives are in [ADR 0001](adr/0001-repository-and-upstream-sync.md).

- **Port files.** All port code is in `port/`. The only other port files are ones upstream doesn't have:
  `.github/workflows/port-sync.yml` (and later `port-*.yml` workflows) and `.github/README.md`, which GitHub shows as
  the landing page in place of upstream's `README.md`. The port never edits an upstream file for its own setup (the
  root `CLAUDE.md`, `AGENTS.md`, `README.md`, `CONTRIBUTING.md`, `.claude/`, upstream's workflows), since every
  merge would conflict there. Upstream's `build.yml`, which fetches the ROMs with a key the port repository doesn't
  have, is turned off there with `gh workflow disable` rather than edited.
- **Staying current.** Upstream landed 150–250 commits a day from 10-13 to 10-16. Two mechanisms keep the port on
  top of it, both described in [SYNC.md](SYNC.md):
  1. An hourly GitHub Action (`port-sync.yml`) merges upstream `main` into the port's `main` when the merge is clean
     and every C file still compiles for both versions. CI has no ROMs, so it can't check the SHA1s. A conflict, a
     compile failure, an upstream workflow change (which the Action's token can't push), game files that differ from
     upstream, a rewrite of upstream's history, commits whose messages name issues in other repositories while this
     repository is public (§9), or a refused push leaves `main` as it was and opens or updates an `upstream-sync`
     issue for the local sync to resolve.
  2. Every port session starts with the local sync, [`port/tools/sync_upstream.sh`](../tools/sync_upstream.sh),
     which merges upstream and verifies the merge with the ROM build and both SHA1s before anything is pushed
     (§8 step 1).
- **Game code stays the decomp's.** Changes outside `port/` are limited to: (1) portability fixes that compile to the
  same bytes under MWCC; (2) hook macros that compile to nothing in the ROM build; (3) rare `#ifdef PLATFORM_PC`
  blocks. Each is listed in `port/docs/game-changes.md`. Shipwright's lesson is that an edited snapshot of a decomp
  drifts until it needs scripts to find its differences; we never copy decomp files. Decomp work itself (matching,
  new source files, config changes, overlay claims) never happens in this repository (§9).
- **Why not a branch of the existing fork:** `CallanCampos/pokebw2` carries the decomp PR branches, GitHub allows an
  account only one fork of a repository, and a port needs its own issues and releases.
- **Why not in upstream:** the port is a solo project (D10). Upstream isn't told about it, and nothing from it is
  proposed there.
- **Visibility:** public, so GitHub Actions minutes are free, macOS runners included
  ([tech-stack.md](research/tech-stack.md)). The tree holds no ROM data beyond what upstream already publishes in
  `data/`. Going private would end the free minutes and needs its own ADR
  ([ADR 0002](adr/0002-solo-project-no-upstream-posts.md), Consequences).

### 4.2 Layers

```
 Mods: Lua scripts, data packs, texture packs, (later) native plugins
 ─────────────────────────────── mod API: hooks, events, registries, settings ───
 Port features: enhancements, randomizer, widescreen, netplay, menus (ImGui)
 ─────────────────────────────────────────────────────────────────────────────────
 Game code (src/: GFL, system, overlays) — decomp C, unchanged but for hook macros
 Libraries (lib/nnsys, lib/spl, portable lib/nitro, lib/twl, lib/msl) — decomp C
 ─────────────────────────────────────────────────────────────────────────────────
 HAL (port/hal): OS scheduler and clock, files and overlays, backup, input, RTC,
   power, math, stubs · display model (registers, VRAM banks, OAM, palettes,
   geometry FIFO) · sound driver (ARM7 replacement)
 ─────────────────────────────────────────────────────────────────────────────────
 Backends: renderer (CPU reference + SDL_GPU), audio (SDL3), input, VFS, network
 App shell (SDL3): window and screen layouts, settings, paths, first-launch setup
```

### 4.3 Directory structure

Everything the port adds lives in `port/`, organized so a modder finds things by what they do. The exceptions are the
two kinds of file upstream doesn't have (§4.1): `.github/workflows/port-*.yml` and `.github/README.md`.

```
port/
  CMakeLists.txt         host build; reads the decomp's file lists from config/
  README.md              build and run, for players and developers
  CLAUDE.md              rules for sessions working on the port
  app/                   main(), SDL3 window, screen layouts, settings file, paths, first-launch ROM setup
  hal/                   host side of the SDK, one directory per SDK module it replaces
    os/                  fiber scheduler with DS priority semantics, IRQ dispatch, virtual clock, alarms, arenas
    fs/                  NitroFS backend, overlay activation (.data restore, .bss zero)
    card/                512 KiB backup image, atomic writes, .sav/.dsv import and export
    input/               PAD and touch panel from keyboard, gamepads, mouse, touch
    rtc/ pm/ mic/        host clock with offset, power stubs, mic from host input or silence
    mi/ fx/              DMA as memcpy, LZ decompression in C, software divider and sqrt
    gx/                  display model: register file, VRAM bank mapping, palettes, OAM, geometry FIFO
    snd/                 sound driver: command packets, sequencer, channels, ADPCM/PCM/PSG, mixer
    stubs/               DS Protect, IR check, CTRDG, MB, DSi-only modules (OS_IsRunOnTwl() = FALSE)
    replaced.txt         SDK source files the host build compiles from hal/ instead of lib/
  gfx/
    ref/                 CPU reference renderers: 2D engines per scanline, 3D rasterizer later; test oracle
    ge/                  geometry engine: command decode, matrix stacks, lighting, clipping, readbacks
    gpu/                 SDL_GPU renderer: 2D compositor, 3D rasterizer with DS rules, capture, upscaling
    shaders/             HLSL sources, compiled offline to SPIR-V, MSL and DXIL
  audio/                 SDL3 audio output, resampling, mic input
  vfs/                   ROM reader (memory-mapped), SHA1 manifest, layered mounts, NARC member overrides
  save/                  save image plus sidecar JSON for port and mod data
  mod/                   mod loader, manifest, hook bus, events, data registries, Lua runtime and bindings
  features/              built-in enhancements, one self-registering directory each
    widescreen/ randomizer/ fastforward/ hd/ qol/
  net/                   GFL network device over UDP, LAN discovery, relay client, infrared-as-pairing
  ui/                    ImGui menus: settings, mods, debug views (VRAM, OAM, 3D, sound, game state)
  tests/                 unit tests, replay tests, golden images, oracle comparisons
  tools/                 upstream sync (sync_upstream.sh), generators (source lists, stubs, overlay sections,
                         Lua bindings), oracle harness, progress tracker, compile probe
  docs/                  this plan, research, sync guide, architecture decision records, status log, modding docs
  third_party/           pinned dependencies (SDL3, Dear ImGui, Lua, minicoro, ...)
mods/examples/           sample mods, one per extension point
```

### 4.4 Build

- **Two builds, one tree.** The ROM build (`configure.py` + `ninja`, MWCC) stays exactly as upstream and must keep
  both SHA1s. The host build is CMake in `port/`.
- **Generated, not hand-kept, source lists.** `port/tools/gen_sources.py` reads `config/` (delinks, symbols) and emits
  the C files to compile: every `.c` in `src/` and `lib/` (complete or not), minus `hal/replaced.txt`.
- **Trap stubs for code that isn't C yet.** For every function with no C, the generator emits a stub that prints the
  function's name, address and caller, then stops. Running the game shows exactly which missing function is next, and
  the list shrinks as upstream lands C.
- **Clang on every platform** (Apple clang on macOS, clang-cl or mingw-clang on Windows, clang on Linux, GCC as an
  extra check): one compiler means one set of layouts and undefined behaviour to verify, and MSVC's `cl` doesn't
  accept the tree's GCC-style `ATTRIBUTE_ALIGN`. On the `windows-msvc` target the tree's own `size_t` typedef
  (`unsigned long`) also clashes with clang's in 992 files until the libc headers go behind the platform switch
  (§4.8 rule 8).
- **Compiler settings that match MWCC's language model:** `-std=gnu99 -fsigned-char` (MWCC's `-char signed`; Linux
  arm64 defaults to unsigned), int-sized enums (never `-fshort-enums`), `-fwrapv`, `-fno-strict-aliasing`
  (mandatory: the code puns types everywhere), `-ffp-contract=off`, no fast-math, `-ftrivial-auto-var-init=zero`
  (uninitialized locals read deterministic garbage on the DS), `-fno-common`. Errors on int-conversion, incompatible
  pointers, implicit declarations and missing returns from the start; the 64-bit warnings (`-Wpointer-to-int-cast`,
  `-Wint-to-pointer-cast`, `-Wshorten-64-to-32`) are counted every build, must only go down, and become errors for
  game code once clean.
- **Defines:** `PLATFORM_PC` on the host build; `BLACK2`/`WHITE2` as upstream; `BUGFIX` off by default, so vanilla
  gameplay can be compared with the reference. Memory-safety bugs are different: the documented out-of-bounds
  accesses, double frees and use-after-frees (about 18 of the decomp's `// BUG:` comments) hit different data once
  64-bit layouts move, so the host build fixes those under `PLATFORM_PC`, each with a comment, even when gameplay
  bugs stay vanilla.
- **Configurations:** Debug (ASan and UBSan available), RelWithDebInfo (default), Release. Game code is optimized
  (`-O2`); the Platinum port's unoptimized game code is a mistake not to copy.

### 4.5 HAL and display model

- **OS:** SDK threads become fibers (minicoro) run by one scheduler with the DS's priority rules on a single host
  thread. That makes the game deterministic, which replays and multiplayer need. A virtual clock drives `OS_GetTick`,
  alarms and the RTC in test mode. `OS_WaitIrq(VBlank)` is where a frame ends: the scheduler runs the VBlank handler,
  presents, and paces to 60 Hz. Disabling interrupts is a no-op, since nothing preempts the game thread.
- **Per-scanline effects:** at scan-out the renderer walks lines 0–191 and, before each, calls the game's HBlank handler
  (GFL's per-line task list), snapshotting the registers it changes. `VCOUNT` reads return a plausible line (192 right
  after VBlank starts, where `mcss.c` and battle uploads expect it).
- **Display model:** one host struct holds the register state of both 2D engines, the 3D control registers, capture
  and `VRAMCNT`; arrays hold palettes, OAM and the nine VRAM banks. Under `PLATFORM_PC` the SDK's register macros point
  into it, so the SDK's 166 `gx` functions (3 of them `asm`, which get C versions) and the 79 header inlines that read
  or write registers (used at 1,569 game sites) compile unchanged. Registers whose writes act (geometry command ports
  and FIFO, divider and sqrt, DMA, capture start, interrupt acknowledge) become function calls. The few raw literal
  addresses in game code (5 outside comments at `0c2bbee9d`, among them `0x06800000` in `event_mapchange.c` and
  `0x050005e2` in `cg_help.c`) become SDK constants (a matching-neutral change).
- **VRAM mapping by page aliasing:** each bank is one block of shared memory mapped into every view (LCDC, BG, OBJ,
  texture, palette) it is assigned to, using `mach_vm_remap` on macOS, `memfd`/`mmap` on Linux and `MapViewOfFile` on
  Windows. Pointers from `G2_GetBG0CharPtr()` then behave exactly as on the DS, writes through one view are visible in
  the others, and remapping on a `VRAMCNT` write costs nothing. Bank sizes are multiples of 16 KB, Apple Silicon's
  page size. libntr only does bookkeeping here, which breaks aliasing. If aliasing proves awkward on a platform, the
  fallback is copy-on-remap; Windows maps views at 64 KB granularity, so the 16 and 32 KB banks (F, G, H, I) may need
  it there.
- **Files and overlays:** NitroFS is served straight from the memory-mapped ROM through the VFS (§5.1). All overlays are
  linked into the executable. Each overlay's source files are compiled with its `.data` and `.bss` in their own named
  sections (spike in phase 0: `#pragma clang section` or section attributes via a forced include). Activating an
  overlay restores its `.data` from a snapshot taken at startup and zeroes its `.bss`, which reproduces the DS's reload
  semantics generically; the Platinum port skipped this and patched cases by hand.
- **Math and memory:** DMA is `memcpy`; the hardware divider and square root become bit-exact C; the 20 `asm` MI
  routines (copy, fill, swap and `MI_UncompressLZ8`) and the 15 `asm` FX matrix helpers get C versions, and so do the
  FX matrix products whose unlinked decomp C differs from the ROM's code
  ([ADR 0008](adr/0008-host-versions-of-sdk-c-that-isnt-the-roms.md)); MSL's soft-float runtime is replaced by the
  host's.
- **Heaps:** the game keeps its own GFL heaps over a host arena, which reproduces the DS's tolerance of small overruns.
  Pointer-bearing structs grow on 64-bit, so heap sizes are scaled on hosts by measurement (each heap reports its
  high-water mark in debug builds), not blindly doubled as the Platinum port did. An ASan build maps the GFL
  allocator to `malloc` so overruns are caught instead of tolerated.
- **Saves:** a 524,288-byte image per profile with atomic writes (temporary file and rename), import and export of
  melonDS `.sav` and DeSmuME `.dsv`, adopting the save's RTC offset and MAC on import.
- **Input:** SDL3 keyboard and gamepads map to the DS buttons; mouse and touch map to the touch panel through the
  inverse of the current screen layout; everything is remappable.
- **Stubs:** DS Protect (overlays 165 and 337) and the IR chip check (overlay 338) return genuine-cartridge results;
  DSi mode reports FALSE; Download Play, wireless and Wi-Fi are stubbed until phase 7.

### 4.6 Renderers

- **2D, CPU reference first:** both engines (text, affine, extended and bitmap backgrounds; normal, affine and bitmap
  sprites; windows; blending; mosaic; master brightness) rendered per scanline at 256×192. It should cost well under a
  millisecond a frame (an estimate, not measured), is exact for raster effects, and is the oracle for every later
  renderer.
- **3D:** the geometry engine runs on the CPU at the API level (matrix stacks, lighting, clipping, box test, readbacks),
  producing a per-frame polygon list with DS attributes. The GPU rasterizes it through SDL_GPU at any internal
  resolution: polygon ID and fog flag targets, stencil for shadow and translucency rules, post passes for edge marking,
  fog and anti-aliasing, and textures decoded from texture VRAM with a cache invalidated by VRAM write generations.
- **Capture and dual-screen 3D:** capture renders into full-resolution textures, downsampled into VRAM only when the
  CPU reads that bank.
- **GPU 2D compositor (phase 6):** the same inputs composited on the GPU, which allows high-resolution and widescreen
  2D. The CPU renderer stays as the reference.
- **Presentation:** screen layouts (stacked, side by side, large + small, single screen with swap, separate windows),
  integer or smooth scaling, and an optional frame-blend filter.

### 4.7 Sound

- **ARM9 side unchanged:** NNS_Snd (193 functions) and the SDK's ARM9 SND code stay as C and keep sending command
  packets with `SND_FlushCommand`.
- **ARM7 side in host C:** a driver that consumes those packets and updates the shared player state, written from the
  ARM7's own asm with the decomp's tools (the project's "write C from the asm" rule applies), so sequencing matches
  the original tick for tick. It covers SSEQ sequences, SBNK/SWAR instruments, IMA-ADPCM, PCM8/16, PSG and noise, and
  the per-track controls the interactive music uses.
- **Mixer and output:** our own mixer at about 32,728 Hz, resampled by SDL3 to the device rate, ticked on the game
  thread so audio stays deterministic. No reverb is needed: BW2 initializes capture but never starts it.

### 4.8 64-bit and dual-target rules

The C must compile for MWCC (byte-identical ROMs) and for 64-bit hosts. Rules for every change outside `port/`:

1. A change either compiles to the same bytes under MWCC (the ROM build proves it) or sits behind `PLATFORM_PC`.
   One header (`include/platform.h`, included by `types.h`) is the only place that decides which.
2. Base types are already right: `u32`/`s32` are `int` in `include/types.h`, so they stay 32-bit on every host
   (libntr makes them `long` except on Linux and Switch, so they are 64-bit on macOS).
3. A value that carries an address uses `uptr`/`sptr`: `u32`/`s32` under MWCC (so retyping a parameter changes no DS
   code) and `uintptr_t`/`intptr_t` on hosts. Retype the API parameters and fields, not every cast.
4. Structs on disk, on the wire or in the save stay pointer-free and carry host `_Static_assert`s on size and key
   offsets. The 61 sizes recoverable from the ROM and the 64 `// size:` comments are the starting list. `s64`, `u64`
   and `f64` get `aligned(4)` typedefs on hosts so these structs keep the DS layout; `RTCDate` and `RivalDataSave`'s
   `unsigned long` fields get a typedef that is `u32` on hosts.
5. File data that NitroSystem relocates in place (G2D cells and animations, G3D resources, sound bank links) gets
   offset accessors or a load-time copy into host structs, with libntrsystem's 64-bit fixes as a checklist. The file
   bytes stay pristine.
6. No new `long`: it is 64-bit on macOS and Linux but 32-bit on Windows.
7. Fixed-size opaque buffers sized for 4-byte pointers are sized from the real struct.
8. Host builds use the host's libc headers (the tree's `include/` libc headers go behind the platform switch) and only
   MSL's fdlibm math, for bit-exact results.

**Measured starting point** ([RESEARCH.md §9](RESEARCH.md#9-the-native-c-compile-probe)): all 1,170 game files
compile for arm64 macOS once `OVERLAY_ID(n)` becomes `n` on hosts (700 of the 1,164 truncation sites). The other
464 sites are in 154 files, all explicit casts, so every one is greppable. `lib/` failures are 43 `asm` files (C or
no-op versions, or replaced by the HAL) and 6 dialect issues. The estimate is 500–700 edited lines in about 160 game
files, 4–6 engineer-weeks including sanitizer triage. A wasm32 layout check in CI verifies the typedefs without
needing the pointer fixes first: with the `aligned(4)` typedefs, wasm32 differs from MWCC's layouts (for which i386
is an exact proxy) in one MSL union and in no save struct.

## 5. Mod architecture

Designed in from phase 0, so features land as mods instead of patches.

### 5.1 Extension points

- **Files:** a layered virtual file system. From bottom to top: the verified base ROM, the port's own fix pack, mods in
  the player's order, and a hot-reload developer folder. Overrides work per file, per NARC member, per text message
  and per sound by SDAT name. A CTRMap-CE project's `vfs/` folder mounts directly, and a `rom2mod` importer turns a
  patched or randomized ROM into a mod folder, warning when the hack changed code.
- **Data registries:** tables compiled into code (the TM/HM list and palettes, move tutors, type chart, starter
  cries, Pickup, the intro Pokémon) exposed by their decomp symbol names, so mods and the randomizer change entries
  without touching code. Shops and the other randomized tables are archive data, which the VFS covers.
- **Hooks, in four tiers:**
  1. Wrapping the game's own tables without source edits: proc tables (`GameProcFunctions`), field script command
     tables, battle event handler tables. This gives mods new script commands, abilities, items and moves.
  2. Typed events at chosen sites: `PORT_EVENT(OnWildEncounter, &params)`.
  3. Vanilla-behaviour hooks in Ship of Harkinian's form: `if (PORT_SHOULD(VB_GIVE_EXP, original_condition))`.
  4. Function-level replace, entry and return hooks, later.

  Tiers 2 and 3 are macros that compile to nothing (or to the bare expression) in the ROM build, so the ROM stays
  byte-identical.
- **Settings:** a registry of typed console variables with ImGui widgets bound to them, saved as TOML.
- **Scripting:** Lua 5.4, sandboxed per mod, with bindings, editor type stubs and docs generated from the decomp's
  headers, plus a console and hot reload. Native C plugins behind a versioned ABI come later; WASM only if mods start
  travelling over the network.
- **Mod package:** a folder or zip with `manifest.json` (id, version, API level, supported games, dependencies,
  conflicts, permissions, options) and `data/`, `fs/`, `text/`, `textures/`, `audio/` and `scripts/`.
- **Saves:** the DS image stays clean; port and mod state go in a sidecar with named, versioned sections.

### 5.2 Widescreen

Following elyro1's Platinum mod, the most complete widescreen design on a DS decomp port so far (Gen4Recomp widens only
the 3D layer, inside its geometry engine, with black margins):

- Widen the 3D scenes through `GFL_G3DSysMtxSetProjection` and the field camera in `FieldG3D_Init`
  (`src/ov036/fieldmap.c`); culling follows because it uses the box test against the projection. The alternative is
  Gen4Recomp's: scale the clip matrix's X output inside our geometry engine (§4.6), with no game edit and with the
  readbacks (clip matrix, position test) kept native.
- Extend map streaming (`MAP_CONFIGS`) with scenery-only chunks and raise the NPC limits so the margins aren't empty;
  collision and scripts still use the original chunks.
- Keep 2D layers native and centred, anchor HUD elements to the edges, and fill margins on 2D-only screens.
- Fix scenes where widening exposes something (NPCs walking off the old edge, map borders) with checked data deltas or
  hooks.
- The bottom screen and C-Gear stay a 4:3 touch panel.

It lands after the GPU 2D compositor, field first, then battle.

### 5.3 Randomizer

Built in, configured by settings, with the seed in the save sidecar. It writes to the data registries and VFS (species
data, learnsets, evolutions, trainers, encounters, Hidden Grottoes, items, shops, TMs and tutors) and uses hooks for
what UPR does with code patches (starters, intro Pokémon, guaranteed catch, fast text). Randomized ROMs from UPR keep
working through the VFS and the in-code table reader. Archipelago support can come later through events.

### 5.4 Multiplayer

In order of value per effort:

1. **Local wireless over the network.** A host device behind `GFLNetDevTable` (`include/gfl/net.h`), selected in
   `src/system/net_define.c`, carrying the game's own protocol unchanged over UDP on a LAN, then through a relay with
   NAT traversal. Union Room, trades, battles and multi battles work between port players.
2. **Broadcast features:** Tag Mode, Funfest Missions and wireless Mystery Gift, relayed.
3. **Infrared as a pairing code**, replacing the IR link for the C-Gear and Feeling Check.
4. **Download Play features as save import** (Poké Transfer from Gen 4 saves, Memory Link from Black/White saves).
5. **Nintendo Wi-Fi Connection** through decompiled DWC and NitroWiFi over host sockets, pointed at revival servers
   (Kaeru WFC/Wiimmfi, Pokémon Classic Network) for cross-play with DS and emulator players. This waits for
   overlay 11's C.
6. **New online features** (co-op, lobbies with pinned mod sets as Gen1Recomp's arena profiles do), later.

## 6. Verification

Testing is how we know the port is the game. Each level runs automatically, from every commit to nightly:

| Level | What | When |
| --- | --- | --- |
| T0 | ROM build: both SHA1s match, so port changes and upstream merges never alter the DS build | every commit a session makes and every local sync; the Action's hourly merges are checked by the next session's sync (CI has no ROMs; it compiles every C file for both versions instead) |
| T1 | Host build (macOS from phase 0; Linux and Windows from phase 5); the 64-bit warning count only goes down | every commit |
| T2 | Unit tests: LZ, NitroFS and VFS, save CRC and slot choice, divider and sqrt bit-exactness, scheduler order, geometry engine maths, 2D renderer golden images from captured display state, SSEQ sequencer against recorded channel state | every commit |
| T3 | Replay tests: recorded input played on the port with a virtual clock; frame hashes and game-state checkpoints against stored goldens | every commit (short), nightly (long) |
| T4 | Oracle tests: the same inputs on melonDS with the real ROM; screens compared (2D exactly, 3D within a perceptual tolerance) and game state compared by decomp symbol (position, party, RNG, current proc and overlays) | nightly, and when a milestone closes |
| T5 | Sanitizers: replays under ASan and UBSan | nightly |
| T6 | Compatibility: saves round-trip with melonDS; ROM verification of good, trimmed and bad dumps | every commit |
| T7 | Playtests: the app driven by scripted input with screenshots, against each milestone's checklist | at each milestone |

- **The oracle harness** (`port/tools/oracle/`) is a small headless program on melonDS's core library that loads the
  ROM, plays an input file, and dumps frames and memory at named symbols (addresses from `config/`). Because the field
  runs at 30 Hz and the DS drops frames under load, inputs are aligned on game-logic frames (a frame counter read from
  RAM) rather than video frames. Building it is part of phase 1.
- **Equivalence of unlinked C:** the 747 KB of C that the ROM doesn't link (and the 442 nonmatching rows) are where the
  port can be wrong while the ROM is right. Oracle state comparisons are the main check, and we keep an "equivalent:
  yes/no/unknown" note for each row we check in `port/docs/decomp-findings.md` (§9).
- **Determinism** is required for T3 and T4: fibers instead of preemptive threads, a virtual clock, the RTC fixed in
  tests, and RNG seeded from the same inputs as the DS.

## 7. Roadmap

Each phase ends when its exit criteria pass on macOS. Phases overlap where the decomp allows.

### Phase 0: foundations

- The repository and its upstream sync (D1, done at creation: §4.1); CI for the macOS host build beside the hourly
  sync's compile check; `port/CLAUDE.md`, `port/docs/STATUS.md`. The ROM build and its SHA1 check stay local, since
  CI has no ROMs.
- CMake skeleton with SDL3; generated source lists; trap stubs; `include/platform.h` with `PLATFORM_PC`, `uptr` and
  the host typedefs; `OVERLAY_ID(n)` → `n` on hosts; `replaced.txt`; the compile probe and progress tracker wired in as
  metrics.
- Compile every game and library C file for arm64 macOS and fix the 64-bit sites far enough to link (the 9
  incompatible function-pointer casts, the 5 missing returns and the save-struct typedefs first).
- Spikes: overlay `.data`/`.bss` sections on Mach-O; VRAM page aliasing on macOS; fibers with DS priority scheduling.
- **Exit:** the executable links on macOS, starts, opens a window and reaches `main()`; both ROMs still match; CI green.

### Phase 1: boot to the logos (M1)

- Scheduler, virtual clock, VBlank and HBlank dispatch; arenas and heaps; ROM verification and memory-mapped NitroFS;
  overlay activation; backup image; keys and touch; a null sound driver that acknowledges commands.
- Display model and the CPU 2D renderer (text and bitmap backgrounds, palettes, master brightness).
- Dual-screen window. The oracle harness and the first replay test.
- **Exit:** the boot screens and Game Freak logo sequence play and match the oracle frame for frame.

### Phase 2: title screen and start menu (M2)

- Sprites (the four ITCM ClAct renderers need C: unless someone else claims them first, a decomp session that the user
  starts writes them in the decomp checkout as ordinary decomp work, §9).
- Geometry engine, GPU 3D rasterizer, capture.
- The sound driver: sequences, banks, waves, the title stream.
- **Exit:** the title screen runs with music and 3D, the start menu responds, oracle comparisons pass within tolerance.

### Phase 3: new game to the overworld (M3, M4)

- Requires overlay 36's C and data (issue #83); until they land, trap stubs show what is left.
- Intro and naming, field 3D maps, NPCs, scripts, text boxes, map transitions, weather, C-Gear on the bottom screen.
- **Exit:** a new game reaches Aspertia; the player walks, talks, enters buildings and changes maps; replays pass.

### Phase 4: battles, menus, saves (M5–M7)

- Wild and trainer battles (overlays 167–170), encounter effects, bag, party, Pokédex, PC, summary.
- Save and load, melonDS save import and export.
- **Exit:** catch a Pokémon, save, quit, reload; a melonDS save continues in the port and back.

### Phase 5: the whole game, hardened

- Story progression through replay routes, each kept as a regression test; sanitizer-clean replays; 60 fps with
  headroom; controller support; one executable for both versions; an ad-hoc signed `.app` in a DMG, with "Open
  Anyway" instructions (no notarization, D11).
- Windows and Linux builds in CI.
- **Exit:** the story can be finished on Black 2 and White 2; a private release candidate.

### Phase 6: enhancement and mod platform

- VFS layers and mod manifests, settings registry and ImGui menus, hook bus and Lua with generated bindings, sample
  mods.
- High internal 3D resolution, the GPU 2D compositor, widescreen (field, then battle), fast-forward, quality-of-life.
- The built-in randomizer.
- **Exit:** widescreen field play, a randomized run, and each sample mod working from a fresh install.

### Phase 7: multiplayer

- The UDP device behind `GFLNetDevTable` on a LAN, then the relay and NAT traversal; infrared as pairing; broadcast
  features; save import for Download Play features.
- Later, DWC over host sockets to revival servers once overlay 11 is C.
- **Exit:** two port instances trade and battle over the internet.

## 8. How each development iteration runs

Sessions working toward this plan follow one loop, so progress is measurable and nothing regresses:

1. **Sync.** Run [`port/tools/sync_upstream.sh`](../tools/sync_upstream.sh) ([SYNC.md](SYNC.md)): it merges
   upstream `main` into the port's `main` (on top of whatever the hourly Action already merged), runs the ROM build
   and checks both SHA1s, builds and tests the host side once `port/CMakeLists.txt` exists, and pushes. A sync that
   doesn't build both ROMs is never pushed. Then regenerate source lists and stubs, and record the decomp tiers,
   trap-stub count, 64-bit warning count and test results in `port/docs/STATUS.md`.
2. **Pick the next task:** the earliest unmet exit criterion of the current phase that the decomp's state allows. A
   task blocked on missing C waits for upstream (or is offered to the decomp session); it is never worked around with
   ARM code or a hack.
3. **Build it in small commits**, each passing T0–T2 and T6. Large subsystems (renderer, sound driver, scheduler) go to
   subagents in their own worktrees, one writer per set of files.
4. **Verify** with the replay and oracle tests, and by running the app and looking at screenshots for anything visual.
5. **Record:** the status log, an architecture decision record for each non-obvious choice, every game-code change in
   `port/docs/game-changes.md`, and lessons in the port's docs.
6. **Publish** to the port repository; never push to upstream, and never post anything about the port there (D10).

## 9. Working with the decomp

- **Where decomp work happens.** Decomp work (matching, new source files, config changes, overlay claims) is done
  upstream, from the decomp checkout `/Users/cjcampos/dev/decomp/pokebw2` (`origin` = `fuddlesworth/pokebw2`, `fork` =
  `CallanCampos/pokebw2`) through fork PRs, under the root `CLAUDE.md`. It reaches the port only by sync. Port work is
  done in the port clone `/Users/cjcampos/dev/decomp/pokebw2-native` (`origin` = `CallanCampos/pokebw2-native`,
  `upstream` = `fuddlesworth/pokebw2`) and its `.claude/worktrees`, under `port/CLAUDE.md`. In the port clone the root
  `CLAUDE.md`'s "push origin HEAD:main" publishes to the port repository.
- **What the port needs from the decomp, in order:** overlay 36's C and data (mariano54, #83), including the
  818-pointer tables of `fldmmdl_blact.c`; overlay 12's event-script command tables as C data (674 pointers); the
  four ITCM ClAct renderers (no issue or owner yet: a decomp session can write them in the decomp checkout as ordinary
  decomp work); overlays 11 and 182 for Wi-Fi (#72, #132), only for phase 7. As far as the research found, nothing
  else blocks phases 0–5.
- **The decomp session keeps its own workflow.** The port never edits decomp files except for the three kinds of
  change in §4.1, and never competes with an overlay claim.
- **Nothing about the port goes upstream (D10).** No issues, comments, pull requests or proposals on
  `fuddlesworth/pokebw2` mention the port, and the portability conventions of §4.8 stay in this repository. Decomp work
  that the port needs is done by decomp sessions as ordinary decomp work, and what they post never mentions the port.
  Port issues, pull requests and commit messages here never link upstream issues or pull requests (`#NNN` with the
  owner, or their URLs), because GitHub would show those links on upstream's timelines, and never @-mention upstream's
  maintainers or contributors, because a mention can notify them. Port commit messages name only the port's own issues,
  as a bare `#N`, and port issues, pull requests and comments should too: no `OWNER/REPO#N` and no issue, pull request
  or discussion URL of any repository, a third party's included. A third-party issue is named by its title. Whatever the
  repository's visibility, both syncs also stop before pushing commits whose messages reference issues
  (`sync_upstream.sh` exit 5, the Action's `references` outcome), and only the user decides to push them with
  `--allow-references`.
- **Port findings stay here:** when the oracle shows that unlinked C behaves differently from the ROM, the finding goes
  in `port/docs/decomp-findings.md`. You decide whether a decomp session fixes it, as ordinary decomp work.

## 10. Risks

| Risk | Mitigation |
| --- | --- |
| 64-bit layout bugs surface one crash at a time, as in the Platinum port | compiler warnings as a metric, size asserts, ASan/UBSan replays, oracle state checks |
| Unlinked C that compiles but behaves differently from the ROM | oracle comparisons by symbol, recorded in `port/docs/decomp-findings.md` |
| DS 3D rules on a GPU (edge marking, depth tolerance, translucency, shadows) | CPU geometry engine, oracle images with tolerance, a CPU rasterizer as a later accuracy mode |
| Display capture and dual-screen 3D at high resolution | full-resolution capture textures, lazy downsampling |
| Lag frames and 30 Hz logic make oracle comparisons drift | align on game-logic frames |
| Port changes break the matching build | T0 on every commit, `PLATFORM_PC` only, a list of every game-code change |
| Widescreen needs scene-by-scene work | field first, following a proven design; margins filled elsewhere |
| SDL_GPU limits and the untagged SDL_shadercross | pinned commit, offline shader compilation, sokol_gfx as fallback |
| Gatekeeper friction without notarization (D11) | ad-hoc signed builds with "Open Anyway" instructions, and building from source documented as the first-class path |
| Upstream churn (renames, file moves) | generated source lists and bindings, hourly merges, a ROM-verified sync at the start of every session |

## 11. Decisions for you

D1, the repository, was decided on 2026-10-16 (§4.1); D10 (no upstream posts) and D11 (no Apple Developer account) on
2026-10-17. Still open:

1. **Scope check:** the definition of "native" in §1, with an emulator used only as a test oracle.
