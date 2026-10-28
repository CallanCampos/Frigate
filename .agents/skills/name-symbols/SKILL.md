---
name: name-symbols
description: Name what is still unnamed in a pokebw2 module whose code is decompiled. Covers address-named functions and data (func_, data_), unk fields, locals and types, placeholder parameters (a0, param1), switches and comparisons on bare numbers, and deciding when a number should be a constant or an enum. Claims the module's [Names] issue, works file by file with naming_report.py, renames with rename_symbol.py, and commits and publishes each file. Use for naming work ("name ov036", "next names", "the names issue", "unk fields", "magic numbers"), and for issue #325's per-module issues.
---

# Name a module

Every module's code is decompiled; what is left is names. Each module has a `[Names] Overlay NNN` issue (or
`[Names] ARM9 main`, `[Names] lib/<name>`, `[Names] Shared headers: include/<dir>`), tracked by issue #325.

## 1. Claim the module

Read the issue's comments. If someone has an active claim, pick another module. Otherwise comment
`Claiming overlay NNN for naming` with the file you start with, recheck for an earlier concurrent claim, and go. Release
the claim in a comment if you leave the module unfinished.

## 2. Take stock

```sh
.venv/bin/python tools/decomp/naming_report.py --module ov036        # counts by file, then every item
.venv/bin/python tools/decomp/version_map.py b2_us w2_us -o build/version_map.tsv --symbols-output build/version_map_symbols.tsv
```

The version map is what `rename_symbol.py` uses to carry names to White 2; make it again after a merge. Work through
the module's files in address order (`docs/source-files.md`), one commit per file.

## 3. Names: the rules

- **English.** No Japanese word and none of Game Freak's private abbreviations; `docs/code-organization.md`, Names, has
  the words and their English (`zukan` is `Pokedex`, `fldmmdl` `FieldActor`). Well-known acronyms stay (BGM, SE, HP, TM,
  NPC, OBJ).
- **From evidence only:** what the code does, what calls it and what it calls, the table slot it fills, the strings it
  passes, the file's own prefixes (`FldmmdlMove_`, `RailActor_`), swan's names of its neighbours, and the Black 1
  decomp's name where the Black 1 counterpart is certain ([aletheia](https://github.com/Project-Holon/aletheia):
  translate its names to ours and credit it with its URL in the commit; never copy its code). pret's decomps name the
  Gen 4 shapes. A name you can't support stays as it is: an `unk_2E4` with a known offset is better than a wrong word.
- **The file's conventions:** a field effect's table slots are `_Create`/`_Delete` and its task's `_Init`, `_Exit`,
  `_Update`, `_Draw`; a script command's handler is `Scrcmd_<Group>_<Name>`, `<Group>` its `scrcmd_<group>.c` file and
  `<Name>` the command's name in the scripts (`Scrcmd_Msg_InfoMsgClose`, `docs/scripts.md`), swan's included; a set of
  near-identical functions told apart only by their arguments is numbered (`InitFaceDirSet0`..`3`).
- **Copies stay address-named** where a name would mislead: a copy-pasted duplicate that does something other than its
  name suggests. Say so in the commit.
- **The ROM spells some names out.** `GFL_ASSERT` keeps its expression as text, so a name in an assert is Game
  Freak's: rename it all the same, and write that assert out with the ROM's text (`docs/code-standards.md`, rule 3).
  A file name passed to `GFL_HeapAllocate` is a string and never changes, and neither do the sound constants (the
  sound archive's own symbols) or the map models' and texture sets' (the assets' names). The constants of the other
  `generated/` lists, the text banks with their headers and message IDs, are renamed only with
  `tools/data/rename_constant.py`, which renames the data with them; an `ARCID_*` name of `include/constants/arc.h`
  names its `res/graphics/` directory, which is renamed with it.

## 4. Functions and data

Name a batch of a file's functions in one list, `old new` a line, and apply it:

```sh
.venv/bin/python tools/decomp/rename_symbol.py --file renames.txt
.venv/bin/python tools/script/sync_handler_names.py      # when a renamed function is a script command's handler
```

`rename_symbol.py` renames both versions' symbols, records the name in `config/names.txt` and rewrites the sources,
string literals included (look at `git diff` for any string it changed). A static's symbol takes its C name; an
address-named data object (`data_ov036_021d4b20`) is named the same way once you know what it holds. The sync script
gives the field script commands, their macros in `include/asm/field_script.inc` and their uses in `res/` the handlers'
new names; the build checks that the scripts assemble to the same bytes.

## 5. Fields, locals, parameters and types

- **Fields:** find every access to the struct before naming a field (the `struct-recovery` agent does this for a
  struct that several files share). Rename the field in its definition and at each access through that struct type only:
  another struct can have a field of the same name. A type that only exists as copies with `unk` fields becomes one
  definition in its owner's header (`docs/code-standards.md`, rule 15).
- **Parameters and locals** (`a0`, `param1`, `var2`) take the name of what they hold, from their uses and their callers'
  arguments; the prototype in the header changes with the definition.
- **Accessors named after an offset** (`Btlv_GetUnk2E4`) are renamed with the field they read.
- No tool renames these: edit them by hand, then let the build and `check_style.py` check you. A field can be set
  in modules far from its struct's (a camera animation's flags are set by the field's fades, its entrances and an
  overlay 12 event): `ninja -k 0` compiles everything and lists each `'unk48' is not a member` that is left.

## 6. Numbers

`naming_report.py` lists the case labels and the comparisons that are bare numbers. They are leads; decide each:

- **An ID of a list** (a species, item, move, ability, type, flag, var, zone, trainer, sound, message, text bank,
  field script): use its constant from `generated/` (`#include "constants/species.h"`, ...). If the list lacks a name for
  it, it keeps its number until the data's pass names it.
- **A value set** (a task's sequence numbers, a result, a mode, a kind, a command, a menu entry, an index into a fixed
  table) that a switch, a comparison and a field or parameter share: an `enum` in the owner's header, as Game Freak
  writes them, closed by a count where the code uses one (`..._MAX`). Pass and compare the constant everywhere the set
  is used; don't change the type of a field or parameter that holds it (rule 15).
- **The SDK's values** (display modes, VRAM banks, key bits, `GX_*`, `PAD_*`): the SDK's constants.
- **A size or count:** `NELEMS(table)` or `sizeof` where that is what it is; otherwise a `#define` with a name.
- **Bits that combine:** `#define`s, or an enum with explicit values when a field holds the combination.
- **Stays a number:** coordinates, pixel and tile positions, colors, timings and frame counts, fixed-point factors and
  other measurements used where they are written, and a loop's literal bound that is just the size of what it walks.
- Constants never change the code, but an enum's type can (`-enum int`): build after each file.

## 7. Verify, commit and publish

```sh
ninja                                             # every file, both versions, both SHA1s, no warnings
.venv/bin/python tools/decomp/check_style.py <changed C files>
```

Commit one file's names with a message that says what was named and from what evidence (`Name fieldmap.c's surface and
zone predicate functions`), crediting aletheia where its names were used. Then `git fetch origin && git merge
origin/main`, build again, and `git push origin HEAD:main` (the `finish-file` skill has the details). After each
commit, or a run of them, comment on the module's issue with the files done, the commits and what is left. When
`naming_report.py --module` lists nothing but deliberate leftovers, post a last comment listing them with their reasons
and close the issue.
