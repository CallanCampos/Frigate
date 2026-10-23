#ifndef POKEBW2_SAVE_TOURNAMENT_BLOCK_H
#define POKEBW2_SAVE_TOURNAMENT_BLOCK_H

#include "types.h"
#include "battle/regulation.h"
#include "pml/poke_party.h"
#include "struct_decls.h"

// The save block that swan calls the tournament block: two entries of 0x160 bytes, each a regulation with the
// competition's data after it, whose fields a number picks, and two records of the teams entered

#define TOURNAMENT_ENTRY_MAX 2
#define TOURNAMENT_TEAM_MAX 6

// The fields of an entry, for TournamentEntry_GetField and TournamentEntry_SetField
enum {
    TOURNAMENT_FIELD_VERSION_MASK = 1,
    TOURNAMENT_FIELD_CUP_ID,
    TOURNAMENT_FIELD_UNK154,
    TOURNAMENT_FIELD_START_YEAR,
    TOURNAMENT_FIELD_START_MONTH,
    TOURNAMENT_FIELD_START_DAY,
    TOURNAMENT_FIELD_END_YEAR,
    TOURNAMENT_FIELD_END_MONTH,
    TOURNAMENT_FIELD_END_DAY,
    TOURNAMENT_FIELD_UNK15B,
    TOURNAMENT_FIELD_VARIANT,
    TOURNAMENT_FIELD_ALLOW_REMATCH
};

// The fields of a team's record, for TournamentTeam_GetField
enum { TOURNAMENT_TEAM_SPECIES, TOURNAMENT_TEAM_FORM, TOURNAMENT_TEAM_HAS_ITEM, TOURNAMENT_TEAM_GENDER };

struct TournamentEntry {
    Regulation regulation;
    // The game versions that may enter, a bit for each
    u32 versionMask;
    // The start of the competition's card, which the card info copies
    u8 cardData[0x92];
    s16 cupId;
    s8 unk154;
    // When the competition starts and ends
    s8 startYear;
    s8 startMonth;
    s8 startDay;
    s8 endYear;
    s8 endMonth;
    s8 endDay;
    // Field 10 of TournamentEntry_GetField
    s8 unk15B;
    // What picks the card's colors, and whether an opponent already battled may be battled again
    u8 variant;
    u8 allowRematch;
    // getCRC16 of everything before it
    u16 crc;
}; // size: 0x160

// The Pokémon of a team entered
typedef struct {
    u16 species[TOURNAMENT_TEAM_MAX];
    u8 form[TOURNAMENT_TEAM_MAX];
    // A bit for each Pokémon: whether it holds an item, and its gender
    u8 hasItem;
    u8 gender;
    u8 unk14[0xa];
} TournamentTeam; // size: 0x1e

struct TournamentBlock {
    TournamentEntry entries[TOURNAMENT_ENTRY_MAX];
    TournamentTeam teams[TOURNAMENT_ENTRY_MAX];
}; // size: 0x2fc

u32 getSizeofTourneyBlock(void);
// An entry's regulation, which is the entry itself, and a parameter of the regulation. The battle type is 0x15
Regulation *TournamentEntry_GetRegulation(void *entry);
// The entry's data of 0x92 bytes
void *TournamentEntry_GetCardData(TournamentEntry *entry);
// An entry's field, and setting it. The Wi-Fi login reads field 2 and field 10 and sets field 10
u32 TournamentEntry_GetField(void *entry, u32 field);
void TournamentEntry_SetField(void *entry, u32 field, u32 value);
// Checks the CRC of an entry
BOOL TournamentEntry_CheckCRC(TournamentEntry *entry);
// The CRC16 of the first 0x15e bytes of an entry
u16 CRC16_350bytes(const void *entry);
// An entry of the block
void *TournamentBlock_GetEntry(TournamentBlock *block, u32 index);
// Copies an entry into the block
void TournamentBlock_SetEntry(TournamentBlock *block, u32 index, const TournamentEntry *entry);
// Clears a team's record
void TournamentTeam_Clear(void *record);
// The block's other entries, of 0x1e bytes
void *TournamentBlock_GetTeam(TournamentBlock *block, u32 index);
// A field of the Pokémon number index of such an entry: the species (0), the form (1), whether it holds an item (2)
// and its gender (3)
u32 TournamentTeam_GetField(void *record, u32 field, u8 index);
// Fills a team's record from a party
void TournamentTeam_SetFromParty(void *record, PokeParty *party);
void clearTournamentBlock(void *block);
void *getTournamentBlockAddress(SaveControl *save);
// Clears the player's VS data
void clearVSPlayer(void);

#endif // POKEBW2_SAVE_TOURNAMENT_BLOCK_H
