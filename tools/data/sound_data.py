#!/usr/bin/env python3
"""The sound archive (swan_sound_data.sdat) from data/sound/: dump writes the sources from the ROM once, and pack builds
the archive from them, which the build does.

    sound_data.py dump extract/b2_us/files data/sound
    sound_data.py pack data/sound ARCHIVE

The archive's entries are the constants of the sound lists in data/constants/, each list's in its order: sequences
(sound.txt, SEQ_*), banks (sound_banks.txt), wave archives (sound_wave_archives.txt), players, groups, stream players
and streams. A sequence, bank, wave archive or stream is a directory named after its constant, in a directory of its
kind and one of its category, the constant's word after its kind's prefix: SEQ_BGM_T_01 is sequences/bgm/t_01/,
BANK_SE_SW_JUMP banks/se/sw_jump/, BANK_PV001 banks/pv/pv001/ and WAVE_ARC_PMWB_123
wave_archives/arc/pmwb_123/. Each has data.json, its info
record, which the schemas in data/sound/ describe, and its file: sequence.sseq, bank.sbnk, wave_archive.swar or
stream.strm. A sequence that plays another's file says so with "file". The Pokémon cries' wave archives, the first
of wave_archives.txt after its empty first slot, are each species' cry.wav in data/pokemon/ (pokemon_cries.py).
players.json, groups.json, stream_players.json and streams' are the small kinds' records, by their constants.

The archive has the files of the sequences, banks, wave archives and streams, each kind in its list's order, and the
symbols of every constant, laid out as the game's (sdat.py), so the sources pack it byte for byte.
"""
import os
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
import tools.data.sdat as sdat  # noqa: E402
from tools.data.datajson import DataError, label, load, load_schema, value, write  # noqa: E402
from tools.data.gen_constants import load as load_list  # noqa: E402
from tools.data.pokemon_cries import cry_folder, read_wav, wav_bytes  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
ARCHIVE = "swan_sound_data.sdat"
POKEMON = ROOT / "data/pokemon"
# Each kind's constant list, the prefixes of its constants, its directory and its file's name
KIND_LISTS = {"sequences": "sound", "sequence_archives": None, "banks": "sound_banks",
              "wave_archives": "sound_wave_archives", "players": "sound_players", "groups": "sound_groups",
              "stream_players": "sound_stream_players", "streams": "sound_streams"}
FILE_KINDS = {"sequences": (("SEQ_",), "sequence.sseq"), "banks": (("BANK_",), "bank.sbnk"),
              "wave_archives": (("WAVE_",), "wave_archive.swar"), "streams": (("STRM_",), "stream.strm")}
SMALL_KINDS = ("players", "groups", "stream_players")
# The wave archives of the Pokémon cries, the slots after the empty first, and the cries' wave format: PCM8, unlooped
CRIES = range(1, 691)
CRY_FORMAT = 0
# The buffer that the game plays a cry's wave archive from
CRY_BUFFER = 0x5C30
TIMER_CLOCK = 16756991
SEQUENCE = struct.Struct("<HHHBBBBH")
BANK = struct.Struct("<HH4H")
WAVE_ARCHIVE = struct.Struct("<HH")
PLAYER = struct.Struct("<BBHI")
STREAM = struct.Struct("<HHBBB5s")
NO_WAVE_ARCHIVE = 0xFFFF
GROUP_SEQUENCE = 0


def slots(kind: str) -> list:
    """Returns a kind's slots, each its constant or None, in the archive's order."""
    if KIND_LISTS[kind] is None:
        return []
    found: dict[int, str] = {}
    for constant, number in load_list(KIND_LISTS[kind]).items():
        found.setdefault(number, constant)
    return [found.get(number) for number in range(max(found) + 1)] if found else []


def entry_dir(root: Path, kind: str, constant: str) -> Path:
    """Returns the directory of a sequence, bank, wave archive or stream: its kind's, its category's and its own."""
    name = constant
    for prefix in FILE_KINDS[kind][0]:
        name = name.removeprefix(prefix)
    category, _, rest = name.partition("_")
    letters = category.rstrip("0123456789")
    if not rest or letters != category:
        # A first word with a number, as BANK_PV001 and BANK_PV492_1: its letters are the category
        return root / kind / letters.lower() / name.lower()
    return root / kind / category.lower() / rest.lower()


def relative_schema(root: Path, path: Path, schema: str) -> str:
    return os.path.relpath(root / schema, path.parent).replace(os.sep, "/")


