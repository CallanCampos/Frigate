#!/usr/bin/env python3
"""Functions of the decomp that two files declare or define with different signatures, from LLVM IR: harmless on the
DS, where a pointer and an int are both 32 bits, and an ABI bug on a 64-bit host that links every file into one
program.

    signatures_ir.py REPO OUT.tsv -- CLANG_ARGS...

Compiles each .c of REPO's src/ and lib/ to IR on stdout (nothing written to the repository), collects each file's
declarations and definitions, and writes the functions whose signatures differ to OUT.tsv. run_all.sh runs it for
arm64 macOS, to signatures_arm64.tsv and .txt (port/docs/research/compile-probe.md, "Where the layout is assumed",
cross-TU consistency).
"""
import re, sys, subprocess, pathlib, collections, concurrent.futures as cf
repo = pathlib.Path(sys.argv[1]).resolve(); outp = sys.argv[2]
args = sys.argv[sys.argv.index("--") + 1:]
clang = "/opt/homebrew/opt/llvm/bin/clang"
INC = ["-Iinclude"] + [f"-I{q.relative_to(repo)}" for q in sorted(repo.glob("lib/*/include"))] + ["-Ibuild/include/generated"]
files = sorted(str(p.relative_to(repo)) for d in ("src", "lib") for p in (repo / d).rglob("*.c"))
LINE = re.compile(r"^(define|declare) (.*?)@\"?([\w.$]+)\"?\((.*)\)")
ATTR = re.compile(r"\b(noundef|zeroext|signext|nonnull|noalias|nocapture|readonly|writeonly|returned|inreg|byval\([^)]*\)|sret\([^)]*\)|align \d+|dereferenceable(_or_null)?\(\d+\)|captures\([^)]*\)|initializes\([^)]*\)|range\([^)]*\))\s*")
def norm_ret(s):
    s = re.sub(r"\b(dso_local|internal|private|hidden|protected|default|dllimport|dllexport|fastcc|ccc|arm_aapcscc|arm_aapcs_vfpcc|x86_stdcallcc|cc \d+|local_unnamed_addr|unnamed_addr)\b", "", s)
    s = ATTR.sub("", s); return s.split()[-1] if s.split() else "?"
def norm_params(p):
    depth = 0; parts = []; cur = ""
    for ch in p:
        if ch in "([{<": depth += 1
        if ch in ")]}>": depth -= 1
        if ch == "," and depth == 0: parts.append(cur); cur = ""; continue
        cur += ch
    if cur.strip(): parts.append(cur)
    out = []
    for x in parts:
        x = ATTR.sub("", x).strip(); x = re.sub(r"\s+%[\w.]+$", "", x).strip(); out.append(x)
    return tuple(out)
def run(f):
    p = subprocess.run([clang, "-S", "-emit-llvm", "-o", "-", "-O0", "-w"] + args + INC + [f], cwd=repo, capture_output=True, text=True, errors="replace")
    sig = []
    if p.returncode == 0:
        for l in p.stdout.splitlines():
            m = LINE.match(l)
            if m and " internal " not in f" {m[2]} " and " private " not in f" {m[2]} " and not m[3].startswith("llvm."):
                sig.append((m[3], m[1], norm_ret(m[2]), norm_params(m[4])))
    return f, sig
by = collections.defaultdict(lambda: collections.defaultdict(list))
with cf.ThreadPoolExecutor(4) as ex:
    for f, sig in ex.map(run, files):
        for name, kind, ret, params in sig: by[name][(ret, params)].append((f, kind))
mism = {n: v for n, v in by.items() if len(v) > 1}
def cls(v):
    sigs = list(v.keys()); flat = " | ".join(f"{r}({','.join(p)})" for r, p in sigs)
    if any("..." in p for _, ps in sigs for p in ps): return "variadic vs fixed"
    ptr_int = False
    rs = {r for r, _ in sigs}
    if "ptr" in rs and ({"i32", "i64"} & rs): ptr_int = True
    pl = [ps for _, ps in sigs]
    if len({len(ps) for ps in pl}) > 1: return "different arity"
    for i in range(len(pl[0])):
        col = {ps[i] for ps in pl}
        if "ptr" in col and ({"i32", "i64"} & col): ptr_int = True
    if ptr_int: return "ptr vs int (64-bit ABI bug)"
    return "other type difference (int widths/float/struct)"
with open(outp, "w") as o:
    c = collections.Counter()
    for n, v in sorted(mism.items()):
        k = cls(v); c[k] += 1
        o.write(f"{n}\t{k}\t" + " || ".join(f"{r}({','.join(p)}) <- {', '.join(f'{f}:{kd}' for f, kd in fl[:3])}" for (r, p), fl in v.items()) + "\n")
print(f"functions seen: {len(by)}; with differing signatures across TUs: {len(mism)}")
for k, n in c.most_common(): print(f"  {n:5d}  {k}")
