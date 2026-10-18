#!/usr/bin/env python3
"""The game's overlays as the port must load them: each overlay's address, sizes, .data and .bss, static initializers
and C coverage, and the groups of overlays that share addresses.

    overlays.py REPO WORK

Reads Black 2's overlay table (REPO/extract/b2_us/arm9_overlays/overlays.yaml, written by dsd extract) and the
coverage that coverage.py wrote to WORK (modules.tsv, files.tsv), and writes to WORK: overlays.tsv (per overlay: its
base and end, code, .bss and .data sizes, how many static initializers it has, its functions, the share complete and
with C, the bytes with no C, and its first files), overlay_groups.tsv (the overlays whose address ranges overlap, so
that only one of a group can be loaded at a time) and overlay_bases.tsv (the overlays at each base address). The DS
loads an overlay over another at the same address; the host gives each its own memory and has to keep the game's
loads and unloads meaningful (port/hal/fs). Research of 2026-10-16 (port/docs/research/engine-boot.md, "Overlay
semantics the port must reproduce").
"""
import os
import re
import sys

W = sys.argv[1]
S = sys.argv[2]
yml = open(os.path.join(W, "extract/b2_us/arm9_overlays/overlays.yaml")).read()
ovs = []
cur = None
for line in yml.splitlines():
    m = re.match(r"\s+- id: (\d+)", line)
    if m:
        cur = {"id": int(m.group(1))}
        ovs.append(cur)
        continue
    m = re.match(r"\s+(\w+): (\S+)", line)
    if m and cur is not None:
        k, v = m.group(1), m.group(2)
        try:
            v = int(v)
        except ValueError:
            pass
        cur[k] = v

mods = {}
for line in open(os.path.join(S, "modules.tsv")).readlines()[1:]:
    f = line.rstrip("\n").split("\t")
    mods[f[0]] = f
files = {}
for line in open(os.path.join(S, "files.tsv")).readlines()[1:]:
    f = line.rstrip("\n").split("\t")
    files.setdefault(f[0], []).append(f)

out = open(os.path.join(S, "overlays.tsv"), "w")
out.write("id\tbase\tend\tcode\tbss\tdata\tctor_n\tnfuncs\tcomplete%\tC%\tnofile\tmissing_bytes+nofile_bytes\tnfiles\tncomplete_files\tfirst_files\n")
for o in ovs:
    name = "ov%03d" % o["id"]
    m = mods.get(name)
    base = o["base_address"]
    end = base + o["code_size"] + o["bss_size"]
    o["end"] = end
    ctor_n = (o["ctor_end"] - o["ctor_start"]) // 4 - 1 if o.get("ctor_end") else 0
    if m:
        n = int(m[1]); b = int(m[2]) or 1
        comp = int(m[4]); wr = int(m[6])
        nofile = int(m[9])
        miss = int(m[8]) + int(m[10])
        data = int(m[12])
        fl = files.get(name, [])
        ncf = sum(1 for f in fl if f[2] == "1")
        names = ",".join(os.path.basename(f[1]) for f in fl[:4])
        out.write(f"{o['id']}\t{base:08x}\t{end:08x}\t{o['code_size']}\t{o['bss_size']}\t{data}\t{ctor_n}\t{n}\t{100*comp/b:.0f}\t{100*(comp+wr)/b:.0f}\t{nofile}\t{miss}\t{len(fl)}\t{ncf}\t{names}\n")
    o["ctor_n"] = ctor_n

# address-sharing groups: connected components of overlap
ovs_sorted = sorted(ovs, key=lambda o: o["base_address"])
groups = []
for o in ovs_sorted:
    placed = False
    for g in groups:
        if o["base_address"] < g["end"] and o["end"] > g["start"]:
            g["members"].append(o["id"])
            g["start"] = min(g["start"], o["base_address"])
            g["end"] = max(g["end"], o["end"])
            placed = True
            break
    if not placed:
        groups.append({"start": o["base_address"], "end": o["end"], "members": [o["id"]]})
# merge groups transitively
changed = True
while changed:
    changed = False
    for i in range(len(groups)):
        for j in range(i + 1, len(groups)):
            a, b = groups[i], groups[j]
            if a["start"] < b["end"] and b["start"] < a["end"]:
                a["members"] += b["members"]
                a["start"] = min(a["start"], b["start"])
                a["end"] = max(a["end"], b["end"])
                del groups[j]
                changed = True
                break
        if changed:
            break
go = open(os.path.join(S, "overlay_groups.tsv"), "w")
for g in sorted(groups, key=lambda g: g["start"]):
    go.write(f"{g['start']:08x}\t{g['end']:08x}\t{len(g['members'])}\t{','.join(str(x) for x in sorted(g['members']))}\n")
# base address clusters
bases = {}
for o in ovs:
    bases.setdefault(o["base_address"], []).append(o["id"])
gb = open(os.path.join(S, "overlay_bases.tsv"), "w")
for b in sorted(bases):
    gb.write(f"{b:08x}\t{len(bases[b])}\t{','.join(str(x) for x in sorted(bases[b]))}\n")
