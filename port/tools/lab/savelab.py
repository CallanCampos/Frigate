#!/usr/bin/env python3
"""The save lab's minter and reader: mints a Black 2 or White 2 save from a save the game wrote and a recipe written in
the game's own constant names, and reads a save back as facts in the same names (port/tools/lab/README.md). It serves
the game's saves: the main save's player block (0x1c: where the player stands and where the next CONTINUE puts them),
its event work block (0x2d: the story's flags and work values), the trainer data's play time (0x1b), the trainer game
info's money and badges (0x34), and, read only, the party (0x1a). Neither the game nor Frigate changes: a minted save is
a save the game loads as its own, on Frigate and on melonDS alike.

    savelab.py mint RECIPE --base SAVE --out SAVE [--force]
    savelab.py read SAVE [--flag NAME]... [--work NAME]... [--all-flags]

mint applies RECIPE (a file, or - for the standard input) to the save in --base and writes the result to --out, which
it never overwrites without --force, then prints what each line set, the two SHA-1s and how many bytes changed, as
JSON. read prints a save's facts as JSON: where the player saved and the two spawns, the play time, money and badges,
the party decoded, every event work value that isn't 0, the flags asked for (EVENT_FLAG_CONTINUE_SCRIPT always) and
how many flags are set, or with --all-flags the name of each. Both exit 1 with a "savelab:" line when they refuse a
recipe or a save, and 2 for bad options.

A recipe is a text file of lines, one verb each; # starts a comment. Every argument is a number or a name of the build's
generated constants headers (build/include/generated/constants/*.h, which the ROM build's `ninja constants_headers`
and the host build's configure write from generated/*.txt), so the lab and the game number everything alike:

    base NAME                        the oracle save (port/tools/oracle/oracle.py SAVES) the recipe is minted from:
                                     --base must have one of the SHA-1s that SAVES pins for it
    place ZONE warp N                the player arrives through the zone's own warp N (res/field/events/<zone>.json)
    place ZONE at X Z [Y] [facing D] the player stands on the ground of tile (X, Z), facing D (WARP_DIR_DOWN); Y, the
                                     level in tiles (0), picks one only where the tile has more than one
    place ZONE fly                   the player stands where Fly lands in the zone (res/field/zones/<zone>.json)
    flag NAME set|clear              a saved event flag (EVENT_FLAG_*)
    work NAME VALUE                  a saved event work value (EVENT_WORK_*), 0 to 65535 or a name
    money N                          the money, 0 to TRAINER_GAME_INFO_CASH_MAX (9,999,999)
    badges N                         the first N of the 8 badges
    playtime H:MM:SS                 the play time, up to PLAYTIME_MAX_HOURS (999) hours

Each target is set once per recipe, and a recipe places the player at most once. place uses the game's own
continue-at-spawn path, the one the Hall of Fame takes to put the player back in their room
(res/field/scripts/global_9600.s): the script command FieldSetNextZone writes PlayerSave.nextSpawn
(src/ov012/scrcmd_game_state.c) and FlagSet sets EVENT_FLAG_CONTINUE_SCRIPT. On CONTINUE, EventFieldContinue_Create
reads that flag and clears it (src/ov035/event_mapchange.c), and its callback deletes the actors the save restored,
loads the spawn's zone and runs FieldMapControl_InitSpawn, the routine every door runs: a warp is resolved through the
destination zone's own warp table (SetupZoneWarpArrival, src/ov012/eventdata_system.c), and the zone's NPCs are spawned
from its events as its flags decide. So place writes nextSpawn as the game's constructors write it
(SetupZoneSpawnInfoWarp, CreateZoneChangeData, LoadZoneSpawnInfoCheckRail; src/system/zone_spawn.c), sets the flag,
clears EVENT_WORK_CONTINUE_SCRIPT so that no continue script runs, and writes SaveLocation, which the start menu shows,
as the player state FieldMapControl_InitSpawn sets from that spawn. A rail warp's SaveLocation keeps only its zone and
facing: the game puts the player on the rail from nextSpawn before anything reads the position. The game sets the
player's height itself, on the ground of the tile (CheckRecalcActorY, src/ov012/field_actor_tool.c): at's Y only picks
the level nearest it where the tile has more than one (FieldG3DMapper_GetTerrain, src/ov036/field_g3d_mapper.c). at and
fly take grid positions, so at refuses the zones the game gives a rail system (ZONE_RAIL_ASSIGNMENTS,
src/system/zonedata.c), fly the zones whose Fly position is on a rail (ZONES_WITH_RAILS, there too) and those whose zone
data has no Fly position, only (0, 0, 0); place those by warp. place refuses a base saved while cycling, surfing or
diving, a state the arrival would keep. The recipe's verbs leave EVENT_FLAG_CONTINUE_SCRIPT and
EVENT_WORK_CONTINUE_SCRIPT to place.

The layout of the backup image comes from port/tools/oracle/saveimage.py: find_blocks discovers the main save's 73
blocks from their footers and CRC table, and validate_image checks both copies. A base must pass it, have its two copies
of the main save alike, as the game's SAVE writes them, and have the game's sizes for the blocks the lab reads; else
it is refused. Every value is written to both copies; then each written block's footer CRC and CRC table entry and the
save footer's CRC of that table are recomputed in each copy (SaveData_FillFooter, src/gfl/backup_system.c), and the
result must pass validate_image against the base. The save counts are left alone, so a minted save is laid out as the
game writes one. The fields' offsets are the decomp's headers' (include/save/*.h, include/field/zone.h,
include/pml/poke_party.h), which port/tests/lab/test_savelab.py holds LAYOUT to as the compiler lays them out. The
party is decoded by port/tools/oracle/savedecode.py, the one copy of the save's crypto: each Pokémon decrypted as
PokeParty_DecryptPkm does (src/system/poke_tool.c), its offsets held to the headers by
port/tests/oracle/test_savedecode.py. The same base, recipe and tool give the same bytes.
Standard library only.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import struct
import sys
import tempfile
from collections.abc import Sequence
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE.parent / "oracle"))
sys.path.insert(0, str(REPO_ROOT))

import savedecode  # noqa: E402
import saveimage  # noqa: E402
from tools.data.event_data import file_name as events_file_name  # noqa: E402
from tools.data.zone_data import file_name as zone_file_name  # noqa: E402

CONSTANTS_DIR = REPO_ROOT / "build" / "include" / "generated" / "constants"
EVENTS_DIR = REPO_ROOT / "res" / "field" / "events"
ZONES_DIR = REPO_ROOT / "res" / "field" / "zones"
ZONEDATA_C = REPO_ROOT / "src" / "system" / "zonedata.c"

# include/save/save_control.h: the main save's blocks that the lab reads or writes
SAVE_BLOCK_PARTY = 0x1a
SAVE_BLOCK_TRAINER_DATA = 0x1b
SAVE_BLOCK_PLAYER = 0x1c
SAVE_BLOCK_EVENT_WORK = 0x2d
SAVE_BLOCK_TRAINER_GAME_INFO = 0x34
SAVE_BLOCK_MAX = 73

# include/save/player_savedata.h, PlayerSave: block 0x1c
PLAYER_SAVE_SIZE = 0xa8
PLAYER_NOW_SPAWN = 0x00
PLAYER_NEXT_SPAWN = 0x38
PLAYER_LOCATION = 0x80
# SaveLocation, where the player saved: the player state that SaveControl_SavePlayerState writes
# (src/system/player_savedata.c)
LOCATION_ZONE = 0x00
LOCATION_POS = 0x04
LOCATION_RAIL_POS = 0x10
LOCATION_ROTATION = 0x18  # unk18, PlayerState.rotation
LOCATION_IS_RAIL = 0x1a  # unk1A, PlayerState.isRail
LOCATION_EX_STATE = 0x1b
# include/field/zone.h, ZoneSpawnInfo, and RailPosition in its position union
SPAWN_SIZE = 0x1c
SPAWN_CHANGE_TYPE = 0x00
SPAWN_ZONE = 0x04
SPAWN_WARP = 0x06
SPAWN_WARP_DIR = 0x08
SPAWN_POS_WEIGHT_BITS = 0x0a
SPAWN_IS_RAIL = 0x0c
SPAWN_POS = 0x10
# u32 changeType, s16 zoneId, s16 warpId, s16 warpDir, u16 posWeightBits, BOOL isRail, then three s32 or a RailPosition
SPAWN_FORMAT = "<IhhhHi12s"
RAIL_POSITION_FORMAT = "<HBBhH"  # componentId, componentIsLine, railDirection, posSide, posFront
ZONE_SPAWN_CHANGE_TYPE_WARP = 0
ZONE_SPAWN_CHANGE_TYPE_POSITION = 1
ZONE_SPAWN_CHANGE_TYPES = ("ZONE_SPAWN_CHANGE_TYPE_WARP", "ZONE_SPAWN_CHANGE_TYPE_POSITION",
                           "ZONE_SPAWN_CHANGE_TYPE_WARP_ARRIVAL", "ZONE_SPAWN_CHANGE_TYPE_3")
# include/field/zone.h: a warp's direction, and the facing of a spawn
WARP_DIRS = {"WARP_DIR_UP": 1, "WARP_DIR_DOWN": 2, "WARP_DIR_LEFT": 3, "WARP_DIR_RIGHT": 4}
WARP_DIR_DOWN = WARP_DIRS["WARP_DIR_DOWN"]
# ConvWarpDirToAngle (src/ov035/event_mapchange.c): the player's rotation for a facing; any other facing is up's
WARP_DIR_ANGLES = {1: 0x0000, 3: 0x4000, 2: 0x8000, 4: 0xc000}
# include/field/player_state.h: walking; the others are cycling, surfing and diving
FLD_PLAYER_EXSTATE_NONE = 0
EX_STATES = ("FLD_PLAYER_EXSTATE_NONE", "FLD_PLAYER_EXSTATE_CYCLING", "FLD_PLAYER_EXSTATE_SURF",
             "FLD_PLAYER_EXSTATE_DIVE")
# FX32_SHIFT (NitroSDK's fx), and a tile's 16 world units: a world position is a tile's << 16
FX32_SHIFT = 12
TILE_SHIFT = FX32_SHIFT + 4

# include/save/event_work.h, EventWork: block 0x2d. Work W is works[W - EVENT_WORK_0x4000]; flag N is bit N % 8 of
# flags[N / 8], for 0 < N < EVENT_FLAG_SAVED_COUNT (EventWork_GetFlagBytePtr, src/system/event_work.c)
EVENT_WORK_SIZE = 0x4de
EVENT_WORK_WORKS = 0x000
EVENT_WORK_FLAGS = 0x35e
EVENT_WORK_COUNT = 431
EVENT_FLAG_SAVED_COUNT = 3064
EVENT_WORK_FIRST = 0x4000

# include/save/trainer_data_savedata.h, TrainerDataSave: block 0x1b; include/save/playtime.h, PlayTime
TRAINER_DATA_SIZE = 0xb0
TRAINER_DATA_PLAY_TIME = 0x24
PLAY_TIME_HOURS = 0x0
PLAY_TIME_MINUTES = 0x2
PLAY_TIME_SECONDS = 0x3
PLAYTIME_MAX_HOURS = 999

# include/save/trainer_game_info.h, TrainerGameInfoSave: block 0x34. Badge B is bit B of badges (addBadge,
# src/system/trainer_game_info_savedata.c)
TRAINER_GAME_INFO_SIZE = 0xf0
TRAINER_GAME_INFO_CASH = 0x0
TRAINER_GAME_INFO_BADGES = 0x4
TRAINER_GAME_INFO_CASH_MAX = 9999999
TRAINER_GAME_INFO_GYM_COUNT = 8

# include/pml/poke_party.h: where the reader finds the party's count and each Pokémon of block 0x1a, which
# port/tools/oracle/savedecode.py decodes, and the block's size; the module's values, held to the headers there
POKE_PARTY_SIZE = savedecode.POKE_PARTY_SIZE
POKE_PARTY_COUNT = savedecode.LAYOUT["PokeParty.count"]
POKE_PARTY_PKMS = savedecode.POKE_PARTY_PKMS
PARTY_SIZE = savedecode.PARTY_SIZE
PARTY_PKM_SIZE = savedecode.PARTY_PKM_SIZE

# The footers that src/gfl/backup_system.c writes: BlockFooter {u16 count; u16 crc} after each block, and the CRC in
# SaveFooter {u32 count, size, magic; u16 unk; u16 crc}, which covers the table of the blocks' CRCs
BLOCK_FOOTER_CRC = 2
SAVE_FOOTER_CRC = 14


def round_up(size: int, alignment: int = 4) -> int:
    return (size + alignment - 1) // alignment * alignment


# Each block's size in the save: its struct's, rounded up to 4 bytes (SaveData_BuildTable, src/gfl/savedata.c)
BLOCK_SIZES = {
    SAVE_BLOCK_PARTY: round_up(POKE_PARTY_SIZE),
    SAVE_BLOCK_TRAINER_DATA: round_up(TRAINER_DATA_SIZE),
    SAVE_BLOCK_PLAYER: round_up(PLAYER_SAVE_SIZE),
    SAVE_BLOCK_EVENT_WORK: round_up(EVENT_WORK_SIZE),
    SAVE_BLOCK_TRAINER_GAME_INFO: round_up(TRAINER_GAME_INFO_SIZE),
}
BLOCK_STRUCTS = {
    SAVE_BLOCK_PARTY: "PokeParty",
    SAVE_BLOCK_TRAINER_DATA: "TrainerDataSave",
    SAVE_BLOCK_PLAYER: "PlayerSave",
    SAVE_BLOCK_EVENT_WORK: "EventWork",
    SAVE_BLOCK_TRAINER_GAME_INFO: "TrainerGameInfoSave",
}

# What the decomp's headers give for every offset, size and number above, under the names that
# port/tests/lab/savelab_layout.c prints them with as the compiler lays the headers out: the test holds one to the other
LAYOUT = {
    "SAVE_BLOCK_PARTY": SAVE_BLOCK_PARTY,
    "SAVE_BLOCK_TRAINER_DATA": SAVE_BLOCK_TRAINER_DATA,
    "SAVE_BLOCK_PLAYER": SAVE_BLOCK_PLAYER,
    "SAVE_BLOCK_EVENT_WORK": SAVE_BLOCK_EVENT_WORK,
    "SAVE_BLOCK_TRAINER_GAME_INFO": SAVE_BLOCK_TRAINER_GAME_INFO,
    "SAVE_BLOCK_MAX": SAVE_BLOCK_MAX,
    "sizeof(PlayerSave)": PLAYER_SAVE_SIZE,
    "PlayerSave.nowSpawn": PLAYER_NOW_SPAWN,
    "PlayerSave.nextSpawn": PLAYER_NEXT_SPAWN,
    "PlayerSave.location": PLAYER_LOCATION,
    "SaveLocation.zoneId": LOCATION_ZONE,
    "SaveLocation.pos": LOCATION_POS,
    "SaveLocation.railPos": LOCATION_RAIL_POS,
    "SaveLocation.unk18": LOCATION_ROTATION,
    "SaveLocation.unk1A": LOCATION_IS_RAIL,
    "SaveLocation.exState": LOCATION_EX_STATE,
    "sizeof(ZoneSpawnInfo)": SPAWN_SIZE,
    "ZoneSpawnInfo.changeType": SPAWN_CHANGE_TYPE,
    "ZoneSpawnInfo.zoneId": SPAWN_ZONE,
    "ZoneSpawnInfo.warpId": SPAWN_WARP,
    "ZoneSpawnInfo.warpDir": SPAWN_WARP_DIR,
    "ZoneSpawnInfo.posWeightBits": SPAWN_POS_WEIGHT_BITS,
    "ZoneSpawnInfo.isRail": SPAWN_IS_RAIL,
    "ZoneSpawnInfo.pos": SPAWN_POS,
    "sizeof(RailPosition)": struct.calcsize(RAIL_POSITION_FORMAT),
    "RailPosition.componentId": 0,
    "RailPosition.componentIsLine": 2,
    "RailPosition.railDirection": 3,
    "RailPosition.posSide": 4,
    "RailPosition.posFront": 6,
    "ZONE_SPAWN_CHANGE_TYPE_WARP": ZONE_SPAWN_CHANGE_TYPE_WARP,
    "ZONE_SPAWN_CHANGE_TYPE_POSITION": ZONE_SPAWN_CHANGE_TYPE_POSITION,
    "ZONE_SPAWN_CHANGE_TYPE_WARP_ARRIVAL": ZONE_SPAWN_CHANGE_TYPES.index("ZONE_SPAWN_CHANGE_TYPE_WARP_ARRIVAL"),
    "ZONE_SPAWN_CHANGE_TYPE_3": ZONE_SPAWN_CHANGE_TYPES.index("ZONE_SPAWN_CHANGE_TYPE_3"),
    **WARP_DIRS,
    **{name: value for value, name in enumerate(EX_STATES)},
    "sizeof(EventWork)": EVENT_WORK_SIZE,
    "EventWork.works": EVENT_WORK_WORKS,
    "EventWork.flags": EVENT_WORK_FLAGS,
    "EVENT_WORK_COUNT": EVENT_WORK_COUNT,
    "EVENT_FLAG_SAVED_COUNT": EVENT_FLAG_SAVED_COUNT,
    "sizeof(TrainerDataSave)": TRAINER_DATA_SIZE,
    "TrainerDataSave.playTime": TRAINER_DATA_PLAY_TIME,
    "PlayTime.hours": PLAY_TIME_HOURS,
    "PlayTime.minutes": PLAY_TIME_MINUTES,
    "PlayTime.seconds": PLAY_TIME_SECONDS,
    "PLAYTIME_MAX_HOURS": PLAYTIME_MAX_HOURS,
    "sizeof(TrainerGameInfoSave)": TRAINER_GAME_INFO_SIZE,
    "TrainerGameInfoSave.cash": TRAINER_GAME_INFO_CASH,
    "TrainerGameInfoSave.badges": TRAINER_GAME_INFO_BADGES,
    "TRAINER_GAME_INFO_CASH_MAX": TRAINER_GAME_INFO_CASH_MAX,
    "TRAINER_GAME_INFO_GYM_COUNT": TRAINER_GAME_INFO_GYM_COUNT,
    "sizeof(PokeParty)": POKE_PARTY_SIZE,
    "PokeParty.count": POKE_PARTY_COUNT,
    "PokeParty.pkms": POKE_PARTY_PKMS,
    "PARTY_SIZE": PARTY_SIZE,
    "sizeof(PartyPkm)": PARTY_PKM_SIZE,
    # The Pokémon's fields that the reader reports, as savedecode decodes them (its LAYOUT, whose bitfields are a first
    # bit and a width: savelab_layout.c prints BoxPkm.partyDecrypted as a byte and a mask)
    "BoxPkm.pid": savedecode.LAYOUT["BoxPkm.pid"],
    "BoxPkm.partyDecrypted.byte": savedecode.LAYOUT["BoxPkm.partyDecrypted.bit"] // 8,
    "BoxPkm.partyDecrypted.mask": 1 << savedecode.LAYOUT["BoxPkm.partyDecrypted.bit"] % 8,
    "BoxPkm.checksum": savedecode.LAYOUT["BoxPkm.checksum"],
    "BoxPkm.contentBuffer": savedecode.LAYOUT["BoxPkm.contentBuffer"],
    "sizeof(PkmBuffer)": savedecode.LAYOUT["sizeof(PkmBuffer)"],
    "PartyPkm.statusCond": savedecode.LAYOUT["PartyPkm.statusCond"],
    "PartyPkm.level": savedecode.LAYOUT["PartyPkm.level"],
    "PartyPkm.nowHP": savedecode.LAYOUT["PartyPkm.nowHP"],
    "PartyPkm.maxHP": savedecode.LAYOUT["PartyPkm.maxHP"],
    "PkmBlockA.species": savedecode.LAYOUT["PkmBlockA.species"],
    "PkmBlockA.item": savedecode.LAYOUT["PkmBlockA.item"],
    "PkmBlockA.exp": savedecode.LAYOUT["PkmBlockA.exp"],
    "PkmBlockB.moves": savedecode.LAYOUT["PkmBlockB.moves"],
}

# The generated constants headers' line for a constant: #define NAME VALUE
DEFINE = re.compile(r"^\s*#define\s+([A-Za-z_]\w*)\s+(0x[0-9a-fA-F]+|-?[0-9]+)\s*$")
# PlayTime as a recipe writes it
PLAYTIME = re.compile(r"^([0-9]+):([0-5][0-9]):([0-5][0-9])$")


class LabError(Exception):
    """A recipe or a save the lab refuses, with what to do about it"""


class Constants:
    """The game's names and numbers as the build numbers them: every #define of the generated constants headers, by
    the header it is in (its kind: zones, flags, vars, species, items, moves, ...), so that a recipe names
    ZONE_FLOCCESY_RANCH_2, never 445, and no second table can drift from the game's. The names of the warp directions,
    an enum of include/field/zone.h, are added as the kind warp_dirs."""

    def __init__(self, directory: Path = CONSTANTS_DIR):
        self.by_name: dict[str, tuple[str, int]] = {}
        self.by_value: dict[str, dict[int, str]] = {}
        headers = sorted(directory.glob("*.h")) if directory.is_dir() else []
        if not headers:
            raise LabError(f"no generated constants headers in {directory}: the ROM build's `ninja constants_headers` "
                           "or the host build's configure writes them")
        for path in headers:
            kind = path.stem
            for line in path.read_text(encoding="utf-8").splitlines():
                match = DEFINE.match(line)
                if match:
                    self.add(kind, match.group(1), int(match.group(2), 0))
        for name, value in WARP_DIRS.items():
            self.add("warp_dirs", name, value)

    def add(self, kind: str, name: str, value: int) -> None:
        self.by_name.setdefault(name, (kind, value))
        self.by_value.setdefault(kind, {}).setdefault(value, name)

    def value(self, token: str, kind: str | None, where: str) -> int:
        """A number, or the value of a name of that kind (any kind for None)"""
        try:
            return int(token, 0)
        except ValueError:
            pass
        if token not in self.by_name:
            raise LabError(f"{where}: {token} is neither a number nor a name of the generated constants headers")
        named_kind, value = self.by_name[token]
        if kind is not None and named_kind != kind:
            raise LabError(f"{where}: {token} is a name of {named_kind}, not of {kind}")
        return value

    def name(self, kind: str, value: int) -> str | int:
        """The first name of that kind with the value, else the number itself"""
        return self.by_value.get(kind, {}).get(value, value)


def sha1(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()


class SaveImage:
    """A backup image as the game writes it: the main save twice, each of its 73 blocks followed by a footer with its
    CRC, the table of the blocks' CRCs, and the save footer. Values are read from the first copy and written to both;
    seal() then recomputes the CRCs of each block written, in each copy."""

    def __init__(self, data: bytes, what: str):
        problems = saveimage.validate_image(data)
        if problems:
            more = f" (and {len(problems) - 3} more)" if len(problems) > 3 else ""
            raise LabError(f"{what} isn't a valid save: {'; '.join(problems[:3])}{more}")
        if not copies_alike(data):
            backup = data[saveimage.COPY_SIZE:saveimage.COPY_SIZE + saveimage.MAIN_SAVE_SIZE]
            differing = sum(1 for a, b in zip(data[:saveimage.MAIN_SAVE_SIZE], backup) if a != b)
            raise LabError(f"{what}'s two copies of the main save differ in {differing} bytes: the game's SAVE writes "
                           "them alike, so this is not a save the game finished writing")
        self.blocks = saveimage.find_blocks(data)
        for block, size in BLOCK_SIZES.items():
            if self.blocks[block][1] != size:
                raise LabError(f"{what}: block {block:#x} has {self.blocks[block][1]:#x} bytes, where the game's "
                               f"{BLOCK_STRUCTS[block]} takes {size:#x}: not a Black 2 or White 2 save")
        self.base = bytes(data)
        self.data = bytearray(data)
        self.written: set[int] = set()

    def offset(self, block: int, field: int, size: int, copy: int = 0) -> int:
        start, length = self.blocks[block]
        if field < 0 or field + size > length:
            raise ValueError(f"block {block:#x} has {length:#x} bytes, no field of {size} at {field:#x}")
        return copy + start + field

    def read(self, fmt: str, block: int, field: int) -> tuple:
        return struct.unpack_from(fmt, self.data, self.offset(block, field, struct.calcsize(fmt)))

    def write(self, fmt: str, block: int, field: int, *values) -> None:
        for copy in saveimage.COPIES:
            struct.pack_into(fmt, self.data, self.offset(block, field, struct.calcsize(fmt), copy), *values)
        self.written.add(block)

    def seal(self) -> bytes:
        """The image with the CRCs of the blocks written recomputed in each copy: each block's footer and table entry,
        then the save footer's CRC of the table, as SaveData_FillFooter computes it. It must then pass the checks the
        base passed, with the base's layout"""
        for copy in saveimage.COPIES:
            for block in sorted(self.written):
                start, size = self.blocks[block]
                crc = saveimage.crc16_ccitt(bytes(self.data[copy + start:copy + start + size]))
                struct.pack_into("<H", self.data, copy + start + size + BLOCK_FOOTER_CRC, crc)
                struct.pack_into("<H", self.data, copy + saveimage.CRC_TABLE + 2 * block, crc)
            table_crc = saveimage.crc16_ccitt(bytes(self.data[copy + saveimage.CRC_TABLE:copy + saveimage.SAVE_FOOTER]))
            struct.pack_into("<H", self.data, copy + saveimage.SAVE_FOOTER + SAVE_FOOTER_CRC, table_crc)
        sealed = bytes(self.data)
        problems = saveimage.validate_image(sealed, reference=self.base)
        if not problems and not copies_alike(sealed):
            problems = ["its two copies of the main save differ"]
        if problems:
            raise LabError("the minted save fails the checks its base passed: " + "; ".join(problems[:3]))
        return sealed


