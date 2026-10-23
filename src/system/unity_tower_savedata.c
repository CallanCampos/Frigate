#include "types.h"
#include "field/unity_tower.h"
#include "nitro/mi.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/country_region.h"

// The Unity Tower's survey: the trainers met through trades and the GTS, and the countries and regions they came
// from, which open the tower's floors. The ROM has no string for the file, so the name is a guess (swan's
// getUnityTower_SurveySaveBlkAddrress). Block 0x1d.

static void setForeignRegionMet(UnityTowerSurveySave *save, u8 met);

u32 UnityTower_GetSurveySaveSize(void) {
    return sizeof(UnityTowerSurveySave);
}

void UnityTower_InitSurveySave(UnityTowerSurveySave *save) {
    MIi_CpuClear32(0, save, sizeof(UnityTowerSurveySave));
}

UnityTowerSurveySave *getUnityTower_SurveySaveBlkAddrress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_UNITY_TOWER_SURVEY);
}

// Makes the player's own country and region, recorded as UNITY_TOWER_REGION_OWN
void UnityTower_SetOwnRegion(UnityTowerSurveySave *save, PlayerInfo *info, int country, int region) {
    PlayerInfo_SetCountryAndProvince(info, country, region);
    UnityTower_SetRegionState(save, country, region, UNITY_TOWER_REGION_OWN);
}

u32 UnityTower_GetRegionState(UnityTowerSurveySave *save, int country, int region) {
    int r = region;
    if (country == COUNTRY_NONE) {
        return UNITY_TOWER_REGION_NONE;
    }
    return (save->regions[r / 4 + (country - 1) * 16] >> ((r % 4) * 2)) & 3;
}

void UnityTower_SetRegionState(UnityTowerSurveySave *save, int country, int region, u32 state) {
    s32 r = region;
    u8 *byte;
    u32 shift;
    u8 clear;
    u8 set;

    if (country == COUNTRY_NONE) {
        return;
    }
    byte = &save->regions[r / 4 + (country - 1) * 16];
    shift = (r % 4) * 2;
    clear = (3 << shift) ^ 0xff;
    set = state << shift;
    *byte &= clear;
    *byte |= set;
    if (country != COUNTRY_JAPAN) {
        setForeignRegionMet(save, TRUE);
    }
}

u8 UnityTower_HasMetForeignRegion(UnityTowerSurveySave *save) {
    return save->foreignRegionMet;
}

static void setForeignRegionMet(UnityTowerSurveySave *save, u8 met) {
    save->foreignRegionMet = met;
}

// A day has passed: every region recorded today becomes one recorded before
void UnityTower_AgeTodayRegions(UnityTowerSurveySave *save) {
    s32 i;
    s32 shift;

    for (i = 0; i < UNITY_TOWER_REGION_BYTES; i++) {
        u8 byte = save->regions[i];
        for (shift = 0; shift < 8; shift += 2) {
            if (((byte >> shift) & 3) == UNITY_TOWER_REGION_TODAY) {
                byte &= (u8)((3 << shift) ^ 0xff);
                byte |= (u8)(UNITY_TOWER_REGION_BEFORE << shift);
            }
        }
        save->regions[i] = byte;
    }
}

u8 UnityTower_GetCountryCount(UnityTowerSurveySave *save) {
    return save->countryCount;
}

u8 UnityTower_GetCountryCountForPlayer(UnityTowerSurveySave *save, PlayerInfo *info, BOOL withOwnCountry) {
    u8 count = UnityTower_GetCountryCount(save);

    if (!withOwnCountry) {
        u8 country = UnityTowerVisitor_GetCountry(info);
        if (country == COUNTRY_NONE) {
            count = 0;
        } else if (UnityTower_HasMetCountry(save, country) && count != 0) {
            count--;
        }
    }
    return count;
}

u8 UnityTower_CountVisitors(UnityTowerSurveySave *save) {
    s32 i;

    for (i = 0; i < UNITY_TOWER_VISITOR_MAX; i++) {
        if (!save->visitors[i].valid) {
            break;
        }
    }
    return i;
}

void UnityTower_IncrementCountryCount(UnityTowerSurveySave *save) {
    if (save->countryCount < COUNTRY_CODE_COUNT) {
        save->countryCount++;
    }
}

