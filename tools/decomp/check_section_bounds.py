#!/usr/bin/env python3
"""Check every ov036 delinks section range against the symbols dsd found, in both versions.

Reports objects that start inside a file's section range but end past its end, which means the boundary is in the
middle of an object, and objects that start before a file's range but are the last symbol inside it.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODULE = "overlays/ov036"

SYM = re.compile(r"^(\S+) kind:(\w+)(\([^)]*\))? addr:(0x[0-9a-f]+)")


def ranges(version):
    out = []
    cur = None
    for line in (ROOT / "config" / version / "arm9" / MODULE / "delinks.txt").read_text().splitlines():
        m = re.match(r"^(src/\S+\.c):", line)
        if m:
            cur = m.group(1)
            continue
        m = re.match(r"^\s+(\.\w+)\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)", line)
        if m and cur:
            out.append((cur, m.group(1), int(m.group(2), 16), int(m.group(3), 16)))
    return out


def objects(version, section_ends):
    """Every data symbol in a version, with the size dsd gave it or the gap to the next one."""
    starts = []
    for line in (ROOT / "config" / version / "arm9" / MODULE / "symbols.txt").read_text().splitlines():
        m = SYM.match(line)
        if not m:
            continue
        size = re.search(r"size=(0x[0-9a-f]+)", line)
        starts.append((int(m.group(4), 16), int(size.group(1), 16) if size else None))
    starts.sort()
    out = []
    for i, (start, size) in enumerate(starts):
        if size is None:
            nxt = starts[i + 1][0] if i + 1 < len(starts) else max(section_ends.values())
            size = max(nxt - start, 0)
        out.append((start, start + size))
    return out


bad = 0
for version in ("b2_us", "w2_us"):
    section_end = {}
    cur = None
    for line in (ROOT / "config" / version / "arm9" / MODULE / "delinks.txt").read_text().splitlines():
        m = re.match(r"^\s+(\.\w+)\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)", line)
        if m and cur is None:
            section_end[m.group(1)] = int(m.group(3), 16)
    objs = objects(version, section_end)
    for source, section, start, end in ranges(version):
        if end <= start:
            continue
        # Any object that meets the range must be wholly inside it
        for o_start, o_end in objs:
            if o_start >= end or o_end <= start:
                continue
            if o_end == end:
                # dsd gives the section's last object no size of its own
                continue
            if o_start < start:
                print(f"{version} {source} {section} 0x{start:08x}-0x{end:08x}: "
                      f"starts inside the object at 0x{o_start:08x} (ends 0x{o_end:08x})")
                bad += 1
            elif o_end > end:
                print(f"{version} {source} {section} 0x{start:08x}-0x{end:08x}: "
                      f"the object at 0x{o_start:08x} ends 0x{o_end:08x}, past the end")
                bad += 1
print(f"{bad} problem(s)")
sys.exit(1 if bad else 0)