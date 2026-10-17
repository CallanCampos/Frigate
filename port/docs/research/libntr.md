# libntr: technical analysis for a native Black 2/White 2 port

Snapshot analysed: `cybervisi0n/libntr` main at `1555d91` (2026-10-09), cloned under `research/libntr`. The Platinum port
that uses it (`cybervisi0n/pokeplatinum`, branch `pc_port`, last commit `35067868f`, 2026-10-10) is at
`research/pokeplatinum-pc`. Nothing was built or run. Paths below are relative to the libntr clone unless they start
with `pokeplatinum-pc/` or `pokebw2:`.

**How to read this:** **[V]** means I read it in the code, git history or GitHub API. **[I]** means I inferred it and
did not check it in the code.

---

## 0. Summary

- libntr is **pret's matching NitroSDK 4.2 decompilation** (`ntrtwl/NitroSDK`, the same tree pret's pokeplatinum
  used, pinned at `e6fbeba`) **plus a PC "simulator" layer** (`libraries/sim`, `libraries/sim7`, `include/simulator`,
  about 21k lines of its own code) **plus ~700 `#ifdef SDK_PORT` edits in 195 SDK files** [V]. In September 2026 a
  TwlSDK 5.5 tree ("libtwl") was merged in as a beta [V].
- The SDK API is the real one: **Nintendo's exact function names and almost all signatures**, compiled natively. Below
  the SDK, the hardware is replaced by **C globals for every I/O register and host arrays for VRAM, palettes and OAM**.
  At each VBlank a renderer **reads that register and VRAM state**. The 2D side is a CPU software PPU that renders each
  layer, and an OpenGL shader composites them. The 3D side is a CPU geometry engine (matrix stacks, lighting,
  clipping, W/Z), and OpenGL rasterizes its output. Sound is the ARM7 sequencer compiled natively plus a 16-channel
  DS-SPU mixer [V]. **The 2D PPU, the SPU mixer and parts of the 3D engine closely match melonDS (GPLv3) code, with no
  attribution, inside a repository labelled MIT** [V for the similarity, I for the provenance].
- In emulator terms there is **no CPU emulation**, but this **is a hardware-model layer**: an emulated 2D PPU and SPU,
  and an emulated geometry engine with GPU rasterization. It works like sm64-port's Fast3D, but one level lower,
  because it reads registers and VRAM, not display lists.