def channels(mask: int) -> list[int]:
    return [channel for channel in range(16) if mask >> channel & 1]


def cry_swar(rate: int, samples: bytes) -> bytes:
    """Returns the wave archive of a cry: one unlooped PCM8 wave."""
    wave = (struct.pack("<BBHHHI", CRY_FORMAT, 0, rate, TIMER_CLOCK // rate, 0, (len(samples) + 3) // 4) + samples
            + bytes(-len(samples) % 4))
    data = struct.pack("<4sI", b"DATA", 0x38 - 0x10 + 8 + len(wave)) + bytes(32) + struct.pack("<II", 1, 0x40) + wave
    return struct.pack("<4sHHIHH", b"SWAR", 0xFEFF, 0x0100, 0x10 + len(data), 0x10, 1) + data


def dump(files: Path, root: Path):
    archive = sdat.read((files / ARCHIVE).read_bytes())
    names = {kind: slots(kind) for kind in sdat.KINDS}
    for kind in sdat.KINDS:
        if names[kind] + [None] * (len(archive["names"][kind]) - len(names[kind])) != archive["names"][kind]:
            raise SystemExit(f"{KIND_LISTS[kind]}.txt doesn't name the archive's {kind}")
    records = archive["records"]
    owners: dict[int, str] = {}
    for kind in FILE_KINDS:
        for index, (constant, record) in enumerate(zip(names[kind], records[kind])):
            if record is None:
                continue
            file_id = struct.unpack_from("<H", record)[0]
            if kind == "wave_archives" and index in CRIES:
                continue
            path = entry_dir(root, kind, constant)
            path.mkdir(parents=True, exist_ok=True)
            data: dict = {"$schema": relative_schema(root, path / "data.json", f"{kind[:-1]}.schema.json")}
            if file_id in owners:
                data["file"] = owners[file_id]
            else:
                owners[file_id] = constant
                (path / FILE_KINDS[kind][1]).write_bytes(archive["files"][file_id])
            if kind == "sequences":
                _, _, bank, volume, channel_priority, player_priority, player, _ = SEQUENCE.unpack(record)
                data.update({"bank": names["banks"][bank], "volume": volume, "channel_priority": channel_priority,
                             "player_priority": player_priority, "player": names["players"][player]})
            elif kind == "banks":
                archives = BANK.unpack(record)[2:]
                data["wave_archives"] = [None if number == NO_WAVE_ARCHIVE else names["wave_archives"][number]
                                         for number in archives]
            elif kind == "streams":
                _, _, volume, priority, player, _ = STREAM.unpack(record)
                data.update({"volume": volume, "priority": priority, "player": names["stream_players"][player]})
            if len(data) > 1:
                write(path / "data.json", data)
    for cry_id in CRIES:
        swar = archive["files"][struct.unpack_from("<H", records["wave_archives"][cry_id])[0]]
        rate, samples = cry_wave(swar)
        if cry_swar(rate, samples) != swar:
            raise SystemExit(f"cry {cry_id}'s wave archive isn't one the packer writes again")
        (cry_folder(POKEMON, cry_id) / "cry.wav").write_bytes(wav_bytes(rate, samples))
    write(root / "players.json", {"$schema": "players.schema.json", **{
        constant: {"max_sequences": count, "channels": channels(mask), "heap_size": heap}
        for constant, record in zip(names["players"], records["players"])
        for count, _, mask, heap in [PLAYER.unpack(record)]}})
    write(root / "stream_players.json", {"$schema": "players.schema.json", **{
        constant: {"channels": list(record[1:1 + record[0]])}
        for constant, record in zip(names["stream_players"], records["stream_players"])}})
    groups = {"$schema": "groups.schema.json"}
    for constant, record in zip(names["groups"], records["groups"]):
        entries = []
        for offset in range(4, len(record), 8):
            kind, load_flags, _, index = struct.unpack_from("<BBHI", record, offset)
            if kind != GROUP_SEQUENCE:
                raise SystemExit(f"{constant} loads something other than a sequence")
            entries.append({"sequence": names["sequences"][index], "load": load_flags})
        groups[constant] = entries
    write(root / "groups.json", groups)
    print(f"wrote the sound archive's sources to {root}")


def cry_wave(swar: bytes) -> tuple[int, bytes]:
    fmt, loop, rate, _, start, length = struct.unpack_from("<BBHHHI", swar, 0x40)
    if fmt != CRY_FORMAT or loop or start or 0x40 + 12 + length * 4 != len(swar):
        raise SystemExit("a cry's wave archive isn't one unlooped PCM8 wave")
    return rate, swar[0x4C:]


def check(condition: bool, path: Path, message: str):
    if not condition:
        raise DataError(f"{label(path)}: {message}")


def pack(root: Path, output: Path):
    names = {kind: slots(kind) for kind in sdat.KINDS}
    numbers = {kind: {constant: index for index, constant in enumerate(slot) if constant}
               for kind, slot in names.items()}

    def number(kind: str, constant, path: Path) -> int:
        check(constant in numbers[kind], path, f"{constant} is no {kind[:-1].replace('_', ' ')}")
        return numbers[kind][constant]

    files: list[bytes] = []
    file_ids: dict[str, int] = {}
    entries: dict[str, dict] = {}
    for kind in FILE_KINDS:
        schema = load_schema(root / f"{kind[:-1]}.schema.json")
        entries[kind] = {}
        for index, constant in enumerate(names[kind]):
            if constant is None:
                continue
            if kind == "wave_archives" and index in CRIES:
                path = cry_folder(POKEMON, index) / "cry.wav"
                swar = cry_swar(*read_wav(path))
                check(len(swar) <= CRY_BUFFER, path, f"too long: its wave archive must fit the game's {CRY_BUFFER:#x}-byte "
                      "buffer")
                file_ids[constant] = len(files)
                files.append(swar)
                entries[kind][constant] = {}
                continue
            path = entry_dir(root, kind, constant)
            data = load(path / "data.json", schema) if (path / "data.json").exists() else {}
            source = path / FILE_KINDS[kind][1]
            if "file" not in data:
                check(source.is_file(), source, "missing")
                file_ids[constant] = len(files)
                files.append(source.read_bytes())
            entries[kind][constant] = data
    records: dict[str, list] = {kind: [None] * len(names[kind]) for kind in sdat.KINDS}
    for kind in FILE_KINDS:
        for constant, data in entries[kind].items():
            path = entry_dir(root, kind, constant)
            owner = data.get("file", constant)
            check(owner in file_ids, path, f"{owner} has no file of its own to play")
            file_id = file_ids[owner]
            if kind == "sequences":
                record = SEQUENCE.pack(file_id, 0, number("banks", data["bank"], path), data["volume"],
                                       data["channel_priority"], data["player_priority"],
                                       number("players", data["player"], path), 0)
            elif kind == "banks":
                record = BANK.pack(file_id, 0, *(NO_WAVE_ARCHIVE if archive is None else
                                                 number("wave_archives", archive, path)
                                                 for archive in data["wave_archives"]))
            elif kind == "wave_archives":
                record = WAVE_ARCHIVE.pack(file_id, 0)
            else:
                record = STREAM.pack(file_id, 0, data["volume"], data["priority"],
                                     number("stream_players", data["player"], path), bytes(5))
            records[kind][numbers[kind][constant]] = record
    players = load(root / "players.json", load_schema(root / "players.schema.json"))
    for constant in names["players"]:
        player = players[constant]
        records["players"][numbers["players"][constant]] = PLAYER.pack(
            player["max_sequences"], 0, sum(1 << channel for channel in player["channels"]), player["heap_size"])
    stream_players = load(root / "stream_players.json", load_schema(root / "players.schema.json"))
    for constant in names["stream_players"]:
        player_channels = stream_players[constant]["channels"]
        records["stream_players"][numbers["stream_players"][constant]] = (
            bytes([len(player_channels), *player_channels]) + b"\xff" * (16 - len(player_channels)) + bytes(7))
    groups = load(root / "groups.json", load_schema(root / "groups.schema.json"))
    for constant in names["groups"]:
        members = groups[constant]
        records["groups"][numbers["groups"][constant]] = struct.pack("<I", len(members)) + b"".join(
            struct.pack("<BBHI", GROUP_SEQUENCE, member["load"], 0,
                        number("sequences", member["sequence"], root / "groups.json")) for member in members)
    archive = {"names": names, "records": records, "files": files}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(sdat.write(archive))


if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "dump":
        dump(Path(sys.argv[2]), Path(sys.argv[3]))
    elif len(sys.argv) == 4 and sys.argv[1] == "pack":
        try:
            pack(Path(sys.argv[2]), Path(sys.argv[3]))
        except DataError as error:
            sys.exit(f"error: {error}")
    else:
        raise SystemExit(__doc__)
