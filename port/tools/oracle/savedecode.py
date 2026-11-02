#!/usr/bin/env python3
"""The save's party and game records decoded field by field: the one copy of the save's crypto, for the save lab's
reader (port/tools/lab/savelab.py) and the oracle (oracle.py's mystery_gift_decrypted takes its generator). The save
lab's planned party verbs (port/tools/lab/README.md) build their Pokémon codec on this module, so that the crypto keeps
one copy.

Game functionality: the saves, and in them the party (block 0x1a) and the trainer card's records (block 0x26), with the
generator the mystery gift block (0x22) shares.

- The party, PokeParty (include/pml/poke_party.h), holds six PartyPkm. Each is a BoxPkm's head in the clear (the
  personality value, PID; the flags; the checksum), its 128 bytes of data, the four blocks PkmBlockA to PkmBlockD in
  the order bits 13 to 17 of the PID pick (blockShufLUT, PML_PkmGetParamBlockCore), encrypted with the checksum, and
  the party's own fields (the status, level, HP, stats and the mail, include/pml/mail.h and
  include/system/pms_data.h) encrypted with the PID: PokeParty_EncryptPkm and PokeParty_DecryptPkm in
  src/system/poke_tool.c. PML_CryptoRun XORs each u16 with the high half of the generator's next state,
  x' = x * 0x41c64e6d + 0x6073, and the checksum is PML_CryptoGenKey's u16 sum of the plain data, made again at every
  encryption, so one PP changed re-encrypts all 128 bytes. An empty slot holds PokeParty_ClearPkm's Pokémon, zeros
  encrypted with keys 0 (src/system/pokeparty.c), and a Pokémon's mail is CreateMailData's, allocated from the heap
  uncleared: ResetMailData (src/system/mail.c) sets every field of it but unk18 and the three sentences' ids.
- The records, GameRecords (include/save/records.h), are 0xa7 counters, the first 0x47 u32 and the rest u16, then a
  checksum and seedHigh. Record 0, the step counter, is in the clear; the rest are encrypted with the same generator
  (decryptData, src/gfl/gf_standard.c), seeded with the checksum, addUpDataStream's byte sum of the plain records,
  plus seedHigh << 16, which New Game takes from the VBlank count (initTrainerCardInfoBlk,
  src/system/trainer_card_savedata.c). Every RecordAdd, RecordSet and RecordSetIfHigher encrypts them again with the
  new sum (EncryptRecordStorage, src/system/record.c), so one counter changed re-encrypts the block.

So bytes can't say which field of those blocks changed, and this module names each field. LAYOUT holds every offset,
size and number of the headers the module uses, under the names port/tests/oracle/savedecode_layout.c prints them with
as the compiler lays the headers out. pokemon_fields, party_fields and records_fields give every field of a block by
its path (POKEMON_FIELDS, PARTY_FIELDS, RECORD_FIELDS), and what doesn't add up to its checksum. Standard library
only: never oracle.py or savelab.py, which import this module.
"""

from __future__ import annotations

import struct

