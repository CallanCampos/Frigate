#!/usr/bin/env python3
"""Check '// size: 0x..' comments on struct definitions against clang's 32-bit layout (validates that clang's
AAPCS layout equals MWCC's for these types). Usage: check_size_comments.py REPO layouts32.tsv layout_diff.tsv"""
import sys, re, pathlib, subprocess
repo = pathlib.Path(sys.argv[1])
lay = {}
for l in open(sys.argv[2]):
    t, s, a, f = l.rstrip("\n").split("\t"); lay.setdefault(t, set()).add(int(s) // 8)
diff = {l.split("\t")[0]: l.rstrip("\n").split("\t") for l in open(sys.argv[3])}
out = subprocess.run(["grep", "-rnE", r"^\s*\}\s*\w*\s*;\s*//\s*size:?\s*0x[0-9a-fA-F]+", "include", "src", "lib"],
                     cwd=repo, capture_output=True, text=True).stdout.splitlines()
ok = bad = miss = 0; grow = 0
for o in out:
    f, ln, text = o.split(":", 2); ln = int(ln)
    want = int(re.search(r"0x([0-9a-fA-F]+)", text.split("//")[1])[1], 16)
    lines = (repo / f).read_text(errors="replace").splitlines()
    depth = 0; start = None
    for i in range(ln - 1, -1, -1):
        depth += lines[i].count("}") - lines[i].count("{")
        if depth == 0: start = i; break
    if start is None: miss += 1; continue
    s = lines[start]; m = re.search(r"(struct|union)\s+(\w+)\s*\{", s)
    keys = []
    if m: keys.append(f"{m[1]} {m[2]}")
    m2 = re.search(r"(struct|union)\s*\{", s)
    if m2: keys.append(f"{m2[1]} (unnamed at {f}:{start+1}:{m2.start()+1 if not s.lstrip().startswith('typedef') else s.index(m2[1])+1})")
    hit = None
    for k in keys:
        for t in lay:
            if t == k or (k.startswith(m2[1] + " (unnamed at " + f + ":" + str(start + 1) + ":") if m2 else False) and t.startswith(k.rsplit(":", 1)[0] + ":"):
                hit = t; break
        if hit: break
    if not hit: miss += 1; continue
    sizes = lay[hit]
    if want in sizes: ok += 1
    else: bad += 1; print(f"MISMATCH {f}:{ln} {hit} comment=0x{want:x} clang32={[hex(x) for x in sizes]}")
    d = diff.get(hit)
    if d and d[1] != d[2]: grow += 1
print(f"size comments: {len(out)}; matched clang 32-bit: {ok}; mismatched: {bad}; not found in layouts: {miss}; of the matched, grow at 64-bit: {grow}")
