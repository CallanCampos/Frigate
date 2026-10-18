#!/usr/bin/env python3
"""The struct types the game overlays on data it loads from its archives and files, from clang's AST, and whether their
layout changes on a 64-bit host, where a type with a pointer or a long no longer matches the bytes the ROM holds.

    ast_file_types.py REPO LAYOUT_DIFF.tsv OUT.tsv

Dumps the AST of each of REPO's src/**/*.c (i386, as MWCC lays out; nothing written to the repository), finds the type
of each cast of a load call's result and of each buffer passed to a read-into call (GFL's archive reads and loads,
FS_ReadFile, NitroSystem's unpacked-data getters), maps the types to records (typemap.py) and checks them against
LAYOUT_DIFF.tsv (layout_diff.py, MWCC against arm64). Writes a row per type to OUT.tsv and prints the counts and the
types that change. run_all.sh writes file_overlay_types.tsv and .txt (port/docs/research/compile-probe.md, "Where the
layout is assumed").
"""
import os, re, sys, subprocess, pathlib, collections, concurrent.futures as cf
repo = pathlib.Path(sys.argv[1]).resolve()
diff = {l.split("\t")[0]: l.rstrip("\n").split("\t") for l in open(sys.argv[2])}
clang = "/opt/homebrew/opt/llvm/bin/clang"
INC = ["-Iinclude"] + [f"-I{q.relative_to(repo)}" for q in sorted(repo.glob("lib/*/include"))] + ["-Ibuild/include/generated"]
LOADRE = re.compile(r"Function 0x[0-9a-f]+ '((?:GFL_Arc(?:Sys|Tool)\w*(?:Read|Load|HeapNew)\w*)|FS_ReadFile|NNS_G2dGetUnpacked\w*)'")
TY = re.compile(r"'([^']*\*)'(?::'([^']*\*)')?")
SKIP = re.compile(r"^(const )?(void|u8|u16|u32|s8|s16|s32|char|unsigned char|unsigned short|unsigned int|int|short|wchar_t|signed char)( const)? \*$")
def run(f):
    cmd = [clang, "-fsyntax-only", "-w", "--target=i386-unknown-linux-gnu", "-D__thumb", "-ffreestanding", "-std=c99", "-DBLACK2",
           "-Xclang", "-ast-dump", "-fno-color-diagnostics"] + INC + [f]
    p = subprocess.Popen(cmd, cwd=repo, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, errors="replace")
    win = collections.deque(maxlen=4); found = []; pending = 0; fn = None
    for line in p.stdout:
        if pending:
            pending -= 1
            if "<BitCast>" in line and "'void *'" in line:
                pending = 1; continue  # the operand is on the next line
            m = TY.search(line)
            if m and "'void *'" not in line and "Function" not in line:
                found.append((fn, "read-into", m[1], m[2] or m[1])); pending = 0
        m = LOADRE.search(line)
        if m:
            fn = m[1]
            for w in list(win)[::-1]:
                if ("<BitCast>" in w or "CStyleCastExpr" in w) and "'void *'" not in w.split("<")[0][-40:]:
                    t = TY.search(w)
                    if t: found.append((fn, "result-cast", t[1], t[2] or t[1])); break
            pending = 6
        win.append(line)
    p.wait()
    return f, found
files = sorted(str(p.relative_to(repo)) for p in repo.glob("src/**/*.c"))
rows = []
with cf.ThreadPoolExecutor(4) as ex:
    for f, found in ex.map(run, files):
        for fn, how, t, canon in found:
            if SKIP.match(t) or SKIP.match(canon): continue
            rows.append((f, fn, how, t, canon))
sys.path.insert(0, str(pathlib.Path(__file__).parent)); import typemap
TMAP = typemap.build(repo, diff.keys())
def key(canon):
    c = re.sub(r"^const ", "", canon).rstrip(" *").strip()
    return c if c in diff else TMAP.get(c, c)
out = open(sys.argv[3], "w"); per = {}
for f, fn, how, t, canon in rows:
    k = key(canon); r = diff.get(k)
    st = "?" if r is None else ("CHANGED" if (r[1] != r[2] or r[3] != r[4]) else "same")
    per.setdefault(k, [st, t, 0, f, r[1:3] if r else None]); per[k][2] += 1
    out.write(f"{f}\t{fn}\t{how}\t{t}\t{k}\t{st}\n")
c = collections.Counter(v[0] for v in per.values())
print(f"load sites with a struct type: {len(rows)}; distinct types: {len(per)}; by 64-bit layout: {dict(c)}")
for k, v in sorted(per.items(), key=lambda kv: kv[1][0]):
    if v[0] != "same": print(f"  {v[0]:8s} {v[1]:40s} uses={v[2]:3d} first={v[3]} sizes(mwcc,64)={v[4]}")
