#!/usr/bin/env python3
"""ROM-free tests of the save lab's stations (port/tools/lab/stations.py) and of oracle.py's station and station-check
commands, on synthetic saves built as test_savelab.py builds them, field by field from the decomp's headers' layout
(savelab_layout.c, compiled as the host build compiles the game's files): a Route 20 save as the oracle's floccesy_save
is shaped, each version's its own, pinned in a patched SAVES; the save the floccesy_ranch recipe mints from it, built
again here from the game's data (res/field/events/floccesy_ranch_2.json's warp 0) rather than taken from the minter;
and resulting saves made from that through savelab.SaveImage as the game's SAVE leaves one after CONTINUE at the gate.
A patched copy of the real station, whose pins are those of the synthetic saves, runs the real recipe, the real input
files and the real spec:

  - SpecTests: every station's files and spec, both versions: its recipe and base, its input files (the version, the
    clock pinned, the start-up seed, floccesy_continue's presses to CONTINUE), its pins, its frames and declarations as
    oracle.py's sequences' are held (test_compare.py), every fact path read from a synthetic save, and no declared save
    byte inside the facts it asserts or the mystery gift block's contents;
  - MintTests: levels 1 and 2, a good mint and the negative controls: a recipe changed without its pin, a pin missing, a
    base elsewhere or of the other version, and a minter that breaks the structure, places elsewhere, leaves the
    continue flag clear or touches the party, its output pinned so that only the read-back can catch it;
  - ResultTests: level 5 on one program's resulting save, a good one and each way a save can be wrong;
  - CompareTests: level 5 between the programs, the facts compared whatever the declarations hide and the mystery gift
    blocks compared decrypted;
  - CommandTests: the commands' skips and refusals, oracle.py run as a program, station-check end to end with a
    stand-in Frigate, and the real command_station with stand-ins for both programs.

The synthetic saves' blocks are packed (test_savelab.Synthetic: block 0 runs from 0x0 to 0x20dfc, the trainer data block
0x1b starts at 0x22d00), so the game's offsets that oracle.py's FLOCCESY_SAVE_BYTES names fall in block 0's filler:
these tests declare their own bytes from the synthetic layout, with the same causes, and every synthetic save that
compare_results reads gets a valid mystery gift block at the game's offset, 0x1c800, inside block 0.

MUTANTS lists deliberate bugs in stations.py, each with the tests that must catch it, and MutationTests applies each to
stations.py's source, loads the result in stations' place for SpecTests, MintTests, ResultTests and CompareTests and
runs the mutant's tests, which must fail, and the same tests on stations.py as it is, which must pass. CommandTests
reach the lab through oracle.load_stations() and aren't mutant tests. The expected values are worked out from the
game's C and data and from the synthetic saves, never from stations.py. Nothing here is a game save or comes from the
ROM. Game functionality: the start menu's saved game, the field's continue at a spawn, the field menu's SAVE and the
save's blocks.

    test_stations.py --layout PROGRAM [unittest options]

PROGRAM is the build's port_test_savelab_layout; the ctest test lab_stations (port/cmake/lab.cmake) passes it. The
generated constants headers (build/include/generated/constants) must exist, as after any configure.
"""

from __future__ import annotations

import contextlib
import dataclasses
import functools
import hashlib
import io
import os
from pathlib import Path
import stat
import struct
import subprocess
import sys
import tempfile
import types
import unittest
from unittest import mock

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(REPO_ROOT / "port" / "tools" / "lab"))
sys.path.insert(0, str(REPO_ROOT / "port" / "tools" / "oracle"))

import compare  # noqa: E402
import oracle  # noqa: E402
import saveimage  # noqa: E402
import savelab  # noqa: E402
import stations  # noqa: E402
import test_savelab  # noqa: E402
from test_savelab import Synthetic, grid, headers_layout, layouts_remembered, pokemon, route20_save, story  # noqa: E402

