"""Place an address of the primary version in another version, through the version map.

An address inside a paired function keeps its offset in that function. Any other address is placed from the anchors of
its section, the addresses whose place in the other version is known: the section's edges and the starts and ends of
the paired functions. An address between two anchors is placed only when both agree on its place, that is when the
bytes from the one before it to the one after it (padding, filler, data) have the same size in both versions. Otherwise
it raises Unplaced, naming both candidates, and the caller asks for the address instead of guessing.

version_map.py's symbol pairs are not anchors: it pairs symbols through the relocations that read them, and a
relocation into the middle of an object pairs a bogus symbol of one version with a real one of the other (overlay 142's
removed 0x021a1614 with White 2's sPocketTabPos, 0x38 past its place).

add_source_file.py places new files' ranges with it, and config_fixes.py every fix and moved boundary.
"""
import csv
import re
from bisect import bisect_left, bisect_right
from pathlib import Path

from tools.decomp.dsd_config import parse_sections

PRIMARY = "b2_us"
OTHERS = ["w2_us"]

FUNCTION_RE = re.compile(r"^\S+ kind:function\(\w+,size=(0x[0-9a-f]+)[^ ]*\) addr:(0x[0-9a-f]+)")


class Unplaced(Exception):
    """An address that the version map cannot place in another version."""


def load_pairs(path: Path, other: str) -> dict[str, dict[int, int]]:
    """Returns primary function address -> other function address by module, from version_map.py's pairs."""
    pairs: dict[str, dict[int, int]] = {}
    with path.open() as f:
        for row in csv.DictReader(f, delimiter="\t"):
            pairs.setdefault(row["module"], {})[int(row[f"{PRIMARY}_addr"], 16)] = int(row[f"{other}_addr"], 16)
    return pairs


def function_ends(config: Path) -> dict[int, int]:
    """Returns the end address of every function of a module's symbols.txt, by start address."""
    ends = {}
    path = config / "symbols.txt"
    if path.exists():
        for line in path.read_text().splitlines():
            match = FUNCTION_RE.match(line)
            if match:
                start = int(match.group(2), 16)
                ends[start] = start + int(match.group(1), 16)
    return ends


class ModulePlacer:
    """Places a module's addresses in one other version."""

    def __init__(self, label: str, sections, other_sections, functions: dict[int, int], other_functions: dict[int, int],
                 function_pairs: dict[int, int]):
        self.label = label
        self.sections = sections
        self.other_sections = other_sections
        self.functions = sorted(functions.items())
        self.function_starts = [start for start, _ in self.functions]
        self.function_pairs = function_pairs
        anchors = {}
        for primary, other in function_pairs.items():
            if primary in functions and other in other_functions:
                anchors[functions[primary]] = other_functions[other]
        # Where a function ends at the next one's start the start wins, which keeps a file that ends there next to the
        # one that begins there when the padding between them differs
        anchors.update(function_pairs)
        self.anchors = anchors
        self.anchor_addresses = sorted(anchors)

    @classmethod
    def from_configs(cls, label, primary_config: Path, other_config: Path, function_pairs):
        return cls(label, parse_sections(primary_config / "delinks.txt"), parse_sections(other_config / "delinks.txt"),
                   function_ends(primary_config), function_ends(other_config), function_pairs)

    def function_at(self, address: int) -> tuple[int, int] | None:
        """Returns the (start, end) of the function containing an address."""
        index = bisect_right(self.function_starts, address) - 1
        if index >= 0 and address < self.functions[index][1]:
            return self.functions[index]
        return None

    def section_of(self, address: int) -> str | None:
        """Returns the section containing an address, or else the one that ends at it."""
        inside = [name for name, (start, end) in self.sections.items() if start <= address < end]
        if inside:
            return inside[0]
        return next((name for name, (_, end) in self.sections.items() if end == address), None)

    def place(self, address: int, section: str | None = None) -> int:
        """Returns the address in the other version, or raises Unplaced. A section's end is an address too, so a
        boundary at it names its section, which tells it from the next section's start."""
        name = section or self.section_of(address)
        if name is None:
            raise Unplaced(f"{self.label} {address:#010x} is in no section")
        if name not in self.sections or name not in self.other_sections:
            raise Unplaced(f"{self.label} has no {name} in both versions")
        start, end = self.sections[name]
        other_start, other_end = self.other_sections[name]
        if not start <= address <= end:
            raise Unplaced(f"{self.label} {address:#010x} is outside {name} ({start:#010x}..{end:#010x})")
        if address == start:
            return other_start
        if address == end:
            return other_end
        function = self.function_at(address)
        if function is not None and function[0] in self.function_pairs:
            return self.function_pairs[function[0]] + address - function[0]
        if address in self.anchors:
            return self.anchors[address]
        # The nearest anchors in the section on either side, its edges if there are none
        index = bisect_left(self.anchor_addresses, address)
        before = self.anchor_addresses[index - 1] if index > 0 else None
        after = self.anchor_addresses[index] if index < len(self.anchor_addresses) else None
        if before is None or before <= start:
            before, other_before = start, other_start
        else:
            other_before = self.anchors[before]
        if after is None or after >= end:
            after, other_after = end, other_end
        else:
            other_after = self.anchors[after]
        from_before = other_before + address - before
        from_after = other_after - (after - address)
        if from_before != from_after:
            unpaired = f" in the unpaired function at {function[0]:#010x}" if function is not None else ""
            raise Unplaced(
                f"{self.label} {name} {address:#010x}{unpaired} is between two anchors that disagree on its place: "
                f"{from_before:#010x} after {before:#010x}, {from_after:#010x} before {after:#010x}"
            )
        return from_before
