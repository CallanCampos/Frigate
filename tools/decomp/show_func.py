#!/usr/bin/env python3
"""Print the disassembly of functions from dsd's `dis` output (build/<version>/asm)."""
import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("names", nargs="+", help="function names")
    parser.add_argument("--asm-dir", type=Path, default=ROOT / "build" / "asm")
    args = parser.parse_args()

    wanted = set(args.names)
    for asm in sorted(args.asm_dir.rglob("*.s")):
        text = asm.read_text()
        for name in list(wanted):
            match = re.search(rf"\n    (?:thumb|arm)_func_start {re.escape(name)}\n(.*?)\n    (?:thumb|arm)_func_end {re.escape(name)}\n", text, re.S)
            if match:
                print(f"; {asm.name}\n{match.group(1)}\n")
                wanted.discard(name)
    for name in wanted:
        print(f"; {name} not found")


if __name__ == "__main__":
    main()
