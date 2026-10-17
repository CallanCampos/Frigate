# Decomp-based native ports: the landscape, and an architecture for a Black 2/White 2 port

As of 2026-10-16. Repositories were shallow-cloned read-only into `research/landscape/` and inspected at these
commits: libultraship `759b134` (2026-09-02), Shipwright `dd702b06` (2026-10-15), 2ship2harkinian `f1dda4c`
(2026-10-14), Starship `3322643`, SpaghettiKart `5073d2e`, Ghostship `40eef13`, PaperBoat `d98b4b6`, Torch `72960ca`,
sm64coopdx `8cd6e59` (2026-05-31), N64Recomp `ffb39cd`, N64ModernRuntime `cdf5abb`, Zelda64Recomp `b65c482`,
SAT-R/sa2 `393357f7`. Others (Lighthouse, Kurausukun's pokeemerald `pc_port`, Project Picori, Gen1Recomp,
devilutionX, OpenRCT2, RSDKv5, the Perfect Dark port, OpenGOAL, UnleashedRecomp) were read through `gh api`.
The DS-specific precedent (cybervisi0n's Platinum port and libntr) is covered by another report and only referenced
here.

Status labels used below: **[read]** means I read the code or docs at the commit above; **[doc]** means the claim
comes from the project's README or docs without reading the code; **[inferred]** is my reading or recommendation.

---

## 1. Harbour Masters and libultraship (LUS)

### 1.1 The split between game code and LUS [read]

LUS calls itself "compile-time WINE" for the N64 (`docs/PORTING.md`). It is a library that is header-compatible
with the N64 SDK (libultra), so the decompiled game compiles against `#include <libultraship/libultraship.h>` instead
of `ultra64.h`. It has three source trees:

| LUS directory | Role |
| --- | --- |
| `src/libultraship/` | The N64-specific part: libultra reimplementation (`libultra/os*.cpp`, `os_vi`, `os_pi`, `os_eeprom`…) and **bridges** (`bridge/audiobridge`, `gfxbridge`, `controllerbridge`, `resourcebridge`, `consolevariablebridge`, `eventsbridge`, `scriptingbridge`, `windowbridge`, `crashhandlerbridge`): C-linkage entry points the game calls. |
| `src/ship/` | Platform-agnostic engine: `core/` (a `Context` holding `Component`s, `Tickable`s, `Action`s), `resource/` (resource manager and archives), `audio/` (SDL, CoreAudio, WASAPI, Null players), `controller/` (ControlDeck, mappings for SDL/keyboard/mouse), `config/` (Config, ConsoleVariable), `window/gui/` (ImGui menus, console, stats, event debugger), `events/`, `scripting/` (TCC script loader, native library loader), `security/` (Keystore for signed mods). |
| `src/fast/` | The Fast3D renderer: a display-list interpreter (`interpreter.cpp`, `f3dex.h`, `f3dex2.h`, `ucodehandlers.h`) with backends `gfx_opengl`, `gfx_direct3d11`, `gfx_metal` and window backends `gfx_sdl`, `gfx_dxgi`, plus a `GfxDebugger`. |

It is MIT-licensed, built with CMake (vcpkg on Windows, Homebrew deps on macOS), runs on Windows, macOS, Linux, iOS,
Android and OpenBSD, and uses SDL3, Dear ImGui, StormLib (MPQ), libzip, nlohmann-json, spdlog, stb, tinyxml2,
libgfxd, libtcc and Monocypher (README). It uses **epoch semantic versioning** (`epoch.major.minor.patch`): a long
refactor moves `main` to a new epoch while the previous epoch stays maintained for ports (`docs/VERSIONING.md`).

Each game is a separate repository that adds LUS (and usually Torch) as git submodules. The game repository's layout
is consistent across the family:

```
Shipwright/                      2ship2harkinian/              Starship/ SpaghettiKart/ Ghostship/ PaperBoat/
├── libultraship/  (submodule)   ├── libultraship/ (submodule) ├── libultraship/ (submodule)
├── torch/         (submodule)   ├── ZAPDTR/, OTRExporter/     ├── torch/ (submodule, not Starship)
├── soh/                         ├── mm/                       ├── src/            decomp C, edited in place
│   ├── src/        decomp C     │   ├── src/   decomp C       │   └── port/       port-only C/C++
│   ├── include/                 │   ├── include/              ├── include/
│   ├── soh/        port C++     │   ├── 2s2h/  port C++       ├── port/           port-made assets, shaders
│   ├── assets/     Torch YAML   │   └── assets/               ├── yamls/ or assets/yaml/   Torch configs
│   └── macosx/                  └── decomp_diff.py            ├── config.yml      ROM SHA1 → Torch config
└── docs/                                                       └── cmake/, Info.plist, *.desktop, *.rc
```

In SoH the port's own code is `soh/soh/` (Enhancements/, Network/, SohGui/, resource/, Extractor/, SaveManager,
frame_interpolation, OTRGlobals), and `soh/src/` is the OoT decompilation, edited in place. `soh/.gitrepo` shows it is a
**git-subrepo** of `HarbourMasters/soh`, i.e. a snapshot that is no longer merged from zeldaret/oot. The newer ports put
port code in `src/port/` beside the decomp files (Starship: `src/port/{hooks,interpolation,extractor,resource,ui,…}`;
Ghostship: `src/port/{api,events,mods,net,permissions,Rando,web,android,…}`).

### 1.2 Resource manager and archives [read]

- **Containers.** `.otr` is MPQ (StormLib, now optional behind `INCLUDE_MPQ_SUPPORT`), `.o2r` is ZIP (libzip, the
  default), and a `FolderArchive` loads loose files for development. SpaghettiKart has dropped `.otr` entirely.
- **Resource format.** Each file in the archive starts with a 64-byte little-endian header: byte order, an "is custom"
  flag, a 4-byte type magic (`ODLT` display list, `OTEX` texture, `OVTX` vertices, `OMTX` matrix, `AARR` asset array…),
  a version, and a 64-bit asset ID (Torch `docs/otr_format/mainpage.md`). Cross-asset pointers become 64-bit hashes of
  asset paths. Typed **factories** (binary and XML) deserialize each type; a game registers its own factories
  (SoH `soh/soh/resource/`, 165 files).
- **Override semantics.** `ArchiveManager` maps `CRC64(path)` to the archive that holds it; archives are added in order
  and **the last archive added wins** per path. The mods folder is loaded after the base archives, so a mod replaces a
  resource by shipping a file at the same path.
- **Alternate assets.** If alt assets are enabled, a lookup of `path` first tries `alt/path`
  (`ResourceManager::LoadResourceProcess`); players toggle all alt assets with Tab. This lets one pack carry both
  "faithful" and "enhanced" versions.
- **`.meta` aliases.** A `path.meta` file beside a resource can redirect the load to a different file (e.g. a PNG with
  metadata), ranked by archive priority (`ResolveMetaAlias`).
- **Caching and threading.** Resources are cached by identifier; loads run on a thread pool.

### 1.3 Asset extraction from the user's ROM [read]

- **Torch** (MIT, "Our Resource Conversion Helper") replaced ZAPD/OTRExporter in the newer ports (2S2H still uses
  `ZAPDTR`+`OTRExporter`; SoH moved to Torch: `soh/soh/Extractor/TorchExtract.cpp`). A top-level `config.yml` is keyed
  by **ROM SHA-1**; each entry names the ROM, optional `preprocess` steps (e.g. MIO0 decompression that produces a new
  SHA-1 to chain to), the YAML asset descriptions to use, and output targets (`binary: sf64.o2r`, `code:`,
  `headers:`, `modding:`). The YAML lists each asset by symbol, type and offset (`soh/assets/yml/ntsc_1-0/objects/*.yml`
  — ~1,450 YAML files per supported ROM revision, 9 revisions).
- **One description, many outputs.** Torch subcommands: `o2r`/`otr` (runtime archive), `code` (C for the decomp
  build), `header`, `binary`, `pack` (folder → archive) and `modding export|import` (dump editable PNGs etc. and
  re-import them). The same YAML drives the matching decomp build and the port's runtime archive.
- **First run.** The game (not a separate tool) asks for the ROM, checks its hash against `supportedHashes.json`,
  runs the extractor in-process, and writes `oot.o2r` next to the saves (SoH README: "Processing OTR" →
  "OTR Successfully Generated"). Ghostship on Android extracts on-device. The port ships its own non-ROM archive
  (`soh.o2r`: custom textures, fonts, etc.) inside the app bundle (`soh/CMakeLists.txt`: installed into
  `../Resources` on macOS).
- **Versioning of the cache.** SoH's major version number "signifies major feature updates that require a new
  `oot.o2r`" (`docs/VERSIONING.md`), i.e. the extracted cache format is versioned with the app.
- **Language packs and romhacks (Lighthouse, Banjo-Kazooie) [doc].** Lighthouse extracts *other regions'* ROMs as
  language packs, and extracts **patched ROMs as mods** ("Romhacks" menu), with "delta" language packs that only carry
  changed dialogs. It warns when a hack is not fully supported.

### 1.4 Fast3D and its backends [read]

