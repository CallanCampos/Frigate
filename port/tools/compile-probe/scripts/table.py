#!/usr/bin/env python3
"""The 64-bit portability warnings of several compile-probe passes side by side: for each warning flag (pointer and int
casts, 64-to-32 shortening, alignment, library redeclarations and the rest), the unique source locations in the game
(src/ and include/) and in the libraries (lib/), as a Markdown table with each pass's files compiled.

    table.py WORKDIR PASS...

Reads WORKDIR/<pass>/diags.tsv and status.tsv (probe.py's). run_all.sh puts the table for i386, arm64, x86-64 Windows
and the arm64 pass with the MWCC shim in metrics.txt (port/docs/research/compile-probe.md, "Host compile"). The port's
64-bit metric is no longer this count but the pointer cast guard's (port/tools/check_ptrcasts.py --report).
"""
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
