#!/usr/bin/env python3
"""The name input's tables, res/name_input/name_input.json (ARCID_NAME_INPUT, a/1/2/0): dump writes it from the ROM
once, and pack builds the archive from it, which the build does.

    name_input_data.py dump extract/b2_us/files res/name_input/name_input.json
    name_input_data.py pack res/name_input/name_input.json ARCHIVE

The archive's members are those of name_input_data.c: NAME_INPUT_KEYBOARD_COUNT keyboards (NameInputKeyboard), grids
of the characters that can be entered, each row a string; the three word tables (NameInputWordTable), each word in
four spellings, which this version leaves empty; and the conversion table (NameInputConvTable), the rows of the
characters that change into each other, which NameInputConvTable_GetNext steps through. A word's spelling and a row
are up to NAME_INPUT_WORD_CHARS characters, which the packer fills with NAME_INPUT_EMPTY_CHAR (an ideographic space),
the character that also fills a keyboard's unused places, which the keyboards keep in their rows.

Both versions have the same file. name_input.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

KEYBOARD_COUNT = 5  # NAME_INPUT_KEYBOARD_COUNT
WORD_TABLE_COUNT = 3  # NARC_NAME_INPUT_CONV_TABLE - NARC_NAME_INPUT_WORD_TABLE_0
WORD_CHARS = 3  # NAME_INPUT_WORD_CHARS
FORMS = 4
EMPTY = 0x3000  # NAME_INPUT_EMPTY_CHAR


def chars(values) -> str:
    if any(0xD800 <= c < 0xE000 or c < 0x20 for c in values):
        raise ValueError("a character isn't one")
    return "".join(map(chr, values))


def cell(values) -> str:
    """A spelling or a row, without the empty characters that fill it."""
    return chars(values).rstrip(chr(EMPTY))


def fill(text: str, where: str) -> list[int]:
    if len(text) > WORD_CHARS:
        raise DataError(f"{where}: {text!r} is longer than {WORD_CHARS} characters")
    return [ord(c) for c in text] + [EMPTY] * (WORD_CHARS - len(text))


def u16s(data: bytes, offset: int) -> list[int]:
    return list(struct.unpack_from(f"<{(len(data) - offset) // 2}H", data, offset))


def dump(files: Path, output: Path):
    members = read_narc((files / "a/1/2/0").read_bytes())
    if len(members) != KEYBOARD_COUNT + WORD_TABLE_COUNT + 1 or any(len(m) % 2 for m in members):
        raise ValueError("the name input's archive isn't its keyboards, word tables and conversion table")
    keyboards = []
    for data in members[:KEYBOARD_COUNT]:
        width, height = struct.unpack_from("<HH", data)
        grid = u16s(data, 4)
        if len(grid) != width * height:
            raise ValueError("a keyboard isn't its width by its height")
        keyboards.append([chars(grid[row * width:(row + 1) * width]) for row in range(height)])
    tables = []
    for data in members[KEYBOARD_COUNT:KEYBOARD_COUNT + WORD_TABLE_COUNT]:
        count, *rest = u16s(data, 0)
        size = FORMS * WORD_CHARS
        if len(rest) != count * size:
            raise ValueError("a word table isn't its count of words")
        tables.append([[cell(rest[i + f * WORD_CHARS:i + (f + 1) * WORD_CHARS]) for f in range(FORMS)]
                       for i in range(0, len(rest), size)])
    count, *rest = u16s(members[-1], 0)
    if len(rest) != count * WORD_CHARS:
        raise ValueError("the conversion table isn't its count of rows")
    conversions = [cell(rest[i:i + WORD_CHARS]) for i in range(0, len(rest), WORD_CHARS)]
    write(output, {"$schema": "name_input.schema.json", "keyboards": keyboards, "word_tables": tables,
                   "conversions": conversions})
    print(f"wrote {len(keyboards)} keyboards, {len(tables)} word tables and {len(conversions)} conversions to {output}")


def pack(path: Path, output: Path):
    data = load(path, load_schema(path.parent / "name_input.schema.json"))
    where = label(path)
    members = []
    for i, rows in enumerate(data["keyboards"]):
        if any(len(row) != len(rows[0]) for row in rows):
            raise DataError(f"{where}: keyboard {i}'s rows aren't of one width")
        members.append(struct.pack("<HH", len(rows[0]), len(rows)) +
                       b"".join(struct.pack(f"<{len(row)}H", *map(ord, row)) for row in rows))
    for i, words in enumerate(data["word_tables"]):
        values = [len(words)] + [c for word in words for form in word for c in fill(form, f"{where}: table {i}")]
        members.append(struct.pack(f"<{len(values)}H", *values))
    rows = data["conversions"]
    values = [len(rows)] + [c for row in rows for c in fill(row, f"{where}: conversions")]
    members.append(struct.pack(f"<{len(values)}H", *values))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write name_input.json from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the name input's archive")
    pack_parser.add_argument("path", type=Path)
    pack_parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.path, args.archive)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
