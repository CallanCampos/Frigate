#!/usr/bin/env python3
"""List source files and whether each is marked complete in config/b2_us delinks. Usage: complete_files.py REPO OUT.tsv"""
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
