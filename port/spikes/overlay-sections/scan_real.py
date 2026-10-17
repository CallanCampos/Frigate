#!/usr/bin/env python3
"""Compile every overlay's real C files with the per-overlay section pragma and report where their objects land.

Usage: scan_real.py REPO OUTDIR [--mode split|combined] [--opt -O2] [--clang PATH] [--jobs N]

For each overlay in config/b2_us/arm9/overlays, every source file listed in its delinks.txt is compiled for
arm64 macOS with the host flags of PLAN.md section 4.4, the compile probe's shim, and a forced include that puts
the file's writable statics in that overlay's sections. The objects go to OUTDIR (under build/, ignored by git);
the repository is only read. The report counts, per section kind, the symbols with static storage, and lists every
writable symbol that escaped the overlay's sections (a "leak"): those would not be reset on a reload.

This is an object-level scan, so it only means something without LTO: an LTO object is bitcode, with no sections,
and the LTO back end can still move a global at link time. The build's guard reads the link map of the linked image
instead (check_sections.py image), which covers both.
"""
import argparse
import concurrent.futures as cf
import os
import pathlib
import re
import subprocess
import sys

ap = argparse.ArgumentParser()
ap.add_argument("repo")
ap.add_argument("outdir")
ap.add_argument("--mode", choices=("split", "combined"), default="split")
ap.add_argument("--opt", default="-O2")
ap.add_argument("--clang", default="/opt/homebrew/opt/llvm/bin/clang")
ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
ap.add_argument("--version", default="b2_us")
a = ap.parse_args()

repo = pathlib.Path(a.repo).resolve()
out = pathlib.Path(a.outdir).resolve()
out.mkdir(parents=True, exist_ok=True)
objdump = "/opt/homebrew/opt/llvm/bin/llvm-objdump"  # reads objects from either clang
shim = repo / "port/tools/compile-probe/shim"

# Host flags from PLAN.md 4.4, plus the compile probe's shim so that the tree compiles before the platform header
# exists. Warnings are irrelevant here: only whether a file compiles and where its objects go.
FLAGS = ["--target=arm64-apple-macos14", "-ffreestanding", "-fshort-wchar", "-fdeclspec", "-std=gnu99",
         "-fsigned-char", "-fwrapv", "-fno-strict-aliasing", "-ffp-contract=off", "-ftrivial-auto-var-init=zero",
         "-fno-common", a.opt, "-w", "-Wno-error=int-conversion", "-Wno-error=incompatible-pointer-types",
         "-Wno-error=implicit-function-declaration", "-Wno-error=implicit-int", "-Wno-error=return-type",
         "-Wno-error=incompatible-function-pointer-types", "-D__thumb", "-DBLACK2" if a.version.startswith("b2") else "-DWHITE2",
         f"-I{shim}/include", "-include", f"{shim}/port_shim.h", "-Iinclude"]
FLAGS += [f"-I{p}" for p in sorted(str(q.relative_to(repo)) for q in repo.glob("lib/*/include"))]
FLAGS += ["-Ibuild/include/generated"]
if a.mode == "combined":
    FLAGS.append("-fno-zero-initialized-in-bss")


def pragma(ov):
    n = ov[2:]
    if a.mode == "combined":
        return f'#pragma clang section data="__DATA,__ov{n}"\n'
    return f'#pragma clang section data="__DATA,__ov{n}_d" bss="__DATA,__ov{n}_b,zerofill"\n'


# Overlay -> its source files, from the delinks (a file entry is an unindented "path.c:" line).
jobs = []
gen = out / "gen"
gen.mkdir(exist_ok=True)
ovdir = repo / "config" / a.version / "arm9" / "overlays"
for d in sorted(ovdir.iterdir()):
    files = [ln[:-1] for ln in (d / "delinks.txt").read_text().splitlines() if re.match(r"^[a-z].*\.c:$", ln)]
    if not files:
        continue
    hdr = gen / f"{d.name}_sections.h"
    hdr.write_text(pragma(d.name))
    for f in files:
        jobs.append((d.name, f, hdr))

SYM = re.compile(r"^[0-9a-f]+ (?P<flags>.{7}) (?P<sect>\S+)\s+(?P<name>\S+)$")


def run(job):
    ov, f, hdr = job
    obj = out / "obj" / ov / (f.replace("/", "_") + ".o")
    obj.parent.mkdir(parents=True, exist_ok=True)
    cmd = [a.clang, "-c", *FLAGS, "-include", str(hdr), f, "-o", str(obj)]
    p = subprocess.run(cmd, cwd=repo, capture_output=True, text=True, errors="replace")
    if p.returncode != 0:
        return ov, f, None
    syms = subprocess.run([objdump, "--syms", str(obj)], capture_output=True, text=True).stdout
    rows = []
    for ln in syms.splitlines():
        m = SYM.match(ln)
        if not m or " O " not in f" {m['flags']} ":
            continue
        name = m["name"]
        if name.startswith(("ltmp", "l_.str", "L")):
            continue
        rows.append((m["sect"], name))
    return ov, f, rows


counts = {}
leaks = []
failed = []
ov_sections = {}
with cf.ThreadPoolExecutor(a.jobs) as ex:
    for ov, f, rows in ex.map(run, jobs):
        if rows is None:
            failed.append(f)
            continue
        for sect, name in rows:
            if sect.startswith(f"__DATA,__ov{ov[2:]}"):
                kind = "overlay section"
                ov_sections.setdefault(ov, set()).add(sect)
            elif sect.startswith("__DATA,__ov"):
                kind = "wrong overlay section"
            elif sect in ("__DATA,__data", "__DATA,__bss", "__DATA,__common"):
                kind = "LEAK " + sect
                leaks.append((ov, f, sect, name))
            else:
                kind = sect
            counts[kind] = counts.get(kind, 0) + 1

nsect = sum(len(v) for v in ov_sections.values())
print(f"mode={a.mode} opt={a.opt} clang={a.clang}")
print(f"files: {len(jobs)} compiled: {len(jobs) - len(failed)} failed: {len(failed)}")
for k in sorted(counts):
    print(f"  {counts[k]:6d}  {k}")
print(f"overlays with an overlay section: {len(ov_sections)}; overlay sections in the image: {nsect}")
print(f"  with data: {sum(1 for v in ov_sections.values() if any(s.endswith('_d') or a.mode == 'combined' for s in v))}"
      f"  with bss: {sum(1 for v in ov_sections.values() if any(s.endswith('_b') for s in v))}")
print(f"leaks: {len(leaks)}")
for row in leaks:
    print("  " + "  ".join(row))
if failed:
    (out / "failed.txt").write_text("\n".join(failed) + "\n")
    print(f"failed files listed in {out / 'failed.txt'}")
sys.exit(0)
