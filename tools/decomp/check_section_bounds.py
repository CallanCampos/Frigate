#!/usr/bin/env python3
"""Check every ov036 delinks section range against the symbols dsd found, in both versions.

Reports objects that start inside a file's section range but end past its end, which means the boundary is in the
middle of an object, and objects that start before a file's range but are the last symbol inside it.

    python3 tools/decomp/check_section_bounds.py

Unknown object sizes are inferred up to the next symbol or the containing module section's end. This checks
configured boundaries against symbol extents; it does not prove source-file ownership or matching C data.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODULE = "overlays/ov036"

SYM = re.compile(r"^(\S+) kind:(\w+)(\([^)]*\))? addr:(0x[0-9a-f]+)")
SECTION = re.compile(r"^\s+(\.\w+)\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)")


def module_sections(version):
    """Only the module header defines its section extents; file ranges can end earlier."""
    sections = {}
    for line in (ROOT / "config" / version / "arm9" / MODULE / "delinks.txt").read_text().splitlines():
        if re.match(r"^src/\S+\.c:", line):
            break
        match = SECTION.match(line)
        if match:
            sections[match.group(1)] = (int(match.group(2), 16), int(match.group(3), 16))
    return sections


def ranges(version):
    out = []
    cur = None
    for line in (ROOT / "config" / version / "arm9" / MODULE / "delinks.txt").read_text().splitlines():
        m = re.match(r"^(src/\S+\.c):", line)
        if m:
            cur = m.group(1)
            continue
        m = SECTION.match(line)
        if m and cur:
            out.append((cur, m.group(1), int(m.group(2), 16), int(m.group(3), 16)))
    return out


def objects(version, sections):
    """Every symbol, with its declared size or an inferred extent within its module section."""
    starts = []
    for line in (ROOT / "config" / version / "arm9" / MODULE / "symbols.txt").read_text().splitlines():
        m = SYM.match(line)
        if not m:
            continue
        size = re.search(r"size=(0x[0-9a-f]+)", line)
        starts.append((int(m.group(4), 16), int(size.group(1), 16) if size else None))
    starts.sort(key=lambda entry: entry[0])
    out = []
    for i, (start, size) in enumerate(starts):
        if size is None:
            nxt = starts[i + 1][0] if i + 1 < len(starts) else max(end for _, end in sections.values())
            for section_start, section_end in sections.values():
                if section_start <= start < section_end:
                    nxt = min(nxt, section_end)
                    break
            size = max(nxt - start, 0)
        out.append((start, start + size))
    return out


def problems(version):
    """Returns diagnostics for file boundaries that cut through a symbol's extent."""
    diagnostics = []
    objs = objects(version, module_sections(version))
    for source, section, start, end in ranges(version):
        if end <= start:
            continue
        # Any object that meets the range must be wholly inside it
        for o_start, o_end in objs:
            if o_start >= end or o_end <= start:
                continue
            if o_start < start:
                diagnostics.append(f"{version} {source} {section} 0x{start:08x}-0x{end:08x}: "
                                   f"starts inside the object at 0x{o_start:08x} (ends 0x{o_end:08x})")
            elif o_end > end:
                diagnostics.append(f"{version} {source} {section} 0x{start:08x}-0x{end:08x}: "
                                   f"the object at 0x{o_start:08x} ends 0x{o_end:08x}, past the end")
    return diagnostics


def main():
    diagnostics = [problem for version in ("b2_us", "w2_us") for problem in problems(version)]
    for diagnostic in diagnostics:
        print(diagnostic)
    print(f"{len(diagnostics)} problem(s)")
    return 1 if diagnostics else 0


if __name__ == "__main__":
    sys.exit(main())
