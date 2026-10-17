# Native port of Pokémon Black 2 and White 2: research

Research for a native PC and Mac port built from the pokebw2 decompilation, done on 2026-10-16 against
`fuddlesworth/pokebw2` at `0c2bbee9d`. It covers the decomp's state, the Pokémon Platinum PC port and its library
suite, other decomp-based ports, the BW2 modding ecosystem, and the technology for a Mac-first build. The plan that
follows from it is [PLAN.md](PLAN.md). The full reports behind each section, with their evidence, are in
[research/](research/). The port repository was seeded from a later upstream `main`, `462e90193`; the numbers here
were not re-measured there.

## Summary

- **The decomp is ready to start a port.** Of 41,018 ARM9 functions (excluding 2,384 linker veneers), 91% already
  exist as C: 75.7% in complete, matching, linked files and 15.5% as C in files not yet linked. Game code with no C is
  only about 122 KB, almost all of it the field engine in overlay 36, which mariano54 claimed on 2026-10-16 and is
  re-landing from a fork branch that, by his count on issue #83, has about 2,700 more matching overlay 36 functions
  than `main`. At each area's recent pace, game code could reach full C in one to two weeks; the rest only by
  mid-November, and only if overlay 182 keeps its pace and someone takes the untracked ITCM, crt0 and DSi remnants.
  These are extrapolations from a 20-day-old project that moves in large merges. The ARM7 binary isn't decompiled at
  all.
- **The precedent is cybervisi0n/pokeplatinum (`pc_port`) on libntr.** It could be finished start to finish 28 days
  after its code went public (it was developed privately before that, for an unknown time). About half of its changed
  game-code lines (by a heuristic count) fix pointers stored in 32-bit integers. It has no tests, its DS ROM build no
  longer matches, it has no macOS target, and it rejects AI-made contributions.
- **libntr should be a source of patterns, not a dependency.** It is pret's NitroSDK 4.2 decompilation plus a
  register-level "simulator" (2D engines, geometry engine, sound unit), parts of which closely match melonDS (GPLv3)
  without attribution. It lacks features BW2 uses (edge marking, display capture, per-scanline effects, translucency
  sorting, opening files by ID), needs OpenGL 4.2 (macOS stops at 4.1), and makes `u32` 64-bit on macOS. pokebw2
  already has its own NitroSDK, NitroSystem and TwlSDK in C (much of it matching and linked, most of the rest written
  but not yet linked; TwlSDK's DSi-only LTD code, crt0 and BIOS stubs and NitroSDK's ITCM IRQ handler and reset code
  have no C yet), so most of what libntr provides is already here.
- **The platform boundary is a hybrid.** Game code, GFL, NitroSystem and most of NitroSDK, `gx` included, compile
  natively unchanged. 217 SDK entry points get host implementations (threads, interrupts, files, saves, input, clock,
  the divider; 48 of them wireless stubs), and the display hardware the game programs directly is modelled as plain
  state that host 2D and 3D renderers draw from. The ARM7's sound driver needs a host replacement.
- **The stack is SDL3 and SDL_GPU** (Metal on Mac, Vulkan, D3D12), built with CMake beside the untouched ROM build.
- **Mods are designed in from the start.** The successful ports (Ship of Harkinian on libultraship, sm64coopdx,
  devilutionX, OpenRCT2) share most of these traits: game code kept close to the original below a platform layer,
  assets read from the user's own copy of the game through a layered file system, hook points that compile away, a
  settings registry with menus, a scripting API with generated bindings, and enhancements built on the same hooks as
  mods.
- **No BW2 PC port exists anywhere.** The only Gen 5 games that run as PC programs are Black and White in
  Gen4Recomp, statically recompiled from the cartridge rather than built from a decomp (§5). pokebw2 is far ahead of
  every other Gen 5 code project.

## 1. State of the decomp

Measured on `0c2bbee9d`, Black 2 ([decomp-progress.md](research/decomp-progress.md); the scripts re-run in about 5
minutes and are kept in [tools/progress](../tools/progress/)).

| Tier | Functions | Code bytes |
| --- | ---: | ---: |
| (a) complete file, matching C, linked | 31,068 (75.7%) | 2,751,648 (71.5%) |
| (b) C written, file incomplete, ROM code still linked | 6,354 (15.5%) | 747,236 (19.4%) |
| (c) no C at all | 3,596 (8.8%) | 351,360 (9.1%) |