ORACLE_PY = REPO_ROOT / "port" / "tools" / "oracle" / "oracle.py"
NAME = "floccesy_ranch"
VERSIONS = ("b2_us", "w2_us")
# The constants the tests name, from build/include/generated/constants (zones.h, flags.h, vars.h, species.h, moves.h)
ZONE_FLOCCESY_RANCH = 444
ZONE_FLOCCESY_RANCH_2 = 445
ZONE_ROUTE_20 = 446
EVENT_FLAG_0x0962 = 0x962
EVENT_FLAG_0x0964 = 0x964
EVENT_FLAG_CONTINUE_SCRIPT = 0x965
EVENT_WORK_CONTINUE_SCRIPT = 0x4041
EVENT_WORK_0x40a3 = 0x40a3
SPECIES_PURRLOIN = 509
MOVE_SCRATCH = 10
MOVE_GROWL = 45
# include/field/zone.h: a warp spawn's change type and the warp direction up
CHANGE_WARP = 0
WARP_DIR_UP = 1
# res/field/events/floccesy_ranch_2.json's warp 0, the gate from ZONE_FLOCCESY_RANCH: its grid position in world units
# (16 to a tile) and its facing, unk4 1, up. GetGridWarpOutPos with posWeightBits 0 makes the position fx32 (<< 12), and
# ConvWarpDirToAngle(WARP_DIR_UP) is 0 (src/ov035/event_mapchange.c)
GATE = (728, 32, 872)
GATE_TILE = (728 // 16, 872 // 16)
# The start-up seeds the DS builds with the clock pinned (port/tools/oracle/README.md, "Random seeds")
START_SEEDS = {"b2_us": 0x5a0af77416c50a14, "w2_us": 0x270fd5258c56ebe3}
# The mystery gift block in the game's layout (include/save/mystery_gift.h, MysteryGiftSaveData, 0xa94 bytes: the
# contents, then a u16 checksum and the u16 key), at 0x1c800 of each copy (port/tools/oracle/README.md, "Saves"), and
# mgEncryptData's generator (src/system/mystery_data.c, with the decryption's in src/gfl/gf_standard.c)
MYSTERY_GIFT = 0x1c800
MYSTERY_GIFT_SIZE = 0xa94
GIFT_MULTIPLIER = 1103515245
GIFT_INCREMENT = 24691
# Each base's key: the VBlank count's (OS_GetVBlankCount() << 8) | OS_GetVBlankCount() when New Game encrypted it
GIFT_KEYS = {"b2_us": 0x5959, "w2_us": 0x5b5a}
# The play time's seconds in the two programs' resulting saves, which each counts with its own clock's ticks
SECONDS = {"melonDS": 42, "Frigate": 40}
# The real game's layout of the blocks a station asserts (port/tools/oracle/README.md "Saves", saveimage.find_blocks on
# an oracle save): the party (0x1a), the player (0x1c), the event work (0x2d, EventWork's 0x4de bytes) and the money and
# badges (0x34's first 8 bytes), each in the first copy
ASSERTED = ((0x18e00, 0x19333), (0x19500, 0x195a7), (0x1ff00, 0x203dd), (0x21100, 0x21107))


def sha1(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()


def layout() -> dict[str, int]:
    return headers_layout()


def gift_block(contents: bytes, key: int) -> bytes:
    """A mystery gift block as mgEncryptData leaves it: the contents XORed u16 by u16 with the generator seeded by the
    checksum and the key, then the checksum (the contents' bytes added up) and the key"""
    checksum = sum(contents) & 0xffff
    out = bytearray(contents)
    seed = checksum + (key << 16)
    for offset in range(0, len(out), 2):
        seed = (seed * GIFT_MULTIPLIER + GIFT_INCREMENT) & 0xffffffff
        struct.pack_into("<H", out, offset, struct.unpack_from("<H", out, offset)[0] ^ (seed >> 16))
    return bytes(out) + struct.pack("<HH", checksum, key)


GIFT_CONTENTS = bytes((i * 7) & 0xff for i in range(MYSTERY_GIFT_SIZE - 4))


def gifted(save: Synthetic, key: int) -> None:
    """A valid mystery gift block at the game's offset, which in the synthetic layout is block 0's filler"""
    start, size = save.blocks[0]
    assert start <= MYSTERY_GIFT and MYSTERY_GIFT + MYSTERY_GIFT_SIZE <= start + size
    save.data[MYSTERY_GIFT:MYSTERY_GIFT + MYSTERY_GIFT_SIZE] = gift_block(GIFT_CONTENTS, key)


def route20_base(version: str, flags: tuple[int, ...] = ()) -> Synthetic:
    """The version's synthetic Route 20 save (test_savelab.route20_save: Tepig and Purrloin, works 0x40a3 and 0x40a5 at
    2, flags 0x961 and 0x962, 3,500 money), its gift block keyed for the version, and flags set besides"""
    save = route20_save(layout())
    for number in flags:
        save.flag(number, True)
    gifted(save, GIFT_KEYS[version])
    return save


def elsewhere_base(version: str) -> bytes:
    """A save of the same story (test_savelab.story) saved a tile east of the Route 20 save's, (131, 661)"""
    save = Synthetic(layout())
    story(save)
    save.location(ZONE_ROUTE_20, 0xc000, False, 0, grid((131 * 16 + 8) << 12, 0x2001f, (661 * 16 + 8) << 12))
    gifted(save, GIFT_KEYS[version])
    return save.image()


def ranch_minted(version: str, flags: tuple[int, ...] = ()) -> bytes:
    """What the recipe base floccesy_save + place ZONE_FLOCCESY_RANCH_2 warp 0 makes of the synthetic base, built again
    from the layout as the game's own writes would: nextSpawn as SetupZoneSpawnInfoWarp makes it (InitZoneSpawnInfo's
    zeros with the zone and warp 0, src/system/zone_spawn.c), SaveLocation the warp's arrival (its grid position as
    fx32, facing up, rotation 0, not on a rail), EVENT_FLAG_CONTINUE_SCRIPT set and EVENT_WORK_CONTINUE_SCRIPT 0"""
    save = route20_base(version, flags)
    save.spawn("nextSpawn", CHANGE_WARP, ZONE_FLOCCESY_RANCH_2, 0, 0, 0, False, bytes(12))
    save.location(ZONE_FLOCCESY_RANCH_2, 0, False, 0, grid(GATE[0] << 12, GATE[1] << 12, GATE[2] << 12))
    save.flag(EVENT_FLAG_CONTINUE_SCRIPT, True)
    save.work(EVENT_WORK_CONTINUE_SCRIPT, 0)
    return save.image()


@functools.cache
def bases() -> dict[str, bytes]:
    return {version: route20_base(version).image() for version in VERSIONS}


@functools.cache
def minted_saves() -> dict[str, bytes]:
    return {version: ranch_minted(version) for version in VERSIONS}


def write(fmt: str, block: str, offset: int, *values):
    """A change to a save through savelab.SaveImage: a field of a block at the headers' offset, in both copies"""
    return lambda save: save.write(fmt, layout()[f"SAVE_BLOCK_{block}"], offset, *values)


def flag(number: int, value: bool):
    """A change: flag N, bit N % 8 of EventWork.flags[N / 8] (EventWork_GetFlagBytePtr, src/system/event_work.c)"""
    def change(save: savelab.SaveImage) -> None:
        block, offset = layout()["SAVE_BLOCK_EVENT_WORK"], layout()["EventWork.flags"] + number // 8
        (byte,) = save.read("<B", block, offset)
        save.write("<B", block, offset, byte | 1 << number % 8 if value else byte & ~(1 << number % 8))
    return change


def work(number: int, value: int):
    """A change: work W, EventWork.works[W - 0x4000] (EventWork_GetWkPtr)"""
    return write("<H", "EVENT_WORK", layout()["EventWork.works"] + 2 * (number - 0x4000), value)


def spawn(field: str, change_type: int, zone: int, warp: int, warp_dir: int, position: bytes):
    """A change: one of PlayerSave's ZoneSpawnInfos, field by field (posWeightBits 0, not on a rail)"""
    def change(save: savelab.SaveImage) -> None:
        at = layout()[f"PlayerSave.{field}"]
        for name, fmt, value in (("changeType", "<I", change_type), ("zoneId", "<h", zone), ("warpId", "<h", warp),
                                 ("warpDir", "<h", warp_dir), ("posWeightBits", "<H", 0), ("isRail", "<i", 0),
                                 ("pos", "<12s", position)):
            write(fmt, "PLAYER", at + layout()[f"ZoneSpawnInfo.{name}"], value)(save)
    return change


def location(zone: int, tile: tuple[int, int], height: int):
    """A change: SaveLocation where the game stood the player, on the tile's centre at the height, facing up"""
    def change(save: savelab.SaveImage) -> None:
        at = layout()["PlayerSave.location"]
        position = grid((tile[0] * 16 + 8) << 12, height << 16, (tile[1] * 16 + 8) << 12)
        write("<H", "PLAYER", at + layout()["SaveLocation.zoneId"], zone)(save)
        write("<12s", "PLAYER", at + layout()["SaveLocation.pos"], position)(save)
        write("<H", "PLAYER", at + layout()["SaveLocation.unk18"], 0)(save)
    return change


def seconds(value: int):
    """A change: the play time's seconds (TrainerDataSave.playTime's PlayTime.seconds)"""
    return write("<B", "TRAINER_DATA", layout()["TrainerDataSave.playTime"] + layout()["PlayTime.seconds"], value)


def purrloin(exp: int):
    """A change: the party's second Pokémon, Purrloin, with another experience, stored as the game stores it"""
    member = pokemon(layout(), 0x8765_4321, SPECIES_PURRLOIN, 0, exp, (MOVE_SCRATCH, MOVE_GROWL, 0, 0), 4, (17, 17))
    return write(f"<{len(member)}s", "PARTY", layout()["PokeParty.pkms"] + layout()["sizeof(PartyPkm)"], member)


def gift_key(key: int):
    """A change: the mystery gift block encrypted again under another key, its contents the same, as a read by the
    game's mgDecryptData and mgEncryptData leaves it (in the synthetic layout, inside block 0)"""
    return lambda save: save.write(f"<{MYSTERY_GIFT_SIZE}s", 0, MYSTERY_GIFT, gift_block(GIFT_CONTENTS, key))


def flip(block: int, offset: int):
    """A change: one bit of a block's byte flipped, its CRCs then made right again by the seal"""
    return lambda save: save.write("<B", block, offset, save.read("<B", block, offset)[0] ^ 1)


def edited(data: bytes, *changes) -> bytes:
    """data through savelab.SaveImage with the changes made in both copies and every block written resealed"""
    save = savelab.SaveImage(data, "a synthetic save")
    for change in changes:
        change(save)
    return save.seal()


def arrived(who: str = "melonDS") -> tuple:
    """What the game's SAVE writes after CONTINUE at the gate (ADR 0032, README.md "Stations"): nowSpawn the arrival
    through warp 0 (a warp spawn of the ranch, facing up, at the gate's position), SaveLocation where the game stood the
    player (the gate's tile, on the ground at height 2, facing up), the continue flag clear and the play time on"""
    return (spawn("nowSpawn", CHANGE_WARP, ZONE_FLOCCESY_RANCH_2, 0, WARP_DIR_UP,
                  grid(GATE[0] << 12, GATE[1] << 12, GATE[2] << 12)),
            location(ZONE_FLOCCESY_RANCH_2, GATE_TILE, 2), flag(EVENT_FLAG_CONTINUE_SCRIPT, False),
            seconds(SECONDS[who]))


def resulting(version: str = "b2_us", who: str = "melonDS", *changes, minted: bytes | None = None) -> bytes:
    """A program's resulting save, from the version's synthetic minted save (or minted), with changes after the
    game's"""
    return edited(minted_saves()[version] if minted is None else minted, *arrived(who), *changes)


def synthetic_declarations(version: str) -> dict[str, str]:
    """Where two synthetic resulting saves may differ, from Synthetic's blocks, with oracle.py's causes: in Black 2, the
    trainer block's play-time seconds (block 0x1b +0x27), its footer CRC, its CRC table entry and the table's CRC (as
    FLOCCESY_SAVE_BYTES in the game's layout); in White 2, the mystery gift block's key, block 0's footer CRC and table
    entry over it, and the table's CRC. The two differ, so that a version's comparison must take its own"""
    blocks = Synthetic(layout()).blocks
    if version == "b2_us":
        start, size = blocks[layout()["SAVE_BLOCK_TRAINER_DATA"]]
        seconds_at = start + layout()["TrainerDataSave.playTime"] + layout()["PlayTime.seconds"]
        entry = saveimage.CRC_TABLE + 2 * layout()["SAVE_BLOCK_TRAINER_DATA"]
        return {oracle.both_copies(f"{seconds_at:#x}"): oracle.PLAY_TIME,
                oracle.both_copies(f"{start + size + 2:#x}-{start + size + 3:#x},{entry:#x}-{entry + 1:#x},"
                                   "0x25fa2-0x25fa3"): oracle.SAVE_CRCS}
    start, size = blocks[0]
    key = MYSTERY_GIFT + MYSTERY_GIFT_SIZE - 2
    return {oracle.both_copies(f"{key:#x}-{key + 1:#x}"): oracle.MYSTERY_KEY,
            oracle.both_copies(f"{start + size + 2:#x}-{start + size + 3:#x},{saveimage.CRC_TABLE:#x}-"
                               f"{saveimage.CRC_TABLE + 1:#x},0x25fa2-0x25fa3"): oracle.CENTER_CRCS}


@contextlib.contextmanager
def synthetic_station(minted: dict[str, bytes] | None = None, base: dict[str, bytes] | None = None, **changes):
    """The real floccesy_ranch station with the synthetic saves' pins: SAVES' floccesy_save pinned to the synthetic
    bases (or base), STATIONS' minted to the synthetic mints (or minted), and the station's other fields changed as
    given; it is the stations module of the moment's (a mutant's, in MutationTests)"""
    real = stations.STATIONS[NAME]
    pins = {version: sha1(data) for version, data in (minted or minted_saves()).items()}
    station = dataclasses.replace(real, minted=pins, **changes)
    saves = {real.base: oracle.SaveMaker(1, {version: sha1(data) for version, data in (base or bases()).items()})}
    with mock.patch.dict(stations.STATIONS, {NAME: station}), mock.patch.dict(oracle.SAVES, saves):
        yield station


class StationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.constants = savelab.Constants()

    def assertFailure(self, failures: list[str], *texts: str) -> None:
        """One failure holds every text"""
        self.assertTrue(any(all(text in failure for text in texts) for failure in failures),
                        f"no failure says {' and '.join(texts)}: {failures}")


class SpecTests(StationTest):
    """Every station of STATIONS, both versions: its files, its pins and its frames as oracle.py's sequences are held"""

    def input_lines(self, name: str, version: str) -> list[str]:
        return oracle.input_lines(stations.input_path(name, version))

    def frame_lines(self, name: str, version: str) -> list[list[str]]:
        return [line.split() for line in self.input_lines(name, version) if line.split()[0].isdigit()]

    def test_recipes(self):
        for name, station in stations.STATIONS.items():
            with self.subTest(station=name):
                with open(stations.recipe_path(name), encoding="utf-8") as f:
                    lines = f.read().splitlines()
                self.assertTrue(lines and lines[0].startswith("#"), "the recipe opens with what it makes")
                verbs = [line.split("#", 1)[0].split() for line in lines]
                self.assertEqual([words[1:] for words in verbs if words and words[0] == "base"], [[station.base]])
                self.assertIn(station.base, oracle.SAVES)
                for version in VERSIONS:
                    self.assertRegex(oracle.SAVES[station.base].sha1.get(version, ""), "^[0-9a-f]{40}$")

    def test_input_files(self):
        for name, station in stations.STATIONS.items():
            for version in VERSIONS:
                with self.subTest(station=name, version=version):
                    path = stations.input_path(name, version)
                    with open(path, encoding="utf-8") as f:
                        raw = f.read().splitlines()
                    self.assertEqual(raw[0], "pbwinput 1")
                    self.assertTrue(raw[1].startswith("#"), "the input file says what it plays")
                    lines = self.input_lines(name, version)
                    self.assertIn(f"version {version}", lines)
                    self.assertIn("clock pinned", lines)
                    self.assertEqual(oracle.input_seeds(path), [START_SEEDS[version]])
                    frames = self.frame_lines(name, version)
                    continued = [line.split() for line in oracle.input_lines(os.path.join(
                        oracle.REPO_ROOT, oracle.SEQUENCE_INPUT.format(name="floccesy_continue", version=version)))
                        if line.split()[0].isdigit()]
                    self.assertEqual(frames[:8], continued[:8], "the title's presses and CONTINUE")
                    self.assertLessEqual(max(int(words[0]) for words in frames), station.sequence.port_frames)

    def test_pins_and_frames(self):
        for name, station in stations.STATIONS.items():
            with self.subTest(station=name):
                for version in VERSIONS:
                    self.assertRegex(station.minted.get(version, ""), "^[0-9a-f]{40}$", version)
                    presses = [int(words[0]) for words in self.frame_lines(name, version)
                               if "X" in words[1].split("+")]
                    self.assertTrue(presses, "a committed station opens the field menu to SAVE")
                    self.assertLess(station.compare_last, min(presses), version)
                self.assertEqual((station.sequence.save, station.sequence.input), ("", ""))
                self.assertLessEqual(station.compare_last, station.sequence.port_frames)
                self.assertTrue(station.sequence.state_frames)
                self.assertLessEqual(max(compare.parse_frames(station.sequence.state_frames)), station.compare_last)

    def test_sequences_declare_what_they_leave_out(self):
        # test_compare.py's invariants for SEQUENCES, held for each station's sequence with its own input files
        for name, station in stations.STATIONS.items():
            sequence = station.sequence
            with self.subTest(station=name):
                ports = set(range(1, sequence.port_frames + 1))
                self.assertTrue(set(compare.parse_frames(sequence.clock)) <= ports)
                for version, lags in sequence.lags.items():
                    self.assertIn(version, VERSIONS)
                    self.assertTrue(set(compare.parse_lags(lags)) <= ports)
                for version, table in sequence.known.items():
                    self.assertIn(version, VERSIONS)
                    self.assertTrue(set(oracle.by_frame(table)) <= ports)
                for version, table in sequence.known_top.items():
                    top = set(oracle.by_frame(table))
                    self.assertTrue(top <= ports)
                    self.assertFalse(top & set(oracle.by_frame(sequence.known.get(version, {}))))
                for version, table in sequence.known_oracle.items():
                    self.assertTrue(set(oracle.by_frame(table)) <= set(range(1, sequence.oracle_frames + 1)))
                for version, table in sequence.close.items():
                    held = set(oracle.by_frame(table))
                    self.assertTrue(held <= ports)
                    self.assertFalse(held & set(oracle.by_frame(sequence.known.get(version, {}))))
                state = set(compare.parse_frames(sequence.state_frames))
                self.assertTrue(state <= ports)
                self.assertTrue(set(compare.parse_frames(sequence.state_oracle)) <=
                                set(range(1, sequence.oracle_frames + 1)))
                for table in list(sequence.known_state.values()) + list(sequence.known_random.values()):
                    self.assertTrue(set(oracle.by_frame(table)) <= state)
                self.assertEqual((sequence.stop_from, sequence.stop, sequence.stop_trap), (0, "", ""))
                labels = [dump.label for dump in sequence.state]
                self.assertEqual(len(labels), len(set(labels)))
                self.assertTrue(all(dump.oracle and dump.port and dump.size > 0 for dump in sequence.state))
                for version in VERSIONS:
                    self.assertTrue(os.path.exists(stations.input_path(name, version)), version)

    def test_declarations_name_their_causes(self):
        causes = {value for module in (oracle, stations) for key, value in vars(module).items()
                  if key.isupper() and isinstance(value, str) and len(value) > 40}
        for name, station in stations.STATIONS.items():
            sequence = station.sequence
            for kind in ("known", "known_top", "known_oracle", "close", "known_state", "known_random"):
                for version, table in getattr(sequence, kind).items():
                    for frames, why in table.items():
                        with self.subTest(station=name, kind=kind, version=version, frames=frames):
                            self.assertIn(why, causes, "a declared frame's cause is a named constant")
            for version, table in station.save_bytes.items():
                self.assertIn(version, VERSIONS)
                for spans, why in table.items():
                    self.assertTrue(oracle.parse_offsets(spans) and why, (name, spans))

    def test_fact_paths_are_read(self):
        # Every path of base_facts and expect names a fact that a station's read of a synthetic save gives
        for name, station in stations.STATIONS.items():
            facts = stations.read_facts(station, minted_saves()["b2_us"], self.constants, "a synthetic save")
            self.assertEqual(set(station.expect), set(VERSIONS), "a committed station SAVEs in both versions")
            paths = list(station.base_facts) + [path for table in station.expect.values() for path in table]
            for path in paths:
                with self.subTest(station=name, path=path):
                    stations.fact(facts, path)
            with self.assertRaises(KeyError):
                stations.fact(facts, "works.no_such_work")

    def test_declared_bytes_hide_no_asserted_fact(self):
        # The structural negative control: no station declares a byte of the facts it asserts, or of the mystery gift
        # block's contents, which the comparison decrypts, in either copy
        protected = {copy + offset for copy in saveimage.COPIES for first, last in ASSERTED
                     for offset in range(first, last + 1)}
        protected |= {copy + MYSTERY_GIFT + i for copy in saveimage.COPIES for i in range(MYSTERY_GIFT_SIZE - 4)}
        for name, station in stations.STATIONS.items():
            for version, table in station.save_bytes.items():
                for spans in table:
                    with self.subTest(station=name, version=version, spans=spans):
                        self.assertFalse(protected & set(oracle.parse_offsets(spans)))

    def test_the_synthetic_station_is_the_games(self):
        # The synthetic layout: the game's offsets of FLOCCESY_SAVE_BYTES fall in block 0, and so does the gift block
        start, size = Synthetic(layout()).blocks[0]
        self.assertEqual((start, start + size), (0, 0x20dfc))
        self.assertEqual(Synthetic(layout()).blocks[layout()["SAVE_BLOCK_TRAINER_DATA"]][0], 0x22d00)
        self.assertEqual((oracle.MYSTERY_GIFT_BLOCK, oracle.MYSTERY_GIFT_SIZE), (MYSTERY_GIFT, MYSTERY_GIFT_SIZE))
        warp = test_savelab.events("floccesy_ranch_2")["warps"][0]
        self.assertEqual((warp["grid"]["x"], warp["grid"]["y"], warp["grid"]["z"], warp["unk4"]), (*GATE, WARP_DIR_UP))
        for version in VERSIONS:
            plain, problems = oracle.mystery_gift_decrypted(bases()[version])
            self.assertEqual(problems, [])
            self.assertEqual(plain[MYSTERY_GIFT:MYSTERY_GIFT + len(GIFT_CONTENTS)], GIFT_CONTENTS)


class MintTests(StationTest):
    """Levels 1 and 2: the station's recipe minted from the version's base, and the negative controls"""

    def test_a_good_mint(self):
        with synthetic_station():
            for version in VERSIONS:
                with self.subTest(version=version):
                    minted, applied, failures = stations.mint(NAME, version, bases()[version], self.constants)
                    self.assertEqual(failures, [])
                    self.assertEqual(sha1(minted), sha1(minted_saves()[version]))
                    self.assertEqual({key: value for key, value in applied[0].items() if key != "line"},
                                     {"base": "floccesy_save", "version": version})
                    self.assertEqual(applied[1]["place"]["arrival"], {"tile": list(GATE_TILE), "height": 2})

    def test_a_recipe_changed_without_its_pin(self):
        # Not another warp: floccesy_ranch_2.json has one, and the minter refuses warp 1 itself
        with tempfile.TemporaryDirectory() as directory, synthetic_station():
            with open(stations.recipe_path(NAME), encoding="utf-8") as f:
                recipe = f.read()
            Path(directory, f"{NAME}.recipe").write_text(recipe + "money 3501\n", encoding="utf-8")
            with mock.patch.object(stations, "STATION_RECIPE", os.path.join(directory, "{name}.recipe")):
                minted, _, failures = stations.mint(NAME, "b2_us", bases()["b2_us"], self.constants)
        self.assertIsNotNone(minted)
        self.assertFailure(failures, f"the minted save is {sha1(minted)}",
                           f"STATIONS pins {sha1(minted_saves()['b2_us'])}")

    def test_a_missing_pin_reports_the_mint(self):
        with synthetic_station(minted={"w2_us": minted_saves()["w2_us"]}):
            _, _, failures = stations.mint(NAME, "b2_us", bases()["b2_us"], self.constants)
        self.assertEqual(failures, [f"STATIONS pins no minted save of {NAME} for b2_us; this mint is "
                                    f"{sha1(minted_saves()['b2_us'])}"])

    def test_a_base_elsewhere_fails_its_facts(self):
        elsewhere = elsewhere_base("b2_us")
        with synthetic_station(base={"b2_us": elsewhere, "w2_us": bases()["w2_us"]}):
            minted, _, failures = stations.mint(NAME, "b2_us", elsewhere, self.constants)
        self.assertIsNone(minted and sha1(minted))
        self.assertFailure(failures, "the base's location.tile is [131, 661], not [130, 661]")

    def test_a_base_of_the_other_version(self):
        # White 2's base given for Black 2: savelab's base line takes any version SAVES pins, so the station's own
        # check names the version and both SHA-1s
        with synthetic_station():
            minted, _, failures = stations.mint(NAME, "b2_us", bases()["w2_us"], self.constants)
        self.assertIsNone(minted and sha1(minted))
        self.assertFailure(failures, sha1(bases()["w2_us"]), sha1(bases()["b2_us"]), "b2_us")

    def minted_by(self, *changes) -> list[str]:
        """The failures of a mint whose minter's output has the changes, pinned, so that only the read-back sees them"""
        made = edited(minted_saves()["b2_us"], *changes)
        real_mint = savelab.mint

        def minter(*args, **kwargs):
            return made, real_mint(*args, **kwargs)[1]

        with synthetic_station(minted={"b2_us": made, "w2_us": minted_saves()["w2_us"]}), \
                mock.patch.object(savelab, "mint", minter):
            minted, _, failures = stations.mint(NAME, "b2_us", bases()["b2_us"], self.constants)
        self.assertEqual(sha1(minted), sha1(made))
        return failures

    def test_a_minter_that_places_in_another_zone(self):
        at = layout()["PlayerSave.location"] + layout()["SaveLocation.zoneId"]
        self.assertFailure(self.minted_by(write("<H", "PLAYER", at, ZONE_FLOCCESY_RANCH)),
                           "the minted save's location.zone is 'ZONE_FLOCCESY_RANCH', not 'ZONE_FLOCCESY_RANCH_2'")

    def test_a_minter_that_places_on_another_tile(self):
        at = layout()["PlayerSave.location"] + layout()["SaveLocation.pos"]
        moved = grid((GATE[0] + 16) << 12, GATE[1] << 12, GATE[2] << 12)
        self.assertFailure(self.minted_by(write("<12s", "PLAYER", at, moved)),
                           "the minted save's location.tile is [46, 54], not [45, 54]")

    def test_a_minter_that_leaves_the_continue_flag_clear(self):
        self.assertFailure(self.minted_by(flag(EVENT_FLAG_CONTINUE_SCRIPT, False)),
                           "flags.EVENT_FLAG_CONTINUE_SCRIPT is False, not True", "place sets the continue pair")

    def test_a_minter_that_touches_the_party(self):
        self.assertFailure(self.minted_by(purrloin(65)), "the minted save's party is", "where the base's is")

    def test_a_minter_that_breaks_the_structure(self):
        # A minter whose output has the second copy's player block changed after sealing: pinned, it must still fail
        # level 1's structure check, against the base's layout
        made = bytearray(minted_saves()["b2_us"])
        start, size = Synthetic(layout()).blocks[layout()["SAVE_BLOCK_PLAYER"]]
        made[saveimage.COPY_SIZE + start + size - 1] ^= 1
        made = bytes(made)
        real_mint = savelab.mint
        with synthetic_station(minted={"b2_us": made, "w2_us": minted_saves()["w2_us"]}), \
                mock.patch.object(savelab, "mint", lambda *args, **kwargs: (made, real_mint(*args, **kwargs)[1])):
            _, _, failures = stations.mint(NAME, "b2_us", bases()["b2_us"], self.constants)
        self.assertFailure(failures, "the minted save fails the checks of its base's layout",
                           f"copy {saveimage.COPY_SIZE:#x} block {layout()['SAVE_BLOCK_PLAYER']}")


class ResultTests(StationTest):
    """Level 5 on one program's resulting save"""

    def failures(self, result: bytes, version: str = "b2_us", minted: bytes | None = None, **changes) -> list[str]:
        with synthetic_station(**changes):
            return stations.result_failures(NAME, version, minted_saves()[version] if minted is None else minted,
                                            result, "melonDS", self.constants)

    def test_a_good_result(self):
        for version in VERSIONS:
            for who in SECONDS:
                with self.subTest(version=version, who=who):
                    self.assertEqual(self.failures(resulting(version, who), version), [])

    def test_an_unwritten_result(self):
        self.assertFailure(self.failures(minted_saves()["b2_us"]), "the game never wrote it")

    def test_a_result_elsewhere(self):
        at = layout()["PlayerSave.location"] + layout()["SaveLocation.zoneId"]
        self.assertFailure(self.failures(resulting("b2_us", "melonDS", write("<H", "PLAYER", at, ZONE_ROUTE_20))),
                           "location.zone is 'ZONE_ROUTE_20', not 'ZONE_FLOCCESY_RANCH_2'")

    def test_a_result_with_the_continue_flag_set(self):
        self.assertFailure(self.failures(resulting("b2_us", "melonDS", flag(EVENT_FLAG_CONTINUE_SCRIPT, True))),
                           "flags.EVENT_FLAG_CONTINUE_SCRIPT is True, not False", "CONTINUE clears the continue flag")

    def test_a_changed_party(self):
        self.assertFailure(self.failures(resulting("b2_us", "melonDS", purrloin(65))),
                           "melonDS's resulting save's party is", "where the minted save's is")

    def test_changed_money(self):
        cash = layout()["TrainerGameInfoSave.cash"]
        self.assertFailure(self.failures(resulting("b2_us", "melonDS", write("<I", "TRAINER_GAME_INFO", cash, 3501))),
                           "resulting save's money is 3501, where the minted save's is 3500")

    def test_a_changed_flag(self):
        self.assertFailure(self.failures(resulting("b2_us", "melonDS", flag(EVENT_FLAG_0x0962, False))),
                           "has EVENT_FLAG_0x0962 clear, where the minted save has it set")

    def test_a_changed_work_value(self):
        self.assertFailure(self.failures(resulting("b2_us", "melonDS", work(EVENT_WORK_0x40a3, 3))),
                           "resulting save's EVENT_WORK_0x40a3 is 3, where the minted save's is 2")

    def test_a_changed_next_spawn(self):
        self.assertFailure(self.failures(resulting("b2_us", "melonDS",
                                                   spawn("nextSpawn", CHANGE_WARP, 0, 0, 0, bytes(12)))),
                           "resulting save's next_spawn is", "the game never clears it")

    def test_a_broken_crc(self):
        broken = bytearray(resulting())
        start, size = Synthetic(layout()).blocks[layout()["SAVE_BLOCK_EVENT_WORK"]]
        broken[saveimage.COPY_SIZE + start + size + 2] ^= 1
        self.assertFailure(self.failures(bytes(broken)), "fails the checks of the minted save's layout",
                           f"copy {saveimage.COPY_SIZE:#x} block {layout()['SAVE_BLOCK_EVENT_WORK']}")

    def test_a_flag_the_spawn_path_clears(self):
        # ZoneChange_OnMapChange (src/ov012/zone_change.c) clears flag 0x964 on every change of map: a base with it set
        # gives a result with it clear, which passes where expect declares it and fails where it doesn't
        base = {version: route20_base(version, (EVENT_FLAG_0x0964,)).image() for version in VERSIONS}
        minted = {version: ranch_minted(version, (EVENT_FLAG_0x0964,)) for version in VERSIONS}
        result = resulting("b2_us", "melonDS", flag(EVENT_FLAG_0x0964, False), minted=minted["b2_us"])
        declared = {version: {**stations.RANCH_GATE, "flags.EVENT_FLAG_0x0964": False} for version in VERSIONS}
        with synthetic_station(minted=minted, base=base, expect=declared):
            self.assertEqual(stations.result_failures(NAME, "b2_us", minted["b2_us"], result, "melonDS",
                                                      self.constants), [])
        with synthetic_station(minted=minted, base=base):
            failures = stations.result_failures(NAME, "b2_us", minted["b2_us"], result, "melonDS", self.constants)
        self.assertFailure(failures, "has EVENT_FLAG_0x0964 clear, where the minted save has it set")

    def test_a_station_without_expect(self):
        # Its input doesn't SAVE: the resulting save is the minted save, and a written one fails
        without = {"w2_us": stations.RANCH_GATE}
        self.assertEqual(self.failures(minted_saves()["b2_us"], expect=without), [])
        self.assertFailure(self.failures(resulting(), expect=without), "doesn't SAVE, so the game only reads the save")


class CompareTests(StationTest):
    """Level 5 between the programs, on the synthetic declarations"""

    def failures(self, version: str, oracle_save: bytes, port_save: bytes,
                 declarations: dict[str, dict[str, str]] | None = None) -> list[str]:
        declared = declarations or {version: synthetic_declarations(version) for version in VERSIONS}
        with synthetic_station(save_bytes=declared):
            return stations.compare_results(NAME, version, oracle_save, port_save, self.constants)

    def test_results_that_differ_in_the_play_time(self):
        theirs, ours = resulting("b2_us", "melonDS"), resulting("b2_us", "Frigate")
        self.assertNotEqual(sha1(theirs), sha1(ours))
        self.assertEqual(self.failures("b2_us", theirs, ours), [])

    def test_a_declaration_hides_a_byte_never_a_fact(self):
        theirs, ours = resulting("b2_us", "melonDS"), resulting("b2_us", "Frigate", purrloin(65))
        failures = self.failures("b2_us", theirs, ours)
        self.assertFailure(failures, "differ where the station declares nothing")
        self.assertFailure(failures, "the resulting saves' party differ")
        # The party's bytes declared too: the bytes pass, the facts still fail
        declared = {version: synthetic_declarations(version) for version in VERSIONS}
        party = ",".join(f"{a:#x}-{b:#x}" for a, b in oracle.save_differences(theirs, ours))
        declared["b2_us"] = {**declared["b2_us"], party: "a declaration that hides the party"}
        failures = self.failures("b2_us", theirs, ours, declared)
        self.assertFalse([failure for failure in failures if "declares nothing" in failure])
        self.assertFailure(failures, "the resulting saves' party differ")

    def test_a_byte_outside_every_declaration(self):
        # A byte of a block the reader doesn't read (block 5's filler), its CRCs right
        start, _ = Synthetic(layout()).blocks[5]
        theirs = resulting("b2_us", "melonDS")
        ours = resulting("b2_us", "melonDS", flip(5, 0))
        failures = self.failures("b2_us", theirs, ours)
        self.assertFailure(failures, "differ where the station declares nothing", f"{start:#x}-{start:#x}")

    def test_gift_blocks_under_other_keys(self):
        # The same contents under the port's key: equal decrypted, with the key and the CRCs over it declared; compared
        # encrypted, the contents would differ
        theirs = resulting("w2_us", "melonDS")
        ours = resulting("w2_us", "melonDS", gift_key(0x7f7f))
        self.assertEqual(self.failures("w2_us", theirs, ours), [])
        declared = synthetic_declarations("w2_us")
        encrypted = oracle.undeclared_differences(ours, theirs, declared)
        self.assertTrue(any(first < MYSTERY_GIFT + MYSTERY_GIFT_SIZE - 4 and last >= MYSTERY_GIFT
                            for first, last in encrypted))

    def test_an_undecryptable_gift_block(self):
        theirs = resulting("w2_us", "melonDS")
        ours = resulting("w2_us", "melonDS", flip(0, MYSTERY_GIFT))
        self.assertFailure(self.failures("w2_us", theirs, ours), "Frigate's resulting save",
                           "doesn't add up to its checksum")


def stand_in(path: str, text: str) -> str:
    """Writes a stand-in program, a Python script that its #! line runs (test_compare.py's stand_in), and returns the
    path to run it by: on Windows, which runs no script by its #! line, a .cmd beside it that runs it with this
    Python"""
    with open(path, "w") as f:
        f.write(text)
    os.chmod(path, os.stat(path).st_mode | stat.S_IEXEC)
    if sys.platform != "win32":
        return path
    with open(path + ".cmd", "w") as f:
        f.write(f'@"{sys.executable}" "{path}" %*\n')
    return path + ".cmd"


# A stand-in for Frigate: it records the save it is given (its save directory's file, named after the ROM's game code)
# and writes a prepared resulting save over it, as a game that SAVEs
FAKE_FRIGATE = """#!{python}
import os
import shutil
import sys
args = sys.argv[1:]
saves = args[args.index("--save-dir") + 1]
names = os.listdir(saves)
shutil.copyfile(os.path.join(saves, names[0]), {record!r})
shutil.copyfile({prepared!r}, os.path.join(saves, names[0]))
"""


def captured(function, *args) -> tuple[int, str]:
    """Runs a command; returns its status and what it printed"""
    out = io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(out):
        status = function(*args)
    return status, out.getvalue()


class CommandTests(StationTest):
    """oracle.py's station and station-check, through oracle.load_stations()"""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.directory = tempfile.TemporaryDirectory()
        cls.root = Path(cls.directory.name)
        cls.rom = cls.root / "rom.nds"
        # A ROM header's game code at 0x0c, which run_port names the save after (IREO, Black 2's)
        cls.rom.write_bytes(b"POKEMON B2\0\0IREO01" + bytes(0x200))
        cls.oracle_program = cls.root / "pbw-oracle"
        cls.oracle_program.touch()

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def test_skips_without_the_oracle_or_the_rom(self):
        missing = str(self.root / "missing")
        for name in oracle.load_stations().STATIONS:
            with self.subTest(station=name):
                self.assertEqual(captured(oracle.main, ["station", name, "--port", missing, "--oracle",
                                                        missing])[0], oracle.SKIP)
                self.assertEqual(captured(oracle.main, ["station", name, "--port", missing, "--oracle",
                                                        str(self.oracle_program), "--rom", missing])[0], oracle.SKIP)
                self.assertEqual(captured(oracle.main, ["station-check", name, "--port", missing, "--rom",
                                                        missing])[0], oracle.SKIP)
                with mock.patch.object(oracle, "rom_matches", return_value=None):
                    status, out = captured(oracle.main, ["station-check", name, "--port", missing])
                    self.assertEqual(status, oracle.SKIP)
                    self.assertIn("no --save-cache", out)
                    empty = self.root / "empty-cache"
                    empty.mkdir(exist_ok=True)
                    status, out = captured(oracle.main, ["station-check", name, "--port", missing, "--save-cache",
                                                         str(empty)])
                    self.assertEqual(status, oracle.SKIP)
                    self.assertIn(f"oracle.py save {oracle.load_stations().STATIONS[name].base} --save-cache", out)

    def test_an_unknown_station(self):
        missing = str(self.root / "missing")
        for command in ("station", "station-check"):
            with self.subTest(command=command):
                status, out = captured(oracle.main, [command, "no_such_station", "--port", missing, "--rom", missing])
                self.assertEqual(status, 2)
                self.assertIn(NAME, out)

    def test_an_unknown_station_as_a_program(self):
        # oracle.py run as a program imports the lab under __main__
        done = subprocess.run([sys.executable, str(ORACLE_PY), "station", "no_such_station", "--port", "missing"],
                              capture_output=True, text=True, cwd=REPO_ROOT,
                              env={**os.environ, "PYTHONDONTWRITEBYTECODE": "1"})
        self.assertEqual(done.returncode, 2, done.stdout + done.stderr)
        self.assertIn(NAME, done.stdout)

    def test_station_check_end_to_end(self):
        # A stand-in Frigate writes a prepared resulting save: a good one holds, one with the party changed fails
        cache = self.root / "cache"
        cache.mkdir(exist_ok=True)
        (cache / "floccesy_save_b2_us.sav").write_bytes(bases()["b2_us"])
        for label, result, wanted in (("good", resulting(), 0), ("party", resulting("b2_us", "Frigate",
                                                                                     purrloin(65)), 1)):
            with self.subTest(result=label):
                prepared, record = self.root / f"{label}.sav", self.root / f"{label}-given.sav"
                prepared.write_bytes(result)
                program = stand_in(str(self.root / f"frigate-{label}"), FAKE_FRIGATE.format(
                    python=sys.executable, record=str(record), prepared=str(prepared)))
                with synthetic_station(), mock.patch.object(oracle, "rom_matches", return_value=None):
                    status, out = captured(oracle.main, ["station-check", NAME, "--port", program, "--rom",
                                                         str(self.rom), "--save-cache", str(cache), "--work",
                                                         str(self.root / f"work-{label}")])
                self.assertEqual(status, wanted, out)
                self.assertEqual(sha1(record.read_bytes()), sha1(minted_saves()["b2_us"]),
                                 "Frigate is given the minted save")

    def run_station(self, oracle_result: bytes | None, port_result: bytes | None, frames: int,
                    minted: dict[str, bytes] | None = None) -> tuple[int, str, list[tuple[str, str]]]:
        """The real command_station with both programs and the frames' comparison stood in for: each program records
        the save it is given and writes its prepared resulting save; returns the status, the output and the records"""
        given = []
        base = self.root / "base.sav"
        base.write_bytes(bases()["b2_us"])

        def melonds(args, out, *rest, **kwargs):
            given.append(("melonDS", sha1(Path(args.save).read_bytes())))
            os.makedirs(out, exist_ok=True)
            if oracle_result is not None:
                Path(args.save_out).write_bytes(oracle_result)
            return 0

        def frigate(binary, out, *rest, save=None, save_out=None, **kwargs):
            given.append(("Frigate", sha1(Path(save).read_bytes())))
            os.makedirs(out, exist_ok=True)
            if port_result is not None:
                Path(save_out).write_bytes(port_result)
            return 0, ""

        out_dir = tempfile.mkdtemp(dir=self.root)
        declared = {version: synthetic_declarations(version) for version in VERSIONS}
        with synthetic_station(minted=minted, save_bytes=declared), \
                mock.patch.object(oracle, "rom_matches", return_value=None), \
                mock.patch.object(oracle, "obtain_save", return_value=(str(base), None)), \
                mock.patch.object(oracle, "run_oracle", side_effect=melonds), \
                mock.patch.object(oracle, "run_port", side_effect=frigate), \
                mock.patch.object(oracle, "report_seeds", return_value=None), \
                mock.patch.object(oracle, "compare_sequence", return_value=frames):
            status, out = captured(oracle.main, ["station", NAME, "--port", "frigate", "--oracle",
                                                 str(self.oracle_program), "--rom", str(self.rom), "--out", out_dir])
        return status, out, given

    def test_command_station(self):
        good = (resulting("b2_us", "melonDS"), resulting("b2_us", "Frigate"))
        pin = sha1(minted_saves()["b2_us"])
        # Both programs are given the minted save, never the base, and good saves with equal frames hold
        status, out, given = self.run_station(*good, frames=0)
        self.assertEqual(status, 0, out)
        self.assertEqual(given, [("melonDS", pin), ("Frigate", pin)])
        self.assertNotIn(sha1(bases()["b2_us"]), [digest for _, digest in given])
        # A mint that fails its pin fails before either program runs
        status, out, given = self.run_station(*good, frames=0, minted={"b2_us": bases()["b2_us"],
                                                                       "w2_us": minted_saves()["w2_us"]})
        self.assertEqual((status, given), (1, []), out)
        self.assertIn("STATIONS pins", out)
        # Frames that differ fail it
        self.assertEqual(self.run_station(*good, frames=1)[0], 1)
        # The oracle's resulting save elsewhere fails it, whatever the frames gave
        at = layout()["PlayerSave.location"] + layout()["SaveLocation.zoneId"]
        status, out, _ = self.run_station(resulting("b2_us", "melonDS", write("<H", "PLAYER", at, ZONE_ROUTE_20)),
                                          good[1], frames=0)
        self.assertEqual(status, 1, out)
        self.assertIn("FAIL: melonDS's resulting save's location.zone", out)
        # A missing resulting save fails it
        self.assertEqual(self.run_station(good[0], None, frames=0)[0], 1)
        self.assertEqual(self.run_station(None, good[1], frames=0)[0], 1)


# Deliberate bugs in stations.py, each the text it replaces (found exactly once in stations.py), the text that replaces
# it, and the tests, ClassName or ClassName.test_name, at least one of which must fail on it
MUTANTS = (
    # Level 1
    ("the pin not checked", "    elif made != pin:\n", "    elif False:\n",
     ("MintTests.test_a_recipe_changed_without_its_pin",)),
    ("the other version's pin used", "    pin = station.minted.get(version)\n",
     '    pin = station.minted.get({"b2_us": "w2_us", "w2_us": "b2_us"}[version])\n',
     ("MintTests.test_a_good_mint",)),
    ("the base's version not checked", "    if digest != pinned:\n", "    if False:\n",
     ("MintTests.test_a_base_of_the_other_version",)),
    ("base_facts skipped", "    for path, wanted in station.base_facts.items():\n",
     "    for path, wanted in {}.items():\n", ("MintTests.test_a_base_elsewhere_fails_its_facts",)),
    ("the minted save's structure not checked",
     "    for problem in saveimage.validate_image(minted, reference=base):\n", "    for problem in []:\n",
     ("MintTests.test_a_minter_that_breaks_the_structure",)),
    # Level 2
    ("the place's zone not read back",
     '        wanted = {"location.zone": place["zone"], "location.facing": place["facing"]}\n',
     '        wanted = {"location.facing": place["facing"]}\n',
     ("MintTests.test_a_minter_that_places_in_another_zone",)),
    ("the place's tile not read back", '            wanted["location.tile"] = place["arrival"]["tile"]\n', "",
     ("MintTests.test_a_minter_that_places_on_another_tile",)),
    ("the continue flag not checked in the minted save",
     '        for path, value in ((f"flags.{CONTINUE_FLAG}", True), (f"works.{CONTINUE_WORK}", 0)):\n',
     '        for path, value in ((f"works.{CONTINUE_WORK}", 0),):\n',
     ("MintTests.test_a_minter_that_leaves_the_continue_flag_clear",)),
    ("the base's other facts not compared", "    failures += kept(base_facts, facts,",
     "    failures += [] and kept(base_facts, facts,", ("MintTests.test_a_minter_that_touches_the_party",)),
    # Level 5, one program's resulting save
    ("validate_image skipped on a result", "    problems = saveimage.validate_image(result, reference=minted)\n",
     "    problems = []\n", ("ResultTests.test_a_broken_crc",)),
    ("the game wrote it check dropped", "    if result == minted:\n", "    if False:\n",
     ("ResultTests.test_an_unwritten_result",)),
    ("expect skipped", "    for path, wanted in expected.items():\n", "    for path, wanted in {}.items():\n",
     ("ResultTests.test_a_result_elsewhere",)),
    ("the continue flag's clearing not checked",
     '    for path, wanted in ((f"flags.{CONTINUE_FLAG}", False), (f"works.{CONTINUE_WORK}", 0)):\n',
     '    for path, wanted in ((f"works.{CONTINUE_WORK}", 0),):\n',
     ("ResultTests.test_a_result_with_the_continue_flag_set",)),
    ("the party not held to the minted save's", 'RESULT_KEPT = ("party", "money", "badges")\n',
     'RESULT_KEPT = ("money", "badges")\n', ("ResultTests.test_a_changed_party",)),
    ("the flags not held to the minted save's", '    now = set(after["set_flags"]) - flags_left\n',
     '    now = set(before["set_flags"]) - flags_left\n', ("ResultTests.test_a_changed_flag",)),
    ("the work values not held to the minted save's",
     '        if before["works"].get(work, 0) != after["works"].get(work, 0):\n', "        if False:\n",
     ("ResultTests.test_a_changed_work_value",)),
    ("next_spawn not held to the minted save's", '    if after["next_spawn"] != before["next_spawn"]:\n',
     "    if False:\n", ("ResultTests.test_a_changed_next_spawn",)),
    ("a flag the spawn path clears not taken from expect",
     '    flags_left = named(list(expected), "flags")\n', "    flags_left = set()\n",
     ("ResultTests.test_a_flag_the_spawn_path_clears",)),
    ("a station without expect not held to its minted save", "        if result != minted:\n", "        if False:\n",
     ("ResultTests.test_a_station_without_expect",)),
    # Level 5, between the programs
    ("the facts not compared between the programs", "            if theirs.get(key) == ours.get(key):\n",
     "            if True:\n", ("CompareTests.test_a_declaration_hides_a_byte_never_a_fact",)),
    ("the other version's save_bytes used", "    declared = station.save_bytes.get(version, {})\n",
     '    declared = station.save_bytes.get({"b2_us": "w2_us", "w2_us": "b2_us"}[version], {})\n',
     ("CompareTests.test_results_that_differ_in_the_play_time", "CompareTests.test_gift_blocks_under_other_keys")),
    ("the mystery gift blocks compared encrypted",
     '    undeclared = oracle.undeclared_differences(plain["Frigate"], plain["melonDS"], declared)\n',
     "    undeclared = oracle.undeclared_differences(port_save, oracle_save, declared)\n",
     ("CompareTests.test_gift_blocks_under_other_keys",)),
    ("a gift block that doesn't decrypt taken",
     '        failures += [f"{who}\'s resulting save: {problem}" for problem in problems]\n', "",
     ("CompareTests.test_an_undecryptable_gift_block",)),
)


class MutationTests(unittest.TestCase):
    """Every row of MUTANTS: stations.py with that one bug, loaded in stations' place, fails at least one of its tests;
    and stations.py as it is passes all of them through the same runner, so that what fails is the bug"""

    @classmethod
    def setUpClass(cls):
        cls.source = Path(stations.__file__).read_text(encoding="utf-8")

    def load(self, source: str) -> types.ModuleType:
        """A module made from that source as stations.py's, its path as stations.py's own; registered in sys.modules
        while its source runs, since dataclasses looks a class's module up there for its annotations"""
        module = types.ModuleType("stations_mutant")
        module.__file__ = stations.__file__
        with mock.patch.object(sys, "path", list(sys.path)), mock.patch.dict(sys.modules, {module.__name__: module}):
            exec(compile(source, stations.__file__, "exec"), module.__dict__)
        return module

    def run_tests(self, module: types.ModuleType, names: tuple[str, ...], first_failure: bool) -> unittest.TestResult:
        """The tests named, with module as stations, each class a subclass of its own so that its class attributes
        aren't the suite's; with first_failure, they stop at the first test or subtest that fails"""
        suite = unittest.TestSuite()
        for name in names:
            class_name, _, method = name.partition(".")
            isolated = type(class_name, (globals()[class_name],), {"__module__": __name__})
            suite.addTests([isolated(method)] if method else unittest.defaultTestLoader.loadTestsFromTestCase(isolated))
        result = unittest.TestResult()
        result.failfast = first_failure
        with mock.patch.dict(globals(), {"stations": module}), layouts_remembered():
            suite.run(result)
        return result

    def test_mutant_tests_are_mutant_classes(self):
        # Only these classes reach stations through this module's global, which the runner patches
        for *_, tests in MUTANTS:
            for name in tests:
                self.assertIn(name.partition(".")[0], ("SpecTests", "MintTests", "ResultTests", "CompareTests"))

    def test_stations_as_it_is_passes_the_mutants_tests(self):
        names = tuple(sorted({name for *_, tests in MUTANTS for name in tests}))
        result = self.run_tests(self.load(self.source), names, first_failure=False)
        self.assertEqual(result.testsRun, len(names))
        self.assertTrue(result.wasSuccessful(), [str(test) for test, _ in result.failures + result.errors])

    def test_each_mutant_fails_its_tests(self):
        self.assertEqual(len({name for name, *_ in MUTANTS}), len(MUTANTS))
        for name, old, new, tests in MUTANTS:
            with self.subTest(mutant=name):
                self.assertEqual(self.source.count(old), 1, "the text a mutant replaces is in stations.py once")
                self.assertNotEqual(old, new)
                result = self.run_tests(self.load(self.source.replace(old, new)), tests, first_failure=True)
                self.assertFalse(result.wasSuccessful(), f"stations.py with this bug passes {', '.join(tests)}")


def main() -> None:
    argv = sys.argv[1:]
    if "--layout" not in argv or argv.index("--layout") + 1 >= len(argv):
        print("usage: test_stations.py --layout PROGRAM [unittest options] (PROGRAM: the build's "
              "port_test_savelab_layout)", file=sys.stderr)
        sys.exit(2)
    index = argv.index("--layout")
    test_savelab.LAYOUT_PROGRAM = os.path.abspath(argv[index + 1])
    del argv[index:index + 2]
    with layouts_remembered():
        unittest.main(argv=[sys.argv[0]] + argv)


if __name__ == "__main__":
    main()
