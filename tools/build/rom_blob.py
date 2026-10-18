#!/usr/bin/env python3
"""Write a binary that the game embeds as it is, such as a DSP program, as the body of a C array's initializer.

The bytes come from the extracted module, at the symbol's address and with its size in the module's symbols.txt, and
are written as a list of bytes for a source file to include between the braces of its array. They can't be committed,
so the build takes them from the base ROM. Without the extracted module, compile_all.py (which CI runs without a ROM)
writes zeros of the same size instead, which compile the same; ninja's build of the file fails.

    rom_blob.py extract/b2_us/dsi/ltd_autoload_0.bin config/b2_us/arm9/ltd_autoload_0/symbols.txt \\
        DSP_G711Image build/include/generated/rom/dsp/dsp_g711_image.inc
"""
import argparse
import re
from pathlib import Path

BYTES_PER_LINE = 16


def symbol_range(symbols: Path, name: str) -> tuple[int, int]:
    """Returns the address and size of a byte array in a symbols.txt."""
    for line in symbols.read_text().splitlines():
        match = re.match(rf"{re.escape(name)} kind:data\(byte\[(\d+)\]\) addr:(0x[0-9a-f]+)", line)
        if match:
            return int(match.group(2), 16), int(match.group(1))
    raise SystemExit(f"{symbols}: no byte array named {name}")


def base_address(module: Path) -> int:
    """Returns the load address of an extracted module, from the .yaml beside it."""
    match = re.search(r"^base_address: (\d+)$", module.with_suffix(".yaml").read_text(), re.MULTILINE)
    if not match:
        raise SystemExit(f"{module.with_suffix('.yaml')}: no base_address")
    return int(match.group(1))


def blob(module: Path, symbols: Path, name: str, allow_missing: bool = False) -> bytes:
    """Returns the symbol's bytes from the extracted module, or, with allow_missing, zeros of its size if the module
    isn't extracted."""
    address, size = symbol_range(symbols, name)
    if not module.exists():
        if allow_missing:
            return bytes(size)
        raise SystemExit(f"{module} not found: extract the base ROM first")
    start = address - base_address(module)
    data = module.read_bytes()[start:start + size]
    if start < 0 or len(data) != size:
        raise SystemExit(f"{module}: {name} lies outside the module")
    return data


def initializer(data: bytes) -> str:
    """Returns the bytes as the lines of an array initializer, without its braces."""
    return "".join(", ".join(f"0x{b:02X}" for b in data[i:i + BYTES_PER_LINE]) + ",\n"
                   for i in range(0, len(data), BYTES_PER_LINE))


def write(path: Path, text: str):
    """Writes the file only if it changes, so that ninja's restat leaves the files that include it alone."""
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("module", type=Path, help="the extracted module, with its .yaml beside it")
    parser.add_argument("symbols", type=Path, help="the module's symbols.txt")
    parser.add_argument("name", help="the symbol of the byte array")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    write(args.output, initializer(blob(args.module, args.symbols, args.name)))


if __name__ == "__main__":
    main()
