#!/usr/bin/env python3
"""Write bytes that the game has as they are, which no source can give, from the extracted base ROM.

`array` writes a binary that the game embeds, such as a DSP program, as the body of a C array's initializer: the bytes
at the symbol's address and with its size in the module's symbols.txt, as a list for a source file to include between
the braces of its array. `range` writes the bytes of an address range of a module as an assembler macro, `NAME start,
end`, which gives the bytes from offset start to offset end of the range, for an assembly source that takes pieces of
it, such as the filler of the system call library in the secure area. mwasmarm's .incbin would take a binary as text
and turn each lone line feed into a carriage return, so the bytes are written as .byte directives. The bytes can't be
committed, so the build takes them from the base ROM. Without the extracted module, compile_all.py (which CI runs
without a ROM) writes zeros of the same size instead, which compile the same; ninja's build of the file fails.

    rom_blob.py array extract/b2_us/dsi/ltd_autoload_0.bin config/b2_us/arm9/ltd_autoload_0/symbols.txt \\
        DSP_G711Image build/include/generated/rom/dsp/dsp_g711_image.inc
    rom_blob.py range extract/b2_us/arm9/arm9.bin 0x02004000 0x02004800 FILLER \\
        build/include/generated/rom/syscall/secure_area_b2_us.inc
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


def module_bytes(module: Path, address: int, size: int, allow_missing: bool = False) -> bytes:
    """Returns the module's bytes at an address, or, with allow_missing, zeros of the size if the module isn't
    extracted."""
    if not module.exists():
        if allow_missing:
            return bytes(size)
        raise SystemExit(f"{module} not found: extract the base ROM first")
    start = address - base_address(module)
    data = module.read_bytes()[start:start + size]
    if start < 0 or len(data) != size:
        raise SystemExit(f"{module}: {size:#x} bytes at {address:#010x} lie outside the module")
    return data


def blob(module: Path, symbols: Path, name: str, allow_missing: bool = False) -> bytes:
    """Returns the symbol's bytes from the extracted module, or, with allow_missing, zeros of its size if the module
    isn't extracted."""
    return module_bytes(module, *symbol_range(symbols, name), allow_missing)


def initializer(data: bytes) -> str:
    """Returns the bytes as the lines of an array initializer, without its braces."""
    return "".join(", ".join(f"0x{b:02X}" for b in data[i:i + BYTES_PER_LINE]) + ",\n"
                   for i in range(0, len(data), BYTES_PER_LINE))


def range_macro(data: bytes, name: str) -> str:
    """Returns the bytes as an assembler macro of mwasmarm, `name start, end`, that gives the bytes from offset start to
    offset end."""
    lines = [f"\t.macro {name} start, end"]
    for offset, b in enumerate(data):
        lines += [f"\t.if \\start <= {offset:#05x} && {offset:#05x} < \\end", f"\t.byte {b:#04x}", "\t.endif"]
    return "\n".join([*lines, "\t.endm", ""])


def write(path: Path, text: str):
    """Writes the file only if it changes, so that ninja's restat leaves the files that include it alone."""
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    array = commands.add_parser("array", help="a byte array's initializer")
    array.add_argument("module", type=Path, help="the extracted module, with its .yaml beside it")
    array.add_argument("symbols", type=Path, help="the module's symbols.txt")
    array.add_argument("name", help="the symbol of the byte array")
    array.add_argument("output", type=Path)
    span = commands.add_parser("range", help="the bytes of an address range, as an assembler macro")
    span.add_argument("module", type=Path, help="the extracted module, with its .yaml beside it")
    span.add_argument("start", type=lambda s: int(s, 0), help="the first address")
    span.add_argument("end", type=lambda s: int(s, 0), help="the address after the last")
    span.add_argument("name", help="the macro's name")
    span.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "array":
        write(args.output, initializer(blob(args.module, args.symbols, args.name)))
    else:
        write(args.output, range_macro(module_bytes(args.module, args.start, args.end - args.start), args.name))


if __name__ == "__main__":
    main()
