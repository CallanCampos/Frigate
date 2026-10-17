#!/usr/bin/env python3
"""Per-milestone blockers: functions without C (c_missing/no_file) and functions in incomplete files, by module set.

Linker veneers (ARM/Thumb interworking stubs of <=12 bytes, named func_ovNNN_*, in ov167) are excluded.
ov036 files are split into core (needed to boot the field) and deferrable.
"""
import os
import sys
from collections import defaultdict

S = sys.argv[1]
W = sys.argv[2]

OV36_DEFER = {
    "ircbattlemenu.c", "comm_entry_menu.c", "union_mine_pms.c", "scrcmd_hidden_hollow.c", "party_select_list.c",
}
OV36_M4 = {  # field interaction: NPC actions, talk, events, transitions, effects
    "fldmmdl_acmd.c", "fieldmap_event.c", "ctalkmsgwin.c", "field_fade.c", "place_name_letter.c",
    "fldeff_reflect.c", "fldeff_iaigiri.c", "fldeff_ochiba.c", "fldeff_hide.c", "fldeff_btrain.c",
    "fldeff_ripple.c", "fldeff_splash.c", "scrcmd_field.c", "scrcmd_pokemon_misc.c", "scrcmd_pokemon_fld.c",
    "sodateya.c", "pleasure_boat.c", "fes_gimmick.c", "field_fmission_info.c",
}
OV36_M5 = {"enceff.c"}
OV36_M7 = {"shortcut_menu.c"}


def ov(*ids):
    return {"ov%03d" % i for i in ids}


MILESTONES = [
    ("M1 boot+logo", {"main", "itcm"} | ov(162, 264)),
    ("M2 title+start menu", ov(162, 331)),
    ("M3 new game->overworld", ov(162, 294, 293, 280, 279, 35, 12, 27, 36, 80, 74, 75, 76, 77, 78, 112, 337)),
    ("M4 walk/NPC/transitions", ov(36, 35, 33, 12)),
    ("M5 wild battle", ov(36, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 166, 167, 168, 169, 170, 285, 286, 287, 288, 289, 297, 284, 337)),
    ("M6 save/load", ov(12, 36, 87, 331, 162)),
    ("M7 menus", ov(36, 139, 142, 165, 207, 296, 298, 299, 300, 301, 302, 303, 304, 255, 186, 140, 144)),
]


def ov36_bucket(fname):
    b = os.path.basename(fname)
    if b in OV36_DEFER:
        return "defer"
    if b in OV36_M4:
        return "M4"
    if b in OV36_M5:
        return "M5"
    if b in OV36_M7:
        return "M7"
    return "M3"


funcs = defaultdict(list)
for line in open(os.path.join(S, "funcs.tsv")):
    mod, name, addr, size, mode, f, st = line.rstrip("\n").split("\t")
    size = int(size)
    if mod == "ov167" and size <= 12 and name.startswith("func_ov167") and st == "c_missing":
        continue  # interworking veneer
    funcs[mod].append((name, size, f, st))

incomplete_files = defaultdict(set)
for line in open(os.path.join(S, "files.tsv")).readlines()[1:]:
    p = line.rstrip("\n").split("\t")
    if p[2] == "0":
        incomplete_files[p[0]].add(p[1])

nonmatch = defaultdict(int)
for line in open(os.path.join(W, "docs/nonmatching-functions.md")):
    if line.startswith("| `"):
        nonmatch[line.split("`")[1]] += 1

out = open(os.path.join(S, "milestones.txt"), "w")
seen = set()
for label, mods in MILESTONES:
    key = label.split()[0]
    nocode = defaultdict(lambda: [0, 0])
    written = defaultdict(lambda: [0, 0])
    for mod in sorted(mods):
        for name, size, f, st in funcs[mod]:
            if mod == "ov036":
                bucket = ov36_bucket(f)
                if bucket == "defer":
                    continue
                if key == "M3" and bucket != "M3":
                    continue
                if key in ("M4", "M5", "M7") and bucket != key:
                    continue
                if key == "M6" and bucket != "M3":
                    continue
            if mod in ("ov012",) and key not in ("M3",):
                # ov12 counted in M3 only
                continue
            if st in ("c_missing", "no_file"):
                nocode[(mod, f)][0] += 1
                nocode[(mod, f)][1] += size
            elif st == "c_written":
                written[(mod, f)][0] += 1
                written[(mod, f)][1] += size
    nf = sum(v[0] for v in nocode.values())
    nb = sum(v[1] for v in nocode.values())
    wf = sum(v[0] for v in written.values())
    wb = sum(v[1] for v in written.values())
    nm = sum(nonmatch.get(f, 0) for (m, f) in written)
    out.write(f"## {label}: no-C {nf} funcs / {nb} B in {len(nocode)} files; C-but-incomplete-file {wf} funcs / {wb} B in {len(written)} files ({nm} nonmatching rows)\n")
    for k in sorted(nocode, key=lambda k: -nocode[k][1]):
        out.write(f"   NOC {k[0]} {k[1]} {nocode[k][0]} {nocode[k][1]}\n")
    for k in sorted(written, key=lambda k: -written[k][1]):
        out.write(f"   INC {k[0]} {k[1]} {written[k][0]} {written[k][1]} nm={nonmatch.get(k[1], 0)}\n")