def copies_alike(data: bytes) -> bool:
    """Whether the two copies of the main save hold the same bytes, as the game's SAVE leaves them"""
    return data[:saveimage.MAIN_SAVE_SIZE] == data[saveimage.COPY_SIZE:saveimage.COPY_SIZE + saveimage.MAIN_SAVE_SIZE]


def read_json(path: Path, what: str) -> dict:
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except OSError as error:
        raise LabError(f"can't read {what} {path}: {error.strerror}") from None


def zonedata_zones(constants: Constants, table: str) -> set[int]:
    """The zones of one of src/system/zonedata.c's tables, the game's own lists, read from its source so that they can't
    drift: ZONES_WITH_RAILS, the zones whose Fly position is a rail position (LoadZoneSpawnInfoCheckRail), or
    ZONE_RAIL_ASSIGNMENTS, the zones the game gives a rail system (GetZoneHasRailSystem; its version swap of Black City
    for White Forest changes nothing here, since both are in the table)"""
    what = {"ZONES_WITH_RAILS": "which zones' Fly positions are on a rail",
            "ZONE_RAIL_ASSIGNMENTS": "which zones have a rail system"}[table]
    try:
        source = ZONEDATA_C.read_text(encoding="utf-8")
    except OSError as error:
        raise LabError(f"can't read {ZONEDATA_C}: {error.strerror}") from None
    match = re.search(rf"\b{table}\[(\d+)\]\s*=\s*\{{(.*?)\}};", source, re.S)
    if match is None:
        raise LabError(f"{ZONEDATA_C} has no {table} table, which says {what}")
    names = re.findall(r"\bZONE_\w+", match.group(2))
    if len(names) != int(match.group(1)):
        raise LabError(f"{ZONEDATA_C}: {table} lists {len(names)} zones, not its {match.group(1)}")
    return {constants.value(name, "zones", str(ZONEDATA_C)) for name in names}