Fast3D (originally Emill's `n64-fast3d-engine`, per the sm64ex README) interprets the game's own display lists at
runtime — the game still builds `Gfx` command lists — and translates them into draw calls: it tracks RSP state
(matrices, lights, vertices), converts the RDP combiner state into generated shaders, caches textures by address/hash,
and batches triangles. Backends: OpenGL (all), Direct3D 11 (Windows), Metal (macOS/iOS). The game loop is
`gfx_start_frame(); /* logic */ gfx_run(cmds); gfx_end_frame();` (`docs/FAST-RENDERER.md`). `GBI_FLOATS` switches
`Vtx`/`Mtx` to floats for precision. LUS adds internal resolution, MSAA, texture filtering, a display-list debugger and
`cmake/cvars.cmake` CVars (`gInternalResolution`, `gMSAAValue`, `gVsyncEnabled`…). Ghostship adds Vulkan shaders in
its `port/shaders/vulkan`.

### 1.5 ImGui menus and CVars [read]

- **CVars** (`include/ship/config/ConsoleVariable.h`): typed values (Integer, Float, String, Color, Color24),
  `CVarGetInteger(name, default)`/`CVarSetInteger`, persisted to the app's JSON config
  (`shipofharkinian.json`, `spaghettify.cfg.json`) with legacy migration (`soh/soh/config/ConfigUpdaters.cpp`). Names
  are prefixed by category through macros (`CVAR_ENHANCEMENT("QuickPutaway")`, `CVAR_CHEAT(...)`,
  `CVAR_SETTING(...)`, `soh/soh/cvar_prefixes.h`).
- **Menus** are ImGui. A widget is declared once and bound to a CVar:
  `AddWidget(path, "Time Multiplier", WIDGET_CVAR_SLIDER_FLOAT).CVar(CVAR_CHEAT("TimeOfDayMultiplier")).Options(...)`
  (`docs/MODDING.md`). 2S2H adds a HUD editor, cosmetic editor and resolution editor (`mm/2s2h/BenGui/`).
- **Feature registration.** Each enhancement is one file that registers itself statically and re-runs when its CVar
  changes:

  ```cpp
  static void RegisterQuickPutaway() {
      COND_VB_SHOULD(VB_ALLOW_QUICK_PUTAWAY, CVarGetInteger(CVAR_QUICK_PUTAWAY, 0), { *should = true; });
  }
  static RegisterShipInitFunc initFunc(RegisterQuickPutaway, { CVAR_QUICK_PUTAWAY });
  ```
  (`soh/soh/Enhancements/Restorations/QuickPutaway.cpp`, `soh/soh/ShipInit.hpp`). There are 475 files under
  `soh/soh/Enhancements/` organized by kind (`QoL/`, `TimeSavers/`, `Fixes/`, `Restorations/`, `Graphics/`,
  `Cheats/`, `Difficulty/`, `cosmetics/`, `debugger/`, `randomizer/`…).
- **Presets** are bundles of CVar values (`Enhancements/Presets`). The **mod menu** keeps the enabled mods and their
  order in a CVar and supports drag reordering (`soh/soh/Enhancements/mod_menu.cpp`).

### 1.6 The hook/event system (GameInteractor and its successors) [read]

- **SoH/2S2H GameInteractor.** A table of typed hooks, `DEFINE_HOOK(OnSceneInit, (int16_t sceneNum))`,
  `DEFINE_HOOK(ShouldActorUpdate, (void* actor, bool* result))`… (`GameInteractor_HookTable.h`, ~70 hooks). Listeners
  register globally, **for an ID** (actor ID, scene ID), **for a pointer** (one actor instance) or **with a filter**
  (`ExecuteHooks`, `ExecuteHooksForID`, `ExecuteHooksForPtr`, `ExecuteHooksForFilter`).
- **"Vanilla behavior" Should-hooks.** The single most important pattern. The vanilla expression stays in the
  decompiled code as the default and listeners may overwrite it:

  ```c
  // soh/src/overlays/actors/ovl_player_actor/z_player.c
  GameInteractor_Should(VB_ALLOW_QUICK_PUTAWAY, !(this->stateFlags1 & PLAYER_STATE1_START_CHANGING_HELD_ITEM)) && …
  ```
  The `GIVanillaBehavior` enum documents each ID with its default expression and its extra `va_list` arguments. At
  this commit `soh/src` has **785 `GameInteractor_Should` sites in 188 files**, 177 `GameInteractor_Execute*` sites in
  93 files, **1,347 `CVarGetInteger` calls in 127 files**, 313 `FrameInterpolation_*` calls in 61 files and 214
  widescreen edge-anchor calls in 16 files — the size of the port's footprint inside the decompiled code. Open PRs
  such as "Hookify Item_Give / Item_CheckObtainability" keep moving logic out of `soh/src` into hooks.
- **LUS EventSystem (newer, shared).** `DEFINE_EVENT(Name, fields…)`, `REGISTER_EVENT`, `CALL_EVENT`,
  `CALL_CANCELLABLE_EVENT(...) { vanilla block }`, `CALL_CANCELLABLE_RETURN_EVENT`, listener priorities, and
  listeners that write through pointer fields to change values (`docs/SCRIPTING.md`). Starship (`src/port/hooks/list/`)
  and Ghostship (`src/port/events/list/{Engine,Game,Player,Render,Audio}Event.h`, documented in `EVENTS.md`) use it.
- **External control.** `Network/CrowdControl` and **Sail** (a TCP protocol defined in the game, driven by a separate
  Go/React desktop app) let outside programs trigger effects and subscribe to events.

### 1.7 Mod loading [read]

1. **Asset packs.** `.o2r`/`.otr`/`.zip` files dropped in `mods/` override resources by path (§1.2). Tools: `retro`
   (OTR generator GUI), `fast64` (Blender exporter). SpaghettiKart has a GameBanana page and documents custom tracks,
   actors and characters.
2. **Code mods, historically: fork the repository.** SoH's `docs/MODDING.md` is a guide to forking Shipwright,
   editing `soh/src/code/z_kankyo.c`, adding an ImGui slider and opening a PR. Most features therefore land upstream
   as CVar-gated enhancements rather than as separately distributed mods.
3. **Code mods, now: LUS scripting (2026).** A mod is a `.o2r` ZIP with `manifest.json` (`name`, `version`,
   `author`, `code_version` that must equal the engine's, `game_version`, `dependencies`, `binaries` per platform key,
   `checksum`/`signature`/`public_key`), optional C sources JIT-compiled by **TCC** (amalgamated by `tools/merge.py`
   into one translation unit per file, listed in `build.gen`) or prebuilt `.dll/.so/.dylib`, plus any assets. Entry
   points `MOD_INIT()`/`MOD_EXIT()`; `malloc`/`free` routed to the engine; cross-mod calls with `ScriptGetFunction`;
   dependencies topologically sorted. Archives are hashed (BLAKE2b-512) and signed (Ed25519, `tools/sign.py`); a
   `Keystore` of trusted publisher keys and a `SafeLevel` (`DISABLE_SCRIPTS`, `ONLY_TRUSTED_SCRIPTS`,
   `WARN_UNTRUSTED_SCRIPTS` default, `ALLOW_ALL_SCRIPTS`) decide what loads.
4. **Ghostship's port APIs on top** (`docs/scripting.md`): a C **UI API** (`C_AddSidebarEntry`, `C_AddWidget` with
   CVar-bound widgets), **achievements**, a **permissions** API (persisted consent dialogs that cannot be changed through
   CVars), and a **relay** API (named channels over the "Satella" WebSocket relay, 64 KB per message, permission-gated).

### 1.8 How the randomizer lives in SoH [read]

`soh/soh/Enhancements/randomizer/` (185 files) is the largest feature: the seed generator and logic (`3drando/`, a port
of the OoT3D Randomizer), `SeedContext`, `Plandomizer`, many `Shuffle*.cpp` feature files, hint and message tables,
check and entrance trackers. It integrates **through the hook layer**, not by editing data files: item-giving and
check logic are VB hooks (`VB_*` IDs and `RegisterShipInitFunc` with `{ "IS_RANDO" }` update paths), the seed and
settings live in the save (§1.10), and settings are CVars in a dedicated menu (`SohMenuRandomizer.cpp`). An
Archipelago fork (`HarbourMasters/Archipelago-SoH`) connects it to the multiworld server. Lighthouse (BK), Ghostship
(SM64, `src/port/Rando/{Logic,Spoiler,CheckTracker,…}`) and 2S2H (`mm/2s2h/Rando`) follow the same shape.

### 1.9 Widescreen and frame interpolation [read]

- **Widescreen** is a projection change in Fast3D plus HUD re-anchoring in game code: HUD X coordinates are passed
  through `OTRGetDimensionFromLeftEdge(v)` = `SCREEN_WIDTH/2 - SCREEN_HEIGHT/2*aspect + v` (and the right-edge twin)
  so elements hug the screen edges (`soh/soh/OTRGlobals.cpp`). Culling distances and some effects need individual
  fixes. 2S2H has a HUD editor for repositioning.
- **Frame interpolation** (`soh/soh/frame_interpolation.cpp`): the game runs its logic at the original rate; while it
  builds a frame, every matrix operation is **recorded** into a tree keyed by `OPEN_DISPS`/`CLOSE_DISPS` and explicit
  `FrameInterpolation_RecordOpenChild(actor, id)` calls. For each display frame between two logic frames the recorded
  matrices are interpolated (in model space for actors, to avoid the "paper effect" on 180° turns) and substituted
  into the display list. Camera cuts call `FrameInterpolation_DontInterpolateCamera()`. Zelda64Recomp does the
  equivalent in RT64 with "matrix group" tags added by patches (`patches/*_transform_tagging.c`).

### 1.10 Saves [read]

