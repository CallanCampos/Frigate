# Game data

The game keeps most of its data in NARC archives under `files/a/`, one numbered file per entry. The archives that are
built from source have their sources in `res/`, and the build packs each archive from them and checks that it matches
the original, so editing an entry is as easy as editing its file, and the matching build proves that the sources say
exactly what the ROM holds. `res/` is grouped by what the data is, as pokeplatinum's is, and each entity has a
directory or file of its own, named after its constant in the lists of `generated/`, which the build also turns into
the C code's headers:

```
res/
  pokemon/<species>/        data, text, battle sprites, icon, footprint, cry
  moves/<move>/             data and text
  items/<item>/             data, text and icon; shared/ for the icons' cells, animations and unused icons
  abilities/  natures/  types/
  trainers/
    data/<trainer>.json     record and party
    classes/  sprites/{front,back}/<sprite>/  ai/  facilities/<facility>/
  field/
    zones/  events/  encounters/  scripts/  areas/  matrices/  cameras/
    maps/{models,textures}/
    props/{exterior,interior}/{models,textures}/
    actors/                 the overworld's people and objects, with pokemon/ and models/ for its Pokémon and 3D models
    trades/  places/
    gimmicks/<archive>/  effects/<archive>/   the gyms' and other gimmicks' models, the field effects' animations
  lighting/{field,battle}/
  town_map/
  sound/{sequences,banks,wave_archives,streams,interactive}/
  graphics/<archive>/       the menus', apps' and screens' graphics and models, an archive each
  demo/<archive>/           the demos and cutscenes: the movies' frames and the 3D demos' models, an archive each
  text/{system,script}/
generated/                  the constant lists: species.txt, items.txt, obj_codes.txt, sound.txt and the others
```

The sources come in these kinds:

- **JSON data**, as pokeplatinum's `res/`: one file per entity, such as `res/pokemon/bulbasaur/data.json`, with a
  JSON schema beside it that documents every field and that editors use for completion and checking. A script packs
  the archives from the files (`DATA_PACKS` in `configure.py`), validating each file against its schema and naming
  the file and field of any mistake. Files are ordered by the constant lists, not by their names, so adding an entry
  is adding its constant and its file; the build notices new, removed and renamed files by itself.
- **Assembly**, for the field and trainer AI scripts, which are code more than data, as pret keeps them: one source
  file per entry, in archive order, assembled with the macros in `include/asm/` (`ARCHIVES` in `configure.py`).
- **glTF 2.0**, for map models under `res/field/maps/models/<model>/` and field prop models under
  `res/field/props/{exterior,interior}/models/`, each with a `data.json`. Their editable mesh and material data is
  packed back into Nitro models.
