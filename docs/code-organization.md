# Code organization

Each source file is one of the game's original source files. The linker placed one object per original file, so a
file's `.text`, `.rodata`, `.data` and `.bss` are each one contiguous range, and the files come in the same order in
every section.

- Name a file after the original. Many functions pass their file's name to `GFL_HeapAllocate` or an assert, so the
  overlay embeds strings such as `"resort_npc.c"`, which sit in that file's `.data`. A file whose name the ROM doesn't
  give gets a name for what it does, such as `gym_nacrene.c`; never an overlay number or a counter.
- An original file is one entry in `delinks.txt`, covering its whole range in each section, even while only some of
  its functions are written. The entry stays without `complete` until every function matches, and the original code
  is linked until then, as with `src/ov059/scrcmd_resort.c`. Never split a file into several entries to link the
  matching parts early, and never put two original files in one entry.
- The C file holds its functions in address order. A function that doesn't match yet stays in the file as the closest
  C found, so objdiff shows how far off it is. Library code built with another compiler can differ: SPL's compiler
  emits a file's functions in reverse source order, so `lib/spl/src/` files hold theirs in reverse address order.
- `src/` holds the code built with the game's compiler and flags. An overlay's files go in `src/ovNNN/`, and the main
  module's in `src/gfl/` (Game Freak's library) and `src/system/` (the game's own code), mirroring `include/`.
- `lib/` holds the libraries built apart from the game, as in pret's projects: `lib/<name>/include/` for the public
  headers, `lib/<name>/src/` for the sources and private headers (`lib/spl/src/spl_internal.h`), and
  `lib/<name>/library.toml` for the library's compiler and flags, which `configure.py` and the probe read: `lib/spl/`
  (Nintendo's SPL particle library), `lib/dsprot/` (Nintendo's DS Protect, in overlays 165 and 337), `lib/nitro/`
  (NitroSDK), `lib/nnsys/` (NitroSystem: FND, G2D, G3D, GFD and sound) and `lib/twl/` (TwlSDK's DSi libraries in the LTD
  autoload: the camera, the DSP and the new DMA and WRAM functions; and in `lib/twl/src/`, the SSP JPEG encoder and
  decoder with their EXIF writer and reader, linked into overlay 257 and built as Thumb with the game's compiler,
  `-ipa file` and `-inline on,noauto`).
  NitroSDK's first source is libcrypto's RC4 (`lib/nitro/src/crypto/rc4.c`, ARM, `dsi/1.1p1`), which the game links
  last among its own code in ARM9 main. NitroSystem's sources (`lib/nnsys/src/fnd`, `gfd`, ...) are Thumb built with
  CodeWarrior 2.0. A file built with other flags than the rest of its library is listed under `file_flags` in the
  `library.toml`, each flag replacing the library's flag of the same option (NitroSystem's sound capture takes
  `-ipa function`; see `docs/matching.md`). Library code uses the SDK's own names (see [Names](#names)). `include/stddef.h` and
  `include/stdlib.h` declare the few C library names the code needs (`size_t`, `offsetof`, `abs`), which the MSL C
  library linked into main defines. A library without sources needs no `library.toml`; its first source file adds
  one with the compiler it was built with. A library's public headers keep its name as their directory, as in
  `lib/nitro/include/nitro/os.h`, so code includes `"nitro/os.h"`. Every file is compiled with `include/` and every
  `lib/*/include/` on its search path.
- `tools/decomp/source_files.py OVERLAY` finds the boundaries: it lists the embedded file names, the functions that
  refer to them, and how well each boundary between two functions keeps every section's data references in file
  order and the calls inside one file.
  `docs/source-files.md` lists every overlay's files with the evidence for their names (`source_files.py --markdown`).
- `struct_decls.h` declares every struct type once, as `typedef struct Name Name;`. The header of the module that
  owns a struct defines its layout when that is known (`struct Name { ... };`, without another typedef), and other
  code only uses pointers to it. A struct that only one file uses, such as an event's work, is defined in that file.
  A layout is defined once: two files that need the same struct share it through the owner's header, and a partial
  layout with padding is still the one definition.
