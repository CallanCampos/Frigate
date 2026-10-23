#!/usr/bin/env python3
"""Give a compiled object's functions the second names that the configs record as labels.

    object_aliases.py build/b2_us/lib/nitrowifi/src/wcm/wcm.unaliased.o build/b2_us/lib/nitrowifi/src/wcm/wcm.o \\
        ReshZekSave_UnresolvedInitTarget=WcmGetLinkLevel

A function can have a second name that other code calls, which dsd keeps as a label at the function's start. A
delinked object defines both; an object that MWCC compiles defines one, since MWCC ignores an alias attribute or
pragma and its ARM assembler takes no `entry`. (A library written in assembly gives its routines both names itself,
as MSL's fp_fastI.s does with mwasmarm.) The one such label left is ReshZekSave_UnresolvedInitTarget, which main's
ReshZekSave_Init calls at an address of the overlay region whose function is still unknown.

The linker can't give the second name either: a symbol that its script assigns has no ARM or Thumb type, so a Thumb
`bl` to it stays a `bl` where an ARM routine needs a `blx`. So this adds each label to the object's symbol table as a
copy of its function's symbol, with the same section, value and type, which the linker treats like any function. Its
size is 0, as a label's, since mwldarm refuses a section whose symbols' sizes add up to more than the section.
configure.py runs this for every compiled C file that has such a label.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

SYMBOL = re.compile(r"^(\S+) kind:(function|label)\([^)]*\) addr:(0x[0-9a-f]+)")
CODE_RANGE = re.compile(r"^\s+\.text\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)")
SHT_SYMTAB = 2
SYMBOL_SIZE = 16


def file_code(delinks: Path) -> list[tuple[str, int, int]]:
    """Returns (file, start, end) of every file's .text range in a delinks.txt."""
    ranges = []
    for block in delinks.read_text().split("\n\n"):
        lines = block.strip("\n").splitlines()
        # The module's own sections come first, indented, with no file's name over them
        if not lines or lines[0][:1].isspace() or not lines[0].endswith(":"):
            continue
        ranges += [(lines[0][:-1], int(m[1], 16), int(m[2], 16)) for m in map(CODE_RANGE.match, lines[1:]) if m]
    return ranges


def label_aliases(config: Path) -> dict[str, dict[str, str]]:
    """Returns, by source file, the function of every label at the start of one of its functions, by label."""
    aliases: dict[str, dict[str, str]] = {}
    for symbols_path in sorted(config.rglob("symbols.txt")):
        functions, labels = {}, []
        for match in map(SYMBOL.match, symbols_path.read_text().splitlines()):
            if match:
                name, kind, addr = match[1], match[2], int(match[3], 16)
                if kind == "function":
                    functions[addr] = name
                else:
                    labels.append((name, addr))
        code = file_code(symbols_path.parent / "delinks.txt")
        for name, addr in labels:
            function = functions.get(addr)
            if function is None or function == name:
                continue
            source = next((file for file, start, end in code if start <= addr < end), None)
            if source is not None:
                aliases.setdefault(source, {})[name] = function
    return aliases


def add_aliases(elf: bytes, aliases: dict[str, str]) -> bytes:
    """Returns a 32-bit little-endian ELF object with each alias added as a copy of its function's symbol. The grown
    symbol and string tables are moved to the end of the file, and the section headers after them."""
    if elf[:4] != b"\x7fELF" or elf[4] != 1 or elf[5] != 1:
        raise SystemExit("not a 32-bit little-endian ELF object")
    shoff, = struct.unpack_from("<I", elf, 0x20)
    shentsize, shnum = struct.unpack_from("<HH", elf, 0x2e)
    headers = [list(struct.unpack_from("<10I", elf, shoff + i * shentsize)) for i in range(shnum)]
    symtab_index = next(i for i, h in enumerate(headers) if h[1] == SHT_SYMTAB)
    symtab, strtab_index = headers[symtab_index], headers[symtab_index][6]
    strtab = headers[strtab_index]
    symbols = bytearray(elf[symtab[4]:symtab[4] + symtab[5]])
    strings = bytearray(elf[strtab[4]:strtab[4] + strtab[5]])

    def name_at(offset: int) -> str:
        return strings[offset:strings.index(0, offset)].decode()

    entries = [struct.unpack_from("<IIIBBH", symbols, i) for i in range(0, len(symbols), SYMBOL_SIZE)]
    # A defined symbol, in a section
    defined = {name_at(e[0]): e for e in entries if e[5] != 0}
    for label, function in aliases.items():
        if label in defined:
            raise SystemExit(f"the object already defines {label}")
        if function not in defined:
            raise SystemExit(f"the object doesn't define {function}, which {label} names")
        # Size 0, as a label: mwldarm refuses a section whose symbols' sizes add up to more than it
        _, value, _, info, other, shndx = defined[function]
        symbols += struct.pack("<IIIBBH", len(strings), value, 0, info, other, shndx)
        strings += label.encode() + b"\0"

    out = bytearray(elf)
    for header, data in ((strtab, strings), (symtab, symbols)):
        out += b"\0" * (-len(out) % 4)
        header[4], header[5] = len(out), len(data)
        out += data
    out += b"\0" * (-len(out) % 4)
    struct.pack_into("<I", out, 0x20, len(out))
    for header in headers:
        out += struct.pack("<10I", *header)
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("aliases", nargs="+", metavar="LABEL=FUNCTION")
    args = parser.parse_args()
    aliases = dict(alias.split("=", 1) for alias in args.aliases)
    args.output.write_bytes(add_aliases(args.input.read_bytes(), aliases))


if __name__ == "__main__":
    sys.exit(main())
