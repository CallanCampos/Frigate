// The ROM names this file scrcmd_ochiba.c.
// GetPWTTournamentInfo and PWT_TOURNAMENT_INFO are our descriptive names for the tournament metadata.

#include "types.h"
#include "constants/arc.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "constants/zones.h"
#include "field/field.h"
#include "field/field_effect.h"
#include "field/field_g3dobj.h"
#include "field/field_pokemon_form.h"
#include "field/field_script.h"
#include "field/medal.h"
#include "field/scrcmd_leaves.h"
#include "field/unity_tower.h"
#include "field/wbt.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/medal_box.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "save/wbt_save.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "text/system/scrcmd_bsubway_wbt_party.h"

// clang-format off
const WbtTournamentInfo PWT_TOURNAMENT_INFO[15] = {
    { ScrcmdBsubwayWbtParty_Text_ChampionsTournament, 17, 2, 1, 2, { 0, 0, 0, 0 } },
    { ScrcmdBsubwayWbtParty_Text_TypeExpertTournament, 0, 2, 2, 2, { 2, 2, 3, 3 } },
    { ScrcmdBsubwayWbtParty_Text_DownloadTournament, 29, 1, 4, 0, { 2, 2, 3, 3 } },
    { ScrcmdBsubwayWbtParty_Text_DriftveilTournament, 18, 1, 0, 0, { 1, 1, 2, 2 } },
    { ScrcmdBsubwayWbtParty_Text_UnovaLeadersTournament, 19, 2, 0, 1, { 1, 1, 2, 2 } },
    { ScrcmdBsubwayWbtParty_Text_KantoLeadersTournament, 20, 2, 1, 1, { 2, 2, 3, 3 } },
    { ScrcmdBsubwayWbtParty_Text_JohtoLeadersTournament, 21, 2, 1, 1, { 2, 2, 3, 3 } },
    { ScrcmdBsubwayWbtParty_Text_HoennLeadersTournament, 22, 2, 1, 1, { 2, 2, 3, 3 } },
    { ScrcmdBsubwayWbtParty_Text_SinnohLeadersTournament, 23, 2, 1, 1, { 2, 2, 3, 3 } },
    { ScrcmdBsubwayWbtParty_Text_WorldLeadersTournament, 24, 2, 1, 2, { 0, 0, 0, 0 } },
    { ScrcmdBsubwayWbtParty_Text_DriftveilTournament, 18, 1, 0, 0, { 1, 0, 0, 0 } },
    { ScrcmdBsubwayWbtParty_Text_RentalTournament, 25, 1, 1, 0, { 1, 1, 2, 2 } },
    { ScrcmdBsubwayWbtParty_Text_RentalMasterTournament, 26, 2, 1, 2, { 2, 2, 3, 3 } },
    { ScrcmdBsubwayWbtParty_Text_MixTournament, 27, 1, 0, 0, { 1, 1, 2, 2 } },
    { ScrcmdBsubwayWbtParty_Text_MixMasterTournament, 28, 2, 1, 2, { 2, 2, 3, 3 } },
};

// clang-format on

