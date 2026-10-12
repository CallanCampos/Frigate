#ifndef POKEBW2_SAVE_MEDAL_BOX_H
#define POKEBW2_SAVE_MEDAL_BOX_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

// The Medal Rally's medals, save block 0x44

#define MEDAL_COUNT 255

// What the player knows of a medal
enum {
    MEDAL_STATUS_UNKNOWN,
    MEDAL_STATUS_1,
    // Shown as a hint
    MEDAL_STATUS_DISCOVERED,
    // Earned, but Mr. Medal hasn't handed it over yet
    MEDAL_STATUS_EARNED,
    MEDAL_STATUS_OBTAINED,
};

// A medal: the day it was obtained, as a year from 2000, and what the player knows of it
typedef struct {
    u16 year : 7;
    u16 month : 4;
    u16 day : 5;
    u16 status : 3;
    // Set when the medal is hinted at or obtained, and cleared by the medal box's screen: a medal not looked at yet
    u16 special : 1;
} MedalInfo;

struct MedalBox {
    MedalInfo medals[MEDAL_COUNT];
    // The medal obtained last, or 0xff
    u8 lastMedal;
    u8 rank;
    u8 unk3FE;
    u8 padding3FF;
    // The Pokédex's habitat list
    u8 habitatList[0x96];
};

#define MEDAL_RANK_MAX 4

// The block's size and its initial state
void MedalBox_Init(MedalBox *box);
u32 MedalBox_GetBlockSize(void);

u8 MedalBox_GetMedalStatus(MedalBox *box, u16 medal);
void MedalBox_GiveMedal(MedalBox *box, u16 medal);
void MedalBox_DiscoverMedal(MedalBox *box, u16 medal);
void MedalBox_AcknowledgeMedal(MedalBox *box, u16 medal, u8 year, u8 month, u8 day);
void MedalBox_DiscoverInitialMedal(MedalBox *box, u16 medal, u8 year, u8 month, u8 day);
void MedalBox_SetMedalNotSpecial(MedalBox *box, u16 medal);
void MedalBox_SetLastMedal(MedalBox *box, u8 medal);
void MedalBox_IncrementRank(MedalBox *box);
// The medal obtained last, or 0xff
u8 MedalBox_GetLastMedal(MedalBox *box);
u8 MedalBox_GetRank(MedalBox *box);
BOOL MedalBox_IsMedalSpecial(MedalBox *box, u16 medal);
void MedalBox_GetMedalDate(MedalBox *box, u16 medal, u8 *year, u8 *month, u8 *day);
// The number of medals needed for the next rank
u32 MedalBox_GetNextRankRequirement(MedalBox *box);
// The medals obtained, and with countEarned also those hinted at and those earned on a date
u32 MedalBox_GetObtainedCount(MedalBox *box, BOOL countEarned);
u8 func_0200fb28(MedalBox *box);
void func_0200fb34(MedalBox *box);
MedalBox *SaveControl_GetMedalBox(SaveControl *save);
// The Pokédex's habitat list in the block
void *MedalBox_GetHabitatList(MedalBox *box);

// The Pokédex's habitat list, kept in the medal box's block: the zone it shows, its season and its kind of habitat.
// These functions come from a later file than the medal box's, which has no header yet
void func_02010ca0(void *data);
void *func_02010cb8(SaveControl *save);
void func_02010d70(void *data, u16 zone);
u16 func_02010d78(void *data);
void func_02010d80(void *data, u32 season);
u8 func_02010d88(void *data);
void func_02010d90(void *data, u8 type);
u8 func_02010d98(void *data);
// What the player has done of a habitat's kind of habitat
u8 func_02010cc4(void *data, u8 habitat, u8 type);

#endif // POKEBW2_SAVE_MEDAL_BOX_H