# What the decomp's headers give for every offset, size and number below, under the names that
# port/tests/oracle/savedecode_layout.c prints them with: a member's offset and its size (all of an array's elements),
# a bitfield's first bit from its struct's start (byte * 8 + bit, from each byte's lowest) and its width, a struct's
# size, a constant's value. The test holds one to the other
LAYOUT = {
    # include/pml/poke_party.h: the party, a Pokémon of it (a BoxPkm and the party's own fields) and its four blocks
    "sizeof(PokeParty)": 0x534,
    "PokeParty.capacity": 0x0, "sizeof(PokeParty.capacity)": 0x4,
    "PokeParty.count": 0x4, "sizeof(PokeParty.count)": 0x4,
    "PokeParty.pkms": 0x8, "sizeof(PokeParty.pkms)": 0x528,
    "PokeParty.slotExists": 0x530, "sizeof(PokeParty.slotExists)": 0x1,
    "PARTY_SIZE": 0x6,
    "sizeof(PartyPkm)": 0xdc,
    "PartyPkm.base": 0x0, "sizeof(PartyPkm.base)": 0x88,
    "PartyPkm.statusCond": 0x88, "sizeof(PartyPkm.statusCond)": 0x4,
    "PartyPkm.level": 0x8c, "sizeof(PartyPkm.level)": 0x1,
    "PartyPkm.unk8D": 0x8d, "sizeof(PartyPkm.unk8D)": 0x1,
    "PartyPkm.nowHP": 0x8e, "sizeof(PartyPkm.nowHP)": 0x2,
    "PartyPkm.maxHP": 0x90, "sizeof(PartyPkm.maxHP)": 0x2,
    "PartyPkm.atk": 0x92, "sizeof(PartyPkm.atk)": 0x2,
    "PartyPkm.def": 0x94, "sizeof(PartyPkm.def)": 0x2,
    "PartyPkm.spe": 0x96, "sizeof(PartyPkm.spe)": 0x2,
    "PartyPkm.spa": 0x98, "sizeof(PartyPkm.spa)": 0x2,
    "PartyPkm.spd": 0x9a, "sizeof(PartyPkm.spd)": 0x2,
    "PartyPkm.mail": 0x9c, "sizeof(PartyPkm.mail)": 0x38,
    "PartyPkm.unkD4": 0xd4, "sizeof(PartyPkm.unkD4)": 0x4,
    "PartyPkm.unkD8": 0xd8, "sizeof(PartyPkm.unkD8)": 0x4,
    "sizeof(BoxPkm)": 0x88,
    "BoxPkm.pid": 0x0, "sizeof(BoxPkm.pid)": 0x4,
    "BoxPkm.partyDecrypted.bit": 0x20, "BoxPkm.partyDecrypted.width": 0x1,
    "BoxPkm.boxDecrypted.bit": 0x21, "BoxPkm.boxDecrypted.width": 0x1,
    "BoxPkm.badEgg.bit": 0x22, "BoxPkm.badEgg.width": 0x1,
    "BoxPkm.unk4_3.bit": 0x23, "BoxPkm.unk4_3.width": 0xd,
    "BoxPkm.checksum": 0x6, "sizeof(BoxPkm.checksum)": 0x2,
    "BoxPkm.contentBuffer": 0x8, "sizeof(BoxPkm.contentBuffer)": 0x80,
    "sizeof(PkmBuffer)": 0x80,
    "sizeof(PkmBlockA)": 0x20,
    "PkmBlockA.species": 0x0, "sizeof(PkmBlockA.species)": 0x2,
    "PkmBlockA.item": 0x2, "sizeof(PkmBlockA.item)": 0x2,
    "PkmBlockA.id": 0x4, "sizeof(PkmBlockA.id)": 0x4,
    "PkmBlockA.exp": 0x8, "sizeof(PkmBlockA.exp)": 0x4,
    "PkmBlockA.friendship": 0xc, "sizeof(PkmBlockA.friendship)": 0x1,
    "PkmBlockA.ability": 0xd, "sizeof(PkmBlockA.ability)": 0x1,
    "PkmBlockA.markings": 0xe, "sizeof(PkmBlockA.markings)": 0x1,
    "PkmBlockA.language": 0xf, "sizeof(PkmBlockA.language)": 0x1,
    "PkmBlockA.hpEV": 0x10, "sizeof(PkmBlockA.hpEV)": 0x1,
    "PkmBlockA.atkEV": 0x11, "sizeof(PkmBlockA.atkEV)": 0x1,
    "PkmBlockA.defEV": 0x12, "sizeof(PkmBlockA.defEV)": 0x1,
    "PkmBlockA.speEV": 0x13, "sizeof(PkmBlockA.speEV)": 0x1,
    "PkmBlockA.spaEV": 0x14, "sizeof(PkmBlockA.spaEV)": 0x1,
    "PkmBlockA.spdEV": 0x15, "sizeof(PkmBlockA.spdEV)": 0x1,
    "PkmBlockA.contest": 0x16, "sizeof(PkmBlockA.contest)": 0x6,
    "PkmBlockA.ribbons": 0x1c, "sizeof(PkmBlockA.ribbons)": 0x4,
    "sizeof(PkmBlockB)": 0x20,
    "PkmBlockB.moves": 0x0, "sizeof(PkmBlockB.moves)": 0x8,
    "PkmBlockB.pp": 0x8, "sizeof(PkmBlockB.pp)": 0x4,
    "PkmBlockB.ppUps": 0xc, "sizeof(PkmBlockB.ppUps)": 0x4,
    "PkmBlockB.hpIV.bit": 0x80, "PkmBlockB.hpIV.width": 0x5,
    "PkmBlockB.atkIV.bit": 0x85, "PkmBlockB.atkIV.width": 0x5,
    "PkmBlockB.defIV.bit": 0x8a, "PkmBlockB.defIV.width": 0x5,
    "PkmBlockB.speIV.bit": 0x8f, "PkmBlockB.speIV.width": 0x5,
    "PkmBlockB.spaIV.bit": 0x94, "PkmBlockB.spaIV.width": 0x5,
    "PkmBlockB.spdIV.bit": 0x99, "PkmBlockB.spdIV.width": 0x5,
    "PkmBlockB.isEgg.bit": 0x9e, "PkmBlockB.isEgg.width": 0x1,
    "PkmBlockB.isNicknamed.bit": 0x9f, "PkmBlockB.isNicknamed.width": 0x1,
    "PkmBlockB.ribbons": 0x14, "sizeof(PkmBlockB.ribbons)": 0x4,
    "PkmBlockB.fatefulEncounter.bit": 0xc0, "PkmBlockB.fatefulEncounter.width": 0x1,
    "PkmBlockB.gender.bit": 0xc1, "PkmBlockB.gender.width": 0x2,
    "PkmBlockB.form.bit": 0xc3, "PkmBlockB.form.width": 0x5,
    "PkmBlockB.nature": 0x19, "sizeof(PkmBlockB.nature)": 0x1,
    "PkmBlockB.hiddenAbility.bit": 0xd0, "PkmBlockB.hiddenAbility.width": 0x1,
    "PkmBlockB.nPoke.bit": 0xd1, "PkmBlockB.nPoke.width": 0x1,
    "PkmBlockB.unk1A_2.bit": 0xd2, "PkmBlockB.unk1A_2.width": 0xe,
    "PkmBlockB.unk1C": 0x1c, "sizeof(PkmBlockB.unk1C)": 0x4,
    "sizeof(PkmBlockC)": 0x20,
    "PkmBlockC.nickname": 0x0, "sizeof(PkmBlockC.nickname)": 0x16,
    "PkmBlockC.unk16": 0x16, "sizeof(PkmBlockC.unk16)": 0x1,
    "PkmBlockC.originGame": 0x17, "sizeof(PkmBlockC.originGame)": 0x1,
    "PkmBlockC.ribbons": 0x18, "sizeof(PkmBlockC.ribbons)": 0x8,
    "sizeof(PkmBlockD)": 0x20,
    "PkmBlockD.otName": 0x0, "sizeof(PkmBlockD.otName)": 0x10,
    "PkmBlockD.eggDate": 0x10, "sizeof(PkmBlockD.eggDate)": 0x3,
    "PkmBlockD.metDate": 0x13, "sizeof(PkmBlockD.metDate)": 0x3,
    "PkmBlockD.eggLocation": 0x16, "sizeof(PkmBlockD.eggLocation)": 0x2,
    "PkmBlockD.metLocation": 0x18, "sizeof(PkmBlockD.metLocation)": 0x2,
    "PkmBlockD.pokerus": 0x1a, "sizeof(PkmBlockD.pokerus)": 0x1,
    "PkmBlockD.ball": 0x1b, "sizeof(PkmBlockD.ball)": 0x1,
    "PkmBlockD.metLevel.bit": 0xe0, "PkmBlockD.metLevel.width": 0x7,
    "PkmBlockD.otGender.bit": 0xe7, "PkmBlockD.otGender.width": 0x1,
    "PkmBlockD.unk1D": 0x1d, "sizeof(PkmBlockD.unk1D)": 0x1,
    "PkmBlockD.unk1E": 0x1e, "sizeof(PkmBlockD.unk1E)": 0x1,
    "PkmBlockD.pokestarFame": 0x1f, "sizeof(PkmBlockD.pokestarFame)": 0x1,
    # include/pml/mail.h and include/system/pms_data.h: a Pokémon's mail and its three sentences
    "sizeof(MailData)": 0x38,
    "MailData.trainerId": 0x0, "sizeof(MailData.trainerId)": 0x4,
    "MailData.trainerGender": 0x4, "sizeof(MailData.trainerGender)": 0x1,
    "MailData.region": 0x5, "sizeof(MailData.region)": 0x1,
    "MailData.gameVersion": 0x6, "sizeof(MailData.gameVersion)": 0x1,
    "MailData.design": 0x7, "sizeof(MailData.design)": 0x1,
    "MailData.trainerName": 0x8, "sizeof(MailData.trainerName)": 0x10,
    "MailData.unk18": 0x18, "sizeof(MailData.unk18)": 0x6,
    "MailData.stampWord": 0x1e, "sizeof(MailData.stampWord)": 0x2,
    "MailData.message": 0x20, "sizeof(MailData.message)": 0x18,
    "sizeof(PMSData)": 0x8,
    "PMSData.type": 0x0, "sizeof(PMSData.type)": 0x2,
    "PMSData.id": 0x2, "sizeof(PMSData.id)": 0x2,
    "PMSData.words": 0x4, "sizeof(PMSData.words)": 0x4,
    "PMS_SENTENCE_WORD_MAX": 0x2,
    # include/save/records.h: the trainer card's records
    "sizeof(GameRecords)": 0x1e0,
    "GameRecords.words": 0x0, "sizeof(GameRecords.words)": 0x11c,
    "GameRecords.halfwords": 0x11c, "sizeof(GameRecords.halfwords)": 0xc0,
    "GameRecords.checksum": 0x1dc, "sizeof(GameRecords.checksum)": 0x2,
    "GameRecords.seedHigh": 0x1de, "sizeof(GameRecords.seedHigh)": 0x2,
    "RECORD_COUNT_WORD": 0x47,
    "RECORD_COUNT": 0xa7,
    # include/save/save_control.h: the blocks the module decodes or protects, and their structs' sizes
    "SAVE_BLOCK_BAG": 0x19,
    "SAVE_BLOCK_PARTY": 0x1a,
    "SAVE_BLOCK_PLAYER": 0x1c,
    "SAVE_BLOCK_MYSTERY_GIFT": 0x22,
    "SAVE_BLOCK_RECORDS": 0x26,
    "SAVE_BLOCK_EVENT_WORK": 0x2d,
    "SAVE_BLOCK_TRAINER_GAME_INFO": 0x34,
    "sizeof(BagSave)": 0x9ec,
    "sizeof(PlayerSave)": 0xa8,
    "sizeof(MysteryGiftSaveData)": 0xa94,
    "sizeof(EventWork)": 0x4de,
    "sizeof(TrainerGameInfoSave)": 0xf0,
    # What a round trip never declares by byte besides the party and the records (include/save/player_savedata.h,
    # mystery_gift.h, trainer_game_info.h; the bag and the event work are protected whole)
    "PlayerSave.nowSpawn": 0x0, "sizeof(PlayerSave.nowSpawn)": 0x1c,
    "PlayerSave.unk1C": 0x1c, "sizeof(PlayerSave.unk1C)": 0x1c,
    "PlayerSave.nextSpawn": 0x38, "sizeof(PlayerSave.nextSpawn)": 0x1c,
    "PlayerSave.escapeRopeSpawn": 0x54, "sizeof(PlayerSave.escapeRopeSpawn)": 0x1c,
    "PlayerSave.location": 0x80, "sizeof(PlayerSave.location)": 0x1c,
    "MysteryGiftSaveData.checksum": 0xa90, "sizeof(MysteryGiftSaveData.checksum)": 0x2,
    "MysteryGiftSaveData.key": 0xa92, "sizeof(MysteryGiftSaveData.key)": 0x2,
    "TrainerGameInfoSave.cash": 0x0, "sizeof(TrainerGameInfoSave.cash)": 0x4,
    "TrainerGameInfoSave.badges": 0x4, "sizeof(TrainerGameInfoSave.badges)": 0x4,
}