`SaveManager` stores saves as JSON with **named, versioned sections**: `AddSaveFunction(name, version, func,
saveWithBase, parentSection)`, `AddLoadFunction(name, version, func)`, per-version base loaders
(`LoadBaseVersion1..4`) and conversion from the pre-JSON format. Features (randomizer, stats) register their own
sections, so adding data never breaks old saves. sm64ex, by contrast, kept the original binary layout and reads
both endiannesses so emulator saves work (README); Picori writes `tmc.sav` in mGBA/VBA-M byte order.

### 1.11 Online features [read]

- **Anchor** (SoH `soh/soh/Network/Anchor/`): a TCP connection to a relay (`anchor.hm64.org:43383` by default, CVar
  configurable) carrying null-terminated JSON packets (`Network.h` over SDL_net). Players join a **room ID**, belong to
  **teams**, and exchange state-level packets: `PlayerUpdate` (position, joint table, equipment — drawn as a
  `DummyPlayer` actor), `SetFlag`/`UnsetFlag`, `GiveItem`, `EntranceDiscovered`, `UpdateDungeonItems`,
  `RequestTeleport`/`TeleportTo`, `DamagePlayer`, `GameComplete`. It is **event/state sync for co-op randomizers**,
  not lockstep simulation; Lighthouse ships "Anchor multiplayer" too.
- **Determinism hygiene**: SoH builds with `-fno-fast-math -ffp-contract=off` (top `CMakeLists.txt`).

### 1.12 macOS, packaging, CI, releases, licensing [read]

- **macOS**: CMake `MACOSX_BUNDLE` with `Info.plist.in`, icons generated with `sips`/`iconutil`,
  `fixup_bundle` to embed dylibs, CPack `Bundle`/DMG output, universal builds via
  `CMAKE_OSX_ARCHITECTURES="x86_64;arm64"`, deployment target 10.15 (SoH). **Every port surveyed signs ad hoc**
  (`CPACK_BUNDLE_APPLE_CERT_APP "-"`, `codesign --sign -`), none notarizes: SpaghettiKart's entitlements add
  `com.apple.security.cs.disable-library-validation` (to load mod dylibs); Zelda64Recomp's add `allow-jit`,
  `allow-unsigned-executable-memory`, `disable-executable-page-protection` and `disable-library-validation` (it JITs
  mods). Users get the Gatekeeper prompt (Sail's README documents `xattr -dr com.apple.quarantine`).
- **CI**: GitHub Actions matrix (macos-14 with MacPorts deps and ccache, Windows MSVC, Linux in Docker → AppImage,
  Switch/Wii U side builds), artifacts exposed per branch through nightly.link links in the README. Ghostship also
  signs Windows builds with a PFX.
- **Releases**: SoH uses `x.y.z` with codenames ("Dewey Alpha 9.3.0", 2026-10-06), where `x` means "regenerate the
  archive". LUS uses epochs.
- **Licensing**: LUS and Torch are MIT; 2S2H, Starship and PaperBoat are CC0; Shipwright has no top-level license
  file; Zelda64Recomp and N64ModernRuntime are GPL-3.0; N64Recomp is MIT.

### 1.13 What made HM ports mod-friendly, and what didn't [inferred]

- Strong: data separated from code by path; last-archive-wins overrides; first-run extraction inside the app;
  one-file-per-feature enhancements toggled by CVars; Should-hooks that keep the vanilla expression readable;
  a shared engine (LUS) so five games share fixes; Torch's single asset description for decomp and port.
- Weak: for years "code mod" meant "fork the whole repo"; the decomp is a **frozen snapshot edited in place**, so names
  drift (both SoH and 2S2H now carry `decomp_diff.py`, added because "some things have also been renamed in SoH for
  convenience, in ways that don't align with the official decomp"). LUS's 2026 TCC/C mod system is the fix for the
  first problem; nothing fixes the second except discipline.

---

## 2. sm64-port, sm64ex and sm64coopdx

### 2.1 Replacing the N64 renderer [read/doc]

`sm64-port` (2020) put the PC target **inside the decompilation repository** ("Beyond Nintendo 64, it can also target
Linux and Windows natively"), with platform code in `src/pc/` and Emill's Fast3D interpreting the unchanged display
lists (`src/pc/gfx/gfx_pc.c`, backends `gfx_opengl.c`, `gfx_direct3d11.cpp`, D3D12 in sm64ex, window layers
`gfx_sdl.c`, `gfx_dxgi.cpp`). Users built it themselves from their ROM (assets compiled in). sm64ex added an options
menu with rebinding, **external data loading** (textures and sound banks from files → texture packs), an OpenGL 1.3
fallback, Puppycam, draw-distance options, both-endian and text save formats (README).

### 2.2 How coopdx added multiplayer [read]

- **Topology**: one player hosts (`NT_SERVER`), others join (`NT_CLIENT`) over UDP sockets (`src/pc/network/socket/`)
  or **CoopNet** (`src/pc/network/coopnet/`, a lobby service with NAT traversal via libjuice, shipped as prebuilt
  `lib/coopnet` with source at `coop-deluxe/coopnet`). Discord integration is optional.
- **What is synced**: 46 packet types (`src/pc/network/packets/`): player state, objects, spawns, collectibles, level
  and area changes, save flags, chat, Lua custom packets and sync tables, mod lists and **mod downloads**. Objects are
  **SyncObjects** with an owner and max sync distance (`sync_object.h`, `override_ownership`); there is
  lag compensation, reliable/ordered channels, ban and moderator lists. It is authoritative-per-object state sync, not
  lockstep.
- **Mod distribution**: the host's enabled mods are sent to joining clients (`packet_mod_list.c`,
  `packet_download.c`, `mod_cache.c`), so players never pre-install anything to join.

### 2.3 What a "mod" is there [read]

- **Lua scripts** (Lua 5.3, vendored): a single `.lua` or a folder with `main.lua`, metadata in header comments
  (`-- name:`, `-- description:`, `-- incompatible:`, `-- category:`, `-- pausable:`). ~70 hook events
  (`HOOK_MARIO_UPDATE`, `HOOK_BEFORE_PHYS_STEP`, `HOOK_ALLOW_INTERACT`, `HOOK_ON_HUD_RENDER`, `HOOK_ON_WARP`,
  `HOOK_ON_PACKET_RECEIVE`, `HOOK_ON_FIND_FLOOR`…), action hooks, behavior hooks, sync tables
  (`gPlayerSyncTable`, `gGlobalSyncTable`) that replicate automatically, `ModFS` storage, live reload
  (`smlua_live_reload.c`).
- **Bindings are generated**: `autogen/convert_functions.py`, `convert_structs.py`, `convert_constants.py`,
  `gen_hooks.py` parse the game's headers (a curated `in_files` list such as `src/game/mario.h`,
  `src/engine/surface_collision.h`) and emit `smlua_*_autogen.c` plus the Lua docs (`docs/lua/functions.md`,
  `structs.md`, `constants.md`) and editor definitions (`autogen/lua_definitions/*.lua`). The API surface grows by
  adding a header to a list.
- **DynOS packs**: models, animations, levels and behaviors loaded from mod folders (`actors/`, `levels/`), written in
  the **decompilation's own source formats** (`geo.inc.c`, `model.inc.c`, `collision.inc.c`) and compiled at load,
  so fast64 exports work directly.
- **Code patches**: deep changes still mean forking the C. Install is drag-and-drop onto the window; mods live in
  `mods/` beside the exe or in the user directory.
- **Assets at runtime [read]**: coopdx no longer compiles ROM assets in. `src/pc/rom_assets.h` declares zero-filled
  arrays with `__attribute__((constructor))` registrations (`ROM_ASSET_LOAD_TEXTURE(name, file, physAddr, size, …)`),
  and `rom_checker.cpp` fills them from the user's ROM at startup. This is the cheapest way to keep a released binary
  free of ROM bytes when assets are referenced as C arrays.

---

## 3. Static recompilation

### 3.1 N64Recomp, N64ModernRuntime and Zelda64Recomp [read]

- **How it works**: N64Recomp translates each MIPS function of the ROM into a C function, instruction by instruction
  (`addiu $r4,$r4,0x20` → `ctx->r4 = ADD32(ctx->r4, 0X20);`), `jal` into calls, `jr` jump tables into `switch`,
  function pointers into `LOOKUP_FUNC(addr)`, relocatable overlays through `RELOC_HI16/LO16` macros. Metadata comes from
  an ELF (in practice from the decompilation's symbols, `Zelda64RecompSyms`). The output runs on **N64ModernRuntime**:
  `ultramodern` reimplements libultra's OS (threads, messages, VI timing) and `librecomp` provides memory, overlays,
  saves (`eep`, `flash`, `pak`), RSP microcode recompilation and the mod loader.
- **Renderer**: RT64 (D3D12, Vulkan, Metal), a high-level renderer for N64 display lists with framebuffer-effect
  accuracy, high frame rates (via matrix tags), widescreen/ultrawide and texture packs (`.rtz`).
- **UI**: RmlUi (HTML/CSS-like, controller-navigable menus), not ImGui.
- **Distribution**: "releases do not contain game assets"; the user selects the US ROM in the main menu and assets load
  from it with no extraction step; the executable itself contains the machine-translated code.
- **Base patches**: Zelda64Recomp's own enhancements are C files in `patches/` compiled **to MIPS** with the decomp's
  headers, recompiled in "single file output mode", and linked ahead of the main output so they replace originals.

### 3.2 The recomp mod model [doc: modding docs at hackmd.io/fMDiGEJ9TBSjomuZZOgzNg]

A mod is an `.nrm` (renamed ZIP) holding a JSON manifest (game ID, mod ID, version, authors, dependencies, config
options), an optional **MIPS binary plus symbol file**, optional textures, thumbnail and native libraries. Authors
write C against the decomp headers using the template (`MMRecompModTemplate`), build an ELF with a MIPS toolchain and
run `RecompModTool`. At game start **LiveRecomp** recompiles the mod's MIPS to native code (offline-recompiled DLLs
exist but are discouraged). Mechanisms:

- `RECOMP_PATCH` replaces a function by its original symbol name; two mods patching one function are incompatible.
- `RECOMP_HOOK("Player_Init")` (entry) and `RECOMP_HOOK_RETURN("Player_Init")` (exit) let many mods stack on one
  function; the runtime **regenerates** hooked vanilla functions with hook slots (`librecomp mods.hpp:
  regenerate_with_hooks`, `hook_slots`).
- `RECOMP_EXPORT`/`RECOMP_IMPORT("mod", …)`, `RECOMP_DECLARE_EVENT`/`RECOMP_CALLBACK("*", event)` for mod-to-mod and
  base-game events; native libraries per platform; config options rendered in the mod menu.
- Mods install by drag-and-drop or the in-game menu, are listed on **Thunderstore**, and built-ins ship in the app
  (`mods/BUILTIN_MODS.md`).

### 3.3 XenonRecomp / UnleashedRecomp [doc]

Same idea for Xbox 360 PowerPC → C++ with a new renderer (D3D12/Vulkan; pipeline compilation integrated with asset
loading, so no shader stutter). Its installer **imports and verifies** the user's game files once. Modding reuses the
community's existing file-replacement format (Hedge Mod Manager / Sonic Generations format); code modding was still
"planned" (README). Lesson: support the format the community already uses.

### 3.4 DS Pokémon recompilation [searched]

I found no Gen 4/5 DS static recompiler ("Gen4Recomp", "nativeplat", "Platinum: Recompiled" return nothing on GitHub
or the web; one empty `NDS-recomp` repository exists). The live "recomp" activity around Pokémon is **Gen1Recomp**
(bryanthaboi, ~3,950 stars), which despite its name is a hand-written Lua/LÖVE2D *reimplementation* (§4.4), and
`EmeraldRecomp`/`MinishCapRecomp` (mstan, GBA static recompilation previews built on a "gbarecomp" framework).

### 3.5 Is recompilation "emulation"? Pros and cons for BW2 [inferred]

- The **CPU is not emulated** (translated ahead of time), but the **machine model is**: all game data lives in an
  emulated address space (`rdram`), registers live in a context struct, pointers are 32-bit console addresses, and the
  GPU's command stream is interpreted by a high-level renderer (RT64) — which is also what Fast3D does for decomp
  ports. The honest line is: decomp port = human-readable C with native types and pointers; recomp = machine-written C
  over an emulated memory map.
- For BW2 specifically:
  - *Pros*: whole game immediately, bug-for-bug; useful as a **differential-testing oracle** for decompiled C.
  - *Cons*: mods would have to be compiled to ARM/Thumb and recompiled at load (the recomp mod model), structures are
    byte offsets into DS RAM, ARM↔Thumb interworking and overlays sharing address ranges must be handled, the ARM7
    side (sound, wireless) still needs HLE, DS Protect checksums must be neutralized, and the decomp's hard-won
    names/types are only a header layer. Widescreen, interpolation and hooks need patch functions written against the
    emulated memory model.
  - A *hybrid* (recompile the still-undecompiled functions, compile the rest natively) does not work cleanly: native C
    uses host pointers and 64-bit layouts, recompiled code uses DS addresses. Recomp works for Zelda64Recomp because
    *all* code shares one emulated memory.
  - **Conclusion**: for a project that already has a decomp heading to 100% C and wants intuitive modding, a decomp
    port is the right base. Note that the port needs *functionally equivalent* C, not *matching* C: nonmatching
    functions are fine if equivalent, so the port can start before 100% match (see §7.16).

---

## 4. GBA and other 2D decomp ports

### 4.1 pokeemerald `pc_port` (Kurausukun, camthesaxman, Pidgey, NT_x86) [read]

A branch of pret's pokeemerald (last merge 2026-08-22) with `src/platform/{sdl2.c,win32.c,system.c,bios.c,dma.c,
cgb_audio.c,gba_easy_draw.c,gba_fast_draw.c}`. The game still writes GBA I/O registers, VRAM, OAM and palette RAM —
now host arrays — and a **software PPU** composites BG/OBJ layers into a framebuffer that SDL presents. The m4a sound
engine runs natively with a CGB audio emulation. Windows and Linux; no widescreen, randomizer or multiplayer.

### 4.2 Sonic Advance trilogy (SAT-R/sa2) [read]

- **In-tree port**: the same repository builds the matching GBA ROMs and `make sdl` (Linux/macOS), `sdl_win32`,
  `ps2` and `sdl_psp`. Platform code is in `src/platform/{pret_sdl,shared,win32,ps2}` (the SDL layer credits the
  pokeemerald `pc_port`). Game code differs by guards: `#if PORTABLE` (29), `#if PLATFORM_GBA` (28), `#if !PLATFORM_GBA`
  (13), plus `NON_MATCHING` and `TAS_TESTING`. Upstream merges are trivial because there is no fork.
