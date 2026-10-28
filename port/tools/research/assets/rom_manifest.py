"""Writes the ROM manifest entry of an experimental Spanish Black 2 dump, read straight from the ROM.

    rom_manifest.py ROM OUT.json

For PORT_ROM_LANGUAGE (port/cmake/rom.cmake), which lets Frigate load a ROM other than the US one the decomp builds:
the game's code stays the US C, and the ROM gives the data. Unlike manifest.py, which reads the decomp's extracted
files, this reads the ROM's own header, file name table (FNT) and file allocation table (FAT), extracting nothing. The
entry has the ROM's game code, header CRC and SHA-1, the size and SHA-1 of its ARM9 overlay table (the header's 0x50
and 0x54, which FS_LoadOverlayInfo reads, port/hal/fs/fs_overlay.c), and the size and SHA-1 of every named NitroFS
file. The output contains hashes and structural metadata, never extracted assets. It goes in OUT.json under the version and language that the game
code says (b2_es for IRES only), which a manifest already in OUT.json keeps alongside. Research of 2026-10-26:
port/docs/research/es-rom-support.md.
"""
import argparse
import hashlib
import json
import mmap
import os
import re
import struct
import tempfile

FNT_ROOT_ID = 0xF000
VALIDATION_VERSION = 1


def crc16(data: bytes) -> int:
    """The header's CRC-16 (polynomial 0xA001, from 0xFFFF)."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def require(condition, message):
    if not condition:
        raise ValueError(message)


def checked_region(rom, offset, size, name):
    require(offset >= 0x200 and size > 0 and offset + size <= len(rom), f"{name} lies outside the ROM")
    return offset, offset + size


def safe_path(path):
    # These names become C string literals in rom.cmake. Reject ambiguous paths and C/CMake metacharacters.
    return (isinstance(path, str) and len(path) < 256
            and all(re.fullmatch(r"[A-Za-z0-9_.-]+", part) and part not in (".", "..")
                    for part in path.split("/")))


def named_files(rom, fnt, fnt_size, ranges):
    """Walk only within the declared FNT, with unique directories, file IDs and safe paths."""
    require(fnt_size >= 8, "FNT has no root")
    count = struct.unpack_from("<H", rom, fnt + 6)[0]
    require(0 < count <= 0x1000 and count * 8 <= fnt_size, "invalid FNT directory count")
    end = fnt + fnt_size
    files, visited, used_ids = {}, set(), set()

    def walk(directory, prefix, parent, depth):
        require(depth <= 32, "FNT nests too deep")
        require(FNT_ROOT_ID <= directory < FNT_ROOT_ID + count, "invalid FNT directory ID")
        require(directory not in visited, "repeated or cyclic FNT directory")
        visited.add(directory)
        entry = fnt + (directory - FNT_ROOT_ID) * 8
        offset, file_id, stored_parent = struct.unpack_from("<IHH", rom, entry)
        require(parent is None or stored_parent == parent, "wrong FNT parent")
        require(count * 8 <= offset < fnt_size, "FNT subtable outside declared bounds")
        p = fnt + offset
        names = set()
        while True:
            require(p < end, "unterminated FNT subtable")
            length = rom[p]
            p += 1
            if length == 0:
                break
            size = length & 0x7f
            require(size > 0 and p + size + (2 if length & 0x80 else 0) <= end, "truncated FNT name")
            name = bytes(rom[p:p + size]).decode("ascii")
            p += size
            require(safe_path(name) and "/" not in name and name not in names, "unsafe or duplicate FNT name")
            names.add(name)
            path = prefix + name
            require(safe_path(path), "unsafe or too long FNT path")
            if length & 0x80:
                child = struct.unpack_from("<H", rom, p)[0]
                p += 2
                walk(child, path + "/", directory, depth + 1)
            else:
                require(file_id < len(ranges) and file_id not in used_ids, "invalid or repeated FNT file ID")
                used_ids.add(file_id)
                files[path] = ranges[file_id]
                file_id += 1

    walk(FNT_ROOT_ID, "", None, 0)
    require(len(visited) == count and files, "unreachable FNT directories or no named files")
    return files


def make_entry(rom, overlay_count=344):
    """Validate the source structure before a whole-file hash can authorize its exact bytes."""
    require(len(rom) >= 0x200, "ROM is shorter than its header")
    require(bytes(rom[0x0c:0x10]) == b"IRES", "only experimental Spanish Black 2 (IRES) is supported")
    header_crc = struct.unpack_from("<H", rom, 0x15e)[0]
    require(crc16(rom[:0x15e]) == header_crc, "invalid header CRC")
    fnt, fnt_size, fat, fat_size, table, table_size = struct.unpack_from("<IIIIII", rom, 0x40)
    regions = [checked_region(rom, fnt, fnt_size, "FNT"), checked_region(rom, fat, fat_size, "FAT"),
               checked_region(rom, table, table_size, "overlay table")]
    ordered = sorted(regions)
    require(all(a[1] <= b[0] for a, b in zip(ordered, ordered[1:])), "overlapping ROM tables")
    require(fat_size % 8 == 0, "FAT size is not a multiple of eight")
    require(table_size == overlay_count * 32, "unexpected ARM9 overlay count")
    ranges = [struct.unpack_from("<II", rom, p) for p in range(fat, fat + fat_size, 8)]
    require(all(0 <= start <= end <= len(rom) for start, end in ranges), "FAT file lies outside the ROM")
    overlay_ids = set()
    for i in range(overlay_count):
        ident, address, size, bss, sinit, einit, file_id, flags = struct.unpack_from("<8I", rom, table + i * 32)
        require(ident == i and file_id < len(ranges) and file_id not in overlay_ids, "invalid overlay ID or file ID")
        overlay_ids.add(file_id)
        stored = flags & 0xffffff if flags & 0x1000000 else size
        require(stored > 0 and stored <= ranges[file_id][1] - ranges[file_id][0], "overlay image exceeds FAT file")
        require(address + size + bss <= 0x100000000 and sinit <= einit, "invalid overlay memory range")
    named = named_files(rom, fnt, fnt_size, ranges)
    digest = lambda start, end: hashlib.sha1(rom[start:end]).hexdigest()
    sha1 = hashlib.sha1()
    for offset in range(0, len(rom), 1 << 24):
        sha1.update(rom[offset:offset + (1 << 24)])
    return {"gamecode": "IRES", "validation_version": VALIDATION_VERSION,
            "header_crc16": f"{header_crc:04x}", "header_crc16_valid": True,
            "rom_sha1": sha1.hexdigest(),
            "overlay_table": {"size": table_size, "sha1": digest(table, table + table_size)},
            "files": {path: {"size": end - start, "sha1": digest(start, end)}
                      for path, (start, end) in sorted(named.items())}}


def load_json(path):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            require(key not in result, f"duplicate JSON key: {key}")
            result[key] = value
        return result
    with open(path, encoding="utf-8") as f:
        return json.load(f, object_pairs_hook=unique)


def validate_entry(entry, overlay_count):
    """Check the generated schema before rom.cmake emits C; this is not an authenticity certificate."""
    require(isinstance(entry, dict) and entry.get("gamecode") == "IRES", "expected IRES manifest")
    require(type(entry.get("validation_version")) is int and entry["validation_version"] == VALIDATION_VERSION,
            "regenerate the manifest with the current structurally validating generator")
    require(entry.get("header_crc16_valid") is True and isinstance(entry.get("header_crc16"), str)
            and re.fullmatch(r"[0-9a-f]{4}", entry["header_crc16"]),
            "manifest header CRC is not validated")
    def hash_value(value):
        require(isinstance(value, str) and re.fullmatch(r"[0-9a-f]{40}", value), "invalid manifest SHA-1")
    hash_value(entry.get("rom_sha1"))
    overlay = entry.get("overlay_table", {})
    require(isinstance(overlay, dict), "invalid overlay table record")
    require(type(overlay.get("size")) is int and overlay["size"] == overlay_count * 32, "wrong overlay table size")
    hash_value(overlay.get("sha1"))
    files = entry.get("files")
    require(isinstance(files, dict) and len(files) > 0, "manifest has no files")
    for path, data in files.items():
        require(safe_path(path) and isinstance(data, dict), "unsafe manifest path or file record")
        require(type(data.get("size")) is int and 0 <= data["size"] <= 0xffffffff, "invalid file size")
        hash_value(data.get("sha1"))


def write_manifest(path, entry):
    manifest = load_json(path) if os.path.exists(path) else {}
    require(isinstance(manifest, dict), "manifest must be an object")
    manifest["b2_es"] = entry
    # Never truncate the previous file on a failed write, including a full device.
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=os.path.dirname(os.path.abspath(path)),
                                         prefix=".rom-manifest-", delete=False) as f:
            temporary = f.name
            json.dump(manifest, f, indent=1)
            f.write("\n")
            f.flush()
            os.fsync(f.fileno())
        os.replace(temporary, path)
    finally:
        if temporary and os.path.exists(temporary):
            os.unlink(temporary)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*")
    parser.add_argument("--validate-manifest", metavar="FILE")
    parser.add_argument("--overlay-count", type=int, default=344)
    args = parser.parse_args()
    try:
        if args.validate_manifest:
            require(not args.paths, "validation takes no ROM paths")
            manifest = load_json(args.validate_manifest)
            require(isinstance(manifest, dict) and "b2_es" in manifest, "manifest has no b2_es entry")
            validate_entry(manifest["b2_es"], args.overlay_count)
        else:
            require(len(args.paths) == 2, "expected ROM OUT.json")
            rom_path, out_path = args.paths
            require(os.path.realpath(rom_path) != os.path.realpath(out_path), "output must not replace the ROM")
            with open(rom_path, "rb") as f:
                require(os.fstat(f.fileno()).st_size >= 0x200, "ROM is shorter than its header")
                with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as rom:
                    entry = make_entry(rom)
            write_manifest(out_path, entry)
            print(f"{out_path}: experimental b2_es (IRES), {len(entry['files'])} files, SHA-1 {entry['rom_sha1']}")
    except (ValueError, OSError, UnicodeError, struct.error) as error:
        parser.exit(1, f"rom_manifest.py: {error}\n")


if __name__ == "__main__":
    main()
