#include "types.h"
#include "constants/arc.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_effect.h"
#include "field/field_g3dobj.h"
#include "field/field_pokemon_form.h"
#include "field/field_script.h"
#include "field/medal.h"
#include "field/scrcmd_ochiba.h"
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
#include "save/pokedex.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

const WbtTournamentInfo data_ov036_021d4920[15] = {
    { 0x4, 17, 2, 1, 2, { 0, 0, 0, 0 } },
    { 0x11, 0, 2, 2, 2, { 2, 2, 3, 3 } },
    { 0x5, 29, 1, 4, 0, { 2, 2, 3, 3 } },
    { 0x6, 18, 1, 0, 0, { 1, 1, 2, 2 } },
    { 0x7, 19, 2, 0, 1, { 1, 1, 2, 2 } },
    { 0x8, 20, 2, 1, 1, { 2, 2, 3, 3 } },
    { 0x9, 21, 2, 1, 1, { 2, 2, 3, 3 } },
    { 0xa, 22, 2, 1, 1, { 2, 2, 3, 3 } },
    { 0xb, 23, 2, 1, 1, { 2, 2, 3, 3 } },
    { 0xc, 24, 2, 1, 2, { 0, 0, 0, 0 } },
    { 0x6, 18, 1, 0, 0, { 1, 0, 0, 0 } },
    { 0xd, 25, 1, 1, 0, { 1, 1, 2, 2 } },
    { 0xe, 26, 2, 1, 2, { 2, 2, 3, 3 } },
    { 0xf, 27, 1, 0, 0, { 1, 1, 2, 2 } },
    { 0x10, 28, 2, 1, 2, { 2, 2, 3, 3 } },
};

const HabitatListHeader HABITAT_LIST_HEADERS[57] = {
    { 0x9b8, 0x1ab, 0x18, 0 }, { 0x9c0, 0x1b5, 0x17, 0 }, { 0x9c1, 0x1be, 0x1c, 0 },
    { 0x9c2, 0x1bc, 0x1a, 0 }, { 0x9b9, 0x1c0, 0x1d, 0 }, { 0x9c3, 0x1c8, 0x20, 0 },
    { 0x9ac, 0x1c, 0x22, 0 }, { 0x9c4, 0x1ef, 0x1e, 0 }, { 0x9c5, 0x146, 0x0, 0 },
    { 0x9c6, 0x9d, 0x29, 0 }, { 0x9c7, 0xa0, 0x15, 0 }, { 0x9c8, 0x149, 0x9, 0 },
    { 0x9c9, 0x17f, 0x8, 0 }, { 0x9ca, 0x181, 0x37, 0 }, { 0x9cb, 0xfd, 0x35, 0 },
    { 0x9cc, 0x14b, 0xd, 0 }, { 0x9cd, 0x1f7, 0x7, 0 }, { 0x9f0, 0x1fa, 0x3, 0 },
    { 0x9ce, 0x14d, 0x31, 0 }, { 0x9cf, 0xc2, 0x39, 0 }, { 0x9d0, 0x151, 0x13, 0 },
    { 0x9d1, 0x152, 0x27, 0 }, { 0x9d2, 0x1cd, 0x25, 0 }, { 0x9d3, 0x1ce, 0x1b, 0 },
    { 0x9b5, 0x19c, 0xc, 0 }, { 0x9d4, 0x172, 0x32, 0 }, { 0x9d5, 0xf0, 0xe, 0 },
    { 0x9d6, 0x176, 0x38, 0 }, { 0x9d7, 0x178, 0xb, 0 }, { 0x9d8, 0x170, 0x2f, 0 },
    { 0x9d9, 0xff, 0x5, 0 }, { 0x9da, 0x16d, 0x2a, 0 }, { 0x9db, 0x15c, 0x1f, 0 },
    { 0x9dc, 0x203, 0x36, 0 }, { 0x9dd, 0x1cf, 0x2, 0 }, { 0x9de, 0x17a, 0x1, 0 },
    { 0x9bf, 0x107, 0xa, 0 }, { 0x9ba, 0x1d1, 0x24, 0 }, { 0x9df, 0x1da, 0x28, 0 },
    { 0x9e0, 0xe6, 0x12, 0 }, { 0x9e1, 0x1db, 0x2d, 0 }, { 0x9e2, 0x23d, 0x14, 0 },
    { 0x9e3, 0x159, 0x19, 0 }, { 0x9e4, 0x15a, 0x2e, 0 }, { 0x9b0, 0x71, 0x34, 0 },
    { 0x9e5, 0xcd, 0x6, 0 }, { 0x9e6, 0xc6, 0x4, 0 }, { 0x9e7, 0x9a, 0x23, 0 },
    { 0x9e8, 0x141, 0x33, 0 }, { 0x9e9, 0x144, 0xf, 0 }, { 0x9aa, 0x6, 0x16, 0 },
    { 0x9ea, 0x98, 0x26, 0 }, { 0x9ec, 0x13f, 0x30, 0 }, { 0x9eb, 0x13d, 0x2c, 0 },
    { 0x9ed, 0x1a7, 0x2b, 0 }, { 0x9ee, 0x183, 0x11, 0 }, { 0x9ef, 0xee, 0x21, 0 },
};

