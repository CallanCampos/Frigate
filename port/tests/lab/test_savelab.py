#!/usr/bin/env python3
"""ROM-free tests of the save lab's minter and reader (port/tools/lab/savelab.py), on synthetic saves built from the
decomp's headers' layout: savelab_layout.c, compiled as the host build compiles the game's files, prints the offsets,
sizes and numbers of include/save/*.h, include/field/zone.h and include/pml/poke_party.h, and the tests build a full
backup image from them field by field (both copies of the main save, 73 blocks with footers, the CRC table and the
save footer, as src/gfl/backup_system.c lays them out), then hold savelab to them:

  - its own table of the layout is the headers', and its reader gives back each field the image was built with: a place
    on the grid and one on a rail, both spawns, a party of six, a Pokémon stored decrypted;
  - an empty recipe leaves the save unchanged;
  - a recipe's writes are resealed in both copies, each block's footer CRC and table entry and the table's CRC;
  - each verb and each form of place gives exactly the save the game's own writes would: the test builds that save
    again from the layout with those fields changed, and the two must be alike byte for byte, so that a value written
    wrong, a field left as the base had it and a byte the verb doesn't own all fail. The base place starts from is saved
    on a rail with EVENT_WORK_CONTINUE_SCRIPT set and the follower and step counter as a game leaves them, so that
    every field place writes changes;
  - a bad base (the wrong size, a broken CRC in either copy, copies that differ, a foreign block layout, the wrong
    oracle save, a state the arrival would keep) is refused, and a base whose SHA-1 SAVES pins is taken.

MUTANTS lists deliberate bugs in savelab.py, each with the tests that must catch it, and MutationTests applies each to
savelab.py's source, loads the result in savelab's place and runs those tests, which must fail: each check is shown to
fail on the bugs it is for, and the same tests are shown to pass on savelab as it is. The expected values are worked
out by hand from the game's C and data (the comments say which), never from savelab. Nothing here is a game save or
comes from the ROM. Game functionality: the saves (where CONTINUE puts the player, the event flags and work values, the
play time, the money and badges, the party).

    test_savelab.py --layout PROGRAM [unittest options]

PROGRAM is the build's port_test_savelab_layout; the ctest test lab_savelab (port/cmake/lab.cmake) passes it. The
generated constants headers (build/include/generated/constants) must exist, as after any configure.
"""

from __future__ import annotations

import contextlib
import functools
import hashlib
import io
import json
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
import tempfile
import types
import unittest
from unittest import mock

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[2]
sys.path.insert(0, str(REPO_ROOT / "port" / "tools" / "lab"))
sys.path.insert(0, str(REPO_ROOT / "port" / "tools" / "oracle"))

import oracle  # noqa: E402
import saveimage  # noqa: E402
import savelab  # noqa: E402

LAYOUT_PROGRAM = None

# PML_CryptoRun's generator (src/system/poke_tool.c, PML_CryptoNextKey)
CRYPTO_MULTIPLIER = 0x41c64e6d
CRYPTO_INCREMENT = 0x6073
# The constants the tests name, from build/include/generated/constants (zones.h, flags.h, vars.h, species.h, moves.h)
ZONE_BLACK_CITY = 0
ZONE_CASTELIA_CITY = 28
ZONE_GIANT_CHASM = 230
ZONE_VILLAGE_BRIDGE = 255
ZONE_ROUTE_19 = 437
ZONE_FLOCCESY_RANCH = 444
ZONE_FLOCCESY_RANCH_2 = 445
ZONE_ROUTE_20 = 446
EVENT_FLAG_RUNNING_SHOES = 0x963
EVENT_FLAG_CONTINUE_SCRIPT = 0x965
EVENT_FLAG_TRAINER_YOUNGSTER_PETEY = 0x778
EVENT_WORK_CONTINUE_SCRIPT = 0x4041
SPECIES_AZURILL = 298
SPECIES_RIOLU = 447
SPECIES_TEPIG = 498
SPECIES_PATRAT = 504
SPECIES_LILLIPUP = 506
SPECIES_PURRLOIN = 509
SPECIES_PIDOVE = 519
ITEM_ORAN_BERRY = 155
MOVE_SCRATCH = 10
MOVE_TACKLE = 33
MOVE_TAIL_WHIP = 39
MOVE_LEER = 43
MOVE_GROWL = 45
MOVE_QUICK_ATTACK = 98
# include/field/zone.h: the spawns' change types and the warp directions
CHANGE_WARP = 0
CHANGE_POSITION = 1
WARP_DIR_UP, WARP_DIR_DOWN, WARP_DIR_LEFT, WARP_DIR_RIGHT = 1, 2, 3, 4
# The fields the lab leaves alone that savelab_layout.c prints for the synthetic saves, which savelab's LAYOUT lacks
UNTOUCHED_FIELDS = ("PlayerSave.followIndex", "PlayerSave.stepCounter")


@functools.cache
def layout_output() -> str:
    """What savelab_layout prints, run once"""
    return subprocess.run([LAYOUT_PROGRAM], check=True, capture_output=True, text=True).stdout


def headers_layout() -> dict[str, int]:
    """savelab_layout's lines, NAME VALUE"""
    layout = {}
    for line in layout_output().splitlines():
        name, value = line.split()
        layout[name] = int(value, 16)
    return layout


def lab_fields(layout: dict[str, int]) -> dict[str, int]:
    """The headers' layout without the fields the lab leaves alone: what savelab's LAYOUT must be"""
    return {name: value for name, value in layout.items() if name not in UNTOUCHED_FIELDS}


def compare_layout(table: dict[str, int], headers: dict[str, int]) -> list[str]:
    """Where savelab's table and the headers' layout disagree"""
    problems = [f"{name}: savelab has {table[name]:#x}, the headers {headers[name]:#x}"
                for name in sorted(set(table) & set(headers)) if table[name] != headers[name]]
    problems += [f"{name}: only in savelab" for name in sorted(set(table) - set(headers))]
    problems += [f"{name}: only in the headers" for name in sorted(set(headers) - set(table))]
    return problems


def crypt(data: bytes, key: int) -> bytes:
    """PML_CryptoRun: each u16 XORed with the high half of the generator's next state, seeded with key"""
    out = bytearray(data)
    for offset in range(0, len(out) - 1, 2):
        key = (key * CRYPTO_MULTIPLIER + CRYPTO_INCREMENT) & 0xffffffff
        struct.pack_into("<H", out, offset, struct.unpack_from("<H", out, offset)[0] ^ (key >> 16))
    return bytes(out)


def games_block_shuffle() -> list[tuple[int, ...]]:
    """blockShufLUT as src/system/poke_tool.c writes it"""
    source = (REPO_ROOT / "src" / "system" / "poke_tool.c").read_text(encoding="utf-8")
    table = re.search(r"blockShufLUT\[32\]\[4\]\s*=\s*\{(.*?)\};", source, re.S)
    return [tuple(int(value, 16) for value in row.split(","))
            for row in re.findall(r"\{\s*(0x[0-9a-f]+\s*,\s*0x[0-9a-f]+\s*,\s*0x[0-9a-f]+\s*,\s*0x[0-9a-f]+)\s*\}",
                                  table.group(1))]


def events(zone_file: str) -> dict:
    return json.loads((REPO_ROOT / "res" / "field" / "events" / f"{zone_file}.json").read_text(encoding="utf-8"))


def zone_data(zone_file: str) -> dict:
    return json.loads((REPO_ROOT / "res" / "field" / "zones" / f"{zone_file}.json").read_text(encoding="utf-8"))


