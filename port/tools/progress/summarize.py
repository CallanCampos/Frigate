#!/usr/bin/env python3
"""Markdown tables from progress.py output (and optionally compile_check.py output and gh issues).

Usage: python3 summarize.py --in out/b2_us [--compile out/compile_b2_us.tsv] [--issues gh_issues.json] > tables.md

The tables of port/docs/research/decomp-progress.md (by module, by origin, the overlays by missing C, data), which
run_all.sh writes to tables_<version>.md and all_overlays_b2_us.md.
"""
import argparse
import csv
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("--in", dest="inp", required=True)
ap.add_argument("--compile")
ap.add_argument("--issues")
ap.add_argument("--all-overlays", action="store_true")
args = ap.parse_args()
inp = Path(args.inp)
F = list(csv.DictReader(open(inp / "functions.tsv"), delimiter="\t"))
FILES = list(csv.DictReader(open(inp / "files.tsv"), delimiter="\t"))
DATA = json.load(open(inp / "data.json"))

# A few hand-written purposes for the overlays that matter most (file names are added automatically)
PURPOSE = {
    "ov011": "Nintendo Wi-Fi Connection stack (DWC, GameSpy, NitroWiFi SOC/SSL/CPS, VCT)",
    "ov036": "field (overworld) engine: map, player, NPCs, rails, scripts, weather",
    "ov182": "Wi-Fi Connection settings utility (DWC util/AOSS) and Download Play",
    "ov012": "field movement models, actors, event-script engine and commands",
    "ov167": "battle engine (server flow, client, handlers)",
    "ov168": "battle view: effects VM, input, MCSS",
    "ov169": "battle (VRAM-resident): scene, side/pos/condition handlers",
    "ov189": "GTS / Battle Subway Wi-Fi (DPW, DWC ghttp/nhttp)",
    "ov257": "Xtransceiver (C-Gear video chat) with SSP JPEG",
    "ov260": "Wi-Fi battle match (rating battles)",
    "ov181": "DS Download Play / multiboot (NitroSDK MB)",
    "ov029": "DWC VCT voice chat",
    "ov165": "party list (pokemon list) + DS Protect",
    "ov337": "DS Protect",
    "ov280": "name input screen",
    "ov194": "Pokemon trade (union/link trade)",
    "ov197": "Mystery Gift album / Mystery Gift",
    "ov139": "shared touch UI: frame list, yes/no menu, touch bar",
    "ov332": "Unova Link-style data flows: Cygnus, Key System, data convert (guess from file names)",
    "ov214": "GTS (World Trade) screens",
    "ov102": "roller-coaster gym gimmick",
    "ov033": "battle facility scripts: Battle Subway, PDW postman, Trial House",
    "ov201": "Global Data Server client (poke_net, GDS)",
    "ov338": "infrared check",
    "ov081": "wbt_subscreen",
}


def tiers_of(rows):
    c = Counter()
    b = Counter()
    for r in rows:
        k = r["sub"]
        k = {"b_order": "b", "b_uncertain": "b"}.get(k, k)
        c[k] += 1
        b[k] += int(r["size"])
    return c, b


def fmt(n):
    return f"{n:,}"


def pct(a, t):
    return f"{100.0 * a / t:.1f}%" if t else "-"


def fix_cat(r):
    cat = r["cat"]
    addr = int(r["addr"], 16)
    if r["sub"] == "c_gap" and r["module"] == "ov011":
        return "lib/dwc"  # every ov011 gap lies in the DWC/GameSpy stack (0x0215dd74 on, between lib/ files)
    if r["sub"] == "c_gap" and r["module"] == "ov182":
        # after dwc_util_screen.c: the DWC utility's screens; after download_play_sys.c: unknown
        return "lib/dwcutilui (likely)" if addr < 0x021b93e8 else "game"
    if r["tier"] != "c" or not cat.startswith("unknown"):
        return cat
    if r["module"] == "main" and addr < 0x02005124:
        return "lib/twl (crt0 + BIOS SVC stubs, unassigned)"
    if r["module"] == "main":
        return "lib/msl" if "lib/msl" in cat else cat
    if r["module"] == "itcm":
        # the four ClAct OAM renderers (gfl/clact.c's renderer table) and NitroSDK's IRQ handler and reset code
        return "game (gfl ClAct renderers in ITCM)" if addr < 0x01ff8bf0 else "lib/nitro (ITCM: OS_IrqHandler, reset)"
    if r["module"] == "ltd_autoload_0":
        return "lib/twl"
    return cat


