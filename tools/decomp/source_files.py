#!/usr/bin/env python3
"""Find the boundaries of an overlay's original source files.

    source_files.py ov036                     # the embedded file names and where they are referenced
    source_files.py ov036 --profile START END # a boundary profile for every function in a range
    source_files.py ov036 --sections START END  # the data a text range refers to, per section
    source_files.py --markdown                # the tables of docs/source-files.md, for every overlay
    source_files.py main                      # the same for ARM9 main

Many functions pass their file's name to GFL_HeapAllocate or an assert, so the overlay embeds strings such as
"resort_npc.c", in the .data of that file. Between two such anchors, the linker's layout gives the boundaries: every
original file is one object, so its .text, .rodata, .data and .bss are each contiguous and in the same order. A
boundary before a function is consistent when no function before it refers to data after any data referred to from
behind it. The profile prints, for each function, the number of references that break that order if a file started
there (`order`), and the number of calls within the overlay that would cross it (`calls`, counting calls between
functions within --window functions of the boundary). Real boundaries have an order of 0, or close to it for data
shared between files, and few crossing calls.
"""
import argparse
import bisect
import re
import sys
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import ROOT  # noqa: E402

SECTION_NAMES = (".text", ".rodata", ".data", ".bss")


class Overlay:
    def __init__(self, name: str, version: str):
        self.name = name
        if name == "main":
            self.module = "main"
            config = ROOT / "config" / version / "arm9"
            image_path = ROOT / "extract" / version / "arm9" / "arm9.bin"
        else:
            self.module = f"overlay({int(name[2:])})"
            config = ROOT / "config" / version / "arm9" / "overlays" / name
            image_path = ROOT / "extract" / version / "arm9_overlays" / f"{name}.bin"
        delinks = (config / "delinks.txt").read_text()
        self.sections = {}
        for match in re.finditer(r"^\s+(\.\w+)\s+start:0x([0-9a-f]+) end:0x([0-9a-f]+) kind", delinks, re.M):
            self.sections[match.group(1)] = (int(match.group(2), 16), int(match.group(3), 16))
        self.functions = []
        for line in (config / "symbols.txt").read_text().splitlines():
            match = re.match(r"(\S+) kind:function\(\w+,size=0x([0-9a-f]+)\S*\) addr:0x([0-9a-f]+)", line)
            if match:
                self.functions.append((int(match.group(3), 16), int(match.group(2), 16), match.group(1)))
        self.functions.sort()
        self.starts = [f[0] for f in self.functions]
        self.relocs = []
        for line in (config / "relocs.txt").read_text().splitlines():
            match = re.match(r"from:0x([0-9a-f]+) kind:(\S+) to:0x([0-9a-f]+) module:(\S+)", line)
            if match:
                self.relocs.append((int(match.group(1), 16), match.group(2), int(match.group(3), 16), match.group(4)))
        image = image_path.read_bytes()
        base = self.sections[".text"][0]
        self.strings = {base + m.start(): m.group(1).decode() for m in re.finditer(rb"([a-z_0-9]+\.c)\x00", image)}
        self.files = {}
        current = None
        for line in delinks.splitlines():
            match = re.match(r"^((?:src|lib)/\S+):$", line)
            if match:
                current = match.group(1)
                continue
            match = re.match(r"\s+\.text\s+start:0x([0-9a-f]+)", line)
            if match and current:
                self.files[int(match.group(1), 16)] = current

    def index(self, address: int) -> int:
        return bisect.bisect_right(self.starts, address) - 1

    def in_text(self, address: int) -> bool:
        start, end = self.sections[".text"]
        return start <= address < end

    def section_of(self, address: int):
        for name, (start, end) in self.sections.items():
            if name != ".text" and start <= address < end:
                return name
        return None

    def data_refs(self):
        """(function index, section, target) for each reference from code to this overlay's data."""
        own = self.module
        refs = []
        for source, _, target, module in self.relocs:
            if module == own and self.in_text(source):
                section = self.section_of(target)
                if section:
                    refs.append((self.index(source), section, target))
        return refs

    def calls(self):
        own = self.module
        out = []
        for source, kind, target, module in self.relocs:
            if "call" in kind and module == own and self.in_text(source) and self.in_text(target):
                a, b = self.index(source), self.index(target)
                if a != b:
                    out.append((min(a, b), max(a, b)))
        return out

    def anchors(self):
        """function index -> embedded file names it refers to"""
        out = {}
        for source, _, target, _ in self.relocs:
            if target in self.strings and self.in_text(source):
                out.setdefault(self.index(source), set()).add(self.strings[target])
        return out


def print_anchors(overlay: Overlay):
    anchors = overlay.anchors()
    spans = {}
    for index, names in anchors.items():
        for name in names:
            low, high = spans.get(name, (index, index))
            spans[name] = (min(low, index), max(high, index))
    referenced = set(spans)
    for address, name in sorted(overlay.strings.items()):
        if name not in referenced:
            print(f"{name:32s} string at {address:#010x}, not referenced")
    for name, (low, high) in sorted(spans.items(), key=lambda item: item[1][0]):
        string = min(a for a, n in overlay.strings.items() if n == name)
        print(f"{name:32s} string at {string:#010x}, referenced from {overlay.functions[low][0]:#010x}"
              f"..{overlay.functions[high][0]:#010x}")


