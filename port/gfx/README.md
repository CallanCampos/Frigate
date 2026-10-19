# port/gfx/

The renderers: what the DS's display hardware makes of the state the game leaves in its registers, palettes, OAM and
VRAM. The game programs that hardware directly, through NitroSDK's GX functions and NitroSystem's G2D and G3D, which
compile unchanged; the platform layer ([../hal](../hal/README.md)) keeps the state in host memory, and the code here
draws from it, line by line for the 2D engines and frame by frame for 3D, as the hardware does. Each renderer is the
reference that a later GPU renderer (PLAN.md §4.6's `gfx/gpu/`, not built yet) will be compared with.

Nothing here includes the decomp's headers: the files use plain C types and the platform layer's plain-C interfaces
(`hal/gx/vram.h`, `hal/hw/hw.h`, `hal/os/port_os.h`, `hal/os/frame_time.h`, `hal/host/host.h`), and cite GBATEK, the DS
hardware reference, for each fact, or melonDS's measured behaviour where GBATEK is silent. The build compiles every C
file here into the `port_gfx` target ([../CMakeLists.txt](../CMakeLists.txt)); the tests build the renderers on their
own, with their own flags ([../cmake/gfx_tests.cmake](../cmake/gfx_tests.cmake),
[../cmake/g3d.cmake](../cmake/g3d.cmake)), and need no ROM.

| File | What it does | Game functionality that depends on it | Tests |
| --- | --- | --- | --- |
| [screens.c](screens.c), [screens.h](screens.h) | The scanline hook: at the start of each visible line, draws that line of both 2D engines from the state the game left, routes each to the screen POWCNT1 gives it, and feeds engine A's line to display capture; before line 0 it has the 3D frame drawn | Every screen of the game, with its raster effects (the HBlank tasks of `src/system/main.c` change registers between lines) | `integrate_screens`, `os_scanline_hook`, every replay and oracle test |
| [ref/ds2d.c](ref/ds2d.c), [ref/ds2d.h](ref/ds2d.h) | The CPU reference renderer of the two 2D engines, one line at a time: text, affine, extended and bitmap backgrounds, sprites, windows, colour effects, mosaic, master brightness | All 2D: backgrounds and text boxes (`src/gfl/bg_sys.c`), sprites (`src/gfl/clact.c`), fades and wipes | `gfx_ds2d`, `gfx_ds2d_sanitized`, `gfx_ds2d_timing` (with `-DPORT_GFX_TIMING_GATE=ON`) |
| [ref/ds3d.c](ref/ds3d.c), [ref/ds3d.h](ref/ds3d.h) | The CPU reference renderer of the 3D engine: the rear plane, opaque and translucent polygons in the hardware's order, textures of every format, toon, highlight and shadow, edge marking, fog and anti-aliasing, into the picture engine A shows as BG0; drawn in bands of lines at once on the workers, the same picture however many ([ADR 0016](../docs/adr/0016-3d-frame-in-bands-on-worker-threads.md)) | Every 3D scene: the title screen, the field (`src/ov036`), battles (`src/ov168`), Pokémon and trainer sprites (`src/system/mcss.c`), through NitroSystem G3D (`lib/nnsys/src/g3d`) | `g3d_ds3d`, `g3d_ds3d_sanitized` (each scene in one band and in three), `g3d_ds3d_bands`, `g3d_ds3d_bands_sanitized` (random scenes in 2 to 8 bands against one), `g3d_timing` (with `-DPORT_G3D_TIMING_GATE=ON`) |
| [workers.c](workers.c), [workers.h](workers.h) | The renderers' worker threads: a job in parts, part 0 on the game's thread and the others on up to 7 workers at once (`--render-threads`, the host's performance cores by default), and a sync of the parts inside a job; the threads and their signals are [../hal/host](../hal/host/host.h)'s. Only renderer code runs on them | The 3D frame of every 3D scene, drawn in bands ([ADR 0016](../docs/adr/0016-3d-frame-in-bands-on-worker-threads.md)) | `g3d_ds3d_bands`, `g3d_ds3d`, and their sanitized twins |
| [ge/ge.c](ge/ge.c), [ge/ge.h](ge/ge.h) | The geometry engine: decodes the geometry command writes the platform layer logs, keeps the matrix stacks, lighting and texture coordinates, culls and clips polygons, and fills polygon RAM; the readbacks (GXSTAT, RAM_COUNT, the matrix results, the box, position and vector tests) give the DS's values | Everything that sends 3D commands: `lib/nitro/src/gx/g3imm.c`, `g3x.c`, NitroSystem G3D (`gecom.c`, `sbc.c`) | `g3d_ge`, `g3d_ge_sanitized` |
| [ge/ge_hal.c](ge/ge_hal.c), [ge/ge_hal.h](ge/ge_hal.h) | The geometry engine and the 3D renderer in the program: one engine fed from the platform layer's log of geometry writes, the readback registers' memory, the swap at each VBlank, and the 3D frame engine A shows, kept from the last frame when the list, the registers and the texture memory it is drawn from are all the last frame's | The same; the program attaches it at startup ([../app/main.c](../app/main.c)) | `g3d_hal`, `g3d_hal_sanitized` |
| [capture.c](capture.c), [capture.h](capture.h) | Display capture: engine A's picture, the 3D layer, a VRAM bank or a blend of two written into VRAM bank A to D as each line is drawn | Motion blur, 3D on both screens (`src/gfl/double3Ddisp.c`), the title screen's picture in VRAM D (`src/ov162/title.c`), the encounter effects (`src/ov148`) | `gfx_capture_plain`, `gfx_capture_sanitized` |

The presentation of the drawn screens in a window is the program's ([../app/present.c](../app/present.c)).
