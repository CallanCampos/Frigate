# Boundaries, names and placement

The linker placed one object per original file, so a file's `.text`, `.rodata`, `.data` and `.bss` are each one
contiguous range, and the files come in the same order in every section. A file is one `delinks.txt` entry covering
its whole range in every section, even while only part of it is written. Never split a file into several entries to
link the matching parts early, and never put two files in one entry.

## Finding them

```sh
.venv/bin/python tools/decomp/source_files.py ov033                       # embedded file names and who uses them
.venv/bin/python tools/decomp/source_files.py ov033 --profile START END   # boundary scores per function
.venv/bin/python tools/decomp/source_files.py ov033 --sections START END  # data a .text range refers to, per section
```

Pipe the output through `head` or `grep`; for a big overlay, use the `boundary-scout` agent.

- **Strings are the anchors.** `GFL_HeapAllocate(..., "trial_house.c", line)` and asserts embed the file's name in
  its `.data`. Every function that passes it belongs to that file, and the line numbers rise through the file.
- **Between anchors:** a real boundary has `order` 0 (or near 0 for shared data) and few crossing `calls`.
- **Data:** a file's `.data` ends at its last object. When the build says "Last symbol X ... not contained within
  the file's section range", the range is short of an object: extend it, or, if the object belongs to the next file,
  fix the next boundary. `config_fixes.py add-data` adds an object nothing references, so the one before doesn't
  seem to run on.
- **Statics:** two files' `.bss` statics are addressed from separate bases, which shows a boundary.
- **Sizes:** MWCC sorts a file's static data by size (`docs/matching.md`, "Static data is sorted by size"), so a run of
  `.rodata` that gets smaller marks a boundary. `btl_server_flow_sub.c`'s 184-byte item effects followed by a 118-byte
  and a 96-byte table split off `btl_handler_work.c` and `btl_server_cmd.c`, whose users confirmed it. It moves `.text`
  boundaries too: overlay 127's two 8-byte wall tables sit after the gimmick's 128-byte placements, so they start
  `infinity_dungeon.c`'s run, and the function that reads them (0x021efaf0), with everything after it, is that file's.
- **Line numbers:** functions that pass `__LINE__` (asserts, and calls like ov167's `PushState`) climb through a file
  and start low again in the next. `btl_server_flow_sub.c` begins where `PushState`'s lines drop from 16185 to 619.
- **SPL and other library code** may be in reverse order (see `docs/decompiling.md`'s compiler section).

## Naming

- From the ROM's string when there is one. Otherwise name it for what it does (`gimmick_nacrene.c`), never with an
  overlay number or a counter. Say a guessed name is guessed in the header comment and in the commit message.
- Follow the names of the overlay's own files: overlay 12's script plugin table and the files `docs/scripts.md` lists
  (`scrcmd_*.c`, `event_*.c`, `wbt_*.c`).

## Placement

- Overlays go in `src/ovNNN/`. Main goes by library, by link order: the game's own code in `src/system/` (below
  GFL's block, which starts at `heapsys.c`, 0x02039a80), Game Freak's library in `src/gfl/` (through `str_sjis.c`),
  then the libraries built apart from the game, each in `lib/<name>/` with its compiler in `library.toml`: SPL in
  `lib/spl/src/` from 0x02050a40, and later `lib/nitro/` and `lib/nnsys/` (headers only so far). `grep -E '^(src|lib)/'
  config/b2_us/arm9/delinks.txt` shows the current files. Headers mirror these under `include/` and
  `lib/<name>/include/`. A library's private header stays with its sources (`lib/spl/src/spl_internal.h`).
- A header is named after the original file that owns its declarations, or after swan's header for it.
- When the evidence for which library a file belongs to is ambiguous (the user once asked "wouldn't that be gfl
  though?" about `printsys.c`), state the evidence, such as its address against GFL's block and the strings, when you
  place it. Ask only if there is no evidence either way.

## White 2

`add_source_file.py` derives White 2's ranges from the version map (`build/version_map.tsv`). Functions marked
`different` there need `#ifdef BLACK2`/`WHITE2` code. Check them with `compiler_probe.py --version w2_us`. White
2-only symbols have a `_w2_us` suffix.
