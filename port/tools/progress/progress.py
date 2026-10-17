#!/usr/bin/env python3
"""Port-readiness progress of the pokebw2 decompilation: which code and data exist as C.

Usage:
    python3 progress.py --root <repo> [--version b2_us] [--rev <git rev>] --out <dir>

With --rev, the configs and sources are read from that git revision (git show / git ls-tree) instead of the working tree,
so the same measurement can be taken on historic commits.

Tiers, per function (from symbols.txt) and per data byte (from the delinks section ranges):
    a   in a source file marked `complete` (matching C, linked from C)
    b   defined in the C of an incomplete file (C present, not linked; may or may not match)
    c   no C: c_gap (in no delinks entry), c_nosrc (entry whose source file does not exist), c_partial (an incomplete
        file's range, but the function is not defined in its C)
Writes <out>/functions.tsv, <out>/files.tsv, <out>/summary.json.
"""
import argparse
import bisect
import json
import os
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cparse  # noqa: E402
from rombytes import RomBytes, is_veneer  # noqa: E402

CODE_SECTIONS = {".text", ".ltdmain", ".init"}
DATA_SECTIONS = {".rodata", ".data", ".bss", ".ctor", ".init", ".sbss", ".sdata"}

OVERLAY_LIBS = {}


class Source:
    """Reads files from the working tree or from a git revision."""

    def __init__(self, root, rev=None):
        self.root = Path(root)
        self.rev = rev
        self._tree = None

    def tree(self):
        if self._tree is None:
            if self.rev:
                out = subprocess.run(["git", "-C", str(self.root), "ls-tree", "-r", "--name-only", self.rev],
                                     capture_output=True, text=True, check=True).stdout
                self._tree = set(out.splitlines())
            else:
                self._tree = None
        return self._tree

    def exists(self, rel):
        if self.rev:
            return rel in self.tree()
        return (self.root / rel).exists()

    def _batch(self):
        if getattr(self, "_proc", None) is None:
            self._proc = subprocess.Popen(["git", "-C", str(self.root), "cat-file", "--batch"], stdin=subprocess.PIPE,
                                          stdout=subprocess.PIPE)
        return self._proc

    def read(self, rel):
        if self.rev:
            if rel not in self.tree():
                return None
            p = self._batch()
            p.stdin.write(f"{self.rev}:{rel}\n".encode())
            p.stdin.flush()
            header = p.stdout.readline().decode()
            if header.endswith("missing\n"):
                return None
            size = int(header.split()[2])
            data = p.stdout.read(size)
            p.stdout.read(1)
            return data.decode("utf-8", "replace")
        p = self.root / rel
        if not p.exists():
            return None
        return p.read_text(errors="replace")

    def glob_configs(self, version):
        base = f"config/{version}/arm9"
        if self.rev:
            return sorted(p for p in self.tree() if p.startswith(base + "/") and p.endswith("/delinks.txt"))
        out = [str(p.relative_to(self.root)) for p in (self.root / base).rglob("delinks.txt")]
        return sorted(out)


def module_name(delinks_rel, version):
    parts = Path(delinks_rel).parts
    i = parts.index("arm9")
    rest = parts[i + 1:-1]
    if not rest:
        return "main"
    if rest[0] == "overlays":
        return rest[1]
    return rest[0]


def parse_delinks(text):
    sections = {}
    files = []
    cur = None
    for line in text.splitlines():
        if not line.strip():
            continue
        if not line[0].isspace() and line.rstrip().endswith(":"):
            cur = {"path": line.rstrip()[:-1], "complete": False, "sections": {}}
            files.append(cur)
            continue
        t = line.split()
        if t[0] == "complete" and cur is not None:
            cur["complete"] = True
            continue
        if t[0].startswith("."):
            kv = dict(x.split(":", 1) for x in t[1:] if ":" in x)
            start, end = int(kv["start"], 16), int(kv["end"], 16)
            if cur is None:
                sections[t[0]] = {"start": start, "end": end, "kind": kv.get("kind")}
            else:
                cur["sections"][t[0]] = (start, end)
    return sections, files


SYM_RE = re.compile(r"^(\S+) kind:(\w+)(\(([^)]*\)?)\))? addr:0x([0-9a-f]+)(.*)$")


