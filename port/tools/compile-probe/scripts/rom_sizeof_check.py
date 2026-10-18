#!/usr/bin/env python3
"""MWCC's own struct sizes, read from the ROM, against clang's for several targets: the evidence that i386 SysV lays out
the decomp's structs as MWCC does, and so is the reference the 64-bit layouts are compared with.

    rom_sizeof_check.py REPO WORKDIR

For each function of REPO's src/ that only returns sizeof(T) (the save blocks' size functions of the save table, and
runtime structs such as GameSystem and G3DResource), reads the constant the ROM's code returns from Black 2's
extracted binaries (extract/b2_us), then compiles a file of _Static_asserts in WORKDIR/sizeof_tu to check sizeof(T)
for i386, the DS's AAPCS (thumbv5te), wasm32 and arm64. Writes WORKDIR/rom_sizeof_check.tsv; run_all.sh writes the
summary to rom_sizeof_check.txt (port/docs/research/compile-probe.md, "MWCC layout proxy").
"""
import re, sys, pathlib, struct, subprocess, glob, yaml
repo = pathlib.Path(sys.argv[1]).resolve(); work = pathlib.Path(sys.argv[2]); tmp = work / "sizeof_tu"; tmp.mkdir(exist_ok=True)
clang = "/opt/homebrew/opt/llvm/bin/clang"
syms = {}
for p in glob.glob(str(repo / "config/b2_us/**/symbols.txt"), recursive=True):
    mod = pathlib.Path(p).parent.name  # arm9 or ovNNN
    for l in open(p):
        m = re.match(r"(\S+) kind:function\((\w+)[^)]*\) addr:0x([0-9a-f]+)", l)
        if m: syms.setdefault(m[1], (mod, m[2], int(m[3], 16)))
def module_bytes(mod):
    if mod == "arm9":
        y = yaml.safe_load(open(repo / "extract/b2_us/arm9/arm9.yaml")); return y["base_address"], (repo / "extract/b2_us/arm9/arm9.bin").read_bytes()
    n = int(mod[2:]); d = repo / "extract/b2_us/arm9_overlays"
    ys = glob.glob(str(d / f"*{n:03d}*.yaml")) or glob.glob(str(d / "**" / f"*{n:03d}*.yaml"), recursive=True)
    bins = glob.glob(str(d / f"*{n:03d}*.bin")) or glob.glob(str(d / "**" / f"*{n:03d}*.bin"), recursive=True)
    if not bins: return None, None
    base = None
    if ys:
        y = yaml.safe_load(open(ys[0])); base = y.get("base_address") or y.get("address")
    return base, pathlib.Path(bins[0]).read_bytes()
def rom_value(mod, kind, addr):
    base, data = module_bytes(mod)
    if data is None or base is None: return None
    o = addr - base
    if kind == "thumb":
        h = [struct.unpack_from("<H", data, o + 2 * i)[0] for i in range(4)]
        if h[0] >> 8 == 0x48:  # ldr r0,[pc,#imm]
            lit = ((addr + 4) & ~3) + (h[0] & 0xff) * 4; return struct.unpack_from("<I", data, lit - base)[0]
        if h[0] >> 8 == 0x20:
            v = h[0] & 0xff
            if h[1] == 0x4770: return v
            if h[1] & 0xf83f == 0x0000 and h[2] == 0x4770: return v << ((h[1] >> 6) & 31)
    else:
        w = struct.unpack_from("<I", data, o)[0]
        if w & 0xffff0000 == 0xe59f0000: return struct.unpack_from("<I", data, o + 8 + (w & 0xfff))[0]
        if w & 0xfffff000 == 0xe3a00000:
            imm, rot = w & 0xff, (w >> 8) & 0xf; return ((imm >> (2 * rot)) | (imm << (32 - 2 * rot))) & 0xffffffff
    return None
sites = subprocess.run(["grep", "-rnE", r"^\s*return sizeof\((struct )?\w+\);", "src"], cwd=repo, capture_output=True, text=True).stdout.splitlines()
INC = ["-Iinclude"] + [f"-I{q}" for q in sorted(repo.glob("lib/*/include"))] + ["-Ibuild/include/generated"]
TARGETS = {"mwcc-proxy i386": ["--target=i386-unknown-linux-gnu"], "aapcs thumb": ["--target=thumbv5te-none-eabi"],
           "wasm32": ["--target=wasm32-unknown-unknown"], "arm64": ["--target=arm64-apple-macos14"]}
res = {t: [0, 0] for t in TARGETS}; n = 0; rows = []
for s in sites:
    f, ln, text = s.split(":", 2); ln = int(ln); T = re.search(r"sizeof\(((?:struct )?\w+)\)", text)[1]
    lines = (repo / f).read_text(errors="replace").splitlines()
    fn = None
    for i in range(ln - 1, -1, -1):
        m = re.match(r"^[A-Za-z_].*?\b(\w+)\s*\([^;]*\)\s*\{?\s*$", lines[i])
        if m: fn = m[1]; break
    if fn not in syms: continue
    v = rom_value(*syms[fn])
    if v is None: continue
    n += 1; row = [f"{f}:{ln}", fn, T, v]
    for t, a in TARGETS.items():
        tu = tmp / "probe.c"; tu.write_text(f'#include "{repo / f}"\n_Static_assert(sizeof({T}) == {v}u, "size");\n')
        p = subprocess.run([clang, "-fsyntax-only", "-w", "-D__thumb", "-ffreestanding", "-std=c11", "-DBLACK2"] + a + INC + [str(tu)], cwd=repo, capture_output=True, text=True)
        okk = p.returncode == 0; res[t][0 if okk else 1] += 1; row.append("ok" if okk else "DIFF")
    rows.append(row)
print(f"sizeof functions checked against the ROM: {n}")
for t, (a, b) in res.items(): print(f"  {t:18s} equal={a:3d} differ={b:3d}")
for r in rows:
    if "DIFF" in r[4:6]: print("  ", *r)
open(work / "rom_sizeof_check.tsv", "w").write("\n".join("\t".join(map(str, r)) for r in rows) + "\n")
