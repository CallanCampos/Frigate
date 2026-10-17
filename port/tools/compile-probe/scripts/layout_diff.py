#!/usr/bin/env python3
"""Compare record layouts between a 32-bit and a 64-bit run. Usage: layout_diff.py REPO L32.tsv L64.tsv OUT.tsv"""
import sys, re, collections, pathlib
repo = pathlib.Path(sys.argv[1])
def load(p):
    d = collections.defaultdict(set)
    for l in open(p):
        t, s, a, f = l.rstrip("\n").split("\t"); d[t].add((int(s), int(a)))
    return d
a32, a64 = load(sys.argv[2]), load(sys.argv[3])
# where named records are defined
defs = {}
rx = re.compile(r"^\s*(?:typedef\s+)?(struct|union)\s+(\w+)\s*\{")
for p in list(repo.glob("include/**/*.h")) + list(repo.glob("lib/*/include/**/*.h")) + list(repo.glob("lib/*/src/**/*.[ch]")) + list(repo.glob("src/**/*.[ch]")):
    try: lines = p.read_text(errors="replace").splitlines()
    except Exception: continue
    for i, l in enumerate(lines, 1):
        m = rx.match(l)
        if m: defs.setdefault(f"{m[1]} {m[2]}", f"{p.relative_to(repo)}:{i}")
def where(t):
    m = re.search(r"\((?:unnamed|anonymous) (?:struct |union )?at ([^)]+)\)", t)
    if m: return m[1]
    return defs.get(t, "?")
def area(loc):
    f = loc.split(":")[0]; p = f.split("/")
    if f == "?" : return "?"
    if p[0] == "lib": return "lib/" + p[1]
    if p[0] == "include": return "include/" + (p[1] if len(p) > 2 else "")
    if p[0] == "src": return "src/" + (p[1] if p[1] in ("gfl", "system") else "ovNNN")
    return p[0]
skip = ("__va_list", "__NSConstantString_tag", "__va_list_tag")
rows = []; amb = 0
for t in sorted(set(a32) & set(a64)):
    if any(s in t for s in skip): continue
    if len(a32[t]) != 1 or len(a64[t]) != 1: amb += 1; continue
    (s32, al32), = a32[t]; (s64, al64), = a64[t]
    loc = where(t)
    rows.append((t, s32 // 8, s64 // 8, al32 // 8, al64 // 8, loc, area(loc)))
with open(sys.argv[4], "w") as o:
    for r in rows: o.write("\t".join(map(str, r)) + "\n")
ch = [r for r in rows if r[1] != r[2] or r[3] != r[4]]
print(f"records compared: {len(rows)} (ambiguous names skipped: {amb}); layout changes at 64-bit: {len(ch)}")
c = collections.Counter(r[6] for r in rows); cc = collections.Counter(r[6] for r in ch)
for k in sorted(c, key=lambda k: -c[k]): print(f"  {k:24s} records={c[k]:5d} changed={cc[k]:5d}")