void UnityTower_SetRatingLevel(UnityTowerSurveySave *save, s32 value) {
    if (value < 5) {
        save->unk347 = value;
    }
}

u8 UnityTower_GetRatingLevel(UnityTowerSurveySave *save) {
    return save->unk347;
}

void setPlayerSurveys(UnityTowerSurveySave *save, s32 hobby) {
    if (hobby < 9) {
        save->hobby = hobby;
    }
}

u8 getPlayerSurveys(UnityTowerSurveySave *save) {
    return save->hobby;
}

u32 UnityTower_GetVisitorParam(UnityTowerSurveySave *save, u32 index, u32 param) {
    UnityTowerVisitor *visitor;

    if (index >= UNITY_TOWER_VISITOR_MAX) {
        return 0;
    }
    visitor = &save->visitors[index];
    switch (param) {
    case UNITY_TOWER_VISITOR_PARAM_SENT_SPECIES:
        return visitor->sentSpecies;
    case UNITY_TOWER_VISITOR_PARAM_RECEIVED_SPECIES:
        return visitor->receivedSpecies;
    case UNITY_TOWER_VISITOR_PARAM_HOBBY:
        return visitor->hobby;
    case UNITY_TOWER_VISITOR_PARAM_COUNTRY_COUNT:
        return visitor->countryCount;
    case UNITY_TOWER_VISITOR_PARAM_UNK26_0:
        return visitor->unk26_0;
    case UNITY_TOWER_VISITOR_PARAM_SCRIPT_FLAG:
        return visitor->scriptFlag;
    case UNITY_TOWER_VISITOR_PARAM_VALID:
        return visitor->valid;
    }
    return 0;
}

void UnityTower_SetVisitorParam(UnityTowerSurveySave *save, int index, u32 param, u32 value) {
    UnityTowerVisitor *visitor;

    if (index >= UNITY_TOWER_VISITOR_MAX) {
        return;
    }
    visitor = &save->visitors[index];
    switch (param) {
    case UNITY_TOWER_VISITOR_PARAM_SENT_SPECIES:
    case UNITY_TOWER_VISITOR_PARAM_RECEIVED_SPECIES:
    case UNITY_TOWER_VISITOR_PARAM_HOBBY:
    case UNITY_TOWER_VISITOR_PARAM_COUNTRY_COUNT:
        break;
    case UNITY_TOWER_VISITOR_PARAM_UNK26_0:
        if (value < 5) {
            visitor->unk26_0 = value;
        }
        break;
    case UNITY_TOWER_VISITOR_PARAM_SCRIPT_FLAG:
        if (value != 0) {
            visitor->scriptFlag = 1;
        } else {
            visitor->scriptFlag = 0;
        }
        break;
    case UNITY_TOWER_VISITOR_PARAM_VALID:
        if (value != 0) {
            visitor->valid = 1;
        } else {
            visitor->valid = 0;
        }
        break;
    }
}

PlayerInfo *UnityTower_GetVisitor(UnityTowerSurveySave *save, int index) {
    if (index >= UNITY_TOWER_VISITOR_MAX) {
        return NULL;
    }
    return &save->visitors[index].info;
}

UnityTowerVisitor *UnityTower_GetVisitors(UnityTowerSurveySave *save) {
    return save->visitors;
}

BOOL UnityTower_MarkCountryMet(UnityTowerSurveySave *save, u32 country) {
    BOOL isNew = FALSE;

    if (country == COUNTRY_NONE) {
        return FALSE;
    }
    if (country < COUNTRY_CODE_COUNT) {
        if (!UnityTower_HasMetCountry(save, country)) {
            isNew = TRUE;
        }
        save->countriesMet[country >> 3] |= (u8)(1 << (u8)(country & 7));
    }
    return isNew;
}

BOOL UnityTower_HasMetCountry(UnityTowerSurveySave *save, u32 country) {
    if (country < COUNTRY_CODE_COUNT) {
        if ((u8)((save->countriesMet[country >> 3] >> (u8)(country & 7)) & 1)) {
            return TRUE;
        }
    }
    return FALSE;
}