PARTY_SIZE = LAYOUT["PARTY_SIZE"]
POKE_PARTY_SIZE = LAYOUT["sizeof(PokeParty)"]
POKE_PARTY_PKMS = LAYOUT["PokeParty.pkms"]
PARTY_PKM_SIZE = LAYOUT["sizeof(PartyPkm)"]
BOX_PKM_PID = LAYOUT["BoxPkm.pid"]
BOX_PKM_CHECKSUM = LAYOUT["BoxPkm.checksum"]
BOX_PKM_CONTENT = LAYOUT["BoxPkm.contentBuffer"]
PKM_BUFFER_SIZE = LAYOUT["sizeof(PkmBuffer)"]
PARTY_DECRYPTED_BIT = LAYOUT["BoxPkm.partyDecrypted.bit"]
PKM_BLOCK_SIZE = LAYOUT["sizeof(PkmBlockA)"]
# The party's own fields, from statusCond to the PartyPkm's end, which PokeParty_EncryptPkm encrypts with the PID
PARTY_PKM_OWN = LAYOUT["PartyPkm.statusCond"]
RECORDS_SIZE = LAYOUT["sizeof(GameRecords)"]
RECORD_COUNT = LAYOUT["RECORD_COUNT"]
RECORDS_CHECKSUM = LAYOUT["GameRecords.checksum"]
RECORDS_SEED_HIGH = LAYOUT["GameRecords.seedHigh"]
# EncryptRecordStorage and DecryptRecordStorage (src/system/record.c) encrypt the records from &words[1], the second
# u32, for their literal 0x1d8 bytes: every record but record 0, the step counter
RECORDS_ENCRYPTED = LAYOUT["GameRecords.words"] + 4
RECORDS_ENCRYPTED_SIZE = 0x1d8

