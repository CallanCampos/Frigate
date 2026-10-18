#!/usr/bin/env python3
"""The game's heap allocations and memory clears and copies whose size is a bare number rather than a sizeof: a block
sized for the DS's 32-bit pointers is too small on a 64-bit host when the struct it holds has pointers.

    alloc_sizes.py REPO

Scans REPO's src/**/*.c for the size argument of GFL_HeapAllocate, Field_AllocGimmickWorkBlock, GFL_ProcAllocWork and
GFL_ProcCreateWork (the game's heaps, src/gfl) and of the MI_Cpu clears and copies, memset and memcpy, and prints how
many sizes use sizeof, a literal, an expression with a literal or something else, with examples of the literals.
run_all.sh writes it to alloc_sizes.txt (port/docs/research/compile-probe.md, "Where the layout is assumed", the
literal sizes).
"""
import re, sys, pathlib, collections
repo = pathlib.Path(sys.argv[1])
FUNCS = {"GFL_HeapAllocate": 1, "Field_AllocGimmickWorkBlock": 1, "GFL_ProcAllocWork": 1, "GFL_ProcAllocWork": 1,
         "MI_CpuClear8": 1, "MI_CpuClearFast": 1, "MI_CpuFill8": 2, "MI_CpuCopy8": 2, "MI_CpuCopyFast": 2,
         "MI_CpuClear32": 1, "MI_CpuCopy32": 2, "memset": 2, "memcpy": 2, "GFL_ProcCreateWork": 1}
def args_of(text, start):
    depth = 0; cur = ""; out = []
    for ch in text[start:]:
        if ch == "(":
            depth += 1
            if depth == 1: continue
        if ch == ")":
            depth -= 1
            if depth == 0: out.append(cur.strip()); return out
        if ch == "," and depth == 1: out.append(cur.strip()); cur = ""; continue
        cur += ch
    return out
cnt = collections.Counter(); ex = collections.defaultdict(list)
for p in repo.glob("src/**/*.c"):
    t = p.read_text(errors="replace")
    for fn, idx in FUNCS.items():
        for m in re.finditer(r"\b" + fn + r"\s*\(", t):
            a = args_of(t, m.end() - 1)
            if len(a) <= idx: continue
            s = a[idx]
            if "sizeof" in s: k = "sizeof"
            elif re.fullmatch(r"\(?\s*(0x[0-9a-fA-F]+|\d+)\s*\)?", s): k = "literal"
            elif re.search(r"0x[0-9a-fA-F]+|\b\d{2,}\b", s): k = "expr with literal"
            else: k = "other (variable/macro)"
            cat = "alloc" if "Alloc" in fn or "Create" in fn else "clear/copy"
            cnt[(cat, k)] += 1
            ln = t.count("\n", 0, m.start()) + 1
            ex[(cat, k)].append(f"{p.relative_to(repo)}:{ln}: {fn}(... {s[:60]} ...)")
for k, n in sorted(cnt.items()): print(f"  {k[0]:10s} {k[1]:24s} {n}")
for k in [("alloc", "literal"), ("clear/copy", "literal")]:
    print("--", k)
    for e in ex[k][:8]: print("   ", e)
