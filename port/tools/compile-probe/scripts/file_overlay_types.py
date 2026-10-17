#!/usr/bin/env python3
"""Find struct types that game code overlays on data read from archives/files, and check whether their layout
changes on a 64-bit host. Usage: file_overlay_types.py REPO layout_mwcc_vs_arm64.tsv"""
import re, sys, pathlib, collections
repo = pathlib.Path(sys.argv[1])
diff = {}
for l in open(sys.argv[2]):
    r = l.rstrip("\n").split("\t"); diff[r[0]] = r
by_loc = {}
for k in diff:
    m = re.search(r"\(unnamed at ([^:]+):(\d+):\d+\)$", k)
    if m and "::" not in k: by_loc[(m[1], int(m[2]))] = k
# typedef name -> record key
tmap = {}
for p in list(repo.glob("include/**/*.h")) + list(repo.glob("lib/*/include/**/*.h")) + list(repo.glob("src/**/*.[ch]")) + list(repo.glob("lib/*/src/**/*.[ch]")):
    f = str(p.relative_to(repo)); L = p.read_text(errors="replace").splitlines()
    for i, l in enumerate(L):
        m = re.match(r"^\s*typedef\s+(struct|union)\s+(\w+)\s+(\w+)\s*;", l)
        if m: tmap.setdefault(m[3], f"{m[1]} {m[2]}"); continue
        m = re.match(r"^\}\s*(\w+)\s*;", l)
        if m:
            depth = 0
            for j in range(i, -1, -1):
                depth += L[j].count("}") - L[j].count("{")
                if depth == 0:
                    mm = re.search(r"typedef\s+(struct|union)\s*(\w+)?\s*\{", L[j])
                    if mm:
                        tmap.setdefault(m[1], f"{mm[1]} {mm[2]}" if mm[2] else by_loc.get((f, j + 1), "?"))
                    break
        m = re.match(r"^\s*(struct|union)\s+(\w+)\s*\{", l)
        if m: tmap.setdefault(m[2], f"{m[1]} {m[2]}")
LOAD = r"GFL_Arc\w*(?:Read|Load)\w*|GFL_ArcSys\w*HeapNew\w*|FS_ReadFile|NNS_G\w+Get\w*Data"
pat = re.compile(r"\((?:const\s+)?(\w+)\s*\*\)\s*(?:" + LOAD + r")\s*\(|\b(?:const\s+)?(\w+)\s*\*\s*\w+\s*=\s*(?:" + LOAD + r")\s*\(")
pat_rd = re.compile(r"\b(?:GFL_Arc\w*Read|GFL_ArcSysRead|GFL_ArcToolRead)\w*\s*\(\s*&(\w+(?:->\w+|\.\w+)*)")
hits = collections.defaultdict(list)
for p in repo.glob("src/**/*.c"):
    t = p.read_text(errors="replace").splitlines()
    for i, l in enumerate(t, 1):
        for m in pat.finditer(l):
            T = m[1] or m[2]
            if T in ("void", "u8", "u16", "u32", "s8", "s16", "s32", "char", "wchar_t"): continue
            hits[T].append(f"{p.relative_to(repo)}:{i}")
types = sorted(hits)
changed = []; same = []; unknown = []
for T in types:
    k = tmap.get(T); r = diff.get(k) if k else None
    if r is None: unknown.append(T)
    elif r[1] != r[2] or r[3] != r[4]: changed.append((T, r[1], r[2], hits[T][0], len(hits[T])))
    else: same.append(T)
print(f"struct types overlaid on archive/file data (cast/decl at a load call): {len(types)}; layout unchanged at 64-bit: {len(same)}; CHANGED: {len(changed)}; unresolved: {len(unknown)}")
for c in changed: print("  CHANGED", c)
print("  unresolved:", " ".join(unknown[:40]))
