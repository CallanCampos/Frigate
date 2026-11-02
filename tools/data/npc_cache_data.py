#!/usr/bin/env python3
"""The zones' NPC info caches in res/field/actors/npc_caches/, one JSON file per cache: dump writes them from the ROM
once, and pack builds ARCID_FIELD_ACTOR_CONFIG (a/0/5/7) from them and the field actors' records, which the build
does.

    npc_cache_data.py dump extract/b2_us/files res/field/actors/npc_caches
    npc_cache_data.py pack res/field/actors/npc_caches ARCHIVE

<n>.json is cache n, which a zone's header names by its "npc_cache" (res/field/zones/), and LoadMModelSystemInfoCache
loads when the player enters the zone (event_mapchange.c): the object codes whose records GetNPCMdlInfoForOBJCODE then
takes from it instead of the model table (field_actor.c). The cache holds a copy of each code's record of the table
(a/0/4/7), which the packer writes from res/field/actors/ as field_actor_data.py does, so a cache follows its actors'
data.json.

Both versions have the same files. npc_cache.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data import field_actor_data  # noqa: E402
from tools.data.datajson import DataError, label, load, load_schema, name, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/0/5/7"
CACHE_MAX = 0x310  # the size of MMSys's actorConfigCache, which FldActSys_ClearCache clears
PADDING = 0xFFFF


def cache_json(data: bytes, table: dict[int, bytes]) -> dict:
    count = struct.unpack_from("<H", data)[0]
    codes = struct.unpack_from(f"<{count}H", data, 2)
    start = 2 * ((count + 1) // 2 * 2 + 2)
    size = field_actor_data.RECORD.size
    padding = struct.unpack_from(f"<{(start - 2 - 2 * count) // 2}H", data, 2 + 2 * count)
    if any(p != PADDING for p in padding) or len(data) != start + size * count:
        raise ValueError("an NPC info cache isn't its codes and their records")
    for i, code in enumerate(codes):
        if data[start + size * i:start + size * (i + 1)] != table.get(code):
            raise ValueError(f"an NPC info cache's record of {name('OBJ_CODE_', code)} isn't the table's")
    return {"$schema": "../npc_cache.schema.json", "codes": [name("OBJ_CODE_", code) for code in codes]}


def cache_bytes(cache: dict, records: dict[str, bytes], where: str) -> bytes:
    codes = cache["codes"]
    missing = [code for code in codes if code not in records]
    if missing:
        raise DataError(f"{where}: {', '.join(map(str, missing))} isn't an object code of generated/obj_codes.txt")
    numbers = [struct.unpack_from("<H", records[code])[0] for code in codes]
    padding = (len(codes) + 1) // 2 * 2 + 1 - len(codes)
    data = struct.pack(f"<H{len(codes)}H{padding}H", len(codes), *numbers, *[PADDING] * padding)
    data += b"".join(records[code] for code in codes)
    if len(data) > CACHE_MAX:
        raise DataError(f"{where}: {len(codes)} codes make {len(data)} bytes, more than the cache's {CACHE_MAX}")
    return data


def dump(files: Path, output: Path):
    table = read_narc((files / field_actor_data.TABLE).read_bytes())[0]
    size = field_actor_data.RECORD.size
    records = {struct.unpack_from("<H", table, o)[0]: table[o:o + size] for o in range(4, len(table), size)}
    members = read_narc((files / ARCHIVE).read_bytes())
    for i, data in enumerate(members):
        write(output / f"{i:03d}.json", cache_json(data, records))
    print(f"wrote {len(members)} NPC info caches to {output}")


def pack(root: Path, output: Path):
    schema = load_schema(root.parent / "npc_cache.schema.json")
    records = field_actor_data.table(root.parent)[0]
    paths = sorted(root.glob("*.json"), key=lambda p: int(p.stem))
    if [int(p.stem) for p in paths] != list(range(len(paths))):
        raise DataError(f"{label(root)}: the caches are numbered from 000 without gaps")
    members = [cache_bytes(load(path, schema), records, label(path)) for path in paths]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/actors/npc_caches/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build ARCID_FIELD_ACTOR_CONFIG from the caches and the actors")
    pack_parser.add_argument("root", type=Path, help="res/field/actors/npc_caches, in the actors' directory")
    pack_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.output)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