// clang-format off
const HabitatListHeader HABITAT_LIST_HEADERS[57] = {
    { 0x9b8, ZONE_ASPERTIA_CITY, 0x18, 0 }, { 0x9c0, ZONE_ROUTE_19, 0x17, 0 }, { 0x9c1, ZONE_ROUTE_20, 0x1c, 0 },
    { 0x9c2, ZONE_FLOCCESY_RANCH, 0x1a, 0 }, { 0x9b9, ZONE_VIRBANK_CITY, 0x1d, 0 }, { 0x9c3, ZONE_VIRBANK_COMPLEX, 0x20, 0 },
    { 0x9ac, ZONE_CASTELIA_CITY, 0x22, 0 }, { 0x9c4, ZONE_CASTELIA_SEWERS, 0x1e, 0 }, { 0x9c5, ZONE_ROUTE_4, 0x0, 0 },
    { 0x9c6, ZONE_DESERT_RESORT, 0x29, 0 }, { 0x9c7, ZONE_RELIC_CASTLE, 0x15, 0 }, { 0x9c8, ZONE_ROUTE_5, 0x9, 0 },
    { 0x9c9, ZONE_ROUTE_16, 0x8, 0 }, { 0x9ca, ZONE_LOSTLORN_FOREST, 0x37, 0 }, { 0x9cb, ZONE_DRIFTVEIL_DRAWBRIDGE, 0x35, 0 },
    { 0x9cc, ZONE_ROUTE_6, 0xd, 0 }, { 0x9cd, ZONE_RELIC_PASSAGE, 0x7, 0 }, { 0x9f0, ZONE_CLAY_TUNNEL, 0x3, 0 },
    { 0x9ce, ZONE_MISTRALTON_CAVE, 0x31, 0 }, { 0x9cf, ZONE_CHARGESTONE_CAVE, 0x39, 0 }, { 0x9d0, ZONE_ROUTE_7, 0x13, 0 },
    { 0x9d1, ZONE_CELESTIAL_TOWER_2, 0x27, 0 }, { 0x9d2, ZONE_REVERSAL_MOUNTAIN, 0x25, 0 }, { 0x9d3, ZONE_STRANGE_HOUSE, 0x1b, 0 },
    { 0x9b5, ZONE_UNDELLA_TOWN, 0xc, 0 }, { 0x9d4, ZONE_ROUTE_13, 0x32, 0 }, { 0x9d5, ZONE_UNDELLA_BAY, 0xe, 0 },
    { 0x9d6, ZONE_ROUTE_14, 0x38, 0 }, { 0x9d7, ZONE_ABUNDANT_SHRINE, 0xb, 0 }, { 0x9d8, ZONE_ROUTE_12, 0x2f, 0 },
    { 0x9d9, ZONE_VILLAGE_BRIDGE, 0x5, 0 }, { 0x9da, ZONE_ROUTE_11, 0x2a, 0 }, { 0x9db, ZONE_ROUTE_9, 0x1f, 0 },
    { 0x9dc, ZONE_SEASIDE_CAVE, 0x36, 0 }, { 0x9dd, ZONE_ROUTE_21, 0x2, 0 }, { 0x9de, ZONE_ROUTE_15, 0x1, 0 },
    { 0x9bf, ZONE_MARVELOUS_BRIDGE, 0xa, 0 }, { 0x9ba, ZONE_HUMILAU_CITY, 0x24, 0 }, { 0x9df, ZONE_ROUTE_22, 0x28, 0 },
    { 0x9e0, ZONE_GIANT_CHASM, 0x12, 0 }, { 0x9e1, ZONE_ROUTE_23, 0x2d, 0 }, { 0x9e2, ZONE_VICTORY_ROAD, 0x14, 0 },
    { 0x9e3, ZONE_ROUTE_8, 0x19, 0 }, { 0x9e4, ZONE_MOOR_OF_ICIRRUS, 0x2e, 0 }, { 0x9b0, ZONE_ICIRRUS_CITY, 0x34, 0 },
    { 0x9e5, ZONE_DRAGONSPIRAL_TOWER, 0x6, 0 }, { 0x9e6, ZONE_TWIST_MOUNTAIN, 0x4, 0 }, { 0x9e7, ZONE_PINWHEEL_FOREST, 0x23, 0 },
    { 0x9e8, ZONE_ROUTE_3, 0x33, 0 }, { 0x9e9, ZONE_WELLSPRING_CAVE, 0xf, 0 }, { 0x9aa, ZONE_STRIATON_CITY, 0x16, 0 },
    { 0x9ea, ZONE_DREAMYARD, 0x26, 0 }, { 0x9ec, ZONE_ROUTE_2, 0x30, 0 }, { 0x9eb, ZONE_ROUTE_1, 0x2c, 0 },
    { 0x9ed, ZONE_ROUTE_17, 0x2b, 0 }, { 0x9ee, ZONE_ROUTE_18, 0x11, 0 }, { 0x9ef, ZONE_P2_LABORATORY, 0x21, 0 },
};

