#ifndef POKEBW2_SAVE_GDS_PROFILE_H
#define POKEBW2_SAVE_GDS_PROFILE_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/pms_data.h"

// gds_profile.c: the profile of the player that a battle video or a musical photo sent to the Global Link carries

#define GDS_PROFILE_NAME_LENGTH 8
// The length of an introduction in free text
#define GDS_PROFILE_INTRODUCTION_LENGTH 40

struct GdsProfile {
    u16 name[GDS_PROFILE_NAME_LENGTH];
    u32 trainerId;
    u8 sex;
    u8 birthMonth;
    // The trainer's appearance in the Union Room
    u8 unionView;
    u8 country;
    u8 region;
    u8 version;
    u8 language;
    // The Pokémon shown with the profile
    u8 egg : 1;
    u8 form : 6;
    u8 gender : 1;
    u16 species;
    // Whether the introduction is free text instead of a sentence
    u8 introductionIsText;
    u8 padding1F;
    union {
        PMSData pms;
        u16 text[GDS_PROFILE_INTRODUCTION_LENGTH];
    } introduction;
    u8 unk70[0xc];
    // The checksum of everything before it
    u16 crc;
    u16 padding7E;
};

// A new profile, with its fields cleared, and the function that frees it
GdsProfile *GdsProfile_Create(u32 heapId);
void GdsProfile_Free(GdsProfile *profile);
// Fills the profile from the player's game data
void GdsProfile_SetFromGameData(GdsProfile *profile, GameData *gameData);
// A new string with the player's name
StrBuf *GdsProfile_CreateNameStrBuf(GdsProfile *profile, u32 heapId);
// The player's sex, or 0 if it isn't valid
u32 GdsProfile_GetSex(GdsProfile *profile);
// The Pokémon shown with the profile: its species, form, gender and whether it is an egg
u32 GdsProfile_GetSpecies(GdsProfile *profile);
u32 GdsProfile_GetForm(GdsProfile *profile);
u32 GdsProfile_GetGender(GdsProfile *profile);
BOOL GdsProfile_IsEgg(GdsProfile *profile);
// Where the player lives
u32 GdsProfile_GetCountry(GdsProfile *profile);
u32 GdsProfile_GetRegion(GdsProfile *profile);
// The player's introduction: a new string if it is free text, or NULL with the sentence written to pms
StrBuf *GdsProfile_GetIntroduction(GdsProfile *profile, PMSData *pms, u32 heapId);
// The month of the player's birthday
u32 GdsProfile_GetBirthMonth(GdsProfile *profile);
// The trainer's appearance in the Union Room
u32 GdsProfile_GetUnionView(GdsProfile *profile);

#endif // POKEBW2_SAVE_GDS_PROFILE_H