# blockShufLUT (src/system/poke_tool.c): the offsets of blocks A to D in a Pokémon's data, by bits 13 to 17 of its PID,
# which PML_PkmGetParamBlockCore takes as (pid & 0x3e000) >> 13
BLOCK_SHUFFLE = (
    (0x00, 0x20, 0x40, 0x60), (0x00, 0x20, 0x60, 0x40), (0x00, 0x40, 0x20, 0x60), (0x00, 0x60, 0x20, 0x40),
    (0x00, 0x40, 0x60, 0x20), (0x00, 0x60, 0x40, 0x20), (0x20, 0x00, 0x40, 0x60), (0x20, 0x00, 0x60, 0x40),
    (0x40, 0x00, 0x20, 0x60), (0x60, 0x00, 0x20, 0x40), (0x40, 0x00, 0x60, 0x20), (0x60, 0x00, 0x40, 0x20),
    (0x20, 0x40, 0x00, 0x60), (0x20, 0x60, 0x00, 0x40), (0x40, 0x20, 0x00, 0x60), (0x60, 0x20, 0x00, 0x40),
    (0x40, 0x60, 0x00, 0x20), (0x60, 0x40, 0x00, 0x20), (0x20, 0x40, 0x60, 0x00), (0x20, 0x60, 0x40, 0x00),
    (0x40, 0x20, 0x60, 0x00), (0x60, 0x20, 0x40, 0x00), (0x40, 0x60, 0x20, 0x00), (0x60, 0x40, 0x20, 0x00),
    (0x00, 0x20, 0x40, 0x60), (0x00, 0x20, 0x60, 0x40), (0x00, 0x40, 0x20, 0x60), (0x00, 0x60, 0x20, 0x40),
    (0x00, 0x40, 0x60, 0x20), (0x00, 0x60, 0x40, 0x20), (0x20, 0x00, 0x40, 0x60), (0x20, 0x00, 0x60, 0x40),
)
PID_SHUFFLE_MASK = 0x3e000
PID_SHUFFLE_SHIFT = 13

