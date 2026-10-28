#ifndef POKEBW2_SAVE_MISC_SAVEDATA_H
#define POKEBW2_SAVE_MISC_SAVEDATA_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "save/hollow_rival_savedata.h"
#include "struct_decls.h"

// Block 0x42, a block of several small things: the trainers met last, a set of flags, the rival's name and the
// Hidden Hollows (misc_savedata.c, a guessed name). The other functions of the file are in save/save_control.h,
// field/rival_select.h and field/union_oneself.h.

#define MISC_MET_TRAINER_MAX 32
#define MISC_FLAG_COUNT 256
#define MISC_RIVAL_NAME_LENGTH 8
#define HIDDEN_HOLLOW_COUNT 20

// What a Hidden Hollow holds
typedef struct {
    u16 filled : 1;
    u16 slot : 4;
    u16 group : 3;
    u16 gender : 7;
} HiddenHollow;

struct RivalDataSave {
    // The IDs of the trainers met last, the newest last
    u32 metTrainers[MISC_MET_TRAINER_MAX];
    u32 metTrainerCount;
    u32 flags[MISC_FLAG_COUNT / 32];
    u16 rivalName[MISC_RIVAL_NAME_LENGTH];
    // What getHollow_RivalBlk gives
    HollowRivalSave hollowRival;
    HiddenHollow hollows[HIDDEN_HOLLOW_COUNT];
    u8 hollowNum;
    u8 paddingF5[3];
    u32 unkF8;
};

// The block's size and its initial state
u32 getSizeofHollow_RivalData(void);
void clearHollowRivalData(RivalDataSave *data);

BOOL Misc_GetFlag(RivalDataSave *data, int flag);
HollowRivalSave *Misc_GetHollowRival(RivalDataSave *data);
void Misc_SetFlag(RivalDataSave *data, int flag);

// The kind of thing in a Hidden Hollow's slot: nothing, a Pokémon, an item in a ball, or a hidden item
enum {
    HOLLOW_SLOT_TYPE_EMPTY,
    HOLLOW_SLOT_TYPE_POKEMON,
    HOLLOW_SLOT_TYPE_ITEM,
    HOLLOW_SLOT_TYPE_HIDDEN_ITEM,
};

// The kind of thing in a Hidden Hollow, a HOLLOW_SLOT_TYPE_ value
u32 getTypeOfFilledSlot(RivalDataSave *data, u32 hollow);
void setSlot(RivalDataSave *data, u32 hollow, u16 slot);
void setGroup(RivalDataSave *data, u32 hollow, u16 group);
void setHollowStatus(RivalDataSave *data, u32 hollow, BOOL filled);
BOOL isHollowFilled(RivalDataSave *data, u8 hollow);
u8 getSlotNumber(RivalDataSave *data, u32 hollow);
u8 getGroupNumber(RivalDataSave *data, u32 hollow);
u8 getHollowGender(RivalDataSave *data, u32 hollow);
void setHollowGender(RivalDataSave *data, u32 hollow, u16 gender);
void setHollowNum(RivalDataSave *data, u8 num);

#endif // POKEBW2_SAVE_MISC_SAVEDATA_H
