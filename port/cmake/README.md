# port/cmake/

The host build's fragments. [../CMakeLists.txt](../CMakeLists.txt) sets up what everything shares (the version, the
generated tree, SDL3, the decomp's language model, the five object libraries `pokebw2_game`, `port_hal`, `port_gfx`,
`port_app` and `port_launcher` (Frigate's test launcher, which every program that links the game links too), the trap
stubs and the program) and then includes every `*.cmake` here in name order. Each fragment holds one module's build
rules (its per-file options, its generated inputs) and the tests of that module, so that a module's build and tests
are read in one place.

| Fragment | What it adds | Game functionality it serves | Tests it defines |
| --- | --- | --- | --- |
| [app.cmake](app.cmake) | The rest of the program's own files (`port_app` starts with `app/main.c`): the window, gamepads, the macOS log, replays, the run directory and symbol lookup, the sound driver's trace, the oracle's input file reader; the window icon's generator; the replay tests; the test launcher's option and gate | The window, the controls, replays, the sound for the oracle (its trace), the field menu's items | `app_window_icon_*`, `app_about`, `app_symbols`, `app_run_dir_*`, `app_launcher_spec_*`, `app_gamepad_*`, `replay_start_menu(_clean)`, `replay_new_game(_clean)` (Black 2), `replay_same_save` |
| [assets.cmake](assets.cmake) | The check of Frigate's artwork ([../assets/branding](../assets/branding/README.md)) | None: the program's look | `assets_branding` |
| [fx_tests.cmake](fx_tests.cmake) | The decomp's libfx C built for a test | 3D maths (cameras, models) | `fx_libfx` |
| [g3d.cmake](g3d.cmake) | The geometry engine and 3D renderer's tests, plain and sanitized, and their timing gate | 3D scenes | `g3d_ge*`, `g3d_ds3d*`, `g3d_hal*`, `g3d_timing` |
| [gfx_tests.cmake](gfx_tests.cmake) | The 2D renderer and capture tests, plain and sanitized, and the renderer's timing gate | 2D screens, capture | `gfx_ds2d*`, `gfx_capture_*`, `gfx_ds2d_timing` |
| [gx.cmake](gx.cmake) | The `port_sdk_` renames of `gx_vramcnt.c`, `gx_state.c`, `mi_wram.c` and `gx_load2d.c`, which [../hal/gx](../hal/README.md#the-modules) wraps | VRAM banks, palette loads | `gx_*` |
| [hal_tests.cmake](hal_tests.cmake) | The known-vector tests of the SDK's assembly in C, the card slot's SPI bus and infrared chip, and the asm oracle (which skips without the extracted ROM, `build/port-venv` or a Ninja build directory) | Copies, fills, decompression, matrices, sorting, SHA-1, RC4; the genuine-card check | `hal_mi`, `hal_fx`, `hal_card_spi_*`, `asm_oracle` |
| [integrate.cmake](integrate.cmake) | Scenarios on the whole program's objects, and the option checks, the test launcher's exit statuses among them (through `tests/app/check_exit.py`) | DMA, the allocator, input, the screens, CONTINUE without the launcher | `integrate_*` |
| [layout_tests.cmake](layout_tests.cmake) | Struct layouts that the game's files must agree on in the host build, from the objects' debug information (it skips without `-g` or `llvm-dwarfdump`) | The trainer card's param, shared by the field's app call and the card | `layout_trainer_card_param` |
| [macos_app.cmake](macos_app.cmake) | The macOS app's target `<program>_app`, its names, icon and bundle identifier (whose prefix is in [../app/macos/bundle_id.cmake](../app/macos/bundle_id.cmake)), and the architectures and minimum macOS it is for, from the build's | None: the program as an app | `app_make_app`, `app_bundle_id`, `app_message_log`, `app_package_macos_out`, `app_slices` (the program's load commands), `app_universal_slices` (both slices' frames) |
| [nnsys.cmake](nnsys.cmake) | The `port_sdk_` renames of NitroSystem's FND heap frees and G2D loaders, which [../hal/nnsys](../hal/README.md#the-modules) replaces | Sprites, cells, animations, fonts, heaps | `nnsys_*` |
| [oracle.cmake](oracle.cmake) | The melonDS test oracle as an external project (only with `PORT_BUILD_ORACLE`), and its tests: the saves made on the oracle (`oracle_saves`, a fixture the others start from), the screens' and the sound's comparisons, the field menu's apps through the test launcher, and the launcher's runs from those saves | Every screen, and the sound, held against melonDS; saves with a Pokémon; the field menu's apps | `oracle_*`, `launch_*` |
| [os.cmake](os.cmake) | minicoro, the OS core's per-file options and stack probes, and its tests | Threads, interrupts, timing, the HBlank interrupt the screen wipes need | `os_*` |
| [overlay_sections.cmake](overlay_sections.cmake) | The per-overlay forced includes that put each overlay's writable statics in its own sections, and the link's section check on macOS | Overlays | `overlay_sections_image_*`, `overlay_sections_budget`, `overlay_sections_leaks` |
| [pxi.cmake](pxi.cmake) | The per-file options of the decomp files the ARM7's services compile through | Everything the ARM7 serves | `pxi_*` |
| [rom.cmake](rom.cmake) | The ROM manifest (`gen/rom/rom_manifest.c`, from `<version>.sha1` and the NitroFS manifest), the development ROM path, the backup's forced include, and the tests of the ROM, the card and the file system as the game links them | Every file read, saves, the genuine-card check | `rom_*`, `card_backup_*`, `rom_game_*` (`rom_game_ir_genuine_card` among them) |
| [sound.cmake](sound.cmake) | `audio/audio.c` into the program, and the sound driver's and sound unit's tests, one scenario a process | Music, sound effects, cries | `snd_*` |

## Options

Each is given at configure time, `cmake -S port -B build/port -DNAME=VALUE`.

| Option | Default | Where | What it does |
| --- | --- | --- | --- |
| `PORT_VERSION` | `b2_us` | [../CMakeLists.txt](../CMakeLists.txt) | The game: `b2_us` (Black 2, `frigate-black2`) or `w2_us` (White 2, `frigate-white2`). One build directory per version |
| `PORT_FETCH_SDL3` | `ON` | [../CMakeLists.txt](../CMakeLists.txt) | Builds SDL3's pinned release when no installed SDL3 3.4 is found |
| `PORT_MACOS_APP` | `OFF` | [macos_app.cmake](macos_app.cmake) | Makes the macOS app with every build, not only with its target |
| `PORT_MACOS_BUNDLE_ID_PREFIX` | `io.github.callancampos.frigate` | [../app/macos/bundle_id.cmake](../app/macos/bundle_id.cmake) | The app's bundle identifier before `.black2` or `.white2`; a build that isn't Frigate's gives its own. A build directory's old placeholder, `com.example.pokebw2-native`, is moved to the default |
| `PORT_BUILD_ORACLE` | `OFF` | [oracle.cmake](oracle.cmake) | Fetches melonDS and builds the test oracle, `pbw-oracle` |
| `PORT_ORACLE_SANITIZE` | `OFF` | [oracle.cmake](oracle.cmake) | Builds the oracle's own sources with AddressSanitizer and UndefinedBehaviorSanitizer |
| `PORT_OVSEC_LAYOUT` | `split` | [overlay_sections.cmake](overlay_sections.cmake) | `split` (data and bss sections apart) or `combined` (one section per overlay) |
| `PORT_OVSEC_BUDGET` | `150` | [overlay_sections.cmake](overlay_sections.cmake) | The most overlay sections a Mach-O link may have before the check stops it |
| `PORT_TEST_BOOT_LOGOS` | `OFF` | [integrate.cmake](integrate.cmake) | Adds `integrate_boot_logos`, 120 frames to the first logo screen |
| `PORT_GFX_TIMING_GATE`, `PORT_GFX_TIMING_LIMIT_US` | `OFF`, `2000` | [gfx_tests.cmake](gfx_tests.cmake) | Runs the 2D renderer's timing as a test, with its limit in microseconds |
| `PORT_G3D_TIMING_GATE`, `PORT_G3D_TIMING_LIMIT_US` | `OFF`, `5000` | [g3d.cmake](g3d.cmake) | The same for the 3D renderer |

A sanitizer build is a build directory of its own with the flags for every file ([os.cmake](os.cmake)'s header has the
command).

## Adding a fragment

A new module's fragment is `<module>.cmake`, opening with a comment that says which module it builds and which part of
the game that serves. Its tests take the module's name as their prefix, set a `TIMEOUT`, give every run of the program
a `--save-dir` in the build tree, and skip with status 77 (or the program's 66, no ROM) when they need something the
machine doesn't have, so that CI, which has no ROM, still passes.