// clang-format on

void LeavesEffect_Start(LeavesEffectArgs *args) {
    TCBManager *tcbManager = Field_GetTCBMgr(args->field);
    LeavesEffectWork *work = GFL_HeapAllocate(HEAPID_TAIL(Field_GetHeapID(args->field)), sizeof(LeavesEffectWork), TRUE,
                                              "scrcmd_ochiba.c", 0x134);
    void *effects = Field_GetFieldEffects(args->field);
    VecFx32 position;
    FieldG3DObjSystem *sys;
    ArcTool *arc;
    FieldG3DObjResRequest request;

    FieldPlayer_GetWPos(args->player, &position);
    sys = FieldEffects_GetFieldMapData(effects);
    arc = FieldEffects_GetArc(effects);
    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 0x61);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 0x85);
    work->sys = sys;
    work->resGroup = FieldG3DObjSystem_AddResGroup(sys, &request, FALSE);
    work->obj = FieldG3DObjSystem_AddObj(sys, work->resGroup, 0, &position);
    GFL_SndSEPlay(SEQ_SE_OCHIBA_01);
    GFL_TCBMgrAddTask(tcbManager, LeavesEffect_UpdateTCB, work, 0);
}

void LeavesEffect_UpdateTCB(TCB *tcb, void *data) {
    LeavesEffectWork *work = data;

    if (FieldG3DObjSystem_StepObjAnm(work->sys, work->obj, FX32_ONE) != TRUE) {
        FieldG3DObjSystem_FreeObj(work->sys, work->obj);
        FieldG3DObjSystem_FreeResGroup(work->sys, work->resGroup);
        GFL_HeapFree(work);
        GFL_TCBRemove(tcb);
    }
}

const WbtTournamentInfo *GetPWTTournamentInfo(s32 tournament) {
    if (tournament == 0 || tournament >= 16) {
        tournament = 4;
    }
    return &PWT_TOURNAMENT_INFO[tournament - 1];
}

void LoadPWTTournamentTypeText(HeapID heapId, u32 tournament, StrBuf *strbuf) {
    u16 name = GetPWTTournamentInfo(tournament)->name;
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SCRCMD_BSUBWAY_WBT_PARTY, heapId);

    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, name, strbuf);
        GFL_MsgDataFree(msgData);
    }
}

u8 GetPWTSeason(const WbtSystem *wbt) {
    return wbt->season;
}

BOOL Scrcmd_Leaves_PokeDexCheckHabitatList(VM *vm, FieldScriptEnv *env) {
    HeapID heapId;
    PokeDexSave *pokedex;
    GameSystem *gsys;
    u16 time;
    s32 i;
    const HabitatListHeader *header;
    u16 zoneId;
    u16 caught;
    u16 *result;
    ArcTool *arc;
    HabitatList list;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    pokedex = GameData_GetPokedex(GSYS_GetGameData(gsys));
    heapId = Field_GetHeapID(GSYS_GetField(gsys));
    zoneId = ScriptReadAny(vm, env);
    time = ScriptReadAny(vm, env);
    caught = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    arc = GFL_ArcSysCreateFileHandle(0x128, heapId);
    MI_CpuFill8(&list, 0, sizeof(HabitatList));
    for (i = 0; i < 57; i++) {
        header = &HABITAT_LIST_HEADERS[i];
        if (header->zoneId == zoneId) {
            GFL_ArcToolRead(arc, header->fileId, &list);
            *result = CheckHabitatList(&list, pokedex, time, caught);
        }
    }
    GFL_ArcToolFree(arc);
    return FALSE;
}