def place(save: SaveImage, constants: Constants, words: list[str], where: str) -> dict:
    """place ZONE warp N | place ZONE at X Z [Y] [facing DIR] | place ZONE fly: the spawn the next CONTINUE takes, the
    continue flag, and SaveLocation; what it set, for mint's report"""
    usage = f"{where}: place takes ZONE warp N, ZONE at X Z [Y] [facing DIR], or ZONE fly"
    if len(words) < 2:
        raise LabError(usage)
    zone = constants.value(words[0], "zones", where)
    zone_name = constants.name("zones", zone)
    if not isinstance(zone_name, str):
        raise LabError(f"{where}: {words[0]} is not a zone")
    (ex_state,) = save.read("<B", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_EX_STATE)
    if ex_state != FLD_PLAYER_EXSTATE_NONE:
        state = EX_STATES[ex_state] if ex_state < len(EX_STATES) else ex_state
        raise LabError(f"{where}: the base was saved in state {state}, which the arrival would keep; place takes a "
                       "base saved walking")
    mode, rest = words[1], words[2:]
    rail_arrival = False
    if mode == "warp":
        if len(rest) != 1:
            raise LabError(usage)
        warp_id = integer(rest[0], 0, 0xff, "a warp", where)
        warps = read_json(EVENTS_DIR / events_file_name(zone_name), "the events of")["warps"]
        if warp_id >= len(warps):
            raise LabError(f"{where}: {zone_name} has {len(warps)} warps, so no warp {warp_id}; use fly for the zone's "
                           "Fly position")
        warp = warps[warp_id]
        # SetupZoneSpawnInfoWarp: the warp's id alone; the game resolves its arrival from the zone's warp table
        spawn = (ZONE_SPAWN_CHANGE_TYPE_WARP, zone, warp_id, 0, 0, 0, bytes(12))
        # SetupZoneWarpArrival takes the facing from the warp, and GetGridWarpOutPos with posWeightBits 0 the position:
        # the warp's grid position, in world units, as fx32
        facing = warp["unk4"]
        grid = warp.get("grid")
        if grid is None:
            rail_arrival = True
            position = (0, 0, 0)
        else:
            position = tuple(to_s16(grid[axis]) << FX32_SHIFT for axis in ("x", "y", "z"))
        what = {"zone": zone_name, "warp": warp_id}
    elif mode in ("at", "fly"):
        if mode == "at":
            # A map with a rail system keeps its player on rails in places, which only a rail position
            # (CreateZoneChangeDataRail) can name, and the game resolves a warp onto its rail
            if zone in zonedata_zones(constants, "ZONE_RAIL_ASSIGNMENTS"):
                raise LabError(f"{where}: {zone_name} has a rail system (ZONE_RAIL_ASSIGNMENTS), and at takes a grid "
                               "position; place it by warp")
            # CreateZoneChangeData, as the script command FieldSetNextZone builds it from tiles
            x, z, y, facing = parse_at(constants, rest, where)
        else:
            if rest:
                raise LabError(usage)
            if zone in zonedata_zones(constants, "ZONES_WITH_RAILS"):
                raise LabError(f"{where}: {zone_name}'s Fly position is on a rail (ZONES_WITH_RAILS), and fly takes a "
                               "grid position; place it by warp")
            # LoadZoneSpawnInfoCheckRail: the Fly position (LoadZoneFlyPosToVecFx32), facing down
            fly = read_json(ZONES_DIR / zone_file_name(zone_name), "the zone data of")["fly"]
            x, y, z, facing = fly["x"], fly["y"], fly["z"], WARP_DIR_DOWN
            if x == y == z == 0:
                # The zone data's empty position: the game's own fallback would stand the player on the map's corner
                raise LabError(f"{where}: {zone_name}'s zone data has no Fly position, only (0, 0, 0), the map's "
                               "corner; place it by warp")
        position = (x << TILE_SHIFT, y << TILE_SHIFT, z << TILE_SHIFT)
        spawn = (ZONE_SPAWN_CHANGE_TYPE_POSITION, zone, -1, facing, 0, 0, struct.pack("<3i", *position))
        what = {"zone": zone_name, mode: [x, z] if mode == "at" else True}
    else:
        raise LabError(usage)
    save.write(SPAWN_FORMAT, SAVE_BLOCK_PLAYER, PLAYER_NEXT_SPAWN, *spawn)
    # SaveLocation, the start menu's place: the player state FieldMapControl_InitSpawn sets from the spawn (its zone,
    # ConvWarpDirToAngle of its facing, its position and isRail FALSE), which a rail arrival sets on the rail instead
    save.write("<H", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_ZONE, zone)
    save.write("<3i", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_POS, *position)
    save.write("<H", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_ROTATION, WARP_DIR_ANGLES.get(facing, 0))
    save.write("<B", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_IS_RAIL, 0)
    write_flag(save, constants.value("EVENT_FLAG_CONTINUE_SCRIPT", "flags", where), True)
    write_work(save, constants.value("EVENT_WORK_CONTINUE_SCRIPT", "vars", where), 0)
    what["facing"] = constants.name("warp_dirs", facing)
    if rail_arrival:
        what["arrival"] = "on a rail, which the game resolves"
    else:
        what["arrival"] = {"tile": [position[0] >> TILE_SHIFT, position[2] >> TILE_SHIFT],
                           "height": position[1] >> TILE_SHIFT}
    return what


