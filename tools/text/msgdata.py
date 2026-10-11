#!/usr/bin/env python3
"""Read the game's text files, the message data in the system and script message archives (a/0/0/2 and a/0/0/3).

    msgdata.py ARCHIVE FILE        # prints each line of one file with its index

A file has one section per language, although this game has a single one. Each line is UTF-16 text, encrypted with a
key that starts from its last character.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.narc import read_narc  # noqa: E402

LINE_END = 0xFFFF
NEWLINE = 0xFFFE
# Control codes, such as a placeholder for a name, start at this value and are followed by their arguments
CONTROL = 0xF000
# A compressed line starts with this value, see decompress
COMPRESSED = 0xF100


def decrypt(chars: list[int]) -> list[int]:
    chars = list(chars)
    if not chars:
        return chars
    key = chars[-1] ^ LINE_END
    for i in reversed(range(len(chars))):
        chars[i] ^= key
        key = ((key >> 3) | (key << 13)) & 0xFFFF
    return chars


def decompress(chars: list[int]) -> list[int]:
    """Unpacks a compressed line: after COMPRESSED, its characters are 9 bits each, packed from the lowest bit of each
    16-bit word up, and end with 0x1ff. Names of trainers, among others, are stored this way."""
    if not chars or chars[0] != COMPRESSED:
        return chars
    bits = 0
    count = 0
    out = []
    for word in chars[1:]:
        bits |= word << count
        count += 16
        while count >= 9:
            c = bits & 0x1FF
            if c == 0x1FF:
                return out + [LINE_END]
            out.append(c)
            bits >>= 9
            count -= 9
    return out + [LINE_END]


def to_text(chars: list[int]) -> str:
    text = ""
    for c in chars:
        if c == LINE_END:
            break
        if c == NEWLINE:
            text += "\n"
        elif c >= CONTROL:
            text += f"[{c:04x}]"
        else:
            text += chr(c)
    return text


def read_msgdata(data: bytes) -> list[str]:
    """Returns the lines of the first language."""
    num_sections, num_lines = struct.unpack_from("<HH", data, 0)
    section = struct.unpack_from("<I", data, 12)[0]
    lines = []
    for i in range(num_lines):
        offset, length, _ = struct.unpack_from("<IHH", data, section + 4 + 8 * i)
        chars = struct.unpack_from(f"<{length}H", data, section + offset)
        lines.append(to_text(decompress(decrypt(chars))))
    return lines


def read_archive_file(archive: Path, index: int) -> list[str]:
    return read_msgdata(read_narc(archive.read_bytes())[index])


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("archive", type=Path)
    parser.add_argument("file", type=int)
    args = parser.parse_args()
    for i, line in enumerate(read_archive_file(args.archive, args.file)):
        print(f"{i}\t{line!r}")


if __name__ == "__main__":
    main()
