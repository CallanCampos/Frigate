#!/bin/sh
# Re-runs the port-readiness measurement: how much of the game (ARM9 main, its autoloads and every overlay, both
# versions) is matching C, C not linked yet, or no C, which says what Frigate compiles from C and what it stubs.
#   sh run_all.sh <repo> [outdir]
# <repo> must have extract/{b2_us,w2_us} (dsd extract) and .venv; nothing is written under <repo>. It runs progress.py
# for both versions, compile_check.py, gh_claims.py on the decomp's overlay issues (read-only gh), summarize.py's
# tables, history.py and area_history.py, nonmatching_scan.py and ByteMatch's endpoints, into outdir (./out by
# default). STATUS.md's Tiers line comes from it; the findings are in port/docs/research/decomp-progress.md.
set -e
REPO=${1:?repo path}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${2:-$HERE/out}
PY=$REPO/.venv/bin/python
mkdir -p "$OUT"
# 1. tiers per function / file / data range, both versions (about 6 s each)
"$PY" "$HERE/progress.py" --root "$REPO" --version b2_us --out "$OUT/b2_us"
"$PY" "$HERE/progress.py" --root "$REPO" --version w2_us --out "$OUT/w2_us"
# 2. compile every incomplete file and compare code/data/pointers with the original ranges (a few minutes)
"$PY" "$HERE/compile_check.py" --root "$REPO" --version b2_us --out "$OUT/compile_b2_us.tsv" > "$OUT/compile_b2_us.log"
# 3. GitHub tracking (read-only)
gh issue list --repo fuddlesworth/pokebw2 --state all --limit 500 \
    --json number,title,state,comments,createdAt,closedAt,body,labels > "$OUT/gh_issues.json"
python3 "$HERE/gh_claims.py" "$OUT/gh_issues.json" > "$OUT/gh_claims.md"
# 4. tables
python3 "$HERE/summarize.py" --in "$OUT/b2_us" --issues "$OUT/gh_issues.json" --compile "$OUT/compile_b2_us.tsv" \
    > "$OUT/tables_b2_us.md"
python3 "$HERE/summarize.py" --in "$OUT/b2_us" --issues "$OUT/gh_issues.json" --compile "$OUT/compile_b2_us.tsv" \
    --all-overlays > "$OUT/all_overlays_b2_us.md"
python3 "$HERE/summarize.py" --in "$OUT/w2_us" > "$OUT/tables_w2_us.md"
# 5. history: one measurement per day on the first-parent history (cached per commit), and per-area tier (c)
"$PY" "$HERE/history.py" --root "$REPO" --version b2_us --out "$OUT/history"
python3 "$HERE/area_history.py" "$OUT/history" > "$OUT/area_history.md"
# 6. nonmatching rows by kind of difference
python3 "$HERE/nonmatching_scan.py" "$REPO" > "$OUT/nonmatching_classes.txt"
# 7. ByteMatch (CI's dashboard): project, per-category measures, history and the full objdiff report
mkdir -p "$OUT/bytematch"
curl -sS -o "$OUT/bytematch/project.json" 'https://bytematch.dev/fuddlesworth/pokebw2.json'
curl -sS -o "$OUT/bytematch/history_b2.json" 'https://bytematch.dev/fuddlesworth/pokebw2.json?mode=history&version=b2_us'
curl -sS -o "$OUT/bytematch/report_b2.json" 'https://bytematch.dev/fuddlesworth/pokebw2.json?mode=report&version=b2_us'
echo "done: $OUT"
