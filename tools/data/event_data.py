#!/usr/bin/env python3
"""The zones' events in data/events/, one JSON file per zone, as pokeplatinum's res/field/events/: dump writes them
from the ROM once, and pack builds the archive of the zones' entities (a/1/2/6) from them, which the build does.

    event_data.py dump extract/b2_us/files data/events
    event_data.py pack data/events ARCHIVE

A zone's events are data/events/<zone>.json, named after its constant: its background events, such as signs
(ZoneBGEntity), its NPCs (ZoneNPC), its warps (ZoneWarp), its triggers (ZoneTrigger), and its init scripts, a map
script table that LoadZoneEntities finds after them. Each is at a grid position or on a rail. data/events/order.json
lists the zones in the order of the archive, which isn't the zones' own, with null where the archive has the game's
4-byte placeholder; zone_data.py gives each zone's header the number of its events from it. events.schema.json
describes each field.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, id_name, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.gen_constants import ordered_names  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

BG = struct.Struct("<HHHH12s")
NPC = struct.Struct("<12HI8s")
WARP = struct.Struct("<HHBBH12s")
TRIGGER = struct.Struct("<5H10sH")
PLACEHOLDER = bytes(4)
NO_ZONE = 0xFFFF


def file_name(zone: str) -> str:
    return zone.removeprefix("ZONE_").lower() + ".json"


def zone_files_from_rom(files: Path) -> dict[int, str]:
    """The zone whose events each entities file is, by the file's number, from the ROM's zone headers."""
    (headers,) = read_narc((files / "a/0/1/2").read_bytes())
    zones = ordered_names("zones")
    return {struct.unpack_from("<H", headers, 0x30 * i + 0x16)[0]: zone for i, zone in enumerate(zones)}


def events_order(root: Path) -> list[str | None]:
    """The zones whose events the entities archive has, in its order, from order.json; null for a placeholder."""
    order = json.loads((root / "order.json").read_text())
    zones = set(ordered_names("zones"))
    named = [zone for zone in order if zone is not None]
    if len(set(named)) != len(named) or set(named) != zones:
        raise DataError("data/events/order.json: every zone is in it once")
    return order


def var_name(var: int):
    """A variable by its name, or in hex; a smaller ID is a number"""
    return id_name("EVENT_WORK_", var) if var >= 0x4000 else var


# Dumping


def grid_or_rail(is_rail: int, union: bytes, grid: tuple, rail: tuple) -> dict:
    """grid and rail: the field names and struct formats of the union's two layouts."""
    names, fmt = rail if is_rail else grid
    values = struct.unpack_from(fmt, union)
    if any(union[struct.calcsize(fmt):]):
        raise ValueError("unused position bytes set")
    return {"rail" if is_rail else "grid": dict(zip(names, values))}


BG_POSITION = ((("x", "y", "z"), "<iii"), (("component", "front", "side"), "<HHh"))
NPC_POSITION = ((("x", "z", "y"), "<HHi"), (("component", "front", "side"), "<HHh"))
WARP_POSITION = ((("x", "y", "z", "width", "height", "unk0a"), "<6H"),
                 (("component", "front", "side", "front_span", "side_span", "param"), "<HHhHHH"))
TRIGGER_POSITION = ((("x", "z", "width", "height", "y"), "<4Hh"),
                    (("component", "front", "side", "front_span", "side_span"), "<HHhHH"))


def init_scripts_json(data: bytes) -> list:
    scripts = []
    pos = 0
    while (kind := struct.unpack_from("<H", data, pos)[0]) != 0:
        if kind == 1:
            table = pos + 6 + struct.unpack_from("<i", data, pos + 2)[0]
            conditions = []
            while (var := struct.unpack_from("<H", data, table)[0]) != 0:
                work_value, script = struct.unpack_from("<HH", data, table + 2)
                conditions.append({"var": var_name(var), "value": work_value, "script": script})
                table += 6
            scripts.append({"type": kind, "conditions": conditions})
        else:
            scripts.append({"type": kind, "script": struct.unpack_from("<I", data, pos + 2)[0]})
        pos += 6
    return scripts


