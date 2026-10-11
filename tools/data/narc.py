#!/usr/bin/env python3
"""Read and write NARC archives, the numbered file archives under files/a/.

    narc.py unpack ARCHIVE OUTPUT_DIR        # writes 0000.bin, 0001.bin, ...
    narc.py pack OUTPUT FILE...              # packs the files in order

Only archives without file names are supported, which is every archive the game loads by ID.
"""
import argparse
import struct
import sys
from pathlib import Path


def read_narc(data: bytes) -> list[bytes]:
    magic, bom, version, size, header_size, sections = struct.unpack_from("<4sHHIHH", data, 0)
    if magic != b"NARC":
        raise ValueError("not a NARC archive")
    offset = header_size
    magic, fat_size, count = struct.unpack_from("<4sIH", data, offset)
    if magic != b"BTAF":
        raise ValueError("missing BTAF section")
    entries = [struct.unpack_from("<II", data, offset + 12 + 8 * i) for i in range(count)]
    offset += fat_size
    magic, fnt_size = struct.unpack_from("<4sI", data, offset)
    if magic != b"BTNF":
        raise ValueError("missing BTNF section")
    offset += fnt_size
    magic, _ = struct.unpack_from("<4sI", data, offset)
    if magic != b"GMIF":
        raise ValueError("missing GMIF section")
    image = offset + 8
    return [data[image + start : image + end] for start, end in entries]


def write_narc(files: list[bytes]) -> bytes:
    entries = []
    image = bytearray()
    for data in files:
        entries.append((len(image), len(image) + len(data)))
        image += data
        image += b"\xff" * (-len(image) % 4)
    fat = struct.pack("<4sIHH", b"BTAF", 12 + 8 * len(files), len(files), 0)
    fat += b"".join(struct.pack("<II", start, end) for start, end in entries)
    # A file name table with only the root directory, which holds every file
    fnt = struct.pack("<4sIIHH", b"BTNF", 16, 4, 0, 1)
    gmif = struct.pack("<4sI", b"GMIF", 8 + len(image)) + image
    size = 16 + len(fat) + len(fnt) + len(gmif)
    header = struct.pack("<4sHHIHH", b"NARC", 0xFFFE, 0x0100, size, 16, 3)
    return header + fat + fnt + gmif


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    unpack = commands.add_parser("unpack")
    unpack.add_argument("archive", type=Path)
    unpack.add_argument("output", type=Path)
    pack = commands.add_parser("pack")
    pack.add_argument("output", type=Path)
    pack.add_argument("files", type=Path, nargs="+")
    args = parser.parse_args()

    if args.command == "unpack":
        args.output.mkdir(parents=True, exist_ok=True)
        for i, data in enumerate(read_narc(args.archive.read_bytes())):
            (args.output / f"{i:04d}.bin").write_bytes(data)
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        # Replace a link rather than writing through it
        args.output.unlink(missing_ok=True)
        args.output.write_bytes(write_narc([f.read_bytes() for f in args.files]))


if __name__ == "__main__":
    sys.exit(main())
