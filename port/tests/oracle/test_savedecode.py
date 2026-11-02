#!/usr/bin/env python3
"""ROM-free tests of the save's decoder (port/tools/oracle/savedecode.py), on synthetic Pokémon, parties and records
built from the decomp's headers' layout: savedecode_layout.c, compiled as the host build compiles the game's files,
prints the offsets, sizes and bits of include/pml/poke_party.h, include/pml/mail.h, include/system/pms_data.h and
include/save/records.h and of the blocks a round trip protects, and the tests build Pokémon, parties and records field
by field from it, encrypted with their own copy of the game's generator, never the module's, so that a wrong
generator in the module can't pass:

  - the layout: the module's LAYOUT is the headers', name for name; its field views are the headers' members (each
    struct's members written here by hand from the headers, with their C types) and cover every bit of PokeParty and
    GameRecords once; its BLOCK_SHUFFLE is blockShufLUT and its generator, the PID's shuffle bits and the records'
    encrypted span are the game's, read from src/system/poke_tool.c, src/gfl/gf_standard.c and src/system/record.c,
    with known answers for seeds 0 and 1;
  - decoding: a party of six with every block order, an empty slot (PokeParty_ClearPkm's), a Pokémon stored
    decrypted, records, and data changed under a checksum, each a problem that names its save, copy and slot;
  - the fields compared: the same block on both sides gives nothing; every field of every slot and every record,
    changed alone and encrypted again as the game does, shows as itself and its checksum and nothing else; every byte
    of both structs flipped alone shows; one PP shows as the PP and the checksum though most of the data's bytes
    change; the HP and the status show alone;
  - MUTANTS: deliberate bugs in savedecode.py, each with the tests that must catch it. MutationTests applies each to
    savedecode.py's source, loads the result in savedecode's place and runs those tests, which must fail; and it runs
    the same tests on savedecode as it is, which must pass.

The expected values are worked out by hand from the game's C and headers (the comments say which), never from
savedecode. Nothing here is a game save or comes from the ROM. Game functionality: the saves (the party, block 0x1a,
and the trainer card's records, block 0x26).

    test_savedecode.py --layout PROGRAM [unittest options]

PROGRAM is the build's port_test_savedecode_layout; the ctest test oracle_save_decode (port/cmake/oracle.cmake) passes
it.
"""

from __future__ import annotations

import functools
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
import types
import unittest
from unittest import mock

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[2]
sys.path.insert(0, str(REPO_ROOT / "port" / "tools" / "oracle"))

import savedecode  # noqa: E402

LAYOUT_PROGRAM = None

# PML_CryptoNextKey (src/system/poke_tool.c) and crypt_SAV_BVideo_MG_misc_data (src/gfl/gf_standard.c): the tests' own
# copy of the game's generator, with which every synthetic Pokémon, record and gift block here is encrypted
MULTIPLIER = 0x41c64e6d
INCREMENT = 0x6073
# EncryptRecordStorage (src/system/record.c): the records from words[1], 0x1d8 bytes, all of them but record 0
RECORDS_ENCRYPTED_FROM = 4
RECORDS_ENCRYPTED_SIZE = 0x1d8

# Each struct's members as the headers declare them, in order, with the C type of one element (a struct's name for an
# array of structs, "bits" for a bitfield): include/pml/poke_party.h, include/pml/mail.h, include/system/pms_data.h.
# BoxPkm's contentBuffer is the four blocks, and PartyPkm's base the BoxPkm
C_MEMBERS = {
    "BoxPkm": (("pid", "u32"), ("partyDecrypted", "bits"), ("boxDecrypted", "bits"), ("badEgg", "bits"),
               ("unk4_3", "bits"), ("checksum", "u16")),
    "PkmBlockA": (("species", "u16"), ("item", "u16"), ("id", "u32"), ("exp", "u32"), ("friendship", "u8"),
                  ("ability", "u8"), ("markings", "u8"), ("language", "u8"), ("hpEV", "u8"), ("atkEV", "u8"),
                  ("defEV", "u8"), ("speEV", "u8"), ("spaEV", "u8"), ("spdEV", "u8"), ("contest", "u8"),
                  ("ribbons", "u32")),
    "PkmBlockB": (("moves", "u16"), ("pp", "u8"), ("ppUps", "u8"), ("hpIV", "bits"), ("atkIV", "bits"),
                  ("defIV", "bits"), ("speIV", "bits"), ("spaIV", "bits"), ("spdIV", "bits"), ("isEgg", "bits"),
                  ("isNicknamed", "bits"), ("ribbons", "u32"), ("fatefulEncounter", "bits"), ("gender", "bits"),
                  ("form", "bits"), ("nature", "u8"), ("hiddenAbility", "bits"), ("nPoke", "bits"), ("unk1A_2", "bits"),
                  ("unk1C", "u32")),
    "PkmBlockC": (("nickname", "u16"), ("unk16", "u8"), ("originGame", "u8"), ("ribbons", "u64")),
    "PkmBlockD": (("otName", "u16"), ("eggDate", "u8"), ("metDate", "u8"), ("eggLocation", "u16"),
                  ("metLocation", "u16"), ("pokerus", "u8"), ("ball", "u8"), ("metLevel", "bits"), ("otGender", "bits"),
                  ("unk1D", "u8"), ("unk1E", "u8"), ("pokestarFame", "u8")),
    "PartyPkm": (("statusCond", "u32"), ("level", "u8"), ("unk8D", "u8"), ("nowHP", "u16"), ("maxHP", "u16"),
                 ("atk", "u16"), ("def", "u16"), ("spe", "u16"), ("spa", "u16"), ("spd", "u16"), ("mail", "MailData"),
                 ("unkD4", "u32"), ("unkD8", "u32")),
    "MailData": (("trainerId", "u32"), ("trainerGender", "u8"), ("region", "u8"), ("gameVersion", "u8"),
                 ("design", "u8"), ("trainerName", "u16"), ("unk18", "u16"), ("stampWord", "u16"),
                 ("message", "PMSData")),
    "PMSData": (("type", "u16"), ("id", "u16"), ("words", "u16")),
}
C_BYTES = {"u8": 1, "u16": 2, "u32": 4, "u64": 8}


