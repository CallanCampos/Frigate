"""How each library of the ROM is laid out and how much of it is C: function counts, bytes, functions in a source file
and in complete files, and unnamed functions, per category (game, SDK, the boundary's "edge") and library.

    layout.py [game]

Reads build/port-research/sdk-surface/db.pkl (db.py: every function of Black 2's modules with its library and file,
from config/b2_us and extract/b2_us) and prints a row per category and library, with the modules it sits in; the game's
rows only when an argument is given, and the game's totals. A summary from the research behind
port/docs/research/sdk-surface.md.
"""
import pickle
import sys
from collections import Counter, defaultdict
from pathlib import Path

db = pickle.load(open(Path(__file__).resolve().parents[4] / "build/port-research/sdk-surface/db.pkl", "rb"))
F = db["funcs"]

per = defaultdict(lambda: Counter())
for f in F:
    key = (f["cat"], f["lib"])
    per[key]["funcs"] += 1
    per[key]["bytes"] += f["size"]
    if not f["inferred"]:
        per[key]["in_file"] += 1
        if f["complete"]:
            per[key]["complete"] += 1
    if f["name"].startswith("func_"):
        per[key]["unnamed"] += 1
    per[key]["mods:" + f["module"]] += 0

mods_of = defaultdict(set)
for f in F:
    mods_of[(f["cat"], f["lib"])].add(f["module"])

print(f"{'cat':5} {'lib':32} {'funcs':>6} {'bytes':>8} {'inFile':>6} {'cmpl':>5} {'unnamed':>7}  modules")
for key in sorted(per, key=lambda k: (k[0], k[1])):
    if key[0] == "game" and len(sys.argv) < 2:
        continue
    c = per[key]
    ms = sorted(mods_of[key])
    msd = ",".join(ms[:6]) + (f"...(+{len(ms)-6})" if len(ms) > 6 else "")
    print(f"{key[0]:5} {key[1]:32} {c['funcs']:6} {c['bytes']:8} {c['in_file']:6} {c['complete']:5} {c['unnamed']:7}  {msd}")

g = Counter()
for f in F:
    if f["cat"] == "game":
        g["funcs"] += 1
        g["bytes"] += f["size"]
        g["in_file"] += not f["inferred"]
print("game total", dict(g))
