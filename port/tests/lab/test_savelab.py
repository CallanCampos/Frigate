#!/usr/bin/env python3
"""ROM-free tests of the save lab's minter and reader (port/tools/lab/savelab.py), on synthetic saves built from the
decomp's headers' layout: savelab_layout.c, compiled as the host build compiles the game's files, prints the offsets,
sizes and numbers of include/save/*.h, include/field/zone.h and include/pml/poke_party.h, and the tests build a full
backup image from them field by field (both copies of the main save, 73 blocks with footers, the CRC table and the
save footer, as src/gfl/backup_system.c lays them out), then hold savelab to them:

  - its own table of the layout is the headers', and its reader gives back each field the image was built with;
  - an empty recipe leaves the save unchanged;
  - a recipe's writes are resealed in both copies, each block's footer CRC and table entry and the table's CRC, and
    nothing else changes;
  - a bad base (the wrong size, a broken CRC in either copy, copies that differ, a foreign block layout, the wrong
    oracle save, a state the arrival would keep) is refused;
  - place writes the spawn, the continue flag and SaveLocation as the game's own constructors would, and each verb
    writes what the game reads.

Each check has a negative control that makes it fail: a layout entry, a reader offset or a footer offset moved, a
copy left unsealed, a recipe that changes something. The expected values are worked out by hand from the game's C and
data (the comments say which), never from savelab. Nothing here is a game save or comes from the ROM. Game
functionality: the saves (where CONTINUE puts the player, the event flags and work values, the play time, the money and
badges, the party).

    test_savelab.py --layout PROGRAM [unittest options]

PROGRAM is the build's port_test_savelab_layout; the ctest test lab_savelab (port/cmake/lab.cmake) passes it. The
generated constants headers (build/include/generated/constants) must exist, as after any configure.
"""

from __future__ import annotations

import contextlib
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
import unittest
from unittest import mock

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[2]
sys.path.insert(0, str(REPO_ROOT / "port" / "tools" / "lab"))
sys.path.insert(0, str(REPO_ROOT / "port" / "tools" / "oracle"))

import saveimage  # noqa: E402
import savelab  # noqa: E402

LAYOUT_PROGRAM = None

# PML_CryptoRun's generator (src/system/poke_tool.c, PML_CryptoNextKey)
CRYPTO_MULTIPLIER = 0x41c64e6d
CRYPTO_INCREMENT = 0x6073
# The constants the tests name, from build/include/generated/constants (zones.h, flags.h, vars.h, species.h, moves.h)
ZONE_ROUTE_20 = 446
ZONE_FLOCCESY_RANCH = 444
ZONE_FLOCCESY_RANCH_2 = 445
EVENT_FLAG_CONTINUE_SCRIPT = 0x965
EVENT_WORK_CONTINUE_SCRIPT = 0x4041
EVENT_FLAG_TRAINER_YOUNGSTER_PETEY = 0x778
SPECIES_TEPIG = 498
SPECIES_PURRLOIN = 509
MOVE_TACKLE = 33
MOVE_TAIL_WHIP = 39


def headers_layout() -> dict[str, int]:
    """savelab_layout's lines, NAME VALUE"""
    output = subprocess.run([LAYOUT_PROGRAM], check=True, capture_output=True, text=True).stdout
    layout = {}
    for line in output.splitlines():
        name, value = line.split()
        layout[name] = int(value, 16)
    return layout


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


def pokemon(layout: dict[str, int], pid: int, species: int, item: int, exp: int, moves: tuple[int, ...], level: int,
            hp: tuple[int, int]) -> bytes:
    """A party Pokémon as the game stores it (PokeParty_EncryptPkm): blocks A and B where the PID's bits 13 to 17 put
    them, the checksum the sum of the blocks' u16s, the blocks encrypted with it and the party's fields with the PID"""
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
    raw[content_at:content_at + len(content)] = crypt(bytes(content), checksum)
    raw[layout["PartyPkm.statusCond"]:] = crypt(bytes(party_fields), pid)
    return bytes(raw)