- Headers are grouped like the game's code: `system/` (game system, game data, events), `field/`, `save/`, `gfl/`
  (Game Freak's library), `pml/` (Pokémon data), `battle/`, `demo/` and `constants/`, whose ID lists, such as species
  and items, are generated from `data/constants/` (see [Constant lists](data.md#constant-lists)). The libraries'
  headers are in `lib/`, as `lib/nitro/include/nitro/`, `lib/nnsys/include/nnsys/`, `lib/spl/include/spl/`,
  `lib/dsprot/include/dsprot/`, `lib/dwc/include/dwc/` (Nintendo's Wi-Fi Connection library) and
  `lib/dpw/include/dpw/` (the Global Trade Station's server library).
  A header is named after the original file that owns its declarations, or after swan's header for it, such as
  `field/field_3dci.h`.
- Put functions, data and callback tables used across source files or overlays in the owning file's header. Declare
  external data with `extern` there, and include the header at each use. Do not add `extern` declarations or
  prototypes of other files' functions in a `.c` file. A file includes its own header, so its definitions are checked
  against the declarations.
- Each proc that an event starts has a header in `app/` with its parameter struct, proc table and overlay ID, such as
  `app/worldtrade.h`. Each event has a header in `field/` with its create functions, such as
  `field/event_worldtrade.h`. A proc built from many files keeps their headers in a directory of `app/` named after
  it, such as `app/comm_tvt/` for the Xtransceiver's files.
- Functions only called within their file are `static` where linking permits it and declared at the top of the file,
  since `-requireprotos` requires a prototype for every function.
- Event callbacks take `void *data`, as `GameEventCallback` does, and cast it to their work.
- A bug gets a `// BUG:` comment on what goes wrong. Where a fix is clear, it goes in an `#ifdef BUGFIX` block next to
  the original code, which stays in the `#else` branch, as in `bg_sys.c`'s `GFL_BGSysFlipTile`.
- Names, layouts and constants from swan are marked as such. swan's headers are generated for hacking tools, so
  they are a reference rather than copied as they are. A type that swan doesn't name gets a name from its owner,
  such as `ResortNPC` in `resort_npc.c`, and structs with the same layout and purpose are one type.

`ninja format` formats `src/`, `include/` and `lib/` with clang-format, using `.clang-format`; prefer running
`clang-format -i` on the files you changed, since clang-format releases disagree. `compile_flags.txt`, which
`configure.py` writes with the build's include path, makes clangd check the code as 32-bit ARM; the generated constant
headers only exist after a build.

## Names

Names come from the [swan](https://github.com/ds-pokemon-hacking/swan) symbol databases (GPL-3.0) by the
ds-pokemon-hacking community, revision `4324f73` (2025-07-03). `tools/decomp/import_swan.py` applies them:

- IDA-generated names are skipped.
- A name is only applied if a symbol starts exactly at its address.
- Names are carried between versions through the version map.
- Only default `func_`/`data_` names are replaced.

```sh
.venv/bin/python tools/decomp/version_map.py b2_us w2_us -o build/map_b2_w2.tsv --symbols-output build/map_b2_w2_symbols.tsv
.venv/bin/python tools/decomp/import_swan.py path/to/swan --map build/map_b2_w2.tsv --symbols-map build/map_b2_w2_symbols.tsv
```

Names that swan lacks are ours, and are recorded in `config/names.txt` by module and Black 2 address.
`tools/decomp/rename_symbol.py` renames a symbol in both versions, updates the source files and records the name:

```sh
.venv/bin/python tools/decomp/rename_symbol.py func_ov035_0217ed70 ElScoreboard_Create
```

## Versions

Black 2 is the primary version. White 2 is the same program: of its 41,423 functions, 41,311 are byte-identical to
Black 2's apart from relocations, and 112 differ. Every White 2 symbol with a Black 2 counterpart uses the Black 2 name,
so source files are shared, and code that differs uses the `BLACK2` and `WHITE2` defines. Symbols only found in White 2
get a `_w2_us` suffix.

`tools/decomp/version_map.py` pairs the functions of two versions by their bytes, and pairs other symbols through
relocations and section offsets. Functions that differ are marked `different` in `build/version_map.tsv`. Check them
with `compiler_probe.py --version w2_us`, which compiles with the `WHITE2` define, before marking a file complete.
