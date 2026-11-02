<!-- Records the proposed design of Frigate's widescreen top screen (PLAN.md §5.2): a Hor+ scale inside its own
geometry engine (port/gfx/ge/ge.c) with the clip and vector matrices and the position and vector tests kept native,
2D native and centred, every enhancement in a declared class, and the rules that keep widescreen possible until it is
built. -->
# 0030: Widescreen by an engine-side Hor+ scale with native readbacks

- **Status:** proposed
- **Date:** 2026-11-01
- **Plan:** §5.2 (widescreen), §4.6 (renderers), §6 (verification), §7 phase 6 (whose exit includes widescreen field
  play), D9
- **Upstream:** `9496bc869b`, as Frigate's `main` has it
- **Note (2026-11-01):** proposed for the owner's decision by a maintainer session, from a read-only scan of `main`;
  no widescreen code exists yet. PLAN.md §5.2 names the game's projection first and this design as its alternative;
  the change that accepts this ADR rewrites §5.2 and [MODDING.md](../MODDING.md)'s widescreen item to match. Rules 1
  to 10 of the [rules](#rules-that-keep-widescreen-possible) hold for every change now, since each keeps phase 6's
  exit reachable under either of §5.2's options, and
  [port/CLAUDE.md](../../CLAUDE.md#keeping-widescreen-possible-planmd-52) points every session to them; rules 11 to 13
  come with this design and bind only once it is accepted.

## Context

Widescreen here is Hor+: the top screen keeps its height and vertical field of view and shows more on the left and
right. PLAN.md §5.2 widens the 3D either in the game's projection, as elyro1's Platinum mod scales the projection's
`_00` term by 256/W ([RESEARCH.md](../RESEARCH.md#the-mods-built-on-it)), or in Frigate's geometry engine, as
Gen4Recomp scales clip-space X where vertices and box-test cubes enter its pipeline, "deliberately NOT to anything the
game can read back"
([platinum-mods-alternatives.md](../research/platinum-mods-alternatives.md#widescreen-and-upscaling)). The field's
overlays (12, 36), the battle's (167, 168) and the main module are decompiled whole, so the facts below rest on C:

- **The game ties 2D to 3D through its own projection.** `NNS_G3dWorldPosToScrPos` and `NNS_G3dScrPosToWorldLine`
  (`lib/nnsys/src/g3d/util.c:40-83`, `88-148`) read `NNS_G3dGlb.projMtx` and a viewport packed 8 bits an edge
  (`lib/nnsys/src/g3d/glbstate.c:40`, `249-260`). 18 calls in 7 files use them, among them the talk balloons' tails
  and quads (`src/ov036/talkmsgwin.c:573-574`, `601`, `705-720`), the save animation
  (`src/ov036/field_saveanime.c:126`) and battle effects' sprites (`src/ov168/btlv_effvm.c:2035`); MCSS's sprites
  (`src/system/mcss.c:378-382`), the battle effects' 4:3 space (`btlv_effvm.c:4051-4065`) and the weather's scroll
  (`src/ov036/weather_task.c:555-557`) do the same arithmetic themselves.
- **The SDK reads the engine back.** It reads the position matrix as CLIPMTX under an identity projection that it
  loads first (`util.c:10-28`, `lib/nnsys/src/g3d/sbc.c:593-617`; Frigate's `port_ge_clip_matrix`,
  `port/gfx/ge/ge.c:1432-1437`), and the vector matrix as it is (`util.c:23`, `sbc.c:821`). POS_TEST would run the
  vertices' `Transform` (`ge.c:455-462`, `1167`), but nothing in the game or the SDK issues POS_TEST or VEC_TEST. The
  SDK's flushes multiply the camera into the projection matrix (`glbstate.c:74-100`), so the engine's projection is
  often more than a projection.
- **Every box test decides only what is drawn.** The game box-tests in two places: GFL's
  `GFL_G3DSysDrawObjBBoxCull` (`src/gfl/g3d_system.c:876-937`), whose answer its 18 calls in 13 files ignore, and the
  field's chunks (`src/ov012/field_g3d_map.c:441-475`), whose count of chunks drawn only splits the next frame's
  chunks between the field's two render passes (`src/ov036/field_g3d_mapper.c:183-184`, `978-1000`). Frigate's
  `BoxTest` runs the polygons' `Transform`, `Clip` and `ToScreen` (`ge.c:906-945`). NPC billboards are never culled
  (`src/gfl/billboard.c:401-552`). Polygon RAM's counts are read in two places: the dual-3D code reads the vertex
  count (`src/gfl/double3Ddisp.c:77`), and the battle reads the polygon count twice and drops it
  (`src/ov168/btlv_effect.c:384-388`). The game's 23 `G3_ViewPort` calls all set the full screen, (0, 0, 255, 191).
- **Filling the margins with content changes the game.** `GFL_RandomLC` (`src/system/gfl_use.c:222-224`) draws for
  wild encounters (`src/ov036/field_encount.c:251`, `334`), NPCs' random moves
  (`src/ov036/field_actor_movecode.c:1283`, `1289`) and weather particles (41 calls in overlays 75 to 77), and field
  actors read terrain through the mapper's loaded chunks (`src/ov012/field_actor_tool.c:647-678`).
- **Frigate's renderers:** the 3D renderer's width is one constant (`port/gfx/ref/ds3d.h:23`); the engine's viewport
  transform is the DS's, X in 9 bits (`ge.c:694-728`, `port/gfx/ge/ge.h:40`); the 2D reference, display capture, the
  screens, the window and the test tools are 256 wide by design (`port/gfx/ref/ds2d.h:66`, `port/gfx/capture.c:97`,
  `port/gfx/screens.h:12-13`, `port/app/present.c:81`, `99-100`, `port/app/run_dir.c:16`,
  `port/tools/oracle/compare.py:52-53`).

## Decision

- **Widen in the geometry engine, never through the game.** For a top screen W pixels wide, `ge.c` clips polygons
  and box tests (`Clip`, `ge.c:656-681`) against left and right planes moved from ±w to ±w·W/256, and gives each
  vertex its native screen X plus the margin, (W − 256) / 2, computed with the native transform's arithmetic and X
  kept past 9 bits. This is the Hor+ scale of clip-space X by 256/W where vertices and box-test corners enter
  clipping (`ge.c:892`, `920`), with the viewport mapped across W columns, kept bit for bit native inside the native
  view volume. W rides on each `PortGeList`, as `swap_param` does (`ge.h:59-68`), and in the frame cache's key
  (`port/gfx/ge/ge_hal.c:27-44`).
- **The matrix and test readbacks stay native.** `Transform`, `ge->proj` and `ge->clip` never see W, so CLIPMTX, the
  vector matrix, POS_TEST and VEC_TEST answer as on a DS. The other readbacks change with the wide view: box tests
  answer for it, so culling widens with it, since every box test the game makes only decides what is drawn; and
  polygon RAM's counts grow, since the wide view keeps more polygons, while the game reads them only in the dual-3D
  scenes, which stay 4:3, and in the battle, which drops them. A box test found to feed logic gets the native answer
  through a hook at its call site, or its scene stays 4:3.
- **The game's projection, aspect and viewport are left alone,** so `NNS_G3dGlb` and every 3D-to-2D placement above
  keep their native values and stay aligned with the centred 2D.
- **2D stays native, 256 wide and centred.** A HUD element that belongs at an edge gets an anchor (left, centre or
  right) where it is placed: a ClAct cell actor where its OAM enters ClAct (`src/gfl/clact.c:1278-1286`), a BMP window
  where it is created (`src/gfl/bmp_win.c:48`, `67`), through hooks that compile away in the ROM build; the wireless
  icon writes OAM itself (`src/gfl/wm_icon.c:88-97`) and needs one of its own. A margin pass beside `ds2d.c` fills
  each line's margins (the 3D, else the backdrop, with the line's blending and master brightness) until the GPU
  compositor (PLAN.md §4.6) does.
- **What stays 4:3:** the bottom screen and its touch panel; 3D on the bottom screen (19 files route engine A there;
  widening follows POWCNT1, `port/hal/gx/gx_display.c:16`); scenes that capture or draw 3D on both screens, 256 wide
  by hardware (the title, the encounter effects, the Hall of Fame, the 3D demos: `src/gfl/double3Ddisp.c:92-112`,
  `port/gfx/capture.c:32-40`); the trainers' VS cut-ins, an orthographic camera ±4 by ±3
  (`src/ov036/fld3d_ci.c:351`); 2D-only screens, pillarboxed; and battles, until their stage is seen to fill a wide
  view. The field comes first, opted in each frame by a hook at its render (`src/ov036/fieldmap.c:1351`).
- **Every enhancement declares its class** in the change that adds it:

  | Class | It may change | Equal with it on | Tests |
  | --- | --- | --- | --- |
  | Presentation-only | only what is shown beyond the native frames: the margins, the window's layout, anchored HUD | the logic digests, every readback, the native 256x192 frames | every test runs with it |
  | Visual-fix-breaks-compare | what the game draws and the hardware state it leaves, never its logic: the wide view and its culling, widened ClAct surfaces, a wider weather background | the logic digests, every readback that feeds logic | frame and hardware comparisons refuse it; the invariants below hold |
  | Gameplay-changing | the game's state or random numbers: wider weather spawning, a neighbouring zone's NPCs loaded early, chunks with terrain | nothing | opt-in, off by default, named in the input file and the save's sidecar, never in an oracle run |

  The engine-side scale itself is visual-fix-breaks-compare: it culls less, so the game draws more.
- **The invariants the tests hold:**
  - *Centre crop equals native.* From one command stream that overflows neither list's polygon or vertex RAM, every
    polygon that the native frame draws without clipping it at its left or right plane is in the wide list, moved by
    the margin and otherwise the same, bit for bit (a unit test of random scenes, as `g3d_ds3d_bands` holds bands to
    one thread). On a route, the wide frame's centre 256 columns equal the native frame's but where a polygon crosses
    the native left or right edge (clipping makes other vertices) or the field's two chunk passes split differently,
    pixels held to the oracle's 3D tolerance. A frame on which either list overflows polygon or vertex RAM
    (`ge.h:30-31`) is left out, since the two lists then drop different polygons, anywhere in the centre: wide mode's
    larger RAM keeps polygons that the native list drops.
  - *Logic digests equal with it on.* A route run with any enhancement but a gameplay-changing one gives the same
    logic digests, frame for frame, as with it off: GFL's random state, the saves and the sound trace, which the
    oracle's tests compare today, and the game-state digests that verification v2
    ([ADR 0031](README.md#numbering-and-files), reserved) adds. Draw-side state is left out: `NNS_G3dGlb`, the
    projection, camera and matrices that the oracle compares too (the engine-side scale leaves it native, but a visual
    fix may change it), GFL's dispatched polygons (`src/gfl/g3d_system.c:940`, read nowhere) and the mapper's chunks
    drawn.
  - *Readbacks native.* One command stream gives the same CLIPMTX, vector matrix, POS_TEST and VEC_TEST with W at 256
    and wider.
  - Replays, recordings, the oracle's run directory and frame hashes run native, and the program refuses any
    enhancement that isn't presentation-only beside them, except in that enhancement's own tests.

### Rules that keep widescreen possible

Rules 1 to 10 hold for every change now: each keeps phase 6's exit reachable under either of PLAN.md §5.2's options.

1. **The DS-exact path stays exact, and the source of every tested frame.** `ds2d.c` and `capture.c` get no width or
   margin logic; widescreen is new code beside them.
2. **The 3D frame is read through a width-aware accessor,** never with a literal 256 or a stride assumed to be 256
   (as `port/tests/g3d/test_g3d_hal.c:232` does now), and its 256-pixel centre reaches `ds2d` and capture from one
   place (`port/gfx/screens.c:47`, `63`).
3. **The geometry engine keeps its shape:** the viewport transform in `ToScreen` alone, clip space until then, no new
   8- or 9-bit narrowing of screen X or Y, list-level state on `PortGeList`, a frame cache key that covers every input
   to the picture, and nothing that scales `ge->proj` or `ge->clip`. A GPU 3D renderer takes positions finer than
   `PortGeVertex`'s whole pixels (`ge.h:36-44`).
4. **The game's projection, aspect, viewport and ClAct surfaces don't change in passing:** the 25
   `FX32_CONST(4.0 / 3.0)` sites, the battle camera's 1.35 (`src/ov168/btlv_camera.c:50-53`), the 23 `G3_ViewPort`
   calls, `NNS_G3dGlb.prmViewPort` and `sSurfaceSetups` (`src/gfl/clact.c:311-314`).
5. **Visibility never feeds game logic.** No port code skips or changes a game update because of a box test, ClAct's
   culling or being on screen, not even to save time on off-screen actors.
6. **Presentation never draws from the game's random numbers;** extra visual content is render-side or has a host
   generator of its own.
7. **Presentation sees each screen.** POWCNT1's routing, DISPCNT's BG0 mode and capture stay visible each frame, not
   flattened into one stacked picture first, and touch maps through the bottom screen's own rectangle
   (`port/app/present.c:131-141`).
8. **One OBJ path.** Frigate's own sprites go through ClAct, where anchors can be recorded; nothing new writes
   hardware OAM.
9. **Capture stays DS-exact in VRAM;** a wide capture is a host texture of its own (PLAN.md §4.6).
10. **Game changes for widescreen are hooks** of the kinds
    [port/CLAUDE.md](../../CLAUDE.md#changes-outside-port-planmd-41-48) allows, each with its row in
    [game-changes.md](../game-changes.md), both ROMs unchanged.

Rules 11 to 13 come with this design and bind only once it is accepted: PLAN.md §5.2 extends `MAP_CONFIGS` and raises
the NPC limits instead, and [MODDING.md](../MODDING.md) lists `PORT_SCREEN_WIDTH` among what widescreen changes.

11. **The native screens keep their meaning.** `PORT_SCREEN_WIDTH` and `port_screens_pixels`, "192 rows of 256"
    (`port/gfx/screens.h:12-13`, `26-28`), stay; a wide top screen is a buffer beside them.
12. **The mapper and the field actors stay the authority** for terrain, collision and scripts. Wide-view content is a
    scenery-only path sized by host hooks like `port_heap_size` (`src/gfl/heapsys.c:14-17`), never by edits to
    `MAP_CONFIGS` (`src/ov036/fieldmap.c:205-235`) or the 64 actors.
13. **Every enhancement declares its class** in the change that adds it.

## Alternatives

- **The game's projection** (PLAN.md §5.2's first option). One function widens most scenes:
  `GFL_G3DSysMtxSetProjection` (`src/gfl/g3d_system.c:218`), which every GFL camera flush calls
  (`src/gfl/g3d_camera.c:33-36`). 26 of the 29 game files that write 4:3 (25 `FX32_CONST(4.0 / 3.0)` in 23 files,
  `FX32_ONE * 4 / 3` in 5, `1.333` in one) pass it through GFL's camera or projection calls, as the battle camera
  passes its 1.35; three set their projection directly (`src/system/screentex.c:128`,
  `src/ov148/encounter_effect_capture.c:57`, `src/ov310/circle_graph.c:317-321`). Like this design
  (`fieldmap.c:1351`), it needs a hook that opts each scene in. A wider projection moves each placement of 2D by 3D
  above off the centred 2D unless it is worked out again, in the two SDK helpers and the three places that do the
  arithmetic themselves; the SDK's viewport can't simply widen, since it holds 8 bits an edge (`glbstate.c:40`,
  `249-260`). It changes `NNS_G3dGlb`, so the oracle's comparison of it (`port/tools/oracle/oracle.py:135-137`) can't
  run with widescreen on. Its one visible gain, weather background quads that widen with the projection
  (`src/ov036/field_3dbg.c:175-208`), stretched, is a hook here.
- **Scaling the projection matrix inside the engine,** as the game loads it. A scaled clip matrix changes
  `Transform`'s rounded 20.12 results (`ge.c:455-462`), so the centre would no longer be bit for bit native, which the
  centre-crop invariant needs. It must also tell identity loads apart: the SDK reads CLIPMTX only under an identity
  projection that it loads first (`util.c:14-16`; `sbc.c:594`, `688`, `768-770`, `1030-1032`), and nothing issues
  POS_TEST or VEC_TEST, so the scale is read back only if identity loads are scaled too, while leaving them unscaled
  stretches whatever is drawn under an identity projection.
- **Anamorphic:** TWiLight Menu++'s BW2 code writes 16:10 over the field camera's 4:3 and the 3DS stretches 256x192
  to 384x240; melonDS's aspect modes only stretch
  ([bw2-mod-ecosystem.md §3.2](../research/bw2-mod-ecosystem.md#32-existing-widescreen-hacks-and-tools)). It leaves
  the logic alone, but stretches every 2D layer and the battles and adds no pixels. Stretching can stay a window
  option.

## Consequences

- No game edit widens the 3D: `NNS_G3dGlb`, the matrix and test readbacks, the 3D-to-2D placements and the oracle's
  state comparison stay valid with widescreen on, and culling widens by itself.
- The work is in `port/`: a 3D target for the widest W with an active width and stride (`ds3d.h:23`,
  `ge_hal.c:23`), screen X past 9 bits, more polygon RAM in wide mode (`ge.h:30-31`), the margin pass, a wide top
  screen beside the native one, and a rectangle for each screen in the window, touch included.
- Hooks and content remain, each with its class: the field's opt-in, the weather backgrounds' width, HUD anchors,
  scenery-only chunks, and what the margins reveal, such as voids at map edges and NPCs that appear at zone borders,
  where a seamless zone change reloads every actor (`src/ov036/fieldmap.c:1130-1132`). Either design needs this
  content; it is most of the work in elyro1's mod, which changed 180 game files
  ([RESEARCH.md](../RESEARCH.md#the-mods-built-on-it)).
- The CPU 3D frame grows with its width, by about 1.33 at 16:9 and 1.75 at 21:9, against routes whose 95th
  percentile is within 2.1 ms on an M2 ([ARCHITECTURE.md](../ARCHITECTURE.md#feature-map), "Frame times").
- Centred-2D widescreen in the field needs only the engine change, the margin pass and per-screen presentation, so it
  need not wait for the GPU 2D compositor that PLAN.md §5.2 puts first (an inference; the order is the owner's call).
- Revisit if a box test turns out to feed logic where no native answer can be given, if the logic digests differ with
  W above 256, or when the GPU 3D renderer (PLAN.md §4.6) changes where clip space is kept.
