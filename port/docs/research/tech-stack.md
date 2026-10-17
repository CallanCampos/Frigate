# Native port of pokebw2: technology choices (research as of 2026-10-16)

Scope: the platform layer for a native (no emulation) port of the pokebw2 C decompilation, macOS on Apple Silicon
first (dev machine: M2, 16 GB, macOS 26.5.1, Xcode 26.6 with Apple clang 21.0.0, Homebrew LLVM 23.1.3), then Windows
and Linux. Goals: native, fast, mod-friendly (widescreen, higher internal resolution, randomizer, multiplayer).
libntr is analyzed elsewhere; it is mentioned here only where it bears on a choice.

Versions and dates below were read from the projects' GitHub releases/tags (via the GitHub API) on 2026-10-16 unless
another source is cited. "Inference" marks a conclusion that is mine, not a source's.

---

## 0. What the codebase itself demands (evidence from this repository)

These facts drive several choices below; they were checked with `grep` in this worktree.

| Fact | Evidence | Consequence |
| --- | --- | --- |
| License is GPLv3 | `LICENSE` | Every linked library must be GPLv3-compatible (section 8). |
| Zero-length arrays are common | 468 `[0];` members (`ivs[0]`, `moves[0]`, `zones[0]`, `langOffsets[0]`) | MSVC warns (C4200) but accepts; clang/GCC accept. |
| Anonymous structs/unions | ~108 bare `struct {`/`union {` members | C11-standard; MSVC accepts in `/std:c11`+. |
| MWCC-isms | 67 `#pragma thumb`, 5 `#pragma define_section`, MWCC `asm` functions in 59 files (mostly `lib/`), `__declspec(ctor)` in `lib/dsprot` | Port excludes hardware/anti-piracy code; the rest needs `-Wno-unknown-pragmas` and shims. |
| `long` used as 32-bit | 75 `long`, 37 `unsigned long`; `u32`/`s32` are `int` in `include/types.h` | `long` is 64-bit on macOS/Linux (LP64), 32-bit on Windows (LLP64): an audit item, and a reason to build on all three early. |
| Pointer/int casts and pointer fields at DS offsets | `docs/porting-notes.md` (SSL/WCM/SOC); pointer-to-`u32` casts in at least 14 `src` files | 64-bit layout hazards; there is no 32-bit target on macOS arm64. |
| Per-scanline (HBlank) effects | 19 `src` files, e.g. `src/system/wipe_sub.c`, `src/ov168/btlv_bg.c` | The 2D compositor must take per-scanline register state. |
| Display capture | `GX_SetCapture` in `src/gfl/double3Ddisp.c` (3D on both screens), `src/ov148/encounter_effect_capture.c`, `src/system/screentex.c`, `src/ov162/title.c`, `src/ov293/demo3d_capture.c` | Capture must work at upscaled resolution or the upscaled frames flicker (the problem melonDS hit). |
| 3D features in use | `G3X_EdgeMarking` 34 files, `G3X_AntiAlias` 35, `G3X_AlphaBlend` 35, `G3X_SetFog` 29, `G3X_SetShading` (toon/highlight) 25, `G3X_SetClearColor` 31, shadow polygons 1 | The GPU 3D path needs edge marking, fog, toon/highlight, translucency, and shadow-stencil semantics. |
| Geometry readback | `G3X_GetBoxTestResult` and `G3X_GetClipMtx` in `lib/nnsys/src/g3d/sbc.c`, `util.c`, `src/gfl/g3d_system.c`, `src/ov012/field_g3d_map.c` | The geometry engine (matrix stacks, box test) has to run on the CPU and answer synchronously; only rasterization goes to the GPU. |

libntr (MIT, meson) translates the DS 3D API to **OpenGL 4.2** (its shaders in `libraries/sim/src/shaders/*.glsl`
declare `#version 420 core`) on **SDL2**, with Dear ImGui, for linux/win64/nx (README). macOS caps OpenGL at 4.1, so
that path does not run on a Mac as-is. That is one reason this report recommends a Metal-native abstraction instead of
OpenGL.

---

## 1. Windowing, input, audio

### SDL3 (recommended)

