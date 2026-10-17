#!/usr/bin/env python3
"""The decomp's symbols for the oracle's memory dumps (port/tools/oracle/README.md).

config/<version>/arm9/**/symbols.txt names what the delinks know: functions with their sizes, data and .bss without.
The statics of the files that are complete (sVBlankCount, GFL's frame count, among them) are only in the ROM build's
link map, build/<version>/arm9.o.xMAP, which `ninja` writes; it gives every linked symbol's size too. This writes the
map's symbols in symbols.txt's format, with size:, module: and file: words, for pbw-oracle's --symbols, and resolves
names the way pbw-oracle does.

    symbols.py statics [--version b2_us] [--out FILE]   the map's symbols (stdout without --out)
    symbols.py resolve [--version b2_us] NAME...        NAME ADDRESS SIZE MODULE for each name

Standard library only.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from dataclasses import dataclass

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
MAX_IMPLIED_SIZE = 0x1000

# A symbol line of the map: "  0209DA64 00000004 .bss    sVBlankCount\t(gfl_use.o)"
MAP_LINE = re.compile(r"^  ([0-9A-F]{8}) ([0-9A-F]{8}) (\.\w+)\s+(\S+)\t\((.+)\)\s*$")
# A section header of the map: "# .arm9", "# .ov033"
MAP_SECTION = re.compile(r"^# \.(\w+)\s*$")


@dataclass
class Symbol:
    name: str
    module: str
    address: int
    size: int
    file: str = ""
    sized: bool = False


def module_of(section: str) -> str:
    return "main" if section == "arm9" else section


def parse_map(lines) -> list[Symbol]:
    """The named symbols with a size in the lines of a link map"""
    out = []
    module = None
    for line in lines:
        line = line.rstrip("\r\n")
        header = MAP_SECTION.match(line)
        if header:
            module = module_of(header.group(1))
            continue
        match = MAP_LINE.match(line)
        if not match or module is None:
            continue
        address, size, _, name, obj = match.groups()
        if name[0] in ".$@" or int(size, 16) == 0:
            continue
        source = obj[:-2] + ".c" if obj.endswith(".o") else obj
        out.append(Symbol(name, module, int(address, 16), int(size, 16), source, True))
    return out


def read_map(version: str) -> list[Symbol]:
    """The ROM build's link map's symbols (none when the ROM build hasn't run)"""
    path = os.path.join(REPO_ROOT, "build", version, "arm9.o.xMAP")
    if not os.path.exists(path):
        return []
    with open(path, encoding="latin-1") as f:
        return parse_map(f)


def read_config(version: str) -> list[Symbol]:
    root = os.path.join(REPO_ROOT, "config", version, "arm9")
    out = []
    for directory, _, files in sorted(os.walk(root)):
        if "symbols.txt" not in files:
            continue
        module = "main" if os.path.samefile(directory, root) else os.path.basename(directory)
        with open(os.path.join(directory, "symbols.txt")) as f:
            for line in f:
                words = line.split()
                if not words or words[0].startswith("#"):
                    continue
                symbol = Symbol(words[0], module, 0, 0)
                for word in words[1:]:
                    key, _, value = word.partition(":")
                    if key == "addr":
                        symbol.address = int(value, 16)
                    elif key == "kind":
                        size = re.search(r"size=(0x[0-9a-fA-F]+)", value)
                        if size:
                            symbol.size = int(size.group(1), 16)
                            symbol.sized = True
                out.append(symbol)
    return out


def finish(symbols: list[Symbol]) -> None:
    """Gives the symbols without a size one, as pbw-oracle does: up to the next symbol of the module"""
    by_module: dict[str, list[Symbol]] = {}
    for symbol in symbols:
        by_module.setdefault(symbol.module, []).append(symbol)
    for group in by_module.values():
        group.sort(key=lambda s: s.address)
        for i, symbol in enumerate(group):
            if symbol.sized:
                continue
            size = 4
            for later in group[i + 1 :]:
                if later.address > symbol.address:
                    size = later.address - symbol.address
                    break
            symbol.size = min(size, MAX_IMPLIED_SIZE)


def find(symbols: list[Symbol], name: str) -> Symbol:
    qualifier, _, bare = name.rpartition("/")
    found = [s for s in symbols if s.name == bare and (not qualifier or qualifier in (s.module, s.file))]
    # The same symbol from both sources counts once, with the map's size
    unique: dict[tuple[str, int], Symbol] = {}
    for symbol in found:
        key = (symbol.module, symbol.address)
        if key not in unique or (symbol.sized and not unique[key].sized):
            unique[key] = symbol
    if len(unique) == 1:
        return next(iter(unique.values()))
    if not unique:
        raise KeyError(f"no symbol {name}")
    places = " ".join(f"{s.file or s.module}/{s.name}" for s in unique.values())
    raise KeyError(f"symbol {name} is in several places: {places}")


def write_statics(symbols: list[Symbol], out) -> None:
    out.write("# The ROM build's link map (build/<version>/arm9.o.xMAP), by port/tools/oracle/symbols.py\n")
    for s in symbols:
        out.write(f"{s.name} kind:map addr:0x{s.address:08x} size:0x{s.size:x} module:{s.module} file:{s.file}\n")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    statics = commands.add_parser("statics", help="write the link map's symbols in symbols.txt's format")
    statics.add_argument("--version", default="b2_us", choices=["b2_us", "w2_us"])
    statics.add_argument("--out")
    resolve = commands.add_parser("resolve", help="print NAME ADDRESS SIZE MODULE for each name")
    resolve.add_argument("--version", default="b2_us", choices=["b2_us", "w2_us"])
    resolve.add_argument("names", nargs="+")
    args = parser.parse_args(argv)

    mapped = read_map(args.version)
    if args.command == "statics":
        if not mapped:
            print(f"symbols.py: no build/{args.version}/arm9.o.xMAP: run the ROM build (ninja) first", file=sys.stderr)
            return 1
        if args.out:
            with open(args.out, "w") as f:
                write_statics(mapped, f)
        else:
            write_statics(mapped, sys.stdout)
        return 0

    symbols = read_config(args.version) + mapped
    finish(symbols)
    status = 0
    for name in args.names:
        try:
            s = find(symbols, name)
            print(f"{name} 0x{s.address:08x} {s.size} {s.module}")
        except KeyError as error:
            print(f"symbols.py: {error.args[0]}", file=sys.stderr)
            status = 1
    return status


if __name__ == "__main__":
    sys.exit(main())
