#!/usr/bin/env python3
"""Add NitroSDK's linker symbols to the linker script that dsd writes.

    lcf_symbols.py build/b2_us/arm9.lcf config/linker_symbols.toml build/b2_us/arm9_symbols.lcf

NitroSDK's code reads a few numbers as the addresses of symbols that its own linker script defines, such as the stack
sizes. dsd's linker script defines only __OVERLAY_COUNT of them, so this adds those of the config after it, as
absolute symbols in the config's order.
"""
import argparse
import re
import tomllib
from pathlib import Path

# dsd's definition that the symbols follow
ANCHOR = re.compile(r"(\s*)__OVERLAY_COUNT = \d+;")


def load_symbols(path: Path) -> dict[str, int]:
    with path.open("rb") as f:
        symbols = tomllib.load(f)
    for name, value in symbols.items():
        if not re.fullmatch(r"[A-Za-z_]\w*", name):
            raise SystemExit(f"{path}: {name!r} is not a symbol name")
        if type(value) is not int or not 0 <= value < 1 << 32:
            raise SystemExit(f"{path}: {name} must be a 32-bit unsigned number")
    return symbols


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("lcf", type=Path)
    parser.add_argument("symbols", type=Path, help="the symbols and their values, in TOML")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    symbols = load_symbols(args.symbols)
    lines = args.lcf.read_text().splitlines()
    defined = [name for name in symbols if any(re.match(rf"\s*{name}\s*=", line) for line in lines)]
    if defined:
        raise SystemExit(f"{args.lcf} already defines {', '.join(defined)}")
    out = []
    anchored = False
    for line in lines:
        out.append(line)
        match = ANCHOR.fullmatch(line)
        if match and not anchored:
            out += [f"{match[1]}{name} = {value:#x};" for name, value in symbols.items()]
            anchored = True
    if not anchored:
        raise SystemExit(f"__OVERLAY_COUNT not found in {args.lcf}")
    args.output.write_text("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