- **Version and maturity:** SDL 3.2.0 was the first stable SDL3 (2025-01-21). 3.4.0 followed on 2026-01-01, and
  3.4.x bug-fix releases have shipped about monthly since: 3.4.10 (31 May), 3.4.12 (1 Jul), 3.4.14 (3 Aug), 3.4.16
  (2 Sep) and **3.4.18 (2026-10-02, latest)** ([releases](https://github.com/libsdl-org/SDL/releases)). The ABI is
  stable within 3.x. Zlib license.
- **3.4.0 highlights:** a GPU-backed 2D renderer (`SDL_CreateGPURenderer`), a pixel-art scale mode, planar audio, a
  no-copy audio stream put, a raw-stream hint, native PNG load/save, pinch gesture events, and wired Switch 2
  controllers. Later 3.4.x added GPU depth texture arrays, Metal packed 16-bit formats and multi-read GPU usages, plus
  many controller fixes ([releases](https://github.com/libsdl-org/SDL/releases)).
- **Gamepads:** `SDL_Gamepad` uses community mappings (`SDL_HINT_GAMECONTROLLERCONFIG`) and HIDAPI drivers for
  Xbox, PlayStation, Switch/Switch 2, 8BitDo, Steam Controller and more. It covers rumble, LEDs, gyro/accelerometer
  and gamepad touchpads. A DualSense touchpad could drive the DS touch screen (inference).
- **Touch, pen, mouse:** separate event families for finger (`SDL_EVENT_FINGER_*`, normalized per touch device),
  pen (`SDL_EVENT_PEN_*`, pressure and eraser) and mouse. Hints control the synthesized cross-events
  (`SDL_HINT_TOUCH_MOUSE_EVENTS`, `SDL_HINT_MOUSE_TOUCH_EVENTS`, `SDL_HINT_PEN_MOUSE_EVENTS`). On macOS, trackpads
  send mouse events by default. `SDL_HINT_TRACKPAD_IS_TOUCH_ONLY` (since 3.2.0) turns them into touch events
  (SDL_hints.h, 3.4.18).
- **High DPI:** create the window with `SDL_WINDOW_HIGH_PIXEL_DENSITY`, then size render targets from
  `SDL_GetWindowSizeInPixels` and UI from `SDL_GetWindowDisplayScale`. The pixel-size and display-scale changed
  events cover moves between Retina and non-Retina displays. Fullscreen-Spaces behavior is controlled by
  `SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES`.
- **Audio:** an `SDL_AudioStream` bound to a device (`SDL_OpenAudioDeviceStream`) converts format and sample rate,
  follows the default device across hot-plug, and accepts a buffer-size hint (`SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES`).
  The port's DS sound mixer (16 channels: PCM8/PCM16/IMA-ADPCM/PSG/noise at about 32.7 kHz) can push its native-rate
  output and let SDL resample.
- **Other useful pieces:** `SDL_dialog` (native file pickers for the user's ROM), `SDL_filesystem` (pref/base paths,
  globbing), `SDL_storage`, `SDL_tray`, `SDL_process`, and the main-callback app model (`SDL_HINT_MAIN_CALLBACK_RATE`).
- **macOS:** Cocoa and Metal-layer windows with `GCController`/IOKit controller support. SDL_GPU's Metal backend runs
  on Apple Silicon (section 2).
- **Companion libraries (zlib):** SDL_net 3.2.0 (2026-05-31), SDL_image 3.4.8 (2026-10-05), SDL_mixer 3.2.4
  (2026-06-03; not needed, since the game mixes its own audio).

### Alternatives

| Option | Version / date | Gaps for this port |
| --- | --- | --- |
| GLFW + miniaudio | GLFW 3.5.1 (2026-07-31; 3.5 skipped for a bad tag); miniaudio 0.11.25 (2026-03-03) | GLFW has no touch or pen events, no file dialogs, and a simpler gamepad layer (it reuses SDL's mapping database but lacks rumble, sensors and touchpads). Audio needs a second library. Two dependencies doing less than SDL3's one. |
| sokol_app + sokol_audio | rolling (latest changelog entry 2026-10-12), zlib | No gamepad API at all, and no multiple windows. It fits sokol_gfx demos, not a game port that needs controllers. |

**Verdict:** SDL3 3.4.x. It is the only option with first-class gamepads, touch, pen, multiple windows, audio
streams, dialogs and a GPU API in one zlib dependency. libntr uses its predecessor, SDL2 (it bundles
`imgui_impl_sdl2`).

### Dual-screen presentation

Render each DS screen into its own texture (256×192×N for 3D and upscaled composition) and treat presentation as a
pure layout transform. Layouts worth supporting, as in melonDS's layout options:

1. **Stacked (vertical, default):** top over bottom, with an optional gap (the physical DS has about 90 px of hinge
   at 1x).
2. **Side by side (horizontal):** suits 16:9 and 16:10 displays.
3. **Hybrid / emphasis:** one large screen plus a small one (e.g. the top screen large with the bottom as a corner
   inset). Good for widescreen and for touch-light scenes.
4. **Single screen with swap:** hotkey or gamepad button. Show a cursor or auto-swap when the game expects touch
   (inference).
5. **Separate windows:** SDL3 supports multiple windows, and one `SDL_GPUDevice` can claim several windows
   (`SDL_ClaimWindowForGPUDevice` per window), so the bottom screen can sit on a second monitor or an iPad via
   Sidecar.

Plus: rotation (0/90/180/270), integer-scale or aspect-fit, nearest or sharp-bilinear filtering for 2D, and per-screen
sizing ratios.

**Touch input:** keep one `layout → screen` affine transform per screen and invert it for pointer events. A left
mouse button, pen tip or finger inside the bottom-screen rectangle becomes `TP` down at
`(x, y) = clamp(inverse(p)) / scale`. Finger coordinates are normalized to the touch device, so map them through the
window first. Pen pressure can be ignored. Keep sub-pixel touch precision available, since higher-resolution touch is a
mod opportunity, but quantize to 0..255/0..191 for game logic (inference). In separate-window mode the bottom
window's events map directly.

---

## 2. Graphics API

### Candidates

| API | Version / date | macOS path | Shader story | Compute | License | Fit |
| --- | --- | --- | --- | --- | --- | --- |
| **SDL_GPU** (in SDL3) | SDL 3.2.0+ (Jan 2025); 3.4.18 (2026-10-02) | Native Metal (macOS 10.14+, Apple Silicon or Mac2 GPU) | Each backend needs its own bytecode: SPIR-V (Vulkan), DXIL/DXBC (D3D12), MSL or metallib (Metal). Compile offline with **SDL_shadercross** (HLSL or SPIR-V in; DXBC/DXIL/SPIR-V/MSL/HLSL out; zlib; no tagged releases, last commit 2026-09-03; needs SPIRV-Cross and DXC) or with Slang | Yes (compute pipelines, storage buffers/textures, indirect dispatch) | Zlib | **Best fit** |
| sokol_gfx | rolling (2026-10-12) | Metal | sokol-shdc: annotated GLSL to MSL/HLSL/GLSL/WGSL/SPIR-V | Yes on D3D11, Metal, WebGPU, Vulkan, GL 4.3. Since 2026-10-12, fragment shaders can write storage buffers | Zlib | Good C option; the Vulkan backend is still "experimental", D3D11 only on Windows (no D3D12), no gamepads in sokol_app |
| bgfx | no releases, active (commit 2026-10-17) | Metal | Own shaderc and a GLSL-like dialect | Yes | BSD-2 | Heavy (bx/bimg/GENie), C++ core with a C99 API; more than a DS renderer needs |
| WebGPU native | wgpu-native v29.0.1.1 (2026-06-23), Dawn v20261014.133826 (2026-10-15) | Metal | WGSL. webgpu.h is the shared stable header ([webgpu.org](https://webgpu.org)) | Yes | MIT/Apache-2.0 (wgpu), BSD-3 (Dawn) | Strong, plus a browser build as a bonus. Large dependency (Rust toolchain or Dawn's Chromium-style build), with extra validation overhead |
| Vulkan on Mac | MoltenVK v1.4.2 (2026-07-24, min macOS 12); KosmicKrisp (Mesa, LunarG) is **Vulkan 1.4 conformant** and ships in Vulkan SDK 1.4.357.0 (2026-07-28). It needs Apple Silicon and Metal 4 (macOS 26) ([Phoronix, Oct 2026](https://www.phoronix.com/news/KosmicKrisp-Vulkan-1.4-2026); [LunarG](https://lunarg.com/lunarg-releases-vulkan-sdk-1-4-357-0/); [LunarG Jan 2026](https://www.lunarg.com/?p=4048)) | Translation layer bundled in the .app | GLSL/HLSL/Slang to SPIR-V | Yes | Apache-2.0 / MIT | Viable, but verbose. On macOS it adds a layer that is still below native Metal (Phoronix) |
| OpenGL | macOS: deprecated since 10.14 (2018), capped at 4.1, no compute, no SSBOs | Apple's GL-on-Metal layer | GLSL | Not on macOS | n/a | Avoid as primary. It is the reason libntr's GL 4.2 path doesn't port as-is |
| Metal directly | Metal 4 (macOS 26) | Native | MSL | Yes | n/a | Best on Mac, but Windows/Linux would need a second and third backend (libultraship maintains D3D11, Metal and OpenGL backends by hand) |

The SDL_GPU details above come from the [SDL3 GPU wiki](https://wiki.libsdl.org/SDL3/CategoryGPU) and from
`SDL_gpu.h` 3.4.18.

**SDL_GPU caveats that matter here** (read from `SDL_gpu.h` 3.4.18):

- **Depth formats:** only D16 is universal. D24 and D32F are "either, not necessarily both" (Apple Silicon has no
  D24S8, but has D32F_S8). Query with `SDL_GPUTextureSupportsFormat`, or keep DS depth in an R32 color attachment.
- **Storage writes:** only compute can write storage buffers and textures (`COMPUTE_STORAGE_WRITE`). Graphics stages
  can only read them. A melonDS-style compute rasterizer is possible, but fragment-shader atomics/UAV tricks are
  not.
- **Targets:** up to 4 color targets plus depth-stencil per render pass. That is enough for color + polygon
  attributes + fog/edge flags + depth.
- **Profiling:** no timestamp queries, so no in-app GPU timing. Debug groups and labels exist
  (`SDL_PushGPUDebugGroup`, `SDL_InsertGPUDebugLabel`) and show up in Xcode's Metal debugger, RenderDoc and PIX.
- **No geometry or mesh shaders:** not needed.
- **Production use:** FNA3D (zlib), the XNA reimplementation behind many shipped PC/console ports, ships an SDL_GPU
  driver and requires SDL 3.2.0+ ([FNA3D README](https://github.com/FNA-XNA/FNA3D)). Dear ImGui ships
  `imgui_impl_sdlgpu3`.
- **Offscreen rendering:** works without a window; a window is only needed for presentation (header text). That
  enables headless GPU tests.

**Shader pipeline recommendation:** write shaders once in HLSL (or Slang v2026.19, 2026-09-29). At build time,
compile to SPIR-V, MSL and DXIL with the SDL_shadercross CLI (or `slangc`), and embed the blobs as C arrays. Nothing
compiles at runtime, so no DXC/SPIRV-Cross DLLs ship. Pin DXC (v1.9.2609, 2026-09-29) and SPIRV-Cross
(`vulkan-sdk-1.4.363.0` tag) in CI. On macOS, prefer precompiled `metallib` (`SDL_GPU_SHADERFORMAT_METALLIB`) over MSL
source, which avoids runtime Metal compiles and hitches.

### (a) DS 2D engine: CPU compositing vs GPU

The 2D engine is 4 BG layers per engine (text, affine, extended/bitmap), 128 OBJs, extended palettes, 2 windows plus
the OBJ window, color special effects (alpha/brightness), master brightness and mosaic. Per-scanline register changes
come from HBlank (used in 19 `src` files), and display capture writes back into VRAM.

- **CPU software compositor at 256×192 (phase 1, recommended):** 2 screens × 49,152 px × a few layers is well under
  1 ms per frame on an M2 (inference). It is scanline-exact (HBlank and window wipes come naturally), bit-exact for
  screenshot tests, and simple to debug. Output goes into two RGBA8 textures (plus a per-pixel "2D-above-3D/blend"
  attribute plane) uploaded each frame with `SDL_UploadToGPUTexture`.
- **GPU compositor (phase 2, for widescreen and hi-res 2D):** upload VRAM, palettes, OAM and a 192-entry per-scanline
  register table as storage buffers or textures, and evaluate the DS layer and priority logic per output pixel in a
  fragment or compute shader. This gives high-res affine BG and sprite rotation/scaling, layers wider than 256 px for
  widescreen, filtering, and composition with the upscaled 3D layer in one pass. melonDS is heading the same way: its
  developer began a GPU 2D renderer in 2025 to handle hi-res capture and hi-res rotation/scale, and says the long-term
  goal is the whole 2D renderer on GPU ([Notebookcheck](https://www.notebookcheck.net/High-accuracy-dual-screen-3D-texture-filtering-and-other-features-coming-to-DS-emulation.1165674.0.html)).
- **Plan:** define one data contract (VRAM/OAM/palette snapshot plus per-line registers) and keep the CPU compositor
  forever as the reference and fallback. Add the GPU one when widescreen work starts.
- **Port advantage:** we own the game code. Dual-screen 3D (`double3Ddisp.c`) can render both screens' 3D every frame
  at 60 fps, instead of alternating with capture at 30 fps.

### (b) DS 3D on the GPU at arbitrary resolution

**Architecture (recommended):** run the geometry engine at the API level on the CPU and only rasterize on the GPU.
The port's `G3_*` and `G3X_*` functions (and NNS G3D's display-list submission) feed a CPU geometry engine that
handles the matrix stacks, lighting, texture-coordinate generation, clipping, box test and clip-matrix readback.
Readback is required because BW2 reads `G3X_GetBoxTestResult` and `G3X_GetClipMtx`. The DS limit is 2,048
polygons and 6,144 vertices per frame, so this costs microseconds. Use float internally for smoother upscaled output,
the equivalent of melonDS's "hi-res coordinates". The engine emits a polygon list with DS attributes (polygon ID,
mode, alpha, fog flag, depth-equal flag, w- or z-buffer, texture params) to a GPU raster pass at `256·N × 192·N`, or
wider for widescreen.

Two GPU raster designs:

1. **Fragment pipeline (recommended first, DeSmuME-GL style):** draw opaque polygons, then translucent ones sorted on
   the CPU (DS auto-sort or manual order). Use MRT outputs for color, polygon attributes (ID, opaque flag, fog flag)
   and depth, and a stencil buffer for shadow volumes and the "same polygon ID doesn't overwrite translucent" rule.
   Full-screen post passes then apply edge marking (neighbor polygon-ID and depth comparison), fog (density table
   indexed by depth) and the final 2D/3D blend. SDL_GPU supports all of this. It is easy to debug in Xcode's GPU
   capture, and BW2's feature list (edge marking, fog, toon/highlight, alpha test/blend, clear color, a few shadow
   polygons) maps cleanly onto it. DS anti-aliasing (35 files) is edge coverage: at higher internal resolution,
   replace it with MSAA (SDL_GPU supports up to 8×) or a post-AA pass.
2. **Compute rasterizer (melonDS style):** more exact (DS interpolation, depth-equal tolerance, 1-pixel edge rules),
   and SDL_GPU's compute support makes it possible. It is substantially more work and harder to debug. Keep it as a
   later "accuracy mode" if fragment-pipeline artifacts appear.

**Reference only (GPL emulator code, not to be copied):**

- **melonDS compute renderer** (`src/GPU3D_Compute.cpp`, GL 4.3+, credited to "Generic" in melonDS 1.0, current
  release 1.1 from 2025-11-18):
  - The CPU splits each polygon into left/right edge spans (`SpanSetupY`) and per-scanline span indices, and groups
    polygons by texture state.
  - GPU passes: span interpolation (Z and W variants); binning (`BinCombined`, `ClearCoarseBinMask`,
    `CalculateWorkOffsets`, `SortWork`) into screen tiles of `min(8·TileScale, 32)` px; a depth-blend pass;
    `Rasterise` variants for untextured/decal/modulate/toon/highlight/shadow-mask; and a `FinalPass` with 8 variants
    covering edge marking × fog × anti-aliasing.
  - Upscaling multiplies vertex positions by `ScaleFactor` (or uses hi-res positions), sizes the target at
    `256·S × 192·S`, and reallocates and recompiles on scale change. Above about 4.5× the tile size doubles.
  - Its classic GL renderer (`GPU3D_OpenGL.cpp`, GL ≈3.1+) instead writes polygon ID, edge flag and fog flag into an
    attribute texture, encodes polygon IDs and shadow masks in stencil, runs translucents after opaques with a `MAX`
    alpha equation, and does fog in a final pass.
- **DeSmuME OpenGL 3.2 renderer** (`OGLRender_3_2.cpp`, GPL-2.0-or-later):
  - Polygon state reaches shaders through a UBO, TBO or texture fallback. The geometry shader writes `gl_FragDepth`
    itself (W-depth `1/w` scaled to 24 bits; Z quantized to 22 bits as a Dragon Quest IV fix).
  - MRT outputs: color, polygon ID (`polyID/63`, wireframe, opaque flag), fog attributes and back-facing.
  - `EdgeMark*` shaders compare a pixel's ID and depth with its 4 neighbors and color edges from `edgeColor[polyID/8]`.
    `Fog*` shaders index a density table from depth/offset/step. `ClearImage*` shaders write the rear-plane image,
    fog and depth.
  - Toon uses `toonColor[32]` indexed by vertex red. It has MSAA variants of edge and fog.

Both renderers are GPLv3-compatible ("or later" in both headers). Reusing them is a legal option, but the user chose
reference-only, matching the decomp's "write it ourselves" rule.

**Display capture:** implement `GX_SetCapture` targets as GPU textures at render scale, and resolve to 256×192 VRAM
only when the CPU actually reads it. This is the problem melonDS hit (upscaled capture had to be downscaled into VRAM,
causing flicker) and the reason for its new GPU 2D work.

**Verdict:** SDL_GPU. One dependency already in the stack gives Metal on macOS, Vulkan on Linux, D3D12 on Windows,
compute when needed, zlib licensing, and an ImGui backend. Fallback if SDL_GPU blocks a feature: sokol_gfx (closest in
spirit, C, single header), or wgpu-native if a browser build becomes a goal.

---

## 3. Build system and compilers

### CMake vs Meson

| | CMake 4.4.4 (2026-10-02) | Meson 1.12.1 (2026-09-22) |
| --- | --- | --- |
| Universal macOS binaries | One build: `CMAKE_OSX_ARCHITECTURES="arm64;x86_64"` | No native support. Two builds with cross files, then `lipo` ([meson#10780 discussion](https://discourse.gnome.org/t/build-glib-fat-library-x86-64-and-arm64-on-macos-apple-m1/11092)) |
| IDEs | Xcode, Visual Studio and Ninja generators; CMakePresets | Ninja plus a limited Xcode/VS backend |
| Dependencies | SDL3, SDL_shadercross, Tracy, GNS, PhysFS, wasmtime/WAMR, Luau all ship CMake builds. FetchContent and vcpkg | WrapDB covers some (and SDL3 can be built through CMake subprojects) |
| Precedent | FNA3D, libultraship (CMake) | libntr (meson) |
| Language | Clunkier | Cleaner |

**Recommendation: CMake (with presets) for the port.** Keep the ROM build exactly as it is (`configure.py` → ninja
with MWCC/wibo). Put the port in a separate tree (e.g. `port/` with its own `CMakeLists.txt`) that consumes the same
`src/`, `include/` and `lib/` sources plus a port-side NitroSDK replacement, with build dir `build-port/`. Never let
the port build write `build/` or touch the SHA1 check, and add a CI job that runs both. If the team adopts libntr
wholesale, Meson becomes reasonable (consume it as a subproject, or call its meson build from CMake via
`ExternalProject`).

### Compilers

- **macOS: Apple clang** (Xcode 26.6, clang 21.0.0) for builds. It matches the SDK, integrates with codesign, Xcode
  and Instruments, builds universal binaries in one invocation (`-arch arm64 -arch x86_64`), and supports ASan, TSan
  and UBSan. Use **Homebrew LLVM 23.1.3** for tooling (clang-format 23, clang-tidy, a newer `-Wshorten-64-to-32`
  sweep), not for the shipped binary: mixing its libc++ with Apple's for the C++ parts (ImGui) is avoidable pain.
- **x86_64 macOS is low priority:** macOS 26 Tahoe is the last macOS for Intel Macs, macOS 27 is Apple Silicon only,
  and full Rosetta ends after macOS 27 ([512pixels, Jun 2026](https://512pixels.net/2026/06/wwdc26-macos27-drops-intel-support/);
  [Thurrott](https://www.thurrott.com/?p=321965)). With CMake a universal release is cheap, so ship it while it is
  free, and test on arm64.
- **Windows: clang-cl as primary, MSVC cl as a CI check.** clang-cl gives the same diagnostics and extensions as
  macOS/Linux clang (zero-length arrays, `__attribute__`, statement expressions, `__builtin_*`) with the MSVC ABI, PDBs
  and Visual Studio debugging.
- **MSVC itself** handles C11/C17 (`/std:c11`, `/std:c17`; anonymous structs/unions are standard there; zero-length
  arrays are an accepted extension with warning C4200). Its C limits come from general knowledge, not sources fetched
  here: no VLAs, no GNU statement expressions, no `__attribute__`; C23 `typeof` only under `/std:clatest`, though
  `__typeof__` works in all modes ([MS docs](https://learn.microsoft.com/de-de/cpp/c-language/typeof-c?view=msvc-170);
  [C4200](https://learn.microsoft.com/is-is/cpp/error-messages/compiler-warnings/compiler-warning-levels-2-and-4-c4200?view=msvc-170);
  [C4201](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4201?view=msvc-160)).
  The decomp is "natural C" for MWCC, so it probably compiles under MSVC with a small shim header; keep that as a CI
  job so it stays true (inference).
- **MinGW-w64** (MSYS2 UCRT64/CLANG64, or llvm-mingw): useful for cross-compiling Windows builds from Linux CI and
  for contributors without Visual Studio. libntr uses MSYS2. GDB/LLDB-only debugging is the tradeoff.
- **Linux:** GCC 14/15 and clang both work. Build with both in CI, since GCC's `-fanalyzer` and warnings catch
  different things.
- **Warnings for the 64-bit port** (inference): `-Wpointer-to-int-cast -Wint-to-pointer-cast -Wshorten-64-to-32
  -Wcast-align -Wstrict-prototypes`, plus `-Wno-unknown-pragmas` for `#pragma thumb`, and `-fno-strict-aliasing` and
  `-fwrapv` at first. Decompiled code type-puns and relies on wrapping arithmetic; remove those flags only after
  UBSan is clean.

### Sanitizers

| Platform | ASan | UBSan | LSan (leaks) | TSan | Other |
| --- | --- | --- | --- | --- | --- |
| macOS arm64 (Apple clang / Xcode) | Yes | Yes | Not exposed by Xcode. Upstream docs say macOS leak detection is off by default (`detect_leaks=1`). Use `leaks --atExit` or Instruments Leaks | Yes | Guard Malloc (`libgmalloc`), Malloc Scribble. No Valgrind on Apple Silicon |
| Linux (clang/GCC) | Yes | Yes | Yes (on by default with ASan) | Yes | MSan (clang), Valgrind |
| Windows | MSVC `/fsanitize=address` and clang-cl ASan (Windows 8.1+) | clang-cl only (UBSan lists Windows as supported) | No | No | Application Verifier, PageHeap |

Sources: [Clang ASan](https://clang.llvm.org/docs/AddressSanitizer.html) and
[Clang UBSan](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html).

Practical setup: a `asan-ubsan` CMake preset on macOS for development, and a Linux CI job that runs deterministic
replays (section 7) under ASan+UBSan+LSan on every pull request. Decomp code tends to hide out-of-bounds reads
(e.g. through `[0]` arrays), uninitialized reads that happened to be benign on the DS, and signed overflow.
`-fsanitize=undefined` with `-fno-sanitize-recover` makes them visible.

---

## 4. Packaging and distribution

### Assets (legal precondition for everything below)

Ship code only. At first run, ask for the user's own ROM (SDL file dialog), verify it against `b2_us.sha1` or
`w2_us.sha1`, and extract or cache NitroFS into the pref path (`SDL_GetPrefPath` → `~/Library/Application Support/...`).
The sm64ex/libultraship ports follow the same model (they extract `.o2r`/`.otr` archives from the user's ROM).

### macOS

- **Bundle:** `PokemonBW2.app/Contents/{Info.plist, MacOS/pokebw2, Resources/(AppIcon.icns or Assets.car, LICENSE,
  licenses/), Frameworks/(only if anything is dynamic)}`. Link SDL3 statically to keep the bundle trivial. `Info.plist`
  needs `CFBundleIdentifier`, `CFBundleExecutable`, `LSMinimumSystemVersion` (macOS 13 or 14 is plenty for SDL_GPU on
  Apple Silicon; inference) and `NSHighResolutionCapable`.
- **Ad-hoc signing:** arm64 code must carry at least an ad-hoc signature to run (the linker adds one). Re-sign after
  assembling the bundle with `codesign --force --sign - App.app`, signing nested code inside-out. Locally built
  bundles then run with no prompts.
- **Unsigned or ad-hoc builds downloaded from GitHub on macOS 26** are quarantined. Since macOS 15 the old
  Control-click → Open bypass is gone. The user launches once, then goes to System Settings → Privacy & Security →
  **Open Anyway**, and confirms; the app is then saved as an exception
  ([Apple support 102445](https://support.apple.com/en-us/102445)). The alternative is `xattr -dr
  com.apple.quarantine App.app`.
- **Homebrew casks:** Homebrew 5.0 removed `--no-quarantine` and planned to disable casks that fail Gatekeeper from
  2026-09-01. Treat a Homebrew cask as requiring notarization (secondary sources:
  [CopyQ issue](https://goodfirstissue.org/hluk/CopyQ/issues/3498)).
- **Developer ID + notarization (smooth installs):** Apple Developer Program, about USD 99 a year, with no extra fee
  for notarization. Sign every Mach-O with `codesign --options runtime --timestamp -s "Developer ID Application: …"`,
  submit with `xcrun notarytool submit … --wait`, then `xcrun stapler staple`
  ([Apple DTS forum answer](https://developer.apple.com/forums/thread/746992); [flaviocopes](https://flaviocopes.com/macos-open-anyway/)).
  Hardened-runtime entitlements:
  - LuaJIT or wasmtime JIT needs `com.apple.security.cs.allow-jit`.
  - Loading unsigned native mod plugins needs `com.apple.security.cs.disable-library-validation`. That weakens the
    signature, so prefer sandboxed scripting.
- **DMG:** `hdiutil create -format UDZO` (or `create-dmg`), then sign and notarize the DMG too. Note one 2026 report
  of a notarized `.pkg` rejected on macOS 26.3: prefer a DMG or zip of the `.app` over `.pkg`.
- **Recommendation:** ad-hoc signed DMG/zip for nightlies, with a README explaining Open Anyway. Developer ID +
  notarization for tagged releases if the project accepts the USD 99 a year. GPLv3's "Installation Information" duty is
  met, because users can always build and ad-hoc sign their own build (inference).

### Windows

- **Formats:** a portable zip (exe + `LICENSE` + licenses) is the norm for game ports. An optional Inno Setup or WiX
  installer.
- **SmartScreen:** unsigned exes trigger it. Microsoft's managed signing service was renamed from Trusted Signing to
  **Artifact Signing** (Basic tier: 5,000 signatures a month) ([Azure](https://azure.microsoft.com/fr-fr/pricing/details/artifact-signing/)).
  Reputation still has to build up.
- **Runtime:** use the static CRT (`/MT`) or ship the VC++ runtime.

### Linux

- **AppImage:** build with appimagetool (continuous build, 2026-10-04) and the static type2 runtime, which removes
  the libfuse2 dependency. Plus a tarball.
- **Flatpak:** flatpak-builder 1.4.12 (2026-09-15) on the freedesktop runtime, for Flathub. The ROM comes through the
  file-chooser portal.
- **Steam Deck:** benefits from SDL3's controller support.

### CI (GitHub Actions, free for public repos)

Runners from the [GitHub docs](https://docs.github.com/en/actions/reference/runners/github-hosted-runners):

| Platform | Labels | Hardware |
| --- | --- | --- |
| macOS arm64 | `macos-26` (= `macos-latest` since June 2026), `macos-15` | M1, 3 vCPU, 7 GB |
| macOS Intel | `macos-26-intel`, `macos-15-intel` | x64 |
| Linux | `ubuntu-24.04`/`26.04`, plus `ubuntu-24.04-arm` | |
| Windows | `windows-2025`, `windows-2025-vs2026`, `windows-11-arm` | |

macOS 26 runners went GA on 2026-02-26 ([changelog](https://github.blog/changelog/2026-02-26-macos-26-is-now-generally-available-for-github-hosted-runners/)).
`macos-14` retires on 2026-11-02 ([changelog](https://github.blog/changelog/2026-10-01-github-actions-macos-14-runner-image-retirement)).

Caching: `actions/cache` (10 GB per repository by default) for ccache or sccache plus FetchContent dependency builds,
keyed on the compiler version and a lockfile of dependency SHAs. Build SDL3 from a pinned tag instead of relying on
runner packages. GPU on CI: macOS arm64 runners are VMs. GitHub's docs say nested virtualization is unsupported
because of Apple's Virtualization Framework (an earlier wording also excluded Metal Performance Shaders), and others
report Metal being missing or limited in macOS CI VMs
([CircleCI](https://support.circleci.com/hc/en-us/articles/360052160592-Tests-Fail-With-Error-There-is-no-available-Metal-device-on-this-system)).
Do not make pixel tests depend on CI Metal (section 7).

---

## 5. Mod infrastructure

- **Dear ImGui v1.93.0 (2026-10-14, MIT):** enhancement menus, debug viewers (VRAM, OAM, palettes, 3D polygon list,
  NARC browser) and a cvar editor. The **docking branch** adds dockable windows and multi-viewport, which gives
  separate OS windows for debug tools and pairs well with separate-screen mode. Backends: `imgui_impl_sdl3` and
  `imgui_impl_sdlgpu3`, with Metal, Metal 4, Vulkan and WGPU also available. ImGui is C++. Keep the game in C and
  write the frontend/debug layer in C++, or use the generated C bindings (dear_bindings/cimgui). libntr and
  libultraship both use ImGui.
- **Scripting:**

  | Engine | Version / date | License | Notes |
  | --- | --- | --- | --- |
  | Lua (PUC) | 5.4.8 (2025-05-21), 5.5.0 (2025-12-13), 5.5.1 (2026-07-23) | MIT | Tiny C; easiest to embed and to sandbox at the API level (no `io`/`os`/`load` of binary chunks; instruction-count hook for runaway scripts). sm64coopdx mods use Lua 5.3.5. |
  | LuaJIT | rolling v2.1 (commit 2026-10-14) | MIT | Fast, Lua 5.1 semantics. JIT on hardened-runtime macOS needs `allow-jit`. Small maintainer bus factor. |
  | Luau | 0.743 (2026-10-16) | MIT | Built-in sandboxing, gradual types, native codegen (x64/arm64). C++ build. Best for **untrusted, shared** mods (e.g. multiplayer mods downloaded from peers). |
  | WAMR | 2.4.5 (2026-06-29) | Apache-2.0 + LLVM exception | C; interpreter, AOT or JIT; strong sandbox; mods in any language. |
  | wasmtime | v49.0.3 (2026-10-14) | Apache-2.0 + LLVM exception | Best WASM engine, but a Rust build and a large binary. JIT needs `allow-jit` on macOS. |
  | wasm3 | v0.9.0 (2026-08-24), v0.9.1-beta.1 (2026-09-10) | MIT | Interpreter-only, tiny; development resumed in 2026 after years of minimal maintenance. |
  | Native plugins (`dlopen`) | n/a | n/a | Maximum power, no sandbox, ABI churn. On macOS they need library-validation disabled or the same signing team. Not recommended for distributed mods. |

  **Recommendation:** start with PUC Lua 5.4.8 (stable; 5.5 is under a year old) with a curated API and an instruction
  hook. Revisit Luau if mods will be exchanged automatically in multiplayer. WASM (WAMR) is the long-term "any language,
  safe" option.

- **Data mods, randomizer, VFS:** the game reads data through NitroSDK `FS_*` and NARC archives. Implement `FS_*` in
  the port on top of a layered VFS: base layer = NitroFS extracted from the user's ROM; overlay layers = mod
  directories and zips, in a user-ordered priority list. **PhysicsFS** (zlib, 3.2.0 from 2022-09-30, still maintained
  with commits in Sept 2026) gives this search-path model with zip/7z support. Or write a small custom layer over
  **miniz 3.1.2** (2026-07-01, MIT) if per-file NARC-member overrides are wanted, which is the natural randomizer
  granularity (inference). A randomizer then becomes a tool that writes an override pack, so the game binary stays
  untouched. libultraship's `FolderArchive`/`O2rArchive` (zip) design is a working precedent.
- **Config and cvars:** a single cvar registry (name, type, default, flags such as `ARCHIVE` and `NET_SYNC`) that
  ImGui, Lua and the command line all read. Persist it as **TOML** with tomlc17 (R261003, 2026-10-03, MIT, TOML 1.1),
  or JSON with yyjson 0.13.0 (2026-09-08, MIT) if machine-written formats are preferred. libultraship's cvars plus
  ImGui menus are the proven pattern.
- **Hot reload:** watch mod directories with **dmon** (single-header C, BSD-2, FSEvents/inotify/ReadDirectoryChangesW)
  and reload Lua scripts, textures and override files at a frame boundary. Do not attempt hot-reloading game C code.

---

## 6. Networking

The game's multiplayer sits on DS wireless (WM/MB, local) and on DWC/NitroWiFi for Internet play, which is being
decompiled in `lib/nitrowifi` and `lib/dwc`. Options, layered:

1. **Emulate local wireless over UDP:** implement the WM API on top of UDP datagrams (LAN broadcast discovery, then
   peer-to-peer). Trades, battles and union-room play then work unchanged, the way melonDS does LAN multiplayer.
2. **Real Internet DWC:** the decompiled sockets path can talk to community Nintendo WFC replacement servers through
   host sockets (`docs/porting-notes.md` already lists the SOC/SOCL handle questions).
3. **New co-op features** (sm64coopdx-style): a custom protocol.

| Library | Version / date | License | Fit |
| --- | --- | --- | --- |
| **ENet** | 1.3.18 (tag; repo active, commit 2026-06-23) | MIT | Reliable/unreliable channels over UDP, tiny C. Ideal for WM emulation and co-op. No NAT traversal. |
| **GameNetworkingSockets** | v1.6.1 (2026-10-03) | BSD-3 | Encrypted connections, reliable messages, P2P with ICE. 1.6.x falls back to a native ICE client when WebRTC isn't built. C++, pulls protobuf plus OpenSSL or libsodium/bcrypt. Steam Datagram Relay needs Steamworks (not GPL-compatible). |
| SDL_net 3 | 3.2.0 (2026-05-31) | Zlib | Rewritten, async, IPv4/IPv6 stream and datagram sockets. A thin socket layer, not a game protocol. |
| **libjuice** | v1.7.4 (2026-09-26) | MPL-2.0 | Small C ICE/STUN/TURN for NAT traversal. Pair it with ENet over the ICE-established UDP path, or use its own datagrams. |

**sm64coopdx** (v1.5.1, 2026-06-01) is a reference: its own UDP socket layer (`src/pc/network/socket*.c`) for
direct connections, and **coopnet** for lobbies and NAT traversal. coopnet is "a signaling server and library for ICE
using libjuice", AGPL-3.0. Mods are Lua 5.3.5.

**Recommendation:** ENet for the transport. libjuice for NAT traversal, with a small signaling/lobby server (our own,
or coopnet if AGPL is acceptable for the server) and a TURN relay (coturn) as fallback. GNS is the upgrade path if
encryption or relays become requirements. Deterministic lockstep isn't needed: the game's own link protocols are
turn-based and tolerant of latency.

---

## 7. Profiling and testing

- **Tracy v0.14.1** (2026-08-22, BSD-3): C API (`TracyC.h`), frame marks, zones, plots, memory tracking, lock
  contention. GPU zones exist for Metal (`TracyMetal.hmm`), Vulkan, D3D11/12, OpenGL and WebGPU, but SDL_GPU doesn't
  expose native command buffers or timestamp queries. Use Tracy for the CPU (game frame, geometry engine, 2D
  compositor, mixer) and native tools for the GPU.
- **Instruments / xctrace** (Xcode 26.6): Time Profiler, Allocations, Leaks, Metal System Trace, plus the Xcode Metal
  debugger (frame capture, shader debugging; SDL_GPU debug labels show up there). Scriptable with
  `xcrun xctrace record`. Use RenderDoc on Linux/Windows (Vulkan/D3D12) and PIX on Windows.
- **Deterministic replay** (design, inference): make the port's frame step a pure function of (state, input). Record
  pad bits, touch state, RTC start time, the microphone-silent flag and the `OS_GetTick` base, and virtualize the
  clocks the game reads (RTC, ticks) so its RNG seeding repeats. Replays become `.bw2rec` files and drive:
  - regression runs under ASan/UBSan on Linux;
  - per-frame framebuffer hashes;
  - netplay desync debugging.
- **Headless screenshot tests:** a `--headless --replay X --frames N --dump-hashes` mode using the **CPU reference
  renderers** (2D compositor plus a software 3D rasterizer at 1×). These are bit-exact and need no GPU, so they run on
  every runner.
  - **GPU renderer tests** run on Linux with Mesa **lavapipe** (`VK_DRIVER_FILES=/usr/share/vulkan/icd.d/lvp_icd.json`).
    SDL_GPU can render with no window. Compare 1× output to the reference with a small tolerance.
  - **Real Metal tests** run on a self-hosted Mac, if ever.
  - **Golden images:** record the same input movie in an emulator running the real ROM (e.g. BizHawk's melonDS core,
    which supports movies and Lua screenshots) to produce goldens. That validates the port's renderers against
    hardware behavior without linking any emulator code.
- **Frame pacing:** the DS runs at about 59.83 Hz. Either run logic at 60.0 Hz and stretch audio by about 0.3%, or
  pace to audio with dynamic rate control. Present with `SDL_GPU_PRESENTMODE_VSYNC` (or MAILBOX for low latency).

---

## 8. Licensing compatibility with GPLv3

| Component | License | GPLv3-compatible? |
| --- | --- | --- |
| SDL3, SDL_net, SDL_image, SDL_shadercross | Zlib | Yes |
| SPIRV-Cross, DXC (build-time only), Slang | Apache-2.0 / LLVM/NCSA; Slang Apache-2.0 + LLVM exception | Yes (Apache-2.0 is compatible with GPLv3, not GPLv2) |
| sokol | Zlib | Yes |
| bgfx | BSD-2 | Yes |
| wgpu-native / Dawn | MIT/Apache-2.0 / BSD-3 | Yes |
| MoltenVK / KosmicKrisp (Mesa) | Apache-2.0 / MIT | Yes |
| GLFW / miniaudio | Zlib / public domain or MIT-0 | Yes |
| Dear ImGui (+ docking), cimgui | MIT | Yes |
| Tracy | BSD-3 | Yes |
| Lua, LuaJIT, Luau, wasm3 | MIT | Yes |
| WAMR, wasmtime | Apache-2.0 + LLVM exception | Yes |
| ENet | MIT | Yes |
| GameNetworkingSockets (+ protobuf, OpenSSL 3) | BSD-3 (BSD-3, Apache-2.0) | Yes |
| libjuice | MPL-2.0 | Yes (MPL-2.0 §3.3 secondary-license compatibility, unless a file is marked "Incompatible With Secondary Licenses") |
| coopnet | AGPL-3.0 | Combinable with GPLv3 (GPLv3 §13). The network-use clause then applies to the combination; run it as a separate server program |
| PhysicsFS, miniz, tomlc17, yyjson, dmon | Zlib, MIT, MIT, MIT, BSD-2 | Yes |
| libntr, libultraship | MIT | Yes |
| melonDS / DeSmuME | GPL-3.0-or-later / GPL-2.0-or-later | Yes (both "or later"), but reference-only by project choice |
| Steamworks SDK (Steam Datagram Relay), Discord Game SDK | Proprietary | **No**: keep out of the GPL binary |
| Apple/Microsoft system frameworks | System-library exception | Yes |

Attribution: ship a `licenses/` folder with every bundled license (MIT, BSD and Zlib all require the notice in binary
distributions).

---

## 9. Recommended stack (one line each)

- **Windowing/input/audio:** SDL 3.4.x (3.4.18 today). One zlib dependency with gamepads, touch, pen, multi-window,
  audio streams, dialogs and macOS-native Metal.
- **Graphics:** SDL_GPU. Metal/Vulkan/D3D12 behind one API with compute, already in SDL3, and an ImGui backend exists.
- **Shaders:** HLSL (or Slang) compiled offline by SDL_shadercross into SPIR-V/MSL(metallib)/DXIL blobs embedded in
  the binary. No runtime compiler DLLs, no first-frame hitches.
- **2D renderer:** CPU scanline compositor at 256×192 now (exact, HBlank-safe, test oracle). GPU shader compositor on
  the same data contract later, for widescreen and hi-res 2D.
- **3D renderer:** CPU geometry engine at the G3 API level (box test and clip-matrix readback work) feeding a
  fragment-pipeline GPU rasterizer with MRT attributes and post passes for edge marking, fog and AA. A compute
  rasterizer is an optional later accuracy mode.
- **Build:** CMake 4.4 with presets and Ninja, in a separate port tree. The configure.py/MWCC ROM build stays
  untouched and both run in CI.
- **Compilers:** Apple clang (Xcode 26.x) on macOS, clang-cl primary plus MSVC cl check on Windows, GCC and clang on
  Linux. Homebrew LLVM 23 for clang-format/tidy only.
- **Sanitizers:** ASan+UBSan preset on macOS for development. ASan+UBSan+LSan replay runs on Linux CI.
- **macOS packaging:** static-linked `.app` in a DMG. Ad-hoc signed nightlies (Open Anyway), Developer ID +
  notarytool + stapler for releases.
- **Windows/Linux packaging:** portable zip (with optional Artifact Signing); AppImage (static runtime) plus Flathub
  later.
- **CI:** GitHub Actions `macos-26`, `ubuntu-24.04` (+ `-arm`), `windows-2025`, with ccache and pinned dependency SHAs.
  No CI pixel tests on Metal.
- **Debug and mod UI:** Dear ImGui 1.93 (docking branch) with `imgui_impl_sdl3` + `imgui_impl_sdlgpu3`.
- **Scripting:** Lua 5.4.8 sandboxed with an instruction hook. Revisit Luau or WAMR when mods are shared over the
  network.
- **Assets/VFS:** port `FS_*` onto a layered VFS (ROM-extracted NitroFS base plus ordered mod dirs and zips via
  PhysicsFS or miniz). The randomizer emits override packs.
- **Config:** one cvar registry persisted as TOML via tomlc17, edited from ImGui, Lua and the CLI. dmon for hot
  reload.
- **Networking:** ENet transport, WM emulated over UDP for LAN, libjuice ICE plus a small signaling server and coturn
  relay for the Internet. GNS if encryption or relays become required.
- **Profiling:** Tracy 0.14 (CPU) plus Instruments/Xcode Metal debugger (GPU); RenderDoc/PIX elsewhere.
- **Testing:** deterministic input replays, headless CPU-reference framebuffer hashes on every runner, lavapipe GPU
  tests on Linux, emulator-recorded goldens.

---

## 10. Main risks

1. **64-bit data-layout assumptions** (highest).
   - Structs with pointers at recovered DS offsets (`docs/porting-notes.md`), pointer-to-`u32` casts, and `long` as
     32-bit (112 uses).
   - File formats read by casting raw NARC/save bytes onto structs that contain pointers or `long`.
   - macOS arm64 has no 32-bit mode, so these must be fixed in source. Use `_Static_assert` on the sizes of serialized
     structs and run with sanitizers early.
2. **Fidelity of the GPU 3D path.** DS-specific rules have no direct GPU equivalents: 1-pixel edge marking,
   depth-equal tolerance, translucent same-ID rule, shadow polygons, W-buffer precision, the 12-bit clip-space
   quirks and the DS AA. They need test-driven work against emulator goldens. Upscaling also changes how edge marking
   and AA look.
3. **Display capture and dual-screen 3D at hi-res.** These are the cause of melonDS's upscaled-capture flicker.
   They need a port-specific design (GPU-side capture textures; native dual-screen 3D).
4. **Widescreen is a game-code change, not a renderer flag.** The 3D can widen through projection, but 2D layers and
   sprites are laid out for 256 px, culled at screen edges, and positioned by game logic. It needs per-scene patches
   behind `#ifdef`/cvars that must not disturb the matching ROM build.
5. **SDL_GPU constraints.** No timestamp queries, compute-only storage writes, depth-format variability, and
   SDL_shadercross having no tagged releases (pin a commit). Mitigation: the renderer sits behind a small internal
   interface; sokol_gfx or wgpu-native are fallbacks.
6. **Two builds of one source tree.** Port-only changes (shims, `#ifdef PLATFORM_PC`, bug fixes) must never change
   MWCC output. CI has to build the ROM and check both SHA1s on every port pull request, and the project's style and
   `check_style.py` rules apply to port code inside `src/`.
7. **Distribution friction on macOS.** Without Developer ID and notarization, every download needs the Open Anyway
   dance, and Homebrew casks are effectively closed. Budget USD 99 a year, or accept the friction.
8. **Licensing hygiene.** Keep proprietary SDKs (Steamworks relay, Discord) out. Treat AGPL coopnet as a separate
   server. Never ship ROM-derived assets.
9. **Upstream churn.** Dear ImGui (docking branch), SDL 3.4.x point releases and wasm/JIT engines move quickly. Pin
   versions, update deliberately, and keep JIT optional because of hardened-runtime entitlements.
10. **Scope pull of compute/accuracy work.** The melonDS-style compute renderer is a large project. Ship the fragment
    pipeline first and measure.

---

## Sources

- SDL releases: https://github.com/libsdl-org/SDL/releases ; SDL GPU wiki: https://wiki.libsdl.org/SDL3/CategoryGPU ;
  SDL_shadercross: https://github.com/libsdl-org/SDL_shadercross ; SDL_shadercross intro:
  https://moonside.games/posts/introducing-sdl-shadercross/ ; SDL_net 3 release:
  https://github.com/libsdl-org/SDL_net/releases ; Debian SDL3 changelog:
  https://tracker.debian.org/media/packages/libs/libsdl3/changelog-3.4.16ds-1
- FNA3D: https://github.com/FNA-XNA/FNA3D
- sokol changelog: https://github.com/floooh/sokol/blob/master/CHANGELOG.md ; bgfx: https://github.com/bkaradzic/bgfx ;
  wgpu-native: https://github.com/gfx-rs/wgpu-native/releases ; webgpu.org: https://webgpu.org
- KosmicKrisp: https://www.phoronix.com/news/KosmicKrisp-Vulkan-1.4-2026 ; https://www.lunarg.com/?p=4048 ;
  https://lunarg.com/lunarg-releases-vulkan-sdk-1-4-357-0/ ;
  https://www.lunarg.com/lunarg-achieves-vulkan-1-3-conformance-with-kosmickrisp-on-apple-silicon/ ;
  MoltenVK: https://github.com/KhronosGroup/MoltenVK/releases
- OpenGL on macOS: https://www.phoronix.com/news/Apple-Deprecates-OpenGL-OpenCL ;
  https://docs.unity3d.com/ru/2019.4/Manual/OpenGLCoreDetails.html
- melonDS: https://github.com/melonDS-emu/melonDS/blob/master/src/GPU3D_Compute.cpp ;
  https://github.com/melonDS-emu/melonDS/blob/master/src/GPU3D_OpenGL.cpp ;
  https://www.notebookcheck.net/High-accuracy-dual-screen-3D-texture-filtering-and-other-features-coming-to-DS-emulation.1165674.0.html ;
  https://redlib.hackliberty.org/r/emulation/comments/1gw3jo9/melonds_10_rc_finally_here_it_is
- DeSmuME: https://github.com/TASEmulators/desmume/blob/master/desmume/src/OGLRender_3_2.cpp
- libntr: https://github.com/cybervisi0n/libntr ; libultraship: https://github.com/Kenix3/libultraship
- Meson universal binaries: https://discourse.gnome.org/t/build-glib-fat-library-x86-64-and-arm64-on-macos-apple-m1/11092 ;
  https://gitlab.freedesktop.org/gstreamer/meson-ports/libffi/-/issues/11
- MSVC: https://learn.microsoft.com/de-de/cpp/c-language/typeof-c?view=msvc-170 ;
  https://learn.microsoft.com/is-is/cpp/error-messages/compiler-warnings/compiler-warning-levels-2-and-4-c4200?view=msvc-170 ;
  https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4201?view=msvc-160 ;
  https://devblogs.microsoft.com/cppblog/c23-support-in-msvc-build-tools-14-51/
- Sanitizers: https://clang.llvm.org/docs/AddressSanitizer.html ; https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
- Intel Mac sunset: https://512pixels.net/2026/06/wwdc26-macos27-drops-intel-support/ ; https://www.thurrott.com/?p=321965 ;
  https://www.techrepublic.com/article/news-apple-macos-27-drops-intel-mac-support-rosetta/
- Gatekeeper/notarization: https://support.apple.com/en-us/102445 ;
  https://macos-tahoe.com/blog/app-is-damaged-cant-be-opened-macos-tahoe-gatekeeper-quarantine-fix-2026/ ;
  https://developer.apple.com/forums/thread/746992 ; https://flaviocopes.com/macos-open-anyway/ ;
  https://developer.apple.com/forums/thread/839168 ; https://goodfirstissue.org/hluk/CopyQ/issues/3498 ;
  https://feedbagel.com/post/homebrew-to-remove-no-quarantine-flag-support-for-macos-security-compliance
- Windows signing: https://azure.microsoft.com/fr-fr/pricing/details/artifact-signing/ ;
  https://codenote.net/en/posts/windows-desktop-app-code-signing-distribution-japan-2026/
- AppImage static runtime: https://www.electron.build/docs/appimage ;
  https://forge.fedoraproject.org/atomic-desktops/tracker/issues/50
- GitHub runners: https://docs.github.com/en/actions/reference/runners/github-hosted-runners ;
  https://github.blog/changelog/2026-02-26-macos-26-is-now-generally-available-for-github-hosted-runners/ ;
  https://github.blog/changelog/2026-10-01-github-actions-macos-14-runner-image-retirement ;
  https://github.blog/changelog/2025-08-07-arm64-hosted-runners-for-public-repositories-are-now-generally-available/
- macOS CI Metal limits: https://support.circleci.com/hc/en-us/articles/360052160592-Tests-Fail-With-Error-There-is-no-available-Metal-device-on-this-system ;
  https://eclecticlight.co/2023/10/26/how-good-is-gpu-access-for-apple-silicon-virtual-machines/
- Headless Vulkan/lavapipe: https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Tooling/02_cicd.html ;
  https://upd.dev/libretro/RetroArch/commit/f1270e8
- sm64coopdx: https://github.com/coop-deluxe/sm64coopdx ; coopnet: https://github.com/coop-deluxe/coopnet
- Library versions (GitHub releases/tags, read 2026-10-16): ocornut/imgui, wolfpld/tracy, lua/lua, LuaJIT/LuaJIT,
  luau-lang/luau, bytecodealliance/wasmtime, bytecodealliance/wasm-micro-runtime, wasm3/wasm3, lsalzman/enet,
  ValveSoftware/GameNetworkingSockets, paullouisageneau/libjuice, icculus/physfs, richgel999/miniz, cktan/tomlc17,
  ibireme/yyjson, septag/dmon, Kitware/CMake, mesonbuild/meson, ninja-build/ninja, glfw/glfw, mackron/miniaudio,
  google/dawn, shader-slang/slang, microsoft/DirectXShaderCompiler, KhronosGroup/SPIRV-Cross, AppImage/appimagetool,
  flatpak/flatpak-builder.
