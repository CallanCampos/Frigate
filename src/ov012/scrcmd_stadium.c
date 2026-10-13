#include "types.h"
#include "field/event_data.h"
#include "field/field_actor.h"
#include "field/field_script.h"
#include "field/stadium_script.h"
#include "field/trainer_script.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL s01E1_StadiumLoadTrainerTable(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *trainers = GFL_ArcSysReadHeapNewLZ(0xce, 0, 0, HEAPID_TAIL(4));
    ScriptWork_SetStadiumTrainers(work, trainers);
    return FALSE;
}

BOOL s01E2_StadiumFreeTrainerTable(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *trainers = ScriptWork_GetStadiumTrainers(work);
    if (trainers != NULL) {
        GFL_HeapFree(trainers);
        ScriptWork_SetStadiumTrainers(work, NULL);
    }
    return FALSE;
}

BOOL s01E3_StadiumSetupActorSingle(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 a = ScriptReadAny(vm, env);
    u16 b = ScriptReadAny(vm, env);
    u16 c = ScriptReadAny(vm, env);
    EventData *eventData = GameData_GetEventData(FieldScriptEnv_GetGameData(env));
    StadiumTrainerEntry *trainers = ScriptWork_GetStadiumTrainers(work);
    u16 idx = FindStadiumTrainerIndex(trainers, a, c);
    u16 trainerId = trainers[idx].trainerId;
    u16 scriptId = GetNormalSCRIDFromTrainerID(trainerId);
    SetZoneNPCSCRID(eventData, b, scriptId);
    return FALSE;
}

BOOL s01E0_StadiumSetupActorsDouble(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 a = ScriptReadAny(vm, env);
    u16 b = ScriptReadAny(vm, env);
    u16 c = ScriptReadAny(vm, env);
    u16 d = ScriptReadAny(vm, env);
    EventData *eventData = GameData_GetEventData(FieldScriptEnv_GetGameData(env));
    StadiumTrainerEntry *trainers = ScriptWork_GetStadiumTrainers(work);
    u16 idx = FindStadiumTrainerIndex(trainers, a, d);
    SetZoneNPCSCRID(eventData, b, GetNormalSCRIDFromTrainerID(trainers[idx].trainerId));
    SetZoneNPCSCRID(eventData, c, GetPairMember2SCRIDFromTrainerID(trainers[idx].trainerId));
    return FALSE;
}

BOOL s01E0_StadiumSetupActorsTriple(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 npc1 = ScriptReadAny(vm, env);
    u16 npc2 = ScriptReadAny(vm, env);
    u16 npc3 = ScriptReadAny(vm, env);
    u16 idx1 = ScriptReadAny(vm, env);
    u16 idx2 = ScriptReadAny(vm, env);
    u16 idx3 = ScriptReadAny(vm, env);
    EventData *eventData = GameData_GetEventData(FieldScriptEnv_GetGameData(env));
    StadiumTrainerEntry *trainers = ScriptWork_GetStadiumTrainers(work);

    SetZoneNPCMdlID(eventData, npc1, trainers[idx1].objCode);
    SetZoneNPCMdlID(eventData, npc2, trainers[idx2].objCode);
    SetZoneNPCMdlID(eventData, npc3, trainers[idx3].objCode);
    SetZoneNPCSCRID(eventData, npc1, GetNormalSCRIDFromTrainerID(trainers[idx1].trainerId));
    SetZoneNPCSCRID(eventData, npc2, GetNormalSCRIDFromTrainerID(trainers[idx2].trainerId));
    SetZoneNPCSCRID(eventData, npc3, GetNormalSCRIDFromTrainerID(trainers[idx3].trainerId));
    return FALSE;
}

BOOL s01E5_StadiumResetTrainerFlags(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    EventWork *eventWork;
    s32 i;
    StadiumTrainerEntry *trainers;

    work = FieldScriptEnv_GetScriptWork(env);
    eventWork = GameData_GetEventWork(GSYS_GetGameData(ScriptWork_GetGameSystem(work)));
    trainers = ScriptWork_GetStadiumTrainers(work);
    i = 0;
    while (i != 0x84) {
        clearTrainerBattleFlag(eventWork, trainers[i].trainerId);
        ++i;
    }
    return FALSE;
}

BOOL func_ov012_02159f90(VM *vm, FieldScriptEnv *env) {
    u16 *results[6];

    FieldScriptEnv_GetScriptWork(env);
    results[0] = ScriptReadVar(vm, env);
    results[1] = ScriptReadVar(vm, env);
    results[2] = ScriptReadVar(vm, env);
    results[3] = ScriptReadVar(vm, env);
    results[4] = ScriptReadVar(vm, env);
    results[5] = ScriptReadVar(vm, env);
    {
        u16 range = 52;
        u16 picked[6] = {0};
        s32 i;
        s32 value;
        s32 j;
        s16 n;

        for (i = 0; i < 6; i++) {
            n = GFL_RandomLC(range);
            for (value = 0; value < 52; value++) {
                for (j = 0; j < i; j++) {
                    if (value == picked[j]) {
                        break;
                    }
                }
                if (j == i && --n < 0) {
                    picked[i] = value;
                    break;
                }
            }
            range--;
        }
        *results[0] = picked[0] + 0x50;
        *results[1] = picked[1] + 0x50;
        *results[2] = picked[2] + 0x50;
        *results[3] = picked[3] + 0x50;
        *results[4] = picked[4] + 0x50;
        *results[5] = picked[5] + 0x50;
    }
    return FALSE;
}

u32 FindStadiumTrainerIndex(StadiumTrainerEntry *trainers, u16 a, u16 b) {
    s32 i;
    for (i = 0; i < 0x84; i++) {
        if (trainers[i].unk00 == a && trainers[i].unk02 == b) {
            return i;
        }
    }
    return 0;
}

BOOL IsReturnLocationNonLeaguePokeCen(GameData *gameData) {
    u32 leagueIdx = GetLeaguePokeCenReturnLocationIdx();
    u16 returnIdx = GetReturnLocationIdx(gameData);
    BOOL isLeague;

    if (leagueIdx == returnIdx) {
        isLeague = TRUE;
    } else {
        isLeague = FALSE;
    }
    if (isLeague) {
        return FALSE;
    }
    return TRUE;
}