# The generator of PML_CryptoNextKey (src/system/poke_tool.c) and crypt_SAV_BVideo_MG_misc_data
# (src/gfl/gf_standard.c): the same, 1103515245 and 24691 in the latter's decimal
CRYPTO_MULTIPLIER = 0x41c64e6d
CRYPTO_INCREMENT = 0x6073


def crypt(data: bytes, seed: int) -> bytes:
    """PML_CryptoRun (src/system/poke_tool.c) and decryptData (src/gfl/gf_standard.c): each u16 of data XORed with the
    high half of the generator's next state, from seed; its own inverse. A last odd byte stays as it is, as the game's
    size / 2 loops leave it"""
    count = len(data) // 2
    words = list(struct.unpack_from(f"<{count}H", data))
    for index in range(count):
        seed = (seed * CRYPTO_MULTIPLIER + CRYPTO_INCREMENT) & 0xffffffff
        words[index] ^= seed >> 16
    return struct.pack(f"<{count}H", *words) + bytes(data[2 * count:])


def pokemon_checksum(content: bytes) -> int:
    """PML_CryptoGenKey (src/system/poke_tool.c): the u16 sum of a Pokémon's plain data's u16s, its checksum"""
    return sum(struct.unpack_from(f"<{len(content) // 2}H", content)) & 0xffff


