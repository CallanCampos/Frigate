#!/usr/bin/env python3
"""Pad a linker script so that code and data move, for `configure.py --shift`.

    shift_lcf.py build/b2_us/arm9.lcf build/b2_us/arm9_shifted.lcf 0x100

The padding goes into the main module's .text just before Game Freak's library (heapsys.o), so that the rest of main's
code, all of its data and every overlay, which are placed after main, move; and at the start of every overlay's .text,
so that each overlay's own code and data move within it. A ROM built this way only works if every pointer to the moved
code and data is a relocation, which is what a mod that changes code sizes relies on.
"""
import argparse
import re
from pathlib import Path

# The first object of the main module that the padding precedes
MAIN_ANCHOR = "heapsys.o(.text)"


def pad(amount: int, indent: str) -> list[str]:
    return [f"{indent}. = . + {amount:#x};"]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("lcf", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("amount", type=lambda s: int(s, 0), help="bytes of padding, a multiple of 32")
    args = parser.parse_args()
    if args.amount % 32:
        parser.error("the padding must be a multiple of 32, the alignment of the sections after it")

    lines = args.lcf.read_text().splitlines()
    out = []
    anchored = False
    for line in lines:
        stripped = line.strip()
        if stripped == MAIN_ANCHOR and not anchored:
            out += pad(args.amount, line[: len(line) - len(line.lstrip())])
            anchored = True
        out.append(line)
        if re.fullmatch(r"OV\d+_TEXT_START = \.;", stripped):
            out += pad(args.amount, line[: len(line) - len(line.lstrip())])
    if not anchored:
        raise SystemExit(f"{MAIN_ANCHOR} not found in {args.lcf}")
    args.output.write_text("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