def to_s16(value: int) -> int:
    value &= 0xffff
    return value - 0x10000 if value & 0x8000 else value


def parse_at(constants: Constants, words: list[str], where: str) -> tuple[int, int, int, int]:
    """X Z [Y] [facing DIR] of place ZONE at: tiles, as FieldSetNextZone takes them, and a WARP_DIR_*"""
    facing = WARP_DIR_DOWN
    if len(words) >= 2 and words[-2] == "facing":
        facing = constants.value(words[-1], "warp_dirs", where)
        if facing not in WARP_DIR_ANGLES:
            raise LabError(f"{where}: facing takes WARP_DIR_UP, _DOWN, _LEFT or _RIGHT, not {words[-1]}")
        words = words[:-2]
    if len(words) not in (2, 3):
        raise LabError(f"{where}: place ZONE at takes X Z [Y] [facing DIR]")
    # FieldSetNextZone takes u16 tiles and shifts them into an s32 world position: X and Z count a map's tiles from 0,
    # and Y's u16 wraps, so that its 65535 is -1, a tile below level 0
    x = integer(words[0], 0, 0x7fff, "a tile", where)
    z = integer(words[1], 0, 0x7fff, "a tile", where)
    y = integer(words[2], -0x8000, 0x7fff, "a height", where) if len(words) == 3 else 0
    return x, z, y, facing