def events_json(data: bytes) -> dict:
    offset = struct.unpack_from("<I", data)[0]
    counts = data[4:8]
    pos = 8
    events = {"$schema": "events.schema.json", "bg_events": [], "npcs": [], "warps": [], "triggers": []}
    for _ in range(counts[0]):
        script, condition, direction, is_rail, union = BG.unpack_from(data, pos)
        events["bg_events"].append({"script": script, "condition": condition, "direction": direction,
                                    **grid_or_rail(is_rail, union, *BG_POSITION)})
        pos += BG.size
    for _ in range(counts[1]):
        (uid, model, movement, kind, flag, script, direction, param0, param1, param2, width, height, is_rail,
         union) = NPC.unpack_from(data, pos)
        events["npcs"].append({"id": uid, "model": model, "movement": movement, "type": kind,
                               "flag": id_name("EVENT_FLAG_", flag), "script": script,
                               "direction": name("DIR_", direction),
                               "params": [param0, param1, param2], "area": {"width": width, "height": height},
                               **grid_or_rail(is_rail, union, *NPC_POSITION)})
        pos += NPC.size
    for _ in range(counts[2]):
        zone, warp, unk4, transition, is_rail, union = WARP.unpack_from(data, pos)
        events["warps"].append({"zone": NO_ZONE if zone == NO_ZONE else name("ZONE_", zone), "warp": warp,
                                "unk4": unk4, "transition": transition, **grid_or_rail(is_rail, union, *WARP_POSITION)})
        pos += WARP.size
    for _ in range(counts[3]):
        script, work_value, var, kind, is_rail, union, unk14 = TRIGGER.unpack_from(data, pos)
        events["triggers"].append({"script": script, "var": var_name(var), "value": work_value, "type": kind,
                                   **grid_or_rail(is_rail, union, *TRIGGER_POSITION), "unk14": unk14})
        pos += TRIGGER.size
    if pos != 4 + offset:
        raise ValueError("the init scripts don't follow the entities")
    events["init_scripts"] = init_scripts_json(data[pos:])
    # One zone's file has more zeros after its init scripts than they need to end at a multiple of 4 bytes
    extra = len(data) - len(events_bytes(events, ""))
    if extra:
        events["extra_padding"] = extra
    return events


def dump(files: Path, output: Path):
    members = read_narc((files / "a/1/2/6").read_bytes())
    zones = zone_files_from_rom(files)
    for number, member in enumerate(members):
        if number not in zones:
            if member != PLACEHOLDER:
                sys.exit(f"entities file {number} belongs to no zone and isn't the placeholder")
            continue
        events = events_json(member)
        if events_bytes(events, "") != member:
            sys.exit(f"entities file {number} doesn't pack back as it was")
        write(output / file_name(zones[number]), events)
    write(output / "order.json", [zones.get(number) for number in range(len(members))])
    print(f"wrote {len(zones)} zones' events to {output}")


# Packing


def position_bytes(event: dict, layouts: tuple, size: int, where: str) -> tuple[int, bytes]:
    if ("grid" in event) == ("rail" in event):
        raise DataError(f"{where}: an event has either a grid or a rail position")
    is_rail = "rail" in event
    names, fmt = layouts[is_rail]
    position = event["rail" if is_rail else "grid"]
    packed = struct.pack(fmt, *(position[n] for n in names))
    return is_rail, packed + bytes(size - len(packed))


def init_scripts_bytes(scripts: list, where: str) -> bytes:
    entries = b""
    tables = b""
    table_start = 6 * len(scripts) + 2
    for i, script in enumerate(scripts):
        if script["type"] == 1:
            relative = table_start + len(tables) - (6 * i + 6)
            entries += struct.pack("<Hi", 1, relative)
            for condition in script["conditions"]:
                tables += struct.pack("<HHH", value(condition["var"], where), condition["value"], condition["script"])
            tables += b"\0\0"
        else:
            entries += struct.pack("<HI", script["type"], script["script"])
    data = entries + b"\0\0" + tables
    return data + bytes(-len(data) % 4)


def events_bytes(events: dict, where: str) -> bytes:
    records = b""
    for event in events["bg_events"]:
        is_rail, union = position_bytes(event, BG_POSITION, 12, where)
        records += BG.pack(event["script"], event["condition"], event["direction"], is_rail, union)
    for npc in events["npcs"]:
        is_rail, union = position_bytes(npc, NPC_POSITION, 8, where)
        records += NPC.pack(npc["id"], npc["model"], npc["movement"], npc["type"], value(npc["flag"], where),
                            npc["script"], value(npc["direction"], where), *npc["params"], npc["area"]["width"],
                            npc["area"]["height"], is_rail, union)
    for warp in events["warps"]:
        is_rail, union = position_bytes(warp, WARP_POSITION, 12, where)
        records += WARP.pack(value(warp["zone"], where), warp["warp"], warp["unk4"], warp["transition"], is_rail, union)
    for trigger in events["triggers"]:
        is_rail, union = position_bytes(trigger, TRIGGER_POSITION, 10, where)
        records += TRIGGER.pack(trigger["script"], trigger["value"], value(trigger["var"], where), trigger["type"],
                                is_rail, union, trigger["unk14"])
    counts = bytes(len(events[key]) for key in ("bg_events", "npcs", "warps", "triggers"))
    init_scripts = init_scripts_bytes(events["init_scripts"], where) + bytes(events.get("extra_padding", 0))
    return struct.pack("<I", 4 + len(records)) + counts + records + init_scripts


def pack(root: Path, output: Path):
    schema = load_schema(root / "events.schema.json")
    members = []
    for zone in events_order(root):
        if zone is None:
            members.append(PLACEHOLDER)
            continue
        path = root / file_name(zone)
        members.append(events_bytes(load(path, schema), label(path)))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/events/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the zone entities archive from data/events/")
    pack_parser.add_argument("root", type=Path)
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
