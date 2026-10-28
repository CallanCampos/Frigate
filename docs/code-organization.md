# Code organization

Each source file is one of the game's original source files. The linker placed one object per original file, so a
file's `.text`, `.rodata`, `.data` and `.bss` are each one contiguous range, and the files come in the same order in
every section.

- Name a file after the original. Many functions pass their file's name to `GFL_HeapAllocate` or an assert, so the
  overlay embeds strings such as `"resort_npc.c"`, which sit in that file's `.data`. A name with a Japanese word or one
  of Game Freak's private abbreviations is put in English, and the file's header gives the ROM's name: `pokedex_registration.c`
  is `pokedex_registration.c`, `fldmmdl_acmd.c` `field_actor_acmd.c` ([Names](#names) has the words). The string in
  the code stays the ROM's. A file whose name the ROM doesn't give gets a name for what it does, such as `gym_nacrene.c`; never an overlay number or a counter.
- An original file is one entry in `delinks.txt`, covering its whole range in each section, even while only some of
  its functions are written. The entry stays without `complete` until every function matches, and the original code
  is linked until then, as with `src/ov059/scrcmd_resort.c`. Never split a file into several entries to link the
  matching parts early, and never put two original files in one entry. An original file that the code shows in two
  modules is one source file with one entry, in ARM9 main's `delinks.txt`, which lists the LTD autoload's part in
  `.ltdmain` sections (`docs/configs.md`, Migrated LTD sections): NitroSDK's `card_rom.c` and `pm.c`, whose DSi parts in
  the LTD autoload reach the main parts' statics from their pooled base, as only code of the same object does, and
  `mi_sharedWram.c`, `os_interrupt.c`, `os_arena.c`, `os_ownerInfoEx.c` and `os_application_jump.c`, which TwlSDK's
  library objects show as one object each with both parts (aletheia's match of Black 1's code to them).
- The C file holds its functions in address order. A function that doesn't match yet stays in the file as the closest
  C found, so objdiff shows how far off it is. Library code built with another compiler can differ: SPL's compiler
  emits a file's functions in reverse source order, so `lib/spl/src/` files hold theirs in reverse address order.
- Code the original link dead-stripped, which `config/deadstrip.toml` lists and the build strips again, is the
  exception to both rules above (`docs/configs.md`, "Dead-stripped code"): written back from another decomp so that
  the data it pooled with live code comes out, or because such code calls it, it has no address and sits among the
  live functions in that decomp's order, can't be checked against the asm, and doesn't keep its file from `complete`.
  The file's comments flag it as reconstructions. It is used for `src/system/wipe_sub.c`'s dead wipe handlers and for
  the two `src/system/wipe_wnd.c` window readers that their plane swap calls.
- `src/` holds the code built with the game's compiler. An overlay's files go in `src/ovNNN/`, and the main
  module's in `src/gfl/` (Game Freak's library) and `src/system/` (the game's own code), mirroring `include/`.
  The default flags are in `configure.py`; a file whose original code and data require another setting goes in
  `config/compiler_flags.toml`, as `ctvt_draw.c` needs `-ipa function`. Different flags alone do not make a game
  source a library. See `docs/configs.md` for the shared compiler settings.
- `lib/` holds the libraries built apart from the game, as in pret's projects: `lib/<name>/include/` for the public
  headers, `lib/<name>/src/` for the sources and private headers (`lib/spl/src/spl_internal.h`), and
  `lib/<name>/library.toml` for the library's compiler and flags, which `configure.py` and the probe read: `lib/spl/`
  (Nintendo's SPL particle library), `lib/dsprot/` (Nintendo's DS Protect, in overlays 165 and 337), `lib/nitro/`
  (NitroSDK: in ARM9 main its startup code, its system call stubs, its card, cartridge, file system, graphics,
  memory, NVRAM, OS, PXI, RTC, sound, SPI (microphone, power and touch panel), STD and wireless libraries, its
  fixed-point math and libmath, and libcrypto's RC4; in the LTD autoload's `.ltdmain`, the DSi parts of its card ROM,
  PM and OS files and of TwlSDK's shared WRAM manager, in `card_rom.c`, `pm.c`, `os_interrupt.c`, `os_arena.c`,
  `os_ownerInfoEx.c`, `os_application_jump.c` and `mi_sharedWram.c`; in overlay 181 the multiboot library),
  `lib/nnsys/` (NitroSystem: FND, G2D, G3D, GFD and sound), `lib/twl/`
  (TwlSDK's DSi libraries: SCFG and the DSi system calls in ARM9 main; in the LTD autoload the camera, the DSP, the
  microphone, NWM, FATFS and the sound and RTC extensions, beside the DSi's photo signature code (names guessed), and
  in `.ltdmain` the card's hashed ROM reads and the new DMA; and in `lib/twl/src/`, the SSP JPEG encoder and decoder
  with their EXIF writer and reader,
  linked into overlay 257 and built as Thumb with the game's compiler, `-ipa file` and `-inline on,noauto`), `lib/dwc/`
  (Nintendo's Wi-Fi Connection library, TWL-DWC: its account, auto-connect, backup and WDS code in ARM9 main, ARM
  code; the rest in overlays 11, 29, 189 and 260, mostly Thumb), `lib/msl/` (Metrowerks' MSL C library for NITRO, the
  C library, fdlibm math and runtime that ARM9 main links after NitroSDK, ARM code built with `dsi/1.2`),
  `lib/nitrowifi/` (NitroWiFi, the TCP/IP stack CPS and the socket layer under DWC in overlay 11, and WCM's DSi
  backend in the LTD autoload's `.ltdmain`, Thumb with the game's compiler), `lib/dpw/` (the Global Trade Station's
  and the Battle Subway's Wi-Fi library in overlay 189, ARM),
  `lib/poke_net/` (the Global Data Server client at the end of overlay 201, ARM), `lib/dwcutil/` (the DWC utility, the
  Wi-Fi Connection settings in overlay 182, built with MWCC 2.0, but for its DS Download Play parent, wireless
  helper and flow, `dwc_mov_mbp.c`, `dwc_mov_wh.c` and `dwc_moveid.c`, built with the game's compiler and flags through
  `file_compilers` and `file_flags`), `lib/dwcutilui/` (that utility's main loop, screens,
  keyboards and selectable objects, and the helpers under them: resources, messages, heap, OAM, cells, fades, text,
  input, interrupts, lists, OBJ characters, strings, VRAM banks, tasks and display, built with the game's compiler and
  flags but unsigned `char`; the name is ours) and `lib/irc/` (the infrared link, overlay 31's last two files, built
  with the game's compiler and flags; the name is ours).
  NitroSDK and TwlSDK are built with the game's compiler, `dsi/1.1p1`, and DWC with `dsi/1.2` (only
  `DWCi_Acc_CreateTempLoginId` tells them apart, and it matches from 1.2 on): NitroSDK as ARM code by default, with most of
  its ARM9 main files Thumb through `file_flags`, and TwlSDK and DWC as Thumb by default, with their ARM files (the LTD
  autoload's TwlSDK, ARM9 main's DWC and libVCT) in `file_flags`. NitroSystem's sources (`lib/nnsys/src/fnd`,
  `gfd`, ...) are Thumb built with CodeWarrior 2.0. A file built with other flags than the rest of its library is
  listed under `file_flags` in the `library.toml`, each flag replacing the library's flag of the same option
  (NitroSystem's sound capture takes `-ipa function`; see `docs/matching.md`), and one built with another compiler
  under `file_compilers` (TwlSDK's `syscall_twl_ltd.c` takes `2.0/sp2`). Library code uses the SDK's own names
  (see [Names](#names)). The C library's headers are MSL's, in `include/` (`string.h`, `stdio.h`, `math.h` and the
  others, and `NMWException.h` for the runtime), and `lib/msl/src/` keeps MSL's private ones. A library whose ROM calls
  the string functions that MWCC builds in place from `<string.h>` declares them in its own header instead (`dwc/ext.h`,
  `ssl_internal.h`; `docs/matching.md`, "a header named `string.h`"), the one exception to declaring a function once.
  A library without sources
  needs no `library.toml`; its first source file adds one with the compiler it was built with. A library's public
  headers keep its name as their directory, as in `lib/nitro/include/nitro/os.h`, so code includes `"nitro/os.h"`.
  Every file is compiled with `include/` and every `lib/*/include/` on its search path.
- `tools/decomp/source_files.py OVERLAY` finds the boundaries: it lists the embedded file names, the functions that
  refer to them, and how well each boundary between two functions keeps every section's data references in file
  order and the calls inside one file.
  `docs/source-files.md` lists every overlay's files with the evidence for their names (`source_files.py --markdown`).
- A binary that the game embeds as it is, and that no compiler of ours can build, is a `const u8` array whose
  initializer includes the bytes from the base ROM. That covers the DSi DSP's TeakLite programs in the LTD autoload,
  `lib/twl/src/dsp/dsp_g711_image.c` and `dsp_graphics_image.c`. `ROM_BLOBS` in `configure.py` names each include file
  under `rom/` of the generated headers with its module and symbol, and `tools/build/rom_blob.py` writes it from the
  extracted module, with the symbol's address and size from `symbols.txt`. The bytes are never committed. Without a
  ROM, `compile_all.py` writes zeros of the same size. The filler of the system call library in the secure area, which
  differs in each version, comes the same way from each version's ROM, as an assembler macro (`ROM_RANGES`,
  `docs/configs.md`).
- The digests of the ARM9 overlays at the end of ARM9 main's `.data`, which NitroSDK's tools write after the link,
  are written the same way: `lib/nitro/src/fs/fs_overlay_digest.c` leaves their room as zeros, `dsd rom config` finds
  its `SDK_OVERLAY_DIGEST` in the linked program, and `dsd rom build` fills it with the HMAC-SHA1 of each overlay
  just built, as stored, and the masked one of the overlay table. They are what FS_StartOverlay checks when the game
  isn't booted from the ROM, and a mod that changes an overlay gets its digest.
- `struct_decls.h` declares every struct type once, as `typedef struct Name Name;`. The header of the module that
  owns a struct defines its layout when that is known (`struct Name { ... };`, without another typedef), and other
  code only uses pointers to it. A struct that only one file uses, such as an event's work, is defined in that file.
  A layout is defined once: two files that need the same struct share it through the owner's header, and a partial
  layout with padding is still the one definition.
- Headers are grouped like the game's code: `system/` (game system, game data, events), `field/`, `save/`, `gfl/`
  (Game Freak's library), `pml/` (Pokémon data), `battle/`, `demo/` and `constants/`, whose ID lists, such as species
  and items, are generated from `generated/` (see [Constant lists](data.md#constant-lists)). The libraries'
  headers are in `lib/`, as `lib/nitro/include/nitro/`, `lib/nnsys/include/nnsys/`, `lib/spl/include/spl/`,
  `lib/dsprot/include/dsprot/`, `lib/dwc/include/dwc/` (Nintendo's Wi-Fi Connection library),
  `lib/nitrowifi/include/nitrowifi/` (NitroWiFi, under DWC), `lib/twl/include/twl/` (TwlSDK),
  `lib/dpw/include/dpw/` (the Global Trade Station's server library), `lib/poke_net/include/poke_net/` (the Global
  Data Server client), `lib/dwcutil/include/dwcutil/` (the DWC utility's AOSS client, connection test and
  DS Download Play parent, wireless helper and flow, which its screens call) and `lib/irc/include/irc/` (the
  infrared link).
  A header is named after the original file that owns its declarations, or after swan's header for it, such as
  `field/field_3dci.h`.
- Put functions, data and callback tables used across source files or overlays in the owning file's header. Declare
  external data with `extern` there, and include the header at each use. Do not add `extern` declarations or
  prototypes of other files' functions in a `.c` file. A file includes its own header, so its definitions are checked
  against the declarations.
- A function the build compiled into more than one file (`tools/decomp/find_duplicates.py` lists them) is written
  once. Its body goes in the owner's header as a `static inline` named after the owner's function with an `Inline`
  suffix, and the owner and each copy keep their own names and linkage (a `static` copy stays `static`) and are each
  one call to it. The header gains the includes the body needs; `ninja` compiles every file that includes it. The
  file's compiler has to inline the call: the game's `-inline on,noauto` and the 2.0 library default do, while
  `-inline off` leaves a call. A copy that doesn't match this way is written out in its own file, with a comment
  naming the function it copies. Only `identical` groups are shared: in a `same via duplicates` group each copy calls
  its own duplicate, which one inline cannot, and a `same shape` group is not proven to be the same code.
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

A script command's handler is `Scrcmd_<Group>_<Name>`, swan's `sNNNN_` handlers included (`docs/scripts.md`). The user
chose this on 2026-10-29.

Every name is English, swan's included: a Japanese word or one of Game Freak's private abbreviations is translated,
and the header of a file whose swan names were translated says so. The user chose this on 2026-10-28. Well-known
acronyms such as BGM, SE, HP, PP, TM, HM, NPC, BG and OBJ stay. A name the ROM spells out is in English too: `GFL_ASSERT`
keeps its expression's text in the game, so an assert that names something by a translated word is written out with
the ROM's text, as `worldtrade_deposit.c`'s for `PokedexSortInitialTable` is (docs/code-standards.md, rule 3). The words
so far:

| Word | English | | Word | English |
|---|---|---|---|---|
| zukan | Pokedex | | dendou | HallOfFame |
| zenkoku, chihou | National, Regional | | hiden | HM |
| toroku | Registration | | koukan | Trade |
| shinka | Evolution | | guru2 | SpinTrade |
| waza, oshie | Move, Tutor | | oekaki | Drawing |
| namein | NameInput | | kuru | Spin |
| fldmmdl, mmdl | FieldActor, Actor | | blact | BillboardActor |
| gyoe | EmotionBubble | | ochiba | Leaves |
| iaigiri | Cut | | iwakudaki | RockSmash |
| kemuri | Dust | | kira | Sparkle |
| namipoke, nami | SurfPokemon, Surf | | denki | Electric |
| zkn, zknd | Pokedex, PokedexDetail | | dpc, ddemo | HallOfFamePC, HallOfFameDemo |
| wazarec | MoveRec | | sodateya | DayCare |
| asonda, tomo | Played, Friend | | siin (the initial's kana row) | Initial |
| kami (the trio) | ForcesOfNature | | | |

The data's names follow the same rule, with three exceptions that the ROM spells out. The sound constants
(`generated/sound*.txt`, `SEQ_BGM_KOUKAN`) are the sound archive's symbols, which the build writes back into it; the
map models' and texture sets' (`generated/map_models.txt`, `map_texture_sets.txt`) are the assets' own names; and a
message ID that quotes its text (`Bank0366_Text_SwKusabi10`) says what the message says. Everything else is English:
the archives (`ARCID_SPIN_TRADE` and its `res/graphics/spin_trade/`), their members and the text banks
(`TEXT_BANK_POKEDEX_SEARCH_MAIN`, whose message IDs are `PokedexSearchMain_Text_*`).
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