class Synthetic:
    """A backup image built from the headers' layout: the five blocks savelab reads at their structs' sizes rounded to
    4 bytes (SaveData_BuildTable, src/gfl/savedata.c), each other block one 256-byte page but block 0, which fills the
    main save up to the CRC table, each block 0x100-aligned with a BlockFooter {u16 count; u16 crc}, then the CRC
    table with its padding and the SaveFooter {count, size, magic, unk, crc} (src/gfl/backup_system.c), and the same
    main save again at 0x26000. Fields are set in the first copy, field by field at the headers' offsets; image()
    copies it and computes every CRC"""

    def __init__(self, layout: dict[str, int], sizes: dict[int, int] | None = None):
        self.layout = layout
        known = {layout[f"SAVE_BLOCK_{name}"]: (layout[f"sizeof({struct_name})"] + 3) // 4 * 4
                 for name, struct_name in (("PARTY", "PokeParty"), ("TRAINER_DATA", "TrainerDataSave"),
                                           ("PLAYER", "PlayerSave"), ("EVENT_WORK", "EventWork"),
                                           ("TRAINER_GAME_INFO", "TrainerGameInfoSave"))}
        known.update(sizes or {})
        count = layout["SAVE_BLOCK_MAX"]
        pages = {block: (known.get(block, 0xfc) + 4 + 0xff) // 0x100 for block in range(1, count)}
        first = (saveimage.CRC_TABLE // 0x100 - sum(pages.values())) * 0x100 - 4
        self.blocks = []
        start = 0
        for block in range(count):
            size = first if block == 0 else known.get(block, 0xfc)
            self.blocks.append((start, size))
            start = (start + size + 4 + 0xff) // 0x100 * 0x100
        assert start == saveimage.CRC_TABLE, hex(start)
        self.data = bytearray(b"\xff" * saveimage.BACKUP_SIZE)
        filler = random.Random(2026)
        for block, (start, size) in enumerate(self.blocks):
            self.data[start:start + size] = bytes(size) if block in known else filler.randbytes(size)

    def put(self, fmt: str, block_name: str, field: int, *values) -> None:
        start, size = self.blocks[self.layout[f"SAVE_BLOCK_{block_name}"]]
        assert field + struct.calcsize(fmt) <= size
        struct.pack_into(fmt, self.data, start + field, *values)

    def fill(self, block_name: str, seed: int) -> None:
        """The block's bytes as filler, so that a field the test sets nowhere still holds something a write would
        change"""
        start, size = self.blocks[self.layout[f"SAVE_BLOCK_{block_name}"]]
        self.data[start:start + size] = random.Random(seed).randbytes(size)

    def flag(self, flag: int, value: bool) -> None:
        """Flag N: bit N % 8 of EventWork.flags[N / 8] (EventWork_GetFlagBytePtr, src/system/event_work.c)"""
        start, _ = self.blocks[self.layout["SAVE_BLOCK_EVENT_WORK"]]
        at = start + self.layout["EventWork.flags"] + flag // 8
        self.data[at] = self.data[at] | 1 << flag % 8 if value else self.data[at] & ~(1 << flag % 8)

    def work(self, work: int, value: int) -> None:
        """Work W: EventWork.works[W - 0x4000] (EventWork_GetWkPtr)"""
        self.put("<H", "EVENT_WORK", self.layout["EventWork.works"] + 2 * (work - 0x4000), value)

    def spawn(self, field: str, change_type: int, zone: int, warp: int, warp_dir: int, weight: int, is_rail: bool,
              position: bytes) -> None:
        """A ZoneSpawnInfo of PlayerSave, field by field; position is its union's 12 bytes"""
        at = self.layout[f"PlayerSave.{field}"]
        for name, fmt, value in (("changeType", "<I", change_type), ("zoneId", "<h", zone), ("warpId", "<h", warp),
                                 ("warpDir", "<h", warp_dir), ("posWeightBits", "<H", weight),
                                 ("isRail", "<i", is_rail)):
            self.put(fmt, "PLAYER", at + self.layout[f"ZoneSpawnInfo.{name}"], value)
        self.put("<12s", "PLAYER", at + self.layout["ZoneSpawnInfo.pos"], position)

    def location(self, zone: int, rotation: int, is_rail: bool, ex_state: int = 0, position: bytes | None = None,
                 rail_position: bytes | None = None) -> None:
        """SaveLocation, field by field; a position left out keeps what the block holds"""
        at = self.layout["PlayerSave.location"]
        self.put("<H", "PLAYER", at + self.layout["SaveLocation.zoneId"], zone)
        if position is not None:
            self.put("<12s", "PLAYER", at + self.layout["SaveLocation.pos"], position)
        if rail_position is not None:
            self.put(f"<{len(rail_position)}s", "PLAYER", at + self.layout["SaveLocation.railPos"], rail_position)
        self.put("<H", "PLAYER", at + self.layout["SaveLocation.unk18"], rotation)
        self.put("<B", "PLAYER", at + self.layout["SaveLocation.unk1A"], is_rail)
        self.put("<B", "PLAYER", at + self.layout["SaveLocation.exState"], ex_state)

    def image(self) -> bytes:
        main = self.data[:saveimage.COPY_SIZE]
        for index, (start, size) in enumerate(self.blocks):
            crc = saveimage.crc16_ccitt(bytes(main[start:start + size]))
            struct.pack_into("<HH", main, start + size, 1, crc)
            struct.pack_into("<H", main, saveimage.CRC_TABLE + 2 * index, crc)
        struct.pack_into("<H", main, saveimage.CRC_TABLE + 2 * len(self.blocks), 0)
        table_crc = saveimage.crc16_ccitt(bytes(main[saveimage.CRC_TABLE:saveimage.SAVE_FOOTER]))
        struct.pack_into("<IIIHH", main, saveimage.SAVE_FOOTER, 1, saveimage.MAIN_SAVE_SIZE, saveimage.SAVE_MAGIC, 0,
                         table_crc)
        image = bytearray(self.data)
        image[:saveimage.COPY_SIZE] = main
        image[saveimage.COPY_SIZE:2 * saveimage.COPY_SIZE] = main
        return bytes(image)


def grid(x: int, y: int, z: int) -> bytes:
    """A VecFx32, three s32"""
    return struct.pack("<3i", x, y, z)


def rail(layout: dict[str, int], component: int, direction: int, side: int, front: int) -> bytes:
    """A RailPosition on a line of its component, field by field"""
    raw = bytearray(layout["sizeof(RailPosition)"])
    for name, fmt, value in (("componentId", "<H", component), ("componentIsLine", "<B", 1),
                             ("railDirection", "<B", direction), ("posSide", "<h", side), ("posFront", "<H", front)):
        struct.pack_into(fmt, raw, layout[f"RailPosition.{name}"], value)
    return bytes(raw)


def pokemon(layout: dict[str, int], pid: int, species: int, item: int, exp: int, moves: tuple[int, ...], level: int,
            hp: tuple[int, int], stored_decrypted: bool = False) -> bytes:
    """A party Pokémon as the game stores it (PokeParty_EncryptPkm): blocks A and B where the PID's bits 13 to 17 put
    them, the checksum the sum of the blocks' u16s, the blocks encrypted with it and the party's fields with the PID.
    stored_decrypted leaves both in the clear with BoxPkm.partyDecrypted set, as PokeParty_DecryptPkm leaves them"""
    shuffle = games_block_shuffle()[(pid & 0x3e000) >> 13]
    content = bytearray(layout["sizeof(PkmBuffer)"])
    struct.pack_into("<HH", content, shuffle[0] + layout["PkmBlockA.species"], species, item)
    struct.pack_into("<I", content, shuffle[0] + layout["PkmBlockA.exp"], exp)
    struct.pack_into("<4H", content, shuffle[1] + layout["PkmBlockB.moves"], *moves)
    checksum = sum(struct.unpack(f"<{len(content) // 2}H", content)) & 0xffff
    party_fields = bytearray(layout["sizeof(PartyPkm)"] - layout["PartyPkm.statusCond"])
    party_fields[layout["PartyPkm.level"] - layout["PartyPkm.statusCond"]] = level
    struct.pack_into("<HH", party_fields, layout["PartyPkm.nowHP"] - layout["PartyPkm.statusCond"], *hp)
    raw = bytearray(layout["sizeof(PartyPkm)"])
    struct.pack_into("<I", raw, layout["BoxPkm.pid"], pid)
    struct.pack_into("<H", raw, layout["BoxPkm.checksum"], checksum)
    content_at = layout["BoxPkm.contentBuffer"]
    if stored_decrypted:
        raw[layout["BoxPkm.partyDecrypted.byte"]] |= layout["BoxPkm.partyDecrypted.mask"]
        raw[content_at:content_at + len(content)] = content
        raw[layout["PartyPkm.statusCond"]:] = party_fields
    else:
        raw[content_at:content_at + len(content)] = crypt(bytes(content), checksum)
        raw[layout["PartyPkm.statusCond"]:] = crypt(bytes(party_fields), pid)
    return bytes(raw)


def story(save: Synthetic, party: tuple[bytes, ...] | None = None) -> None:
    """What every synthetic save holds besides the player's place: the rest of the player block as filler, but the
    follower and the step counter as a game leaves them (none: index 0xff, as FieldFollowWk keeps it, and 0xffff,
    PlayerSave_Init's); story work 0x40a3 and 0x40a5 at 2 and 0x4030 at 1, flags 0x961 and 0x962 (a Pokémon and the
    Pokédex); play time 12:34:56; money 3,500; badges 0 and 2; Tepig and Purrloin, or the party given"""
    layout = save.layout
    save.fill("PLAYER", 0x1c)
    save.put("<B", "PLAYER", layout["PlayerSave.followIndex"], 0xff)
    save.put("<H", "PLAYER", layout["PlayerSave.stepCounter"], 0xffff)
    for work, value in ((0x40a3, 2), (0x40a5, 2), (0x4030, 1)):
        save.work(work, value)
    save.flag(0x961, True)
    save.flag(0x962, True)
    play_time = layout["TrainerDataSave.playTime"]
    save.put("<H", "TRAINER_DATA", play_time + layout["PlayTime.hours"], 12)
    save.put("<BB", "TRAINER_DATA", play_time + layout["PlayTime.minutes"], 34, 56)
    save.put("<I", "TRAINER_GAME_INFO", layout["TrainerGameInfoSave.cash"], 3500)
    save.put("<I", "TRAINER_GAME_INFO", layout["TrainerGameInfoSave.badges"], 0b101)
    if party is None:
        party = (pokemon(layout, 0x1234_5678, SPECIES_TEPIG, 0, 229, (MOVE_TACKLE, MOVE_TAIL_WHIP, 0, 0), 6, (23, 23)),
                 pokemon(layout, 0x8765_4321, SPECIES_PURRLOIN, 0, 64, (MOVE_SCRATCH, MOVE_GROWL, 0, 0), 4, (17, 17)))
    save.put("<ii", "PARTY", layout["PokeParty.count"] - 4, layout["PARTY_SIZE"], len(party))
    for slot, member in enumerate(party):
        save.put(f"<{len(member)}s", "PARTY", layout["PokeParty.pkms"] + slot * layout["sizeof(PartyPkm)"], member)


def route20_save(layout: dict[str, int], ex_state: int = 0, sizes: dict[int, int] | None = None,
                 party: tuple[bytes, ...] | None = None) -> Synthetic:
    """A save shaped as the Route 20 save is: the player saved on ZONE_ROUTE_20 at tile (130, 661), height 2, facing
    right, walking; the last door the warp 0 of ZONE_ROUTE_19; nextSpawn as a new game leaves it, which no script has
    set since; and the story"""
    save = Synthetic(layout, sizes)
    story(save, party)
    # A tile's centre, (tile * 16 + 8) world units as fx32, as the game saves its player state
    save.location(ZONE_ROUTE_20, 0xc000, False, ex_state, grid((130 * 16 + 8) << 12, 0x2001f, (661 * 16 + 8) << 12))
    save.spawn("nowSpawn", CHANGE_WARP, ZONE_ROUTE_19, 0, WARP_DIR_UP, 305, False, grid(53 << 16, 1 << 16, 703 << 16))
    save.spawn("nextSpawn", CHANGE_WARP, 0, 0, 0, 0, False, bytes(12))
    return save


def castelia_rail_save(layout: dict[str, int]) -> Synthetic:
    """A save written on a rail of Castelia City's street, the place every field place writes differs from: SaveLocation
    on component 2's line, side -6 and front 28, turned between up and left (0x2000), as a curve leaves the player; the
    last door Castelia's rail warp 3, its arrival on the rail; nextSpawn a rail position as CreateZoneChangeDataRail
    writes one; EVENT_WORK_CONTINUE_SCRIPT 1, as a link match's script leaves it (res/field/scripts/global_10105.s),
    with the work values on either side of it set too and the continue flag clear; and the story"""
    save = Synthetic(layout)
    story(save)
    save.location(ZONE_CASTELIA_CITY, 0x2000, True, 0, grid(427 << 16, 0, 712 << 16),
                  rail(layout, component=2, direction=0, side=-6, front=28))
    save.spawn("nowSpawn", CHANGE_WARP, ZONE_CASTELIA_CITY, 3, WARP_DIR_UP, 0x121, True,
               rail(layout, component=2, direction=0, side=-6, front=29) + bytes(4))
    save.spawn("nextSpawn", CHANGE_POSITION, ZONE_CASTELIA_CITY, -1, WARP_DIR_DOWN, 0, True,
               rail(layout, component=3, direction=2, side=7, front=25) + bytes(4))
    save.work(EVENT_WORK_CONTINUE_SCRIPT, 1)
    save.work(EVENT_WORK_CONTINUE_SCRIPT - 1, 7)
    save.work(EVENT_WORK_CONTINUE_SCRIPT + 1, 9)
    return save


class LabTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.layout = headers_layout()
        cls.constants = savelab.Constants()
        cls.base = route20_save(cls.layout).image()

    def mint(self, recipe: str, base: bytes | None = None) -> bytes:
        return savelab.mint(recipe, self.base if base is None else base, "test.recipe", self.constants)[0]

    def refused(self, recipe: str, text: str, base: bytes | None = None) -> None:
        with self.assertRaises(savelab.LabError) as caught:
            self.mint(recipe, base)
        self.assertIn(text, str(caught.exception))

    def field(self, data: bytes, fmt: str, block_name: str, field: int, copy: int = 0) -> tuple:
        start, _ = saveimage.find_blocks(data)[self.layout[f"SAVE_BLOCK_{block_name}"]]
        return struct.unpack_from(fmt, data, copy + start + field)

    def assertImage(self, minted: bytes, expected: bytes) -> None:
        """minted is expected byte for byte; else the first differences, by copy, block and offset in the block"""
        if minted == expected:
            return
        self.assertEqual(len(minted), len(expected))
        blocks = saveimage.find_blocks(expected)
        differences = []
        for offset in range(len(expected)):
            if minted[offset] == expected[offset]:
                continue
            copy = 0 if offset < saveimage.COPY_SIZE else saveimage.COPY_SIZE
            where = next((f"block {block:#x} +{offset - copy - start:#x}"
                          for block, (start, size) in enumerate(blocks) if start <= offset - copy < start + size + 4),
                         f"{offset - copy:#x}")
            differences.append(f"copy {copy:#x} {where}: {minted[offset]:#04x}, not {expected[offset]:#04x}")
        self.fail(f"{len(differences)} bytes differ from the expected save: " + "; ".join(differences[:8]))


class LayoutTests(LabTest):
    def test_savelab_layout_is_the_headers(self):
        self.assertEqual(compare_layout(savelab.LAYOUT, lab_fields(self.layout)), [])
        # The fields the lab leaves alone are the headers' too, and savelab has none of them
        self.assertLessEqual(set(UNTOUCHED_FIELDS), set(self.layout))
        self.assertFalse(set(UNTOUCHED_FIELDS) & set(savelab.LAYOUT))

    def test_a_moved_layout_entry_is_caught(self):
        for name in ("SaveLocation.pos", "EventWork.flags", "PartyPkm.level", "sizeof(PlayerSave)"):
            with self.subTest(name=name):
                moved = dict(savelab.LAYOUT, **{name: savelab.LAYOUT[name] + 2})
                self.assertEqual(compare_layout(moved, lab_fields(self.layout)),
                                 [f"{name}: savelab has {moved[name]:#x}, the headers {self.layout[name]:#x}"])

    def test_block_shuffle_is_the_games(self):
        table = games_block_shuffle()
        self.assertEqual(len(table), 32)
        self.assertEqual([tuple(row) for row in savelab.BLOCK_SHUFFLE], table)
        # Control: the comparison sees a row swapped
        swapped = list(savelab.BLOCK_SHUFFLE)
        swapped[1], swapped[2] = swapped[2], swapped[1]
        self.assertNotEqual(swapped, table)

    def test_zonedata_tables_are_the_games(self):
        rail_fly = savelab.zonedata_zones(self.constants, "ZONES_WITH_RAILS")
        rail_system = savelab.zonedata_zones(self.constants, "ZONE_RAIL_ASSIGNMENTS")
        self.assertEqual((len(rail_fly), len(rail_system)), (12, 28))
        self.assertLessEqual(rail_fly, rail_system)
        self.assertIn(ZONE_CASTELIA_CITY, rail_fly)
        self.assertNotIn(ZONE_VILLAGE_BRIDGE, rail_fly)
        self.assertIn(ZONE_VILLAGE_BRIDGE, rail_system)
        self.assertNotIn(ZONE_ROUTE_20, rail_system)
        # Controls: without the game's table, or with a table whose count is wrong, at and fly can't tell, so they
        # refuse
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "zonedata.c"
            for text, table, expected in (
                ("// no table\n", "ZONES_WITH_RAILS", "has no ZONES_WITH_RAILS table"),
                ("static const ZoneAssignment ZONE_RAIL_ASSIGNMENTS[2] = { { ZONE_CASTELIA_CITY, 1 } };\n",
                 "ZONE_RAIL_ASSIGNMENTS", "ZONE_RAIL_ASSIGNMENTS lists 1 zones, not its 2"),
            ):
                source.write_text(text)
                with self.subTest(table=table), mock.patch.object(savelab, "ZONEDATA_C", source):
                    with self.assertRaises(savelab.LabError) as caught:
                        savelab.zonedata_zones(self.constants, table)
                    self.assertIn(expected, str(caught.exception))


class ReaderTests(LabTest):
    ROUTE_20 = {
        "location": {"zone": "ZONE_ROUTE_20", "tile": [130, 661], "height": 2, "facing": "WARP_DIR_RIGHT",
                     "state": "FLD_PLAYER_EXSTATE_NONE"},
        "now_spawn": {"type": "ZONE_SPAWN_CHANGE_TYPE_WARP", "zone": "ZONE_ROUTE_19", "warp": 0,
                      "facing": "WARP_DIR_UP", "pos_weight_bits": 305, "tile": [53, 703], "height": 1},
        # Zone 0 is ZONE_BLACK_CITY, and a warp direction of 0 has no name
        "next_spawn": {"type": "ZONE_SPAWN_CHANGE_TYPE_WARP", "zone": "ZONE_BLACK_CITY", "warp": 0, "facing": 0,
                       "pos_weight_bits": 0, "tile": [0, 0], "height": 0},
        "works": {"EVENT_WORK_0x4030": 1, "EVENT_WORK_CONTINUE_SCRIPT": 0, "EVENT_WORK_0x40a3": 2,
                  "EVENT_WORK_0x40a5": 2},
        "playtime": "12:34:56",
        "money": 3500,
        "badges": {"count": 2, "bits": 0b101},
        "flags_set": 2,
        "party": [
            {"slot": 1, "species": "SPECIES_TEPIG", "level": 6, "hp": [23, 23], "exp": 229, "item": "ITEM_NONE",
             "moves": ["MOVE_TACKLE", "MOVE_TAIL_WHIP"], "checksum_ok": True},
            {"slot": 2, "species": "SPECIES_PURRLOIN", "level": 4, "hp": [17, 17], "exp": 64, "item": "ITEM_NONE",
             "moves": ["MOVE_SCRATCH", "MOVE_GROWL"], "checksum_ok": True},
        ],
    }
    CASTELIA_RAIL = {
        # 0x2000 is no warp direction's angle, so the reader gives the angle itself
        "location": {"zone": "ZONE_CASTELIA_CITY",
                     "rail": {"component": 2, "line": True, "direction": 0, "side": -6, "front": 28},
                     "facing": 0x2000, "state": "FLD_PLAYER_EXSTATE_NONE"},
        "now_spawn": {"type": "ZONE_SPAWN_CHANGE_TYPE_WARP", "zone": "ZONE_CASTELIA_CITY", "warp": 3,
                      "facing": "WARP_DIR_UP", "pos_weight_bits": 0x121,
                      "rail": {"component": 2, "line": True, "direction": 0, "side": -6, "front": 29}},
        "next_spawn": {"type": "ZONE_SPAWN_CHANGE_TYPE_POSITION", "zone": "ZONE_CASTELIA_CITY", "warp": -1,
                       "facing": "WARP_DIR_DOWN", "pos_weight_bits": 0,
                       "rail": {"component": 3, "line": True, "direction": 2, "side": 7, "front": 25}},
        "works": {"EVENT_WORK_0x4030": 1, "EVENT_WORK_0x4040": 7, "EVENT_WORK_CONTINUE_SCRIPT": 1,
                  "EVENT_WORK_0x4042": 9, "EVENT_WORK_0x40a3": 2, "EVENT_WORK_0x40a5": 2},
    }

    def facts(self, data: bytes | None = None) -> dict:
        return savelab.read(self.base if data is None else data, self.constants, flags=["0x961", "0x962", "0x963"])

    def check(self, facts: dict, expected: dict) -> None:
        for key, value in expected.items():
            self.assertEqual(facts[key], value, key)

    def check_route20(self, facts: dict) -> None:
        self.check(facts, self.ROUTE_20)
        # flags.h names 0x961 and 0x962 by their numbers, and 0x963 EVENT_FLAG_RUNNING_SHOES
        self.assertEqual(facts["flags"], {"EVENT_FLAG_CONTINUE_SCRIPT": False, "EVENT_FLAG_0x0961": True,
                                          "EVENT_FLAG_0x0962": True, "EVENT_FLAG_RUNNING_SHOES": False})

    def test_reader_gives_back_the_route_20_save(self):
        self.check_route20(self.facts())

    def test_reader_gives_back_a_place_on_a_rail(self):
        self.check(self.facts(castelia_rail_save(self.layout).image()), self.CASTELIA_RAIL)

    def test_a_moved_reader_offset_is_caught(self):
        # Each of the reader's offsets matters: moved by 2, the facts are no longer the ones the images were built with
        rail_save = castelia_rail_save(self.layout).image()
        # (The reader takes SaveLocation's rotation, rail flag and state together, at LOCATION_ROTATION)
        for name in ("LOCATION_ZONE", "LOCATION_POS", "LOCATION_RAIL_POS", "LOCATION_ROTATION", "PLAYER_NOW_SPAWN",
                     "PLAYER_NEXT_SPAWN", "TRAINER_DATA_PLAY_TIME", "TRAINER_GAME_INFO_CASH", "EVENT_WORK_FLAGS",
                     "EVENT_WORK_WORKS", "PARTY_PKM_LEVEL", "PKM_BLOCK_A_SPECIES", "BOX_PKM_CHECKSUM",
                     "POKE_PARTY_PKMS"):
            with self.subTest(name=name), mock.patch.object(savelab, name, getattr(savelab, name) + 2):
                try:
                    route20, castelia = self.facts(), self.facts(rail_save)
                except (savelab.LabError, ValueError, KeyError, IndexError, struct.error):
                    continue
                with self.assertRaises(AssertionError):
                    self.check_route20(route20)
                    self.check(castelia, self.CASTELIA_RAIL)

    def test_a_bad_checksum_is_reported(self):
        save = route20_save(self.layout)
        members = self.layout["PokeParty.pkms"]
        start, _ = save.blocks[self.layout["SAVE_BLOCK_PARTY"]]
        save.data[start + members + self.layout["BoxPkm.contentBuffer"]] ^= 1
        party = self.facts(save.image())["party"]
        self.assertFalse(party[0]["checksum_ok"])
        self.assertTrue(party[1]["checksum_ok"])

    def test_a_pokemon_stored_decrypted_is_read_as_it_is(self):
        # With BoxPkm.partyDecrypted set, PokeParty_DecryptPkm leaves the blocks and the party's fields as they are
        layout = self.layout
        party = (pokemon(layout, 0x0002_6000, SPECIES_RIOLU, ITEM_ORAN_BERRY, 1000,
                         (MOVE_QUICK_ATTACK, MOVE_LEER, 0, 0), 10, (30, 31), stored_decrypted=True),)
        facts = self.facts(route20_save(layout, party=party).image())
        self.assertEqual(facts["party"], [
            {"slot": 1, "species": "SPECIES_RIOLU", "level": 10, "hp": [30, 31], "exp": 1000,
             "item": "ITEM_ORAN_BERRY", "moves": ["MOVE_QUICK_ATTACK", "MOVE_LEER"], "checksum_ok": True}])

    def test_a_party_of_six(self):
        layout = self.layout
        # PIDs whose bits 13 to 17 pick shuffles 0, 5, 11, 17, 23 and 31 of blockShufLUT
        members = ((0x0000_0000, SPECIES_TEPIG, "SPECIES_TEPIG"), (0x0000_a000, SPECIES_PATRAT, "SPECIES_PATRAT"),
                   (0x0001_6000, SPECIES_LILLIPUP, "SPECIES_LILLIPUP"),
                   (0x0002_2000, SPECIES_PURRLOIN, "SPECIES_PURRLOIN"),
                   (0x0002_e000, SPECIES_PIDOVE, "SPECIES_PIDOVE"), (0x0003_e000, SPECIES_AZURILL, "SPECIES_AZURILL"))
        party = tuple(pokemon(layout, pid, species, 0, 100 + slot, (MOVE_TACKLE, 0, 0, 0), 5 + slot, (20, 20 + slot))
                      for slot, (pid, species, _) in enumerate(members))
        facts = self.facts(route20_save(layout, party=party).image())
        self.assertEqual([(pokemon["slot"], pokemon["species"], pokemon["level"], pokemon["hp"], pokemon["exp"])
                          for pokemon in facts["party"]],
                         [(slot + 1, name, 5 + slot, [20, 20 + slot], 100 + slot)
                          for slot, (_, _, name) in enumerate(members)])
        self.assertTrue(all(pokemon["checksum_ok"] for pokemon in facts["party"]))
        # Control: a count past PARTY_SIZE is no party the game wrote
        save = route20_save(layout, party=party)
        save.put("<i", "PARTY", layout["PokeParty.count"], layout["PARTY_SIZE"] + 1)
        with self.assertRaises(savelab.LabError) as caught:
            self.facts(save.image())
        self.assertIn("the party's count is 7, not 0 to 6", str(caught.exception))

    def test_all_flags_lists_them_by_name(self):
        facts = savelab.read(self.base, self.constants, all_flags=True)
        self.assertEqual(facts["set_flags"], ["EVENT_FLAG_0x0961", "EVENT_FLAG_0x0962"])


class EmptyRecipeTests(LabTest):
    def test_an_empty_recipe_leaves_the_save_unchanged(self):
        rail_save = castelia_rail_save(self.layout).image()
        for recipe in ("", "\n\n", "# only a comment\n   # and another\n"):
            for name, base in (("Route 20", self.base), ("Castelia's rail", rail_save)):
                with self.subTest(recipe=recipe, base=name):
                    self.assertEqual(self.mint(recipe, base), base)

    def test_control_a_recipe_that_sets_something_changes_it(self):
        self.assertNotEqual(self.mint("money 3501"), self.base)
        # Setting what is already there writes the same bytes and reseals the same CRCs
        self.assertEqual(self.mint("money 3500"), self.base)


class SealTests(LabTest):
    RECIPE = ("place ZONE_FLOCCESY_RANCH_2 warp 0\nflag EVENT_FLAG_TRAINER_YOUNGSTER_PETEY set\n"
              "work EVENT_WORK_0x40a3 3\nmoney 5000\nbadges 1\nplaytime 1:30:00\n")

    def written(self) -> set[int]:
        return {self.layout[f"SAVE_BLOCK_{name}"] for name in ("PLAYER", "EVENT_WORK", "TRAINER_GAME_INFO",
                                                               "TRAINER_DATA")}

    def test_both_copies_are_resealed_and_nothing_else_changes(self):
        minted = self.mint(self.RECIPE)
        self.assertEqual(saveimage.validate_image(minted, reference=self.base), [])
        blocks = saveimage.find_blocks(self.base)
        allowed = set()
        for copy in saveimage.COPIES:
            for block in self.written():
                start, size = blocks[block]
                data = minted[copy + start:copy + start + size]
                crc = saveimage.crc16_ccitt(data)
                count, footer_crc = struct.unpack_from("<HH", minted, copy + start + size)
                self.assertEqual((count, footer_crc), (1, crc), f"copy {copy:#x} block {block:#x}")
                self.assertEqual(struct.unpack_from("<H", minted, copy + saveimage.CRC_TABLE + 2 * block)[0], crc)
                allowed |= set(range(copy + start, copy + start + size))
                allowed |= {copy + start + size + 2, copy + start + size + 3}
                allowed |= {copy + saveimage.CRC_TABLE + 2 * block, copy + saveimage.CRC_TABLE + 2 * block + 1}
            footer = struct.unpack_from("<IIIHH", minted, copy + saveimage.SAVE_FOOTER)
            table = minted[copy + saveimage.CRC_TABLE:copy + saveimage.SAVE_FOOTER]
            self.assertEqual(footer, (1, saveimage.MAIN_SAVE_SIZE, saveimage.SAVE_MAGIC, 0,
                                      saveimage.crc16_ccitt(table)))
            allowed |= {copy + saveimage.SAVE_FOOTER + 14, copy + saveimage.SAVE_FOOTER + 15}
        changed = {offset for offset in range(len(minted)) if minted[offset] != self.base[offset]}
        self.assertTrue(changed)
        self.assertLessEqual(changed, allowed)
        self.assertEqual(minted[:saveimage.MAIN_SAVE_SIZE],
                         minted[saveimage.COPY_SIZE:saveimage.COPY_SIZE + saveimage.MAIN_SAVE_SIZE])
        self.assertEqual(minted[2 * saveimage.COPY_SIZE:], self.base[2 * saveimage.COPY_SIZE:])

    def test_minting_is_deterministic(self):
        self.assertEqual(self.mint(self.RECIPE), self.mint(self.RECIPE))

    def test_control_a_copy_left_unsealed_is_refused(self):
        original = savelab.SaveImage.seal

        def seal_first_copy_only(save):
            # The second copy's values are written but its CRCs are left as the base's, and the gate is let through
            with mock.patch.object(saveimage, "COPIES", (0,)), \
                 mock.patch.object(saveimage, "validate_image", return_value=[]), \
                 mock.patch.object(savelab, "copies_alike", return_value=True):
                sealed = original(save)
            return sealed

        with mock.patch.object(savelab.SaveImage, "seal", seal_first_copy_only):
            minted = self.mint("money 5000")
        problems = saveimage.validate_image(minted, reference=self.base)
        self.assertTrue(any(problem.startswith(f"copy {saveimage.COPY_SIZE:#x}") for problem in problems), problems)
        self.assertFalse(any(problem.startswith("copy 0x0") for problem in problems), problems)
        # The minter's own gate refuses that image
        save = savelab.SaveImage(self.base, "the base")
        save.data[:] = minted
        with self.assertRaises(savelab.LabError) as caught:
            save.seal()
        self.assertIn("fails the checks its base passed", str(caught.exception))

    def test_control_a_value_written_to_one_copy_is_refused(self):
        # Each copy then passes its CRCs, but the two differ, which the game's SAVE never leaves
        def write_first_copy_only(save, fmt, block, field, *values):
            struct.pack_into(fmt, save.data, save.offset(block, field, struct.calcsize(fmt)), *values)
            save.written.add(block)

        with mock.patch.object(savelab.SaveImage, "write", write_first_copy_only):
            self.refused("money 5000", "its two copies of the main save differ")

    def test_control_a_wrong_footer_offset_is_refused(self):
        for name in ("BLOCK_FOOTER_CRC", "SAVE_FOOTER_CRC"):
            with self.subTest(name=name), mock.patch.object(savelab, name, getattr(savelab, name) - 2):
                self.refused("money 5000", "fails the checks its base passed")

    def test_a_flipped_byte_in_a_written_block_fails_the_check(self):
        minted = bytearray(self.mint(self.RECIPE))
        start, _ = saveimage.find_blocks(self.base)[self.layout["SAVE_BLOCK_PLAYER"]]
        minted[saveimage.COPY_SIZE + start + 0x40] ^= 1
        problems = saveimage.validate_image(bytes(minted), reference=self.base)
        self.assertIn(f"copy {saveimage.COPY_SIZE:#x} block {self.layout['SAVE_BLOCK_PLAYER']}", problems[0])


class BaseTests(LabTest):
    def test_control_the_good_base_is_accepted(self):
        self.mint("place ZONE_FLOCCESY_RANCH_2 warp 0")

    def test_wrong_length(self):
        for base in (self.base[:-1], self.base + b"\xff", b""):
            with self.subTest(length=len(base)):
                self.refused("money 1", "isn't a valid save", base)

    def test_a_broken_crc_in_either_copy(self):
        block = self.layout["SAVE_BLOCK_EVENT_WORK"]
        start, _ = saveimage.find_blocks(self.base)[block]
        # saveimage finds the layout in the first copy, so a broken block there stops the layout's discovery, and one in
        # the second fails that copy's CRCs
        for copy, text in ((0, f"save layout: block {block}: no footer"),
                           (saveimage.COPY_SIZE, f"copy {saveimage.COPY_SIZE:#x} block {block}")):
            with self.subTest(copy=copy):
                broken = bytearray(self.base)
                broken[copy + start] ^= 1
                self.refused("money 1", text, bytes(broken))

    def test_copies_that_differ(self):
        # Copy 2 holds other money, with its own CRCs right: each copy passes on its own
        save = route20_save(self.layout)
        first = save.image()
        save.put("<I", "TRAINER_GAME_INFO", self.layout["TrainerGameInfoSave.cash"], 9)
        second = save.image()
        mixed = first[:saveimage.COPY_SIZE] + second[saveimage.COPY_SIZE:]
        self.assertEqual(saveimage.validate_image(mixed), [])
        self.refused("money 1", "two copies of the main save differ", mixed)

    def test_a_block_of_another_size(self):
        other = route20_save(self.layout, sizes={self.layout["SAVE_BLOCK_PLAYER"]: 0xac}).image()
        self.assertEqual(saveimage.validate_image(other), [])
        self.refused("money 1", "block 0x1c has 0xac bytes", other)

    def test_a_base_other_than_the_recipes(self):
        self.refused("base floccesy_save", "not one SAVES pins for floccesy_save")
        self.refused("base no_such_save", "is not one of oracle.py's SAVES")

    def test_a_base_that_saves_pins_is_taken(self):
        # A save of SAVES whose White 2 SHA-1 is the base's: the recipe goes on, and its first line reports the version
        pins = {"synthetic_save": oracle.SaveMaker(1, {"b2_us": "0" * 40,
                                                       "w2_us": hashlib.sha1(self.base).hexdigest()})}
        with mock.patch.dict(oracle.SAVES, pins):
            minted, applied = savelab.mint("base synthetic_save\nmoney 3501\n", self.base, "test.recipe",
                                           self.constants)
        self.assertEqual(applied, [{"line": 1, "base": "synthetic_save", "version": "w2_us"},
                                   {"line": 2, "money": 3501}])
        self.assertNotEqual(minted, self.base)

    def test_a_state_the_arrival_would_keep(self):
        surfing = route20_save(self.layout, ex_state=2).image()
        self.refused("place ZONE_FLOCCESY_RANCH_2 warp 0", "FLD_PLAYER_EXSTATE_SURF", surfing)
        # Control: the verbs that don't place take it
        self.assertNotEqual(self.mint("money 1", surfing), surfing)


class PlaceTests(LabTest):
    """Each form of place from the Castelia rail save, held byte for byte to that save built again with what the game's
    own writes would change: nextSpawn as the spawn's constructor makes it; SaveLocation's zone, position and rotation
    (ConvWarpDirToAngle: up 0, left 0x4000, down 0x8000, right 0xc000) and its rail flag cleared, its rail position and
    the player's state left; EVENT_FLAG_CONTINUE_SCRIPT set and EVENT_WORK_CONTINUE_SCRIPT 0"""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.rail_save = castelia_rail_save(cls.layout).image()

    def placed(self, spawn: tuple, zone: int, position: bytes, rotation: int) -> bytes:
        save = castelia_rail_save(self.layout)
        save.spawn("nextSpawn", *spawn)
        save.location(zone, rotation, False, 0, position)
        save.flag(EVENT_FLAG_CONTINUE_SCRIPT, True)
        save.work(EVENT_WORK_CONTINUE_SCRIPT, 0)
        return save.image()

    def test_warp(self):
        minted = self.mint("place ZONE_FLOCCESY_RANCH_2 warp 0", self.rail_save)
        # SetupZoneSpawnInfoWarp (src/system/zone_spawn.c): InitZoneSpawnInfo's zeros with the zone and warp.
        # res/field/events/floccesy_ranch_2.json's warp 0: grid (728, 32, 872), unk4 1, WARP_DIR_UP. GetGridWarpOutPos
        # with posWeightBits 0 makes it fx32 (<< 12), and ConvWarpDirToAngle(WARP_DIR_UP) is 0
        warp = events("floccesy_ranch_2")["warps"][0]
        self.assertEqual((warp["grid"]["x"], warp["grid"]["y"], warp["grid"]["z"], warp["unk4"]), (728, 32, 872, 1))
        self.assertImage(minted, self.placed((CHANGE_WARP, ZONE_FLOCCESY_RANCH_2, 0, 0, 0, False, bytes(12)),
                                             ZONE_FLOCCESY_RANCH_2, grid(728 << 12, 32 << 12, 872 << 12), 0))

    def test_warp_at_a_negative_height(self):
        minted = self.mint("place ZONE_GIANT_CHASM warp 0", self.rail_save)
        # res/field/events/giant_chasm.json's warp 0: grid (10952, 65504, 2440), unk4 2. GetGridWarpOutPos takes each
        # as an s16, so the height is -32 world units, below the map's level 0; WARP_DIR_DOWN's angle is 0x8000
        warp = events("giant_chasm")["warps"][0]
        self.assertEqual((warp["grid"]["x"], warp["grid"]["y"], warp["grid"]["z"], warp["unk4"]),
                         (10952, 65504, 2440, 2))
        self.assertImage(minted, self.placed((CHANGE_WARP, ZONE_GIANT_CHASM, 0, 0, 0, False, bytes(12)),
                                             ZONE_GIANT_CHASM, grid(10952 << 12, -32 << 12, 2440 << 12), 0x8000))

    def test_rail_warp(self):
        minted = self.mint("place ZONE_CASTELIA_CITY warp 0", self.rail_save)
        # res/field/events/castelia_city.json's warp 0 is on component 3's rail, unk4 2: the spawn names the warp alone,
        # and SaveLocation keeps the zone and the facing, its position zero, since the game puts the player on the rail
        # from nextSpawn (SetupZoneWarpArrival, GetRailWarpOutPos) before anything reads it
        warp = events("castelia_city")["warps"][0]
        self.assertEqual((warp["rail"]["component"], warp["unk4"]), (3, 2))
        self.assertImage(minted, self.placed((CHANGE_WARP, ZONE_CASTELIA_CITY, 0, 0, 0, False, bytes(12)),
                                             ZONE_CASTELIA_CITY, bytes(12), 0x8000))

    def test_at(self):
        # CreateZoneChangeData as FieldSetNextZone calls it (src/ov012/scrcmd_game_state.c): tiles << 16, warpId -1,
        # the facing as warpDir
        for recipe, height, facing, rotation in (
            ("place ZONE_ROUTE_20 at 155 651 2 facing WARP_DIR_LEFT", 2, WARP_DIR_LEFT, 0x4000),
            # Without a height and a facing: height 0, facing down
            ("place ZONE_ROUTE_20 at 155 651", 0, WARP_DIR_DOWN, 0x8000),
            # A level below 0: FieldSetNextZone's u16 65535 shifted into an s32
            ("place ZONE_ROUTE_20 at 155 651 -1 facing WARP_DIR_RIGHT", -1, WARP_DIR_RIGHT, 0xc000),
        ):
            with self.subTest(recipe=recipe):
                position = grid(155 << 16, height * 0x10000, 651 << 16)
                self.assertImage(self.mint(recipe, self.rail_save),
                                 self.placed((CHANGE_POSITION, ZONE_ROUTE_20, -1, facing, 0, False, position),
                                             ZONE_ROUTE_20, position, rotation))

    def test_fly(self):
        minted = self.mint("place ZONE_FLOCCESY_RANCH fly", self.rail_save)
        # LoadZoneSpawnInfoCheckRail: res/field/zones/floccesy_ranch.json's Fly position, each << 16
        # (LoadZoneFlyPosToVecFx32), facing down
        fly = zone_data("floccesy_ranch")["fly"]
        self.assertEqual((fly["x"], fly["y"], fly["z"]), (135, 0, 630))
        position = grid(135 << 16, 0, 630 << 16)
        self.assertImage(minted, self.placed((CHANGE_POSITION, ZONE_FLOCCESY_RANCH, -1, WARP_DIR_DOWN, 0, False,
                                              position), ZONE_FLOCCESY_RANCH, position, 0x8000))

    def test_rail_maps_are_placed_by_warp(self):
        # at takes a grid position, so it refuses every zone with a rail system (ZONE_RAIL_ASSIGNMENTS), and fly the
        # zones whose Fly position is a rail position (ZONES_WITH_RAILS)
        for recipe, text in (("place ZONE_CASTELIA_CITY at 1 1", "has a rail system"),
                             ("place ZONE_CASTELIA_CITY_2 at 1 1", "has a rail system"),
                             ("place ZONE_VILLAGE_BRIDGE at 5 4", "has a rail system"),
                             ("place ZONE_CASTELIA_CITY fly", "Fly position is on a rail")):
            with self.subTest(recipe=recipe):
                self.refused(recipe, text, self.rail_save)
        # A rail map outside ZONES_WITH_RAILS has a grid Fly position, which LoadZoneSpawnInfoCheckRail takes as it is
        fly = zone_data("village_bridge")["fly"]
        self.assertEqual((fly["x"], fly["y"], fly["z"]), (5, 0, 4))
        position = grid(5 << 16, 0, 4 << 16)
        self.assertImage(self.mint("place ZONE_VILLAGE_BRIDGE fly", self.rail_save),
                         self.placed((CHANGE_POSITION, ZONE_VILLAGE_BRIDGE, -1, WARP_DIR_DOWN, 0, False, position),
                                     ZONE_VILLAGE_BRIDGE, position, 0x8000))

    def test_a_zone_without_a_fly_position(self):
        # Eleven of Castelia City's zones outside ZONES_WITH_RAILS have Fly (0, 0, 0) in their zone data: the game's
        # own fallback would stand the player on the map's corner
        for zone in ("castelia_city_2", "castelia_city_7"):
            with self.subTest(zone=zone):
                fly = zone_data(zone)["fly"]
                self.assertEqual((fly["x"], fly["y"], fly["z"]), (0, 0, 0))
                self.refused(f"place ZONE_{zone.upper()} fly", "has no Fly position", self.rail_save)

    def test_place_refusals(self):
        for recipe, text in (
            ("place ZONE_FLOCCESY_RANCH_2 warp 1", "has 1 warps, so no warp 1"),
            ("place ZONE_FLOCCESY_RANCH_2 warp", "place takes"),
            ("place ZONE_FLOCCESY_RANCH_2 warp WARP_DIR_UP", "a warp is a number"),
            ("place ZONE_FLOCCESY_RANCH_2 fly 3", "place takes"),
            ("place ZONE_FLOCCESY_RANCH_2 door 0", "place takes"),
            ("place ZONE_NO_SUCH_PLACE warp 0", "neither a number nor a name"),
            ("place SPECIES_TEPIG warp 0", "a name of species, not of zones"),
            ("place 615 warp 0", "615 is not a zone"),
            ("place ZONE_ROUTE_20 at 1", "takes X Z [Y] [facing DIR]"),
            ("place ZONE_ROUTE_20 at 1 2 3 4", "takes X Z [Y] [facing DIR]"),
            ("place ZONE_ROUTE_20 at 1 2 facing SPECIES_TEPIG", "not of warp_dirs"),
            ("place ZONE_ROUTE_20 at 1 2 facing 0", "facing takes"),
            ("place ZONE_ROUTE_20 at 32768 2", "a tile is 0 to 32767"),
            ("place ZONE_ROUTE_20 at -1 2", "a tile is 0 to 32767"),
            ("place ZONE_ROUTE_20 at 1 2 32768", "a height is -32768 to 32767"),
            ("place ZONE_ROUTE_20 at 1 2 -32769", "a height is -32768 to 32767"),
            ("place ZONE_ROUTE_20 at 1 2\nplace ZONE_ROUTE_20 at 1 3", "line 1 already sets that"),
        ):
            with self.subTest(recipe=recipe):
                self.refused(recipe, text)


class VerbTests(LabTest):
    def test_each_verb_writes_exactly_what_the_game_reads(self):
        # Each recipe minted from the Route 20 save is that save built again with the one field changed: flag N is bit
        # N % 8 of flags[N / 8] (EventWork_GetFlagBytePtr, src/system/event_work.c), work W is works[W - 0x4000]
        # (EventWork_GetWkPtr), badge B is bit B (addBadge, src/system/trainer_game_info_savedata.c), and the play time
        # is u16 hours, u8 minutes, u8 seconds (include/save/playtime.h)
        layout = self.layout
        play_time = layout["TrainerDataSave.playTime"]
        cash, badges = layout["TrainerGameInfoSave.cash"], layout["TrainerGameInfoSave.badges"]
        for recipe, change in (
            ("flag EVENT_FLAG_TRAINER_YOUNGSTER_PETEY set",
             lambda save: save.flag(EVENT_FLAG_TRAINER_YOUNGSTER_PETEY, True)),
            ("flag 0x961 clear", lambda save: save.flag(0x961, False)),
            # Set on a set flag and clear on a clear one change nothing
            ("flag 0x962 set", lambda save: None),
            ("flag EVENT_FLAG_RUNNING_SHOES clear", lambda save: None),
            # Bit 7: flag 0x967 is the last of flags[0x12c], whose bits 1 and 2 are set
            ("flag 0x967 set", lambda save: save.flag(0x967, True)),
            ("work EVENT_WORK_0x40a3 3", lambda save: save.work(0x40a3, 3)),
            ("work 0x4100 SPECIES_TEPIG", lambda save: save.work(0x4100, SPECIES_TEPIG)),
            ("money 5000", lambda save: save.put("<I", "TRAINER_GAME_INFO", cash, 5000)),
            # The first N badges, whatever the base had: badges 0 and 2 become badge 0 alone
            ("badges 1", lambda save: save.put("<I", "TRAINER_GAME_INFO", badges, 0b1)),
            ("badges 0", lambda save: save.put("<I", "TRAINER_GAME_INFO", badges, 0)),
            ("badges 8", lambda save: save.put("<I", "TRAINER_GAME_INFO", badges, 0xff)),
            ("playtime 1:02:03", lambda save: save.put("<HBB", "TRAINER_DATA", play_time + layout["PlayTime.hours"],
                                                       1, 2, 3)),
        ):
            with self.subTest(recipe=recipe):
                expected = route20_save(layout)
                change(expected)
                self.assertImage(self.mint(recipe), expected.image())

    def test_a_recipe_of_every_verb_reads_back(self):
        minted = self.mint("flag EVENT_FLAG_TRAINER_YOUNGSTER_PETEY set\nflag 0x961 clear\nwork EVENT_WORK_0x40a3 3\n"
                           "work 0x4100 SPECIES_TEPIG\nmoney 9999999\nbadges 3\nplaytime 999:59:59\n")
        facts = savelab.read(minted, self.constants, flags=["EVENT_FLAG_TRAINER_YOUNGSTER_PETEY", "0x961"])
        self.assertEqual(facts["flags"], {"EVENT_FLAG_CONTINUE_SCRIPT": False,
                                          "EVENT_FLAG_TRAINER_YOUNGSTER_PETEY": True, "EVENT_FLAG_0x0961": False})
        self.assertEqual((facts["money"], facts["badges"], facts["playtime"]),
                         (9999999, {"count": 3, "bits": 0b111}, "999:59:59"))
        self.assertEqual((facts["works"]["EVENT_WORK_0x40a3"], facts["works"]["EVENT_WORK_0x4100"]), (3, SPECIES_TEPIG))

    def test_verb_refusals(self):
        for recipe, text in (
            ("flag EVENT_FLAG_TRAINER_YOUNGSTER_PETEY on", "flag takes NAME set"),
            ("flag 0 set", "not a saved flag"),
            ("flag 3064 set", "not a saved flag"),
            ("flag ZONE_ROUTE_20 set", "a name of zones, not of flags"),
            ("flag EVENT_FLAG_CONTINUE_SCRIPT set", "place's to set"),
            ("work EVENT_WORK_CONTINUE_SCRIPT 1", "place's to set"),
            ("work 0x41af 1", "not a saved work value"),
            ("work EVENT_WORK_0x40a3 65536", "a work value is 0 to 65535"),
            ("work EVENT_WORK_0x40a3", "work takes NAME VALUE"),
            ("money 10000000", "money is 0 to 9999999"),
            ("money lots", "money is a number"),
            ("badges 9", "badges is 0 to 8"),
            ("playtime 1000:00:00", "up to 999:59:59"),
            ("playtime 1:60:00", "playtime takes H:MM:SS"),
            ("playtime 1:5:00", "playtime takes H:MM:SS"),
            ("money 1\nmoney 2", "line 1 already sets that"),
            ("flag EVENT_FLAG_TRAINER_YOUNGSTER_PETEY set\nflag 0x778 clear", "line 1 already sets that"),
            ("teleport ZONE_ROUTE_20", "unknown verb teleport"),
        ):
            with self.subTest(recipe=recipe):
                self.refused(recipe, text)

    def test_refusals_name_the_line(self):
        self.refused("# a comment\n\nmoney 1\nbadges 10\n", "test.recipe:4:")


class CommandLineTests(LabTest):
    def run_main(self, *args: str) -> tuple[int, str, str]:
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            status = savelab.main(list(args))
        return status, out.getvalue(), err.getvalue()

    def test_mint_and_read(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "base.sav").write_bytes(self.base)
            (root / "ranch.recipe").write_text("place ZONE_FLOCCESY_RANCH_2 warp 0  # the ranch's gate\n")
            status, out, err = self.run_main("mint", str(root / "ranch.recipe"), "--base", str(root / "base.sav"),
                                             "--out", str(root / "ranch.sav"))
            self.assertEqual((status, err), (0, ""))
            report = json.loads(out)
            minted = (root / "ranch.sav").read_bytes()
            self.assertEqual(report["minted"], savelab.sha1(minted))
            self.assertEqual(report["bytes_changed"], sum(1 for a, b in zip(self.base, minted) if a != b))
            self.assertEqual(report["applied"][0]["place"]["arrival"], {"tile": [45, 54], "height": 2})
            # An existing save is never replaced without --force: it can be the player's own
            status, _, err = self.run_main("mint", str(root / "ranch.recipe"), "--base", str(root / "base.sav"),
                                           "--out", str(root / "base.sav"))
            self.assertEqual(status, 1)
            self.assertIn("pass --force", err)
            self.assertEqual((root / "base.sav").read_bytes(), self.base)
            status, _, _ = self.run_main("mint", str(root / "ranch.recipe"), "--base", str(root / "base.sav"),
                                         "--out", str(root / "base.sav"), "--force")
            self.assertEqual(status, 0)
            self.assertEqual((root / "base.sav").read_bytes(), minted)
            self.assertEqual(sorted(path.name for path in root.iterdir()), ["base.sav", "ranch.recipe", "ranch.sav"])
            status, out, _ = self.run_main("read", str(root / "ranch.sav"), "--work", "EVENT_WORK_0x40a3")
            self.assertEqual(status, 0)
            facts = json.loads(out)
            self.assertEqual(facts["location"]["zone"], "ZONE_FLOCCESY_RANCH_2")
            self.assertEqual(facts["next_spawn"]["zone"], "ZONE_FLOCCESY_RANCH_2")
            self.assertTrue(facts["flags"]["EVENT_FLAG_CONTINUE_SCRIPT"])

    def test_a_refusal_exits_1_with_its_reason(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "base.sav").write_bytes(self.base[:-1])
            (root / "bad.recipe").write_text("money 1\n")
            status, out, err = self.run_main("mint", str(root / "bad.recipe"), "--base", str(root / "base.sav"),
                                             "--out", str(root / "out.sav"))
            self.assertEqual((status, out), (1, ""))
            self.assertTrue(err.startswith("savelab: the base isn't a valid save"), err)
            self.assertFalse((root / "out.sav").exists())
            status, _, err = self.run_main("read", str(root / "missing.sav"))
            self.assertEqual(status, 1)
            self.assertIn("missing.sav", err)


# Deliberate bugs in savelab.py, each the text it replaces (found exactly once in savelab.py), the text that replaces
# it, and the tests, ClassName or ClassName.test_name, at least one of which must fail on it
MUTANTS = (
    # Writing and sealing both copies
    ("writes reach the first copy only",
     "        for copy in saveimage.COPIES:\n            struct.pack_into(fmt",
     "        for copy in (0,):\n            struct.pack_into(fmt",
     ("SealTests.test_both_copies_are_resealed_and_nothing_else_changes",)),
    ("seal reseals the first copy only",
     "        for copy in saveimage.COPIES:\n            for block in sorted",
     "        for copy in (0,):\n            for block in sorted",
     ("SealTests.test_both_copies_are_resealed_and_nothing_else_changes",)),
    ("seal leaves the blocks' footers",
     '                struct.pack_into("<H", self.data, copy + start + size + BLOCK_FOOTER_CRC, crc)\n', "",
     ("SealTests.test_both_copies_are_resealed_and_nothing_else_changes",)),
    ("seal leaves the CRC table's entries",
     '                struct.pack_into("<H", self.data, copy + saveimage.CRC_TABLE + 2 * block, crc)\n', "",
     ("SealTests.test_both_copies_are_resealed_and_nothing_else_changes",)),
    ("seal leaves the table's CRC",
     '            struct.pack_into("<H", self.data, copy + saveimage.SAVE_FOOTER + SAVE_FOOTER_CRC, table_crc)\n', "",
     ("SealTests.test_both_copies_are_resealed_and_nothing_else_changes",)),
    ("the minter's gate is gone",
     '        if problems:\n            raise LabError("the minted save fails',
     '        if False:\n            raise LabError("the minted save fails',
     ("SealTests.test_control_a_copy_left_unsealed_is_refused",
      "SealTests.test_control_a_wrong_footer_offset_is_refused")),
    ("the gate lets copies that differ through",
     "        if not problems and not copies_alike(sealed):\n", "        if False:\n",
     ("SealTests.test_control_a_value_written_to_one_copy_is_refused",)),
    # The base
    ("a base that fails validation is taken",
     "        problems = saveimage.validate_image(data)\n        if problems:\n",
     "        problems = []\n        if problems:\n",
     ("BaseTests.test_wrong_length", "BaseTests.test_a_broken_crc_in_either_copy")),
    ("a base whose copies differ is taken",
     "        if not copies_alike(data):\n            backup", "        if False:\n            backup",
     ("BaseTests.test_copies_that_differ",)),
    ("the blocks' sizes aren't checked",
     "            if self.blocks[block][1] != size:\n", "            if False:\n",
     ("BaseTests.test_a_block_of_another_size",)),
    ("no base is taken",
     "versions = [version for version, pinned in maker.sha1.items() if pinned == digest]", "versions = []",
     ("BaseTests.test_a_base_that_saves_pins_is_taken",)),
    ("the first version SAVES pins is reported",
     'return {"base": name, "version": versions[0]}', 'return {"base": name, "version": sorted(maker.sha1)[0]}',
     ("BaseTests.test_a_base_that_saves_pins_is_taken",)),
    ("a base saved surfing is taken",
     "    if ex_state != FLD_PLAYER_EXSTATE_NONE:\n", "    if False:\n",
     ("BaseTests.test_a_state_the_arrival_would_keep",)),
    # The layout
    ("SaveLocation.pos 4 bytes on", "LOCATION_POS = 0x04\n", "LOCATION_POS = 0x08\n",
     ("LayoutTests.test_savelab_layout_is_the_headers",)),
    ("nextSpawn 4 bytes on", "PLAYER_NEXT_SPAWN = 0x38\n", "PLAYER_NEXT_SPAWN = 0x3c\n",
     ("LayoutTests.test_savelab_layout_is_the_headers",)),
    ("the flags a byte on", "EVENT_WORK_FLAGS = 0x35e\n", "EVENT_WORK_FLAGS = 0x35f\n",
     ("LayoutTests.test_savelab_layout_is_the_headers",)),
    ("the play time 2 bytes on", "TRAINER_DATA_PLAY_TIME = 0x24\n", "TRAINER_DATA_PLAY_TIME = 0x26\n",
     ("LayoutTests.test_savelab_layout_is_the_headers",)),
    ("the level a byte on", "PARTY_PKM_LEVEL = 0x8c\n", "PARTY_PKM_LEVEL = 0x8d\n",
     ("LayoutTests.test_savelab_layout_is_the_headers",)),
    # place
    ("the facings' angles swapped",
     "WARP_DIR_ANGLES = {1: 0x0000, 3: 0x4000, 2: 0x8000, 4: 0xc000}",
     "WARP_DIR_ANGLES = {1: 0x0000, 3: 0xc000, 2: 0x8000, 4: 0x4000}",
     ("PlaceTests.test_at",)),
    ("right's angle a unit off", "4: 0xc000}", "4: 0xc001}",
     ("PlaceTests.test_at", "ReaderTests.test_reader_gives_back_the_route_20_save")),
    ("a warp spawn takes the warp's facing",
     "        spawn = (ZONE_SPAWN_CHANGE_TYPE_WARP, zone, warp_id, 0, 0, 0, bytes(12))",
     '        spawn = (ZONE_SPAWN_CHANGE_TYPE_WARP, zone, warp_id, warp["unk4"], 0, 0, bytes(12))',
     ("PlaceTests.test_warp",)),
    ("a warp's height read unsigned",
     "position = tuple(to_s16(grid[axis]) << FX32_SHIFT", "position = tuple(grid[axis] << FX32_SHIFT",
     ("PlaceTests.test_warp_at_a_negative_height",)),
    ("a rail warp's SaveLocation keeps the base's position",
     "            rail_arrival = True\n            position = (0, 0, 0)\n",
     '            rail_arrival = True\n            position = save.read("<3i", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + '
     'LOCATION_POS)\n',
     ("PlaceTests.test_rail_warp",)),
    ("tiles at their centre",
     "        position = (x << TILE_SHIFT, y << TILE_SHIFT, z << TILE_SHIFT)",
     "        position = ((x * 16 + 8) << FX32_SHIFT, y << TILE_SHIFT, (z * 16 + 8) << FX32_SHIFT)",
     ("PlaceTests.test_at",)),
    ("at's height left out",
     '    y = integer(words[2], -0x8000, 0x7fff, "a height", where) if len(words) == 3 else 0\n', "    y = 0\n",
     ("PlaceTests.test_at",)),
    ("at's height above 0 only", '-0x8000, 0x7fff, "a height"', '0, 0x7fff, "a height"',
     ("PlaceTests.test_at",)),
    ("fly faces up",
     'x, y, z, facing = fly["x"], fly["y"], fly["z"], WARP_DIR_DOWN',
     'x, y, z, facing = fly["x"], fly["y"], fly["z"], 1',
     ("PlaceTests.test_fly",)),
    ("at takes a zone with a rail system",
     '            if zone in zonedata_zones(constants, "ZONE_RAIL_ASSIGNMENTS"):\n', "            if False:\n",
     ("PlaceTests.test_rail_maps_are_placed_by_warp",)),
    ("fly takes a zone whose Fly position is on a rail",
     '            if zone in zonedata_zones(constants, "ZONES_WITH_RAILS"):\n', "            if False:\n",
     ("PlaceTests.test_rail_maps_are_placed_by_warp",)),
    ("fly takes the empty Fly position", "            if x == y == z == 0:\n", "            if False:\n",
     ("PlaceTests.test_a_zone_without_a_fly_position",)),
    ("the continue flag left clear",
     '    write_flag(save, constants.value("EVENT_FLAG_CONTINUE_SCRIPT", "flags", where), True)\n', "",
     ("PlaceTests.test_warp",)),
    ("the continue work left as the base has it",
     '    write_work(save, constants.value("EVENT_WORK_CONTINUE_SCRIPT", "vars", where), 0)\n', "",
     ("PlaceTests.test_warp",)),
    ("the work after the continue work cleared instead",
     'constants.value("EVENT_WORK_CONTINUE_SCRIPT", "vars", where), 0)',
     'constants.value("EVENT_WORK_CONTINUE_SCRIPT", "vars", where) + 1, 0)',
     ("PlaceTests.test_warp",)),
    ("SaveLocation's rail flag left as the base has it",
     '    save.write("<B", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_IS_RAIL, 0)\n', "",
     ("PlaceTests.test_warp",)),
    ("place also sets the player's state to cycling",
     '    save.write("<B", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_IS_RAIL, 0)\n',
     '    save.write("<BB", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_IS_RAIL, 0, 1)\n',
     ("PlaceTests.test_warp",)),
    ("place clears the follower after SaveLocation",
     '    save.write("<B", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_IS_RAIL, 0)\n',
     '    save.write("<B", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_IS_RAIL, 0)\n'
     '    save.write("<8s", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + 0x1c, bytes(8))\n',
     ("PlaceTests.test_warp",)),
    ("place writes nowSpawn too",
     "    save.write(SPAWN_FORMAT, SAVE_BLOCK_PLAYER, PLAYER_NEXT_SPAWN, *spawn)\n",
     "    save.write(SPAWN_FORMAT, SAVE_BLOCK_PLAYER, PLAYER_NEXT_SPAWN, *spawn)\n"
     "    save.write(SPAWN_FORMAT, SAVE_BLOCK_PLAYER, PLAYER_NOW_SPAWN, *spawn)\n",
     ("PlaceTests.test_warp",)),
    # The other verbs
    ("an empty recipe writes",
     "    applied: list[dict] = []\n    seen",
     '    applied: list[dict] = []\n    save.write("<B", SAVE_BLOCK_PLAYER, PLAYER_LOCATION + LOCATION_IS_RAIL, 1)\n'
     "    seen",
     ("EmptyRecipeTests.test_an_empty_recipe_leaves_the_save_unchanged",)),
    ("a line may set what another set", "        if target in seen:\n", "        if False:\n",
     ("VerbTests.test_verb_refusals",)),
    ("flags toggled", "byte | mask if value else byte & ~mask", "byte ^ mask",
     ("VerbTests.test_each_verb_writes_exactly_what_the_game_reads",)),
    ("bit 7's flag in the next byte",
     "    offset = EVENT_WORK_FLAGS + flag // 8\n", "    offset = EVENT_WORK_FLAGS + (flag + 1) // 8\n",
     ("VerbTests.test_each_verb_writes_exactly_what_the_game_reads",)),
    ("works one along",
     "EVENT_WORK_WORKS + 2 * (work - EVENT_WORK_FIRST), value)",
     "EVENT_WORK_WORKS + 2 * (work - EVENT_WORK_FIRST + 1), value)",
     ("VerbTests.test_each_verb_writes_exactly_what_the_game_reads",)),
    ("money uncapped",
     "cash = integer(args[0], 0, TRAINER_GAME_INFO_CASH_MAX,", "cash = integer(args[0], 0, 0xffffffff,",
     ("VerbTests.test_verb_refusals",)),
    ("money written big-endian",
     'save.write("<I", SAVE_BLOCK_TRAINER_GAME_INFO, TRAINER_GAME_INFO_CASH, cash)',
     'save.write(">I", SAVE_BLOCK_TRAINER_GAME_INFO, TRAINER_GAME_INFO_CASH, cash)',
     ("VerbTests.test_each_verb_writes_exactly_what_the_game_reads",)),
    ("money clears the badges",
     '            save.write("<I", SAVE_BLOCK_TRAINER_GAME_INFO, TRAINER_GAME_INFO_CASH, cash)\n',
     '            save.write("<II", SAVE_BLOCK_TRAINER_GAME_INFO, TRAINER_GAME_INFO_CASH, cash, 0)\n',
     ("VerbTests.test_each_verb_writes_exactly_what_the_game_reads",)),
    ("badges count from 1", "(1 << count) - 1)", "(1 << (count + 1)) - 1)",
     ("VerbTests.test_each_verb_writes_exactly_what_the_game_reads",)),
    ("badges keep the base's", "(1 << count) - 1)",
     '((1 << count) - 1) | save.read("<I", SAVE_BLOCK_TRAINER_GAME_INFO, TRAINER_GAME_INFO_BADGES)[0])',
     ("VerbTests.test_each_verb_writes_exactly_what_the_game_reads",)),
    ("the play time's seconds written as its minutes",
     "play_time + PLAY_TIME_MINUTES, minutes)", "play_time + PLAY_TIME_MINUTES, seconds)",
     ("VerbTests.test_each_verb_writes_exactly_what_the_game_reads",)),
    ("the play time's minutes written as its seconds",
     "play_time + PLAY_TIME_SECONDS, seconds)", "play_time + PLAY_TIME_SECONDS, minutes)",
     ("VerbTests.test_each_verb_writes_exactly_what_the_game_reads",)),
    ("a name of another kind taken", "        if kind is not None and named_kind != kind:\n", "        if False:\n",
     ("PlaceTests.test_place_refusals", "VerbTests.test_verb_refusals")),
    ("flag 0 taken",
     "    if not 0 < flag < EVENT_FLAG_SAVED_COUNT:\n", "    if not 0 <= flag < EVENT_FLAG_SAVED_COUNT:\n",
     ("VerbTests.test_verb_refusals",)),
    ("the work after the last taken",
     "    if not EVENT_WORK_FIRST <= work < EVENT_WORK_FIRST + EVENT_WORK_COUNT:\n",
     "    if not EVENT_WORK_FIRST <= work <= EVENT_WORK_FIRST + EVENT_WORK_COUNT:\n",
     ("VerbTests.test_verb_refusals",)),
    ("an existing --out replaced without --force", "    if os.path.exists(path) and not force:\n", "    if False:\n",
     ("CommandLineTests.test_mint_and_read",)),
    # The reader
    ("the blocks left encrypted", "            content = crypt(content, checksum)\n", "",
     ("ReaderTests.test_reader_gives_back_the_route_20_save",)),
    ("the party's own fields left encrypted", "            stats = crypt(stats, pid)\n", "",
     ("ReaderTests.test_reader_gives_back_the_route_20_save",)),
    ("a Pokémon stored decrypted decrypted again",
     "        if not raw[BOX_PKM_FLAGS] & BOX_PKM_PARTY_DECRYPTED:\n", "        if True:\n",
     ("ReaderTests.test_a_pokemon_stored_decrypted_is_read_as_it_is",)),
    ("a checksum never wrong",
     '"checksum_ok": sum(struct.unpack(f"<{PKM_BUFFER_SIZE // 2}H", content)) & 0xffff == checksum,',
     '"checksum_ok": True,',
     ("ReaderTests.test_a_bad_checksum_is_reported",)),
    ("a party of six refused", "    if not 0 <= count <= PARTY_SIZE:\n", "    if not 0 <= count < PARTY_SIZE:\n",
     ("ReaderTests.test_a_party_of_six",)),
    ("next_spawn read from nowSpawn",
     '"next_spawn": spawn_facts(save, constants, PLAYER_NEXT_SPAWN),',
     '"next_spawn": spawn_facts(save, constants, PLAYER_NOW_SPAWN),',
     ("ReaderTests.test_reader_gives_back_the_route_20_save",)),
    ("a rail position's side and front swapped",
     "        component, is_line, direction, side, front = struct.unpack_from(RAIL_POSITION_FORMAT, position)\n",
     "        component, is_line, direction, front, side = struct.unpack_from(RAIL_POSITION_FORMAT, position)\n",
     ("ReaderTests.test_reader_gives_back_a_place_on_a_rail",)),
    ("SaveLocation's rail flag ignored",
     "        **position_facts(rail if is_rail else position, bool(is_rail)),\n",
     "        **position_facts(position, False),\n",
     ("ReaderTests.test_reader_gives_back_a_place_on_a_rail",)),
    ("a spawn's rail flag ignored",
     "        **position_facts(position, bool(is_rail)),\n", "        **position_facts(position, False),\n",
     ("ReaderTests.test_reader_gives_back_a_place_on_a_rail",)),
)


@contextlib.contextmanager
def layouts_remembered():
    """saveimage finds an image's 73 blocks by walking its first copy 4 bytes at a time, and the mutants' runs mint the
    same few saves hundreds of times: while they run, each image's blocks are kept by its SHA-1, of which they are a
    function, so that a run takes seconds, not a minute"""
    discover = saveimage._discover_blocks
    known: dict[tuple[bytes, int], list[tuple[int, int]]] = {}

    def remembered(save: bytes, count: int) -> list[tuple[int, int]]:
        key = (hashlib.sha1(save).digest(), count)
        if key not in known:
            known[key] = discover(save, count)
        return list(known[key])

    with mock.patch.object(saveimage, "_discover_blocks", remembered):
        yield


class MutationTests(unittest.TestCase):
    """Every row of MUTANTS: savelab.py with that one bug, loaded in savelab's place, fails at least one of its tests;
    and savelab as it is passes all of them through the same runner, so that what fails is the bug"""

    @classmethod
    def setUpClass(cls):
        cls.source = Path(savelab.__file__).read_text(encoding="utf-8")

    def load(self, source: str) -> types.ModuleType:
        """A module made from that source as savelab.py's, its path as savelab.py's own"""
        module = types.ModuleType("savelab_mutant")
        module.__file__ = savelab.__file__
        with mock.patch.object(sys, "path", list(sys.path)):
            exec(compile(source, savelab.__file__, "exec"), module.__dict__)
        return module

    def run_tests(self, module: types.ModuleType, names: tuple[str, ...], first_failure: bool) -> unittest.TestResult:
        """The tests named, with module as savelab, each class a subclass of its own so that its class attributes
        aren't the suite's; with first_failure, they stop at the first test or subtest that fails"""
        suite = unittest.TestSuite()
        for name in names:
            class_name, _, method = name.partition(".")
            isolated = type(class_name, (globals()[class_name],), {"__module__": __name__})
            suite.addTests([isolated(method)] if method else unittest.defaultTestLoader.loadTestsFromTestCase(isolated))
        result = unittest.TestResult()
        result.failfast = first_failure
        with mock.patch.dict(globals(), {"savelab": module}), layouts_remembered():
            suite.run(result)
        return result

    def test_savelab_as_it_is_passes_the_mutants_tests(self):
        names = tuple(sorted({name for *_, tests in MUTANTS for name in tests}))
        result = self.run_tests(self.load(self.source), names, first_failure=False)
        self.assertEqual(result.testsRun, len(names))
        self.assertTrue(result.wasSuccessful(), [str(test) for test, _ in result.failures + result.errors])

    def test_each_mutant_fails_its_tests(self):
        self.assertEqual(len({name for name, *_ in MUTANTS}), len(MUTANTS))
        for name, old, new, tests in MUTANTS:
            with self.subTest(mutant=name):
                self.assertEqual(self.source.count(old), 1, "the text a mutant replaces is in savelab.py once")
                self.assertNotEqual(old, new)
                result = self.run_tests(self.load(self.source.replace(old, new)), tests, first_failure=True)
                self.assertFalse(result.wasSuccessful(), f"savelab with this bug passes {', '.join(tests)}")


def main() -> None:
    global LAYOUT_PROGRAM
    argv = sys.argv[1:]
    if "--layout" not in argv or argv.index("--layout") + 1 >= len(argv):
        print("usage: test_savelab.py --layout PROGRAM [unittest options] (PROGRAM: the build's "
              "port_test_savelab_layout)", file=sys.stderr)
        sys.exit(2)
    index = argv.index("--layout")
    LAYOUT_PROGRAM = os.path.abspath(argv[index + 1])
    del argv[index:index + 2]
    unittest.main(argv=[sys.argv[0]] + argv)


if __name__ == "__main__":
    main()
