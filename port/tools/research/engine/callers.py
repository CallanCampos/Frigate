#!/usr/bin/env python3
"""For every no-C function, find which C source files (anywhere in src/) reference it by name.

Output: missing_refs.tsv  module  missing_file  n_missing  bytes  referencing_c_files(count)
and per C file, which missing files it depends on: cfile_deps.tsv
"""
import os
import re
import sys
from collections import defaultdict

W, S = sys.argv[1], sys.argv[2]
missing = {}  # name -> (module, file, size)
for line in open(os.path.join(S, "funcs.tsv")):
    mod, name, addr, size, mode, f, st = line.rstrip("\n").split("\t")
    if st in ("c_missing", "no_file"):
        missing[name] = (mod, f, int(size))

ident = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
file_refs = defaultdict(set)  # missing file -> set of C files
cdeps = defaultdict(lambda: defaultdict(set))  # cfile -> missing file -> names
for root in ("src", "lib"):
    for dp, dn, fn in os.walk(os.path.join(W, root)):
        for f in fn:
            if not f.endswith((".c", ".h")):
                continue
            p = os.path.join(dp, f)
            rel = os.path.relpath(p, W)
            text = open(p, encoding="utf-8", errors="replace").read()
            for tok in set(ident.findall(text)):
                if tok in missing:
                    mod, mf, size = missing[tok]
                    if mf == rel:
                        continue
                    file_refs[(mod, mf)].add(rel)
                    cdeps[rel][(mod, mf)].add(tok)

agg = defaultdict(lambda: [0, 0])
for name, (mod, f, size) in missing.items():
    agg[(mod, f)][0] += 1
    agg[(mod, f)][1] += size
with open(os.path.join(S, "missing_refs.tsv"), "w") as o:
    for k in sorted(agg, key=lambda k: -agg[k][1]):
        refs = sorted(file_refs.get(k, []))
        o.write(f"{k[0]}\t{k[1]}\t{agg[k][0]}\t{agg[k][1]}\t{len(refs)}\t{','.join(refs[:40])}\n")
with open(os.path.join(S, "cfile_deps.tsv"), "w") as o:
    for c in sorted(cdeps):
        for k, names in sorted(cdeps[c].items()):
            o.write(f"{c}\t{k[0]}\t{k[1]}\t{len(names)}\t{','.join(sorted(names)[:8])}\n")
