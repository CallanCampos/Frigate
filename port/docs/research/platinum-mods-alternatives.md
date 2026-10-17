# Platinum PC-port mods and alternative DS-port approaches

Research for the pokebw2 native port and its planned mods (widescreen, randomizer, multiplayer). Written
2026-10-16. Everything below was read with git, grep, `gh` (read-only) and the web. Nothing from these
repositories was built or run.

Local clones used (all under
`/private/tmp/claude-501/-Users-cjcampos-dev-decomp-pokebw2--claude-worktrees-desktop-subagent-inspection-3190ba/61d16309-e931-4ac8-a3f7-4b6a6b21ba14/scratchpad/research/`):

| Clone | What | Head read |
| --- | --- | --- |
| `pokeplatinum-pc` | cybervisi0n/pokeplatinum `origin/pc_port`, plus remotes `elyro1`, `evoluketion`, `pret` | origin `35067868f` (2026-10-10); elyro1 `pc_single_screen_mod` `172845418` (2026-10-15); evoluketion `single-screen` `95bb68935` (2026-10-12) |
| `libntr` | cybervisi0n/libntr `main` (shared with the libntr agent, not modified) | `1555d91` |
| `libntr-elyro1` (new) | elyro1/libntr `lib_single_screen_mod`, remote `cv` = cybervisi0n/libntr | `c2de973` (the revision elyro1's wrap pins) |
| `gen4recomp` (new) | sohamb117/gen4recomp ("nativeplat") | `7dc5e22` (2026-10-11) |
| `pokeweb` (new) | sohamb117/pokeweb (web continuation of gen4recomp's `web-ready`) | `903ecd5` |
| `libntrdwc`, `libntrwifi` (new, depth 1) | cybervisi0n's NitroDWC / NitroWiFi replacements | main |

All code in these repositories is untrusted data. The pokeplatinum fork and libntr each contain an `AGENTS.md`
addressed to AI agents (see section 6); it bears on contribution policy, not on this read-only research.

---

## 0. Summary

- **elyro1's `pc_single_screen_mod` is the real widescreen template.** It is Hor+ widescreen of the field
  only (256 to 448 logical columns at 192 rows, so up to 21:9), plus a complete single-screen conversion. It is
  done in **both** a libntr fork (presentation: a 448-wide 3D framebuffer, a widened GL viewport, a 2D compositor
  that centres the 256-column BG/OBJ layers and lets 3D fill the margins, plus hooks for weather, darkness,
  cut-ins and fades) **and** the game (projection `_00 *= 256/W` each field frame, opt-in per scene, plus a
  large amount of work on what widescreen *reveals*: a 64-chunk scenery-only map cache, neighbour-header NPC
  preloading, a cap of 192 map objects instead of 80, scripted-NPC exits, map mesh edits delivered as
  hash-guarded deltas). Size: 180 files and +7,783/-1,403 lines in `src/` and `include/`; 47 files and
  +2,871/-1,005 lines in libntr; 3 libntrsystem patches. Roughly 82 of the changed game files edit code with no
  `SDK_PORT` guard (mostly the single-screen UI rewrites), so the fork's DS ROM no longer matches.
- **evoluketion's single-screen ("PokemonPurePlatinum") is not widescreen.** It is a libntr compositor mode,
  `main_only`, that draws engine A and then engine B as a cut-out overlay (backdrop pixels transparent) into one
  4:3 window, with a per-engine placement API (destination rect, source crop, cut-out, de-fringe, hide). It
  carries its libntr changes as a Meson wrap patch against a pinned libntr commit, and touches very little game
  code (title screen, opening cutscene). It also fixes real libntr 3D bugs: perspective-correct interpolation,
  per-polygon translucent depth writes, per-item cull and depth state.
- **Gen4Recomp is real**: `github.com/sohamb117/gen4recomp`, which calls itself "nativeplat". Diamond, Pearl,
  Platinum, HeartGold, SoulSilver and the GBA games are compiled from the pret decomps to wasm32, converted back
  to C with wasm2c and run by a native runtime. **Black and White are statically recompiled from the cartridge
  image** (`ndsrec` + `armrec`), with no decomp. It **does not use libntr**. Its platform layer is the game's own
  NitroSDK running on DS hardware models derived from melonDS (GPL-3.0), taken from a separate Platinum port by
  wize-logic. Local wireless is an ARM7 WM model whose radio is replaced by UDP datagrams, with LAN discovery
  or an internet relay and a room PIN. The reported claims check out (4x render scale, widescreen 3D, save
  editor, LAN Union Room) except "widescreen battle transitions": what exists is a separate *screen layout*
  for battles. Red flags: no top-level licence, apparently agent-driven development, and the sibling pokeweb
  deployment publicly hosts 12 commercial ROMs (encrypted with a public key).
- **Other DS ports:** an SM64DS decomp-based PC port (tangosdev), static-recompilation frameworks (ndsrecomp:
  MKDS on Wiimmfi, Metroid Prime Hunters, LEGO Star Wars II online; ndskit: Moon, Nostalgia, Lufia), KH 358/2
  Days (decomp complete, no port yet), zeldaret ph/st (no port). The only shared decomp-port SDK is
  cybervisi0n's lib family (libntr, libntrsystem, libntrwifi, libntrdwc, libvct, and **libtwl/libtwlsystem**,
  which matter for BW2's TWL-hybrid build). No HGSS or Diamond decomp port exists apart from gen4recomp's
  (cybervisi0n/pokeheartgold is 0 commits ahead of pret).
- **Multiplayer in the Platinum port:** libntr emulates local wireless (WM) over ENet on port 2009, using a host
  lobby and a peer mesh, with no relay or NAT traversal. The ImGui *Net* panel or `sim_config.ini` choose host or
  client. Nintendo WFC (GTS, Wi-Fi Club, Plaza) runs through decompiled NitroWiFi (CPS TCP/IP, SSL) and NitroDWC
  (GameSpy) over host sockets, with DNS from the in-game WFC settings. Press reports say the default DNS is
  Pokémon Classic Network's, the GTS works only partly and the Wi-Fi Club works but is finicky.
- **Randomizers:** none is built in. libntr's extractor dumps whatever `.nds` it is given with no hash check, so
  a UPR-randomized ROM's NARC data (encounters, trainers, base stats, learnsets, scripts) flows into the port,
  but anything the decomp compiles into code is ignored. The starters, for example, are `#define`s in
  `src/choose_starter/choose_starter_app.c`, and the marts are in `src/overlay007/shop_menu.c`. elyro1's mod
  refuses a randomized ROM whenever its script or map deltas hit changed members.
- **Norms:** players bring their own ROM, extracted at first launch. cybervisi0n's README forbids releases that
  bundle compiled assets and asks forks to carry libntr's MIT notice. The README and an `AGENTS.md` (added
  2026-10-08) **reject AI-written PRs and code suggestions**, and libntr's `AGENTS.md` says the same. We must
  not upstream anything to cybervisi0n's repos; we patch or fork instead (MIT allows it). The pokeplatinum fork
  and pret's decomp carry no licence file.

---

## 1. Widescreen and single-screen mods

### 1.1 Baseline: how libntr presents the DS

The relevant files, all in cybervisi0n/libntr:

- **3D.** `libraries/sim/src/g3_handler.cpp` interprets the G3 command stream on the CPU. The matrix stack is
  fx32 and the game's own projection matrix is used. `G3SIM_SubmitPolygon` divides by W on the CPU
  (`screenX = ((x+w)*256)/(2w)`, then to NDC; lines 1670-1683) and **ignores the DS VIEWPORT**: every polygon
  maps to the full 256x192 NDC square. `g3_draw.cpp` sends the polygons to OpenGL 4.2 in an FBO of
  256 x 384 x `internalResolutionScale`, with the two screen halves stacked. The GL viewport for 3D is
  chosen from `POWCNT` bit 15 (which LCD shows engine A).
- **2D.** `sim_g2.cpp` rasterizes BG and OBJ lines on the CPU into textures 256 wide.
  `shaders/g2_fragment.glsl` composes priorities, blending, windows and master brightness per pixel, with BG0
  sampled from the 3D texture when `DISPCNT` bit 3 is set.
- **Window layouts.** `vertical`, `horizontal` and `large` come from `sim_screenquads.c` and
  `config/sim_config.c`. Touch maps the window rectangle of the sub screen to 0-255 and 0-191 (`sim_main.cpp`,
  `s_TouchPanelCoords`).

Widescreen therefore needs (a) a wider 3D framebuffer and GL viewport, (b) a game-side projection change,
because the matrices are the game's, and (c) a decision about what the 256-column 2D layers do in the margins.

### 1.2 elyro1/pokeplatinum `pc_single_screen_mod` (widescreen + single screen)

- Commits: 93 over `origin/pc_port` (merge base `c08911193`, 2026-10-02), dated 2026-10-11 to 2026-10-15.
  Most commits are re-applied from an earlier history and say "Reapplied-from:".
- libntr: `subprojects/libntr.wrap` points at `https://github.com/elyro1/libntr.git` revision `c2de973` (branch
  `lib_single_screen_mod`, 47 commits over cybervisi0n/libntr; the merge base is `f20fa9e`). libntrsystem gets
  three `diff_files` patches: `extended-field-textures.patch`, `fast-forward-players.patch` and
  `simulator-feature-detection.patch`, all in `subprojects/packagefiles/libntrsystem_patch/`.
- The README calls it "Single-Screen Mod … widescreen, single-screen experience". Defaults: layout
  `top_only`, 60 FPS, a 1024x576 window. It also adds three save slots, a Quit item, the Pokétch as a menu item
  and Repel QoL.

#### Presentation, in the libntr fork

Core commit `6c1cb3d` "libnrt widescreen code" (8 files, +258/-48):

- **New layout** `SIM_CONFIG_SCREEN_LAYOUT_TOP_ONLY` ("Physical top display, up to 21:9"), set in
  `include/simulator/config/sim_config.h` and given the ini string `top_only` in `sim_config.c`. It becomes the
  default, with a default window of 1024x576.
- **Width model** in `sim_main.cpp`: `s_MaxViewWidth = 448 // 192*21/9`. `s_WindowViewWidth` is recomputed
  every frame as `clamp(192*winW/winH, 256, 448) & ~1`, an even number so the 256-column UI stays exactly
  centred. `s_FieldViewWidth` is the width the game opted into this frame.
- **3D FBO and viewport.** The `g3RenderTextureId` colour and depth buffers are allocated at
  `448*scale x 384*scale`. `Set3DViewport()` sets
  `glViewport((448-W)/2*scale, POWCNT-dependent y, W*scale, 192*scale)`, centring a W-wide 3D picture in the
  448 buffer.
- **The game-facing API is `u32 SIM_SetFieldWidescreen(BOOL enabled)`** in `include/simulator/sim.h`: "Field
  rendering opts in each logic frame; all other scenes use 4:3. Returns the logical width for the matching
  projection (256..448)." When the width changes it **flushes the buffered geometry**
  (`G3SIM_FlushArray(); G3SIM_DrawItems();`), because polygons already queued under the old viewport must be
  drawn with it.
- **2D compositor** in `shaders/g2_fragment.glsl` (`presentationWidth`, `fieldWidth`, `singleScreen`):
  - Two coordinate systems are used. `uiCoord = 0.5 + (x-0.5)*presentationWidth/256`. `worldCoord` is the
    same with 448. `sampleBg()` reads **BG0-as-3D at `worldCoord`**, the full width. Every other BG, the OBJ
    layers and the OBJ window read at `uiCoord` and **return transparent outside 0..1**, so the 256-column 2D
    layers stay native and centred, and only 3D shows in the margins.
  - For 4:3 scenes (`fieldWidth == 256`) the margins get `sideBackground()`. An extra pass renders the
    composed screen into a 256x256 RGBA16F texture, mipmaps it to one average colour
    (`s_SideAverageFramebuffer`, texture unit 10), and fills the sides with that colour under a dark gradient
    (`mix(0.55, 0.20, smoothstep)`). This is an "ambient pillarbox" rather than black bars.
- **Only the engine routed to the top LCD is drawn**:
  `if (singleScreen && !(dispSel ? !isSub : isSub)) return;` in `DrawEngine`, and `bg0as3d` is forced off for
  the sub engine. The game shows bottom-screen content by swapping the display (see "Single screen" below).
- **Mouse touch is disabled** in `top_only`: `SDL_MOUSEBUTTONDOWN` and the touch-rectangle mapping are both
  skipped.

Follow-up libntr commits for widescreen (`git -C libntr-elyro1 show <sha>`):

| Commit | What it adds |
| --- | --- |
| `e0db378` "widescreen borders" | border handling in the G2 shader and in `sim_main.cpp` (used by the gym-leader VS intros, which stay 4:3) |
| `258fff0` "Screen fade widescreen adjustments" | `SIM_SetScreenFadeWindows(main, sub)`: scales screen-fade window masks to the viewport for each engine |
| `94ca753` "Wayward Cave flash mask" | `SIM_SetFieldDarknessMask`: extends Flash's opaque darkness mask across the margins |
| `852709d` "Forest widescreen shader" | `SIM_SetFieldOverlayTiled`: repeats seamless weather/forest **BG2** textures across the wide viewport |
| `7fd5f6e` | `libraries/sim/include/sim_window.h` (DS WIN0/WIN1/OBJWIN/WINOUT semantics); per-scanline window masks **448 wide** (`s_WindowMasks[2][192*2*448*2]`, with the masks shifted by (448-256)/2); HBlank effects evaluated per scanline |
| (several) | `SIM_SetFieldTiledSpriteRange(addr, size, en)` and `SIM_SetFieldWideSpriteRange(addr, size, en)`: the game registers an OBJ VRAM range whose sprites are drawn into a **448-wide field sprite line** (`SIM_FIELD_SPRITE_WIDTH 448`, `s_FieldSpriteLine[448]` in `sim_g2.cpp`). "Tiled" ranges repeat at +256 (seamless particles such as snow, rain and ash); "wide" ranges continue past the edges without repeating. Recovers OAM X wraparound at 512 for clouds beyond the left edge. `SIM_SetFieldMoveCutIn`: widens the HM cut-in band without stretching it |
| `3f07847`, `05bad75`, `1601331`, `4f29ed5` | depth and W-buffer fixes (chunk-border depth rounding, overlapping foliage in Z and W modes), live resolution changes, a texture pool allocated on demand, `SIM_GetExtendedTextureStorage` / `SIM_Encode/DecodeTextureAddress` (a **virtual texture address space beyond the DS's 512 KB**), and frame profiling |

#### 3D projection, viewport and frustum, in the game

`src/overlay005/fieldmap.c`, `ov5_021D15F4` (the field's per-frame 3D setup):

```c
#ifdef SDK_PORT
    // The field remains alive behind Poketch; only widen it while it is on top.
    u32 viewWidth = SIM_SetFieldWidescreen(GX_GetDispSelect() == GX_DISP_SELECT_MAIN_SUB
        && !fieldSystem->encounterEffectBorders && fieldSystem->npcSceneNativeViewLocks == 0);
    Camera_ComputeProjectionMatrix(fieldSystem->camera->projection, fieldSystem->camera);
    if (viewWidth > 256) {
        MtxFx44 v2 = *NNS_G3dGlbGetProjectionMtx();
        v2._00 = FX_Mul(v2._00, FX_Div(256 * FX32_ONE, viewWidth * FX32_ONE));
        NNS_G3dGlbSetProjectionMtx(&v2);
    }
#endif
```

- This is **Hor+**: the vertical FOV is unchanged, the horizontal FOV widens by W/256, and the centre 256 columns
  are pixel-identical to native.
- `FieldMap_Exit` calls `SIM_SetFieldWidescreen(FALSE)`. `encounter_effect.c` sets
  `fieldSystem->encounterEffectBorders` for `ENCEFF_CUTIN_LEADER_ROARK..VOLKNER` and drops to 4:3 during gym
  leader cut-ins. Battles, menus and every other application stay 4:3 with ambient side fill.
- The frustum and culling follow automatically. libntr's box test (`GFXBoxTest_IsModelInView`) uses the
  current projection, so models in the margins are kept.
- The camera matrices are saved for the cutscene logic: `npcSceneViewWidth`, `npcSceneViewMatrix` and
  `npcSceneProjectionMatrix` are stored in `FieldSystem`.

#### What widescreen reveals, and how it was fixed

| Symptom | Fix | Where |
| --- | --- | --- |
| Only the DS's four loaded 32x32 map chunks (2x2 quadrants) exist, so empty void shows at the sides | a **PC-only prepared-chunk cache** of up to 64 chunks (`PC_PREPARED_COUNT 64`), loaded within a per-frame time budget and **rendered as scenery only** (model plus props; no collision, no scripts: "The original quadrant buffers remain authoritative for scripts and collision"). `PCRequestBounds` projects the camera's four frustum corner rays at **aspect 448/192** onto the ground plane (`camera->perspective.sinFovY/cosFovY`), adds a 16-tile margin and a velocity "lead" from measured load cost, and culls drawn chunks with `GFXBoxTest_IsModelInView` | `src/overlay005/land_data.c` (+709 lines, all `#if defined(SDK_BUILD_LINUX) \|\| defined(SDK_BUILD_WIN64)`), `LandDataManager_RenderLoadedMaps`, `LandDataManager_PrepareInitialView` |
| NPCs of the neighbouring map header pop in or are missing | `MapHeaderData_PreloadNeighborObjects` adds the neighbours' object events (spawned only at field setup); the map object capacity rises from **80 to 192** on the main matrix; "NPC loading overhaul", NPCs standing in grass, berries, trainer loading; "retains the last valid zone in empty space, avoids streaming 'Everywhere'" | `src/map_header_data.c` (+165), `src/map_object.c` (+189), `fieldmap.c` `ov5_021D1878` (`v0 = 192`), `FieldMap_ChangeZone` (bounds check, `MAP_HEADER_EVERYWHERE` ignored) |
| The texture VRAM budget is exhausted by the extra chunks and NPCs | the libntrsystem patch `extended-field-textures.patch` widens `NNSGfdTexKey` (size shift 7, size mask 0xFFF, address mask 0x7FFFF on PC) and adds `NNS_GfdInitLnkTexVramManagerEx(szByte, szPhysicalByte, …)`; libntr maps virtual texture addresses to extended storage; `src/g3d_pipeline.c` comment: "Widescreen field traversal keeps more map/NPC textures resident" | `subprojects/packagefiles/libntrsystem_patch/`, `src/g3d_pipeline.c` |
| Map geometry ends (forest borders, void) inside the wider view | **mesh edits** produced by hash-guarded Python scripts that copy whole tree columns rather than stretching the atlas (`tools/scripts/extend_twinleaf_town.py`: "Continue Twinleaf Town's eastern decorative forest for port cameras… Supports only the hash-guarded original static Twinleaf mesh"); maps edited: Twinleaf, Route 201, 204, 206, 207, 214, 219 (south of Sandgem), Hearthome, Iron Island | `tools/scripts/extend_*.py`, `verify_*.py`, `res/field/maps/data/meson.build` |
| Cutscene NPCs walk to the old screen edge and vanish in plain view | `src/port/scripted_npc_scene.c` (+475): extends an exit walk until the NPC is outside the *wide* view (32x32 billboards only, with an 8-pixel clip guard band), or takes a **native-view lock** (`npcSceneNativeViewLocks`) that drops the field to 4:3 when no safe route exists. New script commands `ApplyMovementWidescreen` (native and wide movement scripts, chosen at run time) and `ApplyMovementSafeExit`; 37 field script files edited | `src/scrcmd.c`, `include/data/scripts/scrcmd.h`, `res/field/scripts/*.s`, `res/port_patch_baselines/scripts/*.patch` |
| Weather (snow, rain, ash, fog, forest shader) covers only the middle 256 columns | `SIM_SetFieldOverlayTiled(weather in {0,1,3,9})`, `SIM_SetFieldDarknessMask(weather == 7)`, `SIM_SetFieldTiledSpriteRange(<OBJ image VRAM of the particle proxy>)` | `src/overlay005/ov5_021D5EB8.c`, `src/overlay009/ov9_02249960.c` (Distortion World: `SIM_SetFieldWideSpriteRange`), `src/overlay006/hm_cut_in.c` |
| Screen fades and window effects | `SIM_SetScreenFadeWindows(existsMain && type > FADE_TYPE_BRIGHTNESS_IN, …)` | `src/screen_fade.c` |
| Gym-leader VS intros | forced 4:3 with widescreen borders | `encounter_effect.c` |

**How the mod ships modified game data without redistributing it.** `src/port/map_patches.cpp` "Apply[ies]
embedded resource deltas to the extracted ROM filesystem". `tools/scripts/make_map_patches.py` ("Embed
member-level copy/literal deltas, never complete game archives") generates `map_patches.inc`, with a
`{path, member, sourceMember, before sha256, after sha256, outputSize, delta}` record for each patch over
`land_data.narc`, `map_matrix.narc`, `pl_msg.narc`, `scr_seq.narc` and `menu_gra.narc`. At launch it checks
the `before` hash of every member, applies the copy and literal operations, checks `after`, keeps originals in
`.map-patch-originals/`, and stages every archive before replacing any. On a mismatch it shows the error
"Original map data unavailable. Re-extract a clean US Platinum ROM". The retail baselines are in
`res/port_patch_baselines/`.

#### Single screen: how the touch screen's content gets onto one screen

There is no compositing. The mod uses four techniques, chosen per screen:

1. **Display swap.** The engine with the needed content is routed to the top LCD with
   `GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN)` or `GXLayers_SwapDisplay()`, and libntr draws only the engine
   routed to the top. The Pokétch is opened from the start menu (`src/start_menu.c` +256 lines,
   `menu->poketchPreviousDisplay` is saved and restored), and the field stays alive behind it in 4:3. The
   other users are the naming-screen keyboard, the town map, the Pokédex details
   (`showingDetailsOnTop`), Underground mining and `underground/top_screen.c`, and `overlay062`.
2. **Re-targeting the sub engine to the main engine.** The battle bag and party menus move every
   `BG_LAYER_SUB_n` to `BG_LAYER_MAIN_n`, `PLTTBUF_SUB_*` to `PLTTBUF_MAIN_*`, and engine B toggles to engine
   A. `bg0As2DOr3D` becomes `GX_BG0_AS_2D`, which hides the 3D battle scene while the menu is up.
   `src/battle/battle_display.c` adds `BattleDisplay_HideBattleScene` and `RestoreBattleScene`: it saves
   64 KB of OBJ tiles, the OBJ mapping mode and OBJ palettes, hides the Pokémon, trainer, healthbox and
   terrain sprites, then restores all of them. Commit `73c0ce3da` documents what broke: "HP-panel leakage and
   the black screen on return".
3. **New button-driven menus on top.** The battle command menu, which on the DS is a touch subscreen,
   becomes a text-window menu on main BG1 (`src/battle/battle_subscreen.c`, +862/-537), with a move-info
   pop-up, PP and type badges in their own palette, and descriptions reflowed for the wider window. The
   Pokétch apps get L/R app switching and per-app button controls (`applications/poketch/*/main.c`); the
   Pokédex search is driven by keys.
4. **Skipping touch-only content.** Parts of Rowan's intro that teach the touch screen are commented out
   (`rowan_intro_app.c`), and the network icon is removed.

Touch input is off in `top_only`, so every converted screen has to work with buttons.

#### Invasiveness

| Area | Files | Lines |
| --- | --- | --- |
| Game `src/` + `include/` | 180 | +7,783 / -1,403 |
| of which single-screen UI (battle, battle_sub_menus, applications, start_menu, underground, overlay062, main_menu) | 63 | +3,323 / -1,120 |
| of which widescreen field (overlay005/006/009, map_header_data, map_object, screen_fade, scrcmd, g3d_pipeline, sprite, billboard, scripted_npc_scene, map_patches) | 28 | +2,464 / -49 |
| `res/` (scripts, text, map build rules, patch baselines) | 96 | +1,410 / -193 |
| libntr fork | 47 | +2,871 / -1,005 |
| libntrsystem patches | 3 | +331 |
| whole repository (with tools and docs) | 313 | +11,711 / -1,691 |

Guarding: about 84 of the changed game files contain `SDK_PORT` or `SDK_BUILD_*` guards. About 82
non-`port/` files are edited **unconditionally**, among them `battle_subscreen.c`, `battle_display.c`, the
`battle_sub_menus/*` re-targeting, `pokedex_search.c`, `underground/*` and `rowan_intro_app.c`. The widescreen
field work is mostly guarded; the single-screen UI work mostly is not. That fork's DS ROM no longer matches.
The figures come from a script in the scratchpad (`scripts/guards.py`), which counts guards inside diff hunks
only, so files guarded outside a hunk may be undercounted.

### 1.3 evoluketion/PokemonPurePlatinum `single-screen`

- Commits: 11 over `origin/pc_port` (merge base `becc4fc22`, 2026-10-08). 20 files, +2,096/-12. The core of
  it is `subprojects/packagefiles/libntr_patch/0001-single-screen-composite.patch` (1,249 lines).
  `subprojects/libntr.wrap` pins libntr to `aa45e528…` and applies the patch through `diff_files`. Its
  comment: "Pinned to an exact commit rather than tracking main, because the diff_files patch below is
  authored against this specific tree… if that pin is bumped and this stops applying, meson fails loudly".
- The design is documented in `docs/single_screen_conversion.md` (255 lines, on that branch).
- **Mechanism.** The new layout `SIM_CONFIG_SCREEN_LAYOUT_MAIN_ONLY` (`main_only`) draws engine A opaque and
  then engine B into the same full-window quad. For the engine B pass `DrawEngine` sets the
  `transparentBackdrop` uniform in the G2 fragment shader, so pixels that would show only the backdrop get
  alpha 0, while drawn BG and OBJ pixels stay opaque. A first version used additive blending and broke on a
  non-black backdrop (the cream title-logo border). The result is a 4:3 single screen with no widescreen.
- **Per-scene placement API** in `simulator/sim.h`: `SIM_MainOnlyPlacement` with a destination rect
  (`centerX/Y`, `width/height`; Y is measured upward), a source crop (`srcCenterX/Y`, `srcWidth/Height`),
  `cutout` (the base engine is whichever has `cutout = FALSE`, so the base can switch between engines),
  `defringe` (a shader that fades near-white anti-aliasing mattes, done in the shader so the art and
  `filesys.sha1` stay byte-identical) and `hidden`. The functions are `SIM_SetMainOnlyPlacement(isSub, &p)`,
  `SIM_GetMainOnlyPlacement` and `SIM_ResetMainOnlyPlacements`, and a debug panel (Tab, then Placement) tunes
  them live.
- **Game changes are small.** `src/applications/title_screen.c` (+65) forces the full-screen border layers
  `TITLE_SCREEN_LAYER_LOGO_BG/_2` off under `main_only`. It still calls `GXLayers_EngineBToggleLayers`,
  because `GXLayers_DisableEngineBLayers()` only clears the software copy, and skipping the call left a stale
  layer register on screen. `src/game_opening/ov77_021D25B0.c` (+172) sets a framing for each scene. Lessons
  recorded in the doc: framing changes must be scoped to the frame where the content changes; the montage art
  is 256x512 split across both engines, so stacking them reassembles it; and `Bg_SetOffset` moves one layer
  where moving the whole engine would also move its background.
- **Touch is not remapped** ("only covers the mechanical 'don't crash' case"). Touch screens "need their own
  redesign… touch-overlay shims were explicitly ruled out."
- **3D fixes in the same patch**, applicable to any libntr port:
  - `g3_handler.cpp` and `g3_vertex.glsl` pass the real clip-space W to the GPU, and the vertex attribute
    pointer becomes 4 floats, which gives perspective-correct interpolation. The quad W-normalisation loop
    now runs `numVerts` times instead of a fixed 4.
  - `G3SIM_DrawItems` re-applies each deferred translucent item's cull and depth state, and writes depth only
    when `POLYGON_ATTR` bit 11 is set ("particles … cut holes in each other" otherwise).
  - A `useWBuffer` uniform.
- Other changes: 60 FPS logic-rate helpers (`SIM_Config_prj_IsLogicRateDoubled`, a forced 30 Hz override for
  frame-timed sequences) and a particle-system debug panel.

### 1.4 Comparison, and the template for a BW2 widescreen mod

| | elyro1 | evoluketion | gen4recomp (section 2) |
| --- | --- | --- | --- |
| Goal | widescreen field + full single-screen UI | single-screen composite, 4:3 | widescreen 3D option + layouts |
| 3D widening | game sets the projection `_00 *= 256/W` each field frame; libntr 448-wide FBO + viewport | none | hardware model scales the clip matrix X by 256/W *inside the geometry engine*; no game hook; readbacks (clip matrix, POS_TEST) stay native |
| 2D in the margins | BGs native and centred; registered OBJ ranges drawn 448 wide or tiled; BG2 weather tiled; window masks 448 wide; 4:3 scenes get an ambient-colour fill | n/a | black where no 3D ("a background is a tile map that ends, and a sprite's X is nine bits") |
| Culling | box test follows the widened projection; a camera-frustum chunk cache for scenery | n/a | box test culls against the wide frustum |
| Content fixes | map mesh deltas, neighbour NPCs, scripted exits, a 4:3 lock | n/a | none (accepts whatever the margins show) |
| Code location | libntr fork + libntrsystem patches + about 180 game files | libntr wrap patch + 2 game files | runtime hardware model only |

What a BW2 widescreen mod should take from this:

1. **Hor+ by projection scaling in the field only, opted into per frame.** Use an API in the shape of
   `SIM_SetFieldWidescreen()` that returns the width and flushes queued geometry when the width changes. Keep
   battles and menus 4:3 at first. BW2's battle 3D, including MCSS Pokémon sprites drawn as textured quads,
   could be widened later, but its HUD is 2D OBJ placed for 256 columns.
2. **Keep the 2D layers native and centred; widen only what is designed to tile.** BW2 weather, seasons, fog,
   the HBlank and window effects, and the field sprite effects need the same "tiled" and "wide" OBJ range
   registration and BG tiling hooks.
3. **Expect most of the work in content, not rendering:** map block streaming (BW2's field mapper loads a
   fixed set of map blocks), NPC and object counts and loading radius, scripted walk-offs and walk-ins, and
   unfinished map edges. elyro1's prepared-chunk cache (scenery-only, collision and scripts still from the
   original buffers), the 4:3 lock and hash-guarded delta delivery are directly reusable *designs*.
4. **For pokebw2's matching rule** ("Both ROMs stay byte for byte"), every mod change has to be behind a port
   or mod define, or live in the platform layer or a wrap patch, as evoluketion does. elyro1's unguarded
   UI rewrites would break the matching build. Carrying libntr changes as a pinned `diff_files` patch is the
   right model, because upstream libntr will not take AI-authored patches (section 6).
5. **Single screen.** Do display swap and re-targeting per screen, and port evoluketion's composite mode for
   overlay-style screens. Gen 5 UIs are mostly operable with buttons, which still needs checking against our
   decomp, so less redesign may be needed than for Platinum. The C-Gear, Xtransceiver, Pokédex area map,
   Join Avenue and Medal Box all need a single-screen presentation decision.

---

## 2. Gen4Recomp / "nativeplat" (sohamb117)

### Identification

- The repository is `https://github.com/sohamb117/gen4recomp`: "Pokemon Platinum, Diamond, and Pearl ported to
  Mac/Win/iOS with recomp features!" Its README title is **nativeplat**. It has 509 commits, all by sohamb117,
  from 2026-10-03 to 2026-10-11, roughly 60 to 116 a day. Branches are `main` and `web-ready`.
- Sibling: `https://github.com/sohamb117/pokeweb`, "Pokemon Diamond, Pearl, and Platinum in the browser, with
  recompiled WASM cores and optional cloud saves". It continues gen4recomp's `web-ready` branch and is
  deployed at pokeweb.morisoba.moe.
- Vendored games: `games/platinum` is "wize-logic pokeplatinum-port (AKCore mirror 3357155c)", a **separate
  Platinum PC port** licensed GPL-3.0. `wize-logic/pokeplatinum-port` no longer resolves on GitHub, and
  `AKCore/pokeplatinum-port` exists. `games/diamond` is pret/pokediamond (`5bc4b1a3`), `games/heartgold` is
  pokeheartgold, and there are also emerald, ruby and `gba-common`. `games/ndsrec` holds the Black/White
  static-recompilation project.

### Architecture

- Build pipeline (`docs/BUILDING.md`): `tools/rom_build.sh` builds the matching ROM and its xMAP; then
  `pc/Makefile.wasm` builds `game.wasm` (wasi-sdk 34 clang, `--target=wasm32-wasip1`); then wasm2c (wabt
  1.0.42) and `tools/wasm2c_postprocess.py`, which gives integer division ARM semantics instead of wasm traps,
  produce C for each host; that C becomes a macOS app, a Windows exe (zig cross-compiled) or an iOS app.
- **Why wasm** (`games/platinum/pc/Makefile.wasm` header): "the ELF build identity-maps the DS (main RAM at
  0x02000000 is a host address), which a 64-bit arm64 iOS/macOS process cannot do. Inside wasm linear memory a
  guest address IS the DS address, the C is ILP32, and wasm2c turns the module back into native C."
  - wasm32 is used here as a portable **ILP32 sandbox**. It sidesteps the 64-bit pointer problems the libntr
    port fights: the Traderoom crash fix `(u32)party` → `(uPtr)party`, doubled heap sizes, and the box
    graphics heap ×2 in elyro1.
  - wasm2c gives ahead-of-time native code, which matters on iOS where JIT is not allowed.
- **Guest/host contract** (`core/include/np_guest_abi.h`, ABI v2):
  - The DS memory map sits below `NP_GUEST_C_BASE 0x0B000000` in 256 MB of linear memory.
  - Imports from `np_host`: fibers (one per guest thread), `vblank` (park until the next frame), cartridge
    reads, backup save, RTC, logging, trap, soft reset, the **net datagram API**
    (`net_self/net_send/net_recv`) and the GBA slot.
  - A per-frame `np_frame_desc`: the guest publishes the two 0x00RRGGBB screens, audio ring, save image and
    status; the host writes keys, touch and options (`NP_OPT_RENDER_SCALE 1..4`, `NP_OPT_WIDESCREEN`,
    `NP_OPT_CAMERA_ZOOM/TILT`, `NP_OPT_RULES` bug fixes, instant text, quick save).
  - The core is deterministic: the same ROM, frames and input give the same hash on every host.

### Platform layer: not libntr

- `games/platinum/README.md`: "`pc/hw/` | DS hardware models: 2D, 3D and sound". `pc/hw/README.md`:
  "Everything in this directory is derived from melonDS … GPLv3-or-later. That is what makes this whole
  repository GPL-3.0-or-later." The files are `pc_gpu2d.c` (from `GPU2D_Soft.cpp`), `pc_gpu3d.c`,
  `pc_gpu3d_soft.c` and `pc_spu.c`. They are checked against a real melonDS oracle (`make diff`).
- The **stock NitroSDK libraries** (pret's) run unmodified on top. `pc/src/*` supplies PXI, timers, DMA, the RTC,
  the cartridge, an ARM7 sound-driver model (`pc/arm7snd`), the WM model (`pc_wm.c`), WVR, DWC stubs and an
  OpenGL 3D path (`pc_gpu3d_gl.c`). The game talks to registers, and the hardware model answers. This is the
  opposite of libntr, which re-implements the SDK API at a high level.
- `pc_wm.c` says outright: "libntr's sim7/wmsp_request.c (MIT, cybervisi0n/libntr) was read as a second
  opinion on the WMSP request mapping, no code was taken from it."
- Black/White (`docs/BW_PLAN.md`): "There is no usable Black/White decompilation (squiddonaut/pokeblack …)".
  B/W are statically recompiled. `tools/ndsrec` does discovery and emit, armrec translates ARM and Thumb to C
  (35,871 of 35,871 functions clean for Black), and the result is linked against the same machine model. The
  ARM9i and ARM7i (TWL) sections are ignored: the game runs in NTR mode.
  - **DS Protect:** overlay 230 is self-modifying (DS Protect) and is left opaque. Its callers in the static
    module, `ov10` and `ov20` were **patched to substitute the outcomes a genuine cartridge produces**
    (`pc/patches/<black|white>/…/*.s.patch`; "ov230 itself is never decoded, decrypted or recompiled"). That
    is anti-piracy circumvention by substitution. BW2 has the same library (pokebw2's `lib/dsprot`).
  - The story runs to the credits on both games, link trades and battles between Black and White work, and
    Mystery Gift and Poké Transfer (a Download Play child station) are in progress or done.

### Widescreen and upscaling

- `pc/hw/pc_gpu3d.c` ("Wide rendering, the recomp-style widescreen enhancement"): a wide width W "scales the
  clip matrix's X output column by 256/W … while the viewport (scaled the other way at the VIEWPORT command)
  keeps one world unit the same number of pixels: the centre 256 columns of a wide render are exactly the
  native picture". The scale is applied where vertices and box-test cubes enter the pipeline, "deliberately
  NOT to anything the game can read back". The clip matrix and POS_TEST answer native values, so billboards
  stay aligned with their shadows, and the box test culls against the wide frustum.
- `pc/src/pc_view.c`: "Only the 3D layer can draw outside 256 columns … So the margins carry 3D where the
  screen is showing 3D and black everywhere else." Widescreen is refused alongside the differential trace
  and frame dumps, because it can change a run (objects kept that a native run culls).
- Render scale 1-4x. `docs/BWHGSS_HOOKS.md`: widescreen and render scale are renderer-side with no game hook,
  so they work on the ROM-only B/W cores too. The field camera zoom and tilt do need game hooks.

### Multiplayer: how LAN Union Room is done

- **Guest side: `games/platinum/pc/src/pc_wm.c`** (2,499 lines), "The ARM7 wireless manager, behind PXI tag 10,
  with the radio replaced by datagrams":
  - The game links the stock NitroSDK ARM9 WM library. `pc_wm.c` reimplements the ARM7 WMSP request thread
    record for record: the same `WMStatus` transitions, callback shapes, size bookkeeping and shared-buffer
    offsets, measured field by field for Diamond's older SDK as well.
  - Requests run from the per-VBlank point, never synchronously inside the PXI send.
  - Station identity comes from `np_host_net_self()`. The MAC is 00:09:BF plus 24 bits, written where the
    firmware keeps it.
  - Parents broadcast beacons every `beaconPeriod`, scanning children keep a table of recent parents, and a
    join assigns the lowest free AID.
  - **MP data uses a reliable, ordered stream for each link** (frame-tagged bundles, sequence numbers, a
    cumulative ACK, retransmission). There is no frame barrier: the game's CommSys clocks itself on
    port-send callbacks and sync tokens, so MP cycles that carry nothing are tolerated.
  - A link with no datagram for the MP lifetime (4 s) is torn down with
    `WM_DISCONNECT_REASON_MP_LIFETIME`.
  - `NP_STAT_LINK_ACTIVE` forces 1x speed during a link.
- **Host side: `shell/src/net.c`.** UDP (`"NPN1"` magic, HELLO and HELLO_ACK discovery on the LAN, join by
  `IP:port`, up to 32 peers, 1,460-byte payloads). Relay mode is `server/relay` (Go): `np-relay -listen :2020`,
  rooms of up to 16 stations keyed by a PIN, per-source rate limits, a 15 s idle timeout. Its protocol is in
  `docs/RELAY.md`: JOIN, ROSTER, RDATA, DATA.
- Tests: `tests/link/run_link_tests.py relay_trade` runs a full Union Room trade through the relay with 10%
  loss in lockstep, and there are Black↔White trades and battles.
- Online WFC is **not** implemented. `pc_dwc_connect.c` models "the same unconfigured console": the access
  point search fails, and the game shows its own error.

### Claims check

| Reported claim | Finding |
| --- | --- |
| Compiles DP/Pt from decompiled C to WebAssembly and back to native C via wasm2c | **True** (also HG/SS and the GBA games; B/W instead use static recompilation of the ROM) |
| 4x 3D upscaling | **True**: `NP_OPT_RENDER_SCALE` 1..4 (the wize-logic core also supports internal resolution) |
| Widescreen battle transitions | **Not found.** What exists is a *widescreen 3D* option and a separate *battle screen layout*, switched while `NP_STAT_IN_BATTLE` (FEATURE_PARITY: "Widescreen battle layout: Separate layout while NP_STAT_IN_BATTLE") |
| Save editors | **True**: trainer, party, boxes, bag, Pokédex, events and Wonder Cards, with undo and redo; HG/SS and B/W saves too |
| LAN Union Rooms | **True**: LAN and an internet relay with a PIN; Union Room trades and battles and the Underground; D/P/Pt meet one another |
| Uses libntr? | **No.** It uses melonDS-derived hardware models with the stock SDK; libntr's WMSP mapping was only read |

### Pros and cons against the libntr approach, for our goals

Where the gen4recomp approach (hardware model under the stock SDK, ILP32 through wasm, or static recompilation
for games with no decomp) is stronger:

- Fidelity comes for free from the register-level model. elyro1 had to fix many libntr G2/G3 bugs: depth, W
  buffer, translucency, HBlank and windows.
- 64-bit pointer issues vanish (ILP32), and the run is deterministic, which makes lockstep link tests and replay
  hashes possible.
- Widescreen 3D works with **no game hook**, even on recompiled ROMs.
- iOS works through wasm2c AOT.
- The WM model is faithful and stress-tested, and includes a NAT-friendly relay.

Where it is weaker:

- **Licence:** the hardware models are GPL-3.0 (melonDS), and gen4recomp itself has no licence file. Reusing its
  code would make our port GPL-3 or leave it unlicensed.
- Performance and binary size: software 2D, wasm bounds handling, roughly 34-42 MB native for each core.
- The 2D margins can only be black. The register model has no notion of "this OBJ range tiles", so
  elyro1-style 2D extension needs high-level knowledge that the game layer or an SDK replacement has.
- Mods that change the game still need game C. For static-recompiled games such as their B/W, only data mods
  (IPS or NARC packages) are practical.
- For BW2 we have a decomp, so a source-level port with an SDK replacement is the better base for mods. The
  recompilation route is only a fallback.

**Ideas worth taking as designs, not code:**

- the WM model's reliable stream per link, with no frame barrier;
- the relay with a room PIN;
- `NP_STAT_LINK_ACTIVE` locking fast-forward during a link;
- deterministic lockstep link tests;
- widescreen applied in the geometry engine with readbacks kept native, a cleaner alternative to editing the
  projection matrix in the game;
- the frame-descriptor options for render scale, widescreen, camera zoom and tilt, bug-fix rules and
  instant text.

**Red flags:**

- No top-level licence.
- The development looks agent-driven. pokeweb has a `web/AGENTS.md` that instructs agents, the commit
  cadence is very high, and the worktree "gate" infrastructure suggests parallel agents.
- **pokeweb publicly hosts Red, Blue, Yellow, Gold, Silver, Crystal, FireRed, LeafGreen, Emerald, Diamond,
  Pearl and Platinum ROMs.** `web/deploy/GCP.md`: "The user explicitly selected public hosting of …
  Cartridges are gzip-compressed then AES-256-GCM encrypted … This is distribution obfuscation, not DRM or
  access control: the public manifest deliberately supplies the decryption key and IV."
- Anti-piracy outcome substitution for B/W.

We should not vendor from these projects or associate pokebw2 with them.

---

## 3. Other DS ports and frameworks

| Project | Game(s) | Approach | Status (Oct 2026) | Licence |
| --- | --- | --- | --- | --- |
| cybervisi0n/pokeplatinum `pc_port` | Platinum | pret decomp + **libntr** (HLE NitroSDK replacement, OpenGL 4.2, SDL2, ImGui); Linux, Win64, Switch (slow) | v0.1.12 (2026-10-10), playable start to finish with crashes | no licence file (pret has none); libntr is MIT |
| wize-logic/pokeplatinum-port (mirror AKCore/pokeplatinum-port; vendored in gen4recomp) | Platinum | pret decomp + **melonDS-derived hardware models** under the stock SDK; Linux, Windows, ARM32, Android, 3DS | the original repository has disappeared; mirror present | GPL-3.0-or-later |
| sohamb117/gen4recomp ("nativeplat") | DPPt, HGSS, B/W, RSE | decomps → wasm → wasm2c; B/W static recompilation (ndsrec/armrec) | all ten boot; broad feature parity | none at top level |
| cybervisi0n/pokeheartgold | HGSS | description claims "fork with support for building a PC port" | **0 commits ahead of pret** (476 behind); nothing there | n/a |
| tangosdev/sm64ds-decomp (+ SCOPIC64/sm64ds-coop-port, SahilKDas/64DS-DX) | SM64DS | matching decomp (99.8% of functions), PC port built from the same source using "stand-ins" for unmatched code; binaries at tangos.dev; co-op and Lua forks | PC port downloadable; heavily AI-assisted (README credits AI tooling and sells "tangOS Console") | MIT |
| RetroPortingToolKit/ndsrecomp (mstan) | firmware, SM64DS, MKDS, Metroid Prime Hunters | static ARM recompiler with a thin runtime (memory bus, two-CPU scheduler, hardware models, SDL); experimental WFC (MKDS on **Wiimmfi**), same-machine local wireless, title-owned 21:9 | v0.6.10, "very early pre-alpha" | **PolyForm Noncommercial 1.0.0** |
| mstan/MetroidPrimeHuntersRecomp | MPH | on ndsrecomp; online play experimental | v0.7.6-alpha | MIT |
| AgitationSkeleton/lsw2ds-pc | LEGO Star Wars II | ndsrecomp + a host-level stand-in for DS wireless, master server, 2-4 players online | new (2026-10-17) | GPL-3.0 |
| vs-sr-dev/ndskit (+ pc-moon, pc-nostalgia, pc-lufiacots) | Moon, Nostalgia, Lufia CotS | game-agnostic static recompiler to C++ with a runtime replacing both CPUs, 2D/3D (OpenGL at any scale) and sound; an interpreter for self-decrypting anti-piracy | early; one game per repository | MIT |
| Yokimitsuro/khdays-decomp (cybervisi0n fork) | KH 358/2 Days | complete matching decomp | "not yet playable on PC" (TheGamer, 2026-09-28) | CC0 |
| zeldaret/ph, zeldaret/st | Phantom Hourglass, Spirit Tracks | decomps in progress | no port | — |
| luispolis124/OpenTS3-DS | The Sims 3 DS | decomp + port plan | early | other |
| NSMB DS, Mario Kart DS decomp | — | MKDS decomp about 15.6% (decomp.dev); no NSMB DS decomp found | — | — |

**Shared "DS port" SDK layers:** cybervisi0n's suite, with each library a drop-in for one Nintendo SDK on
DS, Linux, Win64 and NX targets:

- libntr (NitroSDK)
- libntrsystem (NitroSystem)
- libntrwifi (NitroWiFi)
- libntrdwc (NitroDWC)
- libvct (VoiceChat)
- **libtwl and libtwlsystem** (TwlSDK and TwlSystem: "Drop-in replacement for TwlSDK that targets Nintendo
  DS(i), PC")

libntr itself has a DSi beta (`sdk_version_major=5`, `hardware_type=twl_hyb`), which matters for BW2, a TWL
hybrid. The other frameworks are the static recompilers (ndsrecomp, ndskit, gen4recomp's ndsrec/armrec) and
the melonDS-model approach of the wize-logic port. ndsrecomp and ndskit target games with no decomp.

---

## 4. Multiplayer in the Platinum PC-port ecosystem

**Local wireless** (Union Room, wireless trades and battles, Underground, contests, Battle Frontier
multi-player, and so on), in cybervisi0n/libntr:

- `include/simulator/sim_net.h`, `libraries/sim/src/sim_net.cpp` (920 lines), `sim_gui_net.cpp`, and
  `libraries/sim7/src/wm/wmsp_request.c` (libntr's simulated ARM7 WMSP request handler).
- **Transport: ENet (reliable UDP), port 2009** (`SIM_NET_PORT`), up to 16 players, magic `NTR0`.
  - One instance is the **host lobby** (`SIM_Net_StartHost`). The others connect to its IP
    (`SIM_Net_StartClient`; `sim_config.ini` `netSettings.serverIp`, or the ImGui Net panel).
  - The host distributes the player list (IP, user name, fake DS MAC, MP state, GGID/TGID, user game info),
    and peers connect to one another as a **mesh**.
- **WM mapping:**
  - `WMSP_StartParent` → `SIM_Net_SetGameInfo`, `SetMPState(PARENT)`.
  - Scans → `SIM_Net_Scan` (the parents in the player list stand in for beacons).
  - Connect → `SIM_Net_JoinMP` (a JOIN_REQ/RESP packet "replaces Authentication+Association").
  - MP send → `SIM_Net_SendMPData` (a reliable packet to each peer whose AID is in the bitmap).
  - Disconnect packets.
- There is no relay or NAT traversal. Internet play needs port forwarding or a VPN; press coverage mentions
  Hamachi.
- Evidence it is used: `src/applications/trade_room/application.c`'s 64-bit fix (`35067868f` "Traderoom: Fix
  crash"), and the press reports of Union Room and Underground over LAN.

**Nintendo WFC** (GTS, Wi-Fi Club, Wi-Fi Plaza, Battle Videos, Mystery Gift over Wi-Fi):

- `subprojects/libntrdwc.wrap` and `libntrwifi.wrap`, plus `lib/ppwlobby` (the Wi-Fi Plaza lobby; release
  v0.0.2 "Add support for PPWLobby"), `lib/gds` (the Global Data Station client, carved from overlay 61), and
  `subprojects/libvct.wrap` (Wi-Fi Club voice chat).
- **libntrwifi** is a decompilation of NitroWiFi, with Ghidra-style names (`undefined4`, `iVar1`).
  - `cps/src/network.c` runs the DS's own TCP/IP stack over host sockets, including its own DNS resolver
    (`resolve_sub` sends DNS queries to the configured DNS IP on UDP 53).
  - A commented line shows `//  aDnsIP = 0xD42B3EB2; //178.62.43.212`, the address of Pokémon Classic
    Network's DNS.
  - The DNS comes from the WFC settings stored in `firmware.bin`, through libntr's NVRAM emulation
    (`libraries/sim7/src/spi/nvram/nvram_instruction.c`, `NVRAM_FILE_PATH "firmware.bin"`).
  - It has its own `ssl` module (RC4, MD5, SHA-1, big integer).
- **libntrdwc** is a decompilation of NitroDWC plus the GameSpy SDK, with GameSpy copyright e-mails left in
  the headers.
  - Auto-Connect is stubbed (`ac/src/connect.c`: `#ifdef SDK_PORT //PCPORT_TODO return 8;`).
  - The NAS URL is still `https://nas.nintendowifi.net/ac` (`auth/include/dwc_serverurl.h`), so the revival
    server is chosen by DNS redirection, which is how real DSes use Wiimmfi and PKMN Classic.
- **Press reports** (GBAtemp, "Pokemon Platinum PC port gets a surprise release"; quoted through search
  results because the page returned 403):
  - Local multiplayer over LAN or the internet: press Tab, then NET in ImGui; port 2009; port-forward or use
    a VPN.
  - "By default, the Poke Classic DNS is used", and it can be changed in the game's Wi-Fi settings.
  - Wi-Fi Plaza and Wi-Fi Club included; the GTS partially supported with bugs; no voice chat; a reviewer
    traded and battled a real DS over the Wi-Fi Club, but found it finicky.
  - Not verified first hand; the release notes do say "Fix wifi mystery gift" (v0.1.0) and "Fixed WiFi icon
    appearing everywhere" (v0.1.11).
- Not done: Pal Park and the dual slot (issue #19), Chatot's microphone (#16).

**The mods:** elyro1 removes the network icon (`09d5772e5`), and the single-screen work has not been tested
with link play. evoluketion makes no network changes. gen4recomp has a LAN and relay WM model and no WFC
(section 2).

**What it means for BW2 multiplayer.** Local wireless has to cover four things:

- Union Room;
- the **C-Gear's always-on beacons** (gen4recomp notes "the C-Gear's beacons and the Union Room on
  pc_wm.c");
- Entralink;
- **Download Play / MB** (Poké Transfer Lab, Memory Link).

BW2 also uses the **cartridge infrared** chip (Feeling Check, IR trades and battles); gen4recomp modelled the
HG/SS IR chip for the Pokéwalker.

For online, PKMN Classic Network and Wiimmfi-style revival servers through DNS redirection and a WFC stack
(libntrdwc/libntrwifi, or TWL equivalents) are the proven path. libntr's ENet mesh would benefit from a
gen4recomp-style relay with a room PIN for NAT.

---

## 5. Randomizers

- **No built-in randomizer** exists in origin, elyro1 or evoluketion (`git grep -il randomi` finds only the
  game's own Battle Arcade and comm uses). None is reported in the press either.
- **UPR on the source ROM.** libntr's first-run extractor (`libraries/sim/src/sim_RomExtractor.cpp` →
  `fireflower/nds-extract.cpp`) dumps the header, ARM9, ARM7, overlay tables, FNT, FAT and **every file of
  any `.nds` with no hash or game-code check**. The port then reads the NARCs from the extracted tree. So:
  - **Carried over** (data in NARCs or scripts): wild encounters, trainer parties, base stats, types,
    abilities and TM compatibility (personal), learnsets, evolutions, item data including the TM→move mapping
    (`res/items/data/tm03.json` in the decomp), static, gift and field items in `scr_seq.narc`, and text.
  - **Not carried over** (compiled into the port's code): the starters (`#define STARTER_OPTION_0
    SPECIES_TURTWIG` in `src/choose_starter/choose_starter_app.c`), mart inventories
    (`src/overlay007/shop_menu.c`), and any UPR ARM9 or overlay code patch (type chart, move tutors, and so
    on). The port runs code built from source, so UPR's binary patches to `arm9.bin` and the overlays do
    nothing.
  - **elyro1's mod rejects** a randomized ROM whose `scr_seq.narc`, map or text members differ from retail.
    `map_patches.cpp` checks the `before` SHA-256 of each member and stops with "Re-extract a clean US
    Platinum ROM".
  - This has not been tested by anyone, as far as we found. It is inferred from the code.
- **gen4recomp** has "Mods and custom carts": `.zip` content packages for each game, applied at boot as files
  or NARC members (B/W use `pc_bw_romview.c`). It has no randomizer, but its package system would host one.
- **For BW2:** a randomizer that works at the data layer is the robust design. That means either a built-in
  generator that writes NARC members, or delta packages applied after extraction. Code-resident tables
  (starters, marts, tutors, the type chart, wherever pokebw2 compiles them into C) need port-side hooks or a
  data-driven refactor behind a define. UPR-ZX and UPR-FVX already support BW2 ROMs, so a "randomize the ROM
  first, then extract" flow would work for NARC-resident data, provided our mod deltas tolerate non-retail
  members.

---

## 6. Community and legal norms

- **The ROM-required asset model.** cybervisi0n's README says players "MUST dump and provide your own US ROM
  … (CPUE01)", and every release note repeats it. Since v0.1.8 libntr extracts the ROM in the app; before
  that, users extracted it by hand.
  - **Forking Guidelines** (README): "do not publish any releases containing the compiled assets alongside PC
    port binaries. If you publish releases, you are responsible for also complying with the MIT license on
    libntr by including the copyright and disclaimer. cybervisi0n is not affiliated with any forks."
  - elyro1 follows this: modified assets ship only as SHA-guarded member deltas applied to the user's
    extraction.
  - evoluketion keeps the art byte-identical (de-fringe in a shader) so `filesys.sha1` still matches.
  - gen4recomp: "nativeplat ships no game data", with SHA-1 checks on import. pokeweb nevertheless hosts 12
    ROMs publicly (section 2).
- **Licences:**

  | Project | Licence |
  | --- | --- |
  | libntr | MIT (© 2026 cybervisi0n) |
  | libntrsystem, libntrdwc, libntrwifi, libvct, cybervisi0n/pokeplatinum | GitHub reports no licence |
  | pret/pokeplatinum | no licence file |
  | elyro1 and evoluketion forks | no licence added |
  | wize-logic port (vendored in gen4recomp) | GPL-3.0-or-later (melonDS-derived) |
  | gen4recomp | none at top level |
  | ndsrecomp | PolyForm Noncommercial |
  | ndskit | MIT |
  | sm64ds-decomp | MIT |

- **Provenance, for the libntr agent:**
  - libntrwifi and libntrdwc read as Ghidra decompilations of Nintendo and GameSpy SDK binaries, with the
    original GameSpy headers.
  - libntr's ARM7 NVRAM code follows NitroSDK's own structure, though no Nintendo copyright headers remain.
  - The MIT grant can only cover cybervisi0n's own work.
- **The AI-contribution ban:**
  - cybervisi0n/pokeplatinum README: "Generative AI/LLM shortcuts were **not** used in the creation of this
    project and the libntr libraries. **Pull requests or code suggestions using AI will be rejected.**"
  - The same fork's `AGENTS.md`, added in `becc4fc22` on 2026-10-08, and libntr's `AGENTS.md`: "This is not an
    AI-driven project. We do not accept pull requests created by AI."
  - Consequences for us: no PRs, issues with code suggestions, or patches to cybervisi0n's repositories from
    AI-assisted work. Consume libntr under MIT through a fork or a pinned wrap patch (evoluketion's model),
    keep the licence notice, and do not present our changes as contributions to upstream.
  - libntrsystem, libntrdwc and libntrwifi publish no policy, but they are the same author; assume the same.
  - The elyro1 and evoluketion forks state no AI policy. elyro1's fork predates the `AGENTS.md`, since its
    merge base is 2026-10-02.
- **Other projects' norms:**
  - tangosdev/sm64ds-decomp openly uses AI. Its README speaks to AI assistants directly, which is not
    relevant here.
  - pret's CONTRIBUTING has no AI clause.
  - The static recompilers keep generated code out of git because it "contain[s] code derived from
    user-provided Nintendo dumps" (ndsrecomp) and gate on exact ROM hashes.

---

## 7. Recommendations for pokebw2

1. **Widescreen:**
   - Follow elyro1's split: a per-frame field opt-in that scales the projection (or, more cleanly,
     gen4recomp-style scaling inside our G3 layer with readbacks kept native).
   - In the platform layer: a 3D buffer at the maximum width, a viewport for each frame, 2D layers kept native
     and centred, and opt-in tiled and wide OBJ ranges.
   - Budget most of the effort for BW2-specific content fixes: map block streaming, NPC load radius and caps,
     scripted walk-ins and walk-offs, map edges, seasonal and weather overlays.
   - Ship content changes as hash-guarded deltas over the user's extraction.
2. **Keep the matching build intact.** Put every mod change behind a define, or in the platform layer or a
   pinned patch. Don't follow elyro1's unguarded UI rewrites.
3. **Single screen:** a display swap per screen plus an evoluketion-style composite for overlay screens. Check
   which BW2 bottom-screen apps already support buttons before redesigning any.
4. **Multiplayer:**
   - libntr's ENet WM path is the starting point. Add a relay with a room PIN, and model C-Gear beacons, MB
     (Download Play) and IR.
   - For WFC, use libntrdwc/libntrwifi (or the TWL equivalents), with DNS pointing at a revival server.
5. **Randomizer:** build it on the data and NARC layer, and make our deltas tolerate non-retail members.
6. **Legal hygiene:**
   - Ship no assets; require the user's ROM.
   - Reuse no GPL melonDS-derived code (wize-logic, gen4recomp) unless we accept GPL-3.
   - Use nothing from pokeweb or gen4recomp beyond ideas.
   - Contribute nothing AI-authored to cybervisi0n's repositories.
   - BW2's DS Protect (`lib/dsprot`) needs a deliberate port-side decision. gen4recomp's "substitute the
     genuine outcome" approach is legally sensitive.

## Sources

- Repositories: github.com/cybervisi0n/pokeplatinum, /cybervisi0n/libntr, /cybervisi0n/libntrdwc,
  /cybervisi0n/libntrwifi, /elyro1/pokeplatinum (`pc_single_screen_mod`), /elyro1/libntr
  (`lib_single_screen_mod`), /evoluketion/PokemonPurePlatinum (`single-screen`), /sohamb117/gen4recomp,
  /sohamb117/pokeweb, /RetroPortingToolKit/ndsrecomp, /mstan/MetroidPrimeHuntersRecomp, /vs-sr-dev/ndskit,
  /AgitationSkeleton/lsw2ds-pc, /tangosdev/sm64ds-decomp, /cybervisi0n/khdays-decomp, /cybervisi0n/pokeheartgold.
- cybervisi0n/pokeplatinum releases v0.0.1 to v0.1.12 (through `gh api`).
- [GBAtemp: Pokemon Platinum PC port gets a surprise release](https://gbatemp.net/posts/10863352/) (403 when
  fetched; content seen through search summaries only).
- [TheGamer: Kingdom Hearts 358/2 Days PC decompilation](https://www.thegamer.com/kingdom-hearts-358-2-days-nears-playable-pc-release/).
- [decomp.dev: Mario Kart DS](https://decomp.dev/RonyxDumb/mkds); [zeldaret/ph](https://github.com/zeldaret/ph);
  [zeldaret/st](https://github.com/zeldaret/st); [readonlymemo decomp list](https://readonlymemo.com/decompilation-projects-and-n64-recompiled-list/).
