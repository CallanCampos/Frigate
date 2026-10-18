#!/usr/bin/env python3
"""Per-function C coverage of pokebw2 b2_us, from delinks.txt and symbols.txt.

Writes:
  funcs.tsv    module name addr size mode file status   (status: complete / c_written / c_missing / no_file)
  files.tsv    module file complete nfuncs bytes n_complete_bytes n_written n_written_bytes n_missing n_missing_bytes
  modules.tsv  module nfuncs bytes complete_funcs complete_bytes written_funcs written_bytes missing_funcs missing_bytes nofile_funcs nofile_bytes data_size bss_size

    coverage.py REPO WORK

Reads Black 2's REPO/config/b2_us/**/delinks.txt and symbols.txt and looks for each function's definition in its
file's C. The other engine scripts read its tables from WORK (overlays.py, callers.py, milestones.py). Research of
2026-10-16 (port/docs/research/engine-boot.md, "Coverage in one paragraph" and the overlays' coverage).
"""
import os
import re
import sys

W = sys.argv[1]
OUT = sys.argv[2]
CFG = os.path.join(W, "config/b2_us/arm9")

SEC_RE = re.compile(r"^\s+(\.\w+)\s+start:0x([0-9a-f]+)\s+end:0x([0-9a-f]+)")
SYM_RE = re.compile(r"^(\S+) kind:function\((\w+)(?:,size=0x([0-9a-f]+))?.*?\) addr:0x([0-9a-f]+)")


def parse_delinks(path):
    header = {}
    files = []
    cur = None
    for line in open(path):
        line = line.rstrip("\n")
        if not line.strip():
            continue
        if not line.startswith(" "):
            cur = {"name": line.rstrip(":"), "complete": False, "secs": {}}
            files.append(cur)
            continue
        if line.strip() == "complete":
            cur["complete"] = True
            continue
        m = SEC_RE.match(line)
        if m:
            sec, s, e = m.group(1), int(m.group(2), 16), int(m.group(3), 16)
            if cur is None:
                header[sec] = (s, e)
            else:
                cur["secs"].setdefault(sec, []).append((s, e))
    return header, files


def parse_syms(path):
    out = []
    for line in open(path):
        m = SYM_RE.match(line)
        if m:
            size = int(m.group(3), 16) if m.group(3) else 0
            out.append((m.group(1), int(m.group(4), 16), size, m.group(2)))
    return out


DEF_CACHE = {}


def has_definition(cfile, name):
    path = os.path.join(W, cfile)
    if path not in DEF_CACHE:
        try:
            DEF_CACHE[path] = open(path, encoding="utf-8", errors="replace").read()
        except OSError:
            DEF_CACHE[path] = ""
    text = DEF_CACHE[path]
    for m in re.finditer(r"\b" + re.escape(name) + r"\s*\(", text):
        i = m.end()
        depth = 1
        while i < len(text) and depth:
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
            i += 1
        j = i
        while j < len(text) and text[j] in " \t\r\n":
            j += 1
        if j < len(text) and text[j] == "{":
            return True
    # asm functions: "asm void name(" also caught above
    return False


modules = []
for name in ["main", "itcm", "dtcm", "autoload_2", "autoload_3", "ltd_autoload_0"]:
    d = CFG if name == "main" else os.path.join(CFG, name)
    modules.append((name, d))
for ov in sorted(os.listdir(os.path.join(CFG, "overlays"))):
    modules.append((ov, os.path.join(CFG, "overlays", ov)))

fo = open(os.path.join(OUT, "funcs.tsv"), "w")
ff = open(os.path.join(OUT, "files.tsv"), "w")
fm = open(os.path.join(OUT, "modules.tsv"), "w")
fm.write("module\tnfuncs\tbytes\tcomplete_funcs\tcomplete_bytes\twritten_funcs\twritten_bytes\tmissing_funcs\tmissing_bytes\tnofile_funcs\tnofile_bytes\ttext\tdata\tbss\n")
ff.write("module\tfile\tcomplete\tnfuncs\tbytes\twritten\twritten_bytes\tmissing\tmissing_bytes\n")
for mod, d in modules:
    dl = os.path.join(d, "delinks.txt")
    sy = os.path.join(d, "symbols.txt")
    if not os.path.exists(dl):
        continue
    header, files = parse_delinks(dl)
    syms = parse_syms(sy)
    ranges = []
    for f in files:
        for s, e in f["secs"].get(".text", []):
            ranges.append((s, e, f))
        for s, e in f["secs"].get(".itcm", []):
            ranges.append((s, e, f))
    ranges.sort(key=lambda r: r[0])
    stats = {"complete": [0, 0], "c_written": [0, 0], "c_missing": [0, 0], "no_file": [0, 0]}
    fstats = {}
    for name, addr, size, mode in syms:
        owner = None
        for s, e, f in ranges:
            if s <= addr < e:
                owner = f
                break
        if owner is None:
            st = "no_file"
            fname = "-"
        else:
            fname = owner["name"]
            if owner["complete"]:
                st = "complete"
            elif fname.endswith(".c") and has_definition(fname, name):
                st = "c_written"
            else:
                st = "c_missing"
        stats[st][0] += 1
        stats[st][1] += size
        if owner is not None:
            fs = fstats.setdefault(fname, [owner["complete"], 0, 0, 0, 0, 0, 0])
            fs[1] += 1
            fs[2] += size
            if st == "c_written":
                fs[3] += 1
                fs[4] += size
            elif st == "c_missing":
                fs[5] += 1
                fs[6] += size
        fo.write(f"{mod}\t{name}\t{addr:08x}\t{size}\t{mode}\t{fname}\t{st}\n")
    for fname, fs in fstats.items():
        ff.write(f"{mod}\t{fname}\t{int(fs[0])}\t{fs[1]}\t{fs[2]}\t{fs[3]}\t{fs[4]}\t{fs[5]}\t{fs[6]}\n")
    # files with no functions (data only)
    for f in files:
        if f["name"] not in fstats:
            ff.write(f"{mod}\t{f['name']}\t{int(f['complete'])}\t0\t0\t0\t0\t0\t0\n")

    def secsize(sec):
        if sec in header:
            s, e = header[sec]
            return e - s
        return 0

    n = sum(v[0] for v in stats.values())
    b = sum(v[1] for v in stats.values())
    fm.write(
        f"{mod}\t{n}\t{b}\t{stats['complete'][0]}\t{stats['complete'][1]}\t{stats['c_written'][0]}\t{stats['c_written'][1]}\t"
        f"{stats['c_missing'][0]}\t{stats['c_missing'][1]}\t{stats['no_file'][0]}\t{stats['no_file'][1]}\t"
        f"{secsize('.text')}\t{secsize('.data')}\t{secsize('.bss')}\n"
    )
