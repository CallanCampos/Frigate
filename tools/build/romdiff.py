#!/usr/bin/env python3
"""Compare two NDS/TWL ROMs region by region and report where they differ."""
import argparse
import struct
import sys


def u32(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def regions(rom):
    """Return (name, start, end) for each known region of a ROM, sorted by start."""
    out = [('header', 0, 0x4000)]
    for name, off in [('arm9', 0x20), ('arm7', 0x30)]:
        start, size = u32(rom, off), u32(rom, off + 0xC)
        out.append((name, start, start + size))
    for name, off in [('fnt', 0x40), ('fat', 0x48), ('arm9_ovt', 0x50), ('arm7_ovt', 0x58)]:
        start, size = u32(rom, off), u32(rom, off + 4)
        if size:
            out.append((name, start, start + size))
    banner = u32(rom, 0x68)
    banner_size = u32(rom, 0x208) if rom[0x12] & 2 else 0x840
    out.append(('banner', banner, banner + banner_size))
    fat_off, fat_size = u32(rom, 0x48), u32(rom, 0x4C)
    files = [struct.unpack_from('<II', rom, fat_off + i) for i in range(0, fat_size, 8)]
    files = [(s, e) for s, e in files if e > s]
    if files:
        out.append(('fat_files', min(s for s, _ in files), max(e for _, e in files)))
    if rom[0x12] & 2:
        for name, off in [('arm9i', 0x1C0), ('arm7i', 0x1D0)]:
            start, size = u32(rom, off), u32(rom, off + 0xC)
            out.append((name, start, start + size))
        for name, off in [('digest_sector_hashtable', 0x1F0), ('digest_block_hashtable', 0x1F8)]:
            start, size = u32(rom, off), u32(rom, off + 4)
            out.append((name, start, start + size))
    return sorted(out, key=lambda r: r[1])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('expected')
    parser.add_argument('actual')
    parser.add_argument('-n', '--max-diffs', type=int, default=8, help='differing offsets to show per region')
    args = parser.parse_args()

    expected = open(args.expected, 'rb').read()
    actual = open(args.actual, 'rb').read()
    if len(expected) != len(actual):
        print(f'size differs: expected {len(expected):#x}, actual {len(actual):#x}')

    exp_regions = regions(expected)
    covered = []
    ok = True
    for name, start, end in exp_regions:
        covered.append((start, end))
        a, b = expected[start:end], actual[start:end]
        if a == b:
            print(f'  match  {name:24s} {start:#010x}..{end:#010x}')
            continue
        ok = False
        diffs = [i for i in range(min(len(a), len(b))) if a[i] != b[i]]
        print(f'  DIFFER {name:24s} {start:#010x}..{end:#010x}  {len(diffs)} bytes differ')
        for i in diffs[:args.max_diffs]:
            print(f'           {start + i:#010x}: expected {a[i]:02x} actual {b[i]:02x}')

    # gaps between known regions (padding, unknown data)
    covered.sort()
    pos = 0
    gaps = []
    for start, end in covered + [(max(len(expected), len(actual)), 0)]:
        if start > pos:
            gaps.append((pos, start))
        pos = max(pos, end)
    for start, end in gaps:
        a, b = expected[start:end], actual[start:end]
        if a != b:
            ok = False
            first = next((i for i in range(min(len(a), len(b))) if a[i] != b[i]), min(len(a), len(b)))
            print(f'  DIFFER gap                      {start:#010x}..{end:#010x}  first difference at {start + first:#010x}')

    if ok and len(expected) == len(actual):
        print('ROMs are identical')
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