print("## Code tiers (all modules)\n")
c, b = tiers_of(F)
tot_fn = sum(v for k, v in c.items() if k != "veneer")
tot_b = sum(v for k, v in b.items() if k != "veneer")
print("| Tier | Functions | % | Code bytes | % |\n|---|---:|---:|---:|---:|")
for k, label in (("a", "(a) complete, matching C, linked"), ("b", "(b) C written, file incomplete (not linked)"),
                 ("c_gap", "(c) no C: in no source file (dsd gap)"),
                 ("c_nosrc", "(c) no C: file registered, .c not written"),
                 ("c_partial", "(c) no C: function missing from a partial .c")):
    print(f"| {label} | {fmt(c[k])} | {pct(c[k], tot_fn)} | {fmt(b[k])} | {pct(b[k], tot_b)} |")
cc = c["c_gap"] + c["c_nosrc"] + c["c_partial"]
cb = b["c_gap"] + b["c_nosrc"] + b["c_partial"]
print(f"| **(c) total** | **{fmt(cc)}** | **{pct(cc, tot_fn)}** | **{fmt(cb)}** | **{pct(cb, tot_b)}** |")
print(f"| total (without veneers) | {fmt(tot_fn)} | | {fmt(tot_b)} | |")
print(f"| linker veneers (not C, excluded) | {fmt(c['veneer'])} | | {fmt(b['veneer'])} | |")
asm = sum(1 for r in F if r["asm"] == "True")
asmb = sum(int(r["size"]) for r in F if r["asm"] == "True")
print(f"\nOf tiers (a)+(b), {asm} functions ({fmt(asmb)} bytes) are MWCC `asm` functions in C files (lib/ only).\n")


def group(mod):
    return "overlays" if mod.startswith("ov") else mod


print("## By module group\n")
print("| Module | Functions | Code bytes | (a) bytes | (b) bytes | (c) bytes | (c) % | (c) functions |")
print("|---|---:|---:|---:|---:|---:|---:|---:|")
groups = defaultdict(list)
for r in F:
    if r["sub"] != "veneer":
        groups[group(r["module"])].append(r)
for g in ("main", "itcm", "dtcm", "autoload_2", "autoload_3", "ltd_autoload_0", "overlays"):
    rows = groups.get(g, [])
    c, b = tiers_of(rows)
    t = sum(b.values())
    cb = b["c_gap"] + b["c_nosrc"] + b["c_partial"]
    print(f"| {g} | {fmt(len(rows))} | {fmt(t)} | {fmt(b['a'])} | {fmt(b['b'])} | {fmt(cb)} | {pct(cb, t)} | "
          f"{fmt(c['c_gap'] + c['c_nosrc'] + c['c_partial'])} |")

print("\n## By origin (game code or library)\n")
print("| Origin | Code bytes | (a) bytes | (a) % | (b) bytes | (c) bytes | (c) % | where (c) is |")
print("|---|---:|---:|---:|---:|---:|---:|---|")
cats = defaultdict(list)
for r in F:
    if r["sub"] != "veneer":
        cats[fix_cat(r)].append(r)
for k in sorted(cats, key=lambda k: (k != "game", k)):
    rows = cats[k]
    c, b = tiers_of(rows)
    t = sum(b.values())
    cb = b["c_gap"] + b["c_nosrc"] + b["c_partial"]
    where = Counter(r["module"] for r in rows if r["tier"] == "c")
    wb = defaultdict(int)
    for r in rows:
        if r["tier"] == "c":
            wb[r["module"]] += int(r["size"])
    ws = ", ".join(f"{m} {fmt(v)}" for m, v in sorted(wb.items(), key=lambda x: -x[1]))
    print(f"| {k} | {fmt(t)} | {fmt(b['a'])} | {pct(b['a'], t)} | {fmt(b['b'])} | {fmt(cb)} | {pct(cb, t)} | {ws} |")

# per overlay
issues = {}
if args.issues:
    for i in json.load(open(args.issues)):
        m = re.match(r"\[Decomp\] Overlay (\d+)", i["title"])
        if m:
            issues[f"ov{int(m.group(1)):03d}"] = (i["number"], i["state"])
ov = defaultdict(list)
for r in F:
    if r["module"].startswith("ov") and r["sub"] != "veneer":
        ov[r["module"]].append(r)
ovfiles = defaultdict(list)
for f in FILES:
    if f["module"].startswith("ov"):
        ovfiles[f["module"]].append(f)
# data (rodata+data) and pointer words not in C, per module: gaps, files without .c, and what an incomplete file's
# C object emits short of its range (needs --compile; otherwise incomplete files count as in C)
dnc = defaultdict(int)
pnc = defaultdict(int)
for r in DATA:
    if r["section"] in (".rodata", ".data") and r["tier"] in ("c_gap", "c_nosrc"):
        dnc[r["module"]] += r["bytes"]
        pnc[r["module"]] += r["ptrs"]
if args.compile:
    mod_of = {f["path"]: f["module"] for f in FILES}
    for r in csv.DictReader(open(args.compile), delimiter="\t"):
        if r["status"] != "ok":
            continue
        g = lambda k: int(r[k] or 0)  # noqa: E731
        romd = g("rom.rodata") + g("rom.data")
        cd = g("c.rodata") + g("c.data") + g("c.sdata")
        romp = g("rom_ptrs_rodata") + g("rom_ptrs_data")
        cpp = g("c_ptrs_rodata") + g("c_ptrs_data")
        dnc[mod_of[r["path"]]] += max(0, romd - cd)
        pnc[mod_of[r["path"]]] += max(0, romp - cpp)
rows_out = []
for m, rows in ov.items():
    c, b = tiers_of(rows)
    t = sum(b.values())
    cb = b["c_gap"] + b["c_nosrc"] + b["c_partial"]
    fl = ovfiles.get(m, [])
    top = sorted(fl, key=lambda f: -int(f["text"]))[:3]
    names = ", ".join(Path(f["path"]).stem for f in top)
    purpose = PURPOSE.get(m, "")
    nc = sum(f["complete"] == "True" for f in fl)
    ni = sum(f["complete"] == "False" and f["has_src"] == "True" for f in fl)
    nn = sum(f["has_src"] == "False" for f in fl)
    iss = issues.get(m)
    rows_out.append((cb, b["b"], m, purpose, names, t, b["a"], b["b"], b["c_gap"], b["c_nosrc"], b["c_partial"],
                     c["c_gap"] + c["c_nosrc"] + c["c_partial"], nc, ni, nn,
                     f"#{iss[0]} {iss[1].lower()}" if iss else "-", dnc.get(m, 0), pnc.get(m, 0)))
rows_out.sort(key=lambda x: (-x[0], -x[1], x[2]))
print("\n## Overlays with code not complete, by missing C\n")
print("| Overlay | Purpose / largest files | Code bytes | (a) | (b) | (c) gap | (c) no .c | (c) partial | (c) total "
      "| (c) fns | files complete/incomplete/no .c | data not in C (ptr words) | issue |")
print("|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---|---|")
for x in rows_out:
    (cb, bb, m, purpose, names, t, a, b_, cg, cn, cp_, cf, nc, ni, nn, iss, dn, pn) = x
    if not args.all_overlays and cb == 0 and b_ == 0 and dn == 0:
        continue
    desc = (purpose + "; " if purpose else "") + names
    print(f"| {m} | {desc} | {fmt(t)} | {fmt(a)} | {fmt(b_)} | {fmt(cg)} | {fmt(cn)} | {fmt(cp_)} | **{fmt(cb)}** "
          f"| {cf} | {nc}/{ni}/{nn} | {fmt(dn)} ({pn}) | {iss} |")
full = [x for x in rows_out if x[0] == 0 and x[1] == 0 and x[16] == 0]
print(f"\n{len(full)} overlays with code are fully complete (all C, linked); "
      f"{sum(1 for x in rows_out)} overlays have code in total.\n")

# data
print("## Data sections\n")
print("| Section | Total | (a) complete file | (b) incomplete file with .c | (c) gap | (c) file without .c | "
      "pointer words (a / b / c) |")
print("|---|---:|---:|---:|---:|---:|---|")
agg = defaultdict(lambda: defaultdict(int))
for r in DATA:
    agg[r["section"]][r["tier"]] += r["bytes"]
    agg[r["section"]]["p_" + r["tier"]] += r["ptrs"]
for s in (".rodata", ".data", ".bss", ".ctor"):
    a = agg[s]
    t = a["a"] + a["b_file"] + a["c_gap"] + a["c_nosrc"]
    print(f"| {s} | {fmt(t)} | {fmt(a['a'])} | {fmt(a['b_file'])} | {fmt(a['c_gap'])} | {fmt(a['c_nosrc'])} | "
          f"{a['p_a']} / {a['p_b_file']} / {a['p_c_gap'] + a['p_c_nosrc']} |")
