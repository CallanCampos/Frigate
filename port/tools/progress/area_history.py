"""The decomp's code with no C yet (tier c), day by day, in each area of the game that the port's milestones wait on.

    python3 area_history.py OUT/history

Reads the functions.tsv of each day that history.py measured (OUT/history/<date>_<sha>/: progress.py's tiers of
Black 2's functions, from config/b2_us's symbols.txt and delinks.txt at that day's commit) and prints a Markdown table
of tier (c)'s code bytes per day and area: ARM9 main, the ITCM, the DSi-only LTD autoload, the overlays that the
port's milestones need on their own (11, Nintendo Wi-Fi Connection; 12, the field's movement models and event
scripts; 36, the field engine; 167 and 168, the battle engine and its view; 182, the Wi-Fi settings and Download
Play; 189, the GTS and the Battle Subway's Wi-Fi), and every other overlay together. run_all.sh writes it to
area_history.md, for the pace of port/docs/research/decomp-progress.md ("Pace").
"""
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
