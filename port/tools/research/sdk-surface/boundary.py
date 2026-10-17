"""Quantify the boundary options: NitroSDK/TwlSDK API surface used by everything kept native, by caller layer."""
import json
import pickle
import re
from collections import Counter, defaultdict
from pathlib import Path

HERE = Path(__file__).parent
db = pickle.load(open(HERE / "binscan.pkl", "rb"))
F = db["funcs"]


def layer(f):
    """Coarse layer of a function."""
    lib = f["lib"]
    if f["cat"] == "game":
        return "game:" + ("gfl" if lib == "gfl" else "system" if lib == "system" else "ov")
    top = lib.split("/")[0].split("..")[0].split("|")[0]
    if f["module"] == "itcm" or f["module"] == "main" and f["cat"] == "edge" and "system" in lib:
        return "boot/itcm"
    if f["module"] == "ov011" and f["cat"] == "edge":
        return "dwc"
    if f["module"] == "ov182" and f["cat"] == "edge":
        return "dwcutil" if f["addr"] < 0x021B93E8 else "game:ov"
    if f["module"] == "ltd_autoload_0" and f["cat"] == "edge":
        return "twl"
    if f["module"] == "itcm":
        return "boot/itcm"
    return top


for f in F:
    f["layer"] = layer(f)

SDK = {"nitro", "twl", "boot/itcm"}
# the NitroSDK-internal callees reached from non-SDK code
used = defaultdict(lambda: dict(sites=0, callers=Counter()))
for caller, callee, kind, mod, frm in db["edges"]:
    if caller is None:
        continue
    cl, tl = F[caller]["layer"], F[callee]["layer"]
    if tl in SDK and cl not in SDK:
        u = used[callee]
        u["sites"] += 1
        u["callers"][cl] += 1

bysub = defaultdict(lambda: dict(funcs=0, sites=0, layers=Counter()))
for c, u in used.items():
    sub = F[c]["lib"] if F[c]["layer"] != "boot/itcm" else "boot/itcm"
    if F[c]["layer"] == "twl" and F[c]["cat"] == "edge":
        sub = "twl/ltdmain(undelinked)"
    b = bysub[sub]
    b["funcs"] += 1
    b["sites"] += u["sites"]
    b["layers"].update(u["callers"])
print("NitroSDK/TwlSDK functions reached from outside the SDK (game, NNS, SPL, DWC, NitroWiFi, MSL ...)")
tot = 0
for sub in sorted(bysub, key=lambda s: -bysub[s]["funcs"]):
    b = bysub[sub]
    tot += b["funcs"]
    print(f"  {sub:28} funcs={b['funcs']:4} sites={b['sites']:5} from={dict(b['layers'].most_common(8))}")
print("  TOTAL", tot)

# split: reached from game+NNS+SPL+MSL only (offline game), vs only from networking libs
net = {"dwc", "nitrowifi", "dpw", "poke_net", "dwcutil", "dwcutilui", "irc"}
offline = [c for c, u in used.items() if any(not (l in net) for l in u["callers"])]
only_net = [c for c, u in used.items() if all(l in net for l in u["callers"])]
print("reached from non-network code:", len(offline), " only from network libs:", len(only_net))
json.dump(sorted((F[c]["name"], F[c]["lib"], used[c]["sites"], dict(used[c]["callers"])) for c in used),
          open(HERE / "sdk_used.json", "w"), indent=0)

# NNS: functions of NitroSystem reached from game
nns = defaultdict(set)
for caller, callee, kind, mod, frm in db["edges"]:
    if caller is None:
        continue
    if F[caller]["layer"].startswith("game") and F[callee]["layer"] == "nnsys":
        nns[F[callee]["lib"]].add(callee)
print("NitroSystem funcs reached from game:", {k: len(v) for k, v in nns.items()}, sum(len(v) for v in nns.values()))

# what NNS itself needs from NitroSDK
need = defaultdict(set)
for caller, callee, kind, mod, frm in db["edges"]:
    if caller is None:
        continue
    if F[caller]["layer"] in ("nnsys", "spl") and F[callee]["layer"] in SDK:
        need[F[caller]["lib"]].add(F[callee]["name"])
for k, v in sorted(need.items()):
    print(f"{k} needs {len(v)} SDK funcs: {sorted(v)}")
