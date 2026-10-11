#!/usr/bin/env python3
"""Add a source file to a module's delinks.txt in every version.

Section ranges are given for the primary version. The ranges in the other versions are derived from the version map:
code ranges through the paired functions, other sections by their offset from the section start, which requires the
section to have the same size in both versions.
"""
import argparse
import csv
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import ROOT, config_lock, parse_sections  # noqa: E402
from tools.decomp.mark_complete import externally_used_statics  # noqa: E402

PRIMARY = "b2_us"
OTHERS = ["w2_us"]


def load_function_pairs(path: Path, module: str, other: str) -> dict[int, tuple[int, int]]:
    """Returns primary function address -> (other address, primary function size) for a module."""
    pairs = {}
    with path.open() as f:
        for row in csv.DictReader(f, delimiter="\t"):
            if row["module"] == module:
                pairs[int(row[f"{PRIMARY}_addr"], 16)] = int(row[f"{other}_addr"], 16)
    return pairs


def function_ends(config: Path) -> dict[int, int]:
    """Returns the end address of every function in a module, by start address."""
    ends = {}
    for line in (config / "symbols.txt").read_text().splitlines():
        match = re.match(r"^\S+ kind:function\(\w+,size=(0x[0-9a-f]+)[^ ]*\) addr:(0x[0-9a-f]+)", line)
        if match:
            start = int(match.group(2), 16)
            ends[start] = start + int(match.group(1), 16)
    return ends


def map_range(name, start, end, module, other, primary_config, other_config, function_pairs):
    if name == ".text":
        other_start = function_pairs.get(start)
        # The end is either the end of the last function, or the start of the next one
        starts = sorted(a for a in function_pairs if start <= a < end)
        if other_start is None or not starts:
            sys.exit(f"{name} {start:#010x}..{end:#010x} does not start at a function in {module}")
        last = starts[-1]
        last_end = function_ends(primary_config)[last]
        other_last = function_pairs[last]
        other_last_end = function_ends(other_config)[other_last]
        if end == last_end:
            return other_start, other_last_end
        if end > last_end:
            # Padding after the last function, which has the same size in both versions
            return other_start, other_last_end + (end - last_end)
        sys.exit(f"{name} end {end:#010x} is inside the function at {last:#010x}")

    primary_sections = parse_sections(primary_config / "delinks.txt")
    other_sections = parse_sections(other_config / "delinks.txt")
    primary_start, primary_end = primary_sections[name]
    other_start, other_end = other_sections[name]
    if primary_end - primary_start != other_end - other_start:
        sys.exit(f"{name} has different sizes in {PRIMARY} and {other}, add its range to {other} by hand")
    delta = other_start - primary_start
    return start + delta, end + delta


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", help="path of the source file, e.g. src/ov279/new_game.c")
    parser.add_argument("module", help="module path in the config, e.g. overlays/ov279 or . for ARM9 main")
    parser.add_argument("sections", nargs="+", help="section ranges in the primary version, e.g. .text:0x021e8be0-0x021e8c74")
    parser.add_argument("--incomplete", action="store_true", help="do not mark the file as complete")
    parser.add_argument("--map", type=Path, default=ROOT / "build" / "version_map.tsv")
    args = parser.parse_args()

    ranges = []
    for section in args.sections:
        match = re.match(r"^(\.\w+):(0x[0-9a-fA-F]+)-(0x[0-9a-fA-F]+)$", section)
        if not match:
            sys.exit(f"invalid section range {section!r}, expected e.g. .text:0x021e8be0-0x021e8c74")
        ranges.append((match.group(1), int(match.group(2), 16), int(match.group(3), 16)))

    entries = {PRIMARY: ranges}
    primary_config = ROOT / "config" / PRIMARY / "arm9" / args.module
    for other in OTHERS:
        other_config = ROOT / "config" / other / "arm9" / args.module
        function_pairs = load_function_pairs(args.map, args.module, other)
        entries[other] = [
            (name, *map_range(name, start, end, args.module, other, primary_config, other_config, function_pairs))
            for name, start, end in ranges
        ]

    for version, version_ranges in entries.items():
        delinks = ROOT / "config" / version / "arm9" / args.module / "delinks.txt"
        text = delinks.read_text()
        if f"\n{args.source}:\n" in text:
            sys.exit(f"{args.source} is already in {delinks}")
        lines = [f"{args.source}:"]
        lines += [f"    {name:<11} start:{start:#010x} end:{end:#010x}" for name, start, end in version_ranges]
        delinks.write_text(text.rstrip("\n") + "\n\n" + "\n".join(lines) + "\n")
        print(f"{version}: " + ", ".join(f"{n} {s:#010x}..{e:#010x}" for n, s, e in version_ranges))

    if not args.incomplete:
        # A static function that another module calls or takes the address of links to address 0, so such a file
        # stays incomplete, as mark_complete.py would leave it
        statics = set()
        for version in entries:
            statics.update(externally_used_statics(args.source, ROOT / "config" / version / "arm9"))
        if statics:
            print(f"left incomplete, static but used by other modules: {', '.join(sorted(statics))}")
            return
        for version in entries:
            delinks = ROOT / "config" / version / "arm9" / args.module / "delinks.txt"
            text = delinks.read_text()
            delinks.write_text(text.replace(f"\n{args.source}:\n", f"\n{args.source}:\n    complete\n"))


if __name__ == "__main__":
    with config_lock():
        main()
