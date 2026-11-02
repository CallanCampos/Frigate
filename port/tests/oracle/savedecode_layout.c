// The layout of the save's party and game records that port/tools/oracle/savedecode.py decodes, as the decomp's headers
// give it: a line for each member of BoxPkm, PartyPkm, PkmBlockA to PkmBlockD, MailData, PMSData, PokeParty and
// GameRecords (its offset and its size, or for a bitfield its first bit from the struct's start and its width), each
// of those structs' sizes, the numbers the module counts with, and, for the round trips' declarations, the IDs of the
// main save's blocks it decodes or protects, their structs' sizes and the fields of them that a round trip may never
// declare by byte. "NAME VALUE" in hex, printed with offsetof and sizeof as the compiler lays the headers out.
// port/tests/oracle/test_savedecode.py holds savedecode's LAYOUT to this output, name for name, and builds its
// synthetic saves field by field from it. The host build compiles the headers as the game's own files (port_decomp_env),
// and these save structs hold no pointers, so they are laid out as on the DS: the saves Frigate writes are melonDS's byte
// for byte. Game functionality: the saves (the party, block 0x1a, and the trainer card's records, block 0x26, with the
// bag, the player's place, the mystery gifts, the story's flags and the money and badges); no game code runs.
#include "field/zone.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/event_work.h"
#include "save/mystery_gift.h"
#include "save/player_savedata.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/trainer_game_info.h"
#include "system/pms_data.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void Print(const char *name, unsigned long value) {
    printf("%s 0x%lx\n", name, value);
}

#define FIELD(type, member) Print(#type "." #member, (unsigned long)offsetof(type, member))
#define SIZE(type) Print("sizeof(" #type ")", (unsigned long)sizeof(type))
#define VALUE(name) Print(#name, (unsigned long)(name))

