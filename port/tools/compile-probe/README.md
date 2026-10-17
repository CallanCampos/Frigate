# Compile probe

Compiles every C file in `src/` and `lib/` with clang for the DS target and for 64-bit hosts (arm64 macOS, x86-64
Linux and Windows, wasm32, i386), counts pointer/int and other portability warnings by kind and module, and compares
struct layouts against i386 SysV, which matches MWCC on every size the ROM records. The game-code warning count is a
port metric that must only go down.

    sh port/tools/compile-probe/run_all.sh <configured checkout>

Needs Homebrew LLVM at `/opt/homebrew/opt/llvm/bin/clang`, the checkout's `.venv` and `build/include/generated`.
About 8 minutes on an M2. Outputs and `metrics.txt` are written into this directory (ignored by git). `shim/` holds
the host shim used for the "arm64 + shim" pass (`OVERLAY_ID(n)` as `n`, `__memcpy`, wide strings). The findings are
in `port/docs/research/compile-probe.md`.
