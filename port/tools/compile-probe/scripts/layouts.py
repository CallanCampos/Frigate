#!/usr/bin/env python3
"""Dump every complete record's size/alignment, per TU, for one target, and aggregate.
Usage: layouts.py REPO OUT.tsv -- <clang args>   (writes type<TAB>size_bits<TAB>align_bits<TAB>first_tu)"""
import os, re, sys, subprocess, pathlib, concurrent.futures as cf
repo = pathlib.Path(sys.argv[1]).resolve(); out = sys.argv[2]
args = sys.argv[sys.argv.index("--") + 1:]
clang = os.environ.get("CLANG", "/opt/homebrew/opt/llvm/bin/clang")
files = sorted(str(p.relative_to(repo)) for d in ("src", "lib") for p in (repo / d).rglob("*.c"))
INC = ["-Iinclude"] + [f"-I{q.relative_to(repo)}" for q in sorted(repo.glob("lib/*/include"))] + ["-Ibuild/include/generated"]
def run(f):
    cmd = [clang, "-fsyntax-only", "-w", "-ferror-limit=0", "-Xclang", "-fdump-record-layouts-simple",
           "-Xclang", "-fdump-record-layouts-complete"] + args + INC + [f]
    p = subprocess.run(cmd, cwd=repo, capture_output=True, text=True, errors="replace")
    recs = []; t = None
    for line in p.stdout.splitlines():
        if line.startswith("Type: "): t = line[6:].strip()
        elif t and line.strip().startswith("Size:"): size = int(line.split(":")[1])
        elif t and line.strip().startswith("Alignment:"):
            recs.append((t, size, int(line.split(":")[1]))); t = None
    return f, recs
agg = {}
with cf.ThreadPoolExecutor(int(os.environ.get("JOBS", "4"))) as ex:
    for f, recs in ex.map(run, files):
        for t, s, a in recs:
            agg.setdefault((t, s, a), f)
with open(out, "w") as o:
    for (t, s, a), f in sorted(agg.items()):
        o.write(f"{t}\t{s}\t{a}\t{f}\n")
print(len(agg))
