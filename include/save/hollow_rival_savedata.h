#ifndef POKEBW2_SAVE_HOLLOW_RIVAL_SAVEDATA_H
#define POKEBW2_SAVE_HOLLOW_RIVAL_SAVEDATA_H

#include "types.h"
#include "struct_decls.h"

// The data 0xb4 bytes into save block 0x42 (hollow_rival_savedata.c, a guessed name after swan's
// getHollow_RivalBlk). The other functions of the file are in save/save_control.h

#define HOLLOW_RIVAL_INDEX_COUNT 8

struct HollowRivalSave {
    u8 values[HOLLOW_RIVAL_INDEX_COUNT];
    u8 flags[HOLLOW_RIVAL_INDEX_COUNT];
    // The meeting the secret caller arranged (2: the Ferris wheel in Nimbasa City), and whether it was arranged today
    u8 meeting;
    u8 meetingToday : 1;
    // Whether the scripts stored a species, and whether the meeting has happened
    u8 hasSpecies : 1;
    u8 met : 1;
    u16 species;
    u8 unk14[4];
};

u8 HollowRivalSave_GetFlag(HollowRivalSave *hollowRival, u16 index);
void HollowRivalSave_SetMeeting(HollowRivalSave *hollowRival, u32 value);
BOOL HollowRivalSave_IsMeetingToday(HollowRivalSave *hollowRival);
BOOL HollowRivalSave_HasSpecies(HollowRivalSave *hollowRival);
u16 HollowRivalSave_GetSpecies(HollowRivalSave *hollowRival);
BOOL HollowRivalSave_IsMet(HollowRivalSave *hollowRival);

#endif // POKEBW2_SAVE_HOLLOW_RIVAL_SAVEDATA_H