print("\n### Data not in complete files, per module\n")
print("| Module | Section | Tier | Bytes | Pointer words | Pointer-bearing objects (bytes) |")
print("|---|---|---|---:|---:|---|")
pm = defaultdict(lambda: defaultdict(int))
for r in DATA:
    if r["tier"] == "a":
        continue
    k = (r["module"], r["section"], r["tier"])
    pm[k]["bytes"] += r["bytes"]
    pm[k]["ptrs"] += r["ptrs"]
    pm[k]["pobj"] += r.get("ptr_objects", 0)
    pm[k]["pobjb"] += r.get("ptr_obj_bytes", 0)
for k in sorted(pm, key=lambda k: -pm[k]["bytes"]):
    v = pm[k]
    if v["bytes"] < 64 and v["ptrs"] == 0:
        continue
    po = f"{v['pobj']} ({fmt(v['pobjb'])})" if k[2] == "c_gap" else ""
    print(f"| {k[0]} | {k[1]} | {k[2]} | {fmt(v['bytes'])} | {v['ptrs']} | {po} |")

if args.compile:
    C = list(csv.DictReader(open(args.compile), delimiter="\t"))
    print("\n## Incomplete files compiled: what their C emits against the original ranges\n")
    ok = [r for r in C if r["status"] == "ok"]
    bad = [r for r in C if r["status"].startswith("compile failed")]
    nos = [r for r in C if r["status"] == "no source"]

    def n(r, k):
        return int(r[k] or 0)
    tot = defaultdict(int)
    short = []
    for r in ok:
        for k in ("rom.text", "c.text", "rom.rodata", "c.rodata", "rom.data", "c.data", "rom.bss", "c.bss",
                  "c.sdata", "c.sbss", "rom_ptrs_rodata", "c_ptrs_rodata", "rom_ptrs_data", "c_ptrs_data"):
            tot[k] += n(r, k)
        romd = n(r, "rom.rodata") + n(r, "rom.data")
        cd = n(r, "c.rodata") + n(r, "c.data") + n(r, "c.sdata")
        romp = n(r, "rom_ptrs_rodata") + n(r, "rom_ptrs_data")
        cpp = n(r, "c_ptrs_rodata") + n(r, "c_ptrs_data")
        if cd < romd - 16 or cpp < romp or n(r, "c.text") < n(r, "rom.text") * 0.9:
            short.append((romd - cd, r["path"], n(r, "rom.text"), n(r, "c.text"), romd, cd, romp, cpp,
                          n(r, "rom.bss"), n(r, "c.bss") + n(r, "c.sbss")))
    print(f"{len(ok)} compiled, {len(bad)} failed to compile, {len(nos)} have no .c.\n")
    print("| Totals over compiled incomplete files | ROM range | C object |\n|---|---:|---:|")
    print(f"| .text | {fmt(tot['rom.text'])} | {fmt(tot['c.text'])} |")
    print(f"| .rodata + .data | {fmt(tot['rom.rodata'] + tot['rom.data'])} | "
          f"{fmt(tot['c.rodata'] + tot['c.data'] + tot['c.sdata'])} |")
    print(f"| .bss | {fmt(tot['rom.bss'])} | {fmt(tot['c.bss'] + tot['c.sbss'])} |")
    print(f"| pointer relocations in data | {fmt(tot['rom_ptrs_rodata'] + tot['rom_ptrs_data'])} | "
          f"{fmt(tot['c_ptrs_rodata'] + tot['c_ptrs_data'])} |")
    print("\nFiles whose C emits clearly less code or data than the original range (data short by >16 bytes, fewer "
          "pointer words, or .text < 90%):\n")
    print("| File | .text ROM / C | data ROM / C | data pointers ROM / C | .bss ROM / C |\n|---|---|---|---|---|")
    for x in sorted(short, reverse=True):
        print(f"| `{x[1]}` | {fmt(x[2])} / {fmt(x[3])} | {fmt(x[4])} / {fmt(x[5])} | {x[6]} / {x[7]} | "
              f"{fmt(x[8])} / {fmt(x[9])} |")
    for r in bad:
        print(f"\nCompile failed: `{r['path']}`: {r['status'][:160]}")