def write_flag(save: SaveImage, flag: int, value: bool) -> None:
    offset = EVENT_WORK_FLAGS + flag // 8
    (byte,) = save.read("<B", SAVE_BLOCK_EVENT_WORK, offset)
    mask = 1 << (flag % 8)
    save.write("<B", SAVE_BLOCK_EVENT_WORK, offset, byte | mask if value else byte & ~mask)


def read_flag(save: SaveImage, flag: int) -> bool:
    (byte,) = save.read("<B", SAVE_BLOCK_EVENT_WORK, EVENT_WORK_FLAGS + flag // 8)
    return bool(byte & (1 << (flag % 8)))


def write_work(save: SaveImage, work: int, value: int) -> None:
    save.write("<H", SAVE_BLOCK_EVENT_WORK, EVENT_WORK_WORKS + 2 * (work - EVENT_WORK_FIRST), value)


def saved_flag(constants: Constants, token: str, where: str) -> int:
    flag = constants.value(token, "flags", where)
    if not 0 < flag < EVENT_FLAG_SAVED_COUNT:
        raise LabError(f"{where}: {token} is not a saved flag (1 to {EVENT_FLAG_SAVED_COUNT - 1})")
    return flag


def saved_work(constants: Constants, token: str, where: str) -> int:
    work = constants.value(token, "vars", where)
    if not EVENT_WORK_FIRST <= work < EVENT_WORK_FIRST + EVENT_WORK_COUNT:
        last = EVENT_WORK_FIRST + EVENT_WORK_COUNT - 1
        raise LabError(f"{where}: {token} is not a saved work value ({EVENT_WORK_FIRST:#x} to {last:#x})")
    return work


def integer(token: str, low: int, high: int, what: str, where: str) -> int:
    """A number, decimal or 0x hexadecimal, from low to high"""
    try:
        value = int(token, 0)
    except ValueError:
        raise LabError(f"{where}: {what} is a number, not {token}") from None
    if not low <= value <= high:
        raise LabError(f"{where}: {what} is {low} to {high}, not {value}")
    return value


def check_base(name: str, base: bytes, where: str) -> dict:
    """base NAME: the base's SHA-1 must be one that oracle.py's SAVES pins for that save"""
    import oracle  # noqa: E402 (the oracle's tool, imported only for a recipe that names its base)

    maker = oracle.SAVES.get(name)
    if maker is None:
        raise LabError(f"{where}: {name} is not one of oracle.py's SAVES ({', '.join(sorted(oracle.SAVES))})")
    digest = sha1(base)
    versions = [version for version, pinned in maker.sha1.items() if pinned == digest]
    if not versions:
        pins = ", ".join(f"{version} {pinned[:12]}" for version, pinned in sorted(maker.sha1.items()))
        raise LabError(f"{where}: the base's SHA-1 is {digest[:12]}, not one SAVES pins for {name} ({pins})")
    return {"base": name, "version": versions[0]}


def mint(recipe: str, base: bytes, recipe_name: str = "recipe",
         constants: Constants | None = None) -> tuple[bytes, list[dict]]:
    """The save that recipe (its text) makes of base, and what each of its lines set. Raises LabError for a recipe or
    a base it refuses, naming the line"""
    constants = constants or Constants()
    save = SaveImage(base, "the base")
    applied: list[dict] = []
    seen: dict[str, int] = {}
    for line, raw in enumerate(recipe.splitlines(), 1):
        words = raw.split("#", 1)[0].split()
        if not words:
            continue
        where = f"{recipe_name}:{line}"
        verb, args = words[0], words[1:]
        # What the line sets, which no other line may set
        target = verb
        if verb == "flag":
            if len(args) != 2 or args[1] not in ("set", "clear"):
                raise LabError(f"{where}: flag takes NAME set or NAME clear")
            flag = saved_flag(constants, args[0], where)
            if flag == constants.value("EVENT_FLAG_CONTINUE_SCRIPT", "flags", where):
                raise LabError(f"{where}: EVENT_FLAG_CONTINUE_SCRIPT is place's to set")
            target = f"flag {flag}"
        elif verb == "work":
            if len(args) != 2:
                raise LabError(f"{where}: work takes NAME VALUE")
            work = saved_work(constants, args[0], where)
            if work == constants.value("EVENT_WORK_CONTINUE_SCRIPT", "vars", where):
                raise LabError(f"{where}: EVENT_WORK_CONTINUE_SCRIPT is place's to set")
            target = f"work {work}"
        if target in seen:
            raise LabError(f"{where}: line {seen[target]} already sets that")
        seen[target] = line

        if verb == "base":
            if len(args) != 1:
                raise LabError(f"{where}: base takes the name of an oracle save")
            done = check_base(args[0], base, where)
        elif verb == "place":
            done = {"place": place(save, constants, args, where)}
        elif verb == "flag":
            write_flag(save, flag, args[1] == "set")
            done = {"flag": constants.name("flags", flag), "set": args[1] == "set"}
        elif verb == "work":
            value = constants.value(args[1], None, where)
            if not 0 <= value <= 0xffff:
                raise LabError(f"{where}: a work value is 0 to 65535, not {value}")
            write_work(save, work, value)
            done = {"work": constants.name("vars", work), "value": value}
        elif verb == "money":
            if len(args) != 1:
                raise LabError(f"{where}: money takes N")
            cash = integer(args[0], 0, TRAINER_GAME_INFO_CASH_MAX, "money", where)
            save.write("<I", SAVE_BLOCK_TRAINER_GAME_INFO, TRAINER_GAME_INFO_CASH, cash)
            done = {"money": cash}
        elif verb == "badges":
            if len(args) != 1:
                raise LabError(f"{where}: badges takes N")
            count = integer(args[0], 0, TRAINER_GAME_INFO_GYM_COUNT, "badges", where)
            save.write("<I", SAVE_BLOCK_TRAINER_GAME_INFO, TRAINER_GAME_INFO_BADGES, (1 << count) - 1)
            done = {"badges": count}
        elif verb == "playtime":
            match = PLAYTIME.match(args[0]) if len(args) == 1 else None
            if match is None or int(match.group(1)) > PLAYTIME_MAX_HOURS:
                raise LabError(f"{where}: playtime takes H:MM:SS, up to {PLAYTIME_MAX_HOURS}:59:59")
            hours, minutes, seconds = (int(part) for part in match.groups())
            play_time = TRAINER_DATA_PLAY_TIME
            save.write("<H", SAVE_BLOCK_TRAINER_DATA, play_time + PLAY_TIME_HOURS, hours)
            save.write("<B", SAVE_BLOCK_TRAINER_DATA, play_time + PLAY_TIME_MINUTES, minutes)
            save.write("<B", SAVE_BLOCK_TRAINER_DATA, play_time + PLAY_TIME_SECONDS, seconds)
            done = {"playtime": f"{hours}:{minutes:02}:{seconds:02}"}
        else:
            raise LabError(f"{where}: unknown verb {verb}; the verbs are base, place, flag, work, money, badges and "
                           "playtime")
        applied.append({"line": line, **done})
    return save.seal(), applied


def party(save: SaveImage, constants: Constants) -> list[dict]:
    """The party's Pokémon, the first count of them, each decoded by port/tools/oracle/savedecode.py as
    PokeParty_DecryptPkm decrypts one (src/system/poke_tool.c): the party's fields with the PID, the four blocks with
    the checksum, which must then be the sum of the blocks' u16s (PML_CryptoGenKey), else checksum_ok is false"""
    (count,) = save.read("<i", SAVE_BLOCK_PARTY, POKE_PARTY_COUNT)
    if not 0 <= count <= PARTY_SIZE:
        raise LabError(f"the party's count is {count}, not 0 to {PARTY_SIZE}")
    pokemon = []
    for slot in range(count):
        raw = save.read(f"<{PARTY_PKM_SIZE}s", SAVE_BLOCK_PARTY, POKE_PARTY_PKMS + slot * PARTY_PKM_SIZE)[0]
        fields, problems = savedecode.pokemon_fields(raw)
        moves = [fields[f"blockB.moves[{index}]"] for index in range(4)]
        pokemon.append({
            "slot": slot + 1,
            "species": constants.name("species", fields["blockA.species"]),
            "level": fields["level"],
            "hp": [fields["nowHP"], fields["maxHP"]],
            "exp": fields["blockA.exp"],
            "item": constants.name("items", fields["blockA.item"]),
            "moves": [constants.name("moves", move) for move in moves if move],
            "checksum_ok": not problems,
        })
    return pokemon


def position_facts(position: bytes, is_rail: bool) -> dict:
    """A grid position as its tile and height, or a RailPosition as its fields"""
    if is_rail:
        component, is_line, direction, side, front = struct.unpack_from(RAIL_POSITION_FORMAT, position)
        return {"rail": {"component": component, "line": bool(is_line), "direction": direction, "front": front,
                         "side": side}}
    x, y, z = struct.unpack_from("<3i", position)
    return {"tile": [x >> TILE_SHIFT, z >> TILE_SHIFT], "height": y >> TILE_SHIFT}


def spawn_facts(save: SaveImage, constants: Constants, field: int) -> dict:
    change, zone, warp, warp_dir, weight, is_rail, position = save.read(SPAWN_FORMAT, SAVE_BLOCK_PLAYER, field)
    return {
        "type": ZONE_SPAWN_CHANGE_TYPES[change] if change < len(ZONE_SPAWN_CHANGE_TYPES) else change,
        "zone": constants.name("zones", zone & 0xffff),
        "warp": warp,
        "facing": constants.name("warp_dirs", warp_dir),
        "pos_weight_bits": weight,
        **position_facts(position, bool(is_rail)),
    }


def read(data: bytes, constants: Constants | None = None, flags: Sequence[str] = (), works: Sequence[str] = (),
         all_flags: bool = False, what: str = "the save") -> dict:
    """A save's facts, in the game's names: flags and works name the flags and work values to report besides the
    continue pair and every work value that isn't 0"""
    constants = constants or Constants()
    save = SaveImage(data, what)
    where = "read"
    (zone,) = save.read("<H", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_ZONE)
    position = save.read("12s", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_POS)[0]
    rail = save.read("8s", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_RAIL_POS)[0]
    rotation, is_rail, ex_state = save.read("<HBB", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_ROTATION)
    facings = {angle: direction for direction, angle in WARP_DIR_ANGLES.items()}
    location = {
        "zone": constants.name("zones", zone),
        **position_facts(rail if is_rail else position, bool(is_rail)),
        "facing": constants.name("warp_dirs", facings[rotation]) if rotation in facings else rotation,
        "state": EX_STATES[ex_state] if ex_state < len(EX_STATES) else ex_state,
    }
    (hours,) = save.read("<H", SAVE_BLOCK_TRAINER_DATA, TRAINER_DATA_PLAY_TIME + PLAY_TIME_HOURS)
    minutes, seconds = save.read("<BB", SAVE_BLOCK_TRAINER_DATA, TRAINER_DATA_PLAY_TIME + PLAY_TIME_MINUTES)
    (cash,) = save.read("<I", SAVE_BLOCK_TRAINER_GAME_INFO, TRAINER_GAME_INFO_CASH)
    (badges,) = save.read("<I", SAVE_BLOCK_TRAINER_GAME_INFO, TRAINER_GAME_INFO_BADGES)
    flag_ids = [saved_flag(constants, "EVENT_FLAG_CONTINUE_SCRIPT", where)]
    flag_ids += [saved_flag(constants, name, where) for name in flags]
    all_works = save.read(f"<{EVENT_WORK_COUNT}H", SAVE_BLOCK_EVENT_WORK, EVENT_WORK_WORKS)
    work_ids = [saved_work(constants, "EVENT_WORK_CONTINUE_SCRIPT", where)]
    work_ids += [saved_work(constants, name, where) for name in works]
    work_ids += [EVENT_WORK_FIRST + index for index, value in enumerate(all_works) if value]
    facts = {
        "sha1": sha1(data),
        "location": location,
        "next_spawn": spawn_facts(save, constants, PLAYER_NEXT_SPAWN),
        "now_spawn": spawn_facts(save, constants, PLAYER_NOW_SPAWN),
        "flags": {constants.name("flags", flag): read_flag(save, flag) for flag in flag_ids},
        "works": {constants.name("vars", work): all_works[work - EVENT_WORK_FIRST] for work in sorted(set(work_ids))},
        "money": cash,
        "badges": {"count": bin(badges).count("1"), "bits": badges},
        "playtime": f"{hours}:{minutes:02}:{seconds:02}",
        "party": party(save, constants),
    }
    set_flags = [flag for flag in range(1, EVENT_FLAG_SAVED_COUNT) if read_flag(save, flag)]
    facts["flags_set"] = len(set_flags)
    if all_flags:
        facts["set_flags"] = [constants.name("flags", flag) for flag in set_flags]
    return facts


def write_new(path: str, data: bytes, force: bool) -> None:
    """Writes the file whole, through a temporary file renamed over it, so that a reader never sees half a save; an
    existing file only with force, since it can be the player's own save"""
    if os.path.exists(path) and not force:
        raise LabError(f"{path} exists: pass --force to replace it")
    directory = os.path.dirname(os.path.abspath(path))
    handle, temporary = tempfile.mkstemp(prefix=".savelab-", dir=directory)
    try:
        with os.fdopen(handle, "wb") as f:
            f.write(data)
        os.replace(temporary, path)
    except BaseException:
        if os.path.exists(temporary):
            os.remove(temporary)
        raise


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0],
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    minting = commands.add_parser("mint", help="mint a save from a base and a recipe")
    minting.add_argument("recipe", help="the recipe file, or - for the standard input")
    minting.add_argument("--base", required=True, help="the save the recipe starts from")
    minting.add_argument("--out", required=True, help="the minted save")
    minting.add_argument("--force", action="store_true", help="replace --out when it exists")
    reading = commands.add_parser("read", help="print a save's facts")
    reading.add_argument("save")
    reading.add_argument("--flag", action="append", default=[], help="an event flag to report, by name (repeatable)")
    reading.add_argument("--work", action="append", default=[], help="an event work value to report (repeatable)")
    reading.add_argument("--all-flags", action="store_true", help="list every flag that is set, by name")
    args = parser.parse_args(argv)
    try:
        if args.command == "mint":
            if args.recipe == "-":
                recipe, recipe_name = sys.stdin.read(), "<stdin>"
            else:
                recipe, recipe_name = Path(args.recipe).read_text(encoding="utf-8"), os.path.basename(args.recipe)
            base = Path(args.base).read_bytes()
            minted, applied = mint(recipe, base, recipe_name)
            write_new(args.out, minted, args.force)
            changed = sum(1 for a, b in zip(base, minted) if a != b)
            print(json.dumps({"base": sha1(base), "minted": sha1(minted), "bytes_changed": changed,
                              "applied": applied}, indent=2))
        else:
            facts = read(Path(args.save).read_bytes(), flags=args.flag, works=args.work, all_flags=args.all_flags,
                         what=args.save)
            print(json.dumps(facts, indent=2))
    except LabError as error:
        print(f"savelab: {error}", file=sys.stderr)
        return 1
    except OSError as error:
        print(f"savelab: {error.filename}: {error.strerror}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