BOOL CheckHabitatList(const HabitatList *list, PokeDexSave *pokedex, u8 time, u32 caught) {
    s32 i;
    u8 j;
    BOOL registered;
    u8 found[3];

    for (i = 0; i < list->count; i++) {
        MI_CpuFill8(found, 0, sizeof(found));
        for (j = 0; j < 4; j++) {
            found[time] |= list->entries[i].encounters[j][time];
        }
        if (found[time] >= 1) {
            if (caught == TRUE) {
                registered = PokeDex_IsCaught(pokedex, list->entries[i].species);
            } else {
                registered = PokeDex_IsSeen(pokedex, list->entries[i].species);
            }
            if (!registered) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

BOOL Scrcmd_Leaves_KeyInfoGetMyCityProgress(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    SaveControl *save = GameData_GetSaveControl(gameData);
    u16 *result = ScriptReadVar(vm, env);
    u16 key = GameData_GetMyCity(gameData);

    *result = KeyDataSave_GetProgress(getKeyDataBlkAddress(save), key);
    return FALSE;
}

BOOL Scrcmd_Leaves_WbtIsTrainerBeaten(VM *vm, FieldScriptEnv *env) {
    WbtSave *block = GameData_GetWbtSave(FieldScriptEnv_GetGameData(env));
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = WbtSave_IsTrainerBeaten(block, index);
    return FALSE;
}

BOOL Scrcmd_Leaves_GetWifiHistoryCountryCount(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 index;
    u16 *result;
    GameData *gameData;
    UnityTowerSurveySave *survey;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    index = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    gameData = GSYS_GetGameData(gsys);
    survey = getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(gameData));
    *result = UnityTower_GetCountryCountForPlayer(survey, GetGameDataPlayerInfo(gameData), index);
    return FALSE;
}

BOOL Scrcmd_Leaves_UnityTowerGetVisitorCount(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    result = ScriptReadVar(vm, env);
    *result =
        UnityTower_CountVisitors(getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(GSYS_GetGameData(gsys))));
    return FALSE;
}

BOOL Scrcmd_Leaves_MarkPartnerCandidate(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    UnityTowerFloor *unityTower;
    u8 visitor;
    u32 index;

    FieldScriptEnv_GetScriptWork(env);
    gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    unityTower = GameData_GetUnityTowerSave(gameData);
    visitor = ScriptReadAny(vm, env);
    index = Scrcmd_GetPartnerCandidateHistoryIndex(unityTower, visitor);
    if (index != UNITY_TOWER_HISTORY_INDEX_NONE) {
        UnityTower_SetVisitorParam(getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(gameData)), index,
                                   UNITY_TOWER_VISITOR_PARAM_SCRIPT_FLAG, TRUE);
    }
    return FALSE;
}

BOOL Scrcmd_Leaves_UnityTowerGetVisitorParam(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    UnityTowerFloor *unityTower;
    u16 visitor;
    u16 *result;
    u32 index;

    FieldScriptEnv_GetScriptWork(env);
    gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    unityTower = GameData_GetUnityTowerSave(gameData);
    visitor = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    *result = FALSE;
    index = Scrcmd_GetPartnerCandidateHistoryIndex(unityTower, visitor);
    if (index != UNITY_TOWER_HISTORY_INDEX_NONE &&
        (u8)UnityTower_GetVisitorParam(getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(gameData)), index,
                                       UNITY_TOWER_VISITOR_PARAM_SCRIPT_FLAG)) {
        *result = TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_Leaves_UnityTowerSetRatingLevel(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u8 index;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    index = ScriptReadAny(vm, env);
    UnityTower_SetRatingLevel(getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(GSYS_GetGameData(gsys))),
                              index);
    return FALSE;
}

BOOL Scrcmd_Leaves_UnityTowerSetHobby(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u8 hobby;
    UnityTowerSurveySave *save;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    hobby = ScriptReadAny(vm, env);
    save = getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    setPlayerSurveys(save, hobby);
    return FALSE;
}

BOOL Scrcmd_Leaves_UnityTowerGetHobby(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 *result;
    UnityTowerSurveySave *save;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    result = ScriptReadVar(vm, env);
    save = getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    *result = getPlayerSurveys(save);
    return FALSE;
}

