#!/usr/bin/env python3
"""Runs progress.py on the last first-parent commit of each day and tabulates the tiers over time.

Usage: python3 history.py --root <repo> [--version b2_us] [--since 2026-09-27] --out out/history
Writes <out>/<date>_<sha>/ (progress.py output) and <out>/history.tsv.
"""
import argparse
import csv
import json
import subprocess
import sys
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent


def headline(d):
    c = Counter()
    b = Counter()
    with open(d / "functions.tsv") as fh:
        for r in csv.DictReader(fh, delimiter="\t"):
            sub = r["sub"]
            k = {"b_order": "b", "b_uncertain": "b"}.get(sub, sub)
            c[k] += 1
            b[k] += int(r["size"])
            if r["tier"] in "abc" and r["tier"] != "":
                pass
    files = list(csv.DictReader(open(d / "files.tsv"), delimiter="\t"))
    out = {
        "entries": len(files),
        "complete_files": sum(f["complete"] == "True" for f in files),
        "incomplete_with_src": sum(f["complete"] == "False" and f["has_src"] == "True" for f in files),
        "entries_no_src": sum(f["has_src"] == "False" for f in files),
    }
    for k in ("a", "b", "c_gap", "c_nosrc", "c_partial", "veneer"):
        out[f"fn_{k}"] = c[k]
        out[f"bytes_{k}"] = b[k]
    out["fn_c"] = c["c_gap"] + c["c_nosrc"] + c["c_partial"]
    out["bytes_c"] = b["c_gap"] + b["c_nosrc"] + b["c_partial"]
    data = json.load(open(d / "data.json"))
    for t in ("a", "b_file", "c_gap", "c_nosrc"):
        out[f"data_{t}"] = sum(x["bytes"] for x in data if x["tier"] == t)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True)
    ap.add_argument("--version", default="b2_us")
    ap.add_argument("--since", default="2000-01-01")
    ap.add_argument("--rev", default="HEAD")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    log = subprocess.run(["git", "-C", args.root, "log", "--first-parent", "--format=%H %cI", args.rev],
                         capture_output=True, text=True, check=True).stdout.split("\n")
    seen = {}
    for line in log:
        if not line:
            continue
        sha, date = line.split()
        day = date[:10]
        if day not in seen and day >= args.since:
            seen[day] = (sha, date)
    rows = []
    for day in sorted(seen):
        sha, date = seen[day]
        d = out / f"{day}_{sha[:9]}"
        if not (d / "functions.tsv").exists():
            subprocess.run([sys.executable, str(HERE / "progress.py"), "--root", args.root, "--version", args.version,
                            "--rev", sha, "--out", str(d)], check=True)
        h = headline(d)
        h = {"date": date, "sha": sha[:9], **h}
        rows.append(h)
        print(day, sha[:9], h["complete_files"], h["bytes_a"], h["bytes_b"], h["bytes_c"], flush=True)
    with open(out / "history.tsv", "w") as fh:
        cols = list(rows[0].keys())
        fh.write("\t".join(cols) + "\n")
        for r in rows:
            fh.write("\t".join(str(r[c]) for c in cols) + "\n")


if __name__ == "__main__":
    main()