- Maturity: a single-developer project (109 of 128 commits), and the PC layer is 4 months old. There are no runtime
  tests, a lot of TODO and placeholder code, and visible correctness gaps: no edge marking, toon, clear colour,
  shadow polygons, translucency sorting, display capture, per-scanline HBlank or VRAM display mode. Bank mapping is
  approximated. Shipping Platinum builds still crash in places (open issues #20, #25, #27, #44).
- **macOS:** the code has no x86-specific parts; the NX target already builds for AArch64. It still does not build
  or run on Apple Silicon without about a dozen changes: no macOS target, `u32` becomes 64-bit on LP64, an OpenGL 4.2
  context, `#version 420` shaders, KHR_debug calls, and paths relative to the current directory (section 10).
- **Recommendation:** do **not** take libntr as a dependency and do **not** fork it wholesale. Build pokebw2's own
  platform layer on pokebw2's own `lib/nitro`, `lib/twl` and `lib/nnsys`, and reuse libntr's *patterns*, listed in
  section 14.

---

## 1. Layout, size, languages, build, licence, authors, timeline

### Layout and size [V]

Line counts cover C, C++, headers, GLSL, lex and yacc files.

| Directory | Lines | Files | What it is |
| --- | ---: | ---: | --- |
| `libraries/sim` | 97,849 (11,380 own) | 67 | PC "simulator": SDL2 window, 2D/3D renderer, audio mixer, net, GUI, config, ROM extractor. Most of the rest is ImGui, glad, ImGuiFileDialog, blip_buf, inih and fireflower |
| `libraries/sim7` | 9,224 (8,366) | 23 | ARM7-side SDK code built for the host: SND sequencer and ex-channels (`snd_seq.c` 1,679 lines, `snd_exchannel.c`), SPI/NVRAM, WM, and the simulated PXI (`pxi.c`) |
| `include/simulator` | 23,676 (1,414) | 25 | Simulator headers; glad.h alone is 17,285 lines |
| `include/nitro` | 55,484 | 310 | NitroSDK headers, including the new `hw/X86/*` register and memory-map headers |
| `include/twl` | 32,180 | 161 | TwlSDK headers (merged September 2026) |
| `libraries/{os,mi,fs,fx,gx,snd,card,spi,...}` | ~143k across all non-sim libraries | 333 `.c` | Decompiled NitroSDK and TwlSDK code |
| `libraries/wl_arm7`, `wm_arm7` | 19,741 / 9,996 | | ARM7 wireless firmware libraries, built for the `arm` target only |
| `tools/makelcf` | 3,946 | | Nintendo-style LCF generator (C, flex, bison) |

- Whole tree: about 368k lines. About 110k of those are bundled third-party code. **The PC-specific code is about
  21k lines**: `sim` 11.4k, `sim7` 8.4k and the simulator headers 1.4k. The renderer core is about 6.6k of that:
  `sim_g2.cpp` 1,798, `g3_handler.cpp` 1,778, `sim_main.cpp` 1,578, `sim_g3_CommandProcessor.cpp` 655, `g3_draw.cpp`
  414, and 4 shaders of about 300 lines.
- Port edits inside the SDK: **699 `SDK_PORT` lines in 195 files** [V, `git grep -c`].
- Languages by GitHub byte count: C 10.1 MB, C++ 3.8 MB, then Meson, Yacc/Lex, GLSL, Python and assembly.

### Build [V]

- Build system: **Meson ≥ 1.3**. The `project()` call at `meson.build:1` declares C, C++ and `nasm`, at version
  `4.2.30001`.
- `meson.options` sets `build_target` to one of `arm | win64 | linux | nx`, plus `sdk_version_major` (4 or 5),
  `hardware_type` (`ntr`, `twl_hyb` or `twl_ltd`), `tracy_enable`, `card_game_code` and `card_maker_code`.
- PC targets add `-DSDK_PORT -DSDK_X86 -DSDK_64BIT -D_NITRO -w` (`meson.build:125-160`). NX adds `SDK_AARCH64`. **`-w`
  silences every warning**, including pointer truncation.
- The Platinum port sets `buildtype=plain` (`pokeplatinum-pc/meson.build:5`). The game and the SDK C code therefore
  build **without optimisation**. The sim's C++ is built at `-O2`; its C files are deliberately left unoptimised
  (commit `ea75e83`).
- CI (`.github/workflows/build_{linux,win}.yml`) only **builds** a standalone cube demo (`standalone/main.c`) for
  NTR and TWL. There are no runtime tests.
- Consumers pull libntr through a wrap pinned to `revision = main, depth = 1` (`pokeplatinum-pc/subprojects/
  libntr.wrap`), so builds are not reproducible. The evoluketion fork pins a commit for exactly that reason.

### Licence and provenance [V unless marked]

`LICENSE` (added 2026-08-30 in commit `e5064a9`, by cybervisi0n) reads:

> MIT License — Copyright (c) 2026 cybervisi0n — Permission is hereby granted, free of charge, …

- **The base code is not cybervisi0n's.** The first 11 commits (2023-09 to 2025-05) are by Nomura, mid-kid,
  PikalaxALT and Rachel (lhearachel). That is `ntrtwl/NitroSDK`, a reconstruction of Nintendo's NitroSDK, and its
  GitHub repository has **no licence**. pret's pokeplatinum pinned exactly `ntrtwl/NitroSDK@e6fbeba`
  (`pokeplatinum-pc` history, `subprojects/NitroSDK.wrap` at `1a68c758e`).
- The PC layer arrived in one commit, `39dc0aa` (2026-06-13), authored **`null <null@null.com>`**: 323 files, +75,972
  lines.
- So the MIT grant covers code that its original authors never licensed, and the underlying work reconstructs
  Nintendo's SDK. That is the same IP position as pokebw2's `lib/nitro`.
- The SDK source files carry **no per-file licence headers**. Neither do the sim files (`grep` for
  copyright/licence/melon in `sim_g2.cpp`, `g3_handler.cpp`, `sim_audio.cpp`, `sim_main.cpp` and `g3_draw.cpp`
  finds nothing).

**Third-party code bundled inside** [V]:

| Component | Path | Licence |
| --- | --- | --- |
| Dear ImGui 1.93 WIP | `libraries/sim/src/imgui`, `include/simulator/imgui` | MIT |
| ImGuiFileDialog | `libraries/sim/src/ImGuiFileDialog` | MIT |
| glad (GL 4.2 loader) and KHR headers | `libraries/sim/src/glad.c`, `include/simulator/glad`, `KHR` | permissive |
| nanotime | `libraries/sim/include/nanotime.h` | Unlicense / MIT |
| inih | `libraries/sim/src/config/ini.c` | BSD-3 |
| fireflower `nds-extract` (Overblade) | `libraries/sim/src/fireflower` | BSD-3 |
| **blip_buf** | `libraries/sim/src/blip-buf` | **LGPL-2.1**, statically linked |

External dependencies: SDL2, enet, and optionally Tracy (`cybervisi0n/tracy`, branch `nx_support`).

**Code that closely matches melonDS (GPLv3), without attribution.** I compared it against melonDS sources fetched
with `gh api`:

- **2D.** `G2SIM_DrawOBJ` (`libraries/sim/src/sim_g2.cpp:254-360`) reproduces melonDS `GPU2D::DrawSprites` line for
  line: the `bgnum = 0x0C00 … -= 0x0400` / `sprnum = 127 … 0` loops, the `spritewidth[16]` and `spriteheight[16]`
  tables, `sizeparam`, `boundwidth`, `rotparamgroup`, and `(s32)(attrib[1] << 23) >> 23`.
  - Also shared with melonDS: `pixelattr = ((attrib[2] & 0x0C00) << 6) | 0xC0000` and `spritemode = window ? 0 :
    ((attrib[0] >> 10) & 0x3)`.
  - The mosaic table built in `sim_main.cpp:1505-1511` (`offset = j % (i + 1)`) is melonDS's `MosaicTable`.
  - Function names follow melonDS: `DrawBGLine_Text/Affine/Extended` and `DrawOBJ_RotScale/Normal`.
- **3D.** In `libraries/sim/src/g3_handler.cpp:101-128`, `ClipSegment` is melonDS `GPU3D.cpp` `ClipSegment` with the
  same `factor_num`/`factor_den`, the same `INTERPOLATE` macro, the attribute block commented out and the
  `//outbuf->Clipped = true;` leftover.
  - `ClipAgainstPlane` and `G3SIM_ClipPolygon` mirror melonDS `ClipAgainstPlane` and `ClipPolygon`.
  - `G3SIM_BoxTest` makes six `ClipPolygon(face, 4, 0, FALSE)` calls, as melonDS does.
  - The W normalisation and Z computation in `G3SIM_SubmitPolygon` (`:1653-1778`) is melonDS `SubmitPolygon`
    (`while ((w >> wsize) && (wsize < 32))`, `((s64)z * 0x4000) / w + 0x3FFF) * 0x200`, `0x7FFE00`).
- **Audio.** `libraries/sim/src/sim_audio.cpp` follows melonDS `SPU.cpp`:
  - The ADPCM tables, `PSGTable[8][8]` and the ADPCM index table are the same.
  - `SIM_Audio_RunChannel` matches `SPUChannel::Run`: `(type < 3) && (len+pnt < 16)`, `Timer = TMR + (Timer -
    0x10000)` and the `NextSample_PCM8/PCM16/ADPCM/PSG/Noise` dispatch.
  - `SIM_Audio_StartChannel` matches `Start()`: position −1 or −3 and `NoiseVal = 0x7FFF`.
  - Master volume is applied as `>> 7` and the output then shifted `>> 8`.

What this means for licensing:

- These files are GPLv3-derived, so libntr's MIT label is unreliable.
- For pokebw2, which is GPLv3, GPLv3 code is licence-compatible, but the melonDS attribution would have to be
  restored.
- It also settles the "emulation layer" question for these parts: they come from an emulator's PPU and SPU.

**Contribution policy:** `AGENTS.md` and the Platinum README say the projects do not accept AI-generated pull requests
or code suggestions. That rules out an AI-assisted upstreaming workflow.

### Authors and timeline [V]

- 128 commits in all: cybervisi0n 109, Nomura 4 (plus 2 as Nomura-RH), Rachel 5, PikalaxALT 2, James Petersen 2,
  Scott Broeders 1, coronesys 1, mid-kid 1, and "null" 1.
- First commit `1958e1e`, 2023-09-02 (Nomura's import). Last commit `1555d91`, 2026-10-09.
- Commits per month: 2026-06: 13, 2026-07: 11, 2026-08: 25, 2026-09: 47 (the TWL merge, the C++/ImGui refactor),
  2026-10 (to the 9th): 21.
- **Last 3 months** (since 2026-07-16): 96 commits, 91 by cybervisi0n, +148k/−89k lines (mostly the TWL merge).
- GitHub (`gh api`): repo created 2026-05-30, 6 stars, 4 forks (elyro1, colinven, coronesys, DiegoT346-jpg).
  **Issues are disabled.** There are 3 PRs: #1 the author's own, #2 merged from coronesys, #3 open.
- No tags or releases on libntr; releases happen in the Platinum repo, v0.0.1 (2026-07-03) to v0.1.12 (2026-10-10).
- Sibling repos by the same author:
  - `libntrsystem`, a GitHub fork of `ntrtwl/NitroSystem` with no licence.
  - `libntrwifi` and `libntrdwc`, no licence.
  - `libtwl` and `libtwlsystem`.
  - Unrelated projects: `libPorpoise`, `ps5-*`, `ac-decomp`, `melee`. The author's attention is spread over many of
    them.

**Maturity:** early beta. One developer, no tests, many `TODO`, `PCPORT_TODO` and stub bodies. Rendering features are
added game by game as Platinum issues come in (issues #18 shadows, #7 transparency, #21 fog, #42 lighting and #43
BoxTest are still open).

---

## 2. API coverage

**Names and signatures.** libntr *is* NitroSDK, so it keeps Nintendo's exact names [V]. On PC it changes some
signatures and struct layouts [V]:

- `PXIFifoCallback(tag, uPtr data, err)` widens the data argument to 64 bits (`include/nitro/pxi/common/fifo.h:84`).
- `MIi_DmaSetParams_wait(u32, u64, u64, u32)`, and `CP_SetDivImm64_32(s64, s32)` takes signed arguments
  (`include/nitro/cp/divider.h`).
- `OS_CreateThread` becomes a macro that calls `OS_CreateThreadDebug` with the function name, file and line
  (`thread.h:206`).
- `OSThread` gains `u64` stack fields, `SDL_Thread*` and `win_threadState`. As a result **`<SDL2/SDL_thread.h>` is
  included from the SDK header**.
- `OSMutex` gains an `SDL_mutex*`, `OSMessageQueue` a semaphore, `OSAlarm` an `sdlTimer`, and `FSFile` a `FILE*
  pcFilePtr`.
- `SDK_COMPILER_ASSERT` is disabled on PC (`include/nitro/misc.h:29`), so size asserts no longer catch layout changes.

**NitroSystem is not in libntr.** It is `libntrsystem`, a fork of the `ntrtwl/NitroSystem` decompilation.

- Its PC commit `a1fd278` adds +2,797 lines. Most of that is 64-bit-safe G2D resource structs and loaders (NCG, NCL,
  NAN, NFT, NOB) and G3D accessor inlines (`res_struct_accessor_inline.h`, `gecom.c`, `sbc.c`), plus an 11-line
  `snd/capture.c` change [V via `gh api`].
- So the Platinum port compiles **NitroSystem from the matching decompilation's C with 64-bit patches**. It does not
  reimplement NitroSystem.
- NitroWiFi (`libntrwifi`, +8.6k: socket-handle tables in place of pointer-as-int, SSL, CPS) and DWC (`libntrdwc`,
  +16k, mostly C for previously unmatched files) work the same way [V file lists, I purpose].

### Module status table (PC targets)

"Real code" means the decompiled SDK runs natively. "Sim" means the libntr simulator provides it.

| Module | Status on PC | How | Evidence |
| --- | --- | --- | --- |
| OS threads | **Partial** | Each `OSThread` is an SDL host thread (preemptive). The priority scheduler is disabled. `OS_SleepThread` sets WAITING, `SDL_Delay(1)` and **returns** (polling). `OS_WakeupThreadDirect` busy-waits | `os_thread.c:24-60, 377-430, 978-1012, 1075` |
| OS interrupts | **Stub/approx.** | `OS_DisableInterrupts` and `OS_EnableInterrupts` return 0 and **mask nothing**. VBlank and HBlank handlers run **once per frame** inside `SIM_Render` | `os_system.c:4-18`, `sim_main.cpp:1294-1301` |
| OS alarms, valarms, tick | Partial | Alarms are `SDL_AddTimer` callbacks on SDL's timer thread. `OS_GetTick` is `SDL_GetTicks64` (1 ms resolution) | `os_alarm.c:19-60,195,228`, `os_tick.c:73` |
| OS mutex and messages | Works (host) | `SDL_mutex`; `OS_ReceiveMessage` polls `SDL_SemWaitTimeout(…,1)` | `os_mutex.c`, `os_message.c` |
| OS arena and heap | Works | Arenas live in a **static 8 MB array** `s_HW_MAIN_MEM`, doubled from 4 MB for 64-bit growth. Some ITCM/DTCM/WRAM defaults still cast function addresses to `(u32)` | `hw/X86/mmap_global.h:29-37`, `os_arena.c:59-100` |
| SVC (BIOS) | **Stubbed** | `SVC_CpuSet` and `SVC_CpuSetFast` are empty; `SVC_GetCRC16` returns 0; `SVC_Uncompress*` is not provided | `os_systemCall.c:3-13` |
| MI DMA, copy and fill | Works with a bug | DMA is `memcpy` and `memset`. **`MI_DmaFill32` uses `memset`, so it fills only the low byte of a non-zero pattern.** HBlank and VBlank DMA are one immediate `memcpy` | `mi_dma.c:228-231`, `mi_dma_hblank.c:81-92`, `mi_dma_vblank.c` |
| MI GX-FIFO DMA | Works | `MI_SendGXCommand` copies the packed display list into the G3 command processor | `mi_dma_gxcommand.c:54-63,121-128` |
| MI uncompress | Real code | C LZ, RL and Huffman | `mi_uncompress.c` |
| GX/G2 registers | Sim | Every register is a C global (`s_reg_G2_BG0CNT`, …) read by the renderer | `sim_vars.c`, `hw/X86/ioreg_*.h` |
| GX VRAM banks | **Approximate** | Fixed buffers per role (BG 512 KB, OBJ 256 KB, sub engines, LCDC 656 KB). `GX_SetBankFor*` only records the enum. Textures and extended palettes are read from the LCDC copies of banks A-D and E-H. Data written through one mapping is **not visible** through another | `mmap_global.h:66-139`, `gx_vramcnt.c:410-413,797-802,1081-1086`, `g3_draw.cpp:62-117` |
| G3 geometry | **Partial** | Immediate commands and packed lists go to a CPU geometry engine. `G3_MultMtx44` still writes `reg_G3X_GXFIFO` on PC, so it is **lost**. POS_TEST, VEC_TEST and VIEWPORT are not handled | `g3imm.c:65-71`, `sim_g3_CommandProcessor.cpp:507-511` |
| G3X state | Mostly ignored | Read: fog colour, offset, table, `DISP3DCNT` bit 7 and fog shift. **Ignored:** edge colours, toon table, clear colour and depth, alpha-test ref, anti-alias, highlight/toon shading | grep of `s_reg_G3X_*` reads |
| FX / MTX / VEC | Real code | C. `MTX_Scale43_` was only added on 2026-10-06 (`eef1eca`) | `libraries/fx` |
| CP (div/sqrt) | Sim | Writing the numerator and denominator computes the result immediately. Division by 0 leaves the old result. **`CP_SaveContext` is a TODO**, and the registers are globals shared by all host threads | `include/nitro/cp/divider.h:23-60`, `cp_context.c:6-7` |
| MATH, STD, DGT | Real code | Pure C | |
| FS | **Partial** | `FS_OpenFile(path)` becomes `fopen` relative to the current directory, retried in lower case. Read, write and seek use stdio. **`FS_OpenFileFast` (by file ID), `FS_OpenFileDirect` on non-"rom" archives and memory-file archives do not work on PC**: reads silently copy nothing | `fs_file.c:269-305, 438-450, 506-560, 1574-1586` |
| FS overlays | No PC concept | `FS_LoadOverlay` runs the ROM path. The game's overlays are all statically linked into one executable | `fs_overlay.c:649-693` |
| CARD backup | Works (simple) | `save.bin` in the current directory. Each call does a synchronous fopen, fseek, fread or fwrite; no atomic write; the callback fires at once | `include/nitro/card/backup.h:112-182` |
| CARD ROM, raw SPI | No | There is no raw SPI device, so no IR chip | |
| PAD | Works | `s_reg_PAD_KEYINPUT` and `HW_BUTTON_XY_BUF` filled from SDL keyboard and joystick events | `sim_main.cpp:1070-1235` |
| TP | Works | Mouse position mapped into the touch-screen rectangle, passed through the simulated ARM7 SPI | `sim_main.cpp:1018-1061,1240-1251`, `sim7/src/spi/spi_sp.c` |
| RTC | Works | Host `time`/`localtime` | `rtc/src/external.c:183-190` |
| SPI/PM/NVRAM | Works | `firmware.bin` emulates the user-settings flash. PM is answered | `sim7/src/spi/nvram/nvram_instruction.c:17,193` |
| MIC | Stub | PXI tag answered, no capture (Platinum #16) | |
| SND (ARM9 API) | Real code | Commands cross the simulated PXI to the host-side ARM7 driver | `sim7/src/pxi.c` |
| SND (ARM7 driver) | Real code (host) | `snd_seq.c`, `snd_exchannel.c`, `snd_midiplayer.c` run on the host | `sim7/src/snd` |
| SND hardware | Sim | 16-channel SPU mixer (melonDS-derived) with blip_buf. **No SND capture** (reverb and effects) | `sim_audio.cpp` |
| WM | Partial | Host-side ARM7 WM (`wmsp_*`) plus enet over UDP 2009: beacons, join, MP data | `sim7/src/wm`, `sim_net.cpp` |
| MB, WBT, WFS, WXC | Real code over WM | No port edits; untested | |
| WVR | Minimal | Shutdown fix | `f34319a` |
| CTRDG | Stub | 128 KB zeroed `s_HW_CTRDG_ROM`, so no GBA slot (Platinum #19) | `mmap_global.h:142-146` |
| EXI, PAD-ARM7, NVRAM-ARM7, syscall, wl_arm7 | ARM target only | | `libraries/meson.build:51-58` |
| TwlSDK camera, DSP, EL, SCFG, WXC | Compiles; **no PC backend** | 0 `SDK_PORT` edits except SCFG; no ARM7 handler for their PXI tags | file counts above |
| TwlSDK NWM, FATFS, SSP JPEG, DSi mic | **Absent** | `include/twl/nwm` headers only | |
| Crypto (RC4, AES, SHA-1) | Real code | | |
| NNS G2D, G3D, GFD, FND, SND, MCS | **In `libntrsystem`**, not libntr | Matching decompilation plus 64-bit loaders | `gh api` commit `a1fd278` |
| NitroWiFi / DWC | In `libntrwifi` and `libntrdwc` | Compiles with 64-bit handle fixes. **I found no host-network bridge**, so WFC is not shown to work [I] | `gh api` commit file lists |

---

## 3. Rendering: where it sits

### Hardware state [V]

- Every I/O register is a plain global.
  - In `include/nitro/hw/X86/ioreg_G2.h`, `REG_BG0CNT_ADDR` is `&s_reg_G2_BG0CNT`.
  - The 16-bit and 32-bit views of one register (for example `BG0OFS` and `BG0HOFS`) are **separate variables**,
    commented "PCPORT_NOTES: These have the same offset address". A write through one view does not reach the other.
- VRAM, palettes and OAM are host arrays (`HW_BG_VRAM == (u64)s_HW_BG_VRAM`, …). The SDK code writes to them exactly
  as it would on hardware: `GX_LoadBG0Char`, `GX_LoadOAM`, `GX_LoadTex` into the LCDC banks, and so on.

### Frame model [V]

The frame boundary is `OS_WaitIrq(…, OS_IE_V_BLANK)` (`os_irqHandler.c:352-360`). It calls:

1. `SIM_PreRenderVBlank()`
2. `SIM_Render()`, which:
   - flushes 3D and draws the deferred translucent items;
   - pumps SDL events and ImGui;
   - CPU-renders both 2D engines;
   - calls the **HBlank handler once**, then the VBlank handler;
   - sleeps to about 16.6 ms if the frame rate is capped;
   - swaps buffers;
   - re-binds the 3D framebuffer for the next frame.
3. `SIM_PostRenderVBlank()`

All of this runs **on the game thread**, the process main thread: `main()` is in libntr and calls `NitroMain()` after
`SIM_RenderInit` (`sim_main.cpp:1422-1578`).

### 2D: CPU software PPU plus GPU compositor [V]

`DrawEngine(isSub)` (`sim_main.cpp:560-857`):

- Renders OBJs per scanline into 4 per-priority RGBA buffers and an OBJ-window buffer, using
  `G2SIM_DrawOBJ(line, isSub, prio)`.
- Renders each enabled BG **per line, for all 192 lines at once**, into a 256×384 RGBA buffer per BG:
  `DrawBGLine_Text`, `_Affine` or `_Extended` (`sim_g2.cpp`).
- Uploads the buffers with `glTexSubImage2D`.
- Composites in `shaders/g2_fragment.glsl`: priority order, BG alpha blend and brightness up/down (BG as first target
  only), OBJ window against WINOUT, and master brightness.

Mode-by-mode status:

| Feature | Status |
| --- | --- |
| Text BG | Yes |
| Affine BG | Yes |
| Extended BG (256-colour, bitmap, direct) | Yes |
| Large BG (mode 6) | `PCPORT_TODO`, empty (`sim_g2.cpp:183-188`) |
| BG mosaic | Never applied: every call passes `mosaic = 0` (`sim_g2.cpp:99-230`) |
| OBJ mosaic | Branch is empty, which leaves `sprline` uninitialised (`sim_g2.cpp:296-299`) |
| Normal, affine, double-size and bitmap OBJs | Yes; mode 1 is read, semi-transparent OBJ blending is not in the compositor |
| BG extended palettes | Read from the LCDC copies of E/F/G/H (`getBgExtPlttVramBank`) |
| WIN0/WIN1 rectangles | Applied in `DrawBGLine_Text` only (`sim_g2.cpp:589-690`); **not to affine or extended BGs or OBJs**. elyro1's fork had to add this |
| Display capture | **Not implemented**: `DISPCAPCNT` is only a variable |
| Display modes (VRAM, main memory) | **Not implemented** |
| Per-scanline raster effects (HBlank IRQ, HBlank DMA, VCOUNT) | **Not implemented**: one HBlank call per frame; HBlank DMA is one `memcpy` |

### 3D: CPU geometry engine plus OpenGL 4.2 rasterizer [V]

**Input.**

- Immediate `G3_*` inlines and functions build a `draw_msg_t` (one **`malloc` and `free` per command**,
  `g3imm.h:134-146`) and call `SIM_HandleG3Command`.
- Packed display lists sent through `MI_SendGXCommand(Async)`, which is what NNS G3D uses, are copied and parsed by
  `HandleCommandMessage` `DRAW_CMD_G3_CMD_LIST` (`sim_g3_CommandProcessor.cpp:550-640`). This is the hardware GXFIFO
  format.

**Geometry engine** (`g3_handler.cpp`, in `fx32` and float):

- Projection, position and vector matrix stacks; push, pop, store and restore.
- Multiply and load; clip matrix.
- `G3_Normal` lighting with 4 lights, diffuse, ambient, specular and emission. **Shininess is a TODO** (`:1390-1392`).
- Texture-coordinate generation.
- Primitive assembly for triangles, quads and their strips.
- W and Z/W-buffer depth computed as the DS does.
- BoxTest with melonDS clipping. **Polygon clipping is not applied to drawn polygons.** A hack instead discards any
  vertex beyond ±50 in NDC (`:1755-1764`).

**Rasterization:**

- Vertices are already divided by W on the CPU and sent as `GL_TRIANGLES` with W fixed at 1 (`g3_vertex.glsl`). Texture
  interpolation is therefore **affine, not perspective-correct**. evoluketion's patch "Fix translucent and
  perspective 3D rendering" addresses this.
- Drawing happens **immediately during game code**: `glDrawArrays` on every attribute or texture change, into an FBO
  of 256×384×scale.
- The fragment shader (`g3_fragment.glsl`):
  - modulate mode for `polygonMode == 0`, replace for every other mode, so **decal, toon/highlight and shadow are all
    treated alike**;
  - an alpha discard below 0.05;
  - fog from the 32-entry table.
- Translucent polygons (A3I5/A5I3 or alpha < 31) are deferred and drawn at the end of the frame **unsorted** ("TODO sort
  the items by Z"). Depth writes are not handled separately for them.
- **Missing:** edge marking, toon/highlight table, anti-aliasing, clear colour, depth or clear image, alpha-test
  reference, shadow polygons and polygon IDs (Platinum #18 "Pokémon lack shadows"), 1-dot and wireframe polygons,
  W-buffer precision, rear-plane, and display capture of the 3D output.
- **Textures:**
  - All formats are decoded on the CPU from the LCDC copies of banks A-D.
  - **COMP4x4 asserts and is a TODO** (`g3_draw.cpp:225-228`).
  - `getTextureVramBank` returns only the first bank of a slot set, so non-contiguous sets (AC, AD, BD) are wrong.

**Texture cache** (release v0.1.10, commit `5c84bc5` on 2026-08-29, improved in `f0fba0d` on 2026-10-06):

- `std::unordered_map<u64, shared_ptr<Texture>>`, keyed by CRC32 of the texture bytes in the low 32 bits and CRC32
  of **512 bytes of palette** in the high 32 bits (`sim_g3_TextureCache.cpp`).
- GL textures persist, so there is no re-upload. The CPU still CRC-hashes the texture and palette **on every flush**.
- Nothing is ever evicted.
- The key omits format, size and wrap mode, so a texture with the same bytes in another format collides.

**Upscaling:** an integer `internalResolutionScale` slider (commit `12fbb5c`) resizes **only the 3D FBO**. The 2D
layers stay 256×192 and are sampled `GL_NEAREST`.

### Verdict on "emulation layer"

- libntr runs no CPU emulation and no instruction interpretation.
- At the SDK API it is a native build of the SDK, which is high-level emulation by replacement.
- **Below the SDK it is a hardware model.** The 2D engines and the sound unit are emulated at the register and VRAM
  level, and that code comes from melonDS. The 3D geometry engine is emulated in software, and only rasterization is
  native GPU work.
- The project calls this layer the "simulator" (`SIM_*`, `libraries/sim`).
- If "no emulation layer" means no PPU, SPU or geometry-engine model, libntr's sim layer does not meet it. If it
  means no CPU emulator, libntr meets it, in the same way sm64-port's Fast3D does.

---

## 4. Audio [V]

- **The sequencer is the real NitroSDK ARM7 sound driver**, decompiled and compiled for the host: `sim7/src/snd/
  snd_seq.c`, `snd_exchannel.c` and `snd_midiplayer.c`. The ARM9 SND library (`libraries/snd`) sends its command lists
  over the simulated PXI (`sim7/src/pxi.c:72-300`). SSEQ, SBNK and SWAR therefore play through Nintendo's own logic,
  and the NNS SND archive code sits on top (in libntrsystem).
- The driver ticks inside the SDL audio callback: `SND_UpdateExChannel`, `SND_SeqMain(TRUE)` and
  `SND_ExChannelMain(TRUE)` each callback (`sim_audio.cpp:110-118`). The buffer is 230 samples at 44.1 kHz (5.2154
  ms), the DS sound frame, so tempo is right as long as the device honours the buffer size (`sim_audio.cpp:93-97`).
- **The mixer emulates the 16 SPU channels.** It handles PCM8, PCM16, IMA-ADPCM, PSG (channels 8-13) and noise (14-15)
  at the 16.756991 MHz channel clock, steps 512 cycles per output sample, applies panning, master volume and the 0x200
  bias, and band-limits to 44.1 kHz with blip_buf. This is melonDS-derived.
- **Not implemented:** sound capture, so NNS reverb and effects do not run; libntrsystem patches `snd/capture.c`.
- STRM: refill alarms are `SIM_sndAlarm` threads using `SDL_Delay(ms)` (`pxi.c:422-448`). Timing is coarse and drifts.
- The last fixes are "SND: Fix sound register corruption" and "Fix sound volume" (2026-10-04 and 10-08). elyro1's fork
  still lists "fix audio glitches".

---

## 5. Memory model [V]

- **Main RAM.** `HW_MAIN_MEM` is `(u64)s_HW_MAIN_MEM`, a static 8 MB `u8` array. There is also an 8 MB "EX" array.
  Arenas (`OS_GetArenaLo/Hi`) and every game heap live inside it. Pointers are real 64-bit host pointers.
- **VRAM, palette RAM and OAM.** Fixed host arrays per role:

  | Region | Size |
  | --- | --- |
  | BG VRAM | 512 KB |
  | Sub BG VRAM | 128 KB |
  | OBJ VRAM | 256 KB |
  | Sub OBJ VRAM | 128 KB |
  | LCDC | 656 KB |
  | Palettes | 4 × 512 B |
  | OAM | 2 × 1 KB |

  Bank mapping is bookkeeping only (section 2).
- **I/O registers** are globals. `REG_*_ADDR` is the address of the global, so pointer arithmetic over consecutive
  registers is not guaranteed. `MIi_DmaSetParams` does `(vu32*)((u64)REG_DMA0SAD_ADDR + dmaNo * 12)`, which assumes
  the globals are laid out consecutively.
- **DMA** is `memcpy`/`memset`, done immediately (bug in `MI_DmaFill32`, see section 2).
- **64-bit handling:**
  - New `uPtr` type (`types.h:65-75`).
  - `#ifdef SDK_PORT` u64 casts throughout. PXI payloads are widened to `uPtr`.
  - `-w` hides the rest. There was a "fix truncated pointers" pass (`8ab805a`), and some `(u32)` truncations remain
    (`os_arena.c:59,73,79,99`).
  - NNS binary resources that store `u32` offsets fixed up into pointers in place need **new 64-bit struct layouts
    and loaders**. That work is in libntrsystem (G2D `fmt/*_data.h`, `load/*`) and in the game.
- **Endianness:** little only (`types.h:8`). Alignment is not handled; ARM64 and x86 tolerate unaligned scalar access.

---

## 6. OS: threads, interrupts, pacing [V]

- **Threads.** Each `OS_CreateThread` makes an SDL host thread (`os_thread.c:624`). There is no priority scheduling
  (`OSi_RescheduleThread` is `#ifndef SDK_PORT`). Sleep and wakeup are 1 ms polling loops.
  - `OS_DisableInterrupts` is a no-op, and registers such as the CP divider are shared globals.
  - Any SDK or game code that relied on running uninterrupted, or on cooperative scheduling, **now has races** [I,
    from the design].
- **IRQs.** `OS_IRQTable` handlers for HBlank and then VBlank run once per frame inside `SIM_Render`
  (`sim_main.cpp:1297-1301`). VCOUNT, timer, DMA and GXFIFO interrupts are not delivered by any timing model
  (`MI_SendGXCommandAsync` calls its FIFO callback directly).
- **Pacing.** The target is 16.6 ms, about 60.24 Hz rather than the DS's 59.83 Hz. It sleeps with `SDL_Delay(remaining
  ms − 1)` and optionally vsyncs (`SDL_GL_SetSwapInterval(vsyncInterval)`) (`sim_main.cpp:1314-1328`).
  - There is a fast-forward toggle and a hold key (`35d218e`), and "Pause game logic" loops `SIM_Render` without
    returning to the game.
  - There is **no decoupled or interpolated frame rate**. Game logic is tied to the VBlank wait. Platinum's "60 fps"
    option skips the game's second VBlank wait (`pokeplatinum-pc/src/main.c:154-160`).

---

## 7. FS and ROM extraction [V]

- **Path FS.** `FS_OpenFile("rom:/a/0/1/2")` strips `rom:` and the leading `/`, then `fopen`s the path relative to the
  current directory, retrying in lower case. Size comes from `fseek`/`ftell`. Reads and seeks use stdio, and
  "async" reads are synchronous (`fs_file.c:506-560, 269-305, 1574-1586`).
- **Missing:** opening by file ID (`FS_OpenFileFast`), `FS_OpenFileDirect` (it opens `header.bin` for the "rom"
  archive), and **memory files** (`FS_CreateFileFromMemory`). Their reads go down the command path, which is only
  compiled `#ifdef SDK_BUILD_ARM`, so they **copy nothing but report success**.
- **ROM extraction** lives in libntr.
  1. When the first `FS_OpenFile` fails, an SDL message box opens, then the ImGui file dialog.
  2. fireflower `SIM_NDSExtract` dumps the ROM's whole FNT tree, `overlay9/`, `overlay7/` and `header.bin` **into the
     current working directory** (`fs_file.c:541-557`, `sim_RomExtractor.cpp`, `fireflower/nds-extract.cpp:173+`).
  3. The open is retried, recursively.
  - There is **no ROM identity check** (no CRC or SHA), so any `.nds` is accepted.
- **Overlays:** there is no PC mechanism. The game links every overlay into one executable. `FS_LoadOverlay` runs the
  ROM path, and nothing re-initialises an overlay's `.data`/`.bss` on reload [I: the static-reload semantics are lost].

---

## 8. Save, input, windowing [V]

- **Save.** `save.bin` in the current directory, created on the first read (`include/nitro/card/backup.h:112-182`).
  Every access opens, seeks, reads or writes and closes. There is no atomic write and no fsync.
  - Firmware settings go to `firmware.bin`, config to `./sim_config.ini`.
  - elyro1's fork added "isolated PC save slots and legacy save migration".
- **Input.**
  - Keyboard defaults (Z/X/C/V, arrows, Q/E, Return, RShift) can be rebound in ImGui (`sim_config.c:358-372`,
    `sim_gui_pad.cpp`).
  - Gamepad support uses the raw **`SDL_Joystick` API, first device only**, with no `SDL_GameController` mapping
    database (`sim_main.cpp:1063-1235`). Platinum #33 (rebinding does nothing) and #35 (softlock on disconnect) are
    open.
  - The touch screen is the mouse: button state plus position mapped into the bottom-screen rectangle. There is no
    multi-touch or `SDL_FINGER` handling.
- **Window.**
  - Resizable SDL2 window with an OpenGL context.
  - Layouts: vertical (DS-like), horizontal, or "large" (big screen plus small screen). Screens can be swapped. The
    image is letterboxed to keep the aspect ratio (`sim_main.cpp:980-1013`).
  - Fullscreen is desktop-fullscreen (`1555d91`), and a Steam Deck default of 1280×800 was added.
  - There are no filters or shaders: 2D is `GL_NEAREST`, 3D minification is `GL_LINEAR`.
  - The window is not created with `SDL_WINDOW_ALLOW_HIGHDPI`.

---

## 9. Wireless and Wi-Fi [V unless marked]

- **Local wireless (WM).** The ARM7 WM driver is compiled for the host (`sim7/src/wm/wmsp_{system,request,indicate}.c`)
  and tunnelled over **enet** (UDP port 2009). `sim_net.cpp` (920 lines) handles host, client and player lists, game
  info, beacons, join and disconnect, and MP data sent as reliable packets.
  - You start it with `--net-host` or `--net-client`, or from the ImGui "Multiplayer" panel.
  - The DS's MP timing (parent and child frames each VBlank) is not modelled. Whether it is good enough for trades and
    battles is not shown in this repository [I].
- **MB, WBT and WFS** compile over that WM, with no port edits [I: untested].
- **DWC and NitroWiFi:** `libntrwifi` and `libntrdwc` compile for PC. Their diffs fix pointer-to-int socket handles and
  add C for previously unmatched files.
  - I found no host socket bridge: CPS is the DS's own TCP/IP stack, sitting on WCM [I].
  - "Support all WiFi and multiplayer features" is a **goal** in the Platinum README, not a feature.
- **DSi NWM:** absent.

---

## 10. Platforms and macOS

**Targets today** [V]:

- `win64` (MSYS2 UCRT gcc)
- `linux` (gcc, Docker image `standalone/docker/Dockerfile`)
- `nx` (devkitA64, "working, but poor performance")
- `arm` (MWCC, the original DS library)

There is **no macOS target**.

**What nasm is for** [V]. pret's Meson setup puts the Metrowerks assembler in Meson's `nasm` slot: in
`pokeplatinum-pc/meson/cross.ini`, `nasm = mwasmarm.exe`, and `libraries/syscall/meson.build` passes `-proc arm4t
-16 -gccinc`. On PC targets nothing is assembled. Meson still requires *a* nasm binary because `project()` declares
the language. On a Mac, `brew install nasm` (native arm64) satisfies it, or the language can be made conditional. **It
is not a real blocker.**

**The CPU architecture is not a blocker** [V]:

- No intrinsics, inline asm, `rdtsc` or `cpuid` (grep).
- `SDK_AARCH64` code paths already exist for NX (`nitro.h:9-11,95-108`, `env.h:16`, `mi_wram.c:39`,
  `snd/common/channel.h:131`).
- Apple arm64 has a signed `char`, matching MWCC's `-char signed`. Linux aarch64 would not match.

### Concrete macOS / Apple Silicon blockers

| # | Blocker | Where | Severity |
| --- | --- | --- | --- |
| 1 | No `macos` value in `build_target`. No `SDK_BUILD_MACOS` defines. SDL2 and enet are found only for linux, win64 and nx; `glad.c` only for linux and win64; `native:` flags only for linux and win64 | `meson.options:5`, `meson.build:125-161`, `libraries/sim/meson.build:9-20,96-105`, `libraries/sim/src/meson.build:37-39` | Build |
| 2 | **`u32`/`s32` are `unsigned long`/`signed long` unless `SDK_BUILD_LINUX` or `SDK_BUILD_NX`.** On LP64 macOS that makes them 64-bit, which silently corrupts every struct | `include/nitro/types.h:27-45` | **Critical** |
| 3 | Requests an **OpenGL 4.2 core** context with no forward-compatible flag. macOS caps at 4.1 core, so context creation fails | `libraries/sim/src/sim_main.cpp:317-319` | Run |
| 4 | Shaders use `#version 420 core`; ImGui is initialised with `"#version 420"`. They need 410: the shaders use nothing beyond `layout(location)` inputs and uniforms | `libraries/sim/src/shaders/*.glsl:2` (4 files), `sim_gui.cpp:52` | Run |
| 5 | `glEnable(GL_DEBUG_OUTPUT)` and `glDebugMessageCallback`, plus `glDebugMessageInsert` on every `G3_Begin`. KHR_debug and GL 4.3 do not exist on macOS, so these are NULL function pointers and crash | `sim_main.cpp:395-396`, `g3_handler.cpp:417` | Run |
| 6 | OpenGL is deprecated on macOS (frozen at 4.1, emulated over Metal). There is no Metal, Vulkan/MoltenVK or SDL_GPU backend, and GL calls are spread over `sim_main.cpp`, `sim_g2.cpp`, `g3_draw.cpp` and `g3_handler.cpp` with no abstraction | renderer | Long-term |
| 7 | Every data path is relative to the current directory: `header.bin`, `save.bin`, `./sim_config.ini`, `firmware.bin`, the extracted ROM tree, FS paths. A Finder-launched `.app` starts in `/`. A chdir fix was reverted (`aa45e52`, "broke AppImage") | `sim_main.cpp:1474`, `card/backup.h:118-215`, `sim_config.c:14`, `nvram_instruction.c:17`, `nds-extract.cpp:175`, `fs_file.c:519` | Bundle |
| 8 | GL calls happen synchronously inside G3 and GX calls on whichever thread makes them. macOS needs the context current on that thread, and AppKit needs the window on the main thread | `g3_handler.cpp` (`glDepthFunc`/`glCullFace` in `G3SIM_PolygonAttr`), `g3_draw.cpp` | Risk |
| 9 | SDL2 and enet are found with `find_library` and headers included as `<SDL2/SDL.h>`, with no `dependency()`/pkg-config, so Homebrew's `/opt/homebrew/include` is not added | `libraries/sim/meson.build:4-19` | Build |
| 10 | `meson.build:1` (libntr) and the Platinum `project()` declare `nasm` | as above | Trivial |
| 11 | `SIM_debugBreak` has no macOS branch, and `nanotime` is only implemented for Win64; it falls back to `clock_gettime`, which is fine | `sim_assert.c:31-39`, `sim_main.cpp:77-80` | Minor |
| 12 | `(void *)cmdBlockPtr + 4` in C++ is a GNU extension; Apple clang may warn or reject [I: verify] | `sim_g3_CommandProcessor.cpp:590` | Minor |
| 13 | `(u16*)L"multiboot"` assumes a 16-bit `wchar_t` (also wrong on Linux) | `libraries/mb/src/mb_wm_base.c:718` | Minor |
| 14 | No HiDPI window flag (Retina renders at point resolution) | `sim_main.cpp:379-381` | Cosmetic |

Items 1-5, 7 and 9 are enough to get a first Apple Silicon build running from a terminal. Item 6 is the strategic one.

---

## 11. Debug and tooling [V]

- **Dear ImGui** overlay, toggled with the grave key (`sim_gui.cpp`). It shows the libntr git hash, render and frame
  times and fps. Panels: Config (layout, swap, fullscreen, frame cap, volume, internal resolution), Input,
  Multiplayer, the ImGui demo, and a game-specific "Application" panel through a weak `PrjMain` symbol. There is also a
  "Pause Game Logic" option.
- Per-layer enable flags exist: `s_SIM_DBG_BGenable`, `s_SIM_DBG_OAMenable`.
- **Tracy** zones are in the renderer, audio, flush and BG code (`-Dtracy_enable=true`, using the author's fork with
  NX support).
- Debug print goes to a file (`sim_debug.cpp`). `SIM_assert`/`SIM_debugBreak` call `__debugbreak` on Windows and
  `SIGTRAP` on Linux.
- **There is no VRAM, palette, OAM, texture, 3D-command or sound viewer.**

---

## 12. Generality, and what Black 2/White 2 needs

libntr is generic NitroSDK, not tied to Platinum, but its *simulator* covers only what Platinum exercised, and its
fixes have come from Platinum bug reports. Black 2/White 2 needs much more.

**Feature use in pokebw2.** Counts are `grep -rw` call sites in `pokebw2:src` and `lib`, mostly game code. These are
call sites, not runtime use.

| BW2 need | pokebw2 evidence | libntr status |
| --- | --- | --- |
| TwlSDK 5.x NTR/TWL hybrid | `lib/twl`: camera, DSP, mic, NWM, FATFS, SSP JPEG, photo sign in the LTD autoload | TWL merge is beta, built for PC in CI only for the standalone demo. No PC backend for camera, DSP, mic or NWM, and no SSP. **Practical route: run BW2 as an NTR DS on PC** (LTD code never loads) |
| SDK version | NitroSDK inside TwlSDK 5.x (`pokebw2:lib/nitro`, `lib/twl`) | libntr's NitroSDK is **4.2 (2008)**, TwlSDK 5.5. Replacing pokebw2's matching `lib/nitro` with libntr's would mean two divergent SDK copies, or losing the ROM match |
| NitroSystem (CW 2.0 build) | `pokebw2:lib/nnsys` (fnd, gfd, g2d, g3d, snd) | libntrsystem is the *Platinum-era* NNS decompilation. Its 64-bit loader and accessor patterns are reusable; its code is not BW2's |
| GFL (Game Freak Library) | `pokebw2:src/gfl` | Nothing. Heaps (64-bit growth beyond the 8 MB arena), ArcTool, overlay manager, HBlank system and `double3Ddisp` all need port work |
| Edge marking | `G3X_EdgeMarking` 37, `G3X_SetEdgeColorTable` 12 | **Not implemented** |
| Anti-alias, clear colour, alpha test | 33, 33, 26 | **Ignored** |
| Fog | 27 (+4 tables) | Implemented (approximate) |
| Shading / toon / highlight | `G3X_SetShading` 24 | **Not implemented** (all modes replace) |
| Display capture | `GX_SetCapture` in 7 files: `gfl/double3Ddisp.c`, `system/screentex.c`, `ov162/title.c`, `ov148/encounter_effect_capture.c`, `ov293/demo3d_capture.c`, `ov307`, `ov216` | **Not implemented**, so 3D on both screens, battle-intro captures and screen-to-texture are broken |
| VRAM display mode | `GX_DISPMODE_VRAM`/`MMEM` 14 | **Not implemented** |
| LCDC bank mapping | `GX_SetBankForLCDC` 24 | LCDC is a separate array; data does not follow banks remapped to BG or OBJ |
| Windows | `G2_SetWnd0Position` 21, `Wnd1` 10 | Text BGs only |
| Mosaic | `G2_SetBGMosaicSize` 5 | BG mosaic never applied |
| HBlank IRQ (GFL) | `OS_SetIrqFunction(OS_IE_H_BLANK, GFLAppIRQHBlank)` in `src/system/main.c:50`; `GX_HBlankIntr` 5 | One call per frame, so per-scanline effects are broken |
| Box test, clip and vector matrix reads | 5, 8, 5 | Box test via melonDS clipping; clip matrix result written; vector matrix [I] |
| `G3_MultMtx44` | 4 | **Dropped on PC** (still a FIFO register write) |
| Memory-file archives | `src/gfl/arc_tool.c:82,181` (`FS_CreateFileFromMemory`) | **Unsupported**: reads copy nothing |
| By-ID opens | NNS SndArc: `FS_ConvertPathToFileID` + `FS_OpenFileFast` (`lib/nnsys/src/snd/snd_arc.c:27-32`, `snd_arc_stream.c:970`) | **Unsupported in libntr FS**. libntrsystem must special-case it [I] |
| Overlays | `src/gfl/gf_overlay.c` (`FS_LoadOverlay`/`UnloadOverlay`/`LoadOverlayInfo`) | No PC concept; static linking; reload semantics lost |
| Threads | `OS_CreateThread` 2 in game, 16 in libraries | Host threads, no interrupt masking, shared register globals: race risk |
| IR (`lib/irc`) | Card SPI infrared chip | Not supported |
| DWC, NitroWiFi, DPW (GTS, Subway) | `pokebw2:lib/dwc`, `lib/nitrowifi`, `lib/dpw` | Compile-only in the sibling repos; no WFC replacement |
| 3D texture formats | **Black 2 ROM scan** (uncompressed TEX0 blocks: 2,544 blocks, 57,405 textures): PLTT16 77.3%, PLTT4 10.4%, A3I5 5.7%, A5I3 4.6%, PLTT256 2.0%, DIRECT 9, **COMP4x4 0** | All used formats are decoded. The missing 4x4 is likely irrelevant [V for uncompressed files; LZ-compressed files not scanned]. 10% are translucent formats, so the lack of translucency sorting shows |
| BIOS | `SVC_GetCRC16` 3, `SVC_CpuSet` 1 (library code: `os_deliverArg.c`, `ctrdg.c`, `nwm.c`), SHA-1/HMAC via TWL syscalls | Stubbed or absent (CRC returns 0, CpuSet is a no-op) |
| Sound effects | NNS SND capture (reverb) [I: check BW2 use] | Not implemented |

---

## 13. Extensibility, widescreen, and the single-screen forks

**Upstream hooks** [V]:

- Three screen layouts plus swap.
- An integer internal resolution for **3D only**.
- `SIM_GetSDLWindow`, `SIM_GetConfigPtr`, and the weak `PrjMain` ImGui panel.
- `SIM_NDS_SCREEN_WIDTH` is a fixed `256`, used 65 times in `libraries/sim/src`. The 2D layers are fixed 256×192
  buffers.
- There is no widescreen, no 2D high-resolution path, no render-time interpolation and no single-screen mode.

**evoluketion** (`single-screen` branch of the Platinum port) [V]:

- Pins libntr to `aa45e52` and carries `subprojects/packagefiles/libntr_patch/0001-single-screen-composite.patch`, a
  1,249-line diff (about 720 added lines). The wrap comment explains the pin: "the patch is authored against this
  specific tree".
- It touches `sim_main.cpp`, `g3_draw.cpp`, `g3_handler.cpp`, both G2 and G3 shaders, `sim_screenquads.c` and the
  config.
- Its log includes "Composite both DS engines into a single screen", "Fix translucent and perspective 3D rendering in
  the libntr patch" and "Run frame-timed sequences at the DS logic rate".

**elyro1** (`pc_single_screen_mod` branch plus the `elyro1/libntr` fork pinned at `c2de973`) [V via `gh api compare`]:

- The fork is 47 commits ahead and 16 behind upstream, with about 2,871 added lines across the sim, `card_backup.c`,
  `mi_dma_hblank.c`, `fx_mtx43.c` and `gx_load3d.c`.
- It adds:
  - a `TOP_ONLY` layout up to 21:9, with **game-cooperative widescreen**: `SIM_SetFieldWidescreen` returns a logical
    width of 256-448;
  - per-scene hacks: `SIM_SetFieldOverlayTiled`, `SIM_SetFieldDarknessMask`, `SIM_SetFieldMoveCutIn`,
    `SIM_SetScreenFadeWindows`, `SIM_SetFieldWideSpriteRange`;
  - virtual texture addressing;
  - **per-scanline HBlank and DMA**, and WIN0/WIN1 masks applied to sprites;
  - a frame pacer and profiler, configurable fast-forward, PC save slots;
  - many fixes for fog, distortion and depth.
- None of it is upstream.

**Lesson:** every serious mod so far has had to fork libntr's sim layer. Widescreen on top of a 256-px PPU model ends
up as a growing list of per-scene hacks.

---

## 14. Recommendation for pokebw2

**Do not adopt libntr as a dependency.**

- It would replace pokebw2's matching, version-correct `lib/nitro` and `lib/twl` with a different SDK version (4.2
  and 5.5).
- It brings unclear licensing: an MIT label over unlicensed decompilation code and GPLv3-derived melonDS code without
  attribution.
- It is one developer's fast-moving `main`, with no tags, no tests, issues disabled, and a no-AI contribution policy.
- Its sim layer lacks most of the 3D, 2D and FS features BW2 uses (section 12).
- Its design is a register- and VRAM-level hardware model, which is the "emulation layer" the user wants to avoid.

**Do not fork it wholesale either.** The fork would carry two SDKs, and both community forks show how such a layer
diverges.

**Build pokebw2's own platform layer on pokebw2's own SDK sources, modelled on libntr's proven patterns:**

1. A `SDK_PORT`-style build of `lib/nitro`, `lib/twl` and `lib/nnsys` from the same sources that match the ROM (the
   CLAUDE.md rule that both ROMs stay byte for byte still holds; port code goes behind `#ifdef`). Use a `uPtr` type,
   64-bit-safe PXI, and host-mapped `HW_*` symbols. Define `u32` as `uint32_t` everywhere.
2. Run as an **NTR DS**: `OS_IsRunOnTwl() == FALSE`, so the LTD autoload (camera, DSP, NWM, SSP) never loads.
3. Keep the SDK API, but give GX/G2/G3/SND **direct backend entry points** where the user's no-emulation rule matters:
   - **2D:** a GPU tile and OAM renderer fed from the SDK-level state (BG control, maps, characters, OAM). It has to
     support windows, mosaic, per-line state for GFL's HBlank tasks, capture, and VRAM display.
   - **3D:** keep the matrix and lighting semantics on the CPU, or move them to shaders. Rasterize with
     perspective-correct texturing, sorted translucency, edge marking, toon, fog, shadow polygons and polygon IDs.
   - Use SDL_GPU or Metal-capable rendering (SDL3 GPU, or bgfx/Vulkan through MoltenVK) instead of desktop GL 4.2.
4. Make the FS a host VFS: path, ID, Direct and memory archives, NARC-aware. Use a per-user data directory
   (`SDL_GetPrefPath`), verify the ROM hash, and write saves atomically.
5. Use host threads only where the game needs them, and give `OS_DisableInterrupts` a real meaning: a global mutex,
   or run all "IRQ" work on the game thread.
6. **What to reuse (licence permitting; pokebw2 is GPLv3):**
   - the melonDS-derived SPU mixer and ARM7 sequencer approach, which gives the most exact audio. Take it from melonDS
     directly, with attribution;
   - the enet-based WM tunnel design;
   - the fireflower extractor (BSD-3);
   - the ImGui and Tracy integration;
   - libntrsystem's list of NNS 64-bit resource fixups, as a checklist for `lib/nnsys`.

---

## Appendix: commands used (all read-only)

```text
git log / show / shortlog / grep / diff on the two clones
find | xargs wc -l, grep -rn, sed -n
gh api repos/cybervisi0n/{libntr,libntrsystem,libntrwifi,libntrdwc,...}, gh api users/ntrtwl/repos,
  gh api compare (elyro1 fork), gh release view (pokeplatinum), gh issue list (pokeplatinum)
gh api repos/melonDS-emu/melonDS/contents/src/{GPU2D_Soft,GPU3D,SPU}.cpp (and 0.8.3 GPU2D/SPU) for comparison
python3 -I scratchpad/scripts/tex0_formats.py orig/baserom_b2_us.nds   (our own ROM, TEX0 format tally)
grep -rw over pokebw2 src/ and lib/ for SDK feature call sites
```