def byte_sum(data: bytes) -> int:
    """addUpDataStream (src/gfl/gf_standard.c): the sum of the bytes, whose low 16 bits are the records' checksum and
    the mystery gift block's"""
    return sum(data)


# The members of the structs the field views name, in their headers' order: each with the size in bytes of one of its
# elements (an array has more than one), BITS for a bitfield, or the struct its elements are. BoxPkm's contentBuffer is
# its four blocks, which take its place in a view, and PartyPkm's base is the BoxPkm, its head
BITS = "bits"
MEMBERS = {
    "BoxPkm": (("pid", 4), ("partyDecrypted", BITS), ("boxDecrypted", BITS), ("badEgg", BITS), ("unk4_3", BITS),
               ("checksum", 2)),
    "PkmBlockA": (("species", 2), ("item", 2), ("id", 4), ("exp", 4), ("friendship", 1), ("ability", 1),
                  ("markings", 1), ("language", 1), ("hpEV", 1), ("atkEV", 1), ("defEV", 1), ("speEV", 1),
                  ("spaEV", 1), ("spdEV", 1), ("contest", 1), ("ribbons", 4)),
    "PkmBlockB": (("moves", 2), ("pp", 1), ("ppUps", 1), ("hpIV", BITS), ("atkIV", BITS), ("defIV", BITS),
                  ("speIV", BITS), ("spaIV", BITS), ("spdIV", BITS), ("isEgg", BITS), ("isNicknamed", BITS),
                  ("ribbons", 4), ("fatefulEncounter", BITS), ("gender", BITS), ("form", BITS), ("nature", 1),
                  ("hiddenAbility", BITS), ("nPoke", BITS), ("unk1A_2", BITS), ("unk1C", 4)),
    "PkmBlockC": (("nickname", 2), ("unk16", 1), ("originGame", 1), ("ribbons", 8)),
    "PkmBlockD": (("otName", 2), ("eggDate", 1), ("metDate", 1), ("eggLocation", 2), ("metLocation", 2),
                  ("pokerus", 1), ("ball", 1), ("metLevel", BITS), ("otGender", BITS), ("unk1D", 1), ("unk1E", 1),
                  ("pokestarFame", 1)),
    "PartyPkm": (("statusCond", 4), ("level", 1), ("unk8D", 1), ("nowHP", 2), ("maxHP", 2), ("atk", 2), ("def", 2),
                 ("spe", 2), ("spa", 2), ("spd", 2), ("mail", "MailData"), ("unkD4", 4), ("unkD8", 4)),
    "MailData": (("trainerId", 4), ("trainerGender", 1), ("region", 1), ("gameVersion", 1), ("design", 1),
                 ("trainerName", 2), ("unk18", 2), ("stampWord", 2), ("message", "PMSData")),
    "PMSData": (("type", 2), ("id", 2), ("words", 2)),
}
# The blocks of a Pokémon's data, A to D, as blockShufLUT numbers them
POKEMON_BLOCKS = ("PkmBlockA", "PkmBlockB", "PkmBlockC", "PkmBlockD")


def _member_fields(struct_name: str, prefix: str, at: int) -> list[tuple[str, int, int]]:
    """The fields of a struct that starts at bit at of a view, as (path, first bit, width in bits)"""
    fields = []
    for member, element in MEMBERS[struct_name]:
        name = f"{struct_name}.{member}"
        if element == BITS:
            fields.append((prefix + member, at + LAYOUT[f"{name}.bit"], LAYOUT[f"{name}.width"]))
            continue
        size = LAYOUT[f"sizeof({element})"] if isinstance(element, str) else element
        count = LAYOUT[f"sizeof({name})"] // size
        for index in range(count):
            path = prefix + member + (f"[{index}]" if count > 1 else "")
            start = at + 8 * (LAYOUT[name] + index * size)
            if isinstance(element, str):
                fields += _member_fields(element, path + ".", start)
            else:
                fields.append((path, start, 8 * size))
    return fields