BOOL Scrcmd_Leaves_UnityTowerSetFloor(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 floor;
    u16 value;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    floor = ScriptReadAny(vm, env);
    value = ScriptReadAny(vm, env);
    Scrcmd_FillPartnerCandidates(gsys, floor, value);
    return FALSE;
}

BOOL Scrcmd_Leaves_UnityTowerInitVisitorMessage(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    UnityTowerFloor *save;
    WordSet *wordSet;
    u16 index;
    u16 param;
    u16 *result;

    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    save = GameData_GetUnityTowerSave(GSYS_GetGameData(gsys));
    wordSet = ScriptWork_GetWordSet(work);
    index = ScriptReadAny(vm, env);
    param = VM_Read16(vm);
    result = ScriptReadVar(vm, env);
    *result = Scrcmd_GetPartnerGreetingMessage(wordSet, gsys, save, index, param);
    return FALSE;
}

BOOL Scrcmd_Leaves_UnityTowerGetVisitorCountry(VM *vm, FieldScriptEnv *env) {
    u16 *result;
    GameSystem *gsys;

    result = ScriptReadVar(vm, env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    *result = UnityTowerVisitor_GetCountry(GetGameDataPlayerInfo(GSYS_GetGameData(gsys))) != 0;
    return FALSE;
}

void DiscoverInitialMedalsCore(MedalBox *p_medal_box, HeapID heapId) {
    ArcTool *arc;
    RTCDate date;
    u16 i;
    MedalData medals[MEDAL_COUNT];

    GFL_ASSERT(p_medal_box);
    arc = GFL_ArcSysCreateFileHandle(0xeb, heapId);
    GFL_ArcToolReadRange(arc, 0, 0, sizeof(medals), medals);
    RTC_GetCachedDate(&date);
    for (i = 0; i < MEDAL_COUNT; i++) {
        if (MedalBox_GetMedalStatus(p_medal_box, i) < MEDAL_STATUS_1 && medals[i].hintable == TRUE) {
            MedalBox_DiscoverMedal(p_medal_box, i);
            MedalBox_DiscoverInitialMedal(p_medal_box, i, date.year, date.month, date.day);
        }
    }
    GFL_ArcToolFree(arc);
}

void DiscoverInitialMedals(MedalBox *box, HeapID heapId) {
    DiscoverInitialMedalsCore(box, heapId);
}

void CheckResetKeldeoOrdinaryForme(GameSystem *gsys) {
    GameData *gameData;
    PokeParty *party;
    PartyPkm *pkm;
    s32 count;
    s32 index;
    s32 moveIndex;
    u32 species;
    u32 move;

    gameData = GSYS_GetGameData(gsys);
    party = GameData_GetParty(gameData);
    count = PokeParty_GetPkmCount(party);
    index = 0;
    while (index < count) {
        pkm = PokeParty_GetPkm(party, index);
        move = PokeParty_GetParam(pkm, (PkmField)PKM_PARAM_SPECIES, NULL);
        species = (u16)move;
        if (PokeParty_GetParam(pkm, (PkmField)PKM_PARAM_IS_EGG, NULL) == 0 && species == SPECIES_KELDEO &&
            PokeParty_GetParam(pkm, (PkmField)PKM_PARAM_FORM, NULL) == 1) {
            for (moveIndex = 0; moveIndex < 4; moveIndex++) {
                if (PokeParty_GetParam(pkm, (PkmField)(PKM_PARAM_MOVE1 + moveIndex), NULL) == MOVE_SECRET_SWORD) {
                    break;
                }
            }
            if (moveIndex == 4) {
                PokeParty_ChangeForme(pkm, 0);
            }
        }
        if (species > SPECIES_GENESECT) {
            pkm->base.contentBuffer.chunks[0].rawData[0x16] = 0xff;
            pkm->base.contentBuffer.chunks[0].rawData[0x17] = 0xff;
        }
        index++;
    }
}
