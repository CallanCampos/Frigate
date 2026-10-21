#!/usr/bin/env python3
"""Add a source file to a module's delinks.txt in every version.

Section ranges are given for the primary version. The ranges in the other versions are derived from the version map:
each end of a range is placed from the anchors of its section (its edges, the paired data, .bss and label symbols in it
and, in .text, the paired functions' starts and ends), and an end between two anchors only when both agree on its place.
So a range may start or end where no function or symbol does, among padding, filler or data in .text. An end that the
map cannot place is given with --other-range, and every range is checked against the module's sections and the files
already registered in each version before any version is written.
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


def load_symbol_pairs(path: Path, module: str, other: str) -> dict[int, int]:
    """Returns primary address -> other address of a module's paired data, .bss and label symbols, which version_map.py
    pairs through the relocations that read them."""
    pairs = {}
    if not path.exists():
        return pairs
    with path.open() as f:
        for row in csv.DictReader(f, delimiter="\t"):
            if row["module"] == module:
                pairs[int(row[f"{PRIMARY}_addr"], 16)] = int(row[f"{other}_addr"], 16)
    return pairs


def section_anchors(name, primary_config, other_config, function_pairs, symbol_pairs) -> dict[int, int]:
    """Returns the addresses of a section whose place in the other version is known, primary -> other: the section's
    edges, the paired symbols in it and, in .text, the start and end of every paired function. Where a function ends at
    the next one's start, the start wins, which keeps a file that ends there next to the one that begins there when the
    padding between them differs."""
    primary_start, primary_end = parse_sections(primary_config / "delinks.txt")[name]
    other_start, other_end = parse_sections(other_config / "delinks.txt")[name]
    anchors = {}
    if name == ".text":
        primary_ends = function_ends(primary_config)
        other_ends = function_ends(other_config)
        for primary, other in function_pairs.items():
            if primary in primary_ends and other in other_ends:
                anchors[primary_ends[primary]] = other_ends[other]
    anchors.update((a, b) for a, b in symbol_pairs.items() if primary_start <= a <= primary_end)
    if name == ".text":
        anchors.update(function_pairs)
    anchors[primary_start] = other_start
    anchors[primary_end] = other_end
    return anchors


def map_address(name, address, anchors, functions, module) -> int:
    """Maps an address of a section to the other version: an anchor maps directly, and an address between two anchors
    only when both give it the same place, that is when the bytes from the one before it to the one after it (padding,
    filler, or data in .text, as ARM9 main's secure area and version strings) have the same size in both versions."""
    for start, end in functions:
        if start < address < end:
            sys.exit(f"{name} address {address:#010x} is inside the function at {start:#010x} in {module}")
    if address in anchors:
        return anchors[address]
    before = max((a for a in anchors if a < address), default=None)
    after = min((a for a in anchors if a > address), default=None)
    if before is None or after is None:
        sys.exit(f"{name} address {address:#010x} is outside {module}'s {name}")
    from_before = anchors[before] + (address - before)
    from_after = anchors[after] - (after - address)
    if from_before != from_after:
        raise Unplaced(
            f"{name} address {address:#010x} of {module} is between two anchors that disagree on its place: "
            f"{from_before:#010x} after {before:#010x}, {from_after:#010x} before {after:#010x}"
        )
    return from_before


class Unplaced(Exception):
    """An address that the version map cannot place in another version."""


def map_range(name, start, end, module, other, primary_config, other_config, function_pairs, symbol_pairs=None):
    """Maps a primary section range to the other version, or raises Unplaced."""
    anchors = section_anchors(name, primary_config, other_config, function_pairs, symbol_pairs or {})
    functions = list(function_ends(primary_config).items()) if name == ".text" else []
    other_start = map_address(name, start, anchors, functions, module)
    if name == ".text" and end not in anchors:
        # Padding after the range's last paired function, which has the same size in both versions
        starts = [a for a in function_pairs if start <= a < end]
        if starts:
            last = max(starts)
            last_end = function_ends(primary_config)[last]
            if end < last_end:
                sys.exit(f"{name} end {end:#010x} is inside the function at {last:#010x}")
            return other_start, function_ends(other_config)[function_pairs[last]] + (end - last_end)
    return other_start, map_address(name, end, anchors, functions, module)


def file_ranges(delinks: Path) -> list[tuple[str, str, int, int]]:
    """Returns (file, section, start, end) of every file range in a delinks.txt."""
    ranges = []
    owner = None
    for line in delinks.read_text().splitlines():
        if line and not line[0].isspace() and line.endswith(":"):
            owner = line[:-1]
        match = re.match(r"^\s+(\.\w+)\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)$", line)
        if match and owner:
            ranges.append((owner, match.group(1), int(match.group(2), 16), int(match.group(3), 16)))
    return ranges


def check_range(delinks: Path, source, name, start, end):
    """Exits if a range is empty, leaves its module section, or overlaps a file already registered."""
    sections = parse_sections(delinks)
    if name not in sections:
        sys.exit(f"{delinks} has no {name} section")
    section_start, section_end = sections[name]
    if not section_start <= start < end <= section_end:
        sys.exit(f"{name} {start:#010x}..{end:#010x} is empty or outside {section_start:#010x}..{section_end:#010x} "
                 f"in {delinks}")
    for owner, owner_section, owner_start, owner_end in file_ranges(delinks):
        if owner != source and owner_section == name and start < owner_end and owner_start < end:
            sys.exit(f"{name} {start:#010x}..{end:#010x} overlaps {owner} ({owner_start:#010x}..{owner_end:#010x}) "
                     f"in {delinks}")


def parse_range(text):
    match = re.match(r"^(\.\w+):(0x[0-9a-fA-F]+)-(0x[0-9a-fA-F]+)$", text)
    if not match:
        sys.exit(f"invalid section range {text!r}, expected e.g. .text:0x021e8be0-0x021e8c74")
    return match.group(1), int(match.group(2), 16), int(match.group(3), 16)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", help="path of the source file, e.g. src/ov279/new_game.c")
    parser.add_argument("module", help="module path in the config, e.g. overlays/ov279 or . for ARM9 main")
    parser.add_argument("sections", nargs="+", help="section ranges in the primary version, e.g. .text:0x021e8be0-0x021e8c74")
    parser.add_argument("--incomplete", action="store_true", help="do not mark the file as complete")
    parser.add_argument("--map", type=Path, default=ROOT / "build" / "version_map.tsv")
    parser.add_argument("--symbols-map", type=Path, default=ROOT / "build" / "version_map_symbols.tsv")
    parser.add_argument(
        "--other-range",
        action="append",
        default=[],
        metavar="VERSION:RANGE",
        help="a section's range in another version, for one the version map cannot place, e.g. "
        "w2_us:.text:0x021e8c00-0x021e8c94",
    )
    args = parser.parse_args()

    ranges = [parse_range(section) for section in args.sections]
    given = {}
    for text in args.other_range:
        version, _, section = text.partition(":")
        if version not in OTHERS:
            sys.exit(f"--other-range {text!r}: the version must be one of {', '.join(OTHERS)}")
        name, start, end = parse_range(section)
        given[version, name] = (start, end)

    entries = {PRIMARY: ranges}
    primary_config = ROOT / "config" / PRIMARY / "arm9" / args.module
    unplaced = []
    for other in OTHERS:
        other_config = ROOT / "config" / other / "arm9" / args.module
        function_pairs = load_function_pairs(args.map, args.module, other)
        symbol_pairs = load_symbol_pairs(args.symbols_map, args.module, other)
        entries[other] = []
        for name, start, end in ranges:
            if (other, name) in given:
                entries[other].append((name, *given[other, name]))
                continue
            try:
                mapped = map_range(name, start, end, args.module, other, primary_config, other_config,
                                   function_pairs, symbol_pairs)
            except Unplaced as error:
                unplaced.append(f"{other}: {error}; pass --other-range {other}:{name}:0x...-0x...")
                continue
            entries[other].append((name, *mapped))
    if unplaced:
        sys.exit("\n".join(unplaced))

    # Every version is checked before any is written, so that a failure leaves them all as they were
    for version, version_ranges in entries.items():
        delinks = ROOT / "config" / version / "arm9" / args.module / "delinks.txt"
        if f"\n{args.source}:\n" in delinks.read_text():
            sys.exit(f"{args.source} is already in {delinks}")
        for name, start, end in version_ranges:
            check_range(delinks, args.source, name, start, end)

    for version, version_ranges in entries.items():
        delinks = ROOT / "config" / version / "arm9" / args.module / "delinks.txt"
        text = delinks.read_text()
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
