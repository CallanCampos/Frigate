"""Scan C sources and headers for hardware access: register macros, raw hardware addresses, asm, SVC, CP15.
Writes hwscan.json with per-file findings and prints a per-library summary."""
import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

W = Path("/Users/cjcampos/dev/decomp/pokebw2/.claude/worktrees/desktop-subagent-inspection-3190ba")
OUT = Path(__file__).parent

files = [p for p in list(W.glob("src/**/*.c")) + list(W.glob("lib/**/*.c")) + list(W.glob("lib/**/*.h")) + list(W.glob("include/**/*.h"))]

DEF_RE = re.compile(r"#define\s+(\w+)\s+(.*)")
HEX_RE = re.compile(r"\b0x([0-9a-fA-F]{7,8})\b")


def region(v):
    if 0x04000000 <= v < 0x05000000:
        return "IO"
    if 0x05000000 <= v < 0x06000000:
        return "PLTT"
    if 0x06000000 <= v < 0x07000000:
        return "VRAM"
    if 0x07000000 <= v < 0x08000000:
        return "OAM"
    if 0x08000000 <= v < 0x0B000000:
        return "GBASLOT"
    if 0x01FF8000 <= v < 0x02000000:
        return "ITCM"
    if 0x027E0000 <= v < 0x027E4000 or 0x02FE0000 <= v < 0x02FE4000:
        return "DTCM"
    if 0x027FF000 <= v < 0x02800000 or 0x02FFC000 <= v < 0x03000000 or 0x02FFE000 <= v < 0x02FFF000:
        return "SYSSHARED"
    if 0x03000000 <= v < 0x04000000:
        return "WRAM"
    if 0xFFFF0000 <= v:
        return "BIOS"
    return None


# 1) collect macro -> (address, region)
macros = {}
for p in files:
    try:
        text = p.read_text(errors="replace")
    except Exception:
        continue
    for line in text.splitlines():
        m = DEF_RE.match(line.strip())
        if not m:
            continue
        name, body = m.groups()
        h = HEX_RE.search(body)
        if h:
            v = int(h.group(1), 16)
            r = region(v)
            if r and r not in ("BIOS", "GBASLOT") and (name.startswith(("reg_", "REG_", "HW_")) or "*)" in body or "* )" in body) and not name.endswith(("_MASK", "_SHIFT", "_SIZE")):
                macros[name] = (v, r, str(p.relative_to(W)))
# second pass: macros defined via other macros (e.g. #define X (HW_FOO + 0x10))
for _ in range(3):
    for p in files:
        text = p.read_text(errors="replace")
        for line in text.splitlines():
            m = DEF_RE.match(line.strip())
            if not m:
                continue
            name, body = m.groups()
            if name in macros:
                continue
            for tok in re.findall(r"\b[A-Za-z_]\w*\b", body):
                if tok in macros and tok != name:
                    macros[name] = (macros[tok][0], macros[tok][1], str(p.relative_to(W)))
                    break

TOK_RE = re.compile(r"\b[A-Za-z_]\w*\b")
ASM_FUNC_RE = re.compile(r"^\s*(?:static\s+)?(?:inline\s+)?asm\s+[\w\s\*]+\(", re.M)
ASM_BLOCK_RE = re.compile(r"\basm\s*\{|__asm\b|\basm\s*\(")
CP15_RE = re.compile(r"\b(mcr|mrc)\s+p15", re.I)
SWI_RE = re.compile(r"\b(swi|svc)\s+#?(0x[0-9a-f]+|\d+)", re.I)
MSR_RE = re.compile(r"\b(msr|mrs)\b", re.I)

results = {}
for p in files:
    rel = str(p.relative_to(W))
    text = p.read_text(errors="replace")
    # strip comments
    code = re.sub(r"//.*", "", text)
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    res = dict(macros=Counter(), raw=Counter(), regions=Counter(), asm_funcs=[], asm_blocks=0, cp15=0, swi=[], msr=0)
    defined_here = set()
    for line in code.splitlines():
        m = DEF_RE.match(line.strip())
        if m:
            defined_here.add(m.group(1))
            continue
        for tok in TOK_RE.findall(line):
            if tok in macros:
                res["macros"][tok] += 1
                res["regions"][macros[tok][1]] += 1
        for hm in re.finditer(r"\*\s*\)\s*\(?\s*0x([0-9a-fA-F]{7,8})\b", line):
            v = int(hm.group(1), 16)
            r = region(v)
            if r:
                res["raw"][f"0x{v:08x}"] += 1
                res["regions"][r] += 1
    for m in ASM_FUNC_RE.finditer(code):
        line = code[m.start():code.find("(", m.start())]
        res["asm_funcs"].append(line.strip().split()[-1])
    res["asm_blocks"] = len(ASM_BLOCK_RE.findall(code))
    res["cp15"] = len(CP15_RE.findall(code))
    res["swi"] = SWI_RE.findall(code)
    res["msr"] = len(MSR_RE.findall(code))
    if res["macros"] or res["raw"] or res["asm_funcs"] or res["asm_blocks"] or res["swi"] or res["cp15"]:
        results[rel] = dict(macros=dict(res["macros"]), raw=dict(res["raw"]), regions=dict(res["regions"]),
                            asm_funcs=res["asm_funcs"], asm_blocks=res["asm_blocks"], cp15=res["cp15"],
                            swi=[list(s) for s in res["swi"]], msr=res["msr"])

json.dump(dict(macros={k: [hex(v[0]), v[1], v[2]] for k, v in macros.items()}, files=results),
          open(OUT / "hwscan.json", "w"), indent=1)
print("macros", len(macros), "files with findings", len(results))


def group(rel):
    p = rel.split("/")
    if p[0] == "lib":
        if p[2] == "src":
            return f"lib/{p[1]}/{p[3] if len(p) > 4 else '-'}"
        return f"lib/{p[1]}/include"
    if p[0] == "src":
        return f"src/{p[1]}"
    return f"include/{p[1] if len(p) > 2 else '-'}"


agg = defaultdict(lambda: dict(files=0, regions=Counter(), asm=0, asmb=0, cp15=0, swi=0, msr=0))
for rel, r in results.items():
    g = group(rel)
    if g.startswith("src/ov"):
        g = "src/ovNNN"
    a = agg[g]
    a["files"] += 1
    a["regions"].update(r["regions"])
    a["asm"] += len(r["asm_funcs"])
    a["asmb"] += r["asm_blocks"]
    a["cp15"] += r["cp15"]
    a["swi"] += len(r["swi"])
    a["msr"] += r["msr"]
for g in sorted(agg):
    a = agg[g]
    print(f"{g:28} files={a['files']:3} asmfn={a['asm']:3} asmblk={a['asmb']:3} cp15={a['cp15']:3} swi={a['swi']:2} msr={a['msr']:3} {dict(a['regions'])}")
