"""Per library sub-module dossier: size, C coverage, hardware evidence (binary + C), dependencies on PXI/IRQ/DMA/cache."""
import json
import pickle
import re
from collections import Counter, defaultdict
from pathlib import Path

DATA = Path(__file__).resolve().parents[4] / "build/port-research/sdk-surface"  # ignored by git; db.py fills it
db = pickle.load(open(DATA / "binscan.pkl", "rb"))
hw = json.load(open(DATA / "hwscan.json"))
F = db["funcs"]


def sub(f):
    if f["cat"] == "game":
        return None
    if f["module"] == "itcm":
        return "boot/itcm"
    if f["module"] == "main" and f["cat"] == "edge":
        return "boot/crt0+svc" if f["addr"] < 0x02005124 else "msl/-"
    if f["module"] == "ov011" and f["cat"] == "edge":
        return "dwc/(undelinked ov011)"
    if f["module"] == "ov182" and f["cat"] == "edge":
        return "dwcutil/(undelinked ov182)" if f["addr"] < 0x021B93E8 else None
    if f["module"] == "ltd_autoload_0" and f["cat"] == "edge":
        return "twl/(undelinked .ltdmain)"
    return f["lib"]


DEPS = {
    "PXI": re.compile(r"^PXIi?_"),
    "IRQ": re.compile(r"^OS_(SetIrqFunction|EnableIrqMask|DisableIrqMask|SetIrqCheckFlag|WaitIrq|ResetRequestIrqMask)"),
    "DMA": re.compile(r"^MIi?_(.*Dma|SendGXCommand|SendNDmaGX)"),
    "CACHE": re.compile(r"^(DC|IC)_"),
    "THREAD": re.compile(r"^OS_(CreateThread|SleepThread|WakeupThread|JoinThread|YieldThread)"),
    "ALARM": re.compile(r"^OS_(SetAlarm|SetPeriodicAlarm|CancelAlarm|CreateAlarm|SetVAlarm|CreateVAlarm|CancelVAlarm)"),
    "SVC": re.compile(r"^(SVC_|Div$|Mod$|Sqrt$|CpuSet|CpuFastSet|LZ77|RLUn|HuffUn|BitUnPack|IntrWait|VBlankIntrWait|Halt$)"),
}

agg = defaultdict(lambda: dict(n=0, bytes=0, complete=0, unnamed=0, hwf=0, regions=Counter(), swi=Counter(), cp15=0, psr=0,
                               deps=Counter(), files=set()))
for f in F:
    s = sub(f)
    if s is None:
        continue
    a = agg[s]
    a["n"] += 1
    a["bytes"] += f["size"]
    a["complete"] += f["complete"]
    a["unnamed"] += f["name"].startswith("func_")
    h = f["hw"]
    if h["regions"] or h["swi"] or h["cp15"] or h["psr"]:
        a["hwf"] += 1
    for k in h["regions"]:
        a["regions"][k] += 1
    for x in h["swi"]:
        a["swi"][x] += 1
    a["cp15"] += bool(h["cp15"])
    a["psr"] += bool(h["psr"])
    if f["path"]:
        a["files"].add(f["path"])
    f["sub"] = s

for caller, callee, kind, mod, frm in db["edges"]:
    if caller is None:
        continue
    cf, tf = F[caller], F[callee]
    s = cf.get("sub")
    if not s:
        continue
    if tf.get("sub") == s:
        continue
    for d, rx in DEPS.items():
        if rx.match(tf["name"]):
            agg[s]["deps"][d] += 1

# C-side evidence per sub (macros, asm funcs)
cmac = defaultdict(Counter)
casm = defaultdict(list)
for rel, r in hw["files"].items():
    p = rel.split("/")
    if p[0] != "lib" or p[2] != "src":
        continue
    s = f"{p[1]}/{p[3] if len(p) > 4 else '-'}"
    for k, v in r["macros"].items():
        cmac[s][k] += v
    for k, v in r["raw"].items():
        cmac[s][k] += v
    casm[s].extend(r["asm_funcs"])

out = {}
for s in sorted(agg):
    a = agg[s]
    out[s] = dict(n=a["n"], bytes=a["bytes"], complete=a["complete"], unnamed=a["unnamed"], hw_funcs=a["hwf"],
                  regions=dict(a["regions"].most_common()), swi=dict(a["swi"]), cp15_funcs=a["cp15"], psr_funcs=a["psr"],
                  deps=dict(a["deps"]), c_regs=dict(cmac[s].most_common(14)), c_asm_funcs=casm[s], files=len(a["files"]))
    print(f"== {s}: n={a['n']} bytes={a['bytes']} complete={a['complete']} files={len(a['files'])} hwfuncs={a['hwf']} cp15={a['cp15']} psr={a['psr']} swi={dict(a['swi'])}")
    print(f"   bin regions: {dict(a['regions'].most_common())}")
    print(f"   deps: {dict(a['deps'])}")
    if cmac[s]:
        print(f"   C regs: {dict(cmac[s].most_common(14))}")
    if casm[s]:
        print(f"   C asm funcs ({len(casm[s])}): {casm[s][:25]}")
json.dump(out, open(DATA / "dossier.json", "w"), indent=1)