- **Renderer**: a software PPU adapted from gpSP (`src/platform/shared/video/gpsp_renderer.cc`, GPL-2), selected by
  `RENDERER == RENDERER_SOFTWARE`; a non-functional OpenGL path exists.
- **Widescreen (426×240)**: the decomp already writes `DISPLAY_WIDTH`/`DISPLAY_HEIGHT` instead of 240/160 — **474
  uses in 131 files** — so widescreen is mostly a constant change in `include/gba/defines.h`; `WIDESCREEN_HACK`
  (10+6 sites) fixes the remaining assumptions: tilemap wrap when the camera passes the map edge, 32 vs 64-tile map
  widths, a widened field. TAS-style deterministic input playback (`TAS_TESTING`) exists for regression checks.

### 4.3 Project Picori — The Minish Cap (999sian/tmc) [doc + tree]

Built on zeldaret/tmc with SDL3, a **vendored software PPU** (`port/ppu`), the agbplay audio engine, Linux/Windows/macOS
(Apple Silicon and Intel). One **multi-region binary** validates the ROM by SHA-1 and selects US/EU/JP at runtime. On
first launch it builds a per-region **asset cache** (`assets/usa/…`) plus an **editable** `assets_src/`; an asset pak
loader and a mods include path exist (`port/port_asset_pak*.cpp`, `port_asset_loader_mods.inc`). Saves are
byte-compatible with mGBA/VBA-M and auto-migrated. It already has `port/rando`, accessibility audio cues, Discord RPC,
a debug menu and an experimental Vulkan ray-tracing path. Press coverage (May 2026) calls it rough.

### 4.4 Gen1Recomp / "G1R Deluxe" (bryanthaboi) — the Pokémon precedent for mod culture [doc]

Not a decomp port: the engine is hand-written Lua on LÖVE2D, "ported from the pret disassemblies & C", for Red through
Emerald. Game data, graphics and audio programs are **decoded on first launch** from SHA-1-verified ROMs into a
private cache; music is synthesized at runtime. What matters here is its mod platform:

- `manifest.json` with `id`, semver `version`, `api` level, `entry`, `profile` (`content` / `overhaul` /
  `total_conversion`), `category`, `games`, `game_version` semver range, `priority`, `dependencies` and
  `optional_dependencies` (with semver ranges, per-game scoping and GitHub repo hints for download), `conflicts`,
  **`permissions`** (`engine_internals`, `network`, `filesystem`), and **`required_imports`** — other user-owned
  files (e.g. a Pokémon Stadium 2 ROM, MD5-validated) that the launcher copies privately into the mod's `baseroms/`,
  readable only through a bounded `mod.imports:read(id, offset, len)` API.
- **Content registries** with schemas (`src/mods/Schemas.lua`) and a **generated registry reference**; events and
  hooks; per-mod saves and options; an in-game manager (F10); load order editable by the player; a **mod index repo**
  (`gen1recomp-mod-index`); a twelve-step tutorial ladder, cookbook and example mods "one per kind of author"; a
  custom **Tiled** build for map editing that exports maps as mods; an external save editor.
- **Online**: a launcher lobby over a TCP relay (`../pokeserver`) with rooms, spectators and tournaments; LAN link
  over ENet. Compatibility is enforced by an **"arena profile"** pinned on every room — engine version, API version,
  a **fingerprint of the battle dataset**, ruleset ID, and either `vanilla` or a **sealed "cart"** (an immutable mod
  bundle identified by its manifest hash) — checked by the relay on join (`docs/link-security.md`).
- Its visible success (voxel 3D mod covered by Digital Foundry, Polygon, Kotaku) shows that Pokémon players and modders
  respond to a scripting-first, registry-based mod API with tutorials.

### 4.5 How 2D ports replace the PPU, and widescreen for tile-based 2D [inferred from the above]

