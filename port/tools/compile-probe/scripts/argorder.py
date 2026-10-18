#!/usr/bin/env python3
"""Calls in the game's C that pass two or more side-effecting calls as arguments (random draws, allocations, reads of a
stream), whose order C leaves unspecified: MWCC and clang may evaluate them in different orders, which would change
the game's random numbers or what it reads.

    argorder.py REPO

Scans REPO's src/**/*.c by pattern and prints the count and the first 15 sites. Not run by run_all.sh; it found none,
and the one related site, two draws added in one expression, is in port/docs/research/compile-probe.md ("Undefined
or implementation-defined behavior the DS tolerated", evaluation order).
"""
import re, sys, pathlib
repo = pathlib.Path(sys.argv[1])
SIDE = re.compile(r"\b(GFL_Random\w*|BattleRandom\w*|\w*Rand(om)?(_Next|Next|Get)?\w*|GFL_HeapAllocate|\w*_Read\w*|\w*Pop\w*|\w*Next\w*)\s*\(")
hits = []
for p in repo.glob("src/**/*.c"):
    text = p.read_text(errors="replace")
    text = re.sub(r"//.*", "", text)
    for m in re.finditer(r"\b\w+\s*\(", text):
        i = m.end(); depth = 1; j = i
        while j < len(text) and depth:
            depth += {"(": 1, ")": -1}.get(text[j], 0); j += 1
        args = text[i:j - 1]
        # split top-level args
        d = 0; parts = []; cur = ""
        for ch in args:
            if ch == "(": d += 1
            if ch == ")": d -= 1
            if ch == "," and d == 0: parts.append(cur); cur = ""; continue
            cur += ch
        parts.append(cur)
        n = sum(1 for a in parts if SIDE.search(a))
        if n >= 2:
            hits.append(f"{p.relative_to(repo)}:{text.count(chr(10), 0, m.start()) + 1}: {re.sub(r'\s+', ' ', text[m.start():j])[:140]}")
print(f"calls with >=2 side-effecting calls among their arguments: {len(hits)}")
for h in hits[:15]: print("  ", h)
