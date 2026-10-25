# port/tools/progress/

The port-readiness measurement of the 2026-10-16 research, kept for re-measuring at each sync: how much of the game
(ARM9 main, its autoloads and every overlay, in both versions) is matching C that the ROM links (tier a), C that the
ROM doesn't link yet (tier b), or no C at all (tier c). Frigate's host build compiles tiers (a) and (b) and stubs
tier (c) ([../gen_stubs.py](../gen_stubs.py)), so the tiers say how much of the game the port can run as C.
[STATUS.md](../../docs/STATUS.md)'s Tiers line is its headline; the findings and how they were made are in
[../../docs/research/decomp-progress.md](../../docs/research/decomp-progress.md).

```sh
sh port/tools/progress/run_all.sh <repo> [outdir]     # outdir defaults to port/tools/progress/out
```

It needs a configured checkout with `extract/{b2_us,w2_us}` (from `ninja extract/b2_us/config.yaml
extract/w2_us/config.yaml`), the checkout's `.venv`, and read-only `gh` for the issues. It takes about 5 minutes and
writes nothing under `<repo>`.

| File | What it does | Reads | Writes, in `outdir` |
| --- | --- | --- | --- |
| [run_all.sh](run_all.sh) | Runs every step below in order, and fetches ByteMatch's measures of the decomp | `<repo>` | everything below, and `bytematch/` |
| [progress.py](progress.py) | The tiers, per function (from each module's `symbols.txt`), per file and per data range | `config/<version>/arm9`'s `symbols.txt` and `delinks.txt`, the C of `src/` and `lib/`; `--rev` reads them from a commit | `<version>/functions.tsv`, `files.tsv`, `summary.json` |
| [cparse.py](cparse.py) | progress.py's scanner of a C file's top-level function and data definitions, which tells tier (b) from (c) in an incomplete file | a C file | — |
| [rombytes.py](rombytes.py) | The ROM's bytes at a module's address, so that progress.py tells the linker's interworking veneers from game code with no C | `extract/<version>/` | — |
| [compile_check.py](compile_check.py) | Compiles every incomplete file with the ROM's compiler and compares its functions, code and data bytes and data pointers with the original file's ranges; a file that `config/deadstrip.toml` lists is dead-stripped first, as the link does, and `--all`, which takes the complete files too, assembles a library's assembly source (`fp_fastI.s`) as the build does. Its test is `progress_compile_check` ([../../tests/progress/test_compile_check.py](../../tests/progress/test_compile_check.py)) | the repository's compile probe: `tools/decomp/compiler_probe.py`'s `probe_defaults` (the build's compiler and flags for the file, from `configure.py`), its version defines, include directories and compiler command, `configure.py`'s `source_assembler`, and `tools/build/deadstrip.py` | `compile_b2_us.tsv` |
| [gh_claims.py](gh_claims.py) | The decomp's open `[Decomp] Overlay NNN` issues and who claims each, which says whose work the missing C is | `gh_issues.json`, fetched read-only | `gh_claims.md` |
| [summarize.py](summarize.py) | The report's tables: by module, by origin, the overlays by missing C, the data | progress.py's output, with compile_check.py's and the issues | `tables_<version>.md`, `all_overlays_b2_us.md` |
| [history.py](history.py) | progress.py at the last first-parent commit of each day, cached per commit: the decomp's pace | the repository's history | `history/` |
| [area_history.py](area_history.py) | Tier (c) per day in each area the port's milestones wait on (main, the ITCM, the LTD autoload, overlays 11, 12, 36, 167, 168, 182, 189) | `history/` | `area_history.md` |
| [nonmatching_scan.py](nonmatching_scan.py) | `docs/nonmatching-functions.md`'s rows by the kind of difference they describe; the semantic ones are C that may not do what the ROM does | `docs/nonmatching-functions.md` | `nonmatching_classes.txt` |