- Every GBA decomp port surveyed keeps the game writing hardware state (registers, VRAM, OAM, palettes) into host
  memory and **renders in software** (gpSP-derived, custom "easy/fast draw", Picori's vendored PPU). It is simple,
  exact and cheap at 240×160; GPU work is limited to scaling/filters.
- Widescreen in tile engines needs: (1) a symbolic screen width throughout game code; (2) drawing extra map columns
  (maps are often wider than the screen; at the map's own edge, clamp or wrap deliberately); (3) relaxing sprite
  culling written against the original width; (4) re-anchoring HUD elements to the new edges; (5) scenes that cannot
  widen (menus, cutscenes) pillarboxed per scene.
- Built-in randomizers and multiplayer are rare among GBA decomp ports (Picori's `port/rando` is early); the
  Pokémon examples with both are reimplementations (Gen1Recomp) or emulator-driven (Archipelago's Emerald world).

---

## 5. Other long-lived community ports and their mod scenes

**devilutionX (Diablo, ~9,900 stars).** It turned hard-coded C++ tables into **TSV data files**
(`assets/txtdata/{classes,items,monsters…}/*.tsv`) and made the Hellfire expansion itself a **mod**
(`mods/hf/manifest.ini` with `name`, `saveExtension=hsv`, plus `txtdata/`, `lua/`, `nlevels/`, `ui_art/` overrides). Lua
scripting (`Source/lua/`, `assets/lua/mods/*/init.lua`) handles small gameplay and UI mods. What made modding easy:
data-driven tables in human-editable files, the same override mechanism used by the official expansion, and a
per-mod save extension so modded saves never corrupt vanilla ones.

**OpenRCT2 (~16,400 stars).** Plugins are JavaScript files in the user's `plugin/` folder, run by **QuickJS-NG**
(ES2023; Duktape before v0.5.0), with a typed API (`distribution/scripting/openrct2.d.ts`, also on npm as
`@openrct2/types`), `registerPlugin({ name, version, authors, type, licence, targetApiVersion, minApiVersion, main })`,
**hot reload**, a stdin **REPL**, and three script types (local, remote, intransient). Multiplayer stays consistent
because **all state changes go through serializable game actions**, including custom ones from plugins, and "remote"
plugins are sent from server to clients. Objects (rides, scenery) are JSON+PNG packages, and the project ships
replacements for some original assets. What made modding easy: a typed, versioned API with min/target versions, a
REPL, hot reload, and a multiplayer model where mods are actions, not memory pokes.

**RSDKv5 decompilation (Retro Engine; Sonic Mania, Sonic Origins).** The engine is decompiled separately from game
logic (a game DLL of named object classes). The built-in mod loader (`RETRO_MOD_LOADER`) reads mods with `mod.ini`, a
`Data/` overlay for files and XML asset loading, and native logic libraries using the **ModAPI**: callbacks
(`MODCB_ONGAMESTARTUP`, `ONSTAGELOAD`, `ONUPDATE`, `ONDRAW`…), `RegisterObject` / `RegisterObjectHook` (add or
inherit object classes by name, calling the original through `Super`), `RegisterStateHook(state, hook, priority)` to
wrap state functions by pointer, public functions shared between mods, save redirection. What made it easy: the game's
own indirection (objects by name, states as function pointers) is the hook surface, so mods hook without touching
engine source.

**Perfect Dark PC port (fgsfdsfgs, now `perfect-dark-pc-port/perfect_dark`, ~2,060 stars).** A fork of the n64decomp project with a `port/` directory
(`port/fast3d`, `port/src`), reading **the ROM at runtime** from `data/pd.ntsc-final.z64` (no extraction step),
widescreen, 60 fps and experimental high frame rates, split-screen multiplayer, and netplay on a separate `port-net`
branch. Modding is a data-directory override ("basic mod support… enough to load a few custom levels"). One executable
per region (`-DROMID=pal-final`) — the cost of compile-time version selection.

**OpenGOAL (Jak and Daxter 1–3, ~3,580 stars).** A decompiler turns the game's GOAL bytecode into readable GOAL
source, which a new compiler (`goalc`) builds; game assets are extracted from the user's disc at install by the
launcher. The **REPL** connects to the running game and recompiles functions live — the best mod-development loop in
this survey. Custom levels come from Blender `.glb` plus a `.jsonc` description (`custom_assets/jak1/levels/`). Mods
are forks built from a "mod base" template and distributed through **mod sources** (JSON lists) in the official
launcher (since August 2024). What made modding easy: a live REPL, a level pipeline from Blender, and a launcher that
installs community forks; what makes it heavy: every mod is a full fork.

---

## 6. Cross-cutting comparison [inferred]

| Concern | LUS ports | sm64coopdx | Recomp | GBA ports | Others |
| --- | --- | --- | --- | --- | --- |
| Game code | decomp snapshot edited in place (subrepo) | fork of fork | machine-translated + MIPS patches | in-tree in the decomp (sa2, pc_port) or fork (Picori) | data-driven (devilutionX), named objects (RSDK) |
| Hardware layer | libultra reimplemented ("compile-time WINE") | sm64-port `src/pc` | N64ModernRuntime | software PPU + SDL | — |
| Assets | ROM → Torch → `.o2r` at first run | ROM read at start into C arrays | ROM read at runtime | ROM → cache (Picori) / build-time (sa2) | ROM/disc at runtime (PD, OpenGOAL) |
| Data mods | archive override by path, `alt/` | DynOS packs, external data | `.nrm` textures, `.rtz` | early | TSV/JSON tables, `Data/` overlays |
| Code mods | fork → TCC C mods (2026) | Lua 5.3 with autogen bindings | C → MIPS → LiveRecomp, hooks | — | JS (OpenRCT2), Lua (devilutionX), native (RSDK), forks (OpenGOAL) |
| Hooks | GameInteractor, VB Should-hooks, EventSystem | ~70 Lua hooks | per-function entry/return hooks + patches | — | game actions, object/state hooks |
| Multiplayer | Anchor (state sync via relay) | host + per-object ownership, mod download | — | — | lockstep actions (OpenRCT2) |
| macOS | universal `.app`, ad-hoc signed | ad-hoc signed `.app` | ad-hoc, JIT entitlements | SDL builds | — |

---

## 7. Recommended architecture for the BW2 port

### 7.1 Principles [inferred]

1. **The decomp is the game.** Compile `src/` unmodified; anything platform-specific goes below it (SDK
   reimplementation) or beside it (port layer). Changes that must touch game code go **upstream into pokebw2** as
   DS-build-neutral changes (§7.16). Never fork-and-edit (SoH's drift).
2. **One binary, both versions, data from the user's ROM.** Detect Black 2 / White 2 (and later other regions) by
   SHA-1 at runtime, like Picori; ship no ROM-derived bytes.
3. **Use the DS's real filesystem.** Unlike N64 games, BW2 already keeps nearly all content in NitroFS (NARCs, SDAT,
   NSBMD/NSBTX, NCGR/NCLR/NCER…). Mount the ROM read-only and override files on top; extraction is optional and only for
   modders' convenience.
4. **Mods are data first, scripts second, native code third**, all through one mod package format and one event bus.
5. **Hook at the game's own indirection points first** (process tables, script command table, battle handler tables,
   task callbacks), as RSDK does, before adding hook sites to C.

### 7.2 Layering

```
┌──────────────────────────────────────────────────────────────────────────────────────┐
│ Enhancements (one file per feature, CVar-gated)  Randomizer  Online  Debug tools     │  port/features …
├──────────────────────────────────────────────────────────────────────────────────────┤
│ Mod runtime: manager · manifest · permissions · Lua VM + generated bindings ·        │  port/modding
│ native plugin ABI · registries · event bus (typed events, Should-hooks, table hooks) │
├──────────────────────────────────────────────────────────────────────────────────────┤
│ GAME CODE — pokebw2 src/ (gfl, system, ovNNN) compiled natively, unmodified          │  decomp src/
├──────────────────────────────────────────────────────────────────────────────────────┤
│ SDK layer: NitroSDK / NitroSystem / TwlSDK / DWC / NitroWiFi APIs reimplemented      │  port/ntr (libntr-like)
│ OS · FS(card/NitroFS → VFS) · GX/G2/G3 → video model · SND → audio · WM/DWC/SOC → net│
├───────────────┬────────────────┬───────────────┬────────────────┬────────────────────┤
│ Video: VRAM/  │ Audio: SDAT    │ Input: SDL3   │ VFS: ROM mount │ Platform: SDL3     │
│ OAM/regs model│ HLE player,    │ gamepad, kbd, │ + overlays +   │ window, paths,     │
│ 2D compositor │ 16-ch SPU mixer│ touch/mouse,  │ mods + cache   │ threads, dialogs,  │
│ GX→GPU 3D     │ stream replace │ gyro          │ hot reload     │ crash handler      │
│ SDL_GPU: Metal│ / Vulkan / D3D12                                                     │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

- **SDK layer** is the LUS analog ("compile-time WINE for NitroSDK"): same headers and names as the decomp's `lib/`
  (`OS_*`, `FS_*`, `GX_*`, `G2_*`, `G3_*`, `NNS_G3d*`, `NNS_Snd*`, `CARD_*`, `RTC_*`, `WM_*`, `DWC_*`, `SOC_*`) but host
  implementations; DS Protect (`lib/dsprot`) is stubbed; MSL is replaced by the host libc. The other report covers
  libntr as a candidate.
- **Overlays**: link every `ovNNN` into the executable; `FS_LoadOverlay`/`GFL_OVERLAY` become bookkeeping, but must
  **re-run the overlay's static initialisation and zero its `.bss` on each load**, because game code may rely on fresh
  overlay state. Generate this from `config/*/delinks.txt`.

### 7.3 Repository structure

Two viable shapes:

- **(A) In-tree, as sa2 and sm64-port did** — a `port/` directory inside pokebw2. Port and portability fixes land
  atomically with decomp changes, and the matching build in the same CI keeps them honest.
- **(B) A separate port repository** that pins pokebw2 as a submodule and never patches it — the HM shape minus the
  in-place edits. Independent release cadence, cleaner for players.

Recommendation: **start with (A)** while the decomp is incomplete and portability fixes are frequent, with the rule
that `port/` never contains copies of decomp files; split to (B) later by moving `port/` out, which is easy if the rule
held. Proposed tree (new parts only):

```
pokebw2/
├── src/ include/ lib/ data/ config/ tools/ docs/   (unchanged decomp; matching build via configure.py + ninja)
├── port/
│   ├── CMakeLists.txt, CMakePresets.json           (native build only; never used for the matching ROM)
│   ├── ntr/          SDK reimplementation: os/ fs/ gx/ g2d/ g3d/ snd/ card/ rtc/ wm/ dwc/ soc/ (or libntr submodule)
│   ├── platform/     SDL3 window, paths (Application Support/AppData/XDG), threads, time, dialogs, crash handler
│   ├── video/        hw model (VRAM banks, OAM, palettes, regs), 2d/ compositor, 3d/ GX interpreter,
│   │                 layout/ (dual-screen layouts), widescreen/, interp/ (frame interpolation), backends via SDL_GPU,
│   │                 shaders/ (msl/spirv/dxil generated from one source)
│   ├── audio/        sdat/ (SSEQ/SBNK/SWAR/STRM HLE), spu/ mixer, replace/ (streamed music packs)
│   ├── input/        bindings, controller DB, touch emulation (mouse/trackpad/stick cursor), gyro
│   ├── vfs/          nitrofs/ (ROM reader, NARC, BLZ/LZ), mounts/ (rom, language rom, port pak, mods, dev folder),
│   │                 index/ (path → winner, provenance), watch/ (hot reload)
│   ├── romtables/    generated loaders for ROM-derived tables that live in code sections (§7.5)
│   ├── save/         DS save image I/O (.sav compatible with emulators, PKHeX), sidecar sections for port/mod data
│   ├── engine/       main loop, frame pacing, CVars, config, event bus, table hooks, versions (B2/W2 dispatch)
│   ├── modding/      manager, manifest, permissions, signing/keystore, lua/ (VM + generated bindings), native/ (ABI)
│   ├── net/          relay client, wm-over-internet, dwc/gts replacement client, co-op sync, remote-control protocol
│   ├── features/     enhancements/ (qol/, timesavers/, fixes/, graphics/, difficulty/, cheats/), randomizer/,
│   │                 online/ — each feature one file, self-registering, CVar-gated
│   ├── ui/           ImGui: menus, mod manager, settings, first-run ROM picker, layout editor, notifications
│   ├── debug/        VRAM/OAM/BG/palette viewers, GX command debugger, heap viewer (GFL heap IDs), flag/var editor,
│   │                 party/PC editor, field-script debugger, warp menu (data/zones), RNG viewer, input display
│   ├── assets/       port-made assets only (fonts, icons, UI art, shaders) → packed as port.pak
│   ├── packaging/    macos/ (Info.plist.in, entitlements.plist, icns), linux/ (AppImage, Flatpak), windows/ (rc, manifest)
│   └── tests/        unit tests, replay tests, differential tests against an emulator
├── tools/port/       generators: lua bindings + docs from headers, event/VB tables, romtables, overlay init tables;
│                     modtool (export/import/pack/sign), romhack→mod differ
└── sdk/              mod template(s), example mods (one per kind of author), generated API docs, LuaLS stubs,
                      C plugin headers, JSON Schemas (copied from data/**/*.schema.json)
```

User data (per platform: `~/Library/Application Support/<app>/`, `%APPDATA%`, `$XDG_DATA_HOME`, with a portable-mode
override like SoH's `SHIP_HOME`):

```
config.json            CVars, enabled mods and order, layouts
roms.json              remembered ROM paths + verified hashes (ROMs are not copied unless the user asks)
cache/<rom-sha1>/      optional extracted/converted assets, versioned by cache format
saves/<b2_us|w2_us>/   main.sav (+ main.port.json sidecar), backups
mods/                  <id>.zip or folders; disabled/ ; packs/ (textures, audio)
permissions.json  keystore.json  logs/  crash/
```

### 7.4 Versions in one binary [inferred]

pokebw2 records that 41,311 of 41,423 functions are byte-identical between versions and 112 differ under
`BLACK2`/`WHITE2`. Phase 1 can build two executables (as the PD port does per region). Phase 2: compile the few
translation units containing `#ifdef BLACK2/WHITE2` twice with a symbol prefix (`b2_`, `w2_`; generated from
`build/version_map.tsv`), and route calls through a small dispatch table chosen at boot from the ROM's hash. Version
data (encounters, trainers, text) needs no special handling because it comes from each ROM's NitroFS.

### 7.5 Assets, VFS and the legal model [inferred]

- **First run**: pick the ROM (file dialog or drag-and-drop), verify SHA-1 against a `supportedHashes.json`, remember
  its path. No conversion is required to play: the VFS reads NARCs and DS formats straight from the ROM.
- **Mount stack**, lowest to highest priority: ROM NitroFS → optional language ROM (another region, Lighthouse-style) →
  `port.pak` (project-made assets) → enabled mods in player-defined order → `dev/` loose folder with hot reload. Lookups
  are by NitroFS path and by NARC member (`a/0/1/6#123`); each lookup records which layer won (for the debug UI and
  conflict reports).
- **Two data surfaces for mods**: (1) **NitroFS-level files** (a replaced NARC member, NSBMD, NCGR…), which is what
  existing BW2 tools (CTRMap-CE/CTRMapV, Tinke, NitroPaint, map and script editors) already produce; (2)
  **decomp-level records**: the JSON + PNG layout pokebw2 already defines in `data/` with JSON Schemas
  (`data/abilities/<name>.json`, `data/pokemon/<species>/{data.json,front.png,…}`, `data/trainers/…`), loaded with
  **JSON merge-patch** semantics so a mod can change one field of one species. The port links C++ loaders generated
  from the same schemas; `tools/data/*.py` stays the offline converter (`modtool export|import|pack`, the Torch
  analog).
- **ROM-derived tables inside code**: some content (tables in ARM9/overlay `.data`/`.rodata`) would be compiled into
  the executable from decomp sources. To keep releases free of ROM bytes, generate `port/romtables/` from the symbol
  map: each such table becomes a zeroed buffer filled at startup from the ROM's ARM9/overlay images (decompressed
  BLZ) — coopdx's `ROM_ASSET_LOAD_*` idea driven by `config/` instead of hand-written macros. Torch's "swap C arrays for
  path references" is the heavier alternative.
