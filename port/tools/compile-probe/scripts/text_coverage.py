#!/usr/bin/env python3
"""How much of each module's .text is covered by a C source file in the b2_us delinks (complete or not).
Usage: text_coverage.py REPO"""
import re, sys, pathlib, collections
repo = pathlib.Path(sys.argv[1])
tot = collections.Counter(); cov = collections.Counter(); covc = collections.Counter()
for dl in sorted((repo / "config/b2_us").rglob("delinks.txt")):
    mod = dl.parent.name if dl.parent.name != "arm9" else "arm9 main"
    grp = "overlays" if dl.parent.parent.name == "overlays" else mod
    cur = None; complete = False
    for line in dl.read_text().splitlines():
        m = re.match(r"\s+\.text\s+start:0x([0-9a-f]+) end:0x([0-9a-f]+)", line)
        if not line.strip(): continue
        if not line[0].isspace(): cur = line.rstrip(":"); complete = False; continue
        if line.strip() == "complete": complete = True; continue
        if m:
            n = int(m[2], 16) - int(m[1], 16)
            if cur is None: tot[grp] += n
            elif cur.endswith(".c"):
                cov[grp] += n
                if complete: covc[grp] += n
for g in sorted(tot):
    print(f"  {g:14s} .text={tot[g]:9d}  in C files={cov[g]:9d} ({100*cov[g]/max(tot[g],1):5.1f}%)  complete={covc[g]:9d} ({100*covc[g]/max(tot[g],1):5.1f}%)")
T = sum(tot.values()); C = sum(cov.values()); CC = sum(covc.values())
print(f"  {'ALL':14s} .text={T:9d}  in C files={C:9d} ({100*C/T:5.1f}%)  complete={CC:9d} ({100*CC/T:5.1f}%)")
