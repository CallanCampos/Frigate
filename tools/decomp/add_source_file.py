#!/usr/bin/env python3
"""Add a source file to a module's delinks.txt in every version.

Section ranges are given for the primary version. The ranges in the other versions are placed through the version map
by version_addresses.py: each end from its section's edges and the paired functions' starts and ends, and an end
between two of them only when both agree on its place. So a range may start or end where no function does, among
padding, filler or data. An end that the map cannot place stops the tool with both candidates; give that range with
--other-range. Every range is checked against the module's sections and the files already registered in each version
before any version is written.
"""
import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import ROOT, config_lock, parse_sections  # noqa: E402
from tools.decomp.mark_complete import externally_used_statics  # noqa: E402
from tools.decomp.version_addresses import OTHERS, PRIMARY, ModulePlacer, Unplaced, load_pairs  # noqa: E402


def map_range(placer: ModulePlacer, name, start, end) -> tuple[int, int]:
    """Places a primary section range in the other version, or raises Unplaced."""
    for address in (start, end):
        function = placer.function_at(address) if name == ".text" else None
        if function is not None and function[0] != address:
            sys.exit(f"{name} {address:#010x} is inside the function at {function[0]:#010x} in {placer.label}")
    other_start = placer.place(start, name)
    if name == ".text" and end not in placer.anchors and end != placer.sections[name][1]:
        # Padding after the range's last paired function, which has the same size in both versions
        starts = [a for a in placer.function_pairs if start <= a < end]
        last_end = dict(placer.functions).get(max(starts)) if starts else None
        if last_end in placer.anchors:
            return other_start, placer.anchors[last_end] + (end - last_end)
    return other_start, placer.place(end, name)


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
        function_pairs = load_pairs(args.map, other).get(args.module, {})
        placer = ModulePlacer.from_configs(args.module, primary_config, other_config, function_pairs)
        entries[other] = []
        for name, start, end in ranges:
            if (other, name) in given:
                entries[other].append((name, *given[other, name]))
                continue
            if name not in placer.sections:
                sys.exit(f"{args.module} has no {name} section")
            try:
                mapped = map_range(placer, name, start, end)
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
