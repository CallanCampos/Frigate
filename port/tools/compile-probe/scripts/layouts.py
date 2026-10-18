#!/usr/bin/env python3
"""The size and alignment of every struct and union of the game's and the libraries' C for one target, as clang lays
them out: what layout_diff.py compares between MWCC's 32-bit layouts and a host's.

    layouts.py REPO OUT.tsv -- CLANG_ARGS...

Compiles every .c of REPO's src/ and lib/ with -fsyntax-only and clang's record-layout dump (the include paths of the
ROM build, build/include/generated among them; nothing is written to the repository) and writes one row per distinct
record and layout: its type, size and alignment in bits, and the first file seen with it. CLANG and JOBS set the
compiler and the parallelism. run_all.sh runs it for i386, the DS (thumbv5te), arm64 macOS, x86-64 Windows and wasm32,
and with the shim's types (port/docs/research/compile-probe.md, "Struct layout hazards").
"""
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