- **Romhack import** (Lighthouse): diff a patched ROM's NitroFS against vanilla and emit a mod with only changed
  files; if ARM9 or overlay **code** differs, report that code patches cannot run natively and list the affected
  ranges (data-only hacks and most randomizer outputs then work).
- **Legal**: releases contain code and project-made assets only; the README says "provide your own legally obtained
  copy"; mods should prefer field-level JSON patches or per-file deltas over redistributing whole vanilla files.
  pokebw2 is GPL-3.0; native plugins linking against the port are likely derivative works — decide the plugin
  licensing policy explicitly (not legal advice). Note that the decomp repository commits converted `data/` (pret
  convention) while the port's release artifacts must not embed it.

### 7.6 Rendering, widescreen and dual screens [inferred]

- **Model**: keep the game writing DS state (VRAM banks, OAM, palettes, display registers, GX FIFO commands) into host
  structures, as GBA ports do; render from that state each frame.
- **2D (both engines)**: a GPU compositor that samples tile, map and palette data uploaded from emulated VRAM per
  frame, with a software reference path for testing. 2D runs at native 256×192 per screen and is scaled with
  selectable filters (nearest, sharp-bilinear, xBRZ/ScaleFX-style).
- **3D**: interpret the GX command stream (matrices, polygon attributes, textures, toon/edge/fog) into GPU draws at a
  high internal resolution, the Fast3D/RT64 role; texture replacement keyed by texture hash for HD packs.
  BW2 uses the 3D engine for the overworld and battle scenes, so this path carries most of the visible upgrade.
- **Backends**: **SDL3's GPU API** (Metal on macOS, Vulkan on Linux/Windows, D3D12 on Windows) so one backend covers
  all three platforms, with ImGui's SDL_GPU backend for menus. (LUS maintains three hand-written backends.)
