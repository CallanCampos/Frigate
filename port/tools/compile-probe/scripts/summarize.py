#!/usr/bin/env python3
"""A summary of one pass of the compile probe: how the game's and the libraries' C fares under clang for one target,
the DS's or a 64-bit host's (port/docs/research/compile-probe.md).

    summarize.py RUNDIR [--sev error|warning] [--flags F1,F2] [--top N] [--examples N] [--files files.tsv]

Reads a pass's status.tsv and diags.tsv (probe.py's, in compile-probe/<pass>/) and prints the files that pass and fail
by module (src/gfl, src/system, the overlays, each lib/), by completeness when files.tsv (complete_files.py's) is
given; then the diagnostics of one severity, optionally of some flags, by kind, with their unique sites and files; the
files and modules where the sites are; and examples. Run by hand on a pass; run_all.sh doesn't.
"""
import sys, re, collections, argparse, pathlib
ap = argparse.ArgumentParser()
ap.add_argument("run"); ap.add_argument("--sev", default="error"); ap.add_argument("--flags", default="")
ap.add_argument("--top", type=int, default=30); ap.add_argument("--examples", type=int, default=0)
ap.add_argument("--files", default=None, help="complete-status tsv")
a = ap.parse_args()
run = pathlib.Path(a.run)
status = [l.rstrip("\n").split("\t") for l in open(run / "status.tsv")]
comp = {}
if a.files:
    comp = dict(l.rstrip("\n").split("\t") for l in open(a.files))
def module(f):
    p = f.split("/")
    if p[0] == "lib": return "lib/" + p[1]
    if p[1] in ("gfl", "system"): return "src/" + p[1]
    return "src/ovNNN"
# pass/fail by module
pm = collections.defaultdict(lambda: [0, 0])
for f, st, ne, nw in status:
    pm[module(f)][0 if st == "pass" else 1] += 1
print("== pass/fail by module")
for m in sorted(pm): print(f"  {m:18s} pass={pm[m][0]:5d} fail={pm[m][1]:5d}")
tot = [sum(v[0] for v in pm.values()), sum(v[1] for v in pm.values())]
print(f"  TOTAL              pass={tot[0]:5d} fail={tot[1]:5d}")
if comp:
    c = collections.Counter((comp.get(f, "?"), st) for f, st, *_ in status)
    print("  by completeness:", dict(c))
flags = set(x for x in a.flags.split(",") if x)
rows = [l.rstrip("\n").split("\t") for l in open(run / "diags.tsv")]
rows = [r for r in rows if r[1] == a.sev and (not flags or r[2] in flags)]
def norm(msg):
    msg = re.sub(r"'[^']*'", "'X'", msg)
    msg = re.sub(r"\b\d+\b", "N", msg)
    return msg
print(f"== {a.sev}s: {len(rows)} diagnostics, {len(set((r[3], r[2], r[4]) for r in rows))} unique sites, in {len(set(r[0] for r in rows))} files")
byflag = collections.Counter(r[2] or norm(r[4]) for r in rows)
sites = collections.defaultdict(set); files = collections.defaultdict(set)
for r in rows:
    k = r[2] or norm(r[4]); sites[k].add((r[3], r[4])); files[k].add(r[0])
print("== by kind: diags / unique sites / files")
for k, n in byflag.most_common(a.top):
    print(f"  {n:7d} {len(sites[k]):6d} {len(files[k]):5d}  {k}")
# where the unique sites live (header vs .c)
loc_files = collections.Counter(s[0].rsplit(":", 2)[0] for k in sites for s in sites[k])
print("== top locations (unique sites per source/header file)")
for f, n in loc_files.most_common(a.top): print(f"  {n:6d}  {f}")
bym = collections.Counter(module(s[0].rsplit(':',2)[0]) if s[0].startswith(("src/","lib/")) else s[0].split('/')[0] for k in sites for s in sites[k])
print("== unique sites by module of the location"); 
for m, n in bym.most_common(): print(f"  {n:6d}  {m}")
if a.examples:
    print("== examples")
    for k, n in byflag.most_common(a.top):
        ex = sorted(sites[k])[:a.examples]
        for loc, msg in ex: print(f"  [{k}] {loc}: {msg[:160]}")