def _pokemon_view() -> tuple[tuple[str, int, int], ...]:
    """A PartyPkm decoded (pokemon_plain): its head as stored, base.*; its four blocks in order A to D where its data
    is, blockA.* to blockD.*; and the party's own fields, statusCond to unkD8, the mail's mail.*"""
    fields = _member_fields("BoxPkm", "base.", 8 * LAYOUT["PartyPkm.base"])
    for index, block in enumerate(POKEMON_BLOCKS):
        fields += _member_fields(block, f"block{block[-1]}.", 8 * (BOX_PKM_CONTENT + index * PKM_BLOCK_SIZE))
    fields += _member_fields("PartyPkm", "", 0)
    return tuple(fields)


# Every field of a decoded Pokémon, of a decoded PokeParty and of the plain records: (path, first bit, width in bits). Each view's fields cover every bit of its
# struct once
POKEMON_FIELDS = _pokemon_view()
POKEMON_PATHS = frozenset(path for path, _, _ in POKEMON_FIELDS)
SLOT_EXISTS_END = LAYOUT["PokeParty.slotExists"] + LAYOUT["sizeof(PokeParty.slotExists)"]
# The party's fields in the clear: its capacity and count, the slots' flags, and the struct's padding after them
PARTY_OWN_FIELDS = (
    ("capacity", 8 * LAYOUT["PokeParty.capacity"], 8 * LAYOUT["sizeof(PokeParty.capacity)"]),
    ("count", 8 * LAYOUT["PokeParty.count"], 8 * LAYOUT["sizeof(PokeParty.count)"]),
    ("slotExists", 8 * LAYOUT["PokeParty.slotExists"], 8 * LAYOUT["sizeof(PokeParty.slotExists)"]),
    ("padding", 8 * SLOT_EXISTS_END, 8 * (POKE_PARTY_SIZE - SLOT_EXISTS_END)),
)
# The party's view: its capacity and count, its six Pokémon's, then the slots' flags and the padding
PARTY_FIELDS = PARTY_OWN_FIELDS[:2] + tuple(
    (f"pkms[{slot}].{path}", 8 * (POKE_PARTY_PKMS + slot * PARTY_PKM_SIZE) + bit, width)
    for slot in range(LAYOUT["PARTY_SIZE"]) for path, bit, width in POKEMON_FIELDS) + PARTY_OWN_FIELDS[2:]
