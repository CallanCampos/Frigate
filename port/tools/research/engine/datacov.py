#!/usr/bin/env python3
"""Data/bss symbols not defined in C: per module, symbols in complete files (defined), in incomplete files
(defined in the C file or not), and outside any file."""
import os
import re
import sys
from collections import defaultdict

W, S = sys.argv[1], sys.argv[2]
CFG = os.path.join(W, "config/b2_us/arm9")
SEC_RE = re.compile(r"^\s+(\.\w+)\s+start:0x([0-9a-f]+)\s+end:0x([0-9a-f]+)")
SYM_RE = re.compile(r"^(\S+) kind:(data|bss)\S*.*? addr:0x([0-9a-f]+)")

mods = [("main", CFG)] + [(o, os.path.join(CFG, "overlays", o)) for o in sorted(os.listdir(os.path.join(CFG, "overlays")))]
cache = {}


def defined_in(cfile, name):
    p = os.path.join(W, cfile)
    if p not in cache:
        cache[p] = open(p, errors="replace").read() if os.path.exists(p) else ""
    for line in cache[p].splitlines():
        if re.search(r"\b" + re.escape(name) + r"\b", line) and not line.lstrip().startswith(("extern", "//")):
            if re.search(r"\b" + re.escape(name) + r"\s*(\[[^\]]*\])*\s*(=|;|\[)", line):
                return True
    return False


out = open(os.path.join(S, "datacov.tsv"), "w")
tot = defaultdict(int)
for mod, d in mods:
    files = []
    cur = None
    for line in open(os.path.join(d, "delinks.txt")):
        if line.strip() and not line.startswith(" "):
            cur = {"name": line.strip().rstrip(":"), "complete": False, "r": []}
            files.append(cur)
        elif line.strip() == "complete" and cur:
            cur["complete"] = True
        else:
            m = SEC_RE.match(line)
            if m and cur and m.group(1) in (".data", ".rodata", ".bss", ".sbss"):
                cur["r"].append((int(m.group(2), 16), int(m.group(3), 16)))
    c = defaultdict(int)
    for line in open(os.path.join(d, "symbols.txt")):
        m = SYM_RE.match(line)
        if not m:
            continue
        name, a = m.group(1), int(m.group(3), 16)
        owner = None
        for f in files:
            if any(s <= a < e for s, e in f["r"]):
                owner = f
                break
        if owner is None:
            c["nofile"] += 1
        elif owner["complete"]:
            c["complete"] += 1
        elif defined_in(owner["name"], name):
            c["inc_defined"] += 1
        else:
            c["inc_undefined"] += 1
    for k, v in c.items():
        tot[k] += v
    out.write(f"{mod}\t{c['complete']}\t{c['inc_defined']}\t{c['inc_undefined']}\t{c['nofile']}\n")
print(dict(tot))
