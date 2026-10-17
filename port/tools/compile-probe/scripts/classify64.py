#!/usr/bin/env python3
"""Classify the unique 64-bit hazard sites of a host probe run by the text of the source line.
Usage: classify64.py REPO RUNDIR [--flags a,b] [--show TAG] [--n N]"""
import sys, re, collections, argparse, pathlib
ap = argparse.ArgumentParser(); ap.add_argument("repo"); ap.add_argument("run")
ap.add_argument("--flags", default="-Wpointer-to-int-cast,-Wvoid-pointer-to-int-cast,-Wint-to-pointer-cast,-Wint-to-void-pointer-cast,-Wshorten-64-to-32")
ap.add_argument("--prefix", default=""); ap.add_argument("--show", default=None); ap.add_argument("--n", type=int, default=8)
a = ap.parse_args(); repo = pathlib.Path(a.repo); flags = set(a.flags.split(","))
cache = {}
def line(loc):
    f, l, c = loc.rsplit(":", 2)
    if f not in cache:
        try: cache[f] = (repo / f).read_text(errors="replace").splitlines()
        except Exception: cache[f] = []
    L = cache[f]; i = int(l) - 1
    return L[i] if 0 <= i < len(L) else ""
RULES = [
    ("OVERLAY_ID (linker symbol as int)", lambda t, m, fl: re.search(r"OVERLAY_(ID|[A-Z])", t) is not None),
    ("ptr through a u32 param of a Get/SetParam-style API", lambda t, m, fl: "pointer-to-int" in fl and re.search(r"(Set|Get)(Param|Value|Data|Work)\w*\(|_SetParam|ParamSet|Param_Set|GetWork", t) is not None),
    ("ptr through a u32 return/param of a Get/SetParam-style API (int->ptr)", lambda t, m, fl: "int-to" in fl and re.search(r"(Get|Set)\w*(Param|Value|Data|Info|Work)\w*\(", t) is not None),
    ("hw register / fixed address", lambda t, m, fl: re.search(r"0x0[2-7][0-9a-fA-F]{6}|REG_|HW_|reg_", t) is not None),
    ("alignment math on pointer ((u32)p & / + ...)", lambda t, m, fl: re.search(r"\(u32\)\s*\(?[\w>.\-\[\]]+\)?\s*[&%]|[&%]\s*\(u32\)|ROUND|ALIGN|& ~|& 0x[0-9a-fA-F]*[13f]\b", t) is not None and "pointer-to-int" in fl),
    ("ptr stored in u32/int field or var", lambda t, m, fl: "pointer-to-int" in fl and re.search(r"=\s*\((u32|s32|int|u16|ulong)\)", t) is not None),
    ("int/u32 used as pointer (u32 field holds address)", lambda t, m, fl: "int-to-pointer" in fl or "int-to-void-pointer" in fl),
    ("ptr difference / size_t / long to int", lambda t, m, fl: "shorten" in fl and re.search(r"long|ptrdiff|size_t", m) is not None),
    ("u64 to u32 (intentional truncation, same on DS)", lambda t, m, fl: "shorten" in fl and "u64" in m or "long long" in m),
    ("ptr passed/compared as int (other)", lambda t, m, fl: "pointer-to-int" in fl),
]
rows = set()
for l in open(pathlib.Path(a.run) / "diags.tsv"):
    f, sev, fl, loc, msg = l.rstrip("\n").split("\t")
    if fl in flags and loc.startswith(tuple(a.prefix.split(","))): rows.add((fl, loc, msg))
cnt = collections.Counter(); ex = collections.defaultdict(list); files = collections.defaultdict(set)
for fl, loc, msg in sorted(rows):
    t = line(loc)
    for name, fn in RULES:
        if fn(t, msg, fl):
            break
    else:
        name = "other"
    cnt[name] += 1; files[name].add(loc.rsplit(":", 2)[0]); ex[name].append(f"{loc}: {t.strip()[:150]}")
print(f"unique sites: {len(rows)} in {len(set(r[1].rsplit(':',2)[0] for r in rows))} files")
for k, n in cnt.most_common():
    print(f"  {n:6d} sites {len(files[k]):5d} files  {k}")
for k in cnt:
    if a.show is None or a.show in k:
        print(f"-- {k}")
        for e in ex[k][:a.n]: print("   ", e)