# The records by their IDs, record 0x00 to record 0x46 the u32 words and record 0x47 to record 0xa6 the u16 halfwords
# (GetRecordCounter, src/system/record.c), then the checksum and seedHigh as stored
RECORD_WORD_COUNT = LAYOUT["sizeof(GameRecords.words)"] // 4
RECORD_FIELDS = tuple(
    [(f"record {record:#04x}", 8 * (LAYOUT["GameRecords.words"] + 4 * record), 32)
     for record in range(RECORD_WORD_COUNT)]
    + [(f"record {RECORD_WORD_COUNT + index:#04x}", 8 * (LAYOUT["GameRecords.halfwords"] + 2 * index), 16)
       for index in range(LAYOUT["sizeof(GameRecords.halfwords)"] // 2)]
    + [("checksum", 8 * RECORDS_CHECKSUM, 16), ("seedHigh", 8 * RECORDS_SEED_HIGH, 16)])


def _reader(fields: tuple[tuple[str, int, int], ...]) -> tuple[tuple[str, int, int, int, int], ...]:
    """Each field's bytes, shift and mask in a view, so that a view's fields read fast"""
    return tuple((path, bit // 8, (bit + width + 7) // 8, bit % 8, (1 << width) - 1) for path, bit, width in fields)


def _read(view: bytes, reader: tuple[tuple[str, int, int, int, int], ...]) -> dict[str, int]:
    return {path: int.from_bytes(view[first:end], "little") >> shift & mask for path, first, end, shift, mask in reader}


_POKEMON_READER = _reader(POKEMON_FIELDS)
_PARTY_OWN_READER = _reader(PARTY_OWN_FIELDS)
_RECORD_READER = _reader(RECORD_FIELDS)


def pokemon_plain(raw: bytes) -> tuple[bytes, list[str]]:
    """A PartyPkm (0xdc bytes as the save stores it) as the views read it: its head as stored, its blocks decrypted with
    its checksum and put in order A to D, and its own fields decrypted with its PID, as PokeParty_DecryptPkm decrypts
    them; a Pokémon stored decrypted (BoxPkm.partyDecrypted set) is read as it is. With the problem when its plain data
    doesn't add up to its checksum"""
    (pid,) = struct.unpack_from("<I", raw, BOX_PKM_PID)
    (checksum,) = struct.unpack_from("<H", raw, BOX_PKM_CHECKSUM)
    content = bytes(raw[BOX_PKM_CONTENT:BOX_PKM_CONTENT + PKM_BUFFER_SIZE])
    own = bytes(raw[PARTY_PKM_OWN:PARTY_PKM_SIZE])
    if not raw[PARTY_DECRYPTED_BIT // 8] >> PARTY_DECRYPTED_BIT % 8 & 1:
        content = crypt(content, checksum)
        own = crypt(own, pid)
    problems = []
    content_sum = pokemon_checksum(content)
    if content_sum != checksum:
        problems.append(f"its data adds up to {content_sum:#06x}, not to its checksum {checksum:#06x}")
    order = BLOCK_SHUFFLE[(pid & PID_SHUFFLE_MASK) >> PID_SHUFFLE_SHIFT]
    blocks = b"".join(content[at:at + PKM_BLOCK_SIZE] for at in order)
    return bytes(raw[:BOX_PKM_CONTENT]) + blocks + own, problems


def pokemon_fields(raw: bytes) -> tuple[dict[str, int], list[str]]:
    """Every field of one PartyPkm by its path (POKEMON_FIELDS), decoded as pokemon_plain decodes it, and its
    problems"""
    plain, problems = pokemon_plain(raw)
    return _read(plain, _POKEMON_READER), problems


def party_fields(block: bytes) -> tuple[dict[str, int], list[str]]:
    """Every field of a PokeParty (0x534 bytes) by its path (PARTY_FIELDS): its capacity and count, all six slots
    decoded, those past count too, which hold PokeParty_ClearPkm's Pokémon (zeros under a checksum of 0) when the game
    left them, the slots' flags and the padding; and the problems, each naming its slot"""
    own = _read(block, _PARTY_OWN_READER)
    fields = {"capacity": own["capacity"], "count": own["count"]}
    problems = []
    for slot in range(PARTY_SIZE):
        start = POKE_PARTY_PKMS + slot * PARTY_PKM_SIZE
        pokemon, found = pokemon_fields(block[start:start + PARTY_PKM_SIZE])
        fields.update((f"pkms[{slot}].{path}", value) for path, value in pokemon.items())
        problems += [f"pkms[{slot}]: {problem}" for problem in found]
    # The slots' flags and the padding, after the slots
    fields.update((name, value) for name, value in own.items() if name not in fields)
    return fields, problems


def records_fields(block: bytes) -> tuple[dict[str, int], list[str]]:
    """Every field of a GameRecords (0x1e0 bytes) by its name (RECORD_FIELDS), the records decrypted as
    DecryptRecordStorage decrypts them, record 0 in the clear, the checksum and seedHigh as stored; and the problem
    when the plain records don't add up to their checksum (EncryptRecordStorage's addUpDataStream)"""
    (checksum,) = struct.unpack_from("<H", block, RECORDS_CHECKSUM)
    (seed_high,) = struct.unpack_from("<H", block, RECORDS_SEED_HIGH)
    plain = bytearray(block)
    start, end = RECORDS_ENCRYPTED, RECORDS_ENCRYPTED + RECORDS_ENCRYPTED_SIZE
    plain[start:end] = crypt(bytes(block[start:end]), checksum + (seed_high << 16))
    problems = []
    records_sum = byte_sum(plain[start:end]) & 0xffff
    if records_sum != checksum:
        problems.append(f"they add up to {records_sum:#06x}, not to their checksum {checksum:#06x}")
    return _read(plain, _RECORD_READER), problems