- **PNG**, for map textures under `res/field/maps/textures/<set>/` and field prop textures under
  `res/field/props/{exterior,interior}/textures/`. The packer converts edited pixels to each set's original Nitro
  texture format. Like the JSON, these directories are named after their constants and the lists give their order (see
  [Map model sources](#map-model-sources)).
- **Indexed PNG, JASC palettes, and WAV**, for each Pokémon's battle sprites, icons, footprints, and cries under
  `res/pokemon/<species>/`, and the trainer sprites under `res/trainers/sprites/`. Nitro cell and animation
  resources remain as separate binary files next to the images; and for the 2D graphics of the menus, apps and
  screens under `res/graphics/<archive>/` (see [2D graphics](#2d-graphics)).

The JSON and assembly sources use the same constants as the C code (see [Constant lists](#constant-lists)): the JSON
by name, the assembly through the C preprocessor. Each archive has a script that wrote its sources from the original,
which documents the format and can write them again.

| Archive | Sources | Contents | Script |
| --- | --- | --- | --- |
| `a/0/0/2` | `res/text/system/` | System messages | `tools/text/text_data.py` |
| `a/0/0/3` | `res/text/script/` | Script messages | `tools/text/text_data.py` |
| `a/0/1/2` | `res/field/zones/` (JSON) | Zone headers | `tools/data/zone_data.py` |
| `a/0/1/6`, `a/0/1/8`, `a/0/1/9`, `a/0/2/0`, `a/1/2/4` | `res/pokemon/` (JSON) | Species data, level-up moves, evolutions, baby species, egg moves | `tools/data/species_data.py` |
| `a/0/1/7` | `res/pokemon/growth_rates.csv` | Experience tables of the growth rates | `tools/data/species_data.py` |
| `a/0/0/4`, `a/0/0/7`, `a/0/5/1`, `a/1/6/5`, `a/2/2/9` | `res/pokemon/<species>/` (PNG, palette, Nitro animation files) | Battle sprites, icons, musical sprites, and footprints | `tools/data/pokemon_graphics.py` |
| `a/1/9/1` | `res/pokemon/<species>/data.json` (`cry_volume`) | The Pokémon cries' playback volumes | `tools/data/pokemon_cries.py` |
| `swan_sound_data.sdat` | `res/sound/`, `res/pokemon/<species>/cry.wav` | The sound archive: sequences, banks, wave archives, players, groups, streams and the Pokémon cries | `tools/data/sound_data.py` |
| `a/0/2/1` | `res/moves/` (JSON) | Move data | `tools/data/move_data.py` |
| `a/0/2/4` | `res/items/` (JSON) | Item data | `tools/data/item_data.py` |
| `a/0/2/5` | `res/items/<item>/` (PNG, palette), `res/items/shared/` | Item icons | `tools/data/item_icon_data.py` |
| `a/0/5/6` | `res/field/scripts/` | Field scripts, see [Scripts](scripts.md#field-scripts) | `tools/script/field_script.py` |
| `a/0/9/1`, `a/0/9/2`, `a/0/8/9`, `a/0/9/0` | `res/trainers/data/` (JSON) | Trainers, their parties, and the table of their messages | `tools/data/trainer_data.py` |
| `a/0/7/1`, `a/0/7/2`, `a/0/6/3` | `res/trainers/sprites/{front,back}/<sprite>/` (PNG, palette, Nitro cells and animations) | Trainer sprites, front and back | `tools/data/trainer_graphics.py` |
| `a/0/0/9` | `res/field/matrices/` (JSON) | Map matrices | `tools/data/map_matrix_data.py` |
| `a/0/0/8` | `res/field/maps/models/<model>/` (glTF 2.0) | All 1,065 map models | `tools/data/map_model_data.py` |
| `a/2/2/5`, `a/2/2/6` | `res/field/props/{exterior,interior}/models/<bundle>/<model>/` (glTF 2.0) | Field prop models | `tools/data/field_prop_model_data.py` |
| `a/1/7/4` | `res/field/props/exterior/textures/<bundle>/` (PNG) | Exterior field prop textures | `tools/data/map_texture_data.py` |
| `a/1/7/5` | `res/field/props/interior/textures/<bundle>/` (PNG) | Interior field prop textures | `tools/data/map_texture_data.py` |
| `a/0/1/4` | `res/field/maps/textures/<set>/` (PNG) | All 409 field texture sets | `tools/data/map_texture_data.py` |
| `a/0/1/3` | `res/field/areas/<area>.json` | Area records | `tools/data/area_data.py` |
| `a/1/2/6` | `res/field/events/` (JSON) | Zone events: signs, NPCs, warps, triggers and init scripts | `tools/data/event_data.py` |
| `a/0/4/7`, `a/0/4/8` | `res/field/actors/<code>/` (JSON, PNG, palette, Nitro model) | The field actors' models: their table and graphics | `tools/data/field_actor_data.py` |
| `a/1/2/7` | `res/field/encounters/` (JSON) | Wild encounters | `tools/data/encounter_data.py` |
| `a/1/6/3` | `res/field/trades/` (JSON) | In-game trades | `tools/data/trade_data.py` |
| `a/2/1/2`, `a/2/1/1` | `res/trainers/facilities/battle_subway/` (JSON) | Battle Subway and Trial House trainers and Pokémon | `tools/data/facility_data.py` |
| `a/2/6/2`, `a/2/6/1` | `res/trainers/facilities/black_tower/` (JSON) | Black Tower and White Treehollow trainers and Pokémon | `tools/data/facility_data.py` |
| `a/2/4/8` to `a/2/5/7` | `res/trainers/facilities/pwt_*/` (JSON) | Pokémon World Tournament trainers, Pokémon and rental Pokémon | `tools/data/facility_data.py` |
| `a/1/6/9` | `res/trainers/ai/` | Trainer AI scripts, see [Scripts](scripts.md) | `tools/script/tr_ai_script.py` |
| `ARCID_*` in `include/constants/arc.h` | `res/graphics/<archive>/` (PNG, palette, Nitro screens, cells and animations) | The 2D graphics of the menus, apps and screens, see [2D graphics](#2d-graphics) | `tools/data/graphics_archive.py` |

The text archives are packed by `text_data.py` from text files rather than assembled; `configure.py` lists them in
`TEXT_ARCHIVES`.

## Constant lists

The ID constants that name the game's data, such as species, moves, abilities, items, types, sound sequences, trainer
classes, trainers, zones, wild encounter tables, in-game trades, natures, places, event flags and event variables, are
lists in `generated/`, one name per line. They are the source of truth: to add one, add it to its list, and to
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
sources are `res/text/system/<file>.txt` and `res/text/script/<file>.txt`, UTF-8, with one message per line, in order.
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
`generated/text_banks.txt` (`species_names.txt` for `TEXT_BANK_SPECIES_NAMES`), and a script message file after its
`SCRIPT_TEXT_*` in `script_text_banks.txt`, which names a zone's after the zone (`black_city.txt`) and a global script
file's after its first script ID (`global_10885.txt`). A system message file is named after what it holds, which the
word set function that loads it or its contents show (`move_names.txt`, `natures.txt`), or else after the source file or
function that loads it (`delete_save.txt`, `b_bag_main.txt`), with the index the code picks it by where it loads one of
a run (`pkwd_briefing_movie_00.txt` for `TEXT_BANK_PKWD_BRIEFING_MOVIE_00`, `TEXT_BANK_PKWD_BRIEFING_MOVIE_00 + movie`);
the others keep their number until code that loads them is known (`0009.txt` for `TEXT_BANK_0009`). To rename one, rename its constant with `rename_constant.py`, which renames the file, its message IDs
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

Each species has a directory in `res/pokemon/`, named after its constant (`bulbasaur/` for `SPECIES_BULBASAUR`), as
pokeplatinum's `res/pokemon/`. Its `data.json` holds everything the game keeps about it in five archives: its record of
the species data (`a/0/1/6`, the 0x4c bytes that `PML_PersonalGetParam` reads), its level-up moves (`a/0/1/8`), its
evolutions (`a/0/1/9`), its baby species (`a/0/2/0`) and its egg moves (`a/1/2/4`). `res/pokemon/species.schema.json`
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

- The species of `generated/species.txt`, from `SPECIES_NONE` (`none/`, an empty record) to Genesect, then
  `SPECIES_EGG` and `SPECIES_BAD_EGG` and the 33 Pokéstar Studios species, `SPECIES_POKESTAR_UFO` to
  `SPECIES_POKESTAR_SMEARGLE_2`: the props and costumes of the studio's movies, which the game keeps as species, with
  their own records, sprites, icons and cries (a Steel and Flying UFO with 600 base stats, the doors, the sixteen prop
  boxes). Not being Pokémon, they are in `res/pokemon/pokestar/<name>/`, such as `pokestar/ufo/`. Their names are lines of `species_names.txt` after the national Pokédex's, as the code names Bad Egg's
  message, and they have no Pokédex text, so their files have only their records and `cry_volume`.
- The alternate forms that have records of their own, `res/pokemon/<species>/form_<n>.json`, in the order of
  `res/pokemon/forms.json`. The packer sets each species' first form record from it, so a species' `forms` only
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

The experience tables (`a/0/1/7`) are `res/pokemon/growth_rates.csv`, a row per level from 0 to 100 and a column per
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
`res/pokemon/shared/icon_palettes.pal` (the PC boxes use `icon_palettes_box.pal`); the code in
`src/system/poke_icon.c` picks the bank, and the icons' cells and animations for each OBJ mapping are
`shared/icon_{cells,animations}_{32k,64k,128k}`. `footprint.png` is a 16×32 character sheet and uses
`res/pokemon/shared/footprint_palette.pal`, with the cells and animations of `shared/footprint_*_old` for the species
up to Arceus and `shared/footprint_*` for the rest (`pokedex_info.c`).
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
A cry's wave archive must fit the game's 0x5c30-byte buffer. The sound archive takes the cries from `cry.wav` (see
[Sound](#sound)). Extract the graphics again with `.venv/bin/python tools/data/pokemon_graphics.py extract
res/pokemon` and the cries with `.venv/bin/python tools/data/pokemon_cries.py extract res/pokemon`; the build packs
them automatically. An unchanged source set reproduces the original graphics archives, the volumes and the cries byte
for byte.

## Sound

The sound archive, `swan_sound_data.sdat`, the same in both versions, is built from `res/sound/` by
`tools/data/sound_data.py`. Its entries are the constants of its lists in `generated/`, each in its list's order:
the sequences of `sound.txt` (`SEQ_BGM_*` music, `SEQ_SE_*` sound effects, `SEQ_ME_*` jingles), the banks of
instruments of `sound_banks.txt`, the wave archives of samples of `sound_wave_archives.txt`, and the players, groups,
stream players and streams of `sound_players.txt`, `sound_groups.txt`, `sound_stream_players.txt` and
`sound_streams.txt`, whose names are the archive's own symbols. A sequence, bank, wave archive or stream is a
directory named after its constant, in its kind's directory and its category's, the word after the kind's prefix:
`SEQ_BGM_T_01` is `res/sound/sequences/bgm/t_01/`, `BANK_SE_SW_JUMP` `res/sound/banks/se/sw_jump/`, and
`BANK_PV001` `res/sound/banks/pv/pv001/`. A sequence has its `sequence.s` (see [Sequences](#sequences)), a bank its
instruments in its `data.json` (see [Banks](#banks)), a wave archive its waves as WAV files (see
[Wave archives](#wave-archives)), and a stream its audio, `stream.wav`, such as the title screen's music
(`streams/title/title/`), 8-bit or 16-bit PCM, mono or stereo, with its loop start in samples in its `data.json` if
it loops. A sequence, bank and stream has `data.json`, its record of the archive,
which the schemas in `res/sound/` describe: a sequence's bank, volume, priorities and player, a bank's four wave
archives, a stream's volume, priority, player, loop and its timer where that isn't its rate's. Some entries share a file: 86 sequences play another's, such as the
placeholder sequences, and three banks and three wave archives use another's; such an entry's `data.json` has
`"file"`, naming the one whose file it uses. `players.json`, `stream_players.json` and
`groups.json` hold the small kinds' records by their constants. The Pokémon cries' wave archives, the 690 after the
empty first slot of `sound_wave_archives.txt`, are each species' `cry.wav` (see
[Pokémon graphics and cries](#pokémon-graphics-and-cries)).

To add a sequence, bank, wave archive or stream, add its constant to the end of its list, after its last constant
(nothing in the decompiled code reads the sequences' `SEQ_SE_END` and the like as limits), and its directory, as
a copy of one to start from; a new player, group or stream player needs its constant and its entry in
`players.json`, `groups.json` or `stream_players.json`. An entry's `"file"` must name one with a file of its own.
`rename_constant.py` moves the directory when a constant is renamed. The packer lays the archive out as the game's, the
files of the sequences, banks, wave archives and streams, each kind in its list's order, so the sources pack it byte
for byte, and stops with the file's name and line where a source is wrong. `.venv/bin/python
tools/data/sound_data.py dump extract/b2_us/files res/sound` writes them again.

The interactive sound system's tables, which change the field's music by where the player is, are
`res/sound/interactive/`, a JSON file per archive with its entries in order, which
`tools/data/interactive_sound_data.py` packs: `cities.json`, the towns whose music grows louder toward their center
(`ARCID_ISS_CITY`); `dungeons.json`, a dungeon's pitch and tempo in each season; `zones.json`, the tracks that a zone
fades in and out; `switches.json`, a BGM's nine switches of tracks and the zones they apply in; and `3d_sound.json`, the
BGMs whose tracks play from places on the map. Zones and BGM are their `ZONE_*` and `SEQ_*` constants, and tracks are
lists of track numbers, 0 to 15. `interactive_sound.schema.json` describes the fields, and both versions have the same
tables.

### Sequences

A sequence's `sequence.s` is its commands as text, one a line, in the order of the sequence's data, with labels where
its tracks start and where jumps and calls go; `tools/data/sseq.py` assembles it into the game's sequence byte for
byte. Track 0 starts the sequence, allocates the tracks with `alloc_tracks` and starts the others with `open_track`:

```
track_00:
    alloc_tracks 0x0c5f
    open_track 1, track_01
    ...
    tempo 87
    program 47
    volume 104
    rest 576
label_0045:
    note A5, 120, 113
    rest 6
    pitch_bend -5
    ...
    jump label_0045
```

`note KEY, VELOCITY, LENGTH` plays a key, named `C`, `Cs`, `D`, ... `B` and an octave, middle C (60) being `C4` and a
negative octave `m1`, for a length in ticks, and `rest TICKS` waits. `program` picks an instrument of the sequence's
bank; `tempo`, `volume`, `expression`, `pan`, `transpose`, `pitch_bend`, `bend_range`, `priority`, `note_wait`, `tie`,
`portamento`, `portamento_key`, `portamento_time`, `mod_depth`, `mod_speed`, `mod_type`, `mod_range`, `mod_delay`,
`sweep_pitch`, `attack`, `decay`, `sustain`, `release` and `master_volume` set what their names say; `jump`, `call`,
`return`, `loop_start`, `loop_end` and `end` steer the track. The variable commands are `set_var`, `add_var`,
`sub_var`, `mul_var`, `div_var`, `shift_var`, `random_var` and the comparisons `cmp_eq`, `cmp_ge`, `cmp_gt`, `cmp_le`,
`cmp_lt` and `cmp_ne`; a command after `if` only runs when the last comparison held, after `random` its last argument
is a random number between the two given, and after `from_var` it is the variable given. `.byte` keeps data that no
track reaches, `;` starts a comment, and `rest 5 @2` keeps a wait that the game stored in more bytes than it needs.
The assembler checks that every track ends with `end` or a `jump`, closes the loops it opens, returns only from a
`call` and starts only the tracks `alloc_tracks` gives, and names the line where one doesn't. `sseq.py`'s commands table has every command with its byte and arguments.

To add a song or a sound, add its `SEQ_` constant to the end of `sound.txt` and its directory, with a `data.json`
naming its bank, volume, priorities and player, as a copy of a sequence like it, and its `sequence.s`, written by hand
or converted from a MIDI file:

```
.venv/bin/python tools/data/midi_to_sequence.py SONG.mid res/sound/sequences/bgm/my_song/sequence.s
```

Each MIDI channel with notes becomes a track at the game's 48 ticks a quarter note, and the tempo, program changes,
volume, pan, expression, modulation depth and pitch bend come along; markers named `loopStart` and `loopEnd` loop the
song. A program is an instrument of the sequence's bank, not a General MIDI one, so set the MIDI file's programs to the
bank's instruments.

### Banks

A bank's `data.json` has its four wave archives and its instruments, in the order of their data, each with the
program number that a sequence's `program` picks it by:

```json
{
    "program": 85,
    "type": "wave",
    "wave_archive": "WAVE_MUS_WB_SOUNDLIST",
    "wave": 110,
    "base_key": "Cs2",
    "attack": 127,
    "decay": 125,
    "sustain": 120,
    "release": 120,
    "pan": 64
}
```

A `wave` instrument plays a wave of one of the bank's wave archives, a `psg` one a square wave of a `duty` from 0 to 7
and a `noise` one noise, each at its own pitch on its `base_key`, with its envelope (`attack`, `decay`, `sustain`,
`release`) and `pan`. A `drums` set has an instrument for each key from its `first_key` in its `keys`, and a `split` an
instrument for each region of keys in its `regions`, each up to its `up_to`. Programs that share an instrument list
their numbers, an `unused` entry keeps data that no program uses as hex, and `programs` gives a bank more programs than
its last instrument's. `tools/data/sbnk.py` writes the bank from them byte for byte.

### Wave archives

A wave archive is a directory of its waves, the samples that banks' instruments play, `000.wav` and on, in the order
that instruments number them, and a `data.json` with what a WAV doesn't say: for each wave, an empty object if it plays
once, or the sample its loop starts at, `"loop_start"`, if it loops to its end, and its `"timer"` where that isn't its
rate's. The game's waves are IMA ADPCM, and their WAV files keep them so, as IMA ADPCM WAVs that audio editors and
ffmpeg open, so they pack back byte for byte; a wave larger than a WAV block is in several blocks, each starting with
the decoder's state there. An edited wave can be saved as 8-bit or 16-bit PCM, which the game plays too, mono at a
rate from 4000 to 65535; a loop starts on a multiple of 8 samples in IMA ADPCM, 4 in 8-bit and 2 in 16-bit PCM. To add a
wave, add its WAV after the last and its entry to `waves`, and an instrument that plays it.

## Moves

Each move has a directory in `res/moves/`, named after its constant (`thunderbolt/` for `MOVE_THUNDERBOLT`), whose
`data.json` is its record of the move data (`a/0/2/1`, the 0x24 bytes that `PML_MoveGetParamCore` reads).
`tools/data/move_data.py pack` builds the archive in the order of `generated/moves.txt`, and
`res/moves/move.schema.json` documents each field.

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

Each item has a directory in `res/items/`, named after its constant (`potion/` for `ITEM_POTION`), whose `data.json`
is its file of the item data (`a/0/2/4`, the 36 bytes of `ItemData` in `include/pml/item.h`) and its text: its name,
its name with an article, its plural and its description (see [Text](#text)). `tools/data/item_data.py pack`
builds the archive in the order of `generated/items.txt`, and `res/items/item.schema.json` documents each field.

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

### Item icons

The item icons' archive (`a/0/2/5`) has a member for each constant of `generated/item_icons.txt`, in its order:
an icon (`ITEM_ICON_*`), 32×32 and 16 colors, or a palette (`ITEM_ICON_PALETTE_*`), its colors in JASC format, 0
transparent. `ITEM_GRAPHICS_CHARS_AND_PALETTES` in `src/system/pml_item.c` gives each item an icon and a palette, as
`[ITEM_HYPER_POTION] = { ITEM_ICON_POTION, ITEM_ICON_PALETTE_HYPER_POTION }`, and `LIST_ITEM_GRAPHICS` the Wonder
Launcher's list icons. Items share them, so each is named after the first item that shows it and is a file of that
item's directory: `ITEM_ICON_POTION` is `res/items/potion/icon.png`, `ITEM_ICON_PALETTE_HYPER_POTION`
`res/items/hyper_potion/icon.pal`, and `ITEM_ICON_POTION_LAUNCHER` `res/items/potion/launcher_icon.png` (and
`.pal`). An item whose icon or palette is another's has no file of its own for it: the Hyper Potion shows the Potion's
picture in colors of its own, every TM shows TM01's disc, and the 47 items without an icon, `ITEM_NONE` among them,
show the placeholder in `none/`; `pml_item.c` says whose file an item shows. Editing a shared icon or palette changes
every item that shows it. `res/items/shared/` has the icons' Nitro cells and animations, which code loads as members
0 and 1, the return arrow's icon (`return_icon`), and in `unused_icons/` the 53 icons and 48 palettes that no item
shows, named by their member numbers (`ITEM_ICON_UNUSED_MEMBER_140` is `unused_icons/140.png`).

The palette file sets an icon's colors; its PNG shows it with the palette of the first item that has it, but only the
PNG's indices count, so edit it in an image editor's indexed mode. To add an icon, add its constants to the end of
`item_icons.txt`, named after its item, and its files, and give the item them in `pml_item.c`. Renaming an item with
`rename_constant.py` renames its icons' constants too, and renaming an icon's constant moves its file. The packer
builds each member from its file, and where that gives the same picture, palette or data as one of the original
archive's members, it takes the original's bytes (some palettes keep 240 unused colors after their 16), so unchanged
files pack the archive byte for byte in both versions. Every file must be there and keep its size and 16 colors, and
the build stops with the file's name otherwise. `.venv/bin/python tools/data/item_icon_data.py dump
extract/b2_us/files res/items` writes the icons again.

## Abilities, types, trainer classes, natures and places

An ability's text is `res/abilities/<ability>.json`, its name and description, and a type's `res/types/<type>.json`,
its name, named after their constants and in the order of their lists. What they do is the code's, so they have no
archive: only the text takes them (see [Text](#text)), and `tools/text/text_sources.py` validates them against
`ability.schema.json` and `type.schema.json`. `TYPE_NULL`, the type of a typeless move, has no name and no file. A
trainer class's text, `res/trainers/classes/<class>.json`, is its name and its name with an article, which the game
writes by hand ("a Twin" for Twins, none for the Gym Leaders), a nature's, `res/natures/<nature>.json`, its name,
and a place's, `res/field/places/<place>.json`, its name. Zone headers name their place by its constant in
`generated/places.txt` (`PLACE_ASPERTIA_CITY`), which `make_constants.py --places` named after the place names,
numbering the ones that repeat, as the second copies of Unova's places.
`tools/text/list_text_data.py` wrote them all from the game's text.

## Trainers

Each trainer is `res/trainers/data/<trainer>.json`, named after its constant (`elite_four_shauntal.json` for
`TRAINER_ELITE_FOUR_SHAUNTAL`), as pokeplatinum's `res/trainers/data/`. It holds the trainer's record (`a/0/9/1`) and
its party (`a/0/9/2`), and `tools/data/trainer_data.py pack` builds both archives in the order of
`generated/trainers.txt`. `res/trainers/data/trainer.schema.json` documents each field.

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
writes both, with the trainers in the order of `res/trainers/data/message_order.json`, then any others that have messages. A
party either gives every Pokémon an `item` or none, and `moves` or none; the packer sets the record's party kind
(`PARTY_ITEMS`, `PARTY_MOVES`) from that, and a Pokémon without moves gets the moves of its level. `ai_flags` are the
trainer AI scripts to run (`AI_FLAG_*`, see [Scripts](scripts.md)), `money` a multiplier of the prize money and `reward`
an item given after the battle. A Pokémon's `difficulty` sets its individual values, and `gender` and `ability` pick
them when not 0. `class` is a `TRAINER_CLASS_*` from `generated/trainer_classes.txt`, named after the class's name;
where several classes share one, after their only trainer, their sex or their ID, as `TRAINER_CLASS_SCHOOL_KID_F`
(`make_constants.py --trainer-classes`). `TRAINER_NONE` has no file: the packer writes the empty placeholder the game
has for it. Both versions have the same trainers.

Each trainer's constant in `generated/trainers.txt`, which the field scripts use, is its class and name, as
`TRAINER_YOUNGSTER_JIMMY`, without the class for the story characters, whose class is Pokémon Trainer
(`TRAINER_CHEREN`), and numbered from `_2` where a class and name repeat, as for rematches (`make_constants.py
--trainers`). To add a trainer, add its constant to the end of the list and its file.

### Trainer sprites

Each trainer sprite is a directory in `res/trainers/sprites/front/` or `res/trainers/sprites/back/`, named after its
constant in `generated/trainer_sprites.txt` or `trainer_back_sprites.txt`, and the archives (`a/0/7/1`,
`a/0/7/2`) hold the sprites in the lists' order. A sprite's constant is named after the first trainer class that shows
it, as `TRAINER_SPRITE_YOUNGSTER`; several classes can share one, such as the Battle Subway's bosses' and their
classes'. `TRAINER_CLASS_RESOURCES` in `src/system/tr_tool.c` gives each class its front sprite. The 32 classes with a
back sprite of their own are in `TRAINER_CLASS_BACK_SPRITES`, which has 31 sprites, since two classes share the
Champion's; any other class shows one of the player's. `a/0/6/3` is a copy of the back sprites' archive, packed from
the same directories. A sprite's files:

- `single.png`, the 80×80 picture of the trainer that screens outside battle show, such as the trainer card's
  leaders, which the packer stores as the game's four OBJ blocks. Only 127 front sprites and the rival's back sprite
  have one: the game's others are empty, and without the file the packer writes an empty one.
- `sheet.png`, the 256×128 sheet of the parts that the cells put together into the animated battle sprite, so the
  sheet looks cut up.
- `normal.pal`, the sprite's 16 colors in JASC format, 0 transparent. The palette file sets the colors; a PNG's own
  palette only puts back the indices that an image editor reordered on saving, so recolor a sprite in its `.pal`. A
  color painted in a PNG that the palette doesn't have keeps its index, with the `.pal` file's color, and the build
  warns about it.
- `cells.ncer`, `animations.nanr`, `multi_cells.nmcr` and `multi_animations.nmar`, the battle sprite's Nitro cells and
  animations, uncompressed, which Nitro editors open; the packer compresses the animations as the game loads them.
- `mcss.bin`, the battle sprite system's data (`mcss.c`), which the game reads as it is stored.

To add a sprite, add its constant to the end of its list and its directory, as a copy of a sprite to start from, and
give a class the sprite in `TRAINER_CLASS_RESOURCES`, whose `sprite` field is a byte. The packer builds every file from
its source; where that gives the same picture, palette or data as one of the original archive's files, from the
version being built, it takes the original's bytes, which keeps the ROM's compression and headers, so an unchanged set
packs all three archives byte for byte in both versions. Every file but `single.png` must be there, the pictures must
keep their sizes and the palette its 16 colors, and the build stops with the file's name otherwise. `.venv/bin/python tools/data/trainer_graphics.py dump extract/b2_us/files
res/trainers/sprites` writes the directories again.

## Wild encounters

Each wild encounter table (`a/1/2/7`, which a zone's header names, `GetZoneEncID`) is `res/field/encounters/<table>.json`,
named after its constant in `generated/encounters.txt` (`striaton_city.json` for `ENCOUNTERS_STRIATON_CITY`),
which names the tables after the place of the first zone that has them. `tools/data/encounter_data.py pack`
builds each version's archive in the order of the list, and `res/field/encounters/encounters.schema.json` documents each
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

Each zone's header is `res/field/zones/<zone>.json`, named after its constant (`aspertia_city_gym.json` for
`ZONE_ASPERTIA_CITY_GYM`). `tools/data/zone_data.py pack` puts the 615 headers, 0x30 bytes each, into the single
entry of `a/0/1/2` in the order of `generated/zones.txt`, and `res/field/zones/zone.schema.json` documents each field
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

Each in-game trade is `res/field/trades/<trade>.json`, named after its constant in `generated/trades.txt`, which names
the trades after the species they offer (`petilil.json` for `TRADE_PETILIL`). It holds the offer that
`EventFieldTrade_CreatePkm` makes the Pokémon from (`a/1/6/3`, `FieldTradeOfferData` in `include/field/fld_trade.h`)
and the Pokémon's nickname and trainer's name, which the text takes (see [Text](#text)).
`tools/data/trade_data.py pack` builds the archive in the order of the list, numbering each offer and its two
names' message IDs from its place, and `res/field/trades/trade.schema.json` documents each field: the IVs, nature and sex
are null for random ones. The field scripts name the trades they start, as `FieldTradeCheck 0x8022, TRADE_GIGALITH,
0x8020`. Both versions have the same trades.

## Battle facilities

A battle facility's trainers and the Pokémon sets they pick from are in `res/trainers/facilities/<facility>/`, as
pokeplatinum's `res/trainers/frontier/`: `trainers/<trainer>.json` has a trainer's class, its sets by name, and its
name and messages, which the text takes (see [Text](#text)); `pokemon/<set>.json` is a set (`BSubwayPokemonData` in
`include/field/battle_facility.h`), named after its species and numbered from 1, as `gengar_4.json`; and `order.json`
lists both in archive order. `tools/data/facility_data.py pack` builds the facility's trainers and Pokémon
archives, and `res/trainers/facilities/facility.schema.json` and `set.schema.json` document the fields.

`FACILITIES` in `facility_data.py` says which archives and message files are a facility's: the Battle Subway (`a/2/1/2`
and `a/2/1/1`, whose trainers' names are system message file 15 and their three messages each file 376), which the Trial
House also uses (`BtlInst_MakeRomTrainerAndTeam`); the Black Tower, White Treehollow in White 2 (`a/2/6/2` and `a/2/6/1`, which
`InfinityDungeon_CreateBattleEvent` reads, with the trainers' names in file 53, stored compressed, and no messages); and the Pokémon
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
`generated/map_models.txt` (`MAP_MODEL_*`, `a/0/0/8`), `map_texture_sets.txt` (`MAP_TEXTURE_SET_*`, `a/0/1/4`),
and `field_props_exterior.txt` and `field_props_interior.txt` (`FIELD_PROPS_EXTERIOR_*` and `FIELD_PROPS_INTERIOR_*`,
for both a prop bundle, `a/2/2/5` or `a/2/2/6`, and its textures, `a/1/7/4` or `a/1/7/5`), whose order is the archive's.
For example, `MAP_MODEL_NACRENE_CITY_6_M_WHOUSE01_00_00` is a house, `res/field/maps/models/nacrene_city_6_m_whouse01_00_00/`.
`make_constants.py --field-graphics` bootstrapped the names: a map model by the place of the map matrices that use it,
or the overworld's, and the model's own name; a texture set or prop bundle by the place of the zones whose areas use
it, or else `shared_` and its first texture or model; numbered from `_2` where they repeat. Map matrices name their
cells' models by these constants, and areas their texture sets and props. Rename one with `rename_constant.py`, which
also points the GLBs at a renamed texture directory (the packers' `relink` command). Run `.venv/bin/python tools/data/find_graphics.py nacrene_city_7` (or another `res/field/zones/` filename)
to see the map model, map texture set, prop bundle and prop textures used by that zone.
This is closer to [pokeemerald's per-map directories](https://github.com/pret/pokeemerald/tree/master/data/maps)
than to [pokeplatinum's numbered binary map data](https://github.com/pret/pokeplatinum/tree/main/res/field/maps/data).
Like [pokeplatinum's separate texture sets](https://github.com/pret/pokeplatinum/tree/main/res/field/maps/texture_sets),
shared texture resources belong in separate directories. `res/field/maps/textures/<set>/` holds editable color PNGs
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
`res/field/props/exterior/textures/` and `res/field/props/interior/textures/`, a directory per prop bundle,
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

The models of the other archives (the demos', the gimmicks' and the effects') are GLBs in the same way, packed by
`tools/data/graphics_archive.py` with these converters; see "2D graphics".

## Map matrices, areas, lights and cameras

A zone's header names its map matrix, `res/field/matrices/<matrix>.json` (`a/0/0/9`), by its constant in
`generated/map_matrices.txt`, which names a matrix after the first zone that uses it, the overworld's
`MAP_MATRIX_OVERWORLD`, and one that no zone uses after Game Freak's name of its map model (`MAP_MATRIX_M_DUN0403`
for `MAP_MODEL_M_DUN0403_00_00`). A matrix is a grid of its cells' maps, map models by their `MAP_MODEL_*` constants (null for
none), row by row,
and, for the overworld's, of their zones by name; `tools/data/map_matrix_data.py` packs them in the list's order. A
row stays on one line, so the file reads as the grid.

Each area is `res/field/areas/<area>.json`, named after its constant in `generated/areas.txt`, in whose order
`a/0/1/3` has the records (`AreaData`), which name the graphics that zones share: their props, textures, animations
(null for none), lights and colors. The props and texture set are named by their constants (see
[Map model sources](#map-model-sources)), or by number for the few props that the archive doesn't have; the rest are
file numbers. A zone's header names its area by its constant. The field reads the record of the area plus a variant
(`AreaData_Create`): the season for an area with seasons (records 2 to 0x119, `AreaData_HasSeasons`), whose records
after the first are named `_SUMMER`, `_AUTUMN` and `_WINTER`, and other state for three other ranges
(`func_02018f60`, `func_02018f78` and `func_02018f90` in `fieldmap.c`), whose records after the zone's are numbered
from `_2`. An area is named after the first zone whose header names it, and the 81 records that nothing names, by
their number; `make_constants.py --areas` bootstrapped the list. `res/field/areas/area.schema.json` describes one record
and `tools/data/area_data.py` packs them. Both versions have the same matrices and areas.

An area's lights are a lighting set, `res/lighting/field/<n>.json` (`a/0/6/0`, `ARCID_LIGHTS_FIELD`), by its number;
the battles' are `res/lighting/battle/<n>.json` (`a/0/6/1`). A set is the periods of the day, which
`FieldLight_FlushCore` applies in turn: when each ends (an hour of the season's table and minutes after it), the four
lights (on or off, color and direction in 4096ths), the materials' diffuse, ambient, specular and emission colors, the
fog's color and the color the screen clears to. Colors are 0 to 31 per component. `tools/data/light_data.py`
packs them, and both versions have the same sets.

The field camera's data is in `res/field/cameras/`, which `tools/data/field_camera_data.py` packs, the same in both
versions. A zone's header names its camera setup by number, `setups/<n>.json` (the records of `a/0/5/9`'s one file,
`FieldCameraDefaults`, which `FieldCamera_CoordsLoadDefaults` applies): its distance, pitch, yaw and field of view,
clipping planes, depth buffer mode, and whether it follows the player at an offset or looks at a point. Its `cam_bound`
names a boundary set, `boundaries/<n>.json` (`a/1/0/9`), 65535 for none: up to four rectangles and circles on the X-Z
plane that keep the camera's target or eye inside them, or out of a rectangle (`FieldCameraBoundary_LoadFile`). A warp's
transition type from `WARP_TRANSITION_CAMERA` (7) on has an entrance camera, `entrances/<n>.json` (`a/1/7/7`), where the
camera moves as the player goes in or comes out (`entrance_camera.c`). Distances and positions are in whole units,
except the setups' clipping planes and target and the circles, which are in 4096ths; angles are in 65536ths of a turn.
`setup.schema.json`, `boundary.schema.json` and `entrance.schema.json` describe the fields.

## Town map

The town map's places, `res/town_map/<zone>.json` (`a/0/8/5`), form the table that the town map, the Pokédex's habitat
map and a beacon's details read by `TOWNMAP_PARAM_*` (`include/app/townmap/townmap_data.h`). Each file is named after
its zone constant, and `res/town_map/order.json` gives the order the code numbers them. Each place has its zone,
kind, where its marker, cursor and touch area are, whether the player can fly there, the flags that let them and that
show it (an event flag, or a `TOWNMAP_FLAG_*` that the code works out, from
`constants/town_map.h`), its area on the habitat map, and its description and landmarks, messages of
`res/text/system/town_map.txt` by ID. The versions differ only in a few descriptions, which then give one per version:

```json
"description": {"black2": "TownMap_Text_ConvenientCityRapidChange", "white2": "TownMap_Text_CityRespectsHistoryValues"},
"landmarks": ["TownMap_Text_OpelucidCityGym", "TownMap_Text_PokemonCenter_9"],
```

`res/town_map/place.schema.json` describes each file, and `tools/data/town_map_data.py` packs them for each version.

## Field actors

The field's actors are the people, Pokémon and objects that zones and scripts put on the map. Each one's model is an
object code, a constant of `generated/obj_codes.txt`, which the zones' events give their NPCs as `"model":
"OBJ_CODE_YOUNGSTER"`, and each object code is a directory of `res/field/actors/`: `hero/` for `OBJ_CODE_HERO`,
`pokemon/bulbasaur/` for the walking Pokémon's `OBJ_CODE_POKEMON_BULBASAUR`, and `models/bigrock/` for the 3D models'
`OBJ_CODE_MODEL_BIGROCK`. Its `data.json` is its record of the model table (`a/0/4/7`, `FieldActorConfig` in
`include/field/field_actor.h`, with swan's names), which `field_actor.schema.json` documents: whether it is a sprite,
a 3D model or nothing to draw, its shadow, footprints, reflections, sizes, sex, collision and drawing offset. Its
graphics go in the graphics archive (`a/0/4/8`):

- a sprite has `sprite.png`, its frames side by side, each a square 32 or 64 pixels wide, as the hero's 24 frames of
  facing and walking, and `sprite.pal`, its 16 colors in JASC format, 0 transparent. The palette file sets
  the colors; a PNG's own palette only puts back the indices that an image editor reordered on saving.
- a 3D model has `model.nsbmd`, a Nitro model, unless its `data.json`'s `"model"` names the object code whose model it
  shows.

People and objects are named after the trainer class that most of the trainers the zones' events give them have
(`OBJ_CODE_BIKER`, `OBJ_CODE_ROUGHNECK` and `OBJ_CODE_HARLEQUIN_233` also show other classes'), or where the code
shows what they are (`OBJ_CODE_HERO`, `OBJ_CODE_STRENGTH_ROCK`, `OBJ_CODE_WORK_00` and on, which stand for the
code in a work value); 3D models are named after the model's own name in its `model.nsbmd` (`OBJ_CODE_MODEL_DRAGON_BONE`
for `dragon_bone`); and are numbered otherwise, as `OBJ_CODE_80`: name them with `rename_constant.py` as they are
identified, which moves their directories. Walking Pokémon are named after the species, sex and form that the walking
Pokémon's table (`a/2/0/8`) gives them. The table has the codes in the list's order; the graphics archive has the 3D
models and then the sprites, each in the order of the codes that show them, and the packer writes each code's place
in it into the table. To add an actor, add its constant to the list, with a value of its own, and its directory, as a
copy of one to start from. Two places in the code still load sprites by their place in the graphics archive, the
Funfest Missions' player icons (`sFMissionViewPlayerModels`) and an entrant's default sprite in `wbt_setup.c`, so adding a 3D model moves the sprites those name. Only a 3D model has a `model.nsbmd` or a
`"model"`, and the code it names must have a `model.nsbmd` of its own.
`tools/data/field_actor_data.py` writes the sprites' texture sets as the game's are, so the sources pack both archives
byte for byte; `.venv/bin/python tools/data/field_actor_data.py dump extract/b2_us/files res/field/actors` writes
them again.

## Zone events

Each zone's events are `res/field/events/<zone>.json`, named after its constant, as pokeplatinum's `res/field/events/`:
the zone's entities file (`a/1/2/6`), which `LoadZoneEntities` reads. It has the zone's background events, such as
signs, its NPCs, its warps, its triggers and its init scripts, and `res/field/events/events.schema.json` documents each
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
of that zone it arrives at, and an NPC the direction it faces (`DIR_*`), its move code (`MOVE_CODE_*` of `generated/move_codes.txt`,
such as a random walk) and its event type (`ACTOR_EVENT_TYPE_*` of `generated/actor_event_types.txt`, such as a
Trainer that sees the way it faces). An NPC's spawn flag and a trigger's variable
are written by name where the constant lists have one, and otherwise as their IDs in hex, as `"0x4176"`, as IDs are
read; a file can write any ID that way. The init scripts are a map script table (see [Scripts](scripts.md)): a script by
when it runs, or, for type 1, conditions on variables. A zone's header gives the number of its entities file (`entities`
in `res/field/zones/`), and `tools/data/event_data.py pack` puts each zone's events at that number; the numbers in
`res/field/events/placeholders.json`, which no zone uses, hold the game's 4-byte placeholder. Both versions have the same
events.

## 2D graphics

The menus, apps and screens keep their 2D graphics in archives of Nitro resources, an archive each: the title screen's,
the Pokédex's (`ARCID_POKEDEX_GRA`), the box's (`ARCID_BOX2`) and the others. Each is `res/graphics/<archive>/`, named
after its `ARCID_*` constant in `include/constants/arc.h`, whose number gives the archive's file (`ARCID_TITLE`, 26, is
`a/0/2/6`); `configure.py` packs a directory for each, 111 of them. An archive that swan doesn't name is named after
the source file that loads it, or the prefix of an app's files where several load it (`ARCID_BTLV` for the
`btlv_*.c` files), in the block at the end of `arc.h` with its loaders. Twelve archives with graphics, `a/0/0/0`,
`a/0/0/1`, `a/0/3/5`, `a/0/4/3`, `a/0/5/4`, `a/0/5/5`, `a/0/6/4`, `a/0/6/7`, `a/1/2/1`, `a/1/4/0`, `a/1/4/1` and
`a/2/4/6`, have no loader found yet, so no name, and stay as the ROM has them until code that loads them is; the fonts'
archive is `res/fonts/font/`. The members are files in the archive's order, named after their number in it, or
after their constant where `generated/narc_<archive>.txt` names them (`professor.png` for `NARC_INTRO_PROFESSOR_NCGR`):

- a character resource (NCGR), the tiles of a screen or the characters of a sprite, is an indexed PNG (`05.png`), 4bpp
  or 8bpp as the resource is. Its pixels are the palette indices that the game draws, so edit it in indexed mode and
  keep the palette's order: the packer reads the indices, not the colors. Its colors are only a preview, from the
  archive's nearest palette, since which palette and which 16-color bank a screen or sprite uses is the code's and the
  screen's to say. A background's PNG is as wide as the game's tile sheet, and a sprite's characters, which have no
  size, are drawn in rows of four tiles, as a 32 by 32 OBJ stores them.
- a palette resource (NCLR) is a JASC palette (`11.pal`), which sets the colors.
- a 3D model (BMD0) is a directory named as the member is (`05/`), as a field prop's model is: `model.glb`, its
  `data.json` and, where the model embeds its textures (a TEX0 block), a PNG of each texture, named after it, which the
  GLB's images show. A texture set (BTX0) is a directory of those PNGs alone. See below.
- a font (NFTR) is a directory named as the member is (`00/`): `glyphs.png`, a sheet of its glyphs, and `font.json`,
  its metrics, character maps and glyphs' widths. See below.
- a particle resource (SPA) is a directory named as the member is (`000/`): `particles.json`, its emitters' fields,
  and a PNG of each texture. See below.
- the rest are the members themselves, decompressed: screens (`.nscr`), which place the tiles, cells (`.ncer`), cell
  animations (`.nanr`), multi-cells (`.nmcr`, `.nmar`), 3D animations (`.nsbca`, `.nsbta`, `.nsbma`, `.nsbtp`,
  `.nsbva`), as the field props keep theirs, a font or a particle resource that the converter doesn't take (`.nftr`,
  `.spa`), and other data (`.bin`). Tools such as NitroPaint edit them.

`archive.json` lists the members that the game stores LZ-compressed (`{"compressed": {"05.png": "lz10"}}`), which the
packer compresses again: an edited one as a valid stream of literals, an eighth larger than its data. A member that
differs between the versions is in `black2/` and `white2/` instead, as eight of the title screen's are. An unchanged
member packs as the game's own bytes. An edited one keeps the game's header with its new data and sizes: an image may
change size, which a sized image's character position section follows, and a palette its number of colors, unless it
has a PCMP section. A new member is the next number's file: a PNG becomes a sized character resource, 8bpp where its
palette has more than 16 colors and 4bpp otherwise, a `.pal` a palette, and the rest go in as they are.
`tools/data/graphics_archive.py dump ARCID_TITLE` writes an archive's directory again from both versions' files,
replacing it and any edits.

The same packer takes archives of other subjects, each in the directory for what it holds (`ARCHIVE_ROOTS`):
`res/graphics/` for the menus and apps, `res/demo/` for the demos and cutscenes, `res/field/gimmicks/` and
`res/field/effects/`, `res/particles/` and `res/fonts/`; `dump ARCID_X res/demo` writes one there. A name, like its
`ARCID_*`, is in one of them only. `res/fonts/font/` is `ARCID_FONT` (`a/0/2/3`): the game's five fonts, `00/` to
`04/`, and three palettes, the same in both versions. `res/particles/btlv_effvm_particles/` is
`ARCID_BTLV_EFFVM_PARTICLES` (`a/0/0/6`), the particle effects of the battle's moves, which `btlv_effvm.c` loads, and of
the intro, which `intro_particle.c` does: 739 resources, `000/` to `738/`, the same in both versions.

A model's GLB and PNGs are edited and packed as the field props' are (see "Map model sources"), with the same
converters. `dump` has [Apicula](https://github.com/scurest/apicula) convert each model, from `PATH` or `--apicula
PATH`; `pack`, which the build runs, needs only Python. The model's `data.json` is a prop's without the resource ID:
`glb_sha256`, the hash of the GLB as extracted, and `transformed` and `positions` where the GLB's positions went through
the model's node transforms. A GLB that still has its hash packs the game's model, and an edited one is converted back
into it: its positions, UVs, material colors and texture bindings, as for a prop. An edited PNG packs into the model's
textures as a texture set's does, in the texture's format and its shared palette. The GLB gives each of the model's draw
calls a material of its own, copying a material that several share (`awa_1.001`), since Blender joins the draw calls of
one material on export; edit the copies alike. A model that the converters don't take stays its `.nsbmd` file, as
`dump` says: one with textures that the texture converter doesn't decode (4x4 compressed or direct color), one without
a mesh, or one whose materials pair a texture with another palette than the converter gives it. A new model or texture
set is added as its `.nsbmd` or `.nsbtx` file. The archives dumped before the models were GLBs keep their `.nsbmd` and
`.nsbtx` files, which pack as they are, until they are dumped again: in scratch copies of the 19 such archives in
`res/graphics/`, 214 of their 233 models and all 31 texture sets convert and pack byte for byte.

`ARCID_GIMMICK_EXPOBJ_MARINETUBE` (`res/field/gimmicks/gimmick_expobj_marinetube/`) has 13 models and their
animations, and `ARCID_DEMO3D_RESOURCE` (`res/demo/demo3d_resource/`) the demos' 156 models, 2,472 textures and
297 animations; both pack byte for byte in both versions. An unchanged Blender export repacks byte for byte for 11
of the Marine Tube's 13 models and 126 of the demos' 156; moving one vertex of a Marine Tube model in Blender changes
one byte of it, which Apicula loads with the vertex moved, and an edited pixel packs into its texture. Of the others,
four demo models pack with their triangles encoded again, and the packer refuses the rest rather than move their
vertices: 17 have parts that their node transforms scale to nothing until an animation shows them (`hoeru_1`) or to a
hundredth (the eight `city_plasma_come`), whose positions Blender doesn't export exactly, eight have blended skin
weights, and three have shapes that set matrices as they draw.

A font is converted by `tools/data/nitro_font.py`, which `dump` and `pack` call, from the format as `gf_font.c` reads
it. `glyphs.png` has every glyph in the font's order, a cell each, left to right in rows of 32 (16 for a
font of 256 glyphs or fewer), with a magenta guide line around each cell. Its pixels are the glyphs' values, 2 bits
each: 0 is clear, and the game draws 1 in the letter color, 2 in the shadow's and 3 in the background's
(`GFL_TextRndUpdateColorIndexLUT`), so edit it in indexed mode; its colors are only a preview. The packer reads the
cells and not the guide lines. `font.json`, which `res/fonts/font.schema.json` describes, has the font information
(`lineFeed`, the default widths, `encoding` and the rest of FINF) and the glyph cells' size (`cellWidth`, `cellHeight`,
`baseline`, `maxCharWidth`, `bpp`), under `gf_font.c`'s names; the character maps, in the order the game searches them,
each a range of characters and its method (`direct`, `table` or `scan`); and a line for each glyph, its character
(with its code, `"code": "U+3000"`, where the character doesn't print) and its `left` offset, `width` and `advance`,
which the game keeps at the start of the glyph's cell (`GFL_FontGlyphCopy`). The packer computes the rest: the blocks'
offsets and sizes, each map's contents from the glyphs whose characters its range holds and no earlier range does, and
the widths block. That block is Game Freak's, not the format's per-glyph table: the three commonest advances, a 2-bit
class for each glyph, and a hash table of the other advances, with buckets where glyphs share a hash
(`GFL_FontGetCharWidthDynamic`). Two fonts keep the bytes that pad their classes, which aren't zeros, as
`classPadding`. To add a character, add its glyph at the end of `glyphs` and draw it in the next cell, adding a row of
cells where the sheet is full; every font ends in a `scan` map of every character, which takes it, or a map can be
added or a range extended. A changed pixel, width or advance, a new glyph and a new map pack and read back as the game
reads them, and all five fonts pack byte for byte in both versions. A font that the converter doesn't read, or wouldn't
build again byte for byte, stays its `.nftr` file, as `dump` says.

A particle resource is converted by `tools/data/nitro_particles.py`, which `dump` and `pack` call, from the format as
NitroSystem's particle library reads it (`SPLManager_LoadResources` in `lib/spl/`). `particles.json`, which
`res/particles/particles.schema.json` describes, has the emitters in order, which the game creates by their index
(`SPLManager_CreateEmitter`), and the textures. An emitter has its header's fields (`SPLResourceHeader`) under
`lib/spl`'s names, then the blocks that follow it where it has them, under the names of `SPLResource`'s pointers:
`scaleAnim`, `colorAnim`, `alphaAnim`, `textureAnim`, `childResource` and the behaviors (`gravityBehavior` to
`convergenceBehavior`); the packer sets the header's flags of the blocks from the blocks there are. Fixed-point fields
are the integers the game keeps, in 4096ths, angles in 65536ths of a turn, and the animations run over a particle's
life from 0 to 255, as the schema says for each; colors are `{red, green, blue}`, 0 to 31 each, the enumerations names
(`"emissionType": "circle_border"`), and textures are named. A block's `unused` keeps the fields and bits that the
library never reads but that the game's files don't leave zero (most emitters have some), so that they pack byte for
byte. Each texture is a PNG named after it (`texture00.png`, in the resource's order), RGBA as its format shows it:
A3I5 and A5I3, a palette's index and an alpha a pixel, are the formats of all but 23 of the resources' 2,843
textures, and 4 and 16 colors the rest, which `map_texture_data.py` decodes as a field texture's; direct color is
decoded too, and a texture
of another format (4x4 compressed, which none is) is its data as it is (`texture00.bin`). An unchanged texture packs as
the game's bytes, and an edited one is encoded again in its format and palette, which grows to the colors the PNG
needs, up to its format's; its size is the PNG's, a power of two from 8 to 1024. The packer computes the rest: the
counts, sizes and offsets, a texture animation's count, and the textures' headers but for their format, repeat, flip,
color 0 and sharing. An emitter is added by adding its object to `emitters`, and the code creates it by its index; a
changed field, an edited pixel, and an emitter added or removed pack and read back as the game reads them
(`tools/data/test_nitro_particles.py`). Besides the 739 of `a/0/0/6`, 40 resources of ten archives of
`res/graphics/` are directories too (`plist/`, `hall_of_fame_demo/` and others), and all of them pack byte for byte in
both versions. A resource that the converter doesn't read, or wouldn't build again byte for byte, stays its `.spa`
file, as `dump` says.

A movie is an animated PNG, `movie.png`, as `cdemo_control.c` plays the Game Freak logo (`res/demo/cdemo_gflogo/`) and
the opening (`res/demo/cdemo_openingsw/`, a movie in each version's directory): its frames are the archive's first
members, direct color bitmaps, 15-bit color with bit 15 set where the pixel shows, which the PNG keeps as RGBA, each
color's five bits widened to eight and alpha 255 where the pixel shows, 0 where it doesn't. It plays at the game's 15
frames a second in a browser or an image viewer, and an APNG editor, or `ffmpeg`, edits it; the packer composes the
frames as a viewer shows them, keeps each channel's top five bits and shows the pixels whose alpha is 128 or more, so an
unchanged frame packs as the game's own and an edited one changes only its member. `archive.json`'s `"movie"` gives the
frames' size (256 by 256, of which the game shows the top 192 rows), how many the archive had and their compression; the
members after them are numbered files from there (`626.bin`). The game plays as many frames as `cdemo_control.c`'s
`CDEMO_MOVIES` says, so a movie with more or fewer needs its count changed there too. `dump ARCID_X res/demo --movie
256x256` writes such an archive.