@functools.cache
def headers() -> dict[str, int]:
    """savedecode_layout's lines, NAME VALUE, run once"""
    output = subprocess.run([LAYOUT_PROGRAM], check=True, capture_output=True, text=True).stdout
    layout = {}
    for line in output.splitlines():
        name, value = line.split()
        layout[name] = int(value, 16)
    return layout


def source(path: str) -> str:
    return (REPO_ROOT / path).read_text(encoding="utf-8")


def encrypt(data: bytes, seed: int) -> bytes:
    """PML_CryptoRun and decryptData, written again for the tests: each u16 XORed with the high half of the generator's
    next state, from seed"""
    out = bytearray(data)
    for offset in range(0, len(out) - 1, 2):
        seed = (seed * MULTIPLIER + INCREMENT) & 0xffffffff
        struct.pack_into("<H", out, offset, struct.unpack_from("<H", out, offset)[0] ^ (seed >> 16))
    return bytes(out)


@functools.cache
def games_block_shuffle() -> tuple[tuple[int, ...], ...]:
    """blockShufLUT as src/system/poke_tool.c writes it"""
    table = re.search(r"blockShufLUT\[32\]\[4\]\s*=\s*\{(.*?)\};", source("src/system/poke_tool.c"), re.S)
    return tuple(tuple(int(value, 16) for value in row.split(","))
                 for row in re.findall(r"\{\s*(0x[0-9a-f]+\s*,\s*0x[0-9a-f]+\s*,\s*0x[0-9a-f]+\s*,\s*0x[0-9a-f]+)\s*\}",
                                       table.group(1)))


@functools.cache
def games_shuffle_bits() -> tuple[int, int]:
    """The PID's mask and shift that PML_PkmGetParamBlockCore (src/system/poke_tool.c) picks a block order with"""
    match = re.search(r"blockShufLUT\[\(pid & (0x[0-9a-f]+)\) >> (\d+)\]", source("src/system/poke_tool.c"))
    return int(match.group(1), 16), int(match.group(2))


def shuffle_of(pid: int) -> tuple[int, ...]:
    mask, shift = games_shuffle_bits()
    return games_block_shuffle()[(pid & mask) >> shift]


def view(layout: dict[str, int], struct_name: str, prefix: str, at: int) -> list[tuple[str, int, int]]:
    """A struct's fields, by C_MEMBERS and the headers' offsets, at bit at: (path, first bit, width)"""
    fields = []
    for member, kind in C_MEMBERS[struct_name]:
        name = f"{struct_name}.{member}"
        if kind == "bits":
            fields.append((prefix + member, at + layout[f"{name}.bit"], layout[f"{name}.width"]))
            continue
        size = C_BYTES[kind] if kind in C_BYTES else layout[f"sizeof({kind})"]
        count = layout[f"sizeof({name})"] // size
        for index in range(count):
            path = prefix + member + (f"[{index}]" if count > 1 else "")
            bit = at + 8 * (layout[name] + index * size)
            fields += view(layout, kind, path + ".", bit) if kind in C_MEMBERS else [(path, bit, 8 * size)]
    return fields


def pokemon_view(layout: dict[str, int]) -> list[tuple[str, int, int]]:
    """A PartyPkm decoded: its head, its blocks A to D in order where its data is, then the party's own fields"""
    fields = view(layout, "BoxPkm", "base.", 0)
    at = layout["BoxPkm.contentBuffer"]
    for block in "ABCD":
        fields += view(layout, f"PkmBlock{block}", f"block{block}.", 8 * at)
        at += layout[f"sizeof(PkmBlock{block})"]
    return fields + view(layout, "PartyPkm", "", 0)


def field_at(layout: dict[str, int], path: str) -> tuple[int, int]:
    """A decoded Pokémon's field's first bit and width, by its path"""
    return {name: (bit, width) for name, bit, width in pokemon_view(layout)}[path]


def put(data: bytearray, bit: int, width: int, value: int) -> None:
    """A field of width bits at bit set to value, little-endian, the rest of its bytes kept"""
    first, end = bit // 8, (bit + width + 7) // 8
    number = int.from_bytes(data[first:end], "little")
    mask = ((1 << width) - 1) << bit % 8
    data[first:end] = ((number & ~mask) | (value << bit % 8 & mask)).to_bytes(end - first, "little")


