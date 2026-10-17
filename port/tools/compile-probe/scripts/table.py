#!/usr/bin/env python3
"""Unique-location warning counts per flag, game (src+include) vs lib, for several runs, as a markdown table.
Usage: table.py WORKDIR run1 run2 ..."""
import sys, collections, pathlib
w = pathlib.Path(sys.argv[1]); runs = sys.argv[2:]
FL = ["-Wpointer-to-int-cast", "-Wvoid-pointer-to-int-cast", "-Wint-to-pointer-cast", "-Wint-to-void-pointer-cast",
      "-Wshorten-64-to-32", "-Wcast-align", "-Wincompatible-library-redeclaration", "-Wbad-function-cast", "-Wgnu-pointer-arith", "-Wswitch", "-Wreturn-type"]
data = {}
for r in runs:
    s = collections.defaultdict(set)
    for l in open(w / r / "diags.tsv"):
        f, sev, fl, loc, msg = l.rstrip("\n").split("\t")
        if sev != "warning": continue
        g = "game" if loc.startswith(("src/", "include/")) else "lib"
        s[(fl, g)].add(loc)
    st = [l.split("\t")[1] for l in open(w / r / "status.tsv")]
    data[r] = (s, st.count("pass"), len(st))
print("| flag | " + " | ".join(r.replace("host_", "") + " game/lib" for r in runs) + " |")
print("|---|" + "---|" * len(runs))
print("| files passing | " + " | ".join(f"{data[r][1]}/{data[r][2]}" for r in runs) + " |")
for fl in FL:
    print(f"| `{fl}` | " + " | ".join(f"{len(data[r][0][(fl,'game')])} / {len(data[r][0][(fl,'lib')])}" for r in runs) + " |")
