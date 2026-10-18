# Compile probe

Compiles every C file in `src/` and `lib/` with clang for the DS target and for 64-bit hosts (arm64 macOS, x86-64
Linux and Windows, wasm32, i386), counts pointer/int and other portability warnings by kind and module, and compares
struct layouts against i386 SysV, which matches MWCC on every size the ROM records. The game-code warning count is a
port metric that must only go down.

    sh port/tools/compile-probe/run_all.sh <configured checkout>

Needs Homebrew LLVM at `/opt/homebrew/opt/llvm/bin/clang`, the checkout's `.venv` and `build/include/generated`.
About 8 minutes on an M2. Outputs and `metrics.txt` are written into this directory (ignored by git). The findings are
in `port/docs/research/compile-probe.md`.

| File | What it is |
| --- | --- |
| `run_all.sh` | Runs every pass: the syntax probes for the DS and each host, the layouts, and the summaries |
| `scripts/host_warnings.txt` | The warning flags of the host passes, which `run_all.sh` reads with `$(cat ...)`, so the file holds flags alone and no comment |
| `scripts/ub_warnings.txt` | The same for the arm64 pass that looks for undefined behaviour |
| `scripts/complete_files.py` | Each C file's state in Black 2's configs (complete, incomplete, unregistered): `files.tsv` |
| `scripts/dialect_scan.sh` | Counts the constructs of MWCC's dialect in `src/`, `lib/` and `include/`: `dialect_scan.txt` |
| `scripts/probe.py` | One syntax pass: clang `-fsyntax-only` over every C file for one target, with its diagnostics: `<pass>/diags.tsv`, `status.tsv` |
| `scripts/summarize.py` | One pass's results by module and kind of diagnostic, by hand |
| `scripts/table.py` | The 64-bit warnings of several passes by flag, game and libraries apart, for `metrics.txt`: the port's 64-bit warning metric |
| `scripts/classify64.py` | One host pass's pointer and int sites classed by their source line: `classify_*.txt` |
| `scripts/sites_no_overlay.py` | The game's 64-bit sites that aren't overlay IDs, by file |
| `scripts/layouts.py` | Every struct's and union's size and alignment for one target: `layouts_<target>.tsv` |
| `scripts/layout_diff.py` | Two targets' layouts compared, MWCC's (i386) against each other's: `layout_mwcc_vs_<target>.tsv` and `.summary` |
| `scripts/check_size_comments.py` | The decomp's `// size:` comments against the i386 layouts: `size_comments.txt` |
| `scripts/rom_sizeof_check.py` | The sizes the ROM's `return sizeof(T);` functions return against each target's: `rom_sizeof_check.tsv` and `.txt` |
| `scripts/ast_file_types.py` | The structs the game overlays on data loaded from its archives, from clang's AST, and whether they change at 64 bits: `file_overlay_types.tsv` and `.txt` |
| `scripts/typemap.py` | Typedef names to clang's record keys, for `ast_file_types.py` |
| `scripts/file_overlay_types.py` | The same as `ast_file_types.py` by text pattern, its first take; not in `run_all.sh` |
| `scripts/unions.py` | Unions that mix pointer and integer members: `unions.txt` |
| `scripts/alloc_sizes.py` | Heap allocations and clears or copies sized by a literal rather than `sizeof`: `alloc_sizes.txt` |
| `scripts/argorder.py` | Calls with two side-effecting calls among their arguments, whose order C leaves open; not in `run_all.sh` |
| `scripts/symbols_ir.py` | Whether every file compiles to IR and defines no external symbol another defines: `ir_arm64/`, `ir_arm64.txt` |
| `scripts/signatures_ir.py` | Functions declared with different signatures in different files: `signatures_arm64.tsv` and `.txt` |
| `scripts/text_coverage.py` | Each module's `.text` in C files and in complete ones: `text_coverage.txt` |
| `shim/port_shim.h` | Force-included in the "arm64 + shim" pass: MWCC's built-ins and keywords that clang lacks (`__memcpy`, wide strings) |
| `shim/include/gfl/overlay.h` | That pass's copy of the decomp's overlay header, with `OVERLAY_ID(n)` as `n`, as in the host build |
| `shim/include_types/types.h` | The base types with 64-bit integers aligned as MWCC aligns them, for the layout pass |