def organic_like(layout: dict[str, int], ex_state: int = 0, sizes: dict[int, int] | None = None) -> Synthetic:
    """A save shaped as the Route 20 save is: the player saved on ZONE_ROUTE_20 at tile (130, 661), height 2, facing
    right, walking; the last door the warp 0 of ZONE_ROUTE_19; two Pokémon; story work 0x40a3 and 0x40a5 at 2"""
    save = Synthetic(layout, sizes)
    location = layout["PlayerSave.location"]
    spawn = layout["PlayerSave.nowSpawn"]
    # A tile's centre, (tile * 16 + 8) world units as fx32, as the game saves its player state
    save.put("<H", "PLAYER", location + layout["SaveLocation.zoneId"], ZONE_ROUTE_20)
    save.put("<3i", "PLAYER", location + layout["SaveLocation.pos"],
             (130 * 16 + 8) << 12, 0x2001f, (661 * 16 + 8) << 12)
    save.put("<H", "PLAYER", location + layout["SaveLocation.unk18"], 0xc000)
    save.put("<B", "PLAYER", location + layout["SaveLocation.exState"], ex_state)
    save.put("<I", "PLAYER", spawn + layout["ZoneSpawnInfo.changeType"], 0)
    save.put("<hhhH", "PLAYER", spawn + layout["ZoneSpawnInfo.zoneId"], 437, 0, 1, 305)
    save.put("<3i", "PLAYER", spawn + layout["ZoneSpawnInfo.pos"], 53 << 16, 1 << 16, 703 << 16)
    works = layout["EventWork.works"]
    for work, value in ((0x40a3, 2), (0x40a5, 2), (0x4030, 1)):
        save.put("<H", "EVENT_WORK", works + 2 * (work - 0x4000), value)
    flags = layout["EventWork.flags"]
    # Flags 0x961 and 0x962 (a Pokémon and the Pokédex): bits 1 and 2 of flags[0x961 / 8]
    save.put("<B", "EVENT_WORK", flags + 0x961 // 8, 0b110)
    play_time = layout["TrainerDataSave.playTime"]
    save.put("<H", "TRAINER_DATA", play_time + layout["PlayTime.hours"], 12)
    save.put("<BB", "TRAINER_DATA", play_time + layout["PlayTime.minutes"], 34, 56)
    save.put("<I", "TRAINER_GAME_INFO", layout["TrainerGameInfoSave.cash"], 3500)
    save.put("<I", "TRAINER_GAME_INFO", layout["TrainerGameInfoSave.badges"], 0b101)
    save.put("<ii", "PARTY", layout["PokeParty.count"] - 4, layout["PARTY_SIZE"], 2)
    members = layout["PokeParty.pkms"]
    save.put(f"<{layout['sizeof(PartyPkm)']}s", "PARTY", members,
             pokemon(layout, 0x1234_5678, SPECIES_TEPIG, 0, 229, (MOVE_TACKLE, MOVE_TAIL_WHIP, 0, 0), 6, (23, 23)))
    save.put(f"<{layout['sizeof(PartyPkm)']}s", "PARTY", members + layout["sizeof(PartyPkm)"],
             pokemon(layout, 0x8765_4321, SPECIES_PURRLOIN, 0, 64, (10, 45, 0, 0), 4, (17, 17)))
    return save


class LabTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.layout = headers_layout()
        cls.constants = savelab.Constants()
        cls.base = organic_like(cls.layout).image()

    def mint(self, recipe: str, base: bytes | None = None) -> bytes:
        return savelab.mint(recipe, self.base if base is None else base, "test.recipe", self.constants)[0]

    def refused(self, recipe: str, text: str, base: bytes | None = None) -> None:
        with self.assertRaises(savelab.LabError) as caught:
            self.mint(recipe, base)
        self.assertIn(text, str(caught.exception))

    def field(self, data: bytes, fmt: str, block_name: str, field: int, copy: int = 0) -> tuple:
        start, _ = saveimage.find_blocks(data)[self.layout[f"SAVE_BLOCK_{block_name}"]]
        return struct.unpack_from(fmt, data, copy + start + field)


class LayoutTests(LabTest):
    def test_savelab_layout_is_the_headers(self):
        self.assertEqual(compare_layout(savelab.LAYOUT, self.layout), [])

    def test_a_moved_layout_entry_is_caught(self):
        for name in ("SaveLocation.pos", "EventWork.flags", "PartyPkm.level", "sizeof(PlayerSave)"):
            with self.subTest(name=name):
                moved = dict(savelab.LAYOUT, **{name: savelab.LAYOUT[name] + 2})
                self.assertEqual(compare_layout(moved, self.layout),
                                 [f"{name}: savelab has {moved[name]:#x}, the headers {self.layout[name]:#x}"])

    def test_block_shuffle_is_the_games(self):
        table = games_block_shuffle()
        self.assertEqual(len(table), 32)
        self.assertEqual([tuple(row) for row in savelab.BLOCK_SHUFFLE], table)
        # Control: the comparison sees a row swapped
        swapped = list(savelab.BLOCK_SHUFFLE)
        swapped[1], swapped[2] = swapped[2], swapped[1]
        self.assertNotEqual(swapped, table)

    def test_rail_zones_are_the_games(self):
        zones = savelab.rail_fly_zones(self.constants)
        self.assertEqual(len(zones), 12)
        self.assertIn(self.constants.value("ZONE_CASTELIA_CITY", "zones", "test"), zones)
        self.assertNotIn(ZONE_ROUTE_20, zones)
        # Control: without the game's table, at and fly can't tell, so they refuse
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "zonedata.c"
            source.write_text("// no table\n")
            with mock.patch.object(savelab, "ZONEDATA_C", source):
                with self.assertRaises(savelab.LabError) as caught:
                    savelab.rail_fly_zones(self.constants)
        self.assertIn("ZONES_WITH_RAILS", str(caught.exception))


class ReaderTests(LabTest):
    EXPECTED = {
        "location": {"zone": "ZONE_ROUTE_20", "tile": [130, 661], "height": 2, "facing": "WARP_DIR_RIGHT",
                     "state": "FLD_PLAYER_EXSTATE_NONE"},
        "playtime": "12:34:56",
        "money": 3500,
        "badges": {"count": 2, "bits": 0b101},
    }

    def facts(self, data: bytes | None = None) -> dict:
        return savelab.read(self.base if data is None else data, self.constants, flags=["0x961", "0x962", "0x963"])

    def check(self, facts: dict) -> None:
        for key, value in self.EXPECTED.items():
            self.assertEqual(facts[key], value, key)
        self.assertEqual(facts["now_spawn"]["zone"], "ZONE_ROUTE_19")
        self.assertEqual(facts["now_spawn"]["facing"], "WARP_DIR_UP")
        self.assertEqual(facts["now_spawn"]["tile"], [53, 703])
        self.assertEqual(facts["works"], {"EVENT_WORK_0x4030": 1, "EVENT_WORK_CONTINUE_SCRIPT": 0,
                                          "EVENT_WORK_0x40a3": 2, "EVENT_WORK_0x40a5": 2})
        flag_name = self.constants.by_value["flags"]
        self.assertEqual(facts["flags"], {flag_name[EVENT_FLAG_CONTINUE_SCRIPT]: False, flag_name[0x961]: True,
                                          flag_name[0x962]: True, flag_name[0x963]: False})
        self.assertEqual(facts["flags_set"], 2)
        self.assertEqual(facts["party"], [
            {"slot": 1, "species": "SPECIES_TEPIG", "level": 6, "hp": [23, 23], "exp": 229, "item": "ITEM_NONE",
             "moves": ["MOVE_TACKLE", "MOVE_TAIL_WHIP"], "checksum_ok": True},
            {"slot": 2, "species": "SPECIES_PURRLOIN", "level": 4, "hp": [17, 17], "exp": 64, "item": "ITEM_NONE",
             "moves": ["MOVE_SCRATCH", "MOVE_GROWL"], "checksum_ok": True},
        ])

    def test_reader_gives_back_the_fields_built_from_the_headers(self):
        self.check(self.facts())

    def test_a_moved_reader_offset_is_caught(self):
        # Each of the reader's offsets matters: moved by 2, the facts are no longer the ones the image was built with
        for name in ("LOCATION_ZONE", "LOCATION_POS", "LOCATION_ROTATION", "TRAINER_DATA_PLAY_TIME",
                     "TRAINER_GAME_INFO_CASH", "EVENT_WORK_FLAGS", "EVENT_WORK_WORKS", "PARTY_PKM_LEVEL",
                     "PKM_BLOCK_A_SPECIES", "BOX_PKM_CHECKSUM", "POKE_PARTY_PKMS"):
            with self.subTest(name=name), mock.patch.object(savelab, name, getattr(savelab, name) + 2):
                try:
                    facts = self.facts()
                except (savelab.LabError, ValueError, KeyError, IndexError, struct.error):
                    continue
                with self.assertRaises(AssertionError):
                    self.check(facts)

    def test_a_bad_checksum_is_reported(self):
        save = organic_like(self.layout)
        members = self.layout["PokeParty.pkms"]
        start, _ = save.blocks[self.layout["SAVE_BLOCK_PARTY"]]
        save.data[start + members + self.layout["BoxPkm.contentBuffer"]] ^= 1
        party = self.facts(save.image())["party"]
        self.assertFalse(party[0]["checksum_ok"])
        self.assertTrue(party[1]["checksum_ok"])

    def test_all_flags_lists_them_by_name(self):
        facts = savelab.read(self.base, self.constants, all_flags=True)
        flag_name = self.constants.by_value["flags"]
        self.assertEqual(facts["set_flags"], [flag_name[0x961], flag_name[0x962]])


class EmptyRecipeTests(LabTest):
    def test_an_empty_recipe_leaves_the_save_unchanged(self):
        for recipe in ("", "\n\n", "# only a comment\n   # and another\n"):
            with self.subTest(recipe=recipe):
                self.assertEqual(self.mint(recipe), self.base)

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
        save = organic_like(self.layout)
        first = save.image()
        save.put("<I", "TRAINER_GAME_INFO", self.layout["TrainerGameInfoSave.cash"], 9)
        second = save.image()
        mixed = first[:saveimage.COPY_SIZE] + second[saveimage.COPY_SIZE:]
        self.assertEqual(saveimage.validate_image(mixed), [])
        self.refused("money 1", "two copies of the main save differ", mixed)

    def test_a_block_of_another_size(self):
        other = organic_like(self.layout, sizes={self.layout["SAVE_BLOCK_PLAYER"]: 0xac}).image()
        self.assertEqual(saveimage.validate_image(other), [])
        self.refused("money 1", "block 0x1c has 0xac bytes", other)

    def test_a_base_other_than_the_recipes(self):
        self.refused("base floccesy_save", "not one SAVES pins for floccesy_save")
        self.refused("base no_such_save", "is not one of oracle.py's SAVES")

    def test_a_state_the_arrival_would_keep(self):
        surfing = organic_like(self.layout, ex_state=2).image()
        self.refused("place ZONE_FLOCCESY_RANCH_2 warp 0", "FLD_PLAYER_EXSTATE_SURF", surfing)
        # Control: the verbs that don't place take it
        self.assertNotEqual(self.mint("money 1", surfing), surfing)


class PlaceTests(LabTest):
    def spawn(self, data: bytes) -> tuple:
        return self.field(data, "<IhhhHi3i", "PLAYER", self.layout["PlayerSave.nextSpawn"])

    def location(self, data: bytes) -> tuple:
        location = self.layout["PlayerSave.location"]
        zone = self.field(data, "<H", "PLAYER", location + self.layout["SaveLocation.zoneId"])
        pos = self.field(data, "<3i", "PLAYER", location + self.layout["SaveLocation.pos"])
        rest = self.field(data, "<HBB", "PLAYER", location + self.layout["SaveLocation.unk18"])
        return zone + pos + rest

    def continue_state(self, data: bytes) -> tuple[bool, int]:
        flags = self.layout["EventWork.flags"]
        byte = self.field(data, "<B", "EVENT_WORK", flags + EVENT_FLAG_CONTINUE_SCRIPT // 8)[0]
        work = self.field(data, "<H", "EVENT_WORK", 2 * (EVENT_WORK_CONTINUE_SCRIPT - 0x4000))[0]
        return bool(byte & 1 << EVENT_FLAG_CONTINUE_SCRIPT % 8), work

    def test_warp(self):
        minted = self.mint("place ZONE_FLOCCESY_RANCH_2 warp 0")
        # SetupZoneSpawnInfoWarp (src/system/zone_spawn.c): changeType WARP, the zone and warp, nothing else
        self.assertEqual(self.spawn(minted), (0, ZONE_FLOCCESY_RANCH_2, 0, 0, 0, 0, 0, 0, 0))
        # res/field/events/floccesy_ranch_2.json's warp 0: grid (728, 32, 872), unk4 1, WARP_DIR_UP. GetGridWarpOutPos
        # with posWeightBits 0 makes it fx32 (<< 12), and ConvWarpDirToAngle(WARP_DIR_UP) is 0; isRail 0, walking
        self.assertEqual(self.location(minted), (ZONE_FLOCCESY_RANCH_2, 728 << 12, 32 << 12, 872 << 12, 0, 0, 0))
        self.assertEqual(self.continue_state(minted), (True, 0))
        self.assertEqual(self.continue_state(self.base), (False, 0))

    def test_at(self):
        minted = self.mint("place ZONE_ROUTE_20 at 155 651 2 facing WARP_DIR_LEFT")
        # CreateZoneChangeData as FieldSetNextZone calls it (src/ov012/scrcmd_game_state.c): tiles << 16, warpId -1,
        # the facing as warpDir (WARP_DIR_LEFT, 3); ConvWarpDirToAngle(WARP_DIR_LEFT) is 0x4000
        position = (155 << 16, 2 << 16, 651 << 16)
        self.assertEqual(self.spawn(minted), (1, ZONE_ROUTE_20, -1, 3, 0, 0) + position)
        self.assertEqual(self.location(minted), (ZONE_ROUTE_20,) + position + (0x4000, 0, 0))
        self.assertEqual(self.continue_state(minted), (True, 0))
        # Without a height and a facing: height 0, facing WARP_DIR_DOWN (2), whose angle is 0x8000
        minted = self.mint("place ZONE_ROUTE_20 at 155 651")
        self.assertEqual(self.spawn(minted), (1, ZONE_ROUTE_20, -1, 2, 0, 0, 155 << 16, 0, 651 << 16))

    def test_fly(self):
        minted = self.mint("place ZONE_FLOCCESY_RANCH fly")
        # LoadZoneSpawnInfoCheckRail: res/field/zones/floccesy_ranch.json's fly position, each << 16
        # (LoadZoneFlyPosToVecFx32), facing WARP_DIR_DOWN
        fly = json.loads((REPO_ROOT / "res/field/zones/floccesy_ranch.json").read_text(encoding="utf-8"))["fly"]
        position = (fly["x"] << 16, fly["y"] << 16, fly["z"] << 16)
        self.assertEqual(self.spawn(minted), (1, ZONE_FLOCCESY_RANCH, -1, 2, 0, 0) + position)
        self.assertEqual(self.location(minted), (ZONE_FLOCCESY_RANCH,) + position + (0x8000, 0, 0))

    def test_rail_zones_are_placed_by_warp(self):
        for recipe in ("place ZONE_CASTELIA_CITY at 1 1", "place ZONE_CASTELIA_CITY fly"):
            with self.subTest(recipe=recipe):
                self.refused(recipe, "on a rail")
        castelia = json.loads((REPO_ROOT / "res/field/events/castelia_city.json").read_text(encoding="utf-8"))
        rail_warp = next(index for index, warp in enumerate(castelia["warps"]) if "rail" in warp)
        minted = self.mint(f"place ZONE_CASTELIA_CITY warp {rail_warp}")
        castelia_id = self.constants.value("ZONE_CASTELIA_CITY", "zones", "test")
        self.assertEqual(self.spawn(minted), (0, castelia_id, rail_warp, 0, 0, 0, 0, 0, 0))
        self.assertEqual(self.location(minted)[:4], (castelia_id, 0, 0, 0))

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
            ("place ZONE_ROUTE_20 at 1 2\nplace ZONE_ROUTE_20 at 1 3", "line 1 already sets that"),
        ):
            with self.subTest(recipe=recipe):
                self.refused(recipe, text)


class VerbTests(LabTest):
    def test_each_verb_writes_what_the_game_reads(self):
        minted = self.mint("flag EVENT_FLAG_TRAINER_YOUNGSTER_PETEY set\nflag 0x961 clear\nwork EVENT_WORK_0x40a3 3\n"
                           "work 0x4100 SPECIES_TEPIG\nmoney 9999999\nbadges 3\nplaytime 999:59:59\n")
        flags = self.layout["EventWork.flags"]
        # Flag N is bit N % 8 of flags[N / 8] (EventWork_GetFlagBytePtr, src/system/event_work.c)
        petey = EVENT_FLAG_TRAINER_YOUNGSTER_PETEY
        self.assertTrue(self.field(minted, "<B", "EVENT_WORK", flags + petey // 8)[0] & 1 << petey % 8)
        self.assertEqual(self.field(minted, "<B", "EVENT_WORK", flags + 0x961 // 8)[0], 0b100)
        # Work W is works[W - 0x4000] (EventWork_GetWkPtr)
        self.assertEqual(self.field(minted, "<H", "EVENT_WORK", 2 * (0x40a3 - 0x4000))[0], 3)
        self.assertEqual(self.field(minted, "<H", "EVENT_WORK", 2 * (0x4100 - 0x4000))[0], SPECIES_TEPIG)
        self.assertEqual(self.field(minted, "<II", "TRAINER_GAME_INFO", self.layout["TrainerGameInfoSave.cash"]),
                         (9999999, 0b111))
        play_time = self.layout["TrainerDataSave.playTime"]
        self.assertEqual(self.field(minted, "<HBB", "TRAINER_DATA", play_time + self.layout["PlayTime.hours"]),
                         (999, 59, 59))
        facts = savelab.read(minted, self.constants, flags=["EVENT_FLAG_TRAINER_YOUNGSTER_PETEY"])
        self.assertTrue(facts["flags"]["EVENT_FLAG_TRAINER_YOUNGSTER_PETEY"])
        self.assertEqual((facts["money"], facts["badges"], facts["playtime"]),
                         (9999999, {"count": 3, "bits": 0b111}, "999:59:59"))
        self.assertEqual(facts["works"]["EVENT_WORK_0x40a3"], 3)

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
