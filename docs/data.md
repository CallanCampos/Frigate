# Game data

The game keeps most of its data in NARC archives under `files/a/`, one numbered file per entry. The archives that are
built from source have their sources in `data/`, and the build packs each archive from them and checks that it matches
the original, so editing an entry is as easy as editing its file, and the matching build proves that the sources say
exactly what the ROM holds. The sources come in three kinds:

- **JSON data**, as pokeplatinum's `res/`: one file per entity, such as `data/pokemon/bulbasaur/data.json`, with a
  JSON schema beside it that documents every field and that editors use for completion and checking. A script packs
  the archives from the files (`DATA_PACKS` in `configure.py`), validating each file against its schema and naming
  the file and field of any mistake. Files are ordered by the constant lists, not by their names, so adding an entry
  is adding its constant and its file; the build notices new, removed and renamed files by itself.
- **Assembly**, for the field and trainer AI scripts, which are code more than data, as pret keeps them: one source
  file per entry, in archive order, assembled with the macros in `include/asm/` (`ARCHIVES` in `configure.py`).
- **glTF 2.0**, for map models under `data/map_models/<model>/` and field prop models under
  `data/field_prop_models/`, each with a `data.json`. Their editable mesh and material data is packed back into Nitro
  models.
- **PNG**, for map textures under `data/map_texture_sets/<set>/` and field prop textures under
  `data/field_prop_textures/`. The packer converts edited pixels to each set's original Nitro texture format. Like the
  JSON, these directories are named after their constants and the lists give their order (see
  [Map model sources](#map-model-sources)).
- **Indexed PNG, JASC palettes, and WAV**, for each Pokémon's battle sprites, icons, footprints, and cries under
  `data/pokemon/<species>/`. Nitro cell and animation resources remain as separate binary files next to the images.

The JSON and assembly sources use the same constants as the C code (see [Constant lists](#constant-lists)): the JSON
by name, the assembly through the C preprocessor. Each archive has a script that wrote its sources from the original,
which documents the format and can write them again.

| Archive | Sources | Contents | Script |
| --- | --- | --- | --- |
| `a/0/0/2` | `data/text/system/` | System messages | `tools/text/text_data.py` |
| `a/0/0/3` | `data/text/script/` | Script messages | `tools/text/text_data.py` |
| `a/0/1/2` | `data/zones/` (JSON) | Zone headers | `tools/data/zone_data.py` |
| `a/0/1/6`, `a/0/1/8`, `a/0/1/9`, `a/0/2/0`, `a/1/2/4` | `data/pokemon/` (JSON) | Species data, level-up moves, evolutions, baby species, egg moves | `tools/data/species_data.py` |
| `a/0/1/7` | `data/pokemon/growth_rates.csv` | Experience tables of the growth rates | `tools/data/species_data.py` |
| `a/0/0/4`, `a/0/0/7`, `a/0/5/1`, `a/1/6/5`, `a/2/2/9` | `data/pokemon/<species>/` (PNG, palette, Nitro animation files) | Battle sprites, icons, musical sprites, and footprints | `tools/data/pokemon_graphics.py` |
| `swan_sound_data.sdat`, `a/1/9/1` | `data/pokemon/<species>/cry.wav`, `data.json` (`cry_volume`) | Pokémon cries and playback volumes | `tools/data/pokemon_cries.py` |
| `a/0/2/1` | `data/moves/` (JSON) | Move data | `tools/data/move_data.py` |
| `a/0/2/4` | `data/items/` (JSON) | Item data | `tools/data/item_data.py` |
| `a/0/5/6` | `data/field_scripts/` | Field scripts, see [Scripts](scripts.md#field-scripts) | `tools/script/field_script.py` |
| `a/0/9/1`, `a/0/9/2`, `a/0/8/9`, `a/0/9/0` | `data/trainers/` (JSON) | Trainers, their parties, and the table of their messages | `tools/data/trainer_data.py` |
| `a/0/0/9` | `data/map_matrices/` (JSON) | Map matrices | `tools/data/map_matrix_data.py` |
| `a/0/0/8` | `data/map_models/<model>/` (glTF 2.0) | All 1,065 map models | `tools/data/map_model_data.py` |
| `a/2/2/5`, `a/2/2/6` | `data/field_prop_models/{exterior,interior}/<bundle>/<model>/` (glTF 2.0) | Field prop models | `tools/data/field_prop_model_data.py` |
| `a/1/7/4` | `data/field_prop_textures/exterior/<bundle>/` (PNG) | Exterior field prop textures | `tools/data/map_texture_data.py` |
| `a/1/7/5` | `data/field_prop_textures/interior/<bundle>/` (PNG) | Interior field prop textures | `tools/data/map_texture_data.py` |
| `a/0/1/4` | `data/map_texture_sets/<set>/` (PNG) | All 409 field texture sets | `tools/data/map_texture_data.py` |
| `a/0/1/3` | `data/areas/<area>.json` | Area records | `tools/data/area_data.py` |
| `a/1/2/6` | `data/events/` (JSON) | Zone events: signs, NPCs, warps, triggers and init scripts | `tools/data/event_data.py` |
| `a/1/2/7` | `data/encounters/` (JSON) | Wild encounters | `tools/data/encounter_data.py` |
| `a/1/6/3` | `data/trades/` (JSON) | In-game trades | `tools/data/trade_data.py` |
| `a/2/1/2`, `a/2/1/1` | `data/facilities/battle_subway/` (JSON) | Battle Subway and Trial House trainers and Pokémon | `tools/data/facility_data.py` |
| `a/2/6/2`, `a/2/6/1` | `data/facilities/black_tower/` (JSON) | Black Tower and White Treehollow trainers and Pokémon | `tools/data/facility_data.py` |
| `a/2/4/8` to `a/2/5/7` | `data/facilities/pwt_*/` (JSON) | Pokémon World Tournament trainers, Pokémon and rental Pokémon | `tools/data/facility_data.py` |
| `a/1/6/9` | `data/tr_ai/` | Trainer AI scripts, see [Scripts](scripts.md) | `tools/script/tr_ai_script.py` |

The text archives are packed by `text_data.py` from text files rather than assembled; `configure.py` lists them in
`TEXT_ARCHIVES`.

## Constant lists

The ID constants that name the game's data, such as species, moves, abilities, items, types, sound sequences, trainer
classes, trainers, zones, wild encounter tables, in-game trades, natures, places, event flags and event variables, are
lists in `data/constants/`, one name per line. They are the source of truth: to add one, add it to its list, and to
rename one, use `tools/data/rename_constant.py OLD NEW`, which renames its uses too, and its data file where the data
is named after it. The build generates a header from each list, `constants/<list>.h` in `build/include/generated/`,
which is on the include path, so the C code, the data sources and the scripts all use the same names, and the generated
header can never disagree with its list. A line is a constant's full name, which takes the previous value plus one, or
`NAME = value` (decimal or `0x` hex); `#` starts a comment.

```
# Species, by national Pokédex number (bootstrapped from the ROM by make_constants.py)
SPECIES_NONE = 0
SPECIES_BULBASAUR
SPECIES_IVYSAUR
```

`tools/data/gen_constants.py` writes the headers, and its `load()` and `header_text()` give the scripts the same
constants without a build. The lists were written once from the game's own text, and from the sound archive's symbols,
by `tools/data/make_constants.py`, which can write them again; constants the game has no text for, such as
`ITEM_LAST` or `TYPE_NULL`, were added by hand. Constants whose names come from the code rather than from the game's
data, such as the battle and field script constants, stay hand-written headers in `include/constants/`.

Zones are named after their place name, which most zones share with others: the one the player can fly from, or else
the first, gets the place name alone (`ZONE_CASTELIA_CITY`); a Pokémon Center, gate, lab or gym, told by its music,
gets what it is (`ZONE_ASPERTIA_CITY_POKEMON_CENTER`, `ZONE_ASPERTIA_CITY_GYM`); and the rest are numbered from `_2` in
ID order (`ZONE_CASTELIA_CITY_12`). The code names a few itself, such as `ZONE_VICTORY_ROAD`, the one Escape Rope
leads out of (`ZONE_OVERRIDES` in `make_constants.py --zones`). A numbered zone is a placeholder: when its map shows
what it is, rename it.

`flags.txt` names every event flag and `vars.txt` every event variable, in their ranges, which the event work code
gives: the saved flags from 1, among them each trainer's from 0x5f0 (`EVENT_FLAG_TRAINER_LEADER_CHEREN`,
`TrainerFlagGet`), the ones set on first arriving at a place, which let the player fly there, from 0x9a5
(`EVENT_FLAG_ARRIVED_NUVEMA_TOWN`, from the town map's places and the respawn table), the daily ones from 0xaa0 to 0xb03
(`EventWork_ResetDailyFlags`) and the hidden items' from 0xb04 (`EVENT_FLAG_HIDDEN_ITEM_<n>`), then 64 temporary flags
from 0x4000; and the variables from 0x4000 to 0x41ae. The game
names none of them, so the rest are placeholders by value, as pokeplatinum's are (`EVENT_FLAG_0x0404`,
`EVENT_WORK_0x4150`): rename one with `rename_constant.py` when the code or a script shows what it does, and write the
field scripts and events again so that they use the name. `make_constants.py --flags` writes the lists again, keeping
the names given.

The members of an archive that the code loads by number get a list of their own, `narc_<archive>.txt`, as pret's
`.naix` names: the archive's name, then what the member is, then the kind of file, from its magic, such as
`NARC_INTRO_PROFESSOR_NCLR` in `narc_intro.txt`. The archives have no file names (one of 308 does), so a member is
named only when the code shows what it is, and the list holds only those, each with its value.

## Text

`a/0/0/2` (system messages) and `a/0/0/3` (script messages) hold the game's text, one message file per entry. Their
sources are `data/text/system/<file>.txt` and `data/text/script/<file>.txt`, UTF-8, with one message per line, in order.
Each line starts with the message's ID, `ID = text`, made of its bank's name and words of the message as unpacking first
wrote it; the build makes a header of each bank's IDs, `text/system/<bank>.h` or `text/script/<bank>.h`
(`tools/text/text_ids.py`), so the code and the scripts name messages, and adding or removing a line doesn't change
what they name. An ID can be renamed freely:

```
BlackCity2_Text_ListenUpTheresNothing = Listen up!\nThere's nothing wrong with making money!{be01}\nBut there are wrong ways to do it...
BlackCity2_Text_HowSeriousWillingGet = How serious are you willing to get\nin order to get what you want?
```

- `\n` is a line break within a message, and `\\`, `\{` and `\}` are a backslash and braces.
- `{TTTT}` or `{TTTT:a,b}` is a control code, its type in hex and its arguments: a placeholder for a name or number,
  a color, or `{be01}`, which waits for a button and scrolls.
- `\x{HHHH}` is a character that can't be shown as itself. Some messages end with `\x{ffff}`, an extra terminator that
  the original files have.
- A message that starts with `\c` is stored compressed, as the game stores trainers' names, among others.
- A last line `\pad{XX}` is not a message but the byte that fills the end of the file to a multiple of 4 bytes, which
  the original files have as leftovers rather than 0.
- A line `\from{species.name}` is not a message but stands for messages that come from the data in JSON, one per
  species, move or trainer, as pokeplatinum's text banks come from `res/`: see below.

The text that belongs to a species, a move or a trainer is in its JSON file, so renaming a Pokémon or rewriting a
trainer's lines is an edit in one place. The message files take it with `\from{...}` lines, which
`tools/text/text_sources.py` expands when the text is packed, among the messages that belong to nothing, such as
Egg and Bad Egg after the species' names:

| File | Line | Messages |
| --- | --- | --- |
| `species_names.txt` | `\from{species.name}` | Each species' `name` |
| `species_names_upper.txt` | `\from{species.name_upper}` | The same in capitals |
| `species_names_with_article.txt` | `\from{species.name_with_article}` | The same with "a" or "an" (`name_article` where the first letter doesn't tell) |
| `species_categories.txt` | `\from{species.pokedex.category}` | Each species' `pokedex.category` |
| `pokedex_entries.txt`, `pokedex_heights.txt`, `pokedex_weights.txt` | `\from{species.pokedex.entry}`, `\from{species.pokedex.forms.entry}` (and `height`, `weight`) | Each species' `pokedex.entry`, an empty line, then its alternate forms' `pokedex.forms`, in species order |
| `species_names_en.txt` to `pokedex_weights_ja.txt` | `\from{species.pokedex.<language>.name}` and the like | The other languages' `pokedex.languages`: names, categories, entries, heights and weights |
| `move_names.txt` | `\from{moves.name}` | Each move's `name` |
| `move_names_upper.txt` | `\from{moves.name_upper}` | The same in capitals |
| `btl_main_13.txt` | `\from{moves.description}` | Each move's `description` |
| `item_names.txt` | `\from{items.name}` | Each item's `name` |
| `item_names_with_article.txt` | `\from{items.name_with_article}` | The same with its article (`name_article`, or the whole `name_with_article` where it isn't written as the others) |
| `item_names_plural.txt` | `\from{items.name_plural}` | Each item's `name_plural` |
| `item_descriptions.txt` | `\from{items.description}` | Each item's `description` |
| `ability_names.txt` | `\from{abilities.name}` | Each ability's `name` |
| `ability_descriptions.txt` | `\from{abilities.description}` | Each ability's `description` |
| `type_names.txt` | `\from{types.name}` | Each type's `name` |
| `type_names_upper.txt` | `\from{types.name_upper}` | The same in capitals, the easy chat's types |
| `trade_names.txt` | `\from{trades.names}` | Each trade's `nickname` and `trainer_name` |
| `battle_subway_trainer_names.txt` | `\from{facilities.battle_subway.names}` | Each Battle Subway trainer's `name` |
| `battle_subway_trainer_messages.txt` | `\from{facilities.battle_subway.messages}` | Each one's three `messages` |
| `black_tower_trainer_names.txt` | `\from{facilities.black_tower.names}` | Each Black Tower trainer's `name` |
| `trainer_class_names.txt`, `trainer_class_names_with_article.txt` | `\from{trainer_classes.name}`, `\from{trainer_classes.name_with_article}` | Each trainer class's `name` and `name_with_article` |
| `natures.txt` | `\from{natures.name}` | Each nature's `name` |
| `place_names.txt` | `\from{places.name}` | Each place's `name` |
| `ability_names_upper.txt` | `\from{abilities.name_upper}` | Each ability's `name` in capitals, from ability 1 |
| `trainer_names.txt` | `\from{trainers.name}` | Each trainer's `name` |
| `trainer_messages.txt` | `\from{trainers.messages}` | Each trainer's `messages`, in the order of the trainer message table |

In the JSON, a line break is a real one (`"\n"` in the JSON) rather than `\n`; control codes and other escapes are as
in the text files. The other languages' Pokédex entries and categories, which the game keeps for older species, stay
text.

The game encrypts each message with a key that depends on its ID, which `text_data.py` applies when packing. The files
are named after their constants, which give their order in the archive: a system message file after its `TEXT_BANK_*` in
`data/constants/text_banks.txt` (`species_names.txt` for `TEXT_BANK_SPECIES_NAMES`), and a script message file after its
`SCRIPT_TEXT_*` in `script_text_banks.txt`, which names a zone's after the zone (`black_city.txt`) and a global script
file's after its first script ID (`global_10885.txt`). A system message file is named after what it holds, which the
word set function that loads it or its contents show (`move_names.txt`, `natures.txt`), or else after the source file or
function that loads it (`delete_save.txt`); the others keep their number until they are known (`0001.txt`), as
`TEXT_BANK_0001`. To rename one, rename its constant with `rename_constant.py`, which renames the file, its message IDs
and its header with it (`TEXT_BANK_0377` to `TEXT_BANK_TOWN_MAP` made `Bank0377_Text_FlyWhere` `TownMap_Text_FlyWhere`).
Both versions have the same text. The code names a message file by its constant too: `GFL_MsgSysLoadData(FALSE,
ARCID_SYSTEM_MESSAGE, TEXT_BANK_TRADE_NAMES, heapId)`, from `constants/text_banks.h` and `constants/script_text_banks.h`.

### Easy chat words

The easy chat (PMS) words that fill trainers' sentences and the mail are messages of thirteen files, numbered across them
in order (`pms_word.c`): the species' capitalized names up to the Bad Egg, the moves', the types' and the abilities',
then the words of `pms_words_trainer.txt`, `pms_words_people.txt`, `pms_words_greetings.txt`,
`pms_words_lifestyle.txt`, `pms_words_feelings.txt`, `pms_words_term.txt`, `pms_words_connection.txt`,
`pms_words_animated.txt` and `pms_words_voice.txt`, the input screen's categories of the same names
(`pms_categories.txt`). `tools/text/pms_words.py` makes `constants/pms_words.h` from them: each category's index,
first word and count (`PMS_WORD_CATEGORY_GREETINGS`, `PMS_WORD_FIRST_GREETINGS`, `PMS_WORD_COUNT_GREETINGS`), a
species', move's, type's or ability's word by its constant (`PMS_WORD_SPECIES(SPECIES_PIKACHU)`,
`PMS_WORD_MOVE(MOVE_THUNDERBOLT)`), and every other word by its category and message ID (`PMS_WORD_GREETINGS_HELLO`
for `PmsWordsGreetings_Text_Hello`; rename a word by its ID). The input screen's lists of words by category and by
initial (`pms_input_data.c`) and the code's other words use these names, so a word added to a file renumbers the words
after it everywhere; the input screen's lists are still edited by hand to offer it.

## Species

Each species has a directory in `data/pokemon/`, named after its constant (`bulbasaur/` for `SPECIES_BULBASAUR`), as
pokeplatinum's `res/pokemon/`. Its `data.json` holds everything the game keeps about it in five archives: its record of
the species data (`a/0/1/6`, the 0x4c bytes that `PML_PersonalGetParam` reads), its level-up moves (`a/0/1/8`), its
evolutions (`a/0/1/9`), its baby species (`a/0/2/0`) and its egg moves (`a/1/2/4`). `data/pokemon/species.schema.json`
documents each field.

```json
{
    "$schema": "../species.schema.json",
    "name": "Bulbasaur",
    "pokedex": {
        "category": "Seed Pokémon",
        "entry": "For some time after its birth, it\ngrows by gaining nourishment from\nthe seed on its back.",
        "height": "2'04\"",
        "weight": "15.2 lbs.",
        "languages": {
            "fr": {
                "name": "Bulbizarre",
                ...
            },
            ...
        }
    },
    "base_stats": {
        "hp": 45,
        ...
    },
    "types": [ "TYPE_GRASS", "TYPE_POISON" ],
    "abilities": [ "ABILITY_OVERGROW", "ABILITY_NONE" ],
    "hidden_ability": "ABILITY_CHLOROPHYLL",
    ...
    "learnset": {
        "by_level": [
            [ 1, "MOVE_TACKLE" ],
            [ 3, "MOVE_GROWL" ],
            ...
        ],
        "by_tm": [ "TM06", "TM09", ... ],
        "egg_moves": [ "MOVE_SKULL_BASH", "MOVE_CHARM", "MOVE_PETAL_DANCE", ... ],
        "by_tutor": [ "MOVE_GRASS_PLEDGE", "MOVE_SEED_BOMB", "MOVE_BIND", ... ]
    },
    "evolutions": [
        {
            "method": "EVO_METHOD_LEVEL",
            "param": 16,
            "species": "SPECIES_IVYSAUR"
        }
    ],
    "regional_dex_number": null,
    "baby_species": "SPECIES_BULBASAUR"
}
```

`tools/data/species_data.py pack` builds the archives, one entry per record, in this order:

- The species of `data/constants/species.txt`, from `SPECIES_NONE` (`none/`, an empty record) to Genesect, then
  `SPECIES_EGG` and `SPECIES_BAD_EGG` and the 33 Pokéstar Studios species, `SPECIES_POKESTAR_UFO` to
  `SPECIES_POKESTAR_SMEARGLE_2`: the props and costumes of the studio's movies, which the game keeps as species, with
  their own records, sprites, icons and cries (a Steel and Flying UFO with 600 base stats, the doors, the sixteen prop
  boxes). Not being Pokémon, they are in `data/pokemon/pokestar/<name>/`, such as `pokestar/ufo/`. Their names are lines of `species_names.txt` after the national Pokédex's, as the code names Bad Egg's
  message, and they have no Pokédex text, so their files have only their records and `cry_volume`.
- The alternate forms that have records of their own, `data/pokemon/<species>/form_<n>.json`, in the order of
  `data/pokemon/forms.json`. The packer sets each species' first form record from it, so a species' `forms` only
  gives the number of forms and where their sprites start. Species whose forms only differ in looks, such as Unown,
  have no form records.

The baby species archive stops before the forms, which have no `baby_species`. The species data archive ends with a
table of the species' Unova Pokédex numbers, which their files hold as `regional_dex_number`: its 301
numbers match the game's Unova Pokédex, from #000 for Victini to #300. The TMs and HMs a species learns are named, and
so are its tutor moves: the record keeps them as a bit per move of each tutor, and the packer finds each move's bit, the
type tutor's (the pledges, the starters' ultimate moves and Draco Meteor, in the order `PokeParty_GetTutorMoveID` tests
them) or its place in the shop table of the tutor of Driftveil City, Lentimas Town, Humilau City or Nacrene City in
`src/ov036/scrcmd_shop.c`. A mod that changes a tutor's table keeps every species' tutor moves. Three flags are named
after the species that have them: `underground` (Diglett and Dugtrio, tested by the battle animations), `asymmetric`
(species that don't look the same mirrored, such as Kingler and Absol) and `palette_forms` (Arceus, whose forms only
change its palette).

Evolutions are a method (`EVO_METHOD_*`), its parameter, and the species evolved into, at most seven. The parameter
depends on the method: a level, an item, a move, a species, or another value such as the beauty needed. The level-up
moves are in the order the game checks them, by level.

The experience tables (`a/0/1/7`) are `data/pokemon/growth_rates.csv`, a row per level from 0 to 100 and a column per
table, headed by its `GROWTH_*` constant; the two after the six rates, headed `EXTRA`, are copies of the first that
nothing names. Both versions have the same species data.

To add a species, add its constant to the end of `species.txt` and its directory with a `data.json`; to add a form with
a record of its own, add its `form_<n>.json` and its line to `forms.json`, and count it in the species' `forms`. A
species' name and its `pokedex` go into the text (see [Text](#text)): its category, entry, and height and weight as the
Pokédex shows them, which are text the game keeps apart from the record's numbers (the weights are rounded by hand, so
they can't be computed from them); its alternate forms' (`forms`, one for each form after the first, Unown's and Arceus'
too); and `languages`, the text the game keeps in English, French, German, Italian, Korean, Spanish and Japanese for the
species up to Arceus, which a Pokémon from a game in that language shows, with Giratina's and Shaymin's alternate forms'
sizes, and the Japanese category of every species. Existing species' graphics and cries are built from the files
beside `data.json`.

### Pokémon graphics and cries

Each species folder now keeps its battle graphics, icon, footprint, and cry next to its gameplay data, following the
per-species organization used by [pokeemerald expansion](https://github.com/rh-hideout/pokeemerald-expansion/tree/master/graphics/pokemon).
`front_single.png` and `back_single.png` assemble the game's four OBJ blocks into a 96×96 sprite.
`front.png` and `back.png` are 256×128 linear texture atlases for the animated sprite; their pieces are positioned
by the Nitro cell and multi-cell animation data. Optional female variants have `_female` in their names. The indexed PNGs carry the
original 4-bit color indices, while `normal.pal` and `shiny.pal` are the palettes used by the game. Edit palette
colors in the JASC `.pal` file; the PNG palette is for viewing in an image editor. The game assembles characters
into sprites using the adjacent `*_cells.ncer` and `*_animations.nanr.lz11` Nitro resources, so a raw PNG sheet can look
fragmented. Those resources are preserved as separate source files and can be replaced with edited Nitro data.

`icon.png` is a 32×64 two-frame icon, using a bank in
`data/pokemon/shared/icon_palettes.pal` (the PC boxes use `icon_palettes_box.pal`); the code in
`src/system/poke_icon.c` picks the bank, and the icons' cells and animations for each OBJ mapping are
`shared/icon_{cells,animations}_{32k,64k,128k}`. `footprint.png` is a 16×32 character sheet and uses
`data/pokemon/shared/footprint_palette.pal`, with the cells and animations of `shared/footprint_*_old` for the species
up to Arceus and `shared/footprint_*` for the rest (`zukan_info.c`).
The older musical sprite archive has its own `musical/` subfolder. Files identical to the regular sprite are omitted
there; the packer uses the regular file for those entries, so an edit of one shows in both. Sprite forms have
`form_<n>/` folders under their species, numbered by the forms' `sprite_offset` (`GetPokemonDataIDBase`); the musical
sprites' follow Black and White's species data instead (`MusicalMcss_GetDataIDBase`), so a form's `musical/` is where
it has one. `egg/` holds the egg's sprites and icon, which every egg shows, and `species_icon.png`, the icon file of
`SPECIES_EGG` itself; Manaphy's egg is `manaphy/egg/`. The Substitute doll is `shared/substitute/`
(`BtlvMcss_SetSubstitute`), and `shared/substitute/musical/` is Black and White's copy, which nothing loads.

`cry.wav` is mono PCM8 at the game's sample rate; the packer also accepts mono PCM16 edits and different lengths.
`cry_volume` in each species' `data.json` is the game's 0–127 playback volume; the egg, the bad egg and the
Pokéstar Studios species have cries and volumes of their own too (`pm_voice.c`). Forms with a separate cry have a `form_<n>/cry.wav` and use their species'
volume.
The other sounds in `swan_sound_data.sdat` retain their original data. Extract the graphics again with
`.venv/bin/python tools/data/pokemon_graphics.py extract data/pokemon` and the cries with
`.venv/bin/python tools/data/pokemon_cries.py extract data/pokemon`; the build packs them automatically.
An unchanged source set reproduces all seven original graphics and sound archives byte for byte.

## Moves

Each move has a directory in `data/moves/`, named after its constant (`thunderbolt/` for `MOVE_THUNDERBOLT`), whose
`data.json` is its record of the move data (`a/0/2/1`, the 0x24 bytes that `PML_MoveGetParamCore` reads).
`tools/data/move_data.py pack` builds the archive in the order of `data/constants/moves.txt`, and
`data/moves/move.schema.json` documents each field.

```json
{
    "$schema": "../move.schema.json",
    "name": "Thunderbolt",
    "description": "A strong electric blast is loosed at the\ntarget. It may also leave the target\nwith paralysis.",
    "type": "TYPE_ELECTRIC",
    "quality": "MOVE_QUALITY_DAMAGE_INFLICT",
    "category": "MOVE_CATEGORY_SPECIAL",
    "power": 95,
    "accuracy": 100,
    "pp": 15,
    ...
    "inflicts": {
        "condition": "CONDITION_PARALYSIS",
        "chance": 10,
        ...
    },
    "effect": "BATTLE_EFFECT_PARALYZE_HIT",
    "target": "MOVE_TARGET_SELECTED",
    "stat_changes": [],
    "flags": [ "MOVE_FLAG_PROTECT", "MOVE_FLAG_MIRROR_MOVE" ]
}
```

`effect` is the battle effect the move runs (`BATTLE_EFFECT_*`), and `inflicts` the condition it inflicts with its
chance and duration. `stat_changes` holds up to three changes of the stats of `BATTLEMON_*_STAGE`. `quality` is the
class of the move's effect (`MOVE_QUALITY_*`), `target` which Pokémon it targets (`MOVE_TARGET_*`) and `flags` its
properties (`MOVE_FLAG_*`), such as contact, sound or being blocked by Protect; `include/constants/battle.h` names
each after the moves that have it. The record's "SS" marker, which every move has, is written by the packer. Both
versions have the same move data. A move's name and description go into the text (see [Text](#text)).

## Items

Each item has a directory in `data/items/`, named after its constant (`potion/` for `ITEM_POTION`), whose `data.json`
is its file of the item data (`a/0/2/4`, the 36 bytes of `ItemData` in `include/pml/item.h`) and its text: its name,
its name with an article, its plural and its description (see [Text](#text)). `tools/data/item_data.py pack`
builds the archive in the order of `data/constants/items.txt`, and `data/items/item.schema.json` documents each field.

```json
{
    "$schema": "../item.schema.json",
    "name": "Potion",
    "name_plural": "Potions",
    "description": "A spray-type medicine for wounds.\nIt restores the HP of one Pokémon by\njust 20 points.",
    "price": 30,
    "hold_effect": "HOLD_EFFECT_NONE",
    ...
    "field_pocket": "BAG_POCKET_MEDICINE",
    "battle_pocket": [ "BATTLE_POCKET_HP_PP_RESTORE" ],
    ...
    "effects": {
        "sleep_heal": false,
        ...
        "hp_restore": true,
        ...
        "hp_restore_amount": 20,
        ...
    }
}
```

The price is in tens of Pokédollars, which `PML_ItemGetParam` multiplies by 10. An item has either a single `value` or
`effects`, what it does used on a Pokémon (`ItemParams`); the packer sets the record's work type from which. The pocket
outside battle is a `BAG_POCKET_*`, and its battle bag pockets are a list of `BATTLE_POCKET_*` flags. An item's article
is "a" or "an" by its first letter unless `name_article` says otherwise ("an HP Up", "the Leftovers", none for Honey).
The 20 unused items, named "???", have constants of their own, as `ITEM_UNUSED_113`. Both versions have the same item
data.

## Abilities, types, trainer classes, natures and places

An ability's text is `data/abilities/<ability>.json`, its name and description, and a type's `data/types/<type>.json`,
its name, named after their constants and in the order of their lists. What they do is the code's, so they have no
archive: only the text takes them (see [Text](#text)), and `tools/text/text_sources.py` validates them against
`ability.schema.json` and `type.schema.json`. `TYPE_NULL`, the type of a typeless move, has no name and no file. A
trainer class's text, `data/trainer_classes/<class>.json`, is its name and its name with an article, which the game
writes by hand ("a Twin" for Twins, none for the Gym Leaders), a nature's, `data/natures/<nature>.json`, its name,
and a place's, `data/places/<place>.json`, its name. Zone headers name their place by its constant in
`data/constants/places.txt` (`PLACE_ASPERTIA_CITY`), which `make_constants.py --places` named after the place names,
numbering the ones that repeat, as the second copies of Unova's places.
`tools/text/list_text_data.py` wrote them all from the game's text.

## Trainers

Each trainer is `data/trainers/<trainer>.json`, named after its constant (`elite_four_shauntal.json` for
`TRAINER_ELITE_FOUR_SHAUNTAL`), as pokeplatinum's `res/trainers/data/`. It holds the trainer's record (`a/0/9/1`) and
its party (`a/0/9/2`), and `tools/data/trainer_data.py pack` builds both archives in the order of
`data/constants/trainers.txt`. `data/trainers/trainer.schema.json` documents each field.

```json
{
    "$schema": "trainer.schema.json",
    "name": "Shauntal",
    "class": "TRAINER_CLASS_ELITE_FOUR_SHAUNTAL",
    "battle_style": "BTL_STYLE_SINGLE",
    "items": [ "ITEM_FULL_RESTORE" ],
    "ai_flags": [ "AI_FLAG_BASIC", "AI_FLAG_EVAL_ATTACK", "AI_FLAG_EXPERT" ],
    "heals": false,
    "money": 30,
    "reward": "ITEM_NONE",
    "party": [
        {
            "species": "SPECIES_COFAGRIGUS",
            "form": 0,
            "level": 56,
            "difficulty": 200,
            "gender": 0,
            "ability": 1,
            "item": "ITEM_NONE",
            "moves": [ "MOVE_WILL_O_WISP", ... ]
        },
        ...
    ],
    "messages": [
        {
            "type": 0,
            "text": "..."
        },
        ...
    ]
}
```

A trainer's `name` and `messages` go into the text (see [Text](#text)). The messages are also the trainer message table
(`a/0/8/9`), which pairs each with its trainer and type (0 before the battle, 1 when the trainer loses, 8 when it wins,
and other moments by number), and its offsets for each trainer (`a/0/9/0`), which `TrainerMsg_Load` reads; the packer
writes both, with the trainers in the order of `data/trainers/message_order.json`, then any others that have messages. A
party either gives every Pokémon an `item` or none, and `moves` or none; the packer sets the record's party kind
(`PARTY_ITEMS`, `PARTY_MOVES`) from that, and a Pokémon without moves gets the moves of its level. `ai_flags` are the
trainer AI scripts to run (`AI_FLAG_*`, see [Scripts](scripts.md)), `money` a multiplier of the prize money and `reward`
an item given after the battle. A Pokémon's `difficulty` sets its individual values, and `gender` and `ability` pick
them when not 0. `class` is a `TRAINER_CLASS_*` from `data/constants/trainer_classes.txt`, named after the class's name;
where several classes share one, after their only trainer, their sex or their ID, as `TRAINER_CLASS_SCHOOL_KID_F`
(`make_constants.py --trainer-classes`). `TRAINER_NONE` has no file: the packer writes the empty placeholder the game
has for it. Both versions have the same trainers.

Each trainer's constant in `data/constants/trainers.txt`, which the field scripts use, is its class and name, as
`TRAINER_YOUNGSTER_JIMMY`, without the class for the story characters, whose class is Pokémon Trainer
(`TRAINER_CHEREN`), and numbered from `_2` where a class and name repeat, as for rematches (`make_constants.py
--trainers`). To add a trainer, add its constant to the end of the list and its file.

## Wild encounters

Each wild encounter table (`a/1/2/7`, which a zone's header names, `GetZoneEncID`) is `data/encounters/<table>.json`,
named after its constant in `data/constants/encounters.txt` (`striaton_city.json` for `ENCOUNTERS_STRIATON_CITY`),
which names the tables after the place of the first zone that has them. `tools/data/encounter_data.py pack`
builds each version's archive in the order of the list, and `data/encounters/encounters.schema.json` documents each
field.

Black 2 and White 2 have different encounters in many places. A table's rates, and each group of its slots, is either
the same for both versions or an object with one for each:

```json
{
    "$schema": "encounters.schema.json",
    "all_year": {
        "rates": {
            "grass": 0,
            ...
            "surf": 10,
            "fishing": 50,
            ...
        },
        "grass": [],
        ...
        "surf": {
            "black2": [
                [ "SPECIES_BASCULIN", 45, 60 ],
                ...
            ],
            "white2": [
                [ "SPECIES_BASCULIN", 45, 60, 1 ],
                ...
            ]
        },
        ...
    }
}
```

A table (`EncData`) has a rate for each group of slots, the table's flags, and the slots: up to 12 each for grass,
dark grass and shaking grass, and up to 5 each for surfing, rippling water, fishing and rippling fishing; the packer
fills the rest with empty ones. A slot is a species, its lowest and highest level, and its form when not 0. A table is
`all_year`, or has four, `spring`, `summer`, `autumn` and `winter`. To add a table, add its constant to the end of the
list and its file, and name it in a zone's header.

## Zone headers

Each zone's header is `data/zones/<zone>.json`, named after its constant (`aspertia_city_gym.json` for
`ZONE_ASPERTIA_CITY_GYM`). `tools/data/zone_data.py pack` puts the 615 headers, 0x30 bytes each, into the single
entry of `a/0/1/2` in the order of `data/constants/zones.txt`, and `data/zones/zone.schema.json` documents each field
and the code that reads it.

```json
{
    "$schema": "zone.schema.json",
    "map_type": 16,
    ...
    "scripts": 2,
    "text": 4,
    "bgm": "SEQ_BGM_POKECEN",
    "encounters": null,
    "parent": "ZONE_BLACK_CITY",
    ...
    "fly_from": false,
    ...
}
```

A header names the zone's map, its scripts and script messages, its music (`SEQ_*`, or one for each season), its wild
encounter table (an `ENCOUNTERS_*` constant, or null), the zone it belongs to, its place (`PLACE_*`), its default
weather and camera, its battle background, what it allows (cycling, Escape Rope, flying from it), and where flying
lands. Both versions have the same zone headers.

## In-game trades

Each in-game trade is `data/trades/<trade>.json`, named after its constant in `data/constants/trades.txt`, which names
the trades after the species they offer (`petilil.json` for `TRADE_PETILIL`). It holds the offer that
`EventFieldTrade_CreatePkm` makes the Pokémon from (`a/1/6/3`, `FieldTradeOfferData` in `include/field/fld_trade.h`)
and the Pokémon's nickname and trainer's name, which the text takes (see [Text](#text)).
`tools/data/trade_data.py pack` builds the archive in the order of the list, numbering each offer and its two
names' message IDs from its place, and `data/trades/trade.schema.json` documents each field: the IVs, nature and sex
are null for random ones. The field scripts name the trades they start, as `FieldTradeCheck 0x8022, TRADE_GIGALITH,
0x8020`. Both versions have the same trades.

## Battle facilities

A battle facility's trainers and the Pokémon sets they pick from are in `data/facilities/<facility>/`, as
pokeplatinum's `res/trainers/frontier/`: `trainers/<trainer>.json` has a trainer's class, its sets by name, and its
name and messages, which the text takes (see [Text](#text)); `pokemon/<set>.json` is a set (`BSubwayPokemonData` in
`include/field/battle_facility.h`), named after its species and numbered from 1, as `gengar_4.json`; and `order.json`
lists both in archive order. `tools/data/facility_data.py pack` builds the facility's trainers and Pokémon
archives, and `data/facilities/facility.schema.json` and `set.schema.json` document the fields.

`FACILITIES` in `facility_data.py` says which archives and message files are a facility's: the Battle Subway (`a/2/1/2`
and `a/2/1/1`, whose trainers' names are system message file 15 and their three messages each file 376), which the Trial
House also uses (`func_ov012_02162864`); the Black Tower, White Treehollow in White 2 (`a/2/6/2` and `a/2/6/1`, which
`func_ov127_021efeec` reads, with the trainers' names in file 53, stored compressed, and no messages); and the Pokémon
World Tournament. Its tournaments each pick one of three pools of trainers by `WbtTournamentInfo`'s `unk5`, from the
base archives 248, 251 and 254 (`wbt_tool.c`): `pwt_regular` (Driftveil, Download, Rental and Mix), `pwt_leaders` (the
Unova, Kanto, Johto, Hoenn and Sinnoh Leaders) and `pwt_masters` (Champions, World Leaders, Type Expert, Rental Master
and Mix Master). A pool's base archive gives each trainer a `single_set`, which `func_ov134_021f0258` uses when the
entrant's `unk0_7` is set, the next its sets to pick from, and the one after the sets. The pools' trainers have no names
in the text and their first field isn't a class (2 for all but the first), so it is `unk0` and they are numbered.
`pwt_rental` is the tournament's rental sets (`a/2/5/7`, `wbt_party.c`), with no trainers. Both versions have the same
facility data.

Unlike a regular trainer (see [Trainers](#trainers)), a facility trainer has no party of its own: the game picks some of
its sets at random, and the facility sets the level and IVs. The sets are a pool that many trainers share, which is why
they are files of their own that trainers name.

## Map model sources

Each file of the field graphics' archives is a directory named after its constant, as the JSON data is, in
`data/constants/map_models.txt` (`MAP_MODEL_*`, `a/0/0/8`), `map_texture_sets.txt` (`MAP_TEXTURE_SET_*`, `a/0/1/4`),
and `field_props_exterior.txt` and `field_props_interior.txt` (`FIELD_PROPS_EXTERIOR_*` and `FIELD_PROPS_INTERIOR_*`,
for both a prop bundle, `a/2/2/5` or `a/2/2/6`, and its textures, `a/1/7/4` or `a/1/7/5`), whose order is the archive's.
For example, `MAP_MODEL_NACRENE_CITY_6_M_WHOUSE01_00_00` is a house, `data/map_models/nacrene_city_6_m_whouse01_00_00/`.
`make_constants.py --field-graphics` bootstrapped the names: a map model by the place of the map matrices that use it,
or the overworld's, and the model's own name; a texture set or prop bundle by the place of the zones whose areas use
it, or else `shared_` and its first texture or model; numbered from `_2` where they repeat. Map matrices name their
cells' models by these constants, and areas their texture sets and props. Rename one with `rename_constant.py`, which
also points the GLBs at a renamed texture directory (the packers' `relink` command). Run `.venv/bin/python tools/data/find_graphics.py nacrene_city_7` (or another `data/zones/` filename)
to see the map model, map texture set, prop bundle and prop textures used by that zone.
This is closer to [pokeemerald's per-map directories](https://github.com/pret/pokeemerald/tree/master/data/maps)
than to [pokeplatinum's numbered binary map data](https://github.com/pret/pokeplatinum/tree/main/res/field/maps/data).
Like [pokeplatinum's separate texture sets](https://github.com/pret/pokeplatinum/tree/main/res/field/maps/texture_sets),
shared texture resources belong in separate directories. `data/map_texture_sets/<set>/` holds editable color PNGs
for all 409 texture sets. GLB image paths point to these same PNGs, including resource names containing dots.
The packer reads the original Nitro resource to recover its texture format and palette association. Edit the PNG
in Blender or an image editor. The build packs it back
into `a/0/1/4` and checks the original archive against both ROMs. PNGs must be 8-bit RGB or RGBA and retain their
dimensions. Nitro supports limited colors and alpha precision, so the packer quantizes edited colors to RGB555 and
rejects a set when its shared
palette cannot hold the edited colors. Textures that share the same encoded texels must remain compatible.

Each map model's directory holds `model.glb`, its terrain CSVs and a `data.json` (`map_model.schema.json`): its
`texture_set`, a constant where the map-to-area relationship or resource names confirm one, or else null, and then a
`texture_preview`, a set that contains every texture and palette name the model uses, for Blender viewing.
Texture PNGs are shared by texture set.
The binary glTF (`.glb`) holds the model geometry and its buffer in one file and opens directly in Blender.
For example, `MAP_MODEL_MAP_OUTGRASS` is a four-vertex ground plane, and `MAP_MODEL_NACRENE_CITY_6_M_WHOUSE01_00_00` a
house from Nacrene City's map matrix, with 736 vertices and ten materials. A `data.json` with `"transformed": true`
marks a model whose GLB positions went through its scene transforms on export, which the packer reverses from the GLB
and the original Nitro model. Seven unusual prop models also keep their `positions` as exported, because their
transforms cannot yet be reconstructed. These fields come from the export; edits belong in the GLB or PNG. The packer
derives the original triangle and vertex order from Nitro commands, so a Blender export can reindex vertices while
the packer still maps edits to the right commands.

The same GLB contains a Blender empty for each placed field prop, named `bw2_prop_NNNN_id_XXXXX`. Move or rotate
these empties to edit prop placements; duplicate or delete them to add or remove placements. The five-digit ID in the
name selects the prop resource. They must stay at the scene root and cannot be scaled. The packer writes their
position, Y rotation, ID and count back into each map chunk's prop section.

Collision terrain lives alongside each GLB as `terrain_0.csv` and, for the 60 two-layer `GC` chunks,
`terrain_1.csv`. The eight `RD` chunks store two plane normals and heights directly in each tile row. Other
chunks use a shape, slope-table index, height-table index and tile type per cell; their split-plane records live in
`terrain_<layer>_splits.csv`. The `x` and `z` columns identify cells in row order. These are editable integer
sources, including for the one 64×64 chunk. The packer checks every value and writes it into the original terrain
section. Slope and height indices refer to the game's tables in `src/ov036/field_map_chunk_height.c`; the split
file's `index` is the value in a split tile's `offset` column. Add a row to that file and set a tile's shape to `2`
with its `offset` pointing to the new row to create a split tile. The packer updates the following section offsets
when the split count changes. Terrain grid dimensions remain fixed by the map chunk layout.

The archive has 1,065 chunks (`WB`, `GC`, `NG` or `RD`), each with one `BMD0` model, and the archive is identical in
Black 2 and White 2. All 1,065 models roundtrip through the position converter without edits: 1,007 map directly to
Nitro vertices, while 58 use scene node transforms that the packer reverses when writing positions. For the latter,
the converter checks each vertex against the exported baseline before enabling edits.
Of the 1,065 models, 695 have a confirmed texture set, 346 have a representative preview, and 24 name resources
absent from every shared map texture set. GLBs for 1,036 models reference editable shared PNGs; five models with a
confirmed set have no matching image in the GLB. A preview is for viewing and geometry edits; binding a material to
a different texture needs a confirmed set because the runtime set varies by area.

The build reads each GLB `POSITION` and `TEXCOORD_0` accessor and the exposed material settings, writes representable
values into the original Nitro resource, packs `a/0/0/8` and checks it against both ROMs. An unchanged Blender export of
the house reproduces the archive byte for byte; editing one house vertex or UV changes only chunk 834. The export
can be repeated with
`tools/data/map_model_data.py extract APICULA OUTPUT_DIR`, using an installed
[Apicula](https://github.com/scurest/apicula) executable; the build itself needs only Python.

The packer keeps original Nitro draw commands when Blender edits fit their encoding. When face count, vertex sharing
or coordinate-command limits change, it re-encodes the GLB triangles as Nitro GPU commands, preserving each model's
existing draw calls and material slots. This also permits UVs on a shape that originally had none. New vertices must
fit Nitro's 16-bit fixed-point position and UV ranges. Models with scene transforms use their GLB joint assignments
to recover local vertex positions; blended skin weights are unsupported by this map model format.
Apply the mesh object's transforms in Blender before exporting. The packer rejects an extra mesh object or an
unapplied object transform so no visible geometry edits are silently omitted.
UV edits use Nitro's 1/16-texel coordinate precision and each material's texture dimensions and transform.
GLB material base color, opacity, emission and double-sided settings also pack into Nitro. Color and opacity are
quantized to the DS's five-bit channels. For models with a confirmed texture set, changing a material's
`baseColorTexture` to a PNG from that set updates Nitro's texture and palette bindings, material dimensions and UV
coordinates. Clearing the texture slot removes its binding. The packer rebuilds Nitro's name-search dictionary when
the new texture was not already listed by the model. Blender may embed preview images when exporting a GLB; the
packer resolves those by image name. Texture pixels are sourced from the shared PNGs, so save pixel edits there.
The packer accepts embedded pixel edits when the matching shared PNG is saved, and rejects unsaved
embedded edits instead of silently ignoring them.
Other Nitro material settings retain their original values.
Adding material slots or draw calls is not yet supported. Every map model contains only an
`MDL0` block; its textures come from the shared texture archive.

The exterior and interior field props use separate texture archives. Their PNGs live under
`data/field_prop_textures/exterior/` and `data/field_prop_textures/interior/`, a directory per prop bundle,
named as the bundle is. Editing a PNG repacks its original Nitro texture and shared palette;
the original Nitro archive supplies the format and palette mapping needed for a byte-matching build.
The texture size, pixel format and shared palette capacity still constrain edits. The PNGs can be
re-exported with `tools/data/map_texture_data.py extract OUTPUT_DIR a/1/7/4` or `a/1/7/5`.

All 163 exterior and interior field prop bundles have an editable GLB for each of their 6,331 models.
A bundle is the area's `prop_bundle`, and each of its models a directory named after the model's own name, numbered
from `_2` where a bundle repeats one, in the bundle's order. The model's `data.json` (`field_prop_model.schema.json`)
gives its resource ID, which map models' prop placements name, and `glb_sha256`, the hash of the GLB as extracted: a
GLB that still has it packs the original model unchanged, since not every unedited model survives conversion (the
skinned `c11_flag_01` doesn't), and an edited one is converted. Editing geometry,
UVs, material colors and texture bindings uses the same Nitro conversion as map models.
The prop's resource information and animations retain their original Nitro bytes. Models can be
re-exported with `tools/data/field_prop_model_data.py extract APICULA OUTPUT_DIR`.
An unchanged Blender export of a prop also repacks byte for byte; adding a face to a prop model
changes only its bundle, and the rebuilt model loads in Apicula.

## Map matrices, areas and lights

A zone's header names its map matrix, `data/map_matrices/<matrix>.json` (`a/0/0/9`), by its constant in
`data/constants/map_matrices.txt`, which names a matrix after the first zone that uses it, the overworld's
`MAP_MATRIX_OVERWORLD`. A matrix is a grid of its cells' maps, map models by their `MAP_MODEL_*` constants (null for
none), row by row,
and, for the overworld's, of their zones by name; `tools/data/map_matrix_data.py` packs them in the list's order. A
row stays on one line, so the file reads as the grid.

Each area is `data/areas/<area>.json`, named after its constant in `data/constants/areas.txt`, in whose order
`a/0/1/3` has the records (`AreaData`), which name the graphics that zones share: their props, textures, animations
(null for none), lights and colors. The props and texture set are named by their constants (see
[Map model sources](#map-model-sources)), or by number for the few props that the archive doesn't have; the rest are
file numbers. A zone's header names its area by its constant. The field reads the record of the area plus a variant
(`AreaData_Create`): the season for an area with seasons (records 2 to 0x119, `AreaData_HasSeasons`), whose records
after the first are named `_SUMMER`, `_AUTUMN` and `_WINTER`, and other state for three other ranges
(`func_02018f60`, `func_02018f78` and `func_02018f90` in `fieldmap.c`), whose records after the zone's are numbered
from `_2`. An area is named after the first zone whose header names it, and the 81 records that nothing names, by
their number; `make_constants.py --areas` bootstrapped the list. `data/areas/area.schema.json` describes one record
and `tools/data/area_data.py` packs them. Both versions have the same matrices and areas.

An area's lights are a lighting set, `data/lights/field/<n>.json` (`a/0/6/0`, `ARCID_LIGHTS_FIELD`), by its number;
the battles' are `data/lights/battle/<n>.json` (`a/0/6/1`). A set is the periods of the day, which
`FieldLight_FlushCore` applies in turn: when each ends (an hour of the season's table and minutes after it), the four
lights (on or off, color and direction in 4096ths), the materials' diffuse, ambient, specular and emission colors, the
fog's color and the color the screen clears to. Colors are 0 to 31 per component. `tools/data/light_data.py`
packs them, and both versions have the same sets.

## Town map

The town map's places, `data/town_map/<zone>.json` (`a/0/8/5`), form the table that the town map, the Pokédex's habitat
map and a beacon's details read by `TOWNMAP_PARAM_*` (`include/app/townmap/townmap_data.h`). Each file is named after
its zone constant, and `data/town_map/order.json` gives the order the code numbers them. Each place has its zone,
kind, where its marker, cursor and touch area are, whether the player can fly there, the flags that let them and that
show it (an event flag, or a `TOWNMAP_FLAG_*` that the code works out, from
`constants/town_map.h`), its area on the habitat map, and its description and landmarks, messages of
`data/text/system/town_map.txt` by ID. The versions differ only in a few descriptions, which then give one per version:

```json
"description": {"black2": "TownMap_Text_ConvenientCityRapidChange", "white2": "TownMap_Text_CityRespectsHistoryValues"},
"landmarks": ["TownMap_Text_OpelucidCityGym", "TownMap_Text_PokemonCenter_9"],
```

`data/town_map/place.schema.json` describes each file, and `tools/data/town_map_data.py` packs them for each version.

## Zone events

Each zone's events are `data/events/<zone>.json`, named after its constant, as pokeplatinum's `res/field/events/`:
the zone's entities file (`a/1/2/6`), which `LoadZoneEntities` reads. It has the zone's background events, such as
signs, its NPCs, its warps, its triggers and its init scripts, and `data/events/events.schema.json` documents each
field, with swan's names and layouts (`include/field/zone.h`, `include/field/field_actor.h`).

```json
{
    "$schema": "events.schema.json",
    "bg_events": [
        {
            "script": 5,
            "condition": 0,
            "direction": 6,
            "grid": {
                "x": 47,
                ...
            }
        },
        ...
    ],
    "npcs": [ ... ],
    "warps": [
        {
            "zone": "ZONE_BLACK_CITY_POKEMON_CENTER",
            "warp": 0,
            ...
        },
        ...
    ],
    "triggers": [ ... ],
    "init_scripts": [
        {
            "type": 2,
            "script": 1
        },
        ...
    ]
}
```

An event is at a `grid` position or, in a zone with rails, a `rail` one. A warp names the zone it leads to and the warp
of that zone it arrives at, and an NPC the direction it faces (`DIR_*`). An NPC's spawn flag and a trigger's variable
are written by name where the constant lists have one, and otherwise as their IDs in hex, as `"0x4176"`, as IDs are
read; a file can write any ID that way. The init scripts are a map script table (see [Scripts](scripts.md)): a script by
when it runs, or, for type 1, conditions on variables. A zone's header gives the number of its entities file (`entities`
in `data/zones/`), and `tools/data/event_data.py pack` puts each zone's events at that number; the numbers in
`data/events/placeholders.json`, which no zone uses, hold the game's 4-byte placeholder. Both versions have the same
events.
