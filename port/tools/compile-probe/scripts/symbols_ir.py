#!/usr/bin/env python3
"""Whether every file of the decomp can be linked into one host program, as Frigate links the game: the external
symbols each file defines, and any that two files define, which would collide there.

    symbols_ir.py REPO OUTDIR -- CLANG_ARGS...

Compiles each .c of REPO's src/ and lib/ to LLVM IR on stdout for a host target (nothing written to the repository),
records which compile and the external symbols each defines, and writes OUTDIR/codegen_status.tsv and
OUTDIR/duplicate_symbols.tsv. run_all.sh runs it for arm64 macOS into ir_arm64/ (port/docs/research/compile-probe.md,
"Where the layout is assumed", cross-TU consistency).
"""
import os, re, sys, subprocess, pathlib, collections, concurrent.futures as cf
repo = pathlib.Path(sys.argv[1]).resolve(); out = pathlib.Path(sys.argv[2]); out.mkdir(parents=True, exist_ok=True)
args = sys.argv[sys.argv.index("--") + 1:]
clang = "/opt/homebrew/opt/llvm/bin/clang"
INC = ["-Iinclude"] + [f"-I{q.relative_to(repo)}" for q in sorted(repo.glob("lib/*/include"))] + ["-Ibuild/include/generated"]
files = sorted(str(p.relative_to(repo)) for d in ("src", "lib") for p in (repo / d).rglob("*.c"))
DEF = re.compile(r"^define (?!internal|private|linkonce|weak|available_externally)(?:[\w\"()]+ )*?(?:[^@]*)@\"?([\w.$]+)\"?\(")
GV = re.compile(r"^@\"?([\w.$]+)\"? = (?!internal|private|external|extern_weak|linkonce|weak|available_externally)(?:dso_local )?(?:local_unnamed_addr )?(?:unnamed_addr )?(common )?(?:global|constant)")
def run(f):
    p = subprocess.run([clang, "-S", "-emit-llvm", "-o", "-", "-O0", "-w", "-fno-color-diagnostics"] + args + INC + [f],
                       cwd=repo, capture_output=True, text=True, errors="replace")
    syms = []
    if p.returncode == 0:
        for l in p.stdout.splitlines():
            m = DEF.match(l)
            if m: syms.append(("func", m[1])); continue
            m = GV.match(l)
            if m: syms.append(("common" if m[1] else "data", m[1]))
    err = "" if p.returncode == 0 else next((l for l in p.stderr.splitlines() if "error" in l), "?")
    return f, p.returncode, syms, err
defs = collections.defaultdict(list); status = []
with cf.ThreadPoolExecutor(4) as ex:
    for f, rc, syms, err in ex.map(run, files):
        status.append((f, rc, err))
        for kind, s in syms: defs[s].append((f, kind))
with open(out / "codegen_status.tsv", "w") as o:
    for f, rc, err in status: o.write(f"{f}\t{'ok' if rc == 0 else 'fail'}\t{err}\n")
dups = {s: v for s, v in defs.items() if len(v) > 1}
with open(out / "duplicate_symbols.tsv", "w") as o:
    for s, v in sorted(dups.items()): o.write(f"{s}\t" + " ".join(f"{f}({k})" for f, k in v) + "\n")
ok = sum(1 for _, rc, _ in status if rc == 0)
print(f"codegen ok: {ok}/{len(status)}; external symbols: {len(defs)}; duplicated: {len(dups)} "
      f"(func {sum(1 for v in dups.values() if all(k=='func' for _,k in v))}, data/common {sum(1 for v in dups.values() if any(k!='func' for _,k in v))}); "
      f"common (tentative) symbols: {sum(1 for v in defs.values() for _,k in v if k=='common')}")
