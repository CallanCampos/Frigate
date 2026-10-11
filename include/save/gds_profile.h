#ifndef POKEBW2_SAVE_GDS_PROFILE_H
#define POKEBW2_SAVE_GDS_PROFILE_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// gds_profile.c: the profile of the player that a battle video or a musical photo sent to the Global Link carries

// A new profile, with its fields cleared, and the function that frees it
GdsProfile *func_0200de34(HeapID heapId);
void func_0200de60(GdsProfile *profile);
// Fills the profile from the player's game data
void func_0200de68(GdsProfile *profile, GameData *gameData);
// A new string with the player's name
StrBuf *func_0200df68(GdsProfile *profile, HeapID heapId);
// The player's sex, or 0 if it isn't valid
u32 func_0200df84(GdsProfile *profile);
// The Pokémon shown with the profile: its species, form, gender and whether it is an egg
u32 func_0200df94(GdsProfile *profile);
u32 func_0200dfa4(GdsProfile *profile);
u32 func_0200dfc4(GdsProfile *profile);
BOOL func_0200dfd4(GdsProfile *profile);
// Where the player lives
u32 func_0200dfe4(GdsProfile *profile);
u32 func_0200dff8(GdsProfile *profile);
// The player's introduction: a new string if it is free text, or NULL with the sentence written to pms
StrBuf *func_0200e00c(GdsProfile *profile, PMSData *pms, HeapID heapId);
// The month of the player's birthday
u32 func_0200e0a8(GdsProfile *profile);
// The trainer's appearance in the Union Room
u32 func_0200e0b8(GdsProfile *profile);

#endif // POKEBW2_SAVE_GDS_PROFILE_H
