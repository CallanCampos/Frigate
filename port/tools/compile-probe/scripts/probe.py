#!/usr/bin/env python3
"""Syntax-probe every .c file of src/ and lib/ with clang for one target, and record diagnostics.

Usage: probe.py REPO OUTDIR NAME -- <clang args...>
Writes OUTDIR/NAME/diags.tsv (file, status, severity, flag, loc, message) and OUTDIR/NAME/status.tsv.
The repo is read only: clang runs with -fsyntax-only and writes nothing.

run_all.sh runs it for the DS (ARMv5TE and Thumb), each 64-bit host, wasm32 and i386, and the arm64 passes with
the MWCC shim and with the undefined-behaviour warnings: how the game's and the libraries' C compiles for each
(port/docs/research/compile-probe.md, "Baseline compile" and "Host compile").
"""
import os, re, sys, subprocess, pathlib, concurrent.futures as cf

repo = pathlib.Path(sys.argv[1]).resolve()
out = pathlib.Path(sys.argv[2]) / sys.argv[3]
args = sys.argv[sys.argv.index("--") + 1:]
clang = os.environ.get("CLANG", "/opt/homebrew/opt/llvm/bin/clang")
jobs = int(os.environ.get("JOBS", "4"))
only = os.environ.get("ONLY")  # optional substring filter
out.mkdir(parents=True, exist_ok=True)

files = sorted(str(p.relative_to(repo)) for d in ("src", "lib") for p in (repo / d).rglob("*.c"))
if only:
    files = [f for f in files if only in f]

INC = ["-Iinclude"] + [f"-I{p}" for p in sorted(str(q.relative_to(repo)) for q in repo.glob("lib/*/include"))] + ["-Ibuild/include/generated"]
# lib/<name>/src files may include their own src-local headers through "..." which clang finds next to the file.
DIAG = re.compile(r"^(?P<loc>[^:\s][^:]*:\d+:\d+): (?P<sev>error|warning|fatal error): (?P<msg>.*?)(?: \[(?P<flag>-W[^\],]+)[^\]]*\])?$")

def run(f):
    cmd = [clang, "-fsyntax-only", "-fno-color-diagnostics", "-ferror-limit=0", "-fno-caret-diagnostics",
           "-Wno-unknown-pragmas"] + args + INC + [f]
    p = subprocess.run(cmd, cwd=repo, capture_output=True, text=True, errors="replace")
    rows = []
    for line in p.stderr.splitlines():
        m = DIAG.match(line)
        if m:
            sev = "error" if "error" in m["sev"] else "warning"
            rows.append((sev, m["flag"] or "", m["loc"], m["msg"].replace("\t", " ")))
    return f, p.returncode, rows

with cf.ThreadPoolExecutor(jobs) as ex, open(out / "diags.tsv", "w") as d, open(out / "status.tsv", "w") as s:
    for f, rc, rows in ex.map(run, files):
        ne = sum(1 for r in rows if r[0] == "error")
        nw = len(rows) - ne
        s.write(f"{f}\t{'pass' if rc == 0 else 'fail'}\t{ne}\t{nw}\n")
        for r in rows:
            d.write(f"{f}\t{r[0]}\t{r[1]}\t{r[2]}\t{r[3]}\n")
print(out)
