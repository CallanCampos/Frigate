#include "types.h"
#include "save/hollow_rival_savedata.h"
#include "save/misc_savedata.h"
#include "save/save_control.h"

// A part of save block 0x42, which the script commands of scrcmd_hollow_rival.c read and write: a value and a flag
// of each of eight indices, a counter, three flags and a value. The ROM has no string for the file, so the name is a
// guess after swan's getHollow_RivalBlk.

void *getHollow_RivalBlk(SaveControl *save) {
    return Misc_GetHollowRival(SaveControl_GetBlockPtr(save, SAVE_BLOCK_MISC));
}

void HollowRivalSave_SetValue(HollowRivalSave *hollowRival, u32 index, u16 value) {
    hollowRival->values[index] = value;
}

u16 HollowRivalSave_GetValue(HollowRivalSave *hollowRival, u16 index) {
    return hollowRival->values[index];
}

void HollowRivalSave_SetFlag(HollowRivalSave *hollowRival, u16 index, u32 value) {
    hollowRival->flags[index] = value;
}

u8 HollowRivalSave_GetFlag(HollowRivalSave *hollowRival, u16 index) {
    return hollowRival->flags[index];
}

void HollowRivalSave_SetMeeting(HollowRivalSave *hollowRival, u32 value) {
    hollowRival->meeting = value;
}

void HollowRivalSave_ClearMeeting(HollowRivalSave *hollowRival) {
    hollowRival->meeting = 0;
}

u16 HollowRivalSave_GetMeeting(HollowRivalSave *hollowRival) {
    return hollowRival->meeting;
}

void HollowRivalSave_SetMeetingToday(HollowRivalSave *hollowRival, u32 value) {
    hollowRival->meetingToday = value;
}

BOOL HollowRivalSave_IsMeetingToday(HollowRivalSave *hollowRival) {
    return hollowRival->meetingToday;
}

void HollowRivalSave_SetHasSpecies(HollowRivalSave *hollowRival, u32 value) {
    hollowRival->hasSpecies = value;
}

BOOL HollowRivalSave_HasSpecies(HollowRivalSave *hollowRival) {
    return hollowRival->hasSpecies;
}

void HollowRivalSave_SetSpecies(HollowRivalSave *hollowRival, u16 value) {
    hollowRival->species = value;
}

u16 HollowRivalSave_GetSpecies(HollowRivalSave *hollowRival) {
    return hollowRival->species;
}

void HollowRivalSave_SetMet(HollowRivalSave *hollowRival, u32 value) {
    hollowRival->met = value;
}

BOOL HollowRivalSave_IsMet(HollowRivalSave *hollowRival) {
    return hollowRival->met;
}

BOOL func_0200ffd4(HollowRivalSave *hollowRival, int index) {
    // BUG: the mask is whether the bit's number isn't 0, not the bit, so this is bit 0 of the byte for every index
    // but the first of each byte, which gives FALSE
#ifdef BUGFIX
    return (hollowRival->unk14[index / 8] >> (index % 8)) & 1;
#else
    return hollowRival->unk14[index / 8] & (index % 8 != 0);
#endif
}
