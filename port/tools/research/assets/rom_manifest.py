"""Writes the ROM manifest entry of a Black 2 or White 2 dump of another language, read straight from the ROM.

    rom_manifest.py ROM OUT.json

For PORT_ROM_LANGUAGE (port/cmake/rom.cmake), which lets Frigate load a ROM other than the US one the decomp builds:
the game's code stays the US C, and the ROM gives the data. Unlike manifest.py, which reads the decomp's extracted
files, this reads the ROM's own header, file name table (FNT) and file allocation table (FAT), extracting nothing. The
entry has the ROM's game code, header CRC and SHA-1, the size and SHA-1 of its ARM9 overlay table (the header's 0x50
and 0x54, which FS_LoadOverlayInfo reads, port/hal/fs/fs_overlay.c), and the size and SHA-1 of every named NitroFS
file. It is hashes only, nothing of the game itself. It goes in OUT.json under the version and language that the game
code says (b2_es for IRES), which a manifest already in OUT.json keeps alongside. Research of 2026-10-26:
port/docs/research/es-rom-support.md.
"""
import hashlib
import json
import mmap
import os
import struct
import sys

# The game code's first three letters give the version, and its fourth the language (GBATEK, "DS Cartridge Header")
VERSIONS = {"IRE": "b2", "IRD": "w2"}
LANGUAGES = {"O": "us", "J": "ja", "F": "fr", "D": "de", "I": "it", "S": "es", "K": "ko"}
FNT_ROOT_ID = 0xF000


def crc16(data: bytes) -> int:
    """The header's CRC-16 (polynomial 0xA001, from 0xFFFF), over its first 0x15E bytes."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def named_files(rom, fnt: int, fat: int) -> dict[str, tuple[int, int]]:
    """Every named file of the FNT, by its path without a leading slash, with its start and end in the ROM."""
    files = {}

    def walk(directory: int, prefix: str, depth: int):
        if depth > 32:
            raise SystemExit("the file name table nests too deep")
        entry = fnt + (directory & 0xFFF) * 8
        offset, file_id = struct.unpack_from("<IH", rom, entry)
        p = fnt + offset
        while rom[p] != 0:
            length = rom[p]
            p += 1
            if length < 0x80:
                name = rom[p:p + length].decode("ascii")
                p += length
                files[prefix + name] = struct.unpack_from("<II", rom, fat + 8 * file_id)
                file_id += 1
            else:
                name = rom[p:p + length - 0x80].decode("ascii")
                p += length - 0x80
                subdirectory = struct.unpack_from("<H", rom, p)[0]
                p += 2
                walk(subdirectory, prefix + name + "/", depth + 1)

    walk(FNT_ROOT_ID, "", 0)
    return files


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    rom_path, out_path = sys.argv[1:3]
    with open(rom_path, "rb") as f:
        rom = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
    code = rom[0x0C:0x10].decode("ascii", "replace")
    if code[:3] not in VERSIONS or code[3] not in LANGUAGES:
        raise SystemExit(f"{rom_path}: game code {code} is not Pokémon Black 2 or White 2")
    key = f"{VERSIONS[code[:3]]}_{LANGUAGES[code[3]]}"
    fnt, fat = struct.unpack_from("<I", rom, 0x40)[0], struct.unpack_from("<I", rom, 0x48)[0]
    table, table_size = struct.unpack_from("<II", rom, 0x50)
    files = {}
    for path, (start, end) in sorted(named_files(rom, fnt, fat).items()):
        if end < start or end > len(rom):
            raise SystemExit(f"{rom_path}: {path} lies outside the ROM")
        files[path] = {"size": end - start, "sha1": hashlib.sha1(rom[start:end]).hexdigest()}
    sha1 = hashlib.sha1()
    for offset in range(0, len(rom), 1 << 24):
        sha1.update(rom[offset:offset + (1 << 24)])
    entry = {
        "gamecode": code,
        "header_crc16": "%04x" % struct.unpack_from("<H", rom, 0x15E)[0],
        "header_crc16_valid": crc16(rom[:0x15E]) == struct.unpack_from("<H", rom, 0x15E)[0],
        "rom_sha1": sha1.hexdigest(),
        "overlay_table": {"size": table_size, "sha1": hashlib.sha1(rom[table:table + table_size]).hexdigest()},
        "files": files,
    }
    manifest = {}
    if os.path.exists(out_path):
        with open(out_path, encoding="utf-8") as f:
            manifest = json.load(f)
    manifest[key] = entry
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=1)
        f.write("\n")
    print(f"{out_path}: {key} ({code}), {len(files)} files, overlay table {table_size} bytes, "
          f"SHA-1 {entry['rom_sha1']}")


if __name__ == "__main__":
    main()
