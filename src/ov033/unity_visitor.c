#include "types.h"
#include "field/unity_tower.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "struct_decls.h"
#include "system/aeabi.h"
#include "system/country_region.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/union_view.h"
#include "system/wordset.h"

void Scrcmd_FillPartnerCandidates(GameSystem *gsys, s32 floor, u32 value) {
    GameData *gameData;
    UnityTowerFloor *unityTowerSave;
    SaveControl *saveControl;

    gameData = GSYS_GetGameData(gsys);
    unityTowerSave = GameData_GetUnityTowerSave(gameData);
    if (floor < 0xe9) {
        saveControl = GameData_GetSaveControl(gameData);
        Scrcmd_CollectPartnersByLanguage(getUnityTower_SurveySaveBlkAddrress(saveControl), unityTowerSave, floor,
                                         value);
    } else {
        UnityTowerSave_Init(unityTowerSave);
    }
}

void Scrcmd_CollectPartnersByLanguage(UnityTowerSurveySave *survey, UnityTowerFloor *output, s32 floor, u32 value) {
    PlayerInfo *visitor;
    u32 visitorValue;
    s32 country;
    s32 index;

    output->floor = floor;
    output->value = value;
    output->count = 0;
    for (index = 0; index < 20; index++) {
        if ((u8)UnityTower_GetVisitorParam(survey, index, UNITY_TOWER_VISITOR_PARAM_VALID) == 0) {
            continue;
        }
        visitor = UnityTower_GetVisitor(survey, index);
        if (visitor == NULL) {
            continue;
        }
        country = UnityTowerVisitor_GetCountry(visitor);
        if (country == floor) {
            visitorValue = PlayerInfo_GetUnionView(visitor);
            if (visitorValue > 15) {
                visitorValue = getTrainerGender(visitor) != 0 ? 15 : 11;
            }
            output->visitors[output->count] = index;
            output->trainerClasses[output->count] = UnionView_GetTrainerType(visitorValue);
            output->count++;
            if (output->count >= 5) {
                break;
            }
        }
    }
}

u32 Scrcmd_GetPartnerCandidateHistoryIndex(UnityTowerFloor *output, u32 index) {
    u32 result;

    result = UNITY_TOWER_HISTORY_INDEX_NONE;
    if (index < 5 && index < output->count) {
        result = output->visitors[index];
    }
    return result;
}

u32 Scrcmd_GetPartnerGreetingMessage(WordSet *wordSet, GameSystem *gsys, UnityTowerFloor *save, s32 index, u32 param) {
    GameData *gameData;
    UnityTowerSurveySave *surveySave;
    PlayerInfo *visitor;
    u32 visitorIndex;
    u32 gender;
    u32 id;
    u32 province;
    u32 hasProvince;

    hasProvince = 0;
    gameData = GSYS_GetGameData(gsys);
    surveySave = getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(gameData));
    if (index >= 5) {
        return 4;
    }
    visitorIndex = save->visitors[index];
    LoadUnityTowerVisitorWordSet(wordSet, gameData, visitorIndex);
    visitor = UnityTower_GetVisitor(surveySave, visitorIndex);
    gender = getTrainerGender(visitor);
    province = UnityTower_GetVisitorParam(surveySave, visitorIndex, UNITY_TOWER_VISITOR_PARAM_UNK26_0);
    id = getIDAsUInt(visitor);
    if (CountryHasProvinces(GameData_GetUnityTowerSave(gameData)->floor) != 0) {
        hasProvince = 1;
    }
    return Scrcmd_CalcPartnerGreetingIndex(gender, id, province, param, hasProvince);
}

u32 Scrcmd_CalcPartnerGreetingIndex(u32 gender, u32 id, u32 province, u32 a3, u32 hasProvince) {
    u32 messageId;

    if (hasProvince != 0) {
        messageId = 4;
    } else {
        messageId = 0x44;
    }
    if (province < 5) {
        messageId += province * 12;
    }
    if (gender != 0) {
        messageId += 6;
    }
    if (a3 == 0) {
        messageId++;
        messageId += (u32)(_u32_div_f(id, 5) >> 32);
    }
    return messageId;
}

void LoadUnityTowerVisitorWordSet(WordSet *wordSet, GameData *gameData, u32 index) {
    SaveControl *save;
    UnityTowerSurveySave *survey;
    UnityTowerFloor *towerSave;
    u8 country;
    PlayerInfo *visitor;
    u32 value;
    u8 hobby;

    save = GameData_GetSaveControl(gameData);
    survey = getUnityTower_SurveySaveBlkAddrress(save);
    towerSave = GameData_GetUnityTowerSave(gameData);
    country = towerSave->floor;
    visitor = UnityTower_GetVisitor(survey, index);
    loadCountryToStrbuf(wordSet, 0, country);
    if (CountryHasProvinces(country)) {
        loadCountryAreaToStrbuf(wordSet, 1, country, UnityTowerVisitor_GetProvince(visitor));
    }
    copyVarForText(wordSet, 3, GetGameDataPlayerInfo(gameData));
    WordSet_LoadSpeciesName(wordSet, 5,
                            (u16)UnityTower_GetVisitorParam(survey, index, UNITY_TOWER_VISITOR_PARAM_SENT_SPECIES));
    loadHobbyNameToStrbuf(wordSet, 6, getPlayerSurveys(survey));
    copyVarForText(wordSet, 7, visitor);
    value = UnityTower_GetVisitorParam(survey, index, UNITY_TOWER_VISITOR_PARAM_COUNTRY_COUNT);
    if (value == 0) {
        value = 1;
    }
    WordSetNumber(wordSet, 8, value, 3, 0, 1);
    WordSet_LoadSpeciesName(wordSet, 9,
                            (u16)UnityTower_GetVisitorParam(survey, index, UNITY_TOWER_VISITOR_PARAM_RECEIVED_SPECIES));
    hobby = UnityTower_GetVisitorParam(survey, index, UNITY_TOWER_VISITOR_PARAM_HOBBY);
    loadHobbyNameToStrbuf(wordSet, 10, hobby);
}
