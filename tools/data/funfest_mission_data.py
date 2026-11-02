#!/usr/bin/env python3
"""The Funfest Missions' data in res/funfest/ (ARCID_FMISSION_DATA, a/2/8/9): dump writes it from the ROM once, and pack
builds the archive from it, which the build does.

    funfest_mission_data.py dump extract/b2_us/files res/funfest
    funfest_mission_data.py pack res/funfest ARCHIVE

- missions.json is the FEST_MISSION_COUNT missions, each its FestMissionHeader of file 0 and its FestMissionParams of
  file 1 (field/fest_mission_data.h), which FestMission_Load copies into a FestMissionConfig: its messages, of
  res/text/system/get_text_file_for_fest_missions.txt by ID, its target, items and flag by their constants, and what
  each of its four levels asks.
- species_lists.json is files 2 to 21, the species that a people mission picks from (FesMissionPeople_LoadSpecies):
  file 2 plus its variant for target 8, file 20 for target 5.

Both versions have the same file. missions.schema.json and species_lists.schema.json describe the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_ids import message_numbers  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
ARCHIVE = "a/2/8/9"
MESSAGES = ROOT / "res" / "text" / "system" / "get_text_file_for_fest_missions.txt"

MISSION_COUNT = 55  # FEST_MISSION_COUNT
HEADER = struct.Struct("<BBH")  # FestMissionHeader
PARAMS = struct.Struct("<8B6HI")  # FestMissionParams, before its levels
LEVEL = struct.Struct("<HBB")
LEVEL_COUNT = 4
FIRST_LIST = 2  # the species lists' first file
LIST_COUNT = 20
KIND_SPECIES = 1  # a header kind whose target is a species, as data_ov027_021711c0 says


def messages() -> list[str]:
    return [message for message, _ in message_numbers(MESSAGES)]


def flag(number: int):
    return name("EVENT_FLAG_", number) if number else 0


def mission_json(header: bytes, params: bytes, texts: list[str]) -> dict:
    kind, unk01, header_unk02 = HEADER.unpack(header)
    (unk00, params_unk01, extra_seconds, unk03, unk04, target_kind, message, result_message, second_item, target,
     variant, reward, unk10, required_flag, unk14) = PARAMS.unpack_from(params)
    levels = [LEVEL.unpack_from(params, PARAMS.size + LEVEL.size * i) for i in range(LEVEL_COUNT)]
    return {"kind": kind, "message": texts[message], "result_message": texts[result_message],
            "target": name("SPECIES_", target) if kind == KIND_SPECIES else target, "target_kind": target_kind,
            "variant": variant, "reward_item": name("ITEM_", reward), "second_item": name("ITEM_", second_item),
            "required_flag": flag(required_flag), "extra_seconds_index": extra_seconds,
            "levels": [{"seconds": seconds, "count": count, "unk3": unk3} for seconds, count, unk3 in levels],
            "unk00": unk00, "unk03": unk03, "unk04": unk04, "unk10": unk10, "unk14": unk14,
            "header_unk01": unk01, "header_unk02": header_unk02, "params_unk01": params_unk01}


def mission_bytes(entry: dict, texts: dict[str, int], where: str) -> tuple[bytes, bytes]:
    def text(key: str) -> int:
        if entry[key] not in texts:
            raise DataError(f"{where}: {entry[key]} is not a message of {label(MESSAGES)}")
        return texts[entry[key]]

    header = HEADER.pack(entry["kind"], entry["header_unk01"], entry["header_unk02"])
    params = PARAMS.pack(entry["unk00"], entry["params_unk01"], entry["extra_seconds_index"], entry["unk03"],
                         entry["unk04"], entry["target_kind"], text("message"), text("result_message"),
                         value(entry["second_item"], where), value(entry["target"], where), entry["variant"],
                         value(entry["reward_item"], where), entry["unk10"], value(entry["required_flag"], where),
                         entry["unk14"])
    levels = b"".join(LEVEL.pack(level["seconds"], level["count"], level["unk3"]) for level in entry["levels"])
    return header, params + levels


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    size = PARAMS.size + LEVEL.size * LEVEL_COUNT
    if (len(members) != FIRST_LIST + LIST_COUNT or len(members[0]) != HEADER.size * MISSION_COUNT
            or len(members[1]) != size * MISSION_COUNT or any(len(m) % 2 for m in members[FIRST_LIST:])):
        raise ValueError("the missions' archive isn't 55 missions and 20 species lists")
    texts = messages()
    missions = [mission_json(members[0][HEADER.size * i:HEADER.size * (i + 1)], members[1][size * i:size * (i + 1)],
                             texts) for i in range(MISSION_COUNT)]
    write(output / "missions.json", {"$schema": "missions.schema.json", "missions": missions})
    lists = [[name("SPECIES_", s) for s in struct.unpack(f"<{len(m) // 2}H", m)] for m in members[FIRST_LIST:]]
    write(output / "species_lists.json", {"$schema": "species_lists.schema.json", "lists": lists})
    print(f"wrote {len(missions)} missions and {len(lists)} species lists to {output}")


def pack(root: Path, output: Path):
    path = root / "missions.json"
    missions = load(path, load_schema(root / "missions.schema.json"))["missions"]
    texts = {message: number for message, number in message_numbers(MESSAGES)}
    packed = [mission_bytes(entry, texts, f"{label(path)}: mission {i}") for i, entry in enumerate(missions)]
    lists_path = root / "species_lists.json"
    lists = load(lists_path, load_schema(root / "species_lists.schema.json"))["lists"]
    members = [b"".join(h for h, _ in packed), b"".join(p for _, p in packed)]
    members += [struct.pack(f"<{len(species)}H", *(value(s, f"{label(lists_path)}: list {i}") for s in species))
                for i, species in enumerate(lists)]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/funfest/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the missions' archive")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.archive)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
