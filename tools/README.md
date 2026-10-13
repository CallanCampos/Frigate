# Tools

The repository's own tools are Python, in a package per job, so that a script imports another by its place
(`from tools.data.datajson import load`). Run them from the repository's root with `.venv/bin/python`, such as
`.venv/bin/python tools/decomp/show_func.py NAME`; the ones the build runs use only the standard library, since the
build runs them with the system's `python3`.

| Directory | What it holds |
| --- | --- |
| `build/` | Steps of the build and checks of its output: the ROM's file system (`files_tree.py`), mwccarm's depfiles, the linker script for a shifted build, objdiff's config and progress, comparing two ROMs (`romdiff.py`) and verifying a DSi ROM |
| `decomp/` | Decompiling: probing a file against the ROM (`compiler_probe.py`), showing a function's asm (`show_func.py`), its locals, respellings, the permuter, finding source files, and changing the dsd configs of both versions (`add_source_file.py`, `mark_complete.py`, `rename_symbol.py`, `config_fixes.py`) |
| `data/` | The game data in JSON: the shared reader and validator (`datajson.py`), NARC archives, the constant lists (`gen_constants.py`, `make_constants.py`, `rename_constant.py`) and a packer per kind of data (`species_data.py` and the rest), see [docs/data.md](../docs/data.md) |
| `text/` | The game's text: its message files (`text_data.py`), the messages taken from the JSON (`text_sources.py`), the message ID headers and the easy chat words, see [docs/data.md](../docs/data.md) |
| `script/` | The field and trainer AI scripts' disassemblers and command table, see [docs/scripts.md](../docs/scripts.md) |
| `compiler_tests/` | Small C files that pin down how the compiler behaves |

`dsd`, `mwccarm`, `objdiff-cli` and `wibo` are the external tools, which `configure.py` downloads.
