# BW2 native port: assets, ARM7, sound, saves, DSi and IR

Scope: the data and asset side of a native PC/Mac port of Pokémon Black 2 / White 2 (US) built from the pokebw2
decompilation. The port ships no Nintendo assets and reads them from the user's own ROM. This file covers the
ROM's layout, how the game loads its assets, what `data/` already provides, the ARM7, sound, saves, DSi (TWL)
features, IR, and a design for first-launch extraction.

Sources: the worktree `W = .claude/worktrees/desktop-subagent-inspection-3190ba` (read only), its
`extract/{b2_us,w2_us}`, `orig/baserom_*.nds`, `configure.py`, `docs/`, `src/`, `lib/`, `tools/data`. Everything
measured here came from scripts in
`scratchpad/research/work/assets/scripts/` (`fs_survey.py`, `diff_versions.py`, `sdat_survey.py`,
`sdat_players.py`, `coverage.py`, `manifest.py`). Their outputs sit next to them: `b2_fs.json` (each file and
NARC with its member types), `archives.tsv` (one row per NARC), `diff_files.txt` (the Black 2/White 2 diff) and
`nitrofs_manifest.json` (SHA1 and size of all 318 NitroFS files in both versions, ready for the port's verifier).

---

## 0. Summary

- **ROM**: a 512 MiB NDS/DSi hybrid (`IREO` Black 2, `IRDO` White 2, TwlSDK 5.3). Every byte the port needs, the
  NitroFS file system and the overlays, sits below 0x112A723C (Black 2), about 276 MiB. NitroFS holds 318 files
  (271 MiB). 308 of them are NARCs with 69,850 members. Only 8 NitroFS files differ between Black 2 and White 2.
  The ARM7, ARM7i and ARM9i are byte-identical in both versions.
- **Loading**: the game reads assets in three ways: by archive ID through a 308-entry path table
  (`a/x/y/z`), by NitroFS path for 6 special files, and from `swan_sound_data.sdat` through NitroSystem's sound
  archive. The caller decides whether a member is LZ-compressed (LZ10/LZ11, `MI_UncompressLZ8`); the file does
  not say so. There is no Mobiclip or other video codec: the two "movies" are sequences of LZ11-compressed bitmap
  frames.
- **data/**: 52 of the 318 files (34% of NitroFS bytes, or 66% counting the SDAT) are rebuilt from editable sources
  and checked byte for byte by `ninja`. The JSON and text packers need no ROM. The graphics, model, texture and
  cry packers patch over the original archive, so they need the user's ROM. `data/` contains converted Nintendo
  assets, so a port release must not include it.
- **ARM7**: nothing is configured or decompiled; `config/<v>/` has only `arm9`. The ARM7 is a stock TwlSDK 5.3
  component (same `SDK_VERSION` 0x0503757C as the ARM9). Its ARM7i extension is stock DSi Wi-Fi code
  (Atheros SDIO and WPA). The host must re-create the **SND driver** (sequencer, 16 channels and mixer). Every
  other ARM7 service (RTC, touch, power, mic, NVRAM, backup, wireless) is easier to replace at the ARM9 SDK API.
- **Sound**: one 89.4 MB SDAT holds 1,390 sequences (273 BGM, 1,066 SE), 1,949 banks and 1,949 wave archives
  (8,714 waves: 8,024 IMA-ADPCM and 690 PCM8). It also holds 7 players plus a stream player, and 1 stream (the
  title theme, PCM16 stereo at 32,728 Hz, 112 s). The game uses no sequence archives (SSAR) and starts no capture
  effects. It does need per-track mute, volume, pitch and pan (the interactive sound system), wave output (cries),
  streams, in-memory sequences and banks (musicals) and the microphone.
- **Saves**: a 512 KiB flash (`CARD_BACKUP_TYPE_FLASH_4MBITS_EX`). The main save is 0x26000 bytes, at 0x0 with
  its backup at 0x26000. Each block has a CRC16-CCITT, and a footer with magic 0x31053527 and a save count picks
  the newer copy. 21 extra saves occupy 0x4C000–0x7E000. A raw 524,288-byte host file matches melonDS, DeSmuME
  and real-cart dumps. Import should adopt the save's stored RTC offset, or the game resets its daily events.
- **DSi**: in DSi mode the game adds Xtransceiver video chat (camera, DSP, SSP JPEG, photo signatures), WPA Wi-Fi
  through NWM, a 5-level battery meter and larger heaps. The port should report "not TWL" by default.
- **IR**: `lib/irc` (overlay 31) serves the Feeling Check (overlays 175–180), IR battles and the C-Gear's IR
  marker. Overlay 338 checks the IR chip's ID as anti-piracy. The port replaces the link with a network transport
  and stubs the chip check, and DS Protect (overlays 165 and 337) must be stubbed as well.

---

## 1. ROM layout

### Modules

| Module | Where (RAM) | Size | Notes |
| --- | --- | --- | --- |
| ROM image | – | 512 MiB (capacity code 12) | NTR area used to 0x11831A00, TWL area to 0x118EC400 (Black 2). White 2: 0x117DD400 / 0x1186C400 |
| Header + secure area | – | 0x4000 | `extract/<v>/header.yaml`, `header_logo.png`. Game code `IREO` / `IRDO`, maker `01`, unitcode 2 (hybrid), ROM version 0. Header CRC16 0x7680 (Black 2) / 0xD75C (White 2) |
| ARM9 main | 0x02004000, entry 0x02004800 | 628,544 bytes decompressed (472,740 compressed in ROM) | `.bss` 0x0209D740–0x0214F500. Build info at 0xFB0. `sdk_version` 0x0503757C = TwlSDK 5.3, relstep 30076. 4-word (TWL-style) autoload list |
| ITCM autoload | 0x01FF8000 | 5,024 bytes | SDK fast code |
| DTCM autoload | 0x02FE0000 | 160 bytes + 32 `.bss` | |
| Autoloads 2 and 3 | 0x02400000 and 0x06898000 (LCDC VRAM bank H) | 32 bytes each, all zero | Placeholders. dsd calls them "Unknown" |
| ARM9i | 0x02400000 | 4 bytes after extraction | Its first 0x4000 bytes are modcrypt (AES-CTR). dsd splits out the LTD module |
| LTD autoload 0 (DSi only) | 0x02700000 | 437,984 bytes + 12,032 `.bss` | Compressed inside ARM9i. Holds TwlSDK's DSi libraries (camera, DSP with two Teak COFF images, TWL mic, NWM, FATFS, SNDEX, RTC extensions, photo signature) and `.ltdmain`, the DSi-only Thumb code of main's library files |
| ARM9 overlays | 72 distinct base addresses, most at 0x0214F500 | **344 overlays** (ov000–ov343), 3,689,824 bytes decompressed | 332 are compressed in ROM. The largest are ov036 (field, 352,672 bytes), ov167 (battle, 268,096) and ov011 (243,264). The brief said 343; there are 344 |
| ARM7 | 0x02380000, entry 0x02380000 | 167,812 bytes | Autoloads to ARM7 WRAM 0x037F8000 (0x11414 bytes) and 0x02FE0000 (0x1750C bytes). Same `sdk_version` 0x0503757C |
| ARM7i (DSi only) | 0x02E80000, entry 0x03030444 | 291,064 bytes | DSi Wi-Fi (Atheros AR6k SDIO, WMI/HTC, `wpa_supplicant v0.5.9-Devicescape`), FATFS (`nand:`/`sdmc:`), TWL extensions |
| Banner | – | animated DSi banner | `banner/{bitmap,palette}.png`, `animation.bin` |
| DSi extras | – | – | `dsi/region_prefix.bin` (12 KiB), `arm9/hmac_sha1_key.bin`. Every 0x400 bytes of the ROM is covered by a SHA1-HMAC digest |
| FNT / FAT | ROM 0x363400 / 0x363A00 | 0x4F0 / 0x14B0 bytes | 662 FAT entries: 318 files and 344 overlays |

### NitroFS

- **318 files, 284,345,382 bytes (271.2 MiB), 39 directories.** File data spans ROM offsets 0x7A400–0x112A723C
  in Black 2 (0x1125443C in White 2). Path order: `/skb.narc`, `/soundstatus.narc`, `/swan_sound_data.sdat`,
  `/a`, `/dl_rom`, `/dwc`.
- **`a/0/0/0` … `a/3/0/7`**: 308 numbered files, the archive table. `a/0`, `a/1` and `a/2` hold 100 files each,
  `a/3` holds 8. They total 183.7 MiB: `a/0` 103.8 MiB, `a/1` 40.6, `a/2` 39.0, `a/3` 0.3. 306 are NARCs. Two are
  raw: `a/0/1/3` (area records, 4,090 bytes) and `a/2/2/1` (4 zero bytes).
- **308 NARCs in total** (the 306 under `a/`, plus `skb.narc` with 6 members and `soundstatus.narc` with 4).
  They hold 69,850 members, 5,862 of them empty placeholders. Only one NARC has a filename table, so members are
  addressed by index. The largest NARC has 15,065 members (`a/0/0/4`, Pokémon sprites); the median NARC is 37 KB.
- **Other files**: `swan_sound_data.sdat` (89,373,024 bytes); `dl_rom/child_r_eng.srl` (726,216) and
  `child2_r_eng.srl` (656,584), the DS Download Play child programs; `dl_rom/icon_{b,w}.{char,plt}`; and
  `dwc/utility.bin` (929,852 bytes, LZ10, the Nintendo WFC settings utility's resources, mounted as `dwc:`).

### Extraction and rebuild

- `extract/<v>/` comes from **dsd** (`ds-decomp-cli` 0.12.1, built from the fork
  `fuddlesworth/ds-decomp` tag `v0.12.1-dsi.4`, which adds DSi hybrid support). Ninja's `extract` rule runs
  `dsd rom extract --rom orig/baserom_<v>.nds --output-path extract/<v>`. The output has `config.yaml`,
  `header.yaml`, `arm9/` (main decompressed, ITCM, DTCM, unknown autoloads, HMAC key), `arm9_overlays/` (344
  decompressed `.bin` files and `overlays.yaml`), `arm7/`, `dsi/` (ARM9i, ARM7i, LTD, region prefix), `banner/`,
  `files/` (NitroFS) and `path_order.txt`.
- The build mirrors `extract/<v>/files` into `build/<v>/files` with symbolic links (`tools/build/files_tree.py`),
  writes the archives built from source in place of their links, then runs `dsd rom build` (re-encrypting,
  recompressing and re-signing). It checks both SHA1s: Black 2 e51e6dfb8678a3d19dcd2a10691b96a569ca0abb, White 2
  b5d7490be7b415b8f1e672a53e978a9cc667e56a.

### Black 2 against White 2

- NitroFS: **8 of 318 files differ** (269.8 MB of content is identical). The differing archives, with their
  differing members:
  - `a/0/2/6`: 8 of 15 (title screen 2D; `ARCID_TITLE` is 26).
  - `a/0/8/5`: the town map's places.
  - `a/1/2/7`: 73 of 135 wild encounter tables.
  - `a/1/6/0`: 5 of 27.
  - `a/1/7/6`: 31 of 684.
  - `a/2/2/2`: 615 of 627 (the opening "movie" frames).
  - `a/2/2/7`: 15 of 86 bitmaps.
  - `a/2/9/6`: 35 of 58.

  The text archives (`a/0/0/2`, `a/0/0/3`) and the SDAT are identical.
- Code: ARM9 main (628,544 against 628,608 bytes), ITCM, DTCM, LTD and 261 of 344 overlays differ, mostly by
  address shifts (only 3 overlays differ in size). The ARM7, ARM7i and ARM9i are identical. The banner and DSi
  region prefix differ.

---

## 2. Runtime asset loading

### How the game names and opens files

1. **Archive IDs**. `GFLAppInit` (`src/system/gfl_use.c`) calls
   `GFL_ArcSysInit(NARC_PATH_LIST, ARC_PATH_COUNT = 308)`. `NARC_PATH_LIST[id]` is `"a/0/0/0"` …
   `"a/3/0/7"`, so archive ID N is the path `a/<N/100>/<N/10%10>/<N%10>`. `GFL_ArcSysGetResourcePath(id)` returns
   it. `include/constants/arc.h` names about 115 of the IDs: `ARCID_SYSTEM_MESSAGE` 2, `ARCID_SCRIPT_MESSAGE` 3,
   `ARCID_POKEGRA` 4, `ARCID_MAP_TERRAIN` 8, `ARCID_MAP_MATRIX` 9, `ARCID_ZONEDATA` 12, `ARCID_PERSONAL` 16,
   `ARCID_WAZAINFO` 21, `ARCID_FONT` 23, `ARCID_ITEMINFO` 24, `ARCID_EVENT_SCRIPT` 56, `ARCID_TRSPRITE_*` 71/72,
   `ARCID_TRDATA` 91, `ARCID_ENCOUNTDATA` 127, `ARCID_ISS_*` (sound), `ARCID_DEMO3D_RESOURCE` 158,
   `ARCID_CDEMO_*` 220–222, and others.
2. **GFL archive API** (`include/gfl/arc.h`, `src/gfl/arc_tool.c`, `src/gfl/arc_util.c`).
   - `GFL_ArcSysRead(dest, arcId, fileId)`, `GFL_ArcSysReadHeapNew`, `GFL_ArcSysReadRange` and
     `GFL_ArcSysGetDataLength`/`GetDataMax` open the NARC by path with `FS_OpenFile`. They seek to the
     header's FATB entry and read the member from GMIF.
   - `GFL_ArcSysCreateFileHandle(arcId)` returns an `ArcTool` that keeps the NARC open, with `Read`,
     `ReadRange`, `GetDataOfs`, `Seek` and `ReadRaw`. Its offset-based reads are used for streamed data such as
     the cutscene frames.
   - `GFL_ArcSysCreateMemoryHandle` reads an archive held in memory.
   - `GFL_ArcSysReadRawResource` (`src/system/file_util.c`) reads an archive file by offset without its NARC
     structure.
   - `GFL_ArcSysReadHeapNewLZ(arcId, fileId, BOOL compressed, heap)` and `GFL_ArcToolReadHeapNewLZ` (125 call
     sites) decompress with `MI_UncompressLZ8`, which handles both LZ77 type 0x10 and the extended type 0x11.
     **The caller passes the `compressed` flag**, so a modded member must keep the original's compression, or
     the host loader must detect it.
   - `arc_util.c` wraps these for BG and OBJ loading (NCGR, NCLR, NSCR, NCER and NANR into VRAM).
3. **Paths**, through `FS_OpenFile`:
   - `"swan_sound_data.sdat"`: `NNS_SndArcInit` in `src/system/snd_sys.c`. The sound archive then reads its own
     members by FAT offset.
   - `/dl_rom/child_r_eng.srl` (Poké Transfer), `/dl_rom/child2_r_eng.srl` (Memory Link, and a transfer that
     starts from the start menu) and `/dl_rom/icon_{b,w}.{char,plt}`: DS Download Play, in
     `src/ov181/mb_parent_sys.c` and `mb_parent_dataconv_sys.c`.
   - `dwc/utility.bin`: DWC's WFC settings, which mounts it as `dwc:` and uses files such as
     `dwc:/move/child.srl`.
   - `src/gfl/systemfont.c`: the debug font.
   - `skb.narc` and `soundstatus.narc`: no string reference was found in the ARM9, the overlays or the LTD.
     `skb` is probably a software keyboard. These two are unconfirmed and possibly unused.
4. **Overlays** load through `GFL_OvlLoad` → `FS_LoadOverlay`. Every load re-runs the overlay's `.data`, `.bss`
   and static initializers (DS Protect decrypts its code from `.ctor`). A native port links every overlay
   statically, so it must keep that per-load re-initialization of overlay statics.

### Formats, with counts and sizes

Counts are NARC members in Black 2, from `b2_fs.json`. Sizes are as stored, with the unpacked size of LZ data in
brackets.

| Format | Members | Size | Main archives and use |
| --- | --- | --- | --- |
| NCGR (2D characters) | 10,452 (2,193 plain, 7,216 LZ11, 1,043 LZ10) | 24.2 MB [94 MB] | Everywhere: `a/0/0/4` Pokémon sprites, `a/0/5/1` musical sprites, `a/0/7/1-2` trainer sprites |
| NCLR (palettes) | 5,072 | 1.6 MB | |
| NSCR (BG screens) | 903 | 2.4 MB | UI screens |
| NCER / NANR | 3,643 / 3,643 (3,181 NANR are LZ11) | 1.9 / 4.5 MB | OBJ cells and animations |
| NMCR / NMAR (multi-cell) | 3,181 / 3,181 | 0.7 / 0.8 MB | Pokémon and trainer sprite animations |
| NSBTX (textures) | 1,725 | 29.6 MB | `a/0/1/4` map texture sets (409), `a/0/4/8` overworld sprites as billboards (968), `a/1/7/4-5` prop textures |
| NSBMD (models) | 840, plus 1,065 `BMD0` models inside the map chunks of `a/0/0/8` and the prop bundles of `a/2/2/5-6` | 23.6 MB, plus 31.5 and 13.7 MB | `a/1/5/8` 3D cutscenes (`ARCID_DEMO3D_RESOURCE`, 22.7 MB), `a/0/1/1`, `a/1/1/6`, `a/2/6/3` |
| NSBCA / NSBTA / NSBMA / NSBTP / NSBVA | 512 / 455 / 260 / 143 / 74 | 14.4 / 2.1 / 0.5 / 0.06 / 0.15 MB | Skeletal, texture-SRT, material, pattern and visibility animation |
| SPL particles (`SPA`) | 791 | 9.0 MB | `a/0/0/6` (739, presumably move effects), plus 52 in 11 other archives. Built with `lib/spl` |
| NFTR fonts | 5 (`a/0/2/3`, `ARCID_FONT`, with 3 NCLR) | 0.65 MB | GF text rendering |
| Text (GF message data) | 1,171 files (`a/0/0/2` 495, `a/0/0/3` 676) | 7.8 MB | UTF-16 encrypted per line with a key from its last character, a header of language sections (one here), control codes ≥ 0xF000, optional 0xF100 compression (`tools/text/msgdata.py`) |
| Scripts (VM bytecode) | `a/0/5/6` 1,289 field scripts, `a/1/6/9` 14 TR AI scripts, plus move-animation and musical scripts | 0.6 MB | `src/system/vm.c`: a 16-bit command ID and its arguments. Portable data, no native code |
| Map data (GF custom) | `a/0/0/8` 1,065 chunks (`WB`/`GC`/`NG`/`RD`: a BMD0 model, terrain/collision and prop placements), `a/0/0/9` 416 matrices, `a/0/1/2` zone headers, `a/0/1/3` areas, `a/1/2/6` 616 zone event files, `a/1/2/7` encounters | 31.5 MB for the chunks | Collision and permissions live in the chunk's terrain section (`docs/data.md` § Map model sources) |
| Game tables (GF custom) | `a/0/1/6` 710 personal, `a/0/2/1` 560 moves, `a/0/2/4` 639 items, `a/0/9/1-2` 814 trainers and parties, and others | small | Little-endian packed structs |
| Cutscene frames ("video") | `a/2/2/2` 626 frames (opening, differs per version), `a/2/2/0` 151 frames (Game Freak logo): LZ11, 131,072 bytes unpacked each; `a/2/2/2` also has a 512-byte last member, probably a palette. `a/2/2/7`: 86 LZ10 98,304-byte (256×192×16-bit) bitmaps | 14.0 MB [102 MB] and 1.4 MB [8.5 MB] | Played by `src/ov264/cdemo_*.c` (`CDEMO_MOVIES`: `{arcId, frames, 0x18000, …}`) |
| Sound | `swan_sound_data.sdat` (§5). `a/1/4/0` and `a/1/4/2` hold small SDATs ("PALPARK", Poké Transfer). `a/1/1/2-5` hold SSEQ+SBNK+SWAR sets (musical programs, loaded into SDAT slots by `ov209/sta_acting.c`). `a/2/0/9` holds 3 RIFF WAVs (DSi camera shutter and video sounds). `a/1/9/1` holds cry volumes | 89.4 MB + 0.3 MB | |
| Raw (other GF formats) | 27,252 members | 60.0 MB | Mostly map chunks (31.5 MB), prop bundles (13.7 MB) and text (7.8 MB) |

**Compression totals.** 12,422 members are LZ-compressed: 11,200 LZ11 and 1,222 LZ10, 29.5 MB that unpack to
about 204 MB. The ARM9 and 332 overlays use the SDK's backward ("BLZ") compression, which matters only for
the ROM, not the port.

**Video.** There is no Mobiclip, VX or Actimagine: no such strings in any binary, and no movie files. The only
"movies" are the cdemo frame sequences above. The 3D cutscenes (`a/1/5/8`) are NitroSystem G3D models and
animations driven by scripts.

**Porting notes on formats.**
- NitroSystem's loaders resolve file offsets to pointers in place, in 32-bit slots of the file buffer. Examples
  are `NNSi_G2dUnpackOffset` in `lib/nnsys/src/g2d/g2d_NCG_load.c` and the others, and SND's bank-to-wave-archive
  links (`SNDWaveArcLink`, `SND_SetWaveDataAddress`). On a 64-bit host, these resource headers need host-side
  structs or offset accessors.
- All asset data is little-endian, so it reads unchanged on x86-64 and ARM64 hosts.

---

## 3. What `data/` already provides

`docs/data.md` and `configure.py` (`DATA_PACKS`, `ARCHIVES`, `TEXT_ARCHIVES`) list them. Every archive below is
packed by its script, then `cmp`-checked against `extract/<v>/files/<path>` (the `check_file` rule) in matching
builds. **All of them round-trip byte for byte in both versions.**

| Archive(s) | Source in `data/` | Packer | Needs the ROM to pack? |
| --- | --- | --- | --- |
| `a/0/0/2`, `a/0/0/3` text | `data/text/{system,script}/*.txt` (1,171 files, message IDs, control codes, `\from{}` lines that pull names from the JSON) | `tools/text/text_data.py` | No |
| `a/0/1/6`, `0/1/7`, `0/1/8`, `0/1/9`, `0/2/0`, `1/2/4` species data, growth, learnsets, evolutions, babies, egg moves | `data/pokemon/<species>/data.json`, `growth_rates.csv` | `species_data.py` | No |
| `a/0/0/4`, `0/0/7`, `0/5/1`, `1/6/5`, `2/2/9` battle sprites, icons, musical sprites, footprints | indexed PNG, JASC `.pal`, raw `.ncer`/`.nanr.lz11`/`.nmcr`/`.nmar` | `pokemon_graphics.py` | **Yes**: patches the original |
| `swan_sound_data.sdat` (cries only), `a/1/9/1` cry volumes | `cry.wav` (mono PCM8), `cry_volume` in `data.json` | `pokemon_cries.py` | **Yes**: rewrites only the cry wave archives of the original SDAT. The rest of the SDAT isn't source |
| `a/0/2/1` moves, `a/0/2/4` items | JSON | `move_data.py`, `item_data.py` | No |
| `a/0/9/1`, `0/9/2`, `0/8/9`, `0/9/0` trainers, parties, message table | JSON | `trainer_data.py` | No |
| `a/1/2/7` encounters (per version), `a/0/1/2` zones, `a/1/2/6` zone events, `a/1/6/3` trades, `a/0/0/9` map matrices, `a/0/1/3` areas, `a/0/6/0-1` lights, `a/0/8/5` town map (per version) | JSON | the matching `*_data.py` | No |
| `a/0/0/8` all 1,065 map models with terrain and prop placements | glTF 2.0 `.glb`, terrain CSVs, `data.json` | `map_model_data.py` | **Yes**: reads the original Nitro model to keep command order and transforms |
| `a/2/2/5`, `a/2/2/6` field prop models | glTF | `field_prop_model_data.py` | **Yes** |
| `a/0/1/4` (409 texture sets), `a/1/7/4-5` prop textures | PNG | `map_texture_data.py` | **Yes**: recovers texture formats and palettes, quantizes to RGB555 |
| `a/0/5/6` field scripts, `a/1/6/9` TR AI | `.s` with macros in `include/asm/` | `as` + `narc.py` | No |
| `a/2/1/1-2`, `2/6/1-2`, `2/4/8`–`2/5/7` Battle Subway, Trial House, Black Tower/White Treehollow, PWT | JSON | `facility_data.py` | No |

- **Coverage.** 52 of 318 files: 97.1 MB, 34.2% of NitroFS bytes, or 186.5 MB (65.6%) counting the SDAT, of
  which only the cries are source.
- **Not yet source.**
  - Most UI and 2D graphics archives.
  - The 3D cutscenes, battle and field models (`a/1/5/8`, `a/0/1/1`, `a/1/1/6`, `a/0/4/8`).
  - The trainer sprites (`a/0/7/1-2`), the particles (`a/0/0/6`), the fonts, the cdemo frames, and all BGM and
    sound effect sequences, banks and waves.
  - The musical, Pokéstar Studios, C-Gear and Pokédex skins, and the battle animation scripts.
  - **Game data that lives in code**: tables compiled into C, such as the shop and move tutor tables
    (`src/ov036/scrcmd_shop.c`, which `species_data.py` parses for the tutor bits), and anything else in an
    overlay's `.rodata`.
- **For mods and randomizers.**
  - The JSON and text sources validate against schemas, are ordered by constant lists, and name their fields.
    They are a good mod format.
  - The packers are Python (standard library only, for the build's). A port can ship them as a "mod build" tool
    that runs against the user's ROM.
  - Data compiled into C won't carry over from a randomized ROM. A port should move those tables out to loadable
    data.
- **Licensing.** `data/` contains about 99,500 tracked files of converted Nintendo/Game Freak content (50k PNG,
  7.4k GLB, 690 WAV, JSON, text). A port release must not ship them. Build them at install time from the user's
  ROM, or keep them as a developer-only tree.

---

## 4. The ARM7

**Status.**
- Neither the ARM7 nor the ARM7i is configured in dsd (`config/b2_us/` and `config/w2_us/` have only `arm9/`),
  and nothing in `src/` or `lib/` is ARM7 code.
- The ARM9 side of every ARM7 service is decompiled: `lib/nitro/src/{snd,spi(mic,pm,tp),rtc,wm,nvram,card,pxi,os}`,
  `lib/twl` and `lib/nnsys/src/snd`. Its PXI tags (`lib/nitro/include/nitro/pxi.h`) are the full ARM9↔ARM7
  interface, and a checklist for the host:
  NVRAM 4, RTC 5, TOUCHPANEL 6, SOUND 7, PM 8, MIC 9, WM 10, FS 11, OS 12, CTRDG 13, CARD 14, FATFS 20,
  CAMERA 21, NWM 22, SCFG 23, SNDEX 24.

**Identification.**
- The ARM7's module parameters (at 0x5D4) hold `sdk_version` 0x0503757C, identical to the ARM9's: TwlSDK 5.3,
  relstep 30076. The middleware strings in the ARM9 and overlays are
  `[SDK+NINTENDO:DWC5.3.30054.20100706.1000_TWLDWC_5_3_PATCH1_SSLGDB4]`, `[SDK+NINTENDO:WiFi3.3.30052…]`,
  `[SDK+Nintendo:libVCT 1.4.1]`, `[SDK+UBIQUITOUS:CPS]`/`SSL` and `[SDK+NINTENDO:BACKUP]`.
- `strings` on `arm7.bin` finds almost nothing (6 strings, including `2.88.00`, probably the wireless firmware
  version). The NitroSDK SND driver's 33-entry LFO sine table sits at file offset 0x11468. It is not in the ARM9,
  which confirms that the sequencer runs on the ARM7.
- `arm7i.bin` contains the stock DSi wireless stack and storage code: `sdio_wlan`, `WMI CONTROL`,
  `HTC Recv PKT`, `wpa_supplicant v0.5.9-Devicescape`, `Devicescape Agent Supplicant 2.0.1`, `dragon driver`,
  FATFS paths (`nand:/sys/TWLFontTable.dat`, `nand:/shared2/`, `sdmc:/private/ds/app/`), and `TwlSDK\build\dspcomponents`
  paths in the LTD's DSP images.
- **Conclusion.** This is a stock TwlSDK 5.3 ARM7 component with a main-RAM load and wireless (TwlSDK's hybrid
  WM+NWM component, called "racoon" in the SDK; the name is inferred, not proven), not game code. Supporting
  evidence: identical in Black 2 and White 2, the SDK version stamp, and stock-only strings.

**What it does in BW2.**
- SND driver: the sequencer, tracks, channels, envelopes, LFO, alarms, capture and the SPU.
- WM: local wireless, MP and beacons.
- Touch panel sampling and calibration, the RTC, power management (backlight, sleep, battery, lid), the
  microphone, and NVRAM (the firmware's user settings, owner info, RTC offset and MAC).
- Backup SPI (flash) and card access, and OS/PXI glue.
- In DSi mode, the ARM7i adds NWM (Atheros Wi-Fi with WPA), FATFS (NAND and SD), camera I2C, SNDEX, TWL mic,
  the AES and random number source for photo signatures, and SCFG.

**What the port re-creates.**

| Service | Game users | Host replacement |
| --- | --- | --- |
| **SND driver** | everything audio (§5) | **Must be rebuilt**: a sequencer that executes SSEQ and a 16-channel software SPU with 2 capture units. Keep the decompiled ARM9 SND libraries (`lib/nitro/src/snd`, `lib/nnsys/src/snd`) and feed their command queue and shared work area to a native "ARM7 SND" module on the audio thread. The faithful option: add an ARM7 config to dsd and decompile its SND part, which is stock SDK code. pret's `pokediamond` decompiled its NitroSDK-era ARM7, a reference for structure only. Then emulate only the SPU, with melonDS/DeSmuME as references for PSG duty, noise LFSR, ADPCM clamping and volume/pan tables |
| RTC | `rtc_cache.c` (reads every few frames), seasons, day periods, daily events | Host local time plus a user-set offset (§6) |
| Touch panel | `src/gfl/touchpanel.c` and 24 files | Mouse or touch mapped to the 256×192 bottom screen. No calibration needed |
| PM | `src/gfl/ui.c`: battery level every 600 frames, backlight, `PM_GoSleepMode` on lid close; `PAD_DetectFold` | Battery from the host (or full), sleep mapped to pause or minimize, lid always open |
| MIC | Chatot's Chatter (`snd_mic.c`, 8,180 Hz), the mic test (ov253), the Xtransceiver and DWC voice chat | Host capture device, optional |
| NVRAM/firmware | `OS_GetOwnerInfo` (name, birthday, language), `OS_GetOwnerRtcOffset`, `OS_GetMacAddress` | A per-install config: a stable pseudo-MAC, birthday, name, language, an RTC offset |
| CARD backup | `src/gfl/backup_card.c` | The host save file (§6) |
| IR on the backup SPI | `lib/irc`, ov338 | §8 |
| WM / NWM / Download Play | Union Room, C-Gear wireless, beacons (Entralink, Join Avenue, Xtransceiver, Funfest), `ov181`/`ov182` Download Play parents, NitroWiFi over WM/NWM | Out of the asset scope. Replace at the GFL net device or WM/SOC API with LAN/Internet transports. `docs/porting-notes.md` covers SOC/SOCL/WCM. The Download Play child programs can't run on a PC (§9) |
| CTRDG | `CTRDG_DummyInit` only | No-op |

---

## 5. Sound

### The game's sound stack (all ARM9, decompiled)

- **NitroSDK SND** (`lib/nitro/src/snd/*`: command, interface, work, bank, alarm, util): sends commands and
  shared work to the ARM7.
- **NitroSystem SND** (`lib/nnsys/src/snd/*`): arc, arc_loader, arc_player, arc_stream, stream, waveout,
  player, heap, fader, capture, resource manager. All complete except `snd_arc_stream.c`.
- **The game's wrapper** (`src/system/`):
  - `snd_sys.c`: `GFL_SndInit` calls `NNS_SndInit`, creates a sound heap over `g_GFLSndHeap`, runs
    `NNS_SndArcInit(&sSndArc, "swan_sound_data.sdat", heap, FALSE)`, loads group 0 and calls `SNDEX_Init` on a
    DSi. It also handles BGM, sound effect players, BGM changes with fades in a loading thread, and per-frame
    `NNS_SndMain` and `NNS_SndUpdateDriverInfo`, which reads back each track's state.
  - `snd_bgm_stack.c` (a push/pop stack of BGM with its banks loaded), `pm_voice.c` (cries), `snd_strm.c`
    (the title stream), `snd_mic.c`, `field_sound*.c` and `event_sound.c`.
  - The **interactive sound system** in `src/system/iss_*`: per-BGM track switches, city units, zone fades,
    dungeon pitch shifts, and 3D sound. Its data is in `ARCID_ISS_*`.

### SDAT contents (`swan_sound_data.sdat`, 89,373,024 bytes, 5,197 FAT files, full SYMB names)

| Kind | Count | Size | Notes |
| --- | --- | --- | --- |
| SSEQ | 1,390 valid of 2,416 slots | 2.37 MB | 273 `SEQ_BGM_*`, 1,066 `SEQ_SE_*`, 48 `SEQ_ME_*` and others, 3 `SEQ_PV*` |
| SSAR | 0 | – | No sequence archives: every sound effect is its own SSEQ |
| SBNK | 1,949 valid of 2,290 | 0.48 MB | |
| SWAR | 1,949 valid of 2,290 | 71.6 MB | 8,714 waves: **8,024 IMA-ADPCM, 690 PCM8, 0 PCM16**. 499 `WAVE_ARC_PV*` cry archives |
| STRM | 1 (`STRM_TITLE`) | 14.66 MB | PCM16 stereo at 32,728 Hz, about 112 s, on the stream player (channels 0 and 6) |
| Players | 7 | – | `PLAYER_BGM` (mask 0xBFFE), `SE_SYS`, `SE_1`, `SE_2` and `SE_3` (each 0xF000 with a 10,200-byte heap), `SE_PSG` (0xB800), `BGM2` (0xFFFF). Sequences by player: SE_1 384, BGM 317, SE_2 497, SE_SYS 126, SE_PSG 59, BGM2 5, SE_3 2 |
| Player2 | 1 | – | `PLAYER_STRM` on channels {0, 6} |
| Groups | 5 | – | `GROUP_GLOBAL` (37 entries), `LCG01`, `LCG02`, `KYUREM02_B`, `KYUREM02_W` |

### What the game uses (`grep` of `NNS_Snd*` calls in `src/`)

- Sequences: `NNS_SndArcPlayerStartSeq(Ex)`, stop and pause, `SetVolume`, `SetPlayerVolume`, `MoveVolume`
  (fades), `SetTempoRatio`, `SetTrackVolume`, `SetTrackPitch`, `SetTrackPan`, `SetTrackMute(Ex)`,
  `SetTrackModDepth` and `ModSpeed` (the interactive sound system and dungeon pitch), `NNS_SndPlayerGetTick`,
  `CountPlayingSeq*`, `ReadDriverTrackInfo`, `NNS_SndSetMasterVolume`, and `NNS_SndSetMonoFlag` (the options'
  mono/stereo setting).
- **In-memory substitution.** `snd_sys.c` `func_020064b8` points a SDAT slot's bank and sequence at data in RAM
  (`NNS_SndArcSetFileAddress`). `func_02006528` swaps a wave pointer inside a loaded wave archive
  (`SND_SetWaveDataAddress`). The musical (`ov209/sta_acting.c`) uses both for program music from `a/1/1/2-5`
  or a downloaded program (the 96 KiB `MusicalDist` extra save). **The host sequencer must accept SSEQ, SBNK and
  SWAR from arbitrary memory and patched wave pointers.**
- **Wave output** (`NNS_SndWaveOut*`): the cries (`pm_voice.c`) are PCM8 wave archives read whole and played on
  locked channels, with a second channel for the chorus, plus pitch and speed changes. Chatot plays its
  recording, at a random speed.
- **Streams**: `NNS_SndArcStrm*` for the title theme (prepared start, current position). `NNS_SndStrm*` for raw
  streams: Xtransceiver voice (ov257 `ctvt_mic.c`, IMA-ADPCM via `ima_adpcm.c`) and DWC voice chat
  (ov029 `dwc_rap_vchat.c`, libVCT).
- **Capture and effects.** NNS capture is linked and initialized by `NNS_SndInit`, but no game code starts
  reverb, effects or sampling (`NNS_SndCapture*` has no callers). **No reverb or capture parity is needed.**
- **Mic**: 8,180 Hz recording. On a DSi it adds `SNDEX_SetIirFilter`.

### What a host driver must support for parity

1. The DS channel model:
   - 16 channels: PCM8, PCM16 and IMA-ADPCM on any channel, PSG square (duty 0–7) on 8–13, noise on 14–15.
   - Allocation by the player's channel mask and priority, with stealing, and locked channels for wave output and
     streams.
   - Per-channel volume with the "data shift" divider, 7-bit pan, timer-based pitch and hold.
2. The SSEQ sequencer:
   - Every command: notes with gate and wait, program change, pan, volume, expression, pitch bend and range,
     transpose, tie, portamento, sweep, attack/decay/sustain/release overrides, LFO (vibrato, tremolo, pan;
     depth, speed, range, delay), priority, tempo, jumps and calls, loops.
   - Variables, random and conditional (`if`) commands, and the mute modes.
   - Up to 16 tracks per sequence, with per-track and per-player parameters settable at any time.
   - Tempo ratio, and tick and track info readback.
3. The driver's update rate: a fixed tick of about 5.2 ms (the NitroSDK driver's interval). The output runs at
   the DS's about 32.73 kHz (the STRM's 32,728 Hz). Resample to the host's 48 kHz.
4. Faithful tables: decibel-to-volume, the attack, decay and release curves, the LFO sine table, and the ADPCM
   step and clamp behaviour.
   - The DS SPU does no interpolation. Offer an "accurate" mode alongside a smoothed one.
5. Streams with SND alarms, wave output, the sound heap's save and load levels (keep the NNS heap; it is ARM9
   code), master volume, mono, and the sleep hooks.
6. Optional: the mic for the Chatot recording, the mic test and voice chat, and DSi SNDEX, which can be a
   no-op.

---

## 6. Saves, RTC and firmware settings

### Backup chip

- `identifyGameCardBackup` (`src/gfl/backup_card.c`) asks for `CARD_BACKUP_TYPE_FLASH_4MBITS_EX`
  (= `CARD_BACKUP_TYPE_DEFINE(FLASH, 19, 0xff)`): 512 KiB of flash, of size 1 << 19.
- The cartridge's IR chip shares the backup SPI bus (§8).
- Reads use `CARD_ReadBackup(Async)`. Writes use `CARD_WriteAndVerifyBackupAsync`. Erasing a save
  (`func_0203adb4`) writes 0xFF in 0x1000-byte chunks.

### Layout (`src/system/save_table.c`, `src/gfl/backup_system.c`)

- **Main save**: `SAVE_MAIN_ADDR` 0x0, `SAVE_BACKUP_ADDR` 0x26000, `SAVE_SIZE` 0x26000, `SAVE_MAGIC`
  0x31053527.
  - Its blocks, in table order: the box index, 24 boxes, bag, party, trainer data, player, Unity Tower survey,
    Wi-Fi list, Pal Pad, C-Gear, badge times, Mystery Gift, Dream World, Chatot chatter, adventure, records, easy
    chat words, mail, field actors, musical, Reshiram/Zekrom, trainer log, event work, GTS, tournament, gimmicks,
    Battle Box, day care, strength rocks, trainer game info, Hidden Link, Pokédex, encounters, Battle Subway
    play/score/Wi-Fi, record, symbol encounters, survey, Unity Tower floors, Trial House, Hall of Fame team,
    traded Pokémon, misc (rival), Join Avenue, medal box, key system, Funfest, Pokéstar Studios, Dream Radar.
  - Each block is followed by a `BlockFooter {u16 count; u16 crc;}`. A table of block CRCs comes next, then a
    0x10-byte `SaveFooter {u32 count; u32 size; u32 magic; u16 unk; u16 crc}`, whose CRC covers the CRC table.
  - The CRCs are **CRC16-CCITT** (`MATHi_CRC16InitTable(0x1021)` and `MATH_CalcCRC16CCITT`).
  - **Two copies.** A load takes each block from whichever copy has it valid (`OK_PARTIAL`) and picks the newer
    one by `count`.
  - A save writes only the blocks that differ from the card, comparing 0x800 bytes per frame. It writes the main
    copy's blocks, CRCs and footer, writing the footer's last byte last, which makes that copy valid, and then
    does the same for the backup.
- **Extra saves** (`sv_extra_blk_data`), one copy each unless noted:

  | Address | Size | Contents |
  | --- | --- | --- |
  | 0x4C000, 0x4DA00, 0x4F400, 0x50E00 | 0x1A00 each | 4 battle videos |
  | 0x52800 | 0x2800 | C-Gear skin |
  | 0x55000 | 0x800 | battle examination |
  | 0x55800 | 0x18000 | musical distribution |
  | 0x6D800 | 0x6800 | Pokédex skin |
  | 0x74000 | 0x1800 | Hall of Fame, the **only extra with a backup**, at 0x75800 |
  | 0x77000–0x79A00 | 0x600 each | 8 Pokéstar Studios slots |
  | 0x7A000–0x7C800 | 0x1400 each | 3 tournament slots |
  | 0x7DC00 | 0x400 | one more block |

  The last 0x2000 bytes (to 0x80000) are unused.

### Host save mapping, for interoperability

- Keep a **raw 524,288-byte image** byte for byte as the flash, with 0xFF meaning erased, and one file per game
  code (`IREO` and `IRDO`). Implement the ARM9 CARD backup API, or `backup_card.c`'s helpers, over it.
- Persist when a write completes. Write to a temporary file and rename it, or fsync. The game's own commit
  point, the footer's last byte, already makes interrupted saves recoverable.
- **Import and export.**
  - melonDS `.sav` is the raw 512 KiB.
  - DeSmuME `.dsv` is raw plus a 122-byte trailing footer: strip it, and add it back on export.
  - NO$GBA saves may be in its compressed container, which needs decoding.
  - Real-cart dumps from Checkpoint, GodMode9 and similar tools are raw.
  - Validate an imported save with the same footer, magic and CRC rules. A 3DS Dream Radar transfer writes into
    the cartridge save, so such a save imports as is.
- **Console-bound fields.** `setAdvTimeBlkRtcOffsetOwnerMacBdayMonthDay`
  (`src/system/adventure_savedata.c`) stores the firmware's RTC offset, MAC and birthday in the adventure
  block.
  - On continue, `hasClockNotBeenTampered` compares the stored offset with the console's
    (`ov035/event_mapchange.c`). If they differ, the game calls `setNewDayForCountdown`, which restarts the
    daily-event countdowns (the "24-hour lockout").
  - **When importing, set the port's virtual firmware RTC offset (and MAC) to the save's values**, so a moved
    save isn't penalized.

### RTC and time

- `src/gfl/rtc_cache.c` caches `RTC_GetDateTimeAsync` every few frames. `src/system/rtc.c` maps hours to day
  periods per season (a table of start hours by season), and `season.c` gives
  `season = (month − 1) % 4`.
- The clock drives seasons, day and night lighting, daily events, the play time and save time
  (`playtime.c`), and time-stamped records.
- Port: use host local time plus a user-controlled offset in settings. Changing that offset should change the
  virtual `OS_GetOwnerRtcOffset`, as changing the DS clock does, so the game's own anti-cheat semantics stay.
  Optionally offer a "lock season" debug mode.
- Other firmware values: owner name, birthday and language (the birthday greeting, DWC), and the MAC (beacon
  and Union identity, stored in the save). Generate a stable pseudo-MAC per install, or take it from the
  imported save.

---

## 7. DSi (TWL)

**Linked TwlSDK modules** (`lib/twl`):
- ARM9 main: `scfg/scfg_proc*.c`, `syscall_twl.c` and `card_rom_twl.c`.
- LTD (ARM code): `camera/camera*.c`, `dsp/*` (plus two Teak DSP COFF images, G.711 audio and a graphics
  process), `mic/mic_twl.c`, `nwm/nwm.c`, `fatfs/fatfs_api.c`, `snd/sndex.c`, `rtc/rtc_api_ex.c`,
  `syscall_twl_ltd.c`, and the photo signature (`photo_sign*.c`).
- Overlay 257: SSP (`ssp_jpegenc/dec.c` and `ssp_exifenc/dec.c`).
- On the ARM7i: NWM's Atheros driver, WPA and FATFS.

**What BW2 actually does in DSi mode** (`OS_IsRunOnTwl()` call sites):
- **Xtransceiver video chat** (`src/ov257`):
  - `camera_system.c` captures from either camera into an NDMA ring of buffers. Through the DSP it plays the
    shutter and recording sounds that a DSi must play, from the WAVs in `a/2/0/9`.
  - `ctvt_comm.c` sends frames as SSP JPEG (128×192, YUV422), with photo signatures.
  - `ctvt_game_cam*.c` runs the camera minigame.
  - It is gated by `canPlayerExchangePhotos()` (parental controls, `src/system/dsi.c`) and `OS_IsAgreeEULA()`.
- **DSi Wi-Fi**: WPA/WPA2 access points through NWM/WCM in DWC. On a DSi the in-game WFC settings
  (`DWC_StartUtility`) refuse to run, since the system settings own the configuration (`ov182/dwc_raputil.c`,
  `ov190/wifilogin.c`).
- **Battery**: the DSi's 5-level `PM_GetBatteryLevel` drives the C-Gear's and info window's meters
  (`gfl/ui.c`, `infowin.c`, `c_gear_sw.c`).
- **Heaps**: 0x48AE00 instead of 0x1A3E00 for heap 1, and heap 9 is a child heap instead of an OS-heap block
  (`gfl_use.c`).
- **Mic**: `SNDEX_SetIirFilter` for the Chatot and Xtransceiver mic. `SNDEX_Init` at startup.
- **Beacons** carry an `isDSi` flag (`ov012/game_beacon_search.c`).
- **System**: power-button and exit requests go to the LTD (`machine_use.c` veneers), and SCFG is set up.

**Recommendation.**
- Ship with `OS_IsRunOnTwl() == FALSE` (NITRO behaviour). That turns off the camera, DSP, SSP, SNDEX, NWM and
  FATFS paths and keeps the NTR heap budget the game was tuned for. On a 64-bit host the heap sizes need a
  re-check anyway, since structures grow.
- Implement only the parts the NTR path also needs.
- Optional stretch goal: a "TWL features" flag that enables Xtransceiver video through a host webcam, host JPEG
  (libjpeg-turbo in place of SSP), host audio for the shutter WAVs, and a stubbed photo signature, with
  `canPlayerExchangePhotos` driven by a setting.
- Don't emulate NWM. Wi-Fi goes through host sockets in either mode.

---

## 8. IR (`lib/irc`) and its callers

- **Library.** `lib/irc/src/irc_spi.c` talks to the cartridge's IR chip through the backup SPI bus: a command
  byte (1 to read, 2 to write), then the bytes, with 60 µs pauses. `irc_link.c` frames the data between two
  machines: `IrcLink_Reset`, `Ping`, `IsParent`, `IsLinked`, `Send(data, size, cmd)`, `Poll`,
  `SetRecvCallback` and `GetPeerId`. Both are built into overlay 31 at `-O2,p`.
- **GFL net device.** `src/ov031/net_devirc.c` (a `GFLNetDevTable`) and `net_irc.c` (`NetIrcSys`: packets,
  turn-taking, timeout, peer filter). `src/gfl/net_irc_wireless.c` collects MAC addresses over IR so that the
  wireless can take over.
- **Game users.**
  - The **Feeling Check / compatibility checker**: ov175 `irc_compatible.c`, ov176 menu, ov177 sync check,
    ov178 rhythm check, ov179 result, ov180 ranking.
  - **IR battles** (`ov012/event_ircbattle.c`), and the **C-Gear's infrared marker** (`ov079/c_gear_sw.c`),
    which starts the IR features.
  - The IR-to-wireless handoff. Its identifiers also appear in the battle match overlays (ov263, ov290), whose
    use of it is unverified.
  - **Anti-piracy**: `ov338/ir_check.c` `IrCheck_IsGenuineCard` sends command 0x08 and expects 0xAA.
    - In battle (`ov167/btl_main.c` `func_ov167_0219bdfc`), a failed check skips the post-faint processing in
      `btl_server_flow.c`: the classic "no experience" anti-piracy.
    - In `system/game_beacon.c`, a failed check installs an HBlank task that hides beacons.
- **Port.**
  - Replace `irc_spi.c` (or `IrcLink_*`) with a host transport: a "virtual IR" pairing two port instances over
    LAN or the Internet with a short code. Keep `irc_link` framing and `NetIrcSys` timing above it, with relaxed
    timeouts.
  - Make `IrCheck_IsGenuineCard` return TRUE.
  - Stub **DS Protect** (`lib/dsprot`, overlays 165 and 337, called from `game_system.c`, `event_battle.c`,
    `pokelist.c` and `event_mapchange.c`). It decrypts and checksums its own ARM code, which is meaningless
    natively.

---

## 9. First-launch extraction design

### Verify

1. **Accept a `.nds` file and read its header.** It must be game code `IREO` (Black 2) or `IRDO` (White 2), and
   unit code 2.
   - Reject other regions for now. The decomp builds only `b2_us` and `w2_us`, and other regions' code differs.
     The game codes would be `IRE?` and `IRD?`: J, F, D, I, S, K. Text is one language per cartridge, and the
     US/English ROM is "O".
2. **Fast path**: if the whole-file SHA1 is e51e6dfb8678a3d19dcd2a10691b96a569ca0abb (Black 2) or
   b5d7490be7b415b8f1e672a53e978a9cc667e56a (White 2), the ROM is verified.
3. **Robust path**, for trimmed, re-padded or secure-area-decrypted dumps:
   - Parse the FNT and FAT.
   - Check every NitroFS file against a **per-file SHA1 manifest** shipped with the port:
     `nitrofs_manifest.json` here, 318 files per version, 61 KB of hashes and no copyrighted data.
   - Only NitroFS matters, so accept any trim that keeps everything up to the FAT's maximum end (0x112A723C for
     Black 2, 0x1125443C for White 2). The ARM7i, ARM9i and DSi area aren't needed.
4. **Mismatches.** Report modified files by path. Offer to continue as "modified base ROM" only if every file
   still parses (NARC headers, SDAT); see Randomizers below.

### Extract and cache

- **Don't unpack 271 MiB to disk.** Memory-map the verified ROM (or copy it into the app's data directory once),
  keep its path and manifest ID, and serve files by their FAT ranges. NARC member access is offset arithmetic,
  and LZ decoding is cheap on a PC.
- Optional caches, keyed by manifest and port version, can be deleted:
  - GPU-ready textures (decoded NSBTX/NCGR into RGBA or BCn).
  - Decoded audio tables, if the mixer wants PCM in place of ADPCM.
  - Pre-decompressed cutscene frames (102 MB) only if profiling demands it.
- **Unusable files.**
  - The **ARM binaries in NitroFS**: `dl_rom/*.srl` (Download Play children) and `dwc/utility.bin` (WFC
    settings). Leave them unused.
  - **Poké Transfer**: import from a Gen 4 `.sav` (DP, Platinum, HG/SS) on the host, feeding the same data the
    child program would send.
  - **Memory Link**: read a Black/White `.sav` file and hand its data to the code that receives the child's
    save data (`mb_parent_dataconv_sys.c`).
  - **WFC settings**: a host network settings screen.
- **Icon.** Use the user's ROM's banner (`bitmap.png` and palette, or the animated DSi icon) as the app or window
  icon at runtime, rather than shipping one.

### What can come from `data/` instead of the ROM

- At **build time for developers**, the 52 archives that the packers rebuild byte for byte prove the formats and
  allow editing.
- At **runtime for users**, still read everything from the ROM. `data/` is Nintendo content and can't ship.
- The packers that need no ROM (text, species, moves, items, trainers, encounters, zones, events, trades,
  matrices, areas, lights, facilities, scripts) can run on mod sources alone. The graphics, model, texture and
  cry packers need the user's ROM as their base. Both kinds can run in a "mod build" step on the user's machine.

### Mod and override design (a layered VFS)

The resolver order is: enabled mods, in priority order; then the mod build cache; then the base ROM.

- **Whole-file override**: `mods/<id>/files/a/0/1/6` replaces the archive. Any NitroFS path can be replaced the
  same way, including `swan_sound_data.sdat`.
- **Member override**: `mods/<id>/files/a/0/1/6/<index>[.ext]` replaces one NARC member. The host ArcTool
  builds a virtual FAT in memory, so `ReadRange`, `GetDataOfs` and `Seek` keep working.
  - Since the **caller** chooses LZ decoding, the loader should sniff the member: if the call expects LZ and
    the member isn't LZ-headed (0x10/0x11 plus a size), pass it through; if it doesn't expect LZ and the member
    is LZ-headed with a valid size, decode it.
  - Better: the mod tool records the original's compression and re-compresses to match.
- **Sound overrides by SDAT symbol name** (the SYMB block is present):
  `mods/<id>/sound/SEQ_BGM_xxx.sseq`, `.sbnk`, `WAVE_ARC_xxx.swar` and `STRM_TITLE.strm`. The host patches the
  SDAT's FAT in memory, as `pokemon_cries.py` does on disk.
- **Text overrides by message ID**: `mods/<id>/text/system/<bank>.txt`, holding only the changed
  `ID = text` lines, merged by the decomp's text tool. This is robust against line reordering.
- **Source-level mods**: `mods/<id>/data/...` in the decomp's JSON, PNG, glTF or `.s` formats. The bundled mod
  tool (`tools/data` + `tools/text`) compiles them against the user's ROM into the mod build cache.
- **Code-resident data** (shops, tutors, anything in C tables) should become loadable data files in the port,
  with JSON overrides. Otherwise randomizers and data mods can't reach it.

### Randomizers

- Tools such as Universal Pokémon Randomizer ZX edit NARCs **and** ARM9/overlay bytes. A native port ignores
  the code edits.
- Provide a "**rom2mod**" importer: given the clean ROM and a randomized ROM, diff the NitroFS (per file, then
  per member) and write a mod folder. Warn that code-level patches (shops and other table edits in overlays,
  hard-coded starters if any) aren't carried over until those tables are data.
- Longer term, randomizers can target the port's mod format directly. Its JSON sources are friendlier than
  NARCs.

### Legal and packaging

- The release ships code, tools, the per-file hash manifest and empty mod folders. No `data/`, no `extract/`, no
  converted assets.
- Saves live outside the install, as raw 512 KiB images per game code, with import and export of `.sav` and
  `.dsv` (§6).
