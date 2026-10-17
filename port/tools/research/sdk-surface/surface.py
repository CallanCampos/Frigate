"""Game-side surface: which library functions game code (src/) calls or references, by family."""
import json
import pickle
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

HERE = Path(__file__).parent
W = Path("/Users/cjcampos/dev/decomp/pokebw2/.claude/worktrees/desktop-subagent-inspection-3190ba")
db = pickle.load(open(HERE / "binscan.pkl", "rb"))
F = db["funcs"]

FAMILIES = [
    ("NNS_G2d", r"^NNSi?_G2d"), ("NNS_G3d", r"^NNSi?_G3d"), ("NNS_Gfd", r"^NNSi?_Gfd"), ("NNS_Fnd", r"^NNSi?_Fnd"),
    ("NNS_Snd", r"^NNSi?_Snd"), ("NNS_other", r"^NNSi?_"),
    ("G3X_", r"^G3Xi?_"), ("G3_", r"^G3i?_"), ("G3B_/G3C_", r"^G3[BC]i?_"), ("G2S_", r"^G2Si?_"), ("G2_", r"^G2i?_"),
    ("GXS_", r"^GXSi?_"), ("GX_", r"^GXi?_"), ("SNDEX_", r"^SNDEXi?_"), ("SND_", r"^SNDi?_"), ("OS_", r"^OSi?_"),
    ("FS_", r"^FSi?_"), ("MI_", r"^MIi?_"), ("CARD_", r"^CARDi?_"), ("PAD_", r"^PADi?_"), ("TP_", r"^TPi?_"),
    ("RTC_", r"^RTCi?_"), ("WM_", r"^WMi?_"), ("MB_", r"^MBi?_"), ("CTRDG_", r"^CTRDGi?_"), ("PM_", r"^PMi?_"),
    ("MIC_", r"^MICi?_"), ("DWC_", r"^DWCi?_"), ("SOC_", r"^SOCi?_"), ("CPS_", r"^CPSi?_"), ("WCM_", r"^WCMi?_"),
    ("FX_", r"^FXi?_"), ("MTX_", r"^MTXi?_"), ("VEC_", r"^VECi?_"), ("MATH_", r"^MATHi?_"), ("CP_", r"^CPi?_"),
    ("STD_", r"^STDi?_"), ("SVC_", r"^SVCi?_"), ("SPL_", r"^SPLi?_"), ("CAMERA_", r"^CAMERAi?_"), ("DSP_", r"^DSPi?_"),
    ("NWM_", r"^NWMi?_"), ("FATFS_", r"^FATFSi?_"), ("SCFG_", r"^SCFGi?_"), ("NVRAM_", r"^NVRAMi?_"),
    ("CRYPTO_", r"^CRYPTOi?_"), ("SSP_", r"^SSPi?_"), ("PXI_", r"^PXIi?_"), ("SPI_", r"^SPIi?_"),
    ("IRC_", r"^(IRC|irc|IRCi)_?"), ("DPW_", r"^(Dpw|DPW)"), ("POKE_NET_", r"^(POKE_NET|PokeNet)"),
    ("DSProt", r"^(DSProt|DSP_rot)"),
]


def family(f):
    n = f["name"]
    for fam, rx in FAMILIES:
        if re.match(rx, n):
            return fam
    lib = f["lib"]
    if lib.startswith("msl"):
        return "MSL(libc)"
    return "unnamed:" + lib.split("|")[0].split("..")[0]


def is_lib(f):
    return f["cat"] in ("lib", "edge") and not (f["cat"] == "edge" and f["module"].startswith("ov182") and "ov182" in f["lib"] and "dwcutil" not in f["lib"])


callee_sites = defaultdict(list)
for caller, callee, kind, mod, frm in db["edges"]:
    if caller is None:
        continue
    cf, tf = F[caller], F[callee]
    if cf["cat"] != "game" or tf["cat"] == "game":
        continue
    callee_sites[callee].append((caller, kind))

# C source text of game code for "visible in C" check
src_text = {}
for p in W.glob("src/**/*.c"):
    src_text[str(p.relative_to(W))] = p.read_text(errors="replace")
all_src = "\n".join(src_text.values())
TOK = set(re.findall(r"\b[A-Za-z_]\w*\b", all_src))

fam = defaultdict(lambda: dict(funcs=set(), sites=0, refs=0, callers=set(), in_c=set(), by_area=Counter()))
for callee, sites in callee_sites.items():
    tf = F[callee]
    fm = family(tf)
    a = fam[fm]
    a["funcs"].add(callee)
    for caller, kind in sites:
        if kind == "load":
            a["refs"] += 1
        else:
            a["sites"] += 1
        a["callers"].add(caller)
        area = F[caller]["lib"]
        a["by_area"]["gfl" if area == "gfl" else "system" if area == "system" else "ov"] += 1
    if tf["name"] in TOK:
        a["in_c"].add(callee)

rows = []
for fm, a in fam.items():
    rows.append((fm, len(a["funcs"]), a["sites"], a["refs"], len(a["callers"]), len(a["in_c"]), dict(a["by_area"])))
rows.sort(key=lambda r: -r[2] - r[3])
print(f"{'family':22} {'funcs':>5} {'calls':>6} {'ptrref':>6} {'callerF':>7} {'inC':>4}  by area (gfl/system/ov)")
tot = [0, 0, 0]
for r in rows:
    print(f"{r[0]:22} {r[1]:5} {r[2]:6} {r[3]:6} {r[4]:7} {r[5]:4}  {r[6]}")
    tot[0] += r[1]; tot[1] += r[2]; tot[2] += r[3]
print("TOTAL distinct lib funcs called by game:", tot[0], "call sites", tot[1], "ptr refs", tot[2])

# top functions per family
out = {}
for fm, a in fam.items():
    lst = sorted(a["funcs"], key=lambda c: -len(callee_sites[c]))
    out[fm] = [(F[c]["name"], len(callee_sites[c]), F[c]["lib"], F[c]["name"] in TOK) for c in lst]
json.dump(out, open(HERE / "surface.json", "w"), indent=1)
