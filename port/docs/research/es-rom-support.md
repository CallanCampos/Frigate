# Frigate with the Spanish Black 2 ROM (IRES): what differs and what support would take

Research of 2026-10-26, read-only, on a local clone at `f786fed993`. The question: what Frigate needs to run with the
Spanish ROM of Pokémon Black 2 (game code `IRES`) instead of the US/European English one (`IREO`) that the decomp
and the port are built for. Nothing here was extracted into the repository. The measuring scripts read the ROM in place
and wrote only to the session's scratchpad. Copies of the scripts, without any ROM data, are in the session group's
notes folder (`side-session-notes/…/es-rom-scripts/`, outside the repository). Each claim is marked **measured** (a
script read the bytes), **read** (from the decomp's code or configs) or **inferred**.

The ROM measured was `PNeg2_espa_fix_0.nds`: 512 MiB, SHA-1 `1942ea58fb78f8d85e1a28db82dc0b846e0f4e0a`, header CRC
valid (0x46C8). Its secure area is decrypted (`ffdeffe7…`), as in most dumps, so the secure-area CRC doesn't match.
The "(Fix)" in its name suggests an anti-piracy patch. Without a reference hash there is no telling what it changed,
but such patches touch ARM9 or overlay code, which Frigate never runs. The SHA-1 isn't indexed on the web, so it may
not be a clean No-Intro dump. This report calls the US reference "US": the decomp's `b2_us`, SHA-1 `e51e6dfb…`.

## 1. Header and modules (measured)

| | US (`b2_us`, from [assets-arm7-sound.md](assets-arm7-sound.md)) | IRES |
| --- | --- | --- |
| Title, maker, unit code | `POKEMON B2`, `01`, 2 (DSi hybrid) | same |
| ARM9 main in ROM / static footprint | 472,740 bytes / 628,544 (0x02004000–0x0209D740) | 472,768 bytes / 628,480 (to 0x0209D700): **64 bytes less** |
| Main `.bss` end | 0x0214F500 | 0x0214F4C0 |
| SDK (module params) | 0x0503757C, TwlSDK 5.3 | same |
| Autoloads (ITCM 0x13A0, DTCM 0xA0 + 0x20, 0x02400000 and 0x06898000 placeholders) | as in `config/b2_us` | same sizes and addresses |
| ARM7 / ARM7i | 167,812 / 291,064 bytes | same sizes (not hashed against US: no US bytes here) |
| FNT / FAT | 0x4F0 / 0x14B0 bytes, 662 FAT entries | same sizes; 0x200 earlier in the ROM |
| ARM9 overlay table | 344 overlays, 11,008 bytes, SHA-1 `2c345261…` | 344 overlays, 11,008 bytes, SHA-1 `21852ee6…` |

The overlay table has the same overlays and file IDs but other contents (measured against `config/b2_us/arm9/overlays/*/delinks.txt`,
taking an image's size as `.bss` start minus load address):

- **333 overlays load 0x40 lower**, 2 (ov186, ov215) 0x60 lower, and 9 at the same address (ov169 in VRAM, ov218–ov225
  at 0x02400000+). This is main's 64 bytes less, moving everything above it.
- **4 overlays change size**: ov165 (party list, `plist_*.c`) +32, ov185 (Easy Chat input, `pms_input_data.c`,
  `pmsi_initial_data.c`, `pmsi_search.c`) −32, ov257 (C-Gear Xtransceiver, `ctvt_*.c`) +32, ov262 (Wi-Fi battle
  match) +32. No `.bss` changes.

Compared with the two versions the decomp already builds, IRES is about as far from `b2_us` as `w2_us` is: White 2
also differs from Black 2 by 64 bytes in main, with 261 overlays shifted and 3 resized
([assets-arm7-sound.md](assets-arm7-sound.md#nitrofs)). This is **inferred** from the measurements: Spanish is a
small rebuild of the same code, not another code base.

### Where main's code differs (measured, then read)

Every word of IRES's decompressed main that is a Thumb function pointer was matched exactly against the US symbols
(`config/b2_us/arm9/symbols.txt`). The shift is 0 up to `GFL_WordSetSystemCreate` (0x020241E4) and −48 from
`wcharsncpy` (0x02024AD4) on. The 48 bytes are in **`src/system/wordset.c`** (0x020241D4–0x020249FC). Probing the
function starts there puts the losses in the functions that load names with an article or in the plural:
`loadPokemonTextNameToStrbuf` (`TEXT_BANK_SPECIES_NAMES_WITH_ARTICLE`), `loadItemTextNameToStrbuf`,
`loadItemsNameToStrbuf` and `loadItemText` (about −32), `loadTrainerTypeWithArticleToStrbuf` (−8 more), and the
loaders after `loadPassPowerToStrbuf` (−8 more). That is English grammar (a/an, plurals) that the Spanish build does
another way, or doesn't need. Main's other 16 bytes are further up, in data. The tail of `.rodata` shifts by −0x30,
with the end of the static image at −0x40. They weren't pinned down.

`const u8 region` (`src/system/version.c`, US 0x0208FF80, value 2) is **7 in IRES** (`LANGUAGE_SPANISH`), right
after `game_version` (23). The decomp's own comment calls it "region", but its value is the language ID, and many
overlays read it.

## 2. NitroFS (measured, against `nitrofs_manifest.json`)

The same 318 files, except that `dl_rom/child_r_eng.srl` and `dl_rom/child2_r_eng.srl` are called `…_spa.srl`.

- **252 files are identical**, 216 MiB of the 270: the sound archive, all of species, moves, trainers and their parties,
  encounters, zones, events, map models and textures, field actors, trainer AI and most graphics. Every packer-built
  archive in `res/` that IRES has is identical, except items and trades.
- **23 differ in size**, and **41 have the same size but other bytes**. Every one is a NARC with **the same number of
  members** as the US file.

| File | What it is | How it differs |
| --- | --- | --- |
| `a/0/0/2`, `a/0/0/3` | System and script text | See §3. 5.35 → 5.63 MB and 2.44 → 2.77 MB |
| `a/0/2/3` | `ARCID_FONT` | Same size, other bytes: Spanish glyphs |
| `a/0/2/4` | Item data | Only byte 15 of 209 of the 639 records: `sort_index`, the bag's alphabetical order |
| `a/1/6/3` | In-game trades | Only byte 88 of all 31 records: the language, 2 → 7 |
| `a/0/5/6` | Field scripts | +4 bytes, probably the script that shows the 8 messages Spanish adds (§3); not rebuilt to confirm |
| `a/2/2/2`, `a/2/2/7`, `a/2/1/5`, `a/1/5/9` | Opening frames, bitmaps, the ending, 3D demo 2D | Text in pictures; the opening keeps its 627 members |
| `a/0/2/6`, `a/0/3/4`, `a/0/3/8`, `a/0/4/2`, `a/1/1/9`, `a/2/8/7`, `a/0/8/3`, … (36 more 2D archives) | Title, start menu, Easy Chat keys (`ARCID_PMSI`), name input, C-Gear, place names, … | Graphics with text in them |
| `a/1/1/2`–`a/1/1/6`, `a/1/2/0`, `a/2/6/9` | Musical program sets and others with no `ARCID_*` name | Small changes, not studied |

The US side of items and trades comes from rebuilding the archives from `res/` with the decomp's packers, which
reproduced the manifest's SHA-1s exactly. The graphics packers need the extracted US files, which this clone doesn't
have, so the 2D archives were compared only whole.

## 3. Text (measured, against US archives rebuilt from `res/text`)

`tools/text/text_data.py pack` rebuilt both US text archives from `res/text`, matching the manifest. Then each file was
compared message by message:

- **The same 495 and 676 message files, one language section each, and the same number of messages in every file**,
  except:
  - the 26 Easy Chat word lists by initial, `pms_words_initial_a`–`_z` (A: 81 → 143, S: 264 → 146, W: 69 → 23, …),
    which are sorted by Spanish spelling;
  - `global_10735` (script plugin 9, the Black Tower and Trial House guides), 592 → 600: **the 8 extra messages are
    at the end**, so every message ID that the C code and the US scripts use still points at the same message.
- 74 system and 184 script files are byte for byte the same (names, numbers, control codes). 20,625 of 66,717 system
  messages and 7,142 of 13,089 script messages are longer in Spanish.
- The text is UTF-16 with the same control codes. Characters Spanish has that the English text doesn't use:
  `ª º ¿ Í Ñ Ó Ú` (system), and in scripts also `Á Ç É ß è ú ♀ ♂ ;` and some kana. The font that draws them is IRES's
  own `a/0/2/3`.

Sample: species names are the same (Bulbasaur…); items "Ultraball", "Superball"; moves "Destructor", "Golpe Karate",
"Doblebofetón"; natures lowercase, "huraña", "audaz".

## 4. The game's code against Spanish data (read)

Frigate runs the decomp's C compiled for the host, never the ROM's code, so different overlays don't matter in
themselves. What matters is whether the US C behaves correctly on Spanish data. The data layouts are the same (same
archives, member counts, record sizes and message counts, §2–3), so loading works. These are the places where the US
code is English:

| Where | What the US code does | Effect with Spanish data |
| --- | --- | --- |
| `include/constants/language.h`: `GAME_LANGUAGE LANGUAGE_ENGLISH` | The one switch for the build's language | Beacons, Union Room, GTS, Wi-Fi profiles and Easy Chat save flags say English |
| `src/system/version.c`: `region = 2` | The language stamped on Pokémon the player gets (IRES: 7) | Caught and hatched Pokémon say "English"; trade partners and later games see ENG |
| `src/ov181/mb_parent_sys.c:1019`, `mb_parent_dataconv_sys.c:310` | Opens `/dl_rom/child_r_eng.srl` and `child2_r_eng.srl` | The DS Download Play children don't exist in IRES, so that feature fails |
| `src/ov185/pms_input_data.c` `sInitialWords*`, `pmsi_initial_data.c`, `pmsi_search.c` | English tables of Easy Chat word IDs by initial; the search takes the count of words from the (Spanish) `pms_words_initial_*` file and indexes the English table with it | Easy Chat lists Spanish words under English initials. Where Spanish has more words, `PMSIData_GetInitialWordCode` returns 0. Not a memory error, but wrong lists. IRES's ov185 is 32 bytes smaller: other tables |
| `src/system/wordset.c` (article and plural loaders) | English article/plural logic (§1) | Probably small grammar slips in sentences with an item, a species or a trainer class. Spanish has the same `*_with_article` and plural banks |
| ov165, ov257, ov262 | US layout code (each 32 bytes larger in IRES) | Probably text positions or widths for longer Spanish strings: possible clipping in the party list, Xtransceiver and Wi-Fi battle screens |
| `src/ov034/union_msg.c:482`, `src/ov084/beacon_view_sub.c:249`, `src/ov182/dwc_raputil.c:39` | Literal English: the player's own greeting, a beacon check against 2, `DWC_LANGUAGE_ENGLISH` and `…_FOR_USA` for the Wi-Fi utility | Wrong language for the Union Room greeting and the DWC settings utility. `dwc/utility.bin` is identical, so it carries every language |

Not checked, to verify: the name-input keyboard (`ARCID_NAMEIN_GRAPHIC` differs; if its character tables are in
code, the US keyboard may lack Ñ and accented letters), the help and manual screens, and whether a save made by the
Spanish cartridge loads (the code is the same, so the layout should be; **inferred**).

The decomp only knows US English. `GAME_LANGUAGE` is defined once and never tested in `#if`, `configure.py` and
`config/` have only `b2_us` and `w2_us`, and [PLAN.md](../PLAN.md) doesn't mention regions or languages. The only
earlier statement is [assets-arm7-sound.md §9](assets-arm7-sound.md#9-first-launch-extraction-design): "Reject other
regions for now… other regions' code differs." This report shows that the differences are smaller than that sentence
implies.

## 5. Options

### A. Spanish data on the US code (a language pack, port only)

Frigate accepts IRES through a manifest of its own, and the game plays in Spanish with the US C.

- **Changes, all in `port/`:**
  - a `b2_es` entry in the manifest, of hashes only, written by a script that reads the ROM itself (today's
    `manifest.py` needs `extract/` and `orig/`);
  - `port/cmake/rom.cmake` and `port/hal/rom/rom.c`: accept a second game code with its manifest and overlay table
    hash (`21852ee6…`), choosing by the header's game code at startup;
  - the window title and messages naming the version. The overlay table is read from the ROM as now. IRES's is
    consistent in itself: the load addresses move together, and the 4 resized overlays don't overlap others
    differently (**inferred**; `gf_overlay.c`'s collision check would show it).
- **Cost:** small, about a day.
- **Risks:**
  - every row of §4: Pokémon tagged English, broken Download Play, wrong Easy Chat, small grammar and layout faults;
  - the replay and frame-hash tests (T3) stay US-only;
  - it conflicts with the plan's "one manifest of the decomp's ROM" and needs an ADR;
  - the "(Fix)" dump may not be clean, so a manifest made from it would accept only that dump.
- **What it isn't:** correct Spanish. It is the Spanish text with the US code's English rules.

### B. IRES as its own version of the decomp (`b2_es`)

A third version, like `w2_us`:

- `config/b2_es` through the version map, with `BLACK2` plus a language define;
- `GAME_LANGUAGE`, `region` and each site of §4 under `#if GAME_LANGUAGE == …`, with the Spanish tables taken from
  IRES's ov185 and the other three overlays;
- a matching ROM build against IRES's SHA-1.
- **Cost:** large, weeks. The configs for 344 overlays and main follow the version map as White 2's did. Writing the
  Spanish variants of the language-specific functions is decomp work: disassembling IRES, C that matches, and every
  incomplete file probed in a third version.
- **Risks:**
  - The port's rules forbid it here: no config changes or new versions in this clone, and decomp work happens in the
    decomp checkout.
  - Upstream builds only the US ROMs. A local `b2_es` would conflict with every sync of `config/` and `src/`.
  - A matching build needs a clean IRES dump with a known SHA-1.
  - It is the only way to the game exactly as the Spanish cartridge plays.

### C. A, then the code sites one by one as port variants

Start from A, and fix §4's rows under a host build option (`PORT_LANGUAGE=es` defining `GAME_LANGUAGE` as
`LANGUAGE_SPANISH` for the host build only), each a reviewed `PLATFORM_PC`-style change with its row in
[game-changes.md](../game-changes.md):

- `region` and the `GAME_LANGUAGE` sites: one define;
- the two `_eng.srl` paths: a string per language;
- ov185's Spanish Easy Chat tables: data read from IRES's ov185, written as C;
- `wordset.c` and the three layout overlays: only after disassembling IRES's versions.

- **Cost:** A plus a few days per row; Easy Chat and `wordset.c` are the expensive ones.
- **Risks:** these are behavior changes outside `port/`, beyond the three kinds the port allows
  ([port/CLAUDE.md](../../CLAUDE.md#changes-outside-port-planmd-41-48)), so the user has to decide on them, as an ADR.
  The ROM build must stay byte for byte, which is easy, because `GAME_LANGUAGE` stays English there.

### Others, not recommended

Translating the US ROM's text with a fan patch gives Spanish text on an `IREO` ROM, which Frigate refuses unless every
file matches. Running the ROM's own code isn't native ([PLAN.md §1](../PLAN.md)).

## 6. Recommendation and first step

Take **A as the first step, with C as the path to correct Spanish**, and keep B for the day upstream supports other
regions.

1. **First, concretely:** in a local branch, a script `port/tools/research/assets/rom_manifest.py ROM` that writes the
   manifest entry of any ROM (game code, header CRC, ROM SHA-1, every file's size and SHA-1, the overlay table's size
   and SHA-1) by reading the ROM's FNT and FAT, as this report's scripts do, with no extraction. Then a
   `PORT_ROM_LANGUAGE=es` CMake option that builds the manifest from that entry and lets `port_rom_check` accept
   `IRES`. Boot to the title screen and the first field, and write down what shows wrong. Check the dump first: a clean
   No-Intro IRES dump would give a stable SHA-1 to accept.
2. Then the cheapest rows of §4: `region` and `GAME_LANGUAGE` for the host build, and the `_spa.srl` paths. After
   that, Easy Chat's tables from IRES's ov185, and last, `wordset.c` and the layout overlays.
3. Each step: an ADR for accepting a second ROM and for the language variants, rows in game-changes.md, and the ROM
   build checked byte for byte.

## Resumen en español

La ROM española (IRES) es casi la misma que la US. Los dos ficheros de código son una recompilación pequeña: el ARM9
tiene 64 bytes menos, 333 overlays se cargan 0x40 más abajo y solo 4 cambian de tamaño. Los datos son compatibles con
el código US: hay los mismos 318 ficheros (salvo dos `.srl` que acaban en `_spa`), 252 son idénticos (216 de 270 MiB)
y todos los NARC distintos tienen el mismo número de miembros. Los textos tienen los mismos 495 + 676 ficheros con el
mismo número de mensajes. Las excepciones son las listas del Chat Fácil por inicial y 8 mensajes añadidos al final de
`global_10735`. En objetos solo cambia el orden alfabético y en los intercambios solo el idioma.

El código US del decomp sí tiene partes en inglés:
- `region` vale 2 (en la ROM española, 7);
- las rutas `_eng.srl` del Juego DS Descarga;
- las tablas del Chat Fácil en ov185;
- la gramática de artículos y plurales en `wordset.c`;
- algún ajuste de maquetación en tres overlays.

Recomiendo empezar por la opción A: un manifiesto propio para IRES y aceptar su código de juego, todo dentro de
`port/`. Así el juego se vería en español con el código US. Luego corregiría esas partes una a una con una opción de
compilación de idioma (opción C), con su ADR. La opción B, una versión `b2_es` completa del decomp, es el camino
exacto, pero es trabajo largo de decompilación que las reglas del port no permiten en este clon. Primer paso concreto:
un script que genere el manifiesto leyendo la ROM directamente y una opción `PORT_ROM_LANGUAGE=es`. Conviene comprobar
antes si el volcado "(Fix)" está limpio.