// A member's offset and its size, which for an array is all of its elements'
#define MEMBER(type, member)                                                                                           \
    do {                                                                                                               \
        FIELD(type, member);                                                                                           \
        Print("sizeof(" #type "." #member ")", (unsigned long)sizeof(((type *)0)->member));                            \
    } while (0)

// A bitfield's first bit from the struct's start (byte * 8 + bit, the bits of each byte counted from its lowest) and
// its width: the struct zeroed, the member set to all ones by a decrement from 0, and the bits that changed
#define BITS(type, member)                                                                                             \
    do {                                                                                                               \
        type object;                                                                                                   \
        memset(&object, 0, sizeof(object));                                                                            \
        object.member = 0;                                                                                             \
        object.member--;                                                                                               \
        PrintBits(#type "." #member, (const unsigned char *)&object, sizeof(object));                                  \
    } while (0)

static void PrintBits(const char *name, const unsigned char *bytes, size_t size) {
    char line[128];
    size_t first = 0;
    size_t width = 0;
    size_t bit;

    for (bit = 0; bit < size * 8; bit++) {
        if (bytes[bit / 8] & (1u << (bit % 8))) {
            if (width == 0) {
                first = bit;
            } else if (bit != first + width) {
                // A bitfield whose bits aren't contiguous in the little-endian order the module reads them in
                fprintf(stderr, "%s: its bits are not contiguous\n", name);
                exit(1);
            }
            width++;
        }
    }
    snprintf(line, sizeof(line), "%s.bit", name);
    Print(line, (unsigned long)first);
    snprintf(line, sizeof(line), "%s.width", name);
    Print(line, (unsigned long)width);
}

int main(void) {
    // The party (block 0x1a): its Pokémon, each a BoxPkm's clear head and encrypted blocks and the party's own fields
    SIZE(PokeParty);
    MEMBER(PokeParty, capacity);
    MEMBER(PokeParty, count);
    MEMBER(PokeParty, pkms);
    MEMBER(PokeParty, slotExists);
    VALUE(PARTY_SIZE);
    SIZE(PartyPkm);
    MEMBER(PartyPkm, base);
    MEMBER(PartyPkm, statusCond);
    MEMBER(PartyPkm, level);
    MEMBER(PartyPkm, unk8D);
    MEMBER(PartyPkm, nowHP);
    MEMBER(PartyPkm, maxHP);
    MEMBER(PartyPkm, atk);
    MEMBER(PartyPkm, def);
    MEMBER(PartyPkm, spe);
    MEMBER(PartyPkm, spa);
    MEMBER(PartyPkm, spd);
    MEMBER(PartyPkm, mail);
    MEMBER(PartyPkm, unkD4);
    MEMBER(PartyPkm, unkD8);
    SIZE(BoxPkm);
    MEMBER(BoxPkm, pid);
    BITS(BoxPkm, partyDecrypted);
    BITS(BoxPkm, boxDecrypted);
    BITS(BoxPkm, badEgg);
    BITS(BoxPkm, unk4_3);
    MEMBER(BoxPkm, checksum);
    MEMBER(BoxPkm, contentBuffer);
    SIZE(PkmBuffer);

    SIZE(PkmBlockA);
    MEMBER(PkmBlockA, species);
    MEMBER(PkmBlockA, item);
    MEMBER(PkmBlockA, id);
    MEMBER(PkmBlockA, exp);
    MEMBER(PkmBlockA, friendship);
    MEMBER(PkmBlockA, ability);
    MEMBER(PkmBlockA, markings);
    MEMBER(PkmBlockA, language);
    MEMBER(PkmBlockA, hpEV);
    MEMBER(PkmBlockA, atkEV);
    MEMBER(PkmBlockA, defEV);
    MEMBER(PkmBlockA, speEV);
    MEMBER(PkmBlockA, spaEV);
    MEMBER(PkmBlockA, spdEV);
    MEMBER(PkmBlockA, contest);
    MEMBER(PkmBlockA, ribbons);

    SIZE(PkmBlockB);
    MEMBER(PkmBlockB, moves);
    MEMBER(PkmBlockB, pp);
    MEMBER(PkmBlockB, ppUps);
    BITS(PkmBlockB, hpIV);
    BITS(PkmBlockB, atkIV);
    BITS(PkmBlockB, defIV);
    BITS(PkmBlockB, speIV);
    BITS(PkmBlockB, spaIV);
    BITS(PkmBlockB, spdIV);
    BITS(PkmBlockB, isEgg);
    BITS(PkmBlockB, isNicknamed);
    MEMBER(PkmBlockB, ribbons);
    BITS(PkmBlockB, fatefulEncounter);
    BITS(PkmBlockB, gender);
    BITS(PkmBlockB, form);
    MEMBER(PkmBlockB, nature);
    BITS(PkmBlockB, hiddenAbility);
    BITS(PkmBlockB, nPoke);
    BITS(PkmBlockB, unk1A_2);
    MEMBER(PkmBlockB, unk1C);

    SIZE(PkmBlockC);
    MEMBER(PkmBlockC, nickname);
    MEMBER(PkmBlockC, unk16);
    MEMBER(PkmBlockC, originGame);
    MEMBER(PkmBlockC, ribbons);

    SIZE(PkmBlockD);
    MEMBER(PkmBlockD, otName);
    MEMBER(PkmBlockD, eggDate);
    MEMBER(PkmBlockD, metDate);
    MEMBER(PkmBlockD, eggLocation);
    MEMBER(PkmBlockD, metLocation);
    MEMBER(PkmBlockD, pokerus);
    MEMBER(PkmBlockD, ball);
    BITS(PkmBlockD, metLevel);
    BITS(PkmBlockD, otGender);
    MEMBER(PkmBlockD, unk1D);
    MEMBER(PkmBlockD, unk1E);
    MEMBER(PkmBlockD, pokestarFame);

    // A Pokémon's mail, among the party's own fields, and its three sentences
    SIZE(MailData);
    MEMBER(MailData, trainerId);
    MEMBER(MailData, trainerGender);
    MEMBER(MailData, region);
    MEMBER(MailData, gameVersion);
    MEMBER(MailData, design);
    MEMBER(MailData, trainerName);
    MEMBER(MailData, unk18);
    MEMBER(MailData, stampWord);
    MEMBER(MailData, message);
    SIZE(PMSData);
    MEMBER(PMSData, type);
    MEMBER(PMSData, id);
    MEMBER(PMSData, words);
    VALUE(PMS_SENTENCE_WORD_MAX);

    // The trainer card's records (block 0x26): the counters, then the checksum and the seed's high half
    SIZE(GameRecords);
    MEMBER(GameRecords, words);
    MEMBER(GameRecords, halfwords);
    MEMBER(GameRecords, checksum);
    MEMBER(GameRecords, seedHigh);
    VALUE(RECORD_COUNT_WORD);
    VALUE(RECORD_COUNT);

    // The blocks the module decodes or protects, by their IDs, and their structs' sizes
    VALUE(SAVE_BLOCK_BAG);
    VALUE(SAVE_BLOCK_PARTY);
    VALUE(SAVE_BLOCK_PLAYER);
    VALUE(SAVE_BLOCK_MYSTERY_GIFT);
    VALUE(SAVE_BLOCK_RECORDS);
    VALUE(SAVE_BLOCK_EVENT_WORK);
    VALUE(SAVE_BLOCK_TRAINER_GAME_INFO);
    SIZE(BagSave);
    SIZE(PlayerSave);
    SIZE(MysteryGiftSaveData);
    SIZE(EventWork);
    SIZE(TrainerGameInfoSave);

    // What a round trip never declares by byte besides the party and the records: the player's four spawns and place,
    // the mystery gifts up to their checksum and key, and the money and badges (the bag and the event work whole)
    MEMBER(PlayerSave, nowSpawn);
    MEMBER(PlayerSave, unk1C);
    MEMBER(PlayerSave, nextSpawn);
    MEMBER(PlayerSave, escapeRopeSpawn);
    MEMBER(PlayerSave, location);
    MEMBER(MysteryGiftSaveData, checksum);
    MEMBER(MysteryGiftSaveData, key);
    MEMBER(TrainerGameInfoSave, cash);
    MEMBER(TrainerGameInfoSave, badges);
    return 0;
}
