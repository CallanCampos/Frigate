#include <stddef.h>
#include "types.h"
#include "save/gds_profile.h"
#include "constants/language.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "field/unity_tower.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/trainer_game_info.h"
#include "system/country_region.h"
#include "system/game_data.h"
#include "system/pms.h"
#include "system/pms_data.h"
#include "system/pms_word.h"
#include "system/str_tool.h"

// The profile of the player that a battle video or a musical photo sent to the Global Link carries

// The species and the forms' entries that a profile's Pokémon can be
#define GDS_PROFILE_SPECIES_END 686
#define GDS_PROFILE_SEX_MALE 0
#define GDS_PROFILE_SEX_FEMALE 1
#define GDS_PROFILE_UNION_VIEW_MAX 15
// The sentences an introduction can be: the first five types and their first twenty sentences
#define GDS_PROFILE_PMS_TYPE_END 5
#define GDS_PROFILE_PMS_ID_MAX 19
// The introduction that replaces a broken one
#define GDS_PROFILE_DEFAULT_PMS_TYPE 4
#define GDS_PROFILE_DEFAULT_PMS_WORD 29

static u8 getValidLanguage(GdsProfile *profile);

GdsProfile *GdsProfile_Create(u32 heapId) {
    GdsProfile *profile = GFL_HeapAllocate(heapId, sizeof(GdsProfile), FALSE, "gds_profile.c", 80);

    sys_memset(profile, 0, sizeof(GdsProfile));
    return profile;
}

void GdsProfile_Free(GdsProfile *profile) {
    GFL_HeapFree(profile);
}

void GdsProfile_SetFromGameData(GdsProfile *profile, GameData *gameData) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    UnityTowerSurveySave *survey = getUnityTower_SurveySaveBlkAddrress(save);
    TrainerGameInfoSave *gameInfo = getTrainerCardDataBlkAddress(gameData);
    void *sentences = CGearPhrases_GetBlock(save);
    OSOwnerInfo ownerInfo;
    int species;
    int form;
    int egg;
    int gender;
    int i;

    OS_GetOwnerInfo(&ownerInfo);
    GetFavePokemonData(gameInfo, &species, &form, &egg, &gender);
    sys_memset(profile, 0, sizeof(GdsProfile));
    wcharsncpy(GetPlayerName(playerInfo), profile->name, GDS_PROFILE_NAME_LENGTH);
    profile->trainerId = getIDAsUInt(playerInfo);
    profile->sex = getTrainerGender(playerInfo);
    profile->species = species;
    profile->form = form;
    profile->egg = egg;
    profile->gender = gender;
    profile->country = UnityTowerVisitor_GetCountry(playerInfo);
    profile->region = UnityTowerVisitor_GetProvince(playerInfo);
    for (i = 0; i < GDS_PROFILE_INTRODUCTION_LENGTH; i++) {
        profile->introduction.text[i] = GFL_StrBufGetTerminator();
    }
    CGearPhrases_Get(sentences, 0, &profile->introduction.pms);
    profile->birthMonth = ownerInfo.birthday.month;
    profile->unionView = func_02008bf4(playerInfo);
    profile->version = GAME_VERSION;
    profile->language = GAME_LANGUAGE;
    profile->crc = getCRC16(profile, offsetof(GdsProfile, crc));
}

StrBuf *GdsProfile_CreateNameStrBuf(GdsProfile *profile, u32 heapId) {
    StrBuf *name = GFL_StrBufCreate(15, heapId);

    GFL_StrBufLoadFixedString(name, profile->name, 15);
    return name;
}

u32 GdsProfile_GetSex(GdsProfile *profile) {
    if (profile->sex != GDS_PROFILE_SEX_MALE && profile->sex != GDS_PROFILE_SEX_FEMALE) {
        return GDS_PROFILE_SEX_MALE;
    }
    return profile->sex;
}

u32 GdsProfile_GetSpecies(GdsProfile *profile) {
    if (profile->species >= GDS_PROFILE_SPECIES_END) {
        return 0;
    }
    return profile->species;
}

u32 GdsProfile_GetForm(GdsProfile *profile) {
    if (profile->species >= GDS_PROFILE_SPECIES_END) {
        return 0;
    }
    return PML_PkmGetSpriteForme(profile->species, profile->form);
}

u32 GdsProfile_GetGender(GdsProfile *profile) {
    if (profile->gender != 0 && profile->gender != 1) {
        return 0;
    }
    return profile->gender;
}

BOOL GdsProfile_IsEgg(GdsProfile *profile) {
    if (profile->egg > TRUE) {
        return TRUE;
    }
    return profile->egg;
}

u32 GdsProfile_GetCountry(GdsProfile *profile) {
    return Country_GetValidCountry(profile->country, profile->region, getValidLanguage(profile));
}

u32 GdsProfile_GetRegion(GdsProfile *profile) {
    return Country_GetValidRegion(profile->country, profile->region, getValidLanguage(profile));
}

StrBuf *GdsProfile_GetIntroduction(GdsProfile *profile, PMSData *pms, u32 heapId) {
    int errors = 0;

    if (!profile->introductionIsText) {
        u32 category;
        u32 index;

        *pms = profile->introduction.pms;
        if (pms->type >= GDS_PROFILE_PMS_TYPE_END) {
            errors++;
        } else if (pms->id > GDS_PROFILE_PMS_ID_MAX) {
            errors++;
        } else if ((pms->words[0] != PMS_WORD_NULL && !PMSWord_GetMessage(pms->words[0], &category, &index)) ||
                   (pms->words[1] != PMS_WORD_NULL && !PMSWord_GetMessage(pms->words[1], &category, &index))) {
            errors++;
        }
        if (errors > 0) {
            PMSData_Init(pms, GDS_PROFILE_DEFAULT_PMS_TYPE);
            pms->id = 0;
            pms->words[0] = PMSWord_FromMessage(TEXT_BANK_PMS_WORDS_TRAINER, GDS_PROFILE_DEFAULT_PMS_WORD);
            pms->words[1] = PMS_WORD_NULL;
        }
        return NULL;
    } else {
        StrBuf *text = GFL_StrBufCreate(GDS_PROFILE_INTRODUCTION_LENGTH, heapId);

        GFL_StrBufLoadFixedString(text, profile->introduction.text, GDS_PROFILE_INTRODUCTION_LENGTH);
        return text;
    }
}

u32 GdsProfile_GetBirthMonth(GdsProfile *profile) {
    if (profile->birthMonth < 1 || profile->birthMonth > 12) {
        return 1;
    }
    return profile->birthMonth;
}

u32 GdsProfile_GetUnionView(GdsProfile *profile) {
    if (profile->unionView > GDS_PROFILE_UNION_VIEW_MAX) {
        return 0;
    }
    return profile->unionView;
}

// The language of the game that made the profile, or English if it isn't one
static u8 getValidLanguage(GdsProfile *profile) {
    switch (profile->language) {
        case LANGUAGE_JAPANESE:
        case LANGUAGE_ENGLISH:
        case LANGUAGE_FRENCH:
        case LANGUAGE_ITALIAN:
        case LANGUAGE_GERMAN:
        case LANGUAGE_SPANISH:
        case LANGUAGE_KOREAN:
            return profile->language;
    }
    return LANGUAGE_ENGLISH;
}
