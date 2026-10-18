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
| `scripts/*.py`, `scripts/dialect_scan.sh` | The passes and the summaries; each opens with what it measures |
| `shim/port_shim.h` | Force-included in the "arm64 + shim" pass: MWCC's built-ins and keywords that clang lacks (`__memcpy`, wide strings) |
| `shim/include/gfl/overlay.h` | That pass's copy of the decomp's overlay header, with `OVERLAY_ID(n)` as `n`, as in the host build |
| `shim/include_types/types.h` | The base types with 64-bit integers aligned as MWCC aligns them, for the layout pass |