def parse_symbols(text):
    funcs, datas = [], []
    for line in text.splitlines():
        t = line.split()
        if len(t) < 3:
            continue
        name = t[0]
        kind = t[1][5:]
        addr = int(t[2][5:], 16) if t[2].startswith("addr:") else None
        if addr is None:
            continue
        if kind.startswith("function"):
            m = re.search(r"size=0x([0-9a-f]+)", kind)
            size = int(m.group(1), 16) if m else 0
            mode = "arm" if "(arm" in kind else "thumb"
            funcs.append({"name": name, "addr": addr, "size": size, "mode": mode})
        elif kind.startswith("data") or kind.startswith("bss"):
            datas.append({"name": name, "addr": addr, "kind": kind.split("(")[0]})
    return funcs, datas


def parse_relocs(text):
    out = []
    for line in text.splitlines():
        t = line.split()
        if not t or not t[0].startswith("from:"):
            continue
        frm = int(t[0][5:], 16)
        kind = t[1][5:]
        out.append((frm, kind))
    return out


def category_of(path):
    if path is None:
        return None
    p = Path(path).parts
    if p[0] == "lib":
        return "lib/" + p[1]
    return "game"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True)
    ap.add_argument("--version", default="b2_us")
    ap.add_argument("--rev")
    ap.add_argument("--out", required=True)
    ap.add_argument("--nonmatching", help="docs/nonmatching-functions.md path relative to root (optional)",
                    default="docs/nonmatching-functions.md")
    args = ap.parse_args()
    src = Source(args.root, args.rev)
    rb = RomBytes(args.root, args.version)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    nonmatch_text = src.read(args.nonmatching) or ""
    nonmatch_names = set(re.findall(r"^\| `[^`]*` \| `([^`]+)`", nonmatch_text, re.M))

    cache = {}

    def c_info(path):
        if path not in cache:
            text = src.read(path)
            if text is None:
                cache[path] = None
            else:
                funcs, idents = cparse.scan(text)
                cache[path] = (funcs, idents)
        return cache[path]

    func_rows = []
    file_rows = []
    data_rows = []  # per module per section per tier
    reloc_rows = []
    module_meta = {}

    for drel in src.glob_configs(args.version):
        mod = module_name(drel, args.version)
        mdir = str(Path(drel).parent)
        sections, files = parse_delinks(src.read(drel))
        funcs, datas = parse_symbols(src.read(f"{mdir}/symbols.txt") or "")
        relocs = parse_relocs(src.read(f"{mdir}/relocs.txt") or "")
        module_meta[mod] = {s: v["end"] - v["start"] for s, v in sections.items()}

        # owner lookup per section: sorted list of (start, end, file)
        owners = defaultdict(list)
        for f in files:
            for sec, (a, b) in f["sections"].items():
                owners[sec].append((a, b, f))
        for sec in owners:
            owners[sec].sort(key=lambda x: x[0])

        def owner(sec, addr):
            lst = owners.get(sec, [])
            k = bisect.bisect_right([x[0] for x in lst], addr) - 1
            if k >= 0 and lst[k][0] <= addr < lst[k][1]:
                return lst[k][2]
            return None

        def section_of(addr):
            for s, v in sections.items():
                if v["start"] <= addr < v["end"]:
                    return s
            return None

        # neighbour categories for gap attribution, per code section
        def neighbour_cat(sec, addr):
            lst = owners.get(sec, [])
            starts = [x[0] for x in lst]
            k = bisect.bisect_right(starts, addr) - 1
            prev_c = category_of(lst[k][2]["path"]) if k >= 0 else None
            nxt_c = category_of(lst[k + 1][2]["path"]) if k + 1 < len(lst) else None
            if prev_c == nxt_c and prev_c is not None:
                return prev_c
            if mod.startswith("ov"):
                # overlay gaps default to game unless both neighbours are the same library
                if prev_c and prev_c.startswith("lib") and (nxt_c is None or nxt_c == prev_c):
                    return prev_c
                if nxt_c and nxt_c.startswith("lib") and prev_c is None:
                    return nxt_c
                return "game"
            if prev_c is None and nxt_c is None:
                return "unknown"
            return "unknown:" + "|".join(x or "-" for x in (prev_c, nxt_c))

        file_fn = defaultdict(lambda: {"n": 0, "bytes": 0, "inC": 0, "inC_bytes": 0, "asm": 0})
        # pass 1: owner and veneer flag of each function
        placed = []
        for fn in funcs:
            sec = section_of(fn["addr"]) or "?"
            f = owner(sec, fn["addr"])
            info0 = c_info(f["path"]) if f is not None else None
            in_c = bool(info0) and fn["name"] in info0[0]
            ven = (not in_c) and is_veneer(rb, mod, fn["addr"], fn["size"], fn["mode"])
            placed.append((fn, sec, f, ven))
        # pass 2: for incomplete files with C, align C definitions (source order) with symbols (address order), since
        # statics often have a C name while the config keeps func_ovNNN_xxxxxxxx
        present = {}  # (path, name) -> 'name' | 'order' | 'missing' | 'uncertain'
        by_file = defaultdict(list)
        for fn, sec, f, ven in placed:
            if f is not None and not ven and not f["complete"]:
                by_file[f["path"]].append(fn)
        for path, fl in by_file.items():
            info = c_info(path)
            if info is None:
                continue
            fl.sort(key=lambda x: x["addr"])
            cdefs = list(info[0].keys())
            symnames = [x["name"] for x in fl]
            symset = set(symnames)
            anchors = [n for n in symnames if n in info[0]]
            cpos = {n: i for i, n in enumerate(cdefs)}
            # segments
            seg_s, seg_c = [], []
            last_ci = -1
            ai = 0
            def flush(seg_s, seg_c):
                cnt = [c for c in seg_c if not info[0][c].get("inline")]
                k = len(cnt)
                for j, nm in enumerate(seg_s):
                    if j < k:
                        present[(path, nm)] = "order" if k >= len(seg_s) else "uncertain"
                    else:
                        present[(path, nm)] = "missing"
            for nm in symnames:
                if nm in info[0]:
                    ci = cpos[nm]
                    seg_c = [c for c in cdefs[last_ci + 1:ci] if c not in symset]
                    flush(seg_s, seg_c)
                    present[(path, nm)] = "name"
                    seg_s = []
                    last_ci = max(last_ci, ci)
                else:
                    seg_s.append(nm)
            seg_c = [c for c in cdefs[last_ci + 1:] if c not in symset]
            flush(seg_s, seg_c)

        for fn, sec, f, ven in placed:
            asm = False
            nm = fn["name"] in nonmatch_names
            if ven:
                func_rows.append({
                    "module": mod, "name": fn["name"], "addr": fn["addr"], "size": fn["size"], "mode": fn["mode"],
                    "section": sec, "file": f["path"] if f else "", "tier": "v", "sub": "veneer",
                    "cat": category_of(f["path"]) if f else neighbour_cat(sec, fn["addr"]), "asm": False,
                    "nonmatch_doc": False, "found_in_complete_c": False,
                })
                continue
            if f is None:
                tier, sub = "c", "c_gap"
                cat = neighbour_cat(sec, fn["addr"])
                path = ""
            else:
                path = f["path"]
                cat = category_of(path)
                info = c_info(path)
                if f["complete"]:
                    tier, sub = "a", "a"
                    if info and fn["name"] in info[0]:
                        asm = info[0][fn["name"]]["asm"]
                elif info is None:
                    tier, sub = "c", "c_nosrc"
                else:
                    how = present.get((path, fn["name"]), "missing")
                    if how in ("name", "order"):
                        tier, sub = "b", "b" if how == "name" else "b_order"
                        if how == "name":
                            asm = info[0][fn["name"]]["asm"]
                    elif how == "uncertain":
                        tier, sub = "b", "b_uncertain"
                    else:
                        tier, sub = "c", "c_partial"
                st = file_fn[path]
                st["n"] += 1
                st["bytes"] += fn["size"]
                if tier in "ab":
                    st["inC"] += 1
                    st["inC_bytes"] += fn["size"]
                if asm:
                    st["asm"] += 1
            func_rows.append({
                "module": mod, "name": fn["name"], "addr": fn["addr"], "size": fn["size"], "mode": fn["mode"],
                "section": sec, "file": path, "tier": tier, "sub": sub, "cat": cat, "asm": asm, "nonmatch_doc": nm,
                "found_in_complete_c": (tier == "a" and bool(c_info(path)) and fn["name"] in c_info(path)[0]),
            })

        # data: symbols sized by next symbol within the same section of the module
        datas.sort(key=lambda d: d["addr"])
        by_sec = defaultdict(list)
        for d in datas:
            s = section_of(d["addr"])
            if s:
                by_sec[s].append(d)
        reloc_sources = sorted(r[0] for r in relocs if section_of(r[0]) in DATA_SECTIONS)
        for sec, v in sections.items():
            if sec in CODE_SECTIONS and sec != ".init":
                continue
            total = v["end"] - v["start"]
            # bytes per owning file
            covered = defaultdict(int)
            for (a, b, f) in owners.get(sec, []):
                covered[id(f)] = b - a
            syms = by_sec.get(sec, [])
            for k, d in enumerate(syms):
                end = syms[k + 1]["addr"] if k + 1 < len(syms) else v["end"]
                d["size"] = end - d["addr"]
            # walk the section in byte runs: owned by a file or gap
            for (a, b, f) in owners.get(sec, []):
                info = c_info(f["path"])
                if f["complete"]:
                    tier = "a"
                elif info is None:
                    tier = "c_nosrc"
                else:
                    tier = "b_file"
                # pointer words in this range
                lo = bisect.bisect_left(reloc_sources, a)
                hi = bisect.bisect_left(reloc_sources, b)
                nptr = hi - lo
                # symbols in range named in C (for incomplete files)
                named = unnamed = 0
                named_ptr = unnamed_ptr = 0
                if tier == "b_file":
                    for d in syms:
                        if a <= d["addr"] < b:
                            pl = bisect.bisect_left(reloc_sources, d["addr"])
                            ph = bisect.bisect_left(reloc_sources, d["addr"] + d["size"])
                            if d["name"] in info[1]:
                                named += min(d["size"], b - d["addr"])
                                named_ptr += ph - pl
                            else:
                                unnamed += min(d["size"], b - d["addr"])
                                unnamed_ptr += ph - pl
                data_rows.append({"module": mod, "section": sec, "file": f["path"], "cat": category_of(f["path"]),
                                  "tier": tier, "bytes": b - a, "ptrs": nptr, "named_bytes": named,
                                  "unnamed_bytes": unnamed, "named_ptrs": named_ptr, "unnamed_ptrs": unnamed_ptr})
            # gaps
            pos = v["start"]
            for (a, b, f) in owners.get(sec, []) + [(v["end"], v["end"], None)]:
                if a > pos:
                    lo = bisect.bisect_left(reloc_sources, pos)
                    hi = bisect.bisect_left(reloc_sources, a)
                    # count pointer-bearing objects in the gap
                    pobj = pbytes = 0
                    for d in syms:
                        if pos <= d["addr"] < a:
                            pl = bisect.bisect_left(reloc_sources, d["addr"])
                            ph = bisect.bisect_left(reloc_sources, d["addr"] + d["size"])
                            if ph > pl:
                                pobj += 1
                                pbytes += d["size"]
                    data_rows.append({"module": mod, "section": sec, "file": "", "cat": None, "tier": "c_gap",
                                      "bytes": a - pos, "ptrs": hi - lo, "ptr_objects": pobj, "ptr_obj_bytes": pbytes})
                pos = max(pos, b)

        for f in files:
            info = c_info(f["path"])
            st = file_fn.get(f["path"], {"n": 0, "bytes": 0, "inC": 0, "inC_bytes": 0, "asm": 0})
            file_rows.append({
                "module": mod, "path": f["path"], "complete": f["complete"], "has_src": info is not None,
                "cat": category_of(f["path"]), "funcs": st["n"], "func_bytes": st["bytes"], "funcs_in_c": st["inC"],
                "func_bytes_in_c": st["inC_bytes"], "asm_funcs": st["asm"],
                "c_defs": len(info[0]) if info else 0,
                "text": sum(b - a for s, (a, b) in f["sections"].items() if s in CODE_SECTIONS),
                "data": sum(b - a for s, (a, b) in f["sections"].items() if s not in CODE_SECTIONS),
            })

    with open(out / "functions.tsv", "w") as fh:
        cols = ["module", "name", "addr", "size", "mode", "section", "file", "tier", "sub", "cat", "asm",
                "nonmatch_doc", "found_in_complete_c"]
        fh.write("\t".join(cols) + "\n")
        for r in func_rows:
            fh.write("\t".join(hex(r[c]) if c == "addr" else str(r[c]) for c in cols) + "\n")
    with open(out / "files.tsv", "w") as fh:
        cols = list(file_rows[0].keys())
        fh.write("\t".join(cols) + "\n")
        for r in file_rows:
            fh.write("\t".join(str(r[c]) for c in cols) + "\n")
    with open(out / "data.json", "w") as fh:
        json.dump(data_rows, fh)
    with open(out / "modules.json", "w") as fh:
        json.dump(module_meta, fh)
    print(f"functions {len(func_rows)} files {len(file_rows)} data rows {len(data_rows)}")


if __name__ == "__main__":
    main()
