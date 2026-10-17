"""Tier-c (no C) and tier-b code bytes per area for each day measured by history.py.
Usage: python3 area_history.py out/history"""
import csv, sys
from collections import defaultdict
from pathlib import Path
base = Path(sys.argv[1])
def area(mod):
    if mod in ("main", "itcm", "dtcm", "ltd_autoload_0") or mod.startswith("autoload"):
        return mod
    if mod in ("ov011", "ov036", "ov182", "ov167", "ov168", "ov189", "ov012"):
        return mod
    return "other overlays"
areas = ["main", "itcm", "ltd_autoload_0", "ov011", "ov012", "ov036", "ov167", "ov168", "ov182", "ov189", "other overlays"]
print("| date | " + " | ".join(areas) + " |")
print("|---" * (len(areas) + 1) + "|")
for d in sorted(p for p in base.iterdir() if p.is_dir()):
    c = defaultdict(int); b = defaultdict(int)
    for r in csv.DictReader(open(d / "functions.tsv"), delimiter="\t"):
        if r["tier"] == "c":
            c[area(r["module"])] += int(r["size"])
        elif r["tier"] == "b":
            b[area(r["module"])] += int(r["size"])
    print(f"| {d.name[:10]} | " + " | ".join(f"{c[a]:,}" for a in areas) + " |")