def get(data: bytes, bit: int, width: int) -> int:
    return int.from_bytes(data[bit // 8:(bit + width + 7) // 8], "little") >> bit % 8 & ((1 << width) - 1)


def random_pokemon(layout: dict[str, int], rng: random.Random, pid: int | None = None) -> bytearray:
    """A decoded Pokémon (head, blocks A to D, the party's own fields) of random bytes, stored encrypted
    (BoxPkm.partyDecrypted clear), its PID given or random"""
    plain = bytearray(rng.randbytes(layout["sizeof(PartyPkm)"]))
    put(plain, layout["BoxPkm.partyDecrypted.bit"], 1, 0)
    if pid is not None:
        struct.pack_into("<I", plain, layout["BoxPkm.pid"], pid)
    return plain


def stored_pokemon(layout: dict[str, int], plain: bytes, checksum: int | None = None) -> bytes:
    """A PartyPkm as PokeParty_EncryptPkm leaves it, from its decoded form: blocks A to D where blockShufLUT puts them
    for its PID, its checksum the u16 sum of the plain data's u16s (PML_CryptoGenKey) or the one given, the data
    encrypted with the checksum and the party's own fields with the PID. A decoded form with BoxPkm.partyDecrypted set
    is stored in the clear, as PokeParty_DecryptPkm leaves a Pokémon"""
    (pid,) = struct.unpack_from("<I", plain, layout["BoxPkm.pid"])
    content_at, block_size = layout["BoxPkm.contentBuffer"], layout["sizeof(PkmBlockA)"]
    content = bytearray(layout["sizeof(PkmBuffer)"])
    for block, at in enumerate(shuffle_of(pid)):
        content[at:at + block_size] = plain[content_at + block * block_size:content_at + (block + 1) * block_size]
    if checksum is None:
        checksum = sum(struct.unpack(f"<{len(content) // 2}H", content)) & 0xffff
    head = bytearray(plain[:content_at])
    struct.pack_into("<H", head, layout["BoxPkm.checksum"], checksum)
    own = bytes(plain[layout["PartyPkm.statusCond"]:])
    if get(plain, layout["BoxPkm.partyDecrypted.bit"], 1):
        return bytes(head) + bytes(content) + own
    return bytes(head) + encrypt(bytes(content), checksum) + encrypt(own, pid)


def cleared_pokemon(layout: dict[str, int]) -> bytes:
    """PokeParty_ClearPkm's Pokémon (src/system/poke_tool.c): all zeros, then the data encrypted with the checksum 0 and
    the party's own fields with the PID 0"""
    content_at, own_at = layout["BoxPkm.contentBuffer"], layout["PartyPkm.statusCond"]
    return (bytes(content_at) + encrypt(bytes(layout["sizeof(PkmBuffer)"]), 0)
            + encrypt(bytes(layout["sizeof(PartyPkm)"] - own_at), 0))


def stored_party(layout: dict[str, int], members: list[bytes], count: int | None = None, capacity: int = 6,
                 slot_exists: int = 0, padding: bytes = bytes(3)) -> bytes:
    """A PokeParty: capacity, count (the members' unless given), each slot a stored Pokémon of members (raw, as
    stored_pokemon leaves them) or PokeParty_ClearPkm's, the slots' flags and the struct's padding"""
    block = bytearray(layout["sizeof(PokeParty)"])
    struct.pack_into("<ii", block, layout["PokeParty.capacity"], capacity, len(members) if count is None else count)
    size = layout["sizeof(PartyPkm)"]
    for slot in range(layout["PARTY_SIZE"]):
        member = members[slot] if slot < len(members) else cleared_pokemon(layout)
        block[layout["PokeParty.pkms"] + slot * size:layout["PokeParty.pkms"] + (slot + 1) * size] = member
    block[layout["PokeParty.slotExists"]] = slot_exists
    end = layout["PokeParty.slotExists"] + layout["sizeof(PokeParty.slotExists)"]
    block[end:] = padding
    return bytes(block)


def plain_records(layout: dict[str, int], values: dict[int, int], seed_high: int) -> bytearray:
    """GameRecords in the clear: record N's counter at words[N] (u32) below RECORD_COUNT_WORD, else at
    halfwords[N - RECORD_COUNT_WORD] (u16), as GetRecordCounter reads it (src/system/record.c); seedHigh given"""
    plain = bytearray(layout["sizeof(GameRecords)"])
    for record, value in values.items():
        if record < layout["RECORD_COUNT_WORD"]:
            struct.pack_into("<I", plain, layout["GameRecords.words"] + 4 * record, value)
        else:
            struct.pack_into("<H", plain, layout["GameRecords.halfwords"] + 2 * (record - layout["RECORD_COUNT_WORD"]),
                             value)
    struct.pack_into("<H", plain, layout["GameRecords.seedHigh"], seed_high)
    return plain


def stored_records(layout: dict[str, int], plain: bytes, checksum: int | None = None) -> bytes:
    """GameRecords as EncryptRecordStorage leaves them (src/system/record.c): the checksum the low 16 bits of the plain
    records' byte sum from words[1] (addUpDataStream), or the one given, and those records encrypted with it plus
    seedHigh << 16; record 0 and seedHigh as given"""
    start = layout["GameRecords.words"] + RECORDS_ENCRYPTED_FROM
    end = start + RECORDS_ENCRYPTED_SIZE
    if checksum is None:
        checksum = sum(plain[start:end]) & 0xffff
    (seed_high,) = struct.unpack_from("<H", plain, layout["GameRecords.seedHigh"])
    out = bytearray(plain)
    struct.pack_into("<H", out, layout["GameRecords.checksum"], checksum)
    out[start:end] = encrypt(bytes(plain[start:end]), checksum + (seed_high << 16))
    return bytes(out)


def differing(a: dict[str, int], b: dict[str, int]) -> set[str]:
    """The names whose values differ between two decoded views, and those only one has"""
    return {name for name in a.keys() | b.keys() if a.get(name) != b.get(name)}


class DecodeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.layout = headers()
        cls.pokemon_view = pokemon_view(cls.layout)
        rng = random.Random(0x5a5a)
        # PIDs whose bits 13 to 17 pick shuffles 0, 5, 11, 17, 23 and 31 of blockShufLUT
        cls.plains = [random_pokemon(cls.layout, rng, pid) for pid in (0x0000_0000, 0x0000_a000, 0x0001_6000,
                                                                        0x0002_2000, 0x0002_e000, 0x0003_e000)]


class LayoutTests(DecodeTest):
    def test_layout_is_the_headers(self):
        self.assertEqual(savedecode.LAYOUT, self.layout)

    def test_a_moved_layout_entry_is_caught(self):
        # Control: the comparison sees an entry moved, one missing and one added
        for name in ("PartyPkm.level", "GameRecords.seedHigh", "BoxPkm.unk4_3.width", "sizeof(EventWork)"):
            with self.subTest(name=name):
                self.assertNotEqual(dict(savedecode.LAYOUT, **{name: savedecode.LAYOUT[name] + 1}), self.layout)
        self.assertNotEqual({k: v for k, v in savedecode.LAYOUT.items() if k != "PARTY_SIZE"}, self.layout)
        self.assertNotEqual(dict(savedecode.LAYOUT, extra=1), self.layout)

    def test_field_views_are_the_headers(self):
        # Every field of Appendix A: the headers' members by hand with their C types, at the headers' offsets
        layout = self.layout
        self.assertEqual(list(savedecode.POKEMON_FIELDS), self.pokemon_view)
        party = [("capacity", 8 * layout["PokeParty.capacity"], 32), ("count", 8 * layout["PokeParty.count"], 32)]
        for slot in range(layout["PARTY_SIZE"]):
            at = 8 * (layout["PokeParty.pkms"] + slot * layout["sizeof(PartyPkm)"])
            party += [(f"pkms[{slot}].{path}", at + bit, width) for path, bit, width in self.pokemon_view]
        end = layout["PokeParty.slotExists"] + 1
        party += [("slotExists", 8 * layout["PokeParty.slotExists"], 8),
                  ("padding", 8 * end, 8 * (layout["sizeof(PokeParty)"] - end))]
        self.assertEqual(list(savedecode.PARTY_FIELDS), party)
        words = layout["RECORD_COUNT_WORD"]
        records = [(f"record {record:#04x}", 8 * (layout["GameRecords.words"] + 4 * record), 32)
                   for record in range(words)]
        records += [(f"record {record:#04x}", 8 * (layout["GameRecords.halfwords"] + 2 * (record - words)), 16)
                    for record in range(words, layout["RECORD_COUNT"])]
        records += [("checksum", 8 * layout["GameRecords.checksum"], 16),
                    ("seedHigh", 8 * layout["GameRecords.seedHigh"], 16)]
        self.assertEqual(list(savedecode.RECORD_FIELDS), records)
        # Appendix A's count: 134 fields a Pokémon, 808 a party, 169 records
        self.assertEqual((len(self.pokemon_view), len(party), len(records)), (134, 808, 169))

    def test_field_views_cover_each_struct_bit_for_bit(self):
        for name, fields, size in (("PokeParty", savedecode.PARTY_FIELDS, self.layout["sizeof(PokeParty)"]),
                                   ("GameRecords", savedecode.RECORD_FIELDS, self.layout["sizeof(GameRecords)"])):
            with self.subTest(struct=name):
                covered = [0] * (8 * size)
                for _, bit, width in fields:
                    for index in range(bit, bit + width):
                        covered[index] += 1
                self.assertEqual([index for index, times in enumerate(covered) if times != 1], [])
        # Control: a view without one field leaves its bits out
        covered = [0] * (8 * self.layout["sizeof(PartyPkm)"])
        for path, bit, width in savedecode.POKEMON_FIELDS:
            if path != "nowHP":
                for index in range(bit, bit + width):
                    covered[index] += 1
        self.assertEqual(covered.count(0), 16)

    def test_block_shuffle_is_the_games(self):
        self.assertEqual(len(games_block_shuffle()), 32)
        self.assertEqual(savedecode.BLOCK_SHUFFLE, games_block_shuffle())
        self.assertEqual((savedecode.PID_SHUFFLE_MASK, savedecode.PID_SHUFFLE_SHIFT), games_shuffle_bits())
        # Control: the comparison sees two rows swapped
        swapped = list(savedecode.BLOCK_SHUFFLE)
        swapped[1], swapped[2] = swapped[2], swapped[1]
        self.assertNotEqual(tuple(swapped), games_block_shuffle())

    def test_the_generator_is_the_games(self):
        poke_tool = re.search(r"\*key = \*key \* (0x[0-9a-f]+) \+ (0x[0-9a-f]+);", source("src/system/poke_tool.c"))
        gf_standard = re.search(r"\*seed = \*seed \* (\d+) \+ (\d+);", source("src/gfl/gf_standard.c"))
        self.assertEqual((int(poke_tool.group(1), 16), int(poke_tool.group(2), 16)), (MULTIPLIER, INCREMENT))
        self.assertEqual((int(gf_standard.group(1)), int(gf_standard.group(2))), (MULTIPLIER, INCREMENT))
        self.assertEqual((savedecode.CRYPTO_MULTIPLIER, savedecode.CRYPTO_INCREMENT), (MULTIPLIER, INCREMENT))
        # Known answers, by hand: from seed 0 the states are 0x6073, 0xe97e7b6a and 0x52713895 (high halves 0x0000,
        # 0xe97e, 0x5271); from seed 1, 0x41c64e6d + 0x6073 = 0x41c6aee0, then 0xac2115d3 and 0xd2eef54a
        for seed, words in ((0, (0x0000, 0xe97e, 0x5271)), (1, (0x41c6, 0xac21, 0xd2ee))):
            with self.subTest(seed=seed):
                self.assertEqual(savedecode.crypt(bytes(6), seed), struct.pack("<3H", *words))
                self.assertEqual(encrypt(bytes(6), seed), struct.pack("<3H", *words))
        # Its own inverse, and a last odd byte left as it is
        data = bytes(range(9))
        self.assertEqual(savedecode.crypt(savedecode.crypt(data, 0x1234), 0x1234), data)
        self.assertEqual(savedecode.crypt(data, 7)[8], 8)
        # PML_CryptoGenKey's u16 sum, addUpDataStream's byte sum
        self.assertEqual(savedecode.pokemon_checksum(struct.pack("<3H", 0xffff, 2, 0x10)), 0x0011)
        self.assertEqual(savedecode.byte_sum(b"\xff\x01\x02"), 0x102)

    def test_the_records_encrypted_span_is_the_games(self):
        # EncryptRecordStorage and DecryptRecordStorage encrypt from &records->words[1], 0x1d8 bytes
        found = re.findall(r"&records->words\[(\d+)\], (0x[0-9a-f]+)", source("src/system/record.c"))
        self.assertEqual(len(found), 3)
        self.assertEqual({(int(index), int(size, 16)) for index, size in found}, {(1, RECORDS_ENCRYPTED_SIZE)})
        self.assertEqual(savedecode.RECORDS_ENCRYPTED, self.layout["GameRecords.words"] + 4 * 1)
        self.assertEqual(savedecode.RECORDS_ENCRYPTED_SIZE, RECORDS_ENCRYPTED_SIZE)
        # The span ends at the checksum: every record but record 0
        self.assertEqual(savedecode.RECORDS_ENCRYPTED + RECORDS_ENCRYPTED_SIZE, self.layout["GameRecords.checksum"])



class DecodingTests(DecodeTest):
    def test_a_party_of_six_gives_back_every_field(self):
        layout = self.layout
        stored = [stored_pokemon(layout, plain) for plain in self.plains]
        fields, problems = savedecode.party_fields(stored_party(layout, stored, capacity=6, slot_exists=0x3f,
                                                                padding=b"\x01\x02\x03"))
        self.assertEqual(problems, [])
        self.assertEqual((fields["capacity"], fields["count"], fields["slotExists"], fields["padding"]),
                         (6, 6, 0x3f, 0x030201))
        for slot, plain in enumerate(self.plains):
            for path, bit, width in self.pokemon_view:
                if path == "base.checksum":
                    continue
                self.assertEqual(fields[f"pkms[{slot}].{path}"], get(plain, bit, width), (slot, path))
            # The checksum the game computed, the u16 sum of the plain data
            content = stored[slot][layout["BoxPkm.contentBuffer"]:layout["BoxPkm.contentBuffer"] + 0x80]
            (checksum,) = struct.unpack_from("<H", stored[slot], layout["BoxPkm.checksum"])
            self.assertEqual(fields[f"pkms[{slot}].base.checksum"], checksum)
            self.assertNotEqual(content, bytes(plain[8:0x88]))

    def test_an_empty_slot_decodes_to_zeros(self):
        fields, problems = savedecode.pokemon_fields(cleared_pokemon(self.layout))
        self.assertEqual(problems, [])
        self.assertEqual({path for path, value in fields.items() if value}, set())
        # Control: PokeParty_ClearPkm's bytes are its keystreams, not zeros
        self.assertNotEqual(cleared_pokemon(self.layout), bytes(self.layout["sizeof(PartyPkm)"]))

    def test_a_pokemon_stored_decrypted_is_read_as_it_is(self):
        layout = self.layout
        plain = bytearray(self.plains[3])
        put(plain, layout["BoxPkm.partyDecrypted.bit"], 1, 1)
        raw = stored_pokemon(layout, plain)
        self.assertEqual(raw[layout["PartyPkm.statusCond"]:], bytes(plain[layout["PartyPkm.statusCond"]:]))
        fields, problems = savedecode.pokemon_fields(raw)
        self.assertEqual(problems, [])
        self.assertEqual(fields["base.partyDecrypted"], 1)
        for path, bit, width in self.pokemon_view:
            if path != "base.checksum":
                self.assertEqual(fields[path], get(plain, bit, width), path)

    def test_data_changed_under_its_checksum_is_a_problem(self):
        layout = self.layout
        raw = bytearray(stored_pokemon(layout, self.plains[1]))
        raw[layout["BoxPkm.contentBuffer"] + 5] ^= 0x40
        _, problems = savedecode.pokemon_fields(bytes(raw))
        self.assertEqual(len(problems), 1)
        self.assertIn("not to its checksum", problems[0])
        # In a party, the problem names its slot
        party = stored_party(layout, [stored_pokemon(layout, self.plains[0]), bytes(raw)])
        _, problems = savedecode.party_fields(party)
        self.assertEqual(len(problems), 1)
        self.assertTrue(problems[0].startswith("pkms[1]: "), problems)
        # A stored checksum changed alone (the data encrypted with the right one) is a problem too
        stored = stored_pokemon(layout, self.plains[2])
        (checksum,) = struct.unpack_from("<H", stored, layout["BoxPkm.checksum"])
        changed = bytearray(stored)
        struct.pack_into("<H", changed, layout["BoxPkm.checksum"], checksum ^ 1)
        _, problems = savedecode.pokemon_fields(bytes(changed))
        self.assertEqual(len(problems), 1)

    def test_the_records_give_back_their_counters(self):
        layout = self.layout
        values = {record: random.Random(record).randrange(1 << (32 if record < 0x47 else 16)) for record in range(0xa7)}
        plain = plain_records(layout, values, 0x5b5a)
        stored = stored_records(layout, plain)
        fields, problems = savedecode.records_fields(stored)
        self.assertEqual(problems, [])
        self.assertEqual({record: fields[f"record {record:#04x}"] for record in range(0xa7)}, values)
        self.assertEqual(fields["seedHigh"], 0x5b5a)
        self.assertEqual(fields["checksum"], sum(plain[4:0x1dc]) & 0xffff)
        # Record 0 is in the clear; the others are not
        self.assertEqual(stored[:4], plain[:4])
        self.assertNotEqual(stored[4:8], plain[4:8])
        # Records changed under their checksum are a problem
        changed = bytearray(stored)
        changed[0x100] ^= 2
        _, problems = savedecode.records_fields(bytes(changed))
        self.assertEqual(len(problems), 1)
        self.assertIn("not to their checksum", problems[0])
        changed = bytearray(stored)
        struct.pack_into("<H", changed, layout["GameRecords.checksum"], fields["checksum"] ^ 0x100)
        _, problems = savedecode.records_fields(bytes(changed))
        self.assertEqual(len(problems), 1)


class ComparisonTest(DecodeTest):
    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        layout = cls.layout
        # A party of two, the four slots past its count PokeParty_ClearPkm's; and records with every counter set
        cls.party = stored_party(layout, [stored_pokemon(layout, plain) for plain in cls.plains[:2]], slot_exists=3)
        cls.party_fields, problems = savedecode.party_fields(cls.party)
        assert problems == [], problems
        values = {record: random.Random(~record).randrange(1 << (32 if record < 0x47 else 16))
                  for record in range(0xa7)}
        cls.plain_records = plain_records(layout, values, 0x5959)
        cls.records = stored_records(layout, cls.plain_records)
        cls.records_fields, problems = savedecode.records_fields(cls.records)
        assert problems == [], problems

    def decoded_slot(self, slot: int) -> bytearray:
        """The decoded form of the base party's slot: the plain it was built from, or zeros where it is cleared"""
        return bytearray(self.plains[slot]) if slot < 2 else bytearray(self.layout["sizeof(PartyPkm)"])

    def party_with(self, slot: int, plain: bytes, checksum: int | None = None) -> bytes:
        layout = self.layout
        size = layout["sizeof(PartyPkm)"]
        at = layout["PokeParty.pkms"] + slot * size
        party = bytearray(self.party)
        party[at:at + size] = stored_pokemon(layout, plain, checksum)
        return bytes(party)


class FieldSweepTests(ComparisonTest):
    def test_same_party_and_records_differ_in_nothing(self):
        self.assertEqual(differing(self.party_fields, savedecode.party_fields(self.party)[0]), set())
        self.assertEqual(differing(self.records_fields, savedecode.records_fields(self.records)[0]), set())

    def test_every_party_field_changed_alone_shows_as_itself(self):
        # Each field of each slot, the slots past the party's count too, its lowest bit flipped in the decoded form, and
        # the Pokémon encrypted again as PokeParty_EncryptPkm does: a field of the data shows with the checksum, which
        # the flip moves (it changes one u16 of the data by a power of two), and any other field alone
        content = range(8 * self.layout["BoxPkm.contentBuffer"], 8 * self.layout["PartyPkm.statusCond"])
        for slot in range(self.layout["PARTY_SIZE"]):
            for path, bit, width in self.pokemon_view:
                if path == "base.checksum":
                    continue
                plain = self.decoded_slot(slot)
                put(plain, bit, width, get(plain, bit, width) ^ 1)
                fields, problems = savedecode.party_fields(self.party_with(slot, plain))
                expected = {f"pkms[{slot}].{path}"}
                if bit in content:
                    expected.add(f"pkms[{slot}].base.checksum")
                self.assertEqual((differing(self.party_fields, fields), problems), (expected, []), (slot, path))

    def test_the_partys_own_fields_changed_alone_show_as_themselves(self):
        layout = self.layout
        for name, at in (("capacity", layout["PokeParty.capacity"]), ("count", layout["PokeParty.count"]),
                         ("slotExists", layout["PokeParty.slotExists"]),
                         ("padding", layout["PokeParty.slotExists"] + 1), ("padding", layout["sizeof(PokeParty)"] - 1)):
            party = bytearray(self.party)
            party[at] ^= 1
            fields, problems = savedecode.party_fields(bytes(party))
            self.assertEqual((differing(self.party_fields, fields), problems), ({name}, []), name)

    def test_every_record_changed_alone_shows_as_itself_and_the_checksum(self):
        layout = self.layout
        for record in range(layout["RECORD_COUNT"]):
            plain = bytearray(self.plain_records)
            if record < layout["RECORD_COUNT_WORD"]:
                plain[layout["GameRecords.words"] + 4 * record] ^= 1
            else:
                plain[layout["GameRecords.halfwords"] + 2 * (record - layout["RECORD_COUNT_WORD"])] ^= 1
            fields, problems = savedecode.records_fields(stored_records(layout, plain))
            # Record 0 is outside the sum, the others move it by one
            expected = {f"record {record:#04x}"} | ({"checksum"} if record else set())
            self.assertEqual((differing(self.records_fields, fields), problems), (expected, []), record)
        # seedHigh, the records encrypted again under it, alone
        plain = bytearray(self.plain_records)
        plain[layout["GameRecords.seedHigh"]] ^= 1
        fields, problems = savedecode.records_fields(stored_records(layout, plain))
        self.assertEqual((differing(self.records_fields, fields), problems), ({"seedHigh"}, []))

    def test_halves_that_cancel_leave_the_checksum(self):
        # exp 0x00010000 to 0x00000001: its u16 halves move by -1 and +1, so the sum, the checksum and the keystream
        # stay, and the field shows alone
        layout = self.layout
        bit, width = field_at(layout, "blockA.exp")
        plain = self.decoded_slot(0)
        put(plain, bit, width, 0x00010000)
        before = savedecode.party_fields(self.party_with(0, plain))[0]
        put(plain, bit, width, 0x00000001)
        after, problems = savedecode.party_fields(self.party_with(0, plain))
        self.assertEqual((differing(before, after), problems), ({"pkms[0].blockA.exp"}, []))

    def test_one_pp_changed_shows_as_the_pp_and_the_checksum(self):
        layout = self.layout
        bit, width = field_at(layout, "blockB.pp[2]")
        plain = self.decoded_slot(1)
        before = stored_pokemon(layout, plain)
        put(plain, bit, width, get(plain, bit, width) ^ 1)
        after = stored_pokemon(layout, plain)
        changed = {offset for offset in range(len(before)) if before[offset] != after[offset]}
        # As written: the checksum, and the data, which the new checksum's keystream encrypts again. A byte keeps its
        # value where the plain byte and the two keystreams' bytes happen to agree, about one time in 256, so the bytes
        # that differ are the exact set the tests' generator gives, most of the data's 128 but not all of them, and
        # nothing of the head or the party's own fields
        content_at = layout["BoxPkm.contentBuffer"]
        (old,) = struct.unpack_from("<H", before, layout["BoxPkm.checksum"])
        (new,) = struct.unpack_from("<H", after, layout["BoxPkm.checksum"])
        plain_before = encrypt(before[content_at:content_at + 0x80], old)
        plain_after = encrypt(after[content_at:content_at + 0x80], new)
        old_stream, new_stream = encrypt(bytes(0x80), old), encrypt(bytes(0x80), new)
        expected = {content_at + index for index in range(0x80)
                    if plain_before[index] ^ old_stream[index] != plain_after[index] ^ new_stream[index]}
        expected |= {layout["BoxPkm.checksum"] + index for index in range(2)
                     if struct.pack("<H", old)[index] != struct.pack("<H", new)[index]}
        self.assertEqual(changed, expected)
        self.assertGreater(len(changed & set(range(content_at, content_at + 0x80))), 100)
        # Decoded, the PP and the checksum alone
        size = layout["sizeof(PartyPkm)"]
        at = layout["PokeParty.pkms"] + size
        party = bytearray(self.party)
        party[at:at + size] = after
        fields, problems = savedecode.party_fields(bytes(party))
        self.assertEqual((differing(self.party_fields, fields), problems),
                         ({"pkms[1].blockB.pp[2]", "pkms[1].base.checksum"}, []))

    def test_the_hp_and_the_status_show_alone(self):
        layout = self.layout
        for path in ("nowHP", "statusCond"):
            bit, width = field_at(layout, path)
            plain = self.decoded_slot(0)
            put(plain, bit, width, get(plain, bit, width) ^ 0x5)
            raw = stored_pokemon(layout, plain)
            # As written, only that field's own bytes, encrypted with the PID a u16 at a time
            before = stored_pokemon(layout, self.decoded_slot(0))
            self.assertLessEqual({offset for offset in range(len(raw)) if raw[offset] != before[offset]},
                                 set(range(bit // 8, (bit + width) // 8)))
            fields, problems = savedecode.party_fields(self.party_with(0, plain))
            self.assertEqual((differing(self.party_fields, fields), problems), ({f"pkms[0].{path}"}, []))


class ByteSweepTests(ComparisonTest):
    def test_every_byte_of_the_party_flipped_alone_shows(self):
        missed = []
        for offset in range(len(self.party)):
            party = bytearray(self.party)
            party[offset] ^= 0x01
            fields, problems = savedecode.party_fields(bytes(party))
            if not differing(self.party_fields, fields) and not problems:
                missed.append(offset)
        self.assertEqual(missed, [])

    def test_every_byte_of_the_records_flipped_alone_shows(self):
        missed = []
        for offset in range(len(self.records)):
            records = bytearray(self.records)
            records[offset] ^= 0x01
            fields, problems = savedecode.records_fields(bytes(records))
            if not differing(self.records_fields, fields) and not problems:
                missed.append(offset)
        self.assertEqual(missed, [])


# Deliberate bugs in savedecode.py, each the text it replaces (found exactly once in savedecode.py), the text that
# replaces it, and the tests, ClassName or ClassName.test_name, at least one of which must fail on it
MUTANTS = (
    # A field left out of the views
    ("nowHP left out", '("nowHP", 2), ', "",
     ("LayoutTests.test_field_views_are_the_headers",)),
    ("block D left out", 'POKEMON_BLOCKS = ("PkmBlockA", "PkmBlockB", "PkmBlockC", "PkmBlockD")',
     'POKEMON_BLOCKS = ("PkmBlockA", "PkmBlockB", "PkmBlockC")',
     ("LayoutTests.test_field_views_cover_each_struct_bit_for_bit",)),
    ("the mail left out", '("mail", "MailData"), ', "",
     ("LayoutTests.test_field_views_cover_each_struct_bit_for_bit",)),
    ("a u16 array read as bytes", '("trainerName", 2)', '("trainerName", 1)',
     ("LayoutTests.test_field_views_are_the_headers",)),
    ("the slots past the count left out", "    for slot in range(PARTY_SIZE):\n        start = POKE_PARTY_PKMS",
     '    for slot in range(min(own["count"], PARTY_SIZE)):\n        start = POKE_PARTY_PKMS',
     ("FieldSweepTests.test_every_party_field_changed_alone_shows_as_itself",)),
    ("slotExists and the padding left out",
     "    fields.update((name, value) for name, value in own.items() if name not in fields)\n", "",
     ("FieldSweepTests.test_the_partys_own_fields_changed_alone_show_as_themselves",)),
    ("the halfword records left out",
     '    + [(f"record {RECORD_WORD_COUNT + index:#04x}", 8 * (LAYOUT["GameRecords.halfwords"] + 2 * index), 16)\n'
     '       for index in range(LAYOUT["sizeof(GameRecords.halfwords)"] // 2)]\n', "",
     ("DecodingTests.test_the_records_give_back_their_counters",)),
    ("seedHigh left out", ', ("seedHigh", 8 * RECORDS_SEED_HIGH, 16)', "",
     ("FieldSweepTests.test_every_record_changed_alone_shows_as_itself_and_the_checksum",)),
    # A wrong key or seed
    ("the generator's multiplier", "CRYPTO_MULTIPLIER = 0x41c64e6d", "CRYPTO_MULTIPLIER = 0x41c64e6f",
     ("LayoutTests.test_the_generator_is_the_games",)),
    ("the generator's increment", "CRYPTO_INCREMENT = 0x6073", "CRYPTO_INCREMENT = 0x6075",
     ("DecodingTests.test_a_party_of_six_gives_back_every_field",)),
    ("the state's low half", "words[index] ^= seed >> 16", "words[index] ^= seed & 0xffff",
     ("DecodingTests.test_the_records_give_back_their_counters",)),
    ("the party's fields decrypted with the checksum", "        own = crypt(own, pid)\n",
     "        own = crypt(own, checksum)\n", ("DecodingTests.test_a_party_of_six_gives_back_every_field",)),
    ("the data decrypted with the PID", "        content = crypt(content, checksum)\n",
     "        content = crypt(content, pid)\n", ("DecodingTests.test_a_party_of_six_gives_back_every_field",)),
    ("the blocks left encrypted", "        content = crypt(content, checksum)\n", "",
     ("DecodingTests.test_a_party_of_six_gives_back_every_field",)),
    ("the party's own fields left encrypted", "        own = crypt(own, pid)\n", "",
     ("DecodingTests.test_a_party_of_six_gives_back_every_field",)),
    ("the blocks not unshuffled", "order = BLOCK_SHUFFLE[(pid & PID_SHUFFLE_MASK) >> PID_SHUFFLE_SHIFT]",
     "order = BLOCK_SHUFFLE[0]", ("DecodingTests.test_a_party_of_six_gives_back_every_field",)),
    ("the records' seed without seedHigh", "crypt(bytes(block[start:end]), checksum + (seed_high << 16))",
     "crypt(bytes(block[start:end]), checksum)", ("DecodingTests.test_the_records_give_back_their_counters",)),
    ("the records' seed without the checksum", "crypt(bytes(block[start:end]), checksum + (seed_high << 16))",
     "crypt(bytes(block[start:end]), seed_high << 16)", ("DecodingTests.test_the_records_give_back_their_counters",)),
    ("the records decrypted from words[0]", 'RECORDS_ENCRYPTED = LAYOUT["GameRecords.words"] + 4\n',
     'RECORDS_ENCRYPTED = LAYOUT["GameRecords.words"]\n', ("DecodingTests.test_the_records_give_back_their_counters",)),
    ("a Pokémon stored decrypted decrypted again",
     "    if not raw[PARTY_DECRYPTED_BIT // 8] >> PARTY_DECRYPTED_BIT % 8 & 1:\n", "    if True:\n",
     ("DecodingTests.test_a_pokemon_stored_decrypted_is_read_as_it_is",)),
    ("a Pokémon's checksum never wrong", "    if content_sum != checksum:\n", "    if False:\n",
     ("DecodingTests.test_data_changed_under_its_checksum_is_a_problem",)),
    ("the records' checksum never wrong", "    if records_sum != checksum:\n", "    if False:\n",
     ("DecodingTests.test_the_records_give_back_their_counters",)),
    # The layout
    ("the level a byte on", '"PartyPkm.level": 0x8c,', '"PartyPkm.level": 0x8d,',
     ("LayoutTests.test_layout_is_the_headers",)),
)


class MutationTests(unittest.TestCase):
    """Every row of MUTANTS: savedecode.py with that one bug, loaded in savedecode's place, fails at least one of its
    tests; and savedecode as it is passes all of them through the same runner, so that what fails is the bug"""

    @classmethod
    def setUpClass(cls):
        cls.source = Path(savedecode.__file__).read_text(encoding="utf-8")

    def load(self, text: str) -> types.ModuleType:
        """A module made from that source as savedecode.py's, its path as savedecode.py's own"""
        module = types.ModuleType("savedecode_mutant")
        module.__file__ = savedecode.__file__
        with mock.patch.object(sys, "path", list(sys.path)), mock.patch.dict(sys.modules,
                                                                              {"savedecode_mutant": module}):
            exec(compile(text, savedecode.__file__, "exec"), module.__dict__)
        return module

    def run_tests(self, module: types.ModuleType, names: tuple[str, ...], first_failure: bool) -> unittest.TestResult:
        """The tests named, with module as savedecode, each class a subclass of its own so that its class attributes
        aren't the suite's; with first_failure, they stop at the first test or subtest that fails"""
        suite = unittest.TestSuite()
        for name in names:
            class_name, _, method = name.partition(".")
            isolated = type(class_name, (globals()[class_name],), {"__module__": __name__})
            suite.addTests([isolated(method)] if method else unittest.defaultTestLoader.loadTestsFromTestCase(isolated))
        result = unittest.TestResult()
        result.failfast = first_failure
        with mock.patch.dict(globals(), {"savedecode": module}):
            suite.run(result)
        return result

    def test_savedecode_as_it_is_passes_the_mutants_tests(self):
        names = tuple(sorted({name for *_, tests in MUTANTS for name in tests}))
        result = self.run_tests(self.load(self.source), names, first_failure=False)
        self.assertEqual(result.testsRun, len(names))
        self.assertTrue(result.wasSuccessful(), [str(test) for test, _ in result.failures + result.errors])

    def test_each_mutant_fails_its_tests(self):
        self.assertEqual(len({name for name, *_ in MUTANTS}), len(MUTANTS))
        for name, old, new, tests in MUTANTS:
            with self.subTest(mutant=name):
                self.assertEqual(self.source.count(old), 1, "the text a mutant replaces is in savedecode.py once")
                self.assertNotEqual(old, new)
                result = self.run_tests(self.load(self.source.replace(old, new)), tests, first_failure=True)
                self.assertFalse(result.wasSuccessful(), f"savedecode with this bug passes {', '.join(tests)}")


def main() -> None:
    global LAYOUT_PROGRAM
    argv = sys.argv[1:]
    if "--layout" not in argv or argv.index("--layout") + 1 >= len(argv):
        print("usage: test_savedecode.py --layout PROGRAM [unittest options] (PROGRAM: the build's "
              "port_test_savedecode_layout)", file=sys.stderr)
        sys.exit(2)
    index = argv.index("--layout")
    LAYOUT_PROGRAM = os.path.abspath(argv[index + 1])
    del argv[index:index + 2]
    unittest.main(argv=[sys.argv[0]] + argv)


if __name__ == "__main__":
    main()