- **What has no C:** overlay 11, the Wi-Fi stack (143 KB); overlay 36, the field engine (111 KB, 35 registered files
  with no `.c`, the largest `fldmmdl_acmd.c` and `field_msgbg.c`); overlay 182, the Wi-Fi utility and a Download Play
  tail (79 KB); the DSi-only LTD autoload (11 KB); ITCM (5 KB, including Game Freak's four ClAct sprite renderers);
  ARM9 main (2 KB of crt0, BIOS stubs and `exit`). The engine in ARM9 main is otherwise entirely C.
- **Tier (b) is C for every function.** 168 of 169 incomplete files define all their functions; only
  `lib/dwcutil/src/dwcutil_ac.c` is partial. But the ROM hashes check none of it: 432 of its functions (145 KB) are
  listed as nonmatching, and the rest match individually without being linked. A port running this code needs its own
  equivalence testing.
- **Data:** about 84% of initialized data (excluding the DSi DSP firmware blob and a linker table) exists as C. 3,707 of
  13,063 pointer words in data are not yet C: 2,789 in overlay 36 and 674 in overlay 12's event-script command
  tables (`EVCMD_TABLE`), which sit in a data gap although all overlay 12 code is C. Pointer tables can't be copied
  into a 64-bit build as bytes, so these must become C.
- **Nonmatching functions:** 442 rows (436 functions, 6 data layouts). Most are register allocation or ordering. One
  row records a real behaviour difference (`CPS_SslCanRead`), and 14 NitroWiFi/DWC rows (`wcm.c`, `ssl.c`, `gt2.c`)
  describe C rewritten for behaviour that is unverified.
- **ARM7:** nothing configured or decompiled. `arm7.bin` (168 KB) and `arm7i.bin` (291 KB) are stock TwlSDK 5.3
  components, identical in both versions.
- **White 2:** identical tiers; version differences are `BLACK2`/`WHITE2` blocks in 50 source files and 3 headers,
  and only 8 of the 318 NitroFS files differ.
- **Pace:** the repository is 20 days old (first commit 2026-09-27). In the last 7 days, code with no C fell from 1.98
  MB to 351 KB. Estimates: overlay 36 around Oct 20–25, overlay 11 around Oct 25–30, overlay 182 around Nov 11
  (the long pole). ITCM and crt0 have no issues and no owner.
- **Tracking:** 123 `[Decomp] Overlay` issues, 101 closed. Active claims: #72 (ov011), #83 (ov036), #132 (ov182) by
  mariano54; #120 (ov167) by CallanCampos; #131, #134, #155 by manta89640. ARM9 main, ITCM and LTD have no issues.
- **Already in upstream's goals:** the README's long-term goal is a native PC port, and `docs/porting-notes.md`
  records platform assumptions in NitroWiFi's SSL, SOC and WCM code.

## 2. The game's engine and boot path

From [engine-boot.md](research/engine-boot.md).

- **Boot:** `main()` in `src/system/main.c` → `GFLAppInit` (`gfl_use.c`) → `GameInit`, which loads the resident
  overlays 12 and 27. Each frame: input, `GCTX_ProcMgrUpdate` (the whole game), `GameUpdate`, the 3D buffer swap,
  `OS_WaitIrq(VBlank)`, then VBlank tasks, BG register flush, OAM flush and fades. The field runs its logic at 30 Hz.
- **Core systems, all complete C:** process stack (`procsys.c`), tasks (`tcb.c`), events, 193 heap IDs over
  NitroSystem's heap, the overlay manager (`gf_overlay.c`, 20 slots), BG, windows, fades, input, RTC, save, network
  layer. Written but not yet complete: `arc_tool.c` (the 308 archives by ID), `printsys.c`, `mcss.c`, `wipe_sub.c`,
  `ica_anime.c`, `pokedex.c`.
- **Flow:** boot screens (ov162) → Game Freak logo and opening (ov264, LZ11 bitmap frame sequences, no video codec) →
  title (ov162: 3D model, display capture motion blur, streamed music) → New Game (ov294 intro, ov280 name entry) →
  ov35 season banner and ov293 demo → field (ov36 + ov33) in Aspertia. Battle: ov167/168/169/170, then ov166. Every
  menu overlaps ov36, so the field closes and reopens around each.

| Milestone | Code still without C | Notes |
| --- | --- | --- |
| M1 logos | none in game code (crt0/BIOS/IRQ, replaced by the platform layer) | 2D, bitmap BG, archives, save read |
| M2 title / start menu | 4 ClAct renderers in ITCM (3 KB) | 3D, capture, streamed music |
| M3 new game → Aspertia | 10 ov36 files, 456 functions / 27 KB | the main gate |
| M4 walk, NPCs, talk, warps | 19 ov36 files, 880 functions / 61 KB (`fldmmdl_acmd` 430) | |
| M5 wild battle | `enceff.c` (21 functions) | battle code is all C; its data is C but for 58 bytes, likely padding or pooling |
| M6 save / load | nothing beyond M3 | |
| M7 menus | none (`shortcut_menu.c` is C; 4 functions documented as nonmatching) | bag, Pokédex, PC, summary complete |

- **Overlay semantics:** 344 overlays, 167 with `.data`, 34 with `.bss` (221 KB). The DS re-copies `.data` and zeroes
  `.bss` on each load; some code relies on it (lazily created pointers expecting zero, counters never reset). Only
  overlay 165 has static initializers (DS Protect's self-decryptors).
- **Anti-piracy:** DS Protect (ov165, ov337) checksums its own ARM code, which always fails natively: extra random
  draws every frame, a 4 KB heap leak per map change, a crash leaving the party list. The IR chip check (ov338) skips
  post-faint experience and hides beacons on failure. Both must be stubbed to "genuine cartridge".
- **Displays:** the main engine is on top for field, battle and 3D scenes and on the bottom for touch menus. Dual-3D
  (`double3Ddisp.c`) shows 3D on both screens through capture. Capture is also used by building entry, field-move
  cut-ins, battle intros and the title. The C-Gear is the sub engine on the bottom screen.
- **Lineage:** BW2's GFL is the same Game Freak code as Platinum's engine for overlays, tasks, archives and strings,
  restructured for heaps, processes and BG, and rewritten for sprites, 3D, sound, messages and saves. The Platinum
  port's game-side patches don't transfer as code; its categories of fixes do.

## 3. The SDK and hardware surface

From [sdk-surface.md](research/sdk-surface.md), measured over all 43,384 functions and 199,455 reference edges,
including code still in assembly.

- **Game code references 950 library functions at 11,070 sites**: NitroSDK 384, NitroSystem 238, MSL runtime 37,
  networking 218, TwlSDK 48, SPL 18, DS Protect 7. Most referenced: `MI_CpuFill8` (1,637), `MI_CpuCopy8` (503),
  `FX_Div` (175), `G2x_SetBlendAlpha_` (171), `GXx_SetMasterBrightness_` (126).
- **79 SDK header inlines that read or write registers are used at 1,569 game sites**, and game code writes VRAM,
  capture and blend registers directly in about 25 files. So the display hardware's state has to exist on the host
  regardless of where the boundary sits.
- **3D is a command stream with readback:** NNS G3D, SPL and 33 game files (`mcss.c` among them) issue geometry
  commands, and the game reads back clip matrices and box-test results. A host geometry engine is needed in every
  design.
- **Layering:** NitroSystem (680 functions) and most of NitroSDK (FS core, FX, MI copy, STD, OS sync, ARM9 SND) are
  portable logic. Hardware-bound: OS threads, IRQ, timers, cache, `gx` register writes, DMA, CARD, PXI, TP, PM, MIC,
  RTC, WM, the hardware divider. Stub: MPU/TCM, CTRDG, MB, DS Protect, DSi-only TwlSDK modules (with
  `OS_IsRunOnTwl()` returning FALSE).
- **Boundary options:** (i) replacing NitroSDK wholesale at its API (602 entry points referenced from outside the SDK,
  or 356 once its portable modules are kept); (ii) a full hardware model (about 150–171 registers in 18 blocks, DMA,
  timers, IRQ, the ARM7 over PXI): an emulator without the CPU; (iii) GFL-level graphics (748 functions, but game
  code bypasses GFL at 1,177 sites). **The recommendation is a hybrid:** keep the portable SDK C and the 139 `gx`
  entry points over a host register file, implement the other 217 of the 356 on the host (OS 64, wireless 48 as
  stubs, touch/power/mic 37, divider 30, card 12, sound commands 8, MI 5, RTC 5, ROM reads 4, NVRAM 4), and model
  only the display state (registers, VRAM, palettes, OAM) that the renderers read.
- **asm:** 188 `asm` functions, all in `lib/` (NitroSDK 95, MSL 51, DS Protect 31, DWC 6, TwlSDK 5), none in `src/`.
  `MI_UncompressLZ8` (LZ10/LZ11) is the decompression the game relies on.
- **Threads:** 19 `OS_CreateThread` sites, mostly network libraries. The game itself has one sound-loading thread.
  `VCOUNT` is read in `wipe_sub.c`, `mcss.c`, `field_g3d_mapper.c`, the battle effect code and others, so the host
  must report plausible scanlines.

## 4. Assets, ARM7, sound and saves

From [assets-arm7-sound.md](research/assets-arm7-sound.md).

- **ROM:** a 512 MiB DS/DSi hybrid built with TwlSDK 5.3. ARM9 main 628,544 bytes at 0x02004000, ITCM 5,024 bytes,
  DTCM 160 bytes, the LTD (DSi) autoload 437,984 bytes, 344 overlays (3.69 MB), ARM7 167,812 bytes, ARM7i 291,064 bytes.
- **File system:** 318 files, 271 MiB. 308 numbered files under `a/` (306 of them NARCs) are addressed by archive ID
  through a 308-entry path table (`a/0/0/0`…`a/3/0/7`); with `skb.narc` and `soundstatus.narc` there are 308 NARCs,
  holding 69,850 members. Formats: NCGR 10,452, NCLR 5,072, NCER/NANR 3,643 each, NSBTX 1,725,
  NSBMD 840 plus 1,065 map chunks, NSBCA 512, SPL 791, NFTR 5, text 1,171, field scripts 1,289. 12,422 members are LZ
  compressed, with compression chosen by the caller, not marked in the file.
- **64-bit catch:** NitroSystem loaders write pointers into 32-bit slots of loaded files (G2D unpacking, sound
  bank-to-wave links). They need host-side structures or offset accessors.
- **The decomp's `data/`** rebuilds 52 of 318 files from editable source (text, species, moves, items, trainers,
  encounters, maps as glTF, textures as PNG, scripts, trainer AI, facilities): 34% of file-system bytes. It commits
  about 99,500 converted Nintendo files, so a port release must never ship it.
- **Sound:** an 89.4 MB SDAT: 1,390 sequences (273 BGM and 1,066 SE among them), 1,949 banks and 1,949 wave
  archives, 8,714 waves (8,024 IMA-ADPCM), 7 players plus a stream player, one stream (the title theme). Capture is
  initialized but never started, so no reverb. A host driver needs per-track mute, volume, pitch, pan and modulation
  at any time (interactive music), tick readback, RAM-loaded sequences, cries, streams and the mic.
- **ARM7:** the sound driver (sequencer, 16 channels, mixer) is the part that must be rebuilt. The other ARM7
  services (RTC, touch, power, mic, firmware, save chip, wireless) are easier to replace at the ARM9 SDK API.
- **Saves:** 512 KiB flash. Main save 0x26000 bytes at 0 with a backup at 0x26000, per-block CRC16, footer magic
  0x31053527; 21 extra saves up to 0x7E000. A raw 524,288-byte image is directly compatible with melonDS `.sav` files
  and cartridge dumps. The save stores the console's RTC offset and MAC; on import the port should adopt them, or the
  game resets daily events.
- **DSi:** Xtransceiver video chat, WPA Wi-Fi, battery levels and bigger heaps. Run as a plain DS by default.
- **First launch:** verify the game code and whole-ROM SHA1, fall back to a per-file SHA1 manifest (generated for both
  versions) that tolerates trimmed dumps, then serve files from the ROM directly instead of unpacking it.

## 5. The Pokémon Platinum PC port

From [platinum-port.md](research/platinum-port.md) and [platinum-mods-alternatives.md](research/platinum-mods-alternatives.md).

- **What it is:** [cybervisi0n/pokeplatinum](https://github.com/cybervisi0n/pokeplatinum), branch `pc_port`, a fork
  of [pret/pokeplatinum](https://github.com/pret/pokeplatinum), with
  [cybervisi0n/libntr](https://github.com/cybervisi0n/libntr) replacing NitroSDK. Targets win64, Linux and Switch;
  no macOS. Meson build. 116 stars.
- **History:** branched from a pret commit of 2026-06-05; public history starts with a squashed commit on
  2026-06-13. v0.0.1 came 20 days after that, v0.1.0 (game finishable) on day 28, ROM extraction at v0.1.8 (Oct 2), a
  GPU texture cache at v0.1.10. 107 of 111 commits are by one developer. Open issues: 11 rendering fidelity, 4
  crashes, 4 input, 2 double-speed timing, 2 missing DS hardware.
- **Game-code changes:** 239 files, +3,365/−434 lines, plus a new `src/port/` (ImGui, config, cheats). 697 guard
  directives (`SDK_BUILD_ARM` 485, `SDK_PORT` 146). By a heuristic classification, roughly 2,000 of 3,900 changed
  lines fix 64-bit pointer width:
  index fields carrying function pointers, `u32` function tables, sentinels written as `0xffffffff`, fixed-size opaque
  contexts. They moved from `(u64)` casts to a `uPtr` type and never used `uintptr_t`.
- **Other changes:** `NULL`→`0` in about 100 places, missing prototypes, `static` dropped on tables declared
  non-static, the main heaps doubled rather than measured, about 11 divide-by-zero and 5 NULL guards, asserts and
  warnings off, unoptimized game code, 60 fps by default with five hand-placed speed fixes and the rest running at
  double speed.
- **Overlays:** all linked into one executable; no re-copy of `.data` or zeroing of `.bss`; the static initializers
  that mattered (the 25 Poketch apps registering themselves) are redone by hand.
- **Assets:** on a missing file, libntr opens a file picker, extracts the chosen ROM into the working directory and
  retries. No ROM validation. Releases contain only binaries.
- **Quality:** no automated tests of any kind. The DS build still compiles but CI no longer checks it, and a
  misspelled guard (`SDK_BUILD_ARMs`, July 2026) changed DS code unnoticed. Saves are not DS compatible.
- **Licensing and policy:** the game fork has no license; libntr is MIT (added 2026-08-30) over decompiled code that was
  never licensed, with melonDS-like code and no attribution. Both repositories' README and `AGENTS.md` refuse
  AI-made contributions, so we can't contribute there; we can only learn from them.
- **What transfers to BW2:** well under 5% as literal code, roughly 70–80% as categories and methods.

### libntr

From [libntr.md](research/libntr.md).

- **About 368k lines**, of which about 110k are third-party and about 21k are its own PC layer (renderer core 6.6k).
  699 lines with `SDK_PORT` in 195 SDK files. Exact Nintendo names, because it is the SDK decompilation itself.
  NitroSystem lives in a sibling `libntrsystem` with about 2.8k lines of 64-bit fixes; NitroWiFi and DWC in more
  siblings, plus `libtwl` and `libtwlsystem`.
- **Rendering:** every I/O register is a C global and VRAM, palettes and OAM are host arrays. At each VBlank wait the
  game thread renders the frame: a CPU renderer draws 2D layers line by line, and a CPU geometry engine feeds OpenGL.
  Missing: perspective-correct texturing, translucency sorting, edge marking, toon, anti-aliasing, alpha test, shadow
  polygons, display capture, per-scanline HBlank, BG mosaic. 3D upscaling only; 2D stays 256×192.
- **Audio:** Nintendo's own ARM7 sound driver compiled natively, ticking every 5.2 ms inside the audio callback,
  driving a 16-channel mixer.
- **OS:** SDK threads are preemptive host threads without a scheduler; disabling interrupts does nothing; alarms on
  SDL timers.
- **Wireless:** DS local wireless tunnelled over enet on port 2009, no relay or NAT traversal. Nintendo Wi-Fi goes
  through decompiled NitroWiFi/DWC over host sockets, with the server chosen by in-game DNS.
- **macOS blockers:** no macOS target; `u32`/`s32` are `long` and become 64-bit; OpenGL 4.2 core context and
  `#version 420` shaders; GL debug calls missing on macOS; data paths relative to the working directory; GL calls on
  arbitrary threads.
- **BW2 gaps:** edge marking (37 call sites), clear colour (33), anti-aliasing (33), alpha test (26), display capture
  (7 files), VRAM display mode (14), LCDC mapping (24), windows (31), GFL's HBlank interrupt, in-memory archives,
  opening files by ID, IR, and a different SDK version (NitroSDK 4.2 / TwlSDK 5.5 against BW2's TwlSDK 5.3).

### The mods built on it

- **Widescreen ([elyro1/pokeplatinum](https://github.com/elyro1/pokeplatinum), `pc_single_screen_mod`):** widens only
  the field, from 256 to 448 columns (21:9). The projection's `_00` term is scaled by 256/W; culling follows because
  the box test uses that matrix. 2D layers stay 256 wide and centred; battles and menus stay 4:3 with a coloured
  fill. Most of the work is what widening exposes: a PC-only cache of up to 64 map chunks drawn as scenery, NPC
  preloading with the object cap raised from 80 to 192, cutscene NPCs that would walk off the old edge, and map
  edges extended by SHA-256-checked deltas applied to the player's extracted data. 180 game files changed, about 82
  of them unguarded, so its DS ROM no longer matches.
- **Single screen ([evoluketion/PokemonPurePlatinum](https://github.com/evoluketion/PokemonPurePlatinum)):** draws
  the top engine first and the bottom engine over it with a transparent backdrop, with a per-scene placement API.
  Small game-side changes; its libntr patch fixes real 3D bugs (perspective-correct texturing, translucent depth
  writes).
- **[Gen4Recomp](https://github.com/sohamb117/gen4recomp):** compiles the pret DS decomps to wasm32 and back to native
  C with wasm2c, keeping 32-bit pointers, over hardware models taken from melonDS (GPL). Black/White are statically
  recompiled from the cartridge, not decompiled. Its LAN Union Room re-implements the ARM7 wireless manager over UDP
  with a Go relay. Ideas only: GPL contamination, no top-level license, and a sibling project hosting commercial ROMs.

## 6. Other decomp-based ports

From [port-landscape.md](research/port-landscape.md).

- **libultraship ([Kenix3/libultraship](https://github.com/Kenix3/libultraship)) and its ports** (Ship of Harkinian,
  2 Ship 2 Harkinian, Starship, SpaghettiKart, Ghostship): the N64 SDK reimplemented so decompiled games compile
  against it unchanged ("compile-time WINE"). Fast3D interprets display lists into OpenGL, D3D11 or Metal. Torch
  converts assets from the user's ROM keyed by SHA-1. ZIP archives where the last loaded wins make a mod just a file
  at the same path. Settings are CVars bound to ImGui widgets, and each enhancement is one self-registering file.
  The key hook pattern is `GameInteractor_Should(VB_ID, <original expression>)`, which keeps vanilla code readable;
  Ship of Harkinian has 785 such sites, and its randomizer and Anchor co-op both sit on that layer. Its weakness:
  game code is a frozen, edited snapshot of the decomp, so these ports now carry scripts to find their drift from it.
- **[sm64coopdx](https://github.com/coop-deluxe/sm64coopdx):** Lua 5.3 mods with about 70 hooks, bindings and docs
  generated from game headers, sync tables replicating values between players, host-sends-mods-to-clients.
- **Static recompilation** (N64Recomp, Zelda64Recomp): machine code to C over an emulated memory map, mods hooking
  recompiled functions. Not a fit: BW2 has a decomp, and recompiled code can't mix with native code.
- **GBA ports:** the Sonic Advance 2 port lives inside its decomp repository behind `PORTABLE`/`PLATFORM_GBA` guards,
  so merging upstream is trivial; it gets most of its widescreen by writing `DISPLAY_WIDTH` instead of 240.
- **Long-lived community ports:** devilutionX moved hard-coded tables to TSV and made an expansion a mod; OpenRCT2 has
  a typed scripting API and every state change as a serializable action (which keeps multiplayer in sync). Newer,
  and despite its name a hand-written Lua/LÖVE2D reimplementation rather than a recompilation,
  [Gen1Recomp](https://github.com/bryanthaboi/gen1recomp) (created 2026-07) has mod manifests with semver,
  permissions and an "arena profile" that pins engine version and mod bundle for online play.
- **DS precedents besides Platinum:** a Super Mario 64 DS decomp with a Windows port
  ([tangosdev/sm64ds-decomp](https://github.com/tangosdev/sm64ds-decomp)), DS static recompilers (ndsrecomp, ndskit),
  and HeartGold port attempts that are empty, dormant or unverified.

## 7. BW2 modding, randomizer, widescreen and multiplayer

From [bw2-mod-ecosystem.md](research/bw2-mod-ecosystem.md).

- **swan** ([ds-pokemon-hacking/swan](https://github.com/ds-pokemon-hacking/swan), GPL-3.0) is the Gen 5 hacking
  community's headers and symbol database; pokebw2 imports revision `4324f73`. Code-injection mods (PMC) declare hooks by
  swan names, which the decomp also uses.
- **Randomizer:** checked against [UPR ZX](https://github.com/Ajarmar/universal-pokemon-randomizer-zx) and its successor
  UPR FVX. 22 archives are data-only and carry over if the port reads assets from a randomized ROM. Eight tables live
  in code (TM/HM moves, TM palettes, tutors, starter cries, two intro species tables, Pickup, the type chart at
  0x021D7500 in `src/ov167/btl_calc.c`) and 16 edits are code patches (fast text, Challenge Mode, guaranteed catch,
  and others). All eight tables are named C constants in the decomp, and the port can read them back from a
  randomized ROM at fixed offsets. A built-in randomizer should substitute at runtime hooks (encounters, Pokémon and
  item creation, shops, starters, TM and tutor lookups).
- **Widescreen:** the only existing hack (TWiLight Menu++'s 16:10 code) changes the overworld camera's aspect in
  `FieldG3D_Init` (`src/ov036/fieldmap.c`). In the decomp, projections go through `GFL_G3DSysMtxSetProjection`,
  culling uses the box test, map streaming is one table (`MAP_CONFIGS`), NPCs are camera-facing sprites and trainer
  sight is tile-based. 2D is the hard part: fixed HUD pixels, 256-wide capture buffers, 8-bit window coordinates.
- **Multiplayer:** every connected mode except Download Play goes through `GFLNetDevTable` (`include/gfl/net.h`),
  selected in `src/system/net_define.c`: local wireless (overlay 30, `wih.c`), infrared (overlay 31 + `lib/irc`), Wi-Fi
  (overlay 11, with the GTS in overlay 189). Battle Videos go to the Global Data Server through overlay 201's own
  client. Broadcast-only features (Tag Mode, Funfest Missions, wireless Mystery Gift) never open a connection and are
  easy to relay. Revival services (Kaeru WFC in front of Wiimmfi, Pokémon Classic Network) support the GTS, Battle
  Videos, Musical photos and the Wi-Fi Battle Subway; Game Sync works only through self-hosted Entralinked. Proposed
  order: a UDP/relay device behind `GFLNetDevTable` with the game's protocol unchanged (4–8 person-weeks for LAN,
  3–6 more for the relay), infrared as a pairing code, Download Play as save import, then DWC over host sockets to
  revival servers (blocked until overlay 11 is C).
- **Hacks and tools:** hacks target the same clean US ROMs and ship as xdelta patches. CTRMap-CE projects are a
  `.cmproj` plus a `vfs/` folder of changed files, which a port can mount directly. Frost's Gen 5 Editor, PokeScript,
  and Pokeweb exist; PMC modules are ARM code and can't run natively, but their swan-named hook declarations suggest a
  source-compatible hook API.
- **No BW2 port, recompilation or native reimplementation exists.** For Black and White the only one is
  Gen4Recomp's static recompilation (§5), which the ecosystem report's survey missed. Other Gen 5 decomps are tiny (a
  Black decomp at 0.03%, a BW2 disassembly started 2026-10-08).

## 8. Technology for a Mac-first native build

From [tech-stack.md](research/tech-stack.md); versions as of 2026-10-16.

- **SDL3 3.4.x** (3.4.18, 2026-10-02) for windows, input (gamepads, touch, pen), audio streams, file dialogs and
  multiple windows.
- **SDL_GPU** for Metal, Vulkan and D3D12 with compute. Shaders in HLSL compiled offline with SDL_shadercross to
  SPIR-V, MSL and DXIL (pin a commit; it has no releases). Limits: no GPU timestamps, 24-bit depth not guaranteed
  (Apple Silicon offers 32-bit float plus stencil). Fallbacks: sokol_gfx, wgpu-native. OpenGL is deprecated on macOS
  and capped at 4.1.
- **2D:** a CPU compositor at 256×192 first (exact for per-scanline effects, an estimated well under a millisecond a
  frame, and the test reference), then a GPU compositor from the same inputs for widescreen and high-resolution 2D.
- **3D:** the DS geometry engine on the CPU at the API level; rasterization on the GPU with extra targets for polygon
  ID and fog flags, stencil for shadow and translucency rules, post passes for edge marking and fog. Capture renders
  to full-resolution textures and downsamples only when the CPU reads that VRAM.
- **Build:** CMake 4.x in a separate port tree; the `configure.py`/MWCC ROM build stays untouched and both are built
  on every change (the ROM build where the ROMs are, since they are never committed). Apple clang for builds,
  clang-cl on Windows with MSVC as a strictness check, GCC and clang on Linux. The compile probe (§9) later found that
  MSVC's `cl` can't take the tree's GCC-style attributes, so the plan uses clang everywhere.
- **Sanitizers:** ASan, UBSan, TSan on macOS arm64; all plus LeakSanitizer on Linux; ASan (and UBSan with clang-cl)
  on Windows.
- **Packaging:** never ship assets. A statically linked `.app` in a DMG; ad-hoc-signed builds need "Open Anyway" on
  macOS 15+; Developer ID notarization costs USD 99 a year. Windows zip, Linux AppImage. GitHub runners `macos-26`,
  `ubuntu-24.04`, `windows-2025`; `macos-14` retires 2026-11-02.
- **Mods:** Dear ImGui (docking) for menus; Lua 5.4 with a curated API (Luau if mods travel over the network; WASM via
  WAMR or wasmtime later); a layered virtual file system (PhysicsFS or miniz); settings saved as TOML; dmon for hot
  reload.
- **Networking:** ENet for transport, libjuice for NAT traversal, a small lobby server with a coturn relay as
  fallback; GameNetworkingSockets if encryption or relays become requirements.
- **Profiling and testing:** Tracy; replays of recorded input with the game's clocks virtualized; headless screenshot
  hashes from the CPU reference renderers; GPU tests on Linux with lavapipe; reference images from an emulator running
  the real ROM.
- **Licensing:** every recommended library is GPLv3-compatible. Keep AGPL servers (coopnet) as separate programs.

## 9. The native C compile probe

From [compile-probe.md](research/compile-probe.md): clang over every C file in `src/` and `lib/` for the DS
target and for 64-bit hosts, with struct layouts compared between MWCC and each host ABI. The probe scripts are kept
in [tools/compile-probe](../tools/compile-probe/) so the numbers can be re-measured as a port metric.

- **The game's C is close to host-ready.** Clang accepts all 1,170 `src/` files for 32-bit targets, and for arm64
  macOS once one macro changes (`OVERLAY_ID(n)`, a `(u32)` of a linker symbol, to `n`). Every remaining failure is in
  `lib/`: 43 files with MWCC `asm` and 6 with small dialect issues. Host builds should use clang on all three systems:
  MSVC's `cl` doesn't accept the tree's GCC-style `ATTRIBUTE_ALIGN`, and on the `windows-msvc` target the tree's own
  `size_t` typedef clashes with clang's in 992 files until the libc headers go behind the platform switch.
- **Base types are already right:** `u32`/`s32` are `int` (`include/types.h`), MWCC uses `-char signed` and int-sized
  enums. Linux arm64 defaults to unsigned `char`, so host builds need `-fsigned-char`.
- **64-bit pointer debt is small and greppable.** On arm64 there are 1,164 unique pointer/int truncation sites in game
  code, 700 of them the overlay-ID macro. That leaves 464 sites in 154 files: about 311 carry pointers through `u32`
  parameters, fields, returns and variables (SetParam-style APIs, task and callback user data, menu parameters; 102 of
  them read an integer back as a pointer), 115 are narrowings (about 74 real, 41 intentional `u64`→`u32`), 25 are
  hardware addresses and 13 are alignment computations. No game site
  converts implicitly: `-Wint-conversion`, `-Wincompatible-pointer-types` and `-Wimplicit-function-declaration` find
  nothing. `lib/` has 678 sites in 117 files, concentrated in libraries the port replaces.
- **Layouts:** i386 SysV matches MWCC exactly (all 61 struct sizes recoverable from the ROM's own `sizeof` constants,
  all 64 `// size:` comments), so it is the layout reference for checks. On arm64, 53% of 4,447 records change, which
  only matters where something assumes the layout. **Save blocks are pointer-free** and keep the DS layout on 64-bit
  hosts with two changes: `aligned(4)` typedefs for `s64`/`u64`/`f64`, and a typedef for the `unsigned long` fields in
  `RTCDate` and `RivalDataSave`.
- **Behaviour the DS tolerated:** pervasive type punning (`-fno-strict-aliasing` is mandatory), 282 cast-align sites,
  531 variable shifts (ARM gives 0 at ≥32, hosts mask), 56 sometimes-uninitialized locals, 158 `// BUG:` comments with
  about 15 out-of-bounds accesses and 3 double frees or use-after-frees, 5 missing returns, 9 casts between
  incompatible function-pointer types (7 in game code), one unsequenced evaluation, and MWCC's division-by-zero
  returning a value. Recommended flags: `-std=gnu99 -fno-strict-aliasing -fwrapv -fsigned-char -ffp-contract=off
  -ftrivial-auto-var-init=zero -fno-common`, errors on int-conversion, incompatible pointers, implicit declarations
  and missing returns, and ASan + UBSan replays (MSan on Linux).
- **The 32-bit alternative** (wasm32 then wasm2c, as Gen4Recomp does; or `-m32`) would remove the pointer work: wasm32
  and i386 compile the same 1,520 files with zero pointer warnings, and with `aligned(4)` typedefs only one MSL union
  differs from MWCC. But `-m32` can't target Apple Silicon at all, and wasm2c gives up debugging, sanitizers, native
  mods and direct platform calls, to save a few weeks. Native 64-bit is the better path; wasm32 stays useful as a cheap
  CI check that the layout typedefs are right.
- **Effort:** roughly 500–700 edited lines in about 160 of the 1,170 game files plus a few header typedefs, about 4–6
  engineer-weeks including sanitizer triage. The fixes keep MWCC byte-identical: a `uptr` typedef that is `u32` under
  MWCC and `uintptr_t` on hosts, so retyping parameters changes no DS code; host-only size asserts; offset accessors
  or a load-time copy for NitroSystem's in-place relocation (G2D: 30 sites in 7 files, about 55 pointer fields); host
  libc headers behind the platform switch, keeping only MSL's fdlibm math for bit-exact results. The HAL and renderers
  remain the dominant cost.
