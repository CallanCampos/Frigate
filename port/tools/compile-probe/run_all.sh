#!/bin/sh
# Re-runs the whole 64-bit portability probe as a regression metric. Read-only on the repo: clang runs with
# -fsyntax-only or emits IR to stdout; every output goes under this directory.
#   sh run_all.sh /path/to/pokebw2-checkout
# Needs Homebrew LLVM (clang 23) at /opt/homebrew/opt/llvm/bin/clang and the repo's .venv (PyYAML).
set -e
REPO=${1:?repo}
W=$(cd "$(dirname "$0")" && pwd)
S=$W/scripts
PY=$REPO/.venv/bin/python
cd "$REPO"
HOSTW=$(cat $S/host_warnings.txt)
COMMON="-D__thumb -std=c99 -DBLACK2"
$PY $S/complete_files.py . $W/files.tsv
sh $S/dialect_scan.sh "$REPO" > $W/dialect_scan.txt
# 1. syntax probes
$PY $S/probe.py . $W base_armv5te -- --target=armv5te-none-eabi -ffreestanding -std=c99 -DBLACK2 >/dev/null
$PY $S/probe.py . $W base_thumb -- --target=thumbv5te-none-eabi -ffreestanding $COMMON >/dev/null
for t in arm64-apple-macos14 x86_64-unknown-linux-gnu x86_64-pc-windows-gnu x86_64-pc-windows-msvc wasm32-unknown-unknown i386-unknown-linux-gnu; do
  $PY $S/probe.py . $W host_$t -- --target=$t $COMMON $HOSTW >/dev/null
done
$PY $S/probe.py . $W shim2_arm64 -- -I$W/shim/include --target=arm64-apple-macos14 -ffreestanding -fshort-wchar -fdeclspec \
  -include $W/shim/port_shim.h $COMMON $HOSTW >/dev/null
$PY $S/probe.py . $W ub_arm64 -- --target=arm64-apple-macos14 -ffreestanding $COMMON $(cat $S/ub_warnings.txt) >/dev/null
$PY $S/probe.py . $W fncast_wasm32 -- --target=wasm32-unknown-unknown -ffreestanding $COMMON -Wcast-function-type -Wno-cast-function-type-strict >/dev/null
# 2. layouts (i386 SysV is the MWCC proxy: it matches every ROM-checked sizeof and size comment)
for t in i386-unknown-linux-gnu thumbv5te-none-eabi arm64-apple-macos14 x86_64-pc-windows-gnu wasm32-unknown-unknown; do
  $PY $S/layouts.py . $W/layouts_$t.tsv -- --target=$t -ffreestanding $COMMON >/dev/null
done
$PY $S/layouts.py . $W/layouts_shim_arm64.tsv -- -I$W/shim/include_types --target=arm64-apple-macos14 -ffreestanding $COMMON >/dev/null
$PY $S/layouts.py . $W/layouts_shim_wasm32.tsv -- -I$W/shim/include_types --target=wasm32-unknown-unknown -ffreestanding $COMMON >/dev/null
for t in thumbv5te-none-eabi arm64-apple-macos14 x86_64-pc-windows-gnu wasm32-unknown-unknown shim_arm64 shim_wasm32; do
  f=$W/layouts_$t.tsv
  $PY $S/layout_diff.py . $W/layouts_i386-unknown-linux-gnu.tsv $f $W/layout_mwcc_vs_$t.tsv > $W/layout_mwcc_vs_$t.summary
done
$PY $S/check_size_comments.py . $W/layouts_i386-unknown-linux-gnu.tsv $W/layout_mwcc_vs_arm64-apple-macos14.tsv > $W/size_comments.txt
$PY $S/rom_sizeof_check.py . $W > $W/rom_sizeof_check.txt
# 3. hazard classification and IR checks
$PY $S/classify64.py . $W/host_arm64-apple-macos14 --prefix src/,include/ --show NONE > $W/classify_game_raw.txt
$PY $S/classify64.py . $W/shim2_arm64 --prefix src/,include/ --show NONE > $W/classify_game_shim.txt
$PY $S/classify64.py . $W/shim2_arm64 --prefix lib/ --show NONE > $W/classify_lib_shim.txt
$PY $S/alloc_sizes.py . > $W/alloc_sizes.txt
$PY $S/unions.py . > $W/unions.txt
$PY $S/ast_file_types.py . $W/layout_mwcc_vs_arm64-apple-macos14.tsv $W/file_overlay_types.tsv > $W/file_overlay_types.txt
$PY $S/symbols_ir.py . $W/ir_arm64 -- --target=arm64-apple-macos14 -ffreestanding $COMMON > $W/ir_arm64.txt
$PY $S/signatures_ir.py . $W/signatures_arm64.tsv -- --target=arm64-apple-macos14 -ffreestanding $COMMON > $W/signatures_arm64.txt
$PY $S/text_coverage.py . > $W/text_coverage.txt
# 4. headline metrics
{
  echo "# compile-probe metrics $(date +%F) $(git -C "$REPO" rev-parse --short HEAD)"
  for r in base_armv5te base_thumb host_i386-unknown-linux-gnu host_wasm32-unknown-unknown host_arm64-apple-macos14 host_x86_64-unknown-linux-gnu host_x86_64-pc-windows-gnu host_x86_64-pc-windows-msvc shim2_arm64; do
    printf '%-36s pass=%s/%s\n' $r "$(grep -c '	pass	' $W/$r/status.tsv)" "$(wc -l < $W/$r/status.tsv | tr -d ' ')"
  done
  $PY $S/table.py $W host_i386-unknown-linux-gnu host_arm64-apple-macos14 host_x86_64-pc-windows-gnu shim2_arm64
  for f in $W/layout_mwcc_vs_*.summary; do echo "$(basename $f .summary): $(head -1 $f)"; done
  cat $W/size_comments.txt $W/rom_sizeof_check.txt | grep -E "size comments|checked|equal="
  head -1 $W/classify_game_shim.txt; head -1 $W/classify_lib_shim.txt; head -1 $W/file_overlay_types.txt
  cat $W/ir_arm64.txt; head -1 $W/signatures_arm64.txt; head -1 $W/unions.txt
} > $W/metrics.txt
cat $W/metrics.txt
