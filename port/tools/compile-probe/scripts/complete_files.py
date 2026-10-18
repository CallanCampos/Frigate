#!/usr/bin/env python3
"""Every C file of the decomp (src/ and lib/) with its state in Black 2's configs: complete (matching and linked into
the ROM), incomplete (registered, still linking the ROM's object) or unregistered.

    complete_files.py REPO OUT.tsv

Reads REPO/config/b2_us/**/delinks.txt and writes a row per file to OUT.tsv (run_all.sh: files.tsv), which
summarize.py takes to split a pass's results by completeness. The first step of run_all.sh
(port/docs/research/compile-probe.md).
"""
import sys, pathlib, re
repo = pathlib.Path(sys.argv[1])
status = {}
for dl in (repo / "config/b2_us").rglob("delinks.txt"):
    cur = None
    for line in dl.read_text(errors="replace").splitlines():
        if line and not line[0].isspace() and line.rstrip().endswith(":"):
            cur = line.rstrip()[:-1]
            status.setdefault(cur, False)
        elif cur and line.strip() == "complete":
            status[cur] = True
allc = sorted(str(p.relative_to(repo)) for d in ("src", "lib") for p in (repo / d).rglob("*.c"))
with open(sys.argv[2], "w") as f:
    for c in allc:
        st = "complete" if status.get(c) else ("incomplete" if c in status else "unregistered")
        f.write(f"{c}\t{st}\n")