void func_ov036_021c97b8(OchibaEffectArgs *args) {
    TCBManager *tcbManager = Field_GetTCBMgr(args->field);
    OchibaEffectWork *work = GFL_HeapAllocate(HEAPID_TAIL(Field_GetHeapID(args->field)), sizeof(OchibaEffectWork), TRUE,
                                              "scrcmd_ochiba.c", 0x134);
    void *effects = Field_GetFieldEffects(args->field);
    VecFx32 position;
    FieldG3DObjSystem *sys;
    ArcTool *arc;
    FieldG3DObjResRequest request;

    FieldPlayer_GetWPos(args->player, &position);
    sys = func_ov036_021a3724(effects);
    arc = FieldEffects_GetArc(effects);
    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 0x61);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 0x85);
    work->sys = sys;
    work->resGroup = FieldG3DObjSystem_AddResGroup(sys, &request, FALSE);
    work->obj = FieldG3DObjSystem_AddObj(sys, work->resGroup, 0, &position);
    GFL_SndSEPlay(0x87e);
    GFL_TCBMgrAddTask(tcbManager, func_ov036_021c9870, work, 0);
}

void func_ov036_021c9870(TCB *tcb, void *data) {
    OchibaEffectWork *work = data;

    if (FieldG3DObjSystem_StepObjAnm(work->sys, work->obj, FX32_ONE) != TRUE) {
        FieldG3DObjSystem_FreeObj(work->sys, work->obj);
        FieldG3DObjSystem_FreeResGroup(work->sys, work->resGroup);
        GFL_HeapFree(work);
        GFL_TCBRemove(tcb);
    }
}

const WbtTournamentInfo *func_ov036_021c98a4(s32 tournament) {
    if (tournament == 0 || tournament >= 16) {
        tournament = 4;
    }
    return &data_ov036_021d4920[tournament - 1];
}

void LoadPWTTournamentTypeText(HeapID heapId, u32 tournament, StrBuf *strbuf) {
    u16 name = func_ov036_021c98a4(tournament)->name;
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SCRCMD_BSUBWAY_WBT_PARTY, heapId);

    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, name, strbuf);
        GFL_MsgDataFree(msgData);
    }
}

u8 func_ov036_021c98f4(const WbtSystem *wbt) {
    return wbt->unk1E;
}

BOOL s02CF_PokeDexCheckHabitatList(VM *vm, FieldScriptEnv *env) {
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
    sys_memset(&list, 0, sizeof(HabitatList));
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
        sys_memset(found, 0, sizeof(found));
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

BOOL func_ov036_021c9a40(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    SaveControl *save = GameData_GetSaveControl(gameData);
    u16 *result = ScriptReadVar(vm, env);
    u16 key = func_02017220(gameData);

    *result = func_02010274(getKeyDataBlkAddress(save), key);
    return FALSE;
}

BOOL func_ov036_021c9a7c(VM *vm, FieldScriptEnv *env) {
    void *block = func_020179f8(FieldScriptEnv_GetGameData(env));
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = func_0200fec8(block, index);
    return FALSE;
}

BOOL func_ov036_021c9ab0(VM *vm, FieldScriptEnv *env) {
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
    *result = func_02009cac(survey, GetGameDataPlayerInfo(gameData), index);
    return FALSE;
}

BOOL s02DD_UnityTowerGetVisitorCount(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    result = ScriptReadVar(vm, env);
    *result = func_02009ce4(getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(GSYS_GetGameData(gsys))));
    return FALSE;
}

BOOL func_ov036_021c9b38(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    UnityTowerFloor *unityTower;
    u8 visitor;
    u32 index;

    FieldScriptEnv_GetScriptWork(env);
    gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    unityTower = GameData_GetUnityTowerSave(gameData);
    visitor = ScriptReadAny(vm, env);
    index = func_ov033_0217aac4(unityTower, visitor);
    if (index != 0xff) {
        func_02009db4(getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(gameData)), index, 5, 1);
    }
    return FALSE;
}

BOOL func_ov036_021c9b88(VM *vm, FieldScriptEnv *env) {
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
    index = func_ov033_0217aac4(unityTower, visitor);
    if (index != 0xff &&
        (u8)UnityTower_GetVisitorParam(getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(gameData)), index, 5)) {
        *result = TRUE;
    }
    return FALSE;
}

BOOL func_ov036_021c9bec(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u8 index;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    index = ScriptReadAny(vm, env);
    func_02009d18(getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(GSYS_GetGameData(gsys))), index);
    return FALSE;
}

BOOL s02DE_UnityTowerSetHobby(VM *vm, FieldScriptEnv *env) {
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

BOOL s02DF_UnityTowerGetHobby(VM *vm, FieldScriptEnv *env) {
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

BOOL s02DB_UnityTowerSetFloor(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 floor;
    u16 value;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    floor = ScriptReadAny(vm, env);
    value = ScriptReadAny(vm, env);
    func_ov033_0217aa1c(gsys, floor, value);
    return FALSE;
}

BOOL s02DC_UnityTowerInitVisitorMessage(VM *vm, FieldScriptEnv *env) {
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
    *result = func_ov033_0217aad8(wordSet, gsys, save, index, param);
    return FALSE;
}

BOOL func_ov036_021c9d24(VM *vm, FieldScriptEnv *env) {
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