def print_profile(overlay: Overlay, start: int, end: int, window: int):
    refs = overlay.data_refs()
    by_section = {}
    for index, section, target in refs:
        by_section.setdefault(section, []).append((index, target))
    for section in by_section:
        by_section[section].sort()
    calls = overlay.calls()
    anchors = overlay.anchors()
    refs_of = {}
    for index, section, target in refs:
        refs_of.setdefault(index, []).append((section, target))

    def order_cost(i: int) -> int:
        cost = 0
        for items in by_section.values():
            before = [t for f, t in items if f < i]
            after = [t for f, t in items if f >= i]
            if before and after:
                low, high = min(after), max(before)
                cost += sum(1 for t in before if t >= low) + sum(1 for t in after if t <= high)
        return cost

    def call_cost(i: int) -> int:
        return sum(1 for a, b in calls if a < i <= b and a >= i - window and b < i + window)

    print(f"{'order':>5} {'calls':>5} {'address':8} {'function':40} {'anchor':24} {'current file':32} data")
    for i, (address, _, name) in enumerate(overlay.functions):
        if not start <= address < end:
            continue
        data = " ".join(f"{s[1:3]}:{t:x}" for s, t in sorted(refs_of.get(i, []), key=lambda r: r[1])[:5])
        anchor = ",".join(sorted(anchors.get(i, [])))
        current = overlay.files.get(address, "").split("/")[-1]
        print(f"{order_cost(i):5d} {call_cost(i):5d} {address:08x} {name[:40]:40} {anchor[:24]:24} {current[:32]:32} {data}")


def print_sections(overlay: Overlay, start: int, end: int):
    first, last = overlay.index(start), overlay.index(end - 1)
    targets = {}
    for index, section, target in overlay.data_refs():
        if first <= index <= last:
            targets.setdefault(section, []).append(target)
    print(f".text {start:#010x}..{end:#010x}")
    for section in SECTION_NAMES[1:]:
        if section in targets:
            print(f"{section} refers to {min(targets[section]):#010x}..{max(targets[section]):#010x}"
                  f" ({len(targets[section])} references)")


def print_markdown(version: str):
    configs = (ROOT / "config" / version / "arm9" / "overlays").glob("ov*/delinks.txt")
    for delinks in sorted(configs, key=lambda path: int(path.parent.name[2:])):
        name = delinks.parent.name
        text = delinks.read_text()
        if "\nsrc/" not in text:
            continue
        try:
            overlay = Overlay(name, version)
        except KeyError:
            continue
        strings = {}
        for address, string in sorted(overlay.strings.items()):
            strings.setdefault(string, address)
        files = []
        for entry in text.split("\n\n"):
            lines = entry.strip().splitlines()
            match = re.search(r"\.text\s+start:0x([0-9a-f]+) end:0x([0-9a-f]+)", entry)
            if lines and lines[0].startswith(("src/", "lib/")) and match:
                path = lines[0].rstrip(":")
                files.append((int(match.group(1), 16), int(match.group(2), 16), path.split("/")[-1],
                              "complete" in entry))
        files.sort()

        def count(start, end):
            return sum(1 for address, _, _ in overlay.functions if start <= address < end)

        inside = sum(count(start, end) for start, end, _, _ in files)
        unused = sorted(string for string in strings if string not in {file[2] for file in files})
        print(f"### Overlay {int(overlay.name[2:])}\n")
        line = f"{inside} of {len(overlay.functions)} functions are in source files."
        if unused:
            line += " Embedded names without a file yet: " + ", ".join(f"`{string}`" for string in unused) + "."
        print(line + "\n")
        print("| File | `.text` (Black 2) | Functions | Status | Name |")
        print("| --- | --- | --- | --- | --- |")
        for start, end, file, complete in files:
            source = f"string at `{strings[file]:#010x}`" if file in strings else "descriptive"
            status = "complete" if complete else "partial"
            print(f"| `{file}` | `{start:#010x}`–`{end:#010x}` | {count(start, end)} | {status} | {source} |")
        print()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("overlay", nargs="?", help="for example ov036, or main")
    parser.add_argument("--version", default="b2_us")
    parser.add_argument("--profile", nargs=2, metavar=("START", "END"))
    parser.add_argument("--sections", nargs=2, metavar=("START", "END"))
    parser.add_argument("--window", type=int, default=32)
    parser.add_argument("--markdown", action="store_true")
    args = parser.parse_args()
    if args.markdown:
        print_markdown(args.version)
        return
    if not args.overlay:
        parser.error("an overlay is required")
    overlay = Overlay(args.overlay, args.version)
    if args.profile:
        print_profile(overlay, int(args.profile[0], 16), int(args.profile[1], 16), args.window)
    elif args.sections:
        print_sections(overlay, int(args.sections[0], 16), int(args.sections[1], 16))
    else:
        print_anchors(overlay)


if __name__ == "__main__":
    sys.exit(main())
