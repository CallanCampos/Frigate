"""Game Freak's library (GFL, src/gfl) as a boundary: which of its functions the rest of the game calls, and which
graphics calls of the game go around it to NitroSDK's GX and G3 or NitroSystem's G2D, G3D and GFD and SPL directly.

    gfl.py

Reads build/port-research/sdk-surface/binscan.pkl (binscan.py) and prints the GFL functions called from outside GFL
by family, the graphics functions that src/system and the overlays call directly, the game functions whose machine
code holds hardware addresses, and how many of those are in complete files. The answer to option (iii) of
port/docs/research/sdk-surface.md, replacing GFL's graphics wrappers with a modern renderer's calls.
"""
import json
import pickle
import re
from collections import Counter, defaultdict
from pathlib import Path

DATA = Path(__file__).resolve().parents[4] / "build/port-research/sdk-surface"  # ignored by git; db.py fills it
db = pickle.load(open(DATA / "binscan.pkl", "rb"))
F = db["funcs"]

GFX_FAM = re.compile(r"^(GXS?i?_|G2S?i?_|G2x_|GXx_|G3X?i?_|G3[BC]_|NNSi?_G2d|NNSi?_G3d|NNSi?_Gfd|NNS_G3D|SPL|MI_.*GX|DC_)")

# GFL functions called from outside gfl
gfl_api = defaultdict(lambda: dict(sites=0, files=set()))
for caller, callee, kind, mod, frm in db["edges"]:
    if caller is None:
        continue
    cf, tf = F[caller], F[callee]
    if tf["cat"] == "game" and tf["lib"] == "gfl" and not (cf["cat"] == "game" and cf["lib"] == "gfl"):
        g = gfl_api[callee]
        g["sites"] += 1
by_file = defaultdict(lambda: dict(funcs=0, sites=0))
for c, g in gfl_api.items():
    p = F[c]["path"].split("/")[-1]
    by_file[p]["funcs"] += 1
    by_file[p]["sites"] += g["sites"]
print("GFL functions used outside GFL:", len(gfl_api), "sites", sum(g["sites"] for g in gfl_api.values()))
for p, b in sorted(by_file.items(), key=lambda x: -x[1]["sites"]):
    print(f"  {p:28} funcs={b['funcs']:4} sites={b['sites']:5}")

# graphics calls bypassing GFL: system/overlay callers -> graphics SDK/NNS functions
byp = defaultdict(lambda: dict(sites=0, callers=set()))
for caller, callee, kind, mod, frm in db["edges"]:
    if caller is None:
        continue
    cf, tf = F[caller], F[callee]
    if cf["cat"] == "game" and cf["lib"] != "gfl" and tf["cat"] != "game" and GFX_FAM.match(tf["name"]):
        b = byp[callee]
        b["sites"] += 1
        b["callers"].add(caller)
callers = set()
for b in byp.values():
    callers |= b["callers"]
files = set(F[c]["path"] for c in callers)
print("graphics SDK/NNS funcs called directly by system+overlays:", len(byp), "sites", sum(b["sites"] for b in byp.values()),
      "caller funcs", len(callers), "caller files", len(files))
fam = Counter()
for c, b in byp.items():
    n = F[c]["name"]
    m = re.match(r"^(NNSi?_G2d|NNSi?_G3d|NNS_G3D|NNSi?_Gfd|GXS|GX|G2S|G2|G3X|G3|SPL|MI|DC|G2x|GXx)", n)
    fam[m.group(1) if m else n] += b["sites"]
print("  by family (sites):", dict(fam.most_common()))
# game functions with hardware addresses (binary heuristic), by area/file completeness
hwg = [f for f in F if f["cat"] == "game" and f["hw"]["regions"]]
print("game functions with hardware constants:", len(hwg), "in complete files:", sum(f["complete"] for f in hwg),
      "files:", len(set(f["path"] for f in hwg)))
reg = Counter()
for f in hwg:
    for k in f["hw"]["regions"]:
        reg[k] += 1
print("  regions:", dict(reg.most_common()))
# game completion overall
g = [f for f in F if f["cat"] == "game"]
print("game funcs", len(g), "in complete files", sum(f["complete"] for f in g))