- **Widescreen**: for 3D scenes widen the projection and relax view culling; for 2D layers draw extra map columns where
  the map is wider than 256 px and clamp at map edges (sa2's `WIDESCREEN_HACK` cases); interpret OAM X (9-bit,
  wraps at 512) as signed beyond the visible width; re-anchor HUD elements through a helper like
  `OTRGetDimensionFromLeftEdge`. Upstream to pokebw2 the habit sa2 has: named screen-size constants (NitroSDK's
  `HW_LCD_WIDTH`/`HW_LCD_HEIGHT`, not yet declared in pokebw2's `lib/nitro`) instead of literal 256/192 in screen
  math, which is matching-neutral. Scenes that cannot widen (menus, the touch screen) are pillarboxed per scene.
- **Dual-screen layouts**: stacked, side-by-side, large top with small bottom, bottom screen as a pop-up on a key, or
  single-screen swap; touch input from mouse/trackpad, a stick-driven cursor, or direct touch on touch displays; a
  **layout editor** like 2S2H's HUD editor; per-scene presets (battle vs overworld).
- **Frame pacing and interpolation**: run logic at the original rate with exact pacing; where the 3D scene is updated
  at a lower rate than the display, record matrices per object and interpolate (SoH's technique). Uncapped rendering
  with VSync or a limiter.

### 7.7 Audio and input [inferred]

- Audio: a high-level SDAT player (SSEQ interpreter, SBNK instruments, SWAR samples, STRM streams) feeding a 16-channel
  SPU-accurate mixer (ADPCM/PCM/PSG), with streamed replacements per sequence ID (SoH/2S2H custom music) and output
  through SDL3 audio.
- Input: SDL3 gamepads with a controller DB and rebinding UI (LUS ControlDeck is a model), keyboard, mouse-as-touch,
  gyro for touch cursor, and input display/recording for tests.

### 7.8 Hook system design [inferred]

Four tiers, in order of preference:

1. **Table hooks (no source edits).** BW2 runs on Game Freak's process manager: each screen is a
   `GameProcFunctions { init, main, exit }` table queued with its overlay ID through `GCTX_ProcMgrQueueProc` /
   `GCTX_ProcMgrReplaceProc` (`include/gfl/proc.h`). Field scripts dispatch through script command tables
   (`include/field/scrcmd_*.h`, `src/ov059/scrcmd_resort.c`). The battle engine registers per-ability/item/move
   **event handler tables** (`const BattleEventHandlerEntry *EventAddIntimidate(u32 *priority)`, handlers taking
   `BattleEventItem *, BtlServerFlow *, u8 monId, s32 *work` in `include/battle/btl_ability.h`). Many systems use task
   callbacks. At boot the port can wrap any entry of these tables: `register_script_command(id, fn)` lets mods add
   commands callable from field scripts (what BW2 hackers do today with code injection); `wrap_proc(functions, phase,
   fn)`; `register_battle_handler(kind, id, event, fn)` for new abilities, items and moves. The tables and their
   element types come from the decomp's headers; a generator in `tools/port/` emits the registry.
2. **Typed events (curated sites).** `PORT_EVENT(OnZoneChange, .zone = id)` and cancellable variants, using LUS's
   `DEFINE_EVENT` style with listener priorities, filters by ID (zone, species, trainer, item) and per-instance,
   following GameInteractor's `ForID`/`ForPtr`. Starter set for BW2: boot, frame, field init, zone change, map load,
   script command executed, flag/work-var set, item given/used, Pokémon created/added to party/PC, wild encounter
   generated, trainer battle start, battle start/end/turn/move executed, experience/evolution, trade, save/load, text
   printed (for TTS and translation), menu opened.
3. **Should-hooks (curated sites).** `PORT_SHOULD(VB_SKIP_INTRO_SCENE, <original expression>)`, keeping the vanilla
   expression in place and documenting each ID with its default, as `GIVanillaBehavior.h` does. In the DS build both
   macros expand to nothing or to the expression itself, so codegen is identical and the decomp can carry them (§7.16).
4. **Function hooks (later).** Entry/return hooks and replacement of any non-inlined function by name, like
   `RECOMP_HOOK`/`RECOMP_PATCH`. Options: a clang pass or source generator that gives every hookable function a
   predictable check of a per-function hook slot; or `-fpatchable-function-entry` with runtime patching (needs JIT-style
   entitlements on macOS, as Zelda64Recomp has). Measure overhead before committing.

All tiers feed one event bus that Lua, native plugins and built-in features share, so built-in enhancements are
written exactly like mods (SoH's `RegisterShipInitFunc` + `COND_VB_SHOULD` pattern), which keeps the API exercised.

### 7.9 Mod format and scripting [inferred]

**Package**: a folder or `.zip` (`.bw2mod` optional) with:

```
manifest.json      id, name, semver version, authors, license, description, icon,
                   api (integer level), app_version (semver range), games ["b2_us","w2_us"],
                   profile (content | overhaul | total_conversion), dependencies / optional_dependencies (semver),
                   conflicts, load_after, permissions ["network","filesystem","engine_internals"],
                   network: "client" | "all_players", options (declarative config → auto UI), signature fields
data/              decomp-layout JSON/PNG overrides (merge-patch per record)
fs/                NitroFS-path file overrides (whole files or NARC members)
text/              message overrides by bank/ID per language (delta packs)
textures/ audio/   HD texture replacements by hash; streamed music by sequence ID
scripts/           Lua (entry: scripts/main.lua)
native/            optional plugins per platform key (darwin-arm64, windows-x64, linux-x64)
saves schema       per-mod save section name/version (stored in the sidecar)
```

**Scripting language**: **Lua 5.4** as the primary language — the most familiar mod language in games (coopdx,
devilutionX, Garry's Mod, Factorio), small, embeddable in C, easy to sandbox (no `io`/`os`, instruction budget per
callback, separate state per mod), with **generated bindings** from decomp headers (coopdx's `autogen/` approach),
generated **LuaLS annotation stubs** for editor completion (OpenRCT2's `.d.ts` equivalent), generated reference docs,
an in-game console/REPL and hot reload. **Native C plugins** as the second tier for heavy mods, with a versioned C ABI
(`api` level checked at load, like LUS `code_version` and OpenRCT2 `targetApiVersion`) and the same event bus;
optionally TCC-compiled C sources as LUS does. **WASM** is the option to revisit if mods distributed automatically
in netplay need stronger isolation; it costs modders a toolchain.

**Developer experience**: a template repository; example mods "one per kind of author" (data tweak, new move via
battle handler, new script command, UI widget, texture pack, music pack, online-aware mod); a tutorial ladder; a
generated registry/API reference; drag-and-drop install; an in-game manager with load order, conflicts by path and by
hook, per-mod error isolation (a failing mod is disabled, not the game), and per-mod logs; a community mod index
(a GitHub index repo like Gen1Recomp's, or Thunderstore/GameBanana) with update checks.

### 7.10 Configuration and UI [inferred]

- CVars with typed values and hierarchical, prefixed names (`gSettings.*`, `gEnhancements.*`, `gCheats.*`,
  `gRandomizer.*`, `gMods.<id>.*`), persisted to `config.json` with migrations; console commands to get/set; presets as
  JSON bundles; flags on each CVar for "cheat" (disabled in competitive online rooms, like Ghostship's
  `race_disable`) and "synced" (must match between peers).
- Widgets declared once and bound to CVars (SoH `AddWidget(...).CVar(...)`); mod options declared in the manifest
  render automatically (Zelda64Recomp config options). Dear ImGui for settings and developer tools first; consider a
  controller-first themed UI (RmlUi, as Zelda64Recomp uses) later.

### 7.11 Randomizer [inferred]

Built in, SoH-style: settings as CVars, seed generated in-app, **seed and settings stored in the save sidecar**,
spoiler log JSON, plandomizer input. Implementation through the same layers as mods: data-level randomization
(encounters, trainers, items, TMs, starters, abilities, types, evolutions) produces an in-memory data layer from the
decoded records; behavior changes (items from NPCs, gift Pokémon, key items) use Should-hooks; logic-aware placement
for progression items. Archipelago support through events (location checked → send; item received → give), as the
SoH fork does. The Universal Pokémon Randomizer already handles BW2 at ROM level; data-only randomized ROMs would also
load through romhack import (§7.5).

### 7.12 Multiplayer [inferred]

Three layers, in order of value per effort:

1. **The game's own link features over the internet.** BW2 already has trades, link battles, Union Room-style
   features, Entralink/Join Avenue and Wi-Fi services. Implement the SDK's wireless/Wi-Fi APIs (WM, DWC, SOC) on top of
   a relay/lobby with NAT traversal (CoopNet/libjuice-style P2P with relay fallback, or a WebSocket relay like
   Ghostship's Satella), so the original battle and trade logic runs unchanged. GTS/Global Link need replacement servers
   (community services exist for Gen 4/5 Wi-Fi; verify before depending on them).
2. **Lobby-based battles and trades in the launcher**, with Gen1Recomp's **arena profile**: app version, API level,
   ruleset, a fingerprint of the battle-relevant dataset (species/moves/items/abilities after mods) and either
   "vanilla" or a sealed **cart** (an immutable, hashed mod set), checked by the relay on join; spectators and
   tournaments as rooms of rooms.
3. **Co-op and shared-world features** (Anchor/coopdx-style): other players visible in the overworld as ghost NPCs,
   shared flags/items for co-op randomizer runs, through events and relay channels with permission prompts. All
   network-affecting state changes should go through serializable actions (OpenRCT2) so they can be validated.

Determinism: NitroSDK math is fixed-point (`fx32`), which makes cross-platform lockstep easier than float-heavy N64
games; still build game code with `-fno-fast-math -ffp-contract=off` (as SoH does) and avoid host-dependent iteration
order in port code.

### 7.13 Saves [inferred]

Keep the **original DS save image** byte-compatible (so saves move between the port, emulators, flash carts and PKHeX)
and put everything port- or mod-specific in a **sidecar JSON with named, versioned sections** (SoH's SaveManager).
Per-mod sections are namespaced by mod ID; a profile whose mods change data incompatibly (e.g. a total conversion)
gets its own save directory (devilutionX's `saveExtension` idea). Auto-backups on load; an import/export UI.

### 7.14 Debug tooling and testing [inferred]

- In-app: console with CVar/command access, log viewer, stats/frame graph, VRAM/OAM/BG/palette viewers, GX command
  debugger (LUS's display-list debugger equivalent), GFL heap viewer by heap ID, event-flag and work-variable editor,
  party/PC/bag editor, field-script stepper with breakpoints, warp menu from `data/zones`, RNG viewer, input
  display, event-bus inspector (which listeners fired, LUS's EventDebuggerWindow), VFS provenance view.
- Tests: input-replay tests with deterministic RNG seeds (sa2's `TAS_TESTING` idea); **differential tests** that run the
  matching ROM in a headless emulator and the port with the same inputs, comparing game-state snapshots at
  checkpoints, so nonmatching-but-supposedly-equivalent functions are verified.

### 7.15 CI, packaging and releases [inferred]

- In pokebw2's CI: keep the matching build (both SHA-1s) and add a host compile of every game file with clang
  `-Wall` on macOS and Linux so portability regressions are caught where they are introduced.
- Port CI: macOS (universal arm64+x86_64, min 11 or 12), Windows (clang-cl or MSVC), Linux (AppImage and Flatpak);
  ccache; per-PR artifacts (nightly.link pattern); tag-based releases with release notes; a version scheme where a major
  bump signals a cache or save-sidecar format change (SoH).
- macOS: `.app` via CMake `MACOSX_BUNDLE`, `Info.plist` with document types for `.nds`/mod archives, icns, embedded
  dylibs, DMG. **Prefer Developer ID signing and notarization** (`notarytool`, hardened runtime) if a maintainer can hold
  an Apple developer account — every surveyed port ships ad-hoc signed and users meet Gatekeeper prompts. Entitlements:
  `disable-library-validation` only if native plugins are allowed; `allow-jit` only if a JIT (TCC, LuaJIT, runtime
  patching) is used.

### 7.16 Keeping game code close to the decomp [inferred]

What the surveyed projects show:

| Model | Projects | Upstream merges |
| --- | --- | --- |
| Port in the decomp repo, guarded by `PLATFORM_*`/`PORTABLE` | sm64-port, sa2, pokeemerald `pc_port` | trivial, continuous |
| Snapshot + in-place edits | SoH (git-subrepo), 2S2H, coopdx, PD | manual; names drift; diff scripts needed |
| No game source (recomp) | Zelda64Recomp | n/a; mods depend on symbol names |

Mechanisms to adopt:

1. **No copies of decomp files in `port/`.** The port compiles `src/` as-is; a check in CI fails if a file under
   `port/` duplicates one under `src/`.
2. **Absorb differences below the game** in the SDK layer, using the decomp's own `lib/` headers' names and
   signatures.
3. **Upstream portability fixes as matching-neutral changes**: symbolic screen constants; correct types where
   pointer-size assumptions exist (pokebw2's `docs/porting-notes.md` already tracks these); removal of UB the native
   compiler exploits; `NON_MATCHING` alternatives that are also *equivalent*.
4. **Upstream hook sites as no-op macros** (`PORT_EVENT`, `PORT_SHOULD(id, expr)`) defined in one header to nothing or
   to `(expr)` for the DS build, so the matching build is byte-identical. This needs the decomp maintainers' agreement
   (it affects the "natural C" rules); if they decline, keep tier-1 table hooks and tier-4 function hooks, which need no
   source sites at all, and limit curated sites to a patch queue regenerated on each merge.
5. **Generate all glue** (Lua bindings, event tables, romtables, overlay init, version dispatch, docs) from decomp
   headers and `config/` on every build, so renames upstream flow through automatically; never rename in the port.
6. **Track functional equivalence**: the port can start once every function has *equivalent* C, before every function
   *matches*. Add an "equivalent: yes/no/unknown" column to `docs/nonmatching-functions.md`, and use the differential
   tests (§7.14) to move entries to "yes".

### 7.17 Phasing [inferred]

1. **Boot**: SDK layer stubs, VFS on the ROM, software 2D, title screen, input, one executable per version.
2. **Playable**: 3D GX path, SDAT audio, saves (DS image), dual-screen layouts, ImGui settings, CVars, debug viewers;
   macOS first, then Linux/Windows CI.
3. **Moddable**: mod manager and package format, data overrides (NitroFS + decomp JSON), event bus tiers 1–3, Lua with
   generated bindings and docs, romhack import, template and examples.
4. **Enhanced**: widescreen, high internal resolution, HD packs, interpolation, QoL features, single binary for both
   versions.
5. **Social**: link/Wi-Fi over the internet, lobby battles with arena profiles, randomizer, Archipelago, co-op.
6. **Deep mods**: native plugins, function hooks, signing/keystore.

### 7.18 Decisions to make explicitly [inferred]

- In-tree `port/` vs separate repository (recommended: in-tree first).
- Whether pokebw2 accepts no-op hook macros and symbolic-constant changes in game code.
- Lua 5.4 (recommended) vs LuaJIT/Luau/JS/WASM for scripting; whether native plugins are allowed in online rooms.
- Renderer API: SDL_GPU (recommended) vs per-platform backends.
- Online infrastructure: who runs the relay and lobby, and which Wi-Fi replacement services to rely on.
- Plugin licensing policy under GPL-3.0.
- Apple Developer ID ownership for notarization.

---

## 8. Sources

Code read at the commits listed at the top:
[libultraship](https://github.com/Kenix3/libultraship) (`README.md`, `docs/PORTING.md`, `docs/SCRIPTING.md`,
`docs/FAST-RENDERER.md`, `docs/VERSIONING.md`, `src/ship/resource/*`, `cmake/cvars.cmake`),
[Shipwright](https://github.com/HarbourMasters/Shipwright) (`README.md`, `docs/MODDING.md`, `docs/VERSIONING.md`,
`soh/soh/Enhancements/game-interactor/*`, `ShipInit.hpp`, `frame_interpolation.*`, `OTRGlobals.cpp`, `SaveManager.h`,
`Network/*`, `Enhancements/mod_menu.cpp`, `CMakeLists.txt`, `.github/workflows/generate-builds.yml`),
[2ship2harkinian](https://github.com/HarbourMasters/2ship2harkinian) (`decomp_diff.py`, `mm/2s2h/`),
[Starship](https://github.com/HarbourMasters/Starship) (`config.yml`, `src/port/`),
[SpaghettiKart](https://github.com/HarbourMasters/SpaghettiKart), [Ghostship](https://github.com/HarbourMasters/Ghostship)
(`docs/scripting.md`, `src/port/events/EVENTS.md`), [PaperBoat](https://github.com/HarbourMasters/PaperBoat),
[Lighthouse](https://github.com/HarbourMasters/Lighthouse) (README), [Sail](https://github.com/HarbourMasters/Sail)
(README), [Torch](https://github.com/HarbourMasters/Torch) (`README.md`, `docs/otr_format/*`, `src/main.cpp`),
[sm64coopdx](https://github.com/coop-deluxe/sm64coopdx) (`src/pc/{lua,mods,network,gfx}`, `src/pc/rom_assets.h`,
`autogen/`, `docs/lua/lua.md`), [sm64ex](https://github.com/sm64pc/sm64ex) and
[sm64-port](https://github.com/sm64-port/sm64-port) (READMEs), [N64Recomp](https://github.com/N64Recomp/N64Recomp)
(README), [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime) (`librecomp/include/librecomp/mods.hpp`),
[Zelda64Recomp](https://github.com/Zelda64Recomp/Zelda64Recomp) (README, `patches/`, `.github/macos/`,
`mods/BUILTIN_MODS.md`), [UnleashedRecomp](https://github.com/hedge-dev/UnleashedRecomp) (README),
[SAT-R/sa2](https://github.com/SAT-R/sa2) (README, `src/platform/`, `include/gba/defines.h`),
[Kurausukun/pokeemerald pc_port](https://github.com/Kurausukun/pokeemerald/tree/pc_port),
[Project Picori](https://github.com/999sian/tmc) (README, tree),
[Gen1Recomp](https://github.com/bryanthaboi/gen1recomp) (README, `docs/modding.md`, `docs/guides/online-play.md`,
`docs/link-security.md`), [devilutionX](https://github.com/diasurgical/devilutionX) (`mods/hf/manifest.ini`,
`assets/txtdata/`), [OpenRCT2](https://github.com/OpenRCT2/OpenRCT2) (`distribution/scripting/scripting.md`),
[RSDKv5-Decompilation](https://github.com/RSDKModding/RSDKv5-Decompilation) (README, `RSDK/Core/ModAPI.hpp`),
[Perfect Dark port](https://github.com/perfect-dark-pc-port/perfect_dark) (README), [OpenGOAL](https://github.com/open-goal/jak-project)
(tree), and pokebw2's own `docs/code-organization.md`, `docs/porting-notes.md` and `data/`.

Web: [Zelda64Recomp modding documentation](https://hackmd.io/fMDiGEJ9TBSjomuZZOgzNg);
[OpenGOAL progress report Aug 2024](https://opengoal.dev/blog/progress-report-aug-2024) (launcher mod sources);
[Time Extension on the Minish Cap port](https://www.timeextension.com/news/2026/05/zelda-minish-cap-gets-natively-ported-to-pc);
[Tech e Blog on the Minish Cap port](https://www.techeblog.com/the-legend-of-zelda-the-minish-cap-native-pc-port-trailer-download/);
[EmeraldRecomp README](https://raw.githubusercontent.com/mstan/EmeraldRecomp/HEAD/README.md);
[CTRMap tag on Project Pokémon](https://projectpokemon.org/home/tags/CTRMap/).
Searches for a DS Pokémon static recompiler ("Gen4Recomp", "nativeplat", "Platinum: Recompiled") found nothing.
