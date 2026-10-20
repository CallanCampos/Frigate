#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_internal.h"
#include "field/field_script.h"
#include "field/medal.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/rtc.h"
#include "save/medal_box.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL s026E_MedalGetCount(VM *vm, FieldScriptEnv *env) {
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    u8 mode = VM_Read8(vm);
    u16 *result = ScriptReadVar(vm, env);

    switch (mode) {
    case 0:
        *result = CountMedalsByStatus(env, MEDAL_STATUS_1);
        break;
    case 1:
        *result = CountMedalsByStatus(env, MEDAL_STATUS_DISCOVERED);
        break;
    case 2:
        *result = CountMedalsByStatus(env, MEDAL_STATUS_EARNED);
        break;
    case 3:
        *result = CountMedalsByStatus(env, MEDAL_STATUS_OBTAINED);
        break;
    case 4:
        *result = MedalBox_GetNextRankRequirement(box) + 1;
        break;
    case 5:
        *result = GetHintableMedalCount(env);
        break;
    case 6:
        *result = MEDAL_COUNT - CountMedalsByStatus(env, MEDAL_STATUS_OBTAINED);
        break;
    case 7:
        *result = MedalBox_GetRank(box);
        break;
    }
    return FALSE;
}

BOOL s0271_MedalAcknowledge(VM *vm, FieldScriptEnv *env) {
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    u16 medal = ScriptReadAny(vm, env);
    u16 acknowledge = ScriptReadAny(vm, env);
    RTCDate date;

    RTC_GetCachedDate(&date);
    if (acknowledge) {
        MedalBox_AcknowledgeMedal(box, medal, date.year, date.month, date.day);
        GameBeaconSys_SetMedalCount(MedalBox_GetObtainedCount(box, 0));
    } else {
        MedalBox_DiscoverInitialMedal(box, medal, date.year, date.month, date.day);
    }
    return FALSE;
}

BOOL s0272_MedalGetGuruActor(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 *exists = ScriptReadVar(vm, env);
    u16 *uid = ScriptReadVar(vm, env);

    if (GetMrMedalActorIndex(field) != NULL) {
        *exists = TRUE;
    } else {
        *exists = FALSE;
    }
    *uid = GetMrMedalActorUID(gameData);
    return FALSE;
}

BOOL s0273_MedalGive(VM *vm, FieldScriptEnv *env) {
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    u16 medal = ScriptReadAny(vm, env);

    if (MedalBox_GetMedalStatus(box, medal) < MEDAL_STATUS_EARNED) {
        MedalBox_GiveMedal(box, medal);
    }
    return FALSE;
}

BOOL s029F_MedalDiscover(VM *vm, FieldScriptEnv *env) {
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    u16 medal = ScriptReadAny(vm, env);

    if (MedalBox_GetMedalStatus(box, medal) == MEDAL_STATUS_UNKNOWN) {
        MedalBox_DiscoverMedal(box, medal);
    }
    return FALSE;
}

BOOL s029E_MedalGetFieldEffectID(VM *vm, FieldScriptEnv *env) {
    MedalData data;
    u16 medal;
    u16 *result;

    SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    medal = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    GFL_ArcSysRead(&data, 0xeb, medal);
    *result = data.type + 0x4e;
    return FALSE;
}

u32 CountMedalsByStatus(FieldScriptEnv *env, u32 status) {
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    u32 i;
    u32 count = 0;

    for (i = 0; i < MEDAL_COUNT; i++) {
        u8 medalStatus = MedalBox_GetMedalStatus(box, i);
        if (medalStatus == status) {
            count++;
        }
    }
    return count;
}

u32 GetHintableMedalCount(FieldScriptEnv *env) {
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    ArcTool *arc = GFL_ArcSysCreateFileHandle(0xeb, FieldScriptEnv_GetHeapID(env));
    u32 i, count;
    MedalData data;

    for (i = 0, count = 0; i < MEDAL_COUNT; i++) {
        GFL_ArcToolRead(arc, i, &data);
        if (data.hintable && MedalBox_GetMedalStatus(box, i) == MEDAL_STATUS_UNKNOWN) {
            count++;
        }
    }
    GFL_ArcToolFree(arc);
    return count;
}

BOOL s02A0_MedalIsObtained(VM *vm, FieldScriptEnv *env) {
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    u16 *result = ScriptReadVar(vm, env);

    if (MedalBox_GetMedalStatus(box, ScriptReadAny(vm, env)) == MEDAL_STATUS_OBTAINED) {
        *result = TRUE;
    } else {
        *result = FALSE;
    }
    return FALSE;
}

BOOL s02A1_MedalGetMostCompleteCategory(VM *vm, FieldScriptEnv *env) {
    s32 j;
    fx32 best = 0;
    fx32 obtained[5] = { 0 };
    fx32 total[5] = { 0 };
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    ArcTool *arc = GFL_ArcSysCreateFileHandle(0xeb, FieldScriptEnv_GetHeapID(env));
    u16 *result = ScriptReadVar(vm, env);
    u32 typeToCategory[27] = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4 };
    MedalData data;
    u32 i;

    for (i = 0; i < MEDAL_COUNT; i++) {
        GFL_ArcToolRead(arc, i, &data);
        if (MedalBox_GetMedalStatus(box, i) == MEDAL_STATUS_OBTAINED) {
            obtained[typeToCategory[data.type]] += FX32_ONE;
        }
        total[typeToCategory[data.type]] += FX32_ONE;
    }
    GFL_ArcToolFree(arc);
    {
        u32 categories[5] = { 4, 3, 1, 2, 0 };
        u32 category;
        fx32 ratio;

        *result = 0;
        for (j = 0; j < 5; j++) {
            category = categories[j];
            ratio = FX_Div(obtained[category], total[category]);
            if (best < ratio) {
                best = ratio;
                *result = category;
            }
        }
    }
    return FALSE;
}

FieldActor *GetMrMedalActorIndex(Field *field) {
    MMSys *actorSystem;
    u32 index;
    FieldActor *actor;

    index = 0;
    actorSystem = Field_GetActorSystem(field);
    if (NextActor(actorSystem, &actor, &index) == TRUE) {
        do {
            if (FldAct_GetSCRID(actor) == 0x298e) {
                return actor;
            }
        } while (NextActor(actorSystem, &actor, &index) == TRUE);
    }
    return NULL;
}

u32 GetMrMedalActorUID(GameData *gameData) {
    EventData *eventData;
    const ZoneNPC *npcs;
    s32 count;
    s32 i;

    eventData = GameData_GetEventData(gameData);
    npcs = GetZoneNPCs(eventData);
    count = GetZoneNPCsCount(eventData);
    if (npcs == NULL || count == 0) {
        return -1;
    }
    for (i = 0; i < count; i++) {
        if (npcs[i].scrId == 0x298e) {
            return npcs[i].uid;
        }
    }
    return -1;
}

BOOL s02E1_MedalDiscoverInitial(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    SaveControl *save;
    MedalBox *box;
    HeapID heapId;

    gameData = FieldScriptEnv_GetGameData(env);
    save = GameData_GetSaveControl(gameData);
    box = SaveControl_GetMedalBox(save);
    heapId = FieldScriptEnv_GetHeapID(env);
    DiscoverInitialMedals(box, heapId);
    return FALSE;
}
