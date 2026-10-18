"""How many overlays have data and .bss symbols, and how many symbols they have.

    datasyms.py REPO

Counts, over Black 2's overlays, the symbols in each overlay's .data and its .bss: the writable statics that the
host build puts in per-overlay sections, so that loading an overlay sets them back as the DS does
(port/hal/fs/ovsec.h). Research of 2026-10-16 (port/docs/research/engine-boot.md).
"""
import os, re, sys
W = sys.argv[1]
base = os.path.join(W, "config/b2_us/arm9/overlays")
tot_data = tot_bss = ov_data = ov_bss = 0
strs = 0
for ov in sorted(os.listdir(base)):
    secs = {}
    for line in open(os.path.join(base, ov, "delinks.txt")):
        if not line.startswith(" "):
            break
        m = re.match(r"\s+(\.\w+)\s+start:0x([0-9a-f]+)\s+end:0x([0-9a-f]+)", line)
        if m:
            secs[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
    nd = nb = 0
    for line in open(os.path.join(base, ov, "symbols.txt")):
        m = re.match(r"(\S+) kind:(data|bss)\S* addr:0x([0-9a-f]+)", line)
        if not m:
            continue
        a = int(m.group(3), 16)
        if m.group(2) == "bss":
            nb += 1
        elif ".data" in secs and secs[".data"][0] <= a < secs[".data"][1]:
            nd += 1
    tot_data += nd; tot_bss += nb
    ov_data += nd > 0; ov_bss += nb > 0
print("overlays with .data symbols", ov_data, "symbols", tot_data, "; with .bss symbols", ov_bss, "symbols", tot_bss)
