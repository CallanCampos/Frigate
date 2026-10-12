#include "types.h"
#include "battle/battle_rec_tool.h"
#include "battle/btl_setup.h"
#include "battle/regulation.h"
#include "field/bsubway_scr.h"
#include "field/event_wifi_bsubway.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_gimmick_bsubway.h"
#include "field/field_script.h"
#include "field/player_state.h"
#include "field/scrcmd_bsubway.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_system.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/bsubway_save.h"
#include "save/event_work.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/iss_switch_sys.h"
#include "system/iss_sys.h"
#include "system/vm.h"

// The script plugin of the Battle Subway (plugin 1), commands from 1000. Command 1003 does most of the work, with its
// own command IDs: from 300 they need the subway's work

// Set while the player is on the subway
#define FLAG_BSUBWAY_ON_TRAIN 0x965
#define BSUBWAY_COUNT_MAX 9999

#define OVERLAY_EVENT_WIFI_BSUBWAY OVERLAY_ID(9)

typedef struct {
    s16 x;
    s16 unk2;
    s16 z;
    s16 unk6;
} BSubwayActorPos;

typedef struct {
    u16 objCode;
    u16 stage;
    s16 x;
    u16 unk6;
    s16 z;
    u16 unkA;
    u16 unkC;
    u16 unkE;
} BSubwayActor;

// The message window event of command 44
typedef struct {
    FieldScriptEnv *env;
    u16 *var;
    void *window;
    u32 message;
} BSubwayMsgEvent;

static const u16 sTable7008[] = { 20, 21, 22, 22, 20, 20, 21, 22, 22 };
static const u16 sTable701a[] = { 4, 5, 6, 6, 11, 7, 8, 9, 9 };
static const u16 sTable702c[] = { 44, 11, 12, 48, 72, 28, 13, 71, 30, 32 };
static const u16 sTable7040[] = { 33, 15, 16, 49, 53, 17, 31, 33, 45, 31 };
static const VecFx32 sTable7054[] = {
    { 0x2d0000, -0x20000, 0xb6000 },
    { 0x2f0000, -0x30000, 0x97000 },
};
static const u32 sTable706c[] = { 0, 2, 4, 4, 6, 1, 3, 5, 5 };
static const BSubwayActorPos sActorPositions[] = {
    { 70, 0, 16, 0 }, { 61, 0, 14, 0 }, { 47, 0, 16, 0 }, { 37, 0, 14, 0 }, { 26, 0, 16, 0 },
};
// The zones to warp to for each play mode
static const u32 sWarpZones[] = { 67, 69, 71, 71, 73, 68, 70, 72, 72 };
static const u32 sStages[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 24, 32, 46, 146 };
static const VecFx32 sWarpPositions[] = {
    { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 },
    { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 },
    { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 },
};
// The actors on each stage of the subway, until objCode 0xffff
static const BSubwayActor sActors[] = {
    { 11, 1, 71, 0, 17, 32, 0, 0 },     { 11, 4, 62, 0, 14, 33, 0, 0 },       { 15, 1, 66, 0, 14, 34, 0, 0 },
    { 15, 4, 50, 0, 16, 35, 0, 0 },     { 12, 1, 23, 0, 16, 36, 0, 0 },       { 12, 4, 41, 0, 14, 37, 0, 0 },
    { 16, 1, 35, 0, 16, 38, 0, 0 },     { 16, 4, 40, 0, 14, 39, 0, 0 },       { 48, 1, 61, 0, 13, 40, 0, 0 },
    { 48, 2, 31, 0, 14, 41, 0, 0 },     { 48, 4, 58, 0, 15, 42, 0, 0 },       { 48, 9, 22, 0, 13, 43, 0, 0 },
    { 48, 15, 42, 0, 17, 44, 0, 0 },    { 49, 2, 67, 0, 17, 45, 0, 0 },       { 49, 4, 12, 0, 16, 46, 0, 0 },
    { 49, 8, 38, 0, 14, 47, 0, 0 },     { 49, 12, 28, 0, 16, 48, 0, 0 },      { 72, 2, 44, 0, 15, 49, 0, 0 },
    { 72, 3, 19, 0, 16, 50, 0, 0 },     { 72, 5, 50, 0, 16, 51, 0, 0 },       { 72, 11, 68, 0, 14, 52, 0, 0 },
    { 28, 1, 43, 0, 14, 53, 0, 0 },     { 28, 2, 22, 0, 14, 54, 0, 0 },       { 28, 3, 58, 0, 15, 55, 0, 0 },
    { 28, 5, 61, 0, 16, 56, 0, 0 },     { 28, 6, 21, 0, 14, 57, 58, 51 },     { 28, 8, 20, 0, 16, 59, 0, 0 },
    { 28, 12, 67, 0, 14, 60, 0, 0 },    { 53, 3, 67, 0, 15, 61, 0, 0 },       { 53, 4, 82, 0, 17, 62, 0, 0 },
    { 53, 6, 60, 0, 16, 63, 0, 0 },     { 53, 11, 34, 0, 15, 64, 0, 0 },      { 68, 3, 45, 0, 14, 65, 0, 0 },
    { 68, 5, 12, 0, 15, 66, 0, 0 },     { 68, 6, 78, 0, 15, 67, 0, 0 },       { 68, 7, 19, 0, 16, 68, 0, 0 },
    { 68, 8, 62, 0, 14, 69, 0, 0 },     { 68, 15, 61, 0, 17, 70, 0, 0 },      { 68, 18, 67, 0, 15, 71, 0, 0 },
    { 13, 4, 32, 0, 15, 72, 0, 0 },     { 17, 1, 55, 0, 17, 73, 0, 0 },       { 71, 17, 59, 0, 15, 74, 0, 0 },
    { 30, 9, 69, 0, 17, 75, 0, 0 },     { 30, 10, 81, 0, 14, 76, 0, 0 },      { 30, 12, 49, 0, 14, 77, 0, 0 },
    { 30, 14, 63, 0, 16, 78, 0, 0 },    { 30, 21, 70, 0, 17, 79, 0, 0 },      { 31, 7, 67, 0, 17, 80, 81, 50 },
    { 31, 9, 45, 0, 15, 82, 0, 0 },     { 31, 11, 23, 0, 16, 83, 0, 0 },      { 31, 16, 17, 0, 16, 84, 0, 0 },
    { 31, 20, 45, 0, 15, 85, 86, 207 }, { 61, 1, 83, 0, 17, 87, 0, 0 },       { 61, 2, 50, 0, 16, 88, 0, 0 },
    { 61, 5, 43, 0, 13, 89, 0, 0 },     { 69, 3, 39, 0, 16, 90, 91, 51 },     { 69, 5, 35, 0, 16, 92, 0, 0 },
    { 69, 7, 58, 0, 15, 93, 0, 0 },     { 69, 22, 50, 0, 14, 94, 0, 0 },      { 32, 7, 39, 0, 14, 95, 0, 0 },
    { 32, 9, 62, 0, 16, 96, 0, 0 },     { 32, 11, 53, 0, 14, 97, 0, 0 },      { 32, 13, 59, 0, 15, 98, 0, 0 },
    { 32, 15, 55, 0, 14, 99, 0, 0 },    { 33, 8, 57, 0, 16, 100, 0, 0 },      { 33, 10, 39, 0, 13, 101, 0, 0 },
    { 33, 13, 60, 0, 15, 102, 0, 0 },   { 33, 16, 68, 0, 15, 103, 0, 0 },     { 33, 19, 64, 0, 14, 104, 0, 0 },
    { 45, 3, 25, 0, 14, 105, 0, 0 },    { 45, 5, 82, 0, 15, 106, 0, 0 },      { 45, 8, 50, 0, 15, 107, 0, 0 },
    { 44, 3, 26, 0, 14, 108, 0, 0 },    { 44, 6, 44, 0, 14, 109, 0, 0 },      { 44, 7, 48, 0, 16, 110, 0, 0 },
    { 44, 10, 58, 0, 16, 111, 0, 0 },   { 52, 6, 66, 0, 14, 112, 0, 0 },      { 52, 7, 55, 0, 13, 113, 0, 0 },
    { 52, 10, 20, 0, 16, 114, 0, 0 },   { 52, 18, 28, 0, 14, 115, 116, 206 }, { 0xffff },
};

static BOOL func_ov050_021e69a8(VM *vm, void *env);
static BOOL func_ov050_021e69cc(VM *vm, void *env);
static BOOL func_ov050_021e6a20(VM *vm, void *env);
static BOOL func_ov050_021e6ab4(VM *vm, void *env);
static BOOL func_ov050_021e6b38(VM *vm, void *env);
static GameEvent *func_ov050_021e6be0(GameSystem *gsys, BSubwayScrWork *bsw);
static GameEvent *func_ov050_021e6c88(GameSystem *gsys, FieldScriptEnv *env, u16 *var, u32 message);
static void func_ov050_021e6cb0(BSubwayScrWork *bsw, GameData *gameData, MMSys *mmSys, Field *field);
static u16 func_ov050_021e6e6c(FieldActor *actor);
static BOOL func_ov050_021e6e78(BSubwayScrWork *bsw, u16 index);
static void func_ov050_021e6e9c(BSubwayScrWork *bsw, u16 index, u16 *a, u16 *b, u16 *c);
static BOOL func_ov050_021e6ee0(VM *vm, void *env);
static u32 func_ov050_021e6f60(u32 mode);
static u32 func_ov050_021e6f90(Field *field);
static u16 func_ov050_021e6fd4(PlayerInfo *info);

BOOL func_ov050_021e5800(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *fieldWork = ScriptWork_GetFieldWork(work);
    GameSystem *gsys = ScriptWork_GetGameSystem(work);
    u16 a1 = VM_Read16(vm);

    func_ov033_0217b478(gsys, a1, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL func_ov050_021e5838(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *fieldWork = ScriptWork_GetFieldWork(work);

    func_ov033_0217b468(ScriptWork_GetGameSystem(work));
    return FALSE;
}

BOOL func_ov050_021e5854(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *fieldWork = ScriptWork_GetFieldWork(work);
    GameSystem *gsys = ScriptWork_GetGameSystem(work);

    func_ov033_0217b664(gsys, func_0201794c(GSYS_GetGameData(gsys)));
    return FALSE;
}

BOOL BSubwayCmd_Tool(VM *vm, FieldScriptEnv *env) {
    PartyPkm *pkm;
    BOOL result = FALSE;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *fieldWork = ScriptWork_GetFieldWork(work);
    GameSystem *gsys = ScriptWork_GetGameSystem(work);
    GameData *gameData = GSYS_GetGameData(gsys);
    SaveControl *save = GameData_GetSaveControl(gameData);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    BSubwayScrWork *bsw = func_0201794c(gameData);
    BSubwayPlayData *playData = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_PLAY);
    u32 mode = BSubwayPlayData_GetData(playData, 0, NULL);
    BSubwayScoreData *score = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_SCORE);
    void *data3A = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_3A);
    Field *field = GSYS_GetField(gsys);
    u16 cmd = VM_Read16(vm);
    u16 param0 = ScriptReadAny(vm, env);
    u16 param1 = ScriptReadAny(vm, env);
    u16 retWkId = VM_Read16(vm);
    u16 *retWk = ScriptWork_GetWkAddr(work, gameData, retWkId);
    FieldActor *actor;
    EventWifiBSubwayArgs args;

    if (bsw == NULL && cmd >= 300) {
        return result;
    }
    switch (cmd) {
    case 0:
        *retWk = func_ov050_021e6f90(field);
        break;
    case 1:
        sys_reset(0);
        break;
    case 2:
        BSubwayPlayData_Init(playData);
        break;
    case 3:
        *retWk = BSubwayPlayData_GetSaveFlag(playData);
        break;
    case 4: {
        VecFx32 pos;
        ZoneSpawnInfo spawn;
        FieldPlayer *player = Field_GetPlayer(field);
        u32 dir = FieldPlayer_GetFaceDir(player);
        int warpDir;

        FieldPlayer_GetWPos(player, &pos);
        switch (dir) {
        case DIR_UP:
            warpDir = 1;
            break;
        case DIR_DOWN:
            warpDir = 2;
            break;
        case DIR_LEFT:
            warpDir = 3;
            break;
        case DIR_RIGHT:
            warpDir = 4;
            break;
        default:
            warpDir = 0;
            break;
        }
        CreateZoneChangeData(&spawn, Field_GetPlayerStateZoneID(field), warpDir, pos.x, pos.y, pos.z);
        GameData_SetNextZone(gameData, &spawn);
        EventWork_FlagSet(eventWork, FLAG_BSUBWAY_ON_TRAIN);
        break;
    }
    case 5:
        EventWork_FlagReset(eventWork, FLAG_BSUBWAY_ON_TRAIN);
        break;
    case 6:
        *retWk = BSubwayScoreData_GetStreak(score, param0);
        if (*retWk > BSUBWAY_COUNT_MAX) {
            *retWk = BSUBWAY_COUNT_MAX;
        }
        break;
    case 7:
        *retWk = func_ov033_0217b86c(gsys);
        break;
    case 12:
        *retWk = FieldPlayer_GetObjCodeByForme(FieldPlayer_GetSex(Field_GetPlayer(field)), 0);
        break;
    case 13:
        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        if (actor != NULL) {
            SetActorHidden(actor, param1);
        }
        break;
    case 8:
        *retWk = BSubwayPlayData_GetRound(playData) + 1;
        break;
    case 11:
        *retWk = BSubwayPlayData_GetRound(bsw->unk70);
        *retWk = *retWk + 1;
        break;
    case 14:
        if (sTable701a[param0] == 11) {
            *retWk = TRUE;
        } else {
            *retWk = BSubwayScoreData_SetFlag(score, sTable701a[param0], 0);
        }
        break;
    case 15:
        if (sTable701a[param0] != 11) {
            BSubwayScoreData_SetFlag(score, sTable701a[param0], 1);
        }
        break;
    case 16:
        *retWk = BSubwayScoreData_SetFlag(score, 10, 0);
        break;
    case 17:
        BSubwayScoreData_SetFlag(score, 10, 1);
        break;
    case 18:
        FieldGimmickBSubway_StartTrainEffect(field, sTable706c[param0], &sTable7054[param1]);
        break;
    case 19:
        func_ov036_021c65a8(FieldGimmickBSubway_GetTrainEffect(field), param0);
        break;
    case 20:
        func_ov036_021c65e8(FieldGimmickBSubway_GetTrainEffect(field), param0);
        break;
    case 21:
        *retWk = BSubwayPlayData_GetData(playData, 0, NULL);
        break;
    case 22:
        *retWk = FALSE;
        switch (param0) {
        case 2:
        case 3:
        case 7:
        case 8:
            *retWk = TRUE;
            break;
        }
        break;
    case 23: {
        fx32 y = param1 << 16;
        VecFx32 pos;

        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        if (actor != NULL) {
            CopyActorWPos(actor, &pos);
            pos.y = y;
            SetActorWPosAll(actor, &pos, GetActorFaceDir(actor));
            func_ov012_0216763c(actor, TRUE);
        }
        break;
    }
    case 24:
        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        if (actor != NULL) {
            func_ov012_0216763c(actor, FALSE);
        }
        break;
    case 25:
        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        *retWk = FALSE;
        if (actor != NULL) {
            *retWk = TRUE;
        }
        break;
    case 26:
        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        *retWk = 0;
        if (actor != NULL) {
            *retWk = func_ov050_021e6e6c(actor);
        }
        break;
    case 27:
        *retWk = func_ov033_0217bcb4(score, gsys, 0);
        if ((s16)*retWk <= 0) {
            *retWk = 1;
        }
        break;
    case 28:
        *retWk = func_ov033_0217bcb4(score, gsys, 3);
        break;
    case 29:
        *retWk = func_ov033_0217bcb4(score, gsys, 4);
        break;
    case 30:
        *retWk = BSubwayScoreData_GetStage(score, param0);
        break;
    case 31:
        *retWk = FieldPlayer_GetObjCodeByExState(
            getTrainerGender(&GameData_GetPlayerState(gameData)->playerInfo) == GENDER_MALE ? 1 : 0, 0);
        break;
    case 32: {
        BattleBoxSave *battleBox = getBattleBox(save);

        *retWk = FALSE;
        if (BattleBoxSave_HasPkm(battleBox) == TRUE) {
            *retWk = TRUE;
        }
        break;
    }
    case 33:
        *retWk = 0;
        break;
    case 34:
        FieldGimmickBSubway_StartTrainEffect(field, 7, sTable7054);
        break;
    case 35:
        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        if (actor != NULL) {
            CheckSetActorFaceDir(actor, param1);
        }
        break;
    case 36:
        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        if (actor != NULL) {
            DisableActorMovement(actor);
        }
        break;
    case 37:
        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        ScriptWork_CallEvent(work, func_ov012_0216657c(gsys, GetActorUserParam(actor, 0), param0));
        result = TRUE;
        break;
    case 38:
        *retWk = BSubwayScoreData_HasStage(score, param0);
        break;
    case 39: {
        u16 index;

        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        index = GetActorUserParam(actor, 0);
        *retWk = FALSE;
        if (func_ov050_021e6e78(bsw, index) == TRUE) {
            *retWk = TRUE;
        }
        break;
    }
    case 40: {
        u16 a, b, c;

        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        func_ov050_021e6e9c(bsw, GetActorUserParam(actor, 0), &a, &b, &c);
        *retWk = a;
        break;
    }
    case 41: {
        u16 a, b, c;

        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        func_ov050_021e6e9c(bsw, GetActorUserParam(actor, 0), &a, &b, &c);
        *retWk = b;
        break;
    }
    case 42: {
        u16 a, b, c;

        actor = FindFieldActor(Field_GetActorSystem(field), param0);
        func_ov050_021e6e9c(bsw, GetActorUserParam(actor, 0), &a, &b, &c);
        *retWk = c;
        break;
    }
    case 43: {
        void *list = BSubwayWifiData_AllocLeaders(data3A, HEAPID_GAMEEVENT);

        *retWk = BSubwayWifiLeader_CountList(list);
        GFL_HeapFree(list);
        break;
    }
    case 44: {
        BOOL message = FALSE;

        if (param0 == TRUE) {
            message = TRUE;
        }
        ScriptWork_CallEvent(work, func_ov050_021e6c88(gsys, env, retWk, message));
        result = TRUE;
        break;
    }
    case 45: {
        ZoneSpawnInfo spawn;
        u16 index = func_ov050_021e6f90(field);
        const VecFx32 *pos = &sWarpPositions[index];

        CreateZoneChangeData(&spawn, sWarpZones[index], 4, pos->x, pos->y, pos->z);
        GameData_SetNextZone(gameData, &spawn);
        EventWork_FlagSet(eventWork, FLAG_BSUBWAY_ON_TRAIN);
        break;
    }
    case 100:
        if (param0 == 0) {
            BSubwayScoreData_SetFlag(score, 1, 2);
        } else {
            BSubwayScoreData_SetFlag(score, 1, 1);
        }
        break;
    case 101:
        *retWk = BSubwayScoreData_SetFlag(score, 1, 0);
        break;
    case 103:
        args.mode = 0;
        args.result = retWk;
        ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_WIFI_BSUBWAY,
                                                                   EventWifiBSubway_CreateFromArgs, &args));
        result = TRUE;
        break;
    case 104:
        result = TRUE;
        args.mode = 1;
        args.result = retWk;
        ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_WIFI_BSUBWAY,
                                                                   EventWifiBSubway_CreateFromArgs, &args));
        break;
    case 105:
        args.mode = 2;
        args.result = retWk;
        ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_WIFI_BSUBWAY,
                                                                   EventWifiBSubway_CreateFromArgs, &args));
        result = TRUE;
        break;
    case 106:
        *retWk = BSubwayWifiData_GetPlayerRank(data3A);
        break;
    case 107:
        *retWk = BSubwayWifiData_HasPlayers(data3A);
        break;
    case 108: {
        u8 value = param0;

        BSubwayPlayData_SetData(playData, 11, &value);
        break;
    }
    case 109:
        *retWk = BSubwayPlayData_GetData(playData, 11, NULL);
        break;
    case 110:
        *retWk = BSubwayWifiData_HasLeaders(data3A);
        break;
    case 111:
        ScriptWork_CallEvent(work, func_ov012_02166294(gsys));
        result = TRUE;
        break;
    case 112:
        *retWk = BSubwayWifiData_GetLeaderRank(data3A);
        break;
    case 113:
        *retWk = BSubwayWifiData_GetLeaderRoom(data3A);
        break;
    case 200:
        *retWk = 0;
        break;
    case 203:
        *retWk = 0;
        break;
    case 322:
        if (mode < 9) {
            ZoneSpawnInfo spawn;
            const VecFx32 *pos = &sWarpPositions[mode];

            CreateZoneChangeData(&spawn, sWarpZones[mode], 4, pos->x, pos->y, pos->z);
            GameData_SetNextZone(gameData, &spawn);
            EventWork_FlagSet(eventWork, FLAG_BSUBWAY_ON_TRAIN);
        }
        break;
    case 300: {
        u8 value = BSubwayPlayData_GetData(playData, 10, NULL);

        ScriptWork_CallEvent(work, func_ov012_02165f70(bsw, gsys, value));
        result = TRUE;
        break;
    }
    case 301:
        *retWk = func_ov033_0217ba94(bsw, gsys);
        break;
    case 302:
        *retWk = 0;
        break;
    case 303:
        if (func_ov033_0217bb20(bsw) == TRUE) {
            *retWk = 1;
        } else {
            *retWk = 0;
        }
        break;
    case 304:
        func_ov033_0217b8ac(gsys, bsw);
        break;
    case 305:
        func_ov033_0217bb4c(bsw, gsys);
        break;
    case 306:
        func_ov033_0217b708(bsw);
        break;
    case 307:
        func_ov033_0217bbac(bsw);
        break;
    case 308:
        *retWk = func_ov033_0217bca0(bsw, param0);
        break;
    case 309:
        ScriptWork_CallEvent(work, func_ov012_02166070(bsw, gsys, field));
        result = TRUE;
        break;
    case 310:
        *retWk = bsw->playMode;
        break;
    case 311:
        bsw->unkC_1 = param0;
        break;
    case 312:
        *retWk = bsw->unkC_1;
        break;
    case 313:
        *retWk = func_ov033_0217b8ec(bsw);
        break;
    case 314:
        bsw->unkC_5 = param0;
        break;
    case 315:
        *retWk = bsw->unkC_5;
        break;
    case 316:
        func_ov033_0217b9dc(bsw);
        break;
    case 317:
        *retWk = bsw->memberSlots[param0];
        break;
    case 319:
        bsw->recvCount = 0;
        sys_memset(bsw->recvBuf, 0, sizeof(bsw->recvBuf));
        break;
    case 320:
        ScriptWork_CallEvent(work, func_ov012_02166118(bsw, gsys, param0, param1, 5));
        result = TRUE;
        break;
    case 321:
        func_ov033_0217b7e8(bsw);
        break;
    case 323:
        func_ov033_0217b790(bsw, gsys);
        break;
    case 324: {
        u8 value = param0;

        BSubwayPlayData_SetData(playData, 10, &value);
        break;
    }
    case 325:
        func_ov033_0217b6b4(bsw);
        break;
    case 326:
        if (mode == 5 || mode == 6 || mode == 7 || mode == 8) {
            *retWk = 1;
        } else {
            *retWk = 0;
        }
        break;
    case 329:
        func_ov050_021e6cb0(bsw, gameData, Field_GetActorSystem(field), field);
        break;
    case 330:
        bsw->unkC_8 = param0;
        break;
    case 331:
        *retWk = bsw->unkC_8;
        break;
    case 332:
        if (bsw->btlSetup == NULL) {
            *retWk = 0;
        } else if (bsw->btlSetup->unkA8 == 1) {
            *retWk = 1;
        } else {
            *retWk = 0;
        }
        break;
    case 333:
        func_ov033_0217bb98(bsw, gsys);
        break;
    // Fades in the switches for the train's cars
    case 334: {
        ISSSwitchIndex i = 0;
        u16 count = BSubwayPlayData_GetRound(playData);
        ISSSwitchSys *switchSys;

        if (count != 0) {
            switchSys = ISS_GetSwitchSys(GameSystem_GetISS(gsys));
            count = i + count + 1;
            for (; i < count; i++) {
                if (i < ISS_SWITCH_COUNT && i != 0) {
                    ISSSwitchSys_ReqSwitchFadeIn(switchSys, i);
                }
            }
        }
        break;
    }
    case 335:
        func_ov033_0217be88(bsw,
                            getTrainerGender(&GameData_GetPlayerState(gameData)->playerInfo) == GENDER_MALE ? 1 : 0);
        break;
    case 336:
        *retWk = bsw->memberSpecies[param0];
        break;
    case 337:
        bsw->unkC_9 = param0;
        break;
    case 338:
        *retWk = bsw->unkC_9;
        break;
    case 339: {
        PokeParty *party = NULL;
        u32 useBattleBox;
        Regulation *regulation;
        PokeParty *checked;
        u32 check;

        *retWk = 0;
        useBattleBox = BSubwayPlayData_GetData(playData, 10, NULL);
        regulation = PokeRegulation_LoadNew(func_ov050_021e6f60(mode), HEAPID_GAMEEVENT);
        if (useBattleBox == TRUE) {
            party = convertBoxedPokeSetToParty(getBattleBox(save), HEAPID_GAMEEVENT);
            checked = party;
        } else {
            checked = GameData_GetParty(gameData);
        }
        check = PokeRegulation_CheckPartyCanEnter(regulation, checked);
        GFL_HeapFree(regulation);
        if (party != NULL) {
            GFL_HeapFree(party);
        }
        if (check <= 1) {
            *retWk = 1;
        }
        break;
    }
    case 340:
        func_ov033_0217bd34(bsw);
        break;
    case 341:
        *retWk = func_ov050_021e6f60(mode);
        break;
    case 342:
        *retWk = func_ov033_0217bdc0(mode);
        break;
    case 343: {
        u32 buffer;

        if (bsw->unkC_10 == 0) {
            bsw->unkC_10 = 1;
            if (func_0200bcf8(save, 4, &buffer, 0) == 1) {
                bsw->unkC_10 = 2;
            }
        }
        break;
    }
    case 344:
        if (bsw->unkC_10 == 1) {
            *retWk = 0;
        } else {
            *retWk = 1;
        }
        break;
    case 345:
        func_0200c1f0();
        BattleRecTool_StoreSetup(bsw->btlSetup);
        func_0200c200();
        break;
    case 346:
        bsw->unk7EC = 0;
        bsw->unk7EE = 0;
        bsw->unk10 = param0;
        VM_SetNativeCallback(vm, func_ov050_021e6ee0);
        bsw->unkC_10 = 2;
        result = TRUE;
        break;
    case 347:
        if (bsw->btlSetup != NULL) {
            freeVSPlayerBlkClearPtr();
            BtlSetup_Free(bsw->btlSetup);
            bsw->btlSetup = NULL;
        }
        break;
    case 348:
        sys_memset(&bsw->ov306Param, 0, sizeof(bsw->ov306Param));
        bsw->ov306Param.gameData = gameData;
        bsw->ov306Param.unk4 = 1;
        bsw->ov306Param.unk8 = 0;
        bsw->ov306Param.unkC = 1;
        bsw->ov306Param.unk10 = sTable7008[mode];
        bsw->ov306Param.unk14 = func_ov033_0217bd84(bsw);
        if (bsw->ov306Param.unk14 > BSUBWAY_COUNT_MAX) {
            bsw->ov306Param.unk14 = BSUBWAY_COUNT_MAX;
        }
        ScriptWork_CallEvent(
            work, EventFieldSubprocessTransition_CreateWithCallback(gsys, field, OVERLAY_ID(306), &BtlRecSelProcFunctions, &bsw->ov306Param, NULL, NULL));
        result = TRUE;
        break;
    case 349:
        BSubwayPlayData_ResetRound(playData);
        BSubwayScoreData_ResetStage(score, mode);
        break;
    case 350:
        FieldGimmickBSubway_StopShake(field);
        break;
    case 351: {
        u16 index = BSubwayPlayData_GetRound(playData);

        switch (mode) {
        case 2:
        case 3:
        case 7:
        case 8:
            *retWk = bsw->unk32[param0 + index * 2];
            break;
        default:
            *retWk = bsw->unk32[index];
            break;
        }
        break;
    }
    case 352:
        ScriptWork_CallEvent(work, func_ov012_02166118(bsw, gsys, param0, param1, 6));
        result = TRUE;
        break;
    case 353:
        if (bsw->btlSetup != NULL) {
            u8 a = bsw->btlSetup->unkD2;
            u8 b = bsw->btlSetup->unkDB;
            int count;
            PokeParty *party = bsw->btlSetup->party[0];
            int i;
            u32 hp;
            u32 hpSum;
            u32 maxHpSum;

            count = PokeParty_GetPkmCount(party);
            hpSum = 0;
            maxHpSum = 0;
            for (i = 0; i < count; i++) {
                pkm = PokeParty_GetPkm(party, i);
                hp = PokeParty_GetParam(pkm, 0xa0, NULL);
                maxHpSum += PokeParty_GetParam(pkm, 0xa1, NULL);
                hpSum += hp;
            }
            BSubwayPlayData_AddWifiRecord(playData, b, a, maxHpSum - hpSum);
        }
        break;
    case 354:
        bsw->unkC_12 = param0;
        break;
    case 355:
        *retWk = bsw->unkC_12;
        break;
    case 356:
        *retWk = BSubwayPlayData_GetData(playData, 10, NULL);
        break;
    case 357:
        BSubwayPlayData_IncRound(playData);
        func_ov033_0217bd8c(bsw);
        *retWk = func_ov033_0217bd84(bsw);
        if (*retWk > BSUBWAY_COUNT_MAX) {
            *retWk = BSUBWAY_COUNT_MAX;
        }
        break;
    case 358:
        *retWk = func_ov033_0217bd84(bsw);
        if (*retWk > BSUBWAY_COUNT_MAX) {
            *retWk = BSUBWAY_COUNT_MAX;
        }
        break;
    case 359:
        if (BSubwayScoreData_HasStage(score, mode) == 0) {
            BSubwayScoreData_StartStage(score, mode);
            BSubwayPlayData_ResetRound(playData);
            func_ov033_0217bda0(bsw);
        }
        break;
    case 400:
        if (bsw->unkC_9 == TRUE) {
            func_ov012_02161894(bsw);
        } else {
            func_ov012_02161844(bsw);
        }
        break;
    case 401:
        ScriptWork_CallEvent(work, func_ov050_021e6be0(gsys, bsw));
        result = TRUE;
        break;
    case 402:
        bsw->unk727 = param0;
        bsw->unk72A = retWkId;
        func_ov012_021618b8(bsw->unk727);
        VM_SetNativeCallback(vm, func_ov050_021e69cc);
        result = TRUE;
        break;
    case 403:
        bsw->resultVar = retWk;
        bsw->unk71C = func_ov036_021c3d9c(GetGameDataPlayerInfo(gameData), field, 2, 2, 4, 0, 2, 0);
        VM_SetNativeCallback(vm, func_ov050_021e6a20);
        result = TRUE;
        break;
    case 404:
        bsw->resultVar = retWk;
        bsw->unk71C = func_ov036_021c3d9c(GetGameDataPlayerInfo(gameData), field, 2, 2, 4, 1, 2, 0);
        result = TRUE;
        VM_SetNativeCallback(vm, func_ov050_021e6ab4);
        break;
    case 405:
        func_ov012_02161990(bsw, param0, param1);
        VM_SetNativeCallback(vm, func_ov050_021e69a8);
        result = TRUE;
        break;
    case 406:
        func_ov012_02161a88(bsw, param0);
        bsw->unk72A = retWkId;
        VM_SetNativeCallback(vm, func_ov050_021e6b38);
        result = TRUE;
        break;
    case 407:
        *retWk = func_0203ffc4();
        break;
    case 408:
        sys_memset(&bsw->ircMatchParam, 0, sizeof(bsw->ircMatchParam));
        bsw->ircMatchParam.gameData = gameData;
        bsw->ircMatchParam.result = IRCBATTLEMATCH_MODE_BSUBWAY;
        ScriptWork_CallEvent(work, EventFieldSubprocessTransition_CreateWithCallback(gsys, field, OVERLAY_IRCBATTLEMATCH, &IRCBATTLEMATCH_PROC_FUNCTIONS,
                                                 &bsw->ircMatchParam, NULL, NULL));
        result = TRUE;
        break;
    case 409:
        switch (bsw->ircMatchParam.result) {
        case IRCBATTLEMATCH_MODE_BSUBWAY:
            *retWk = 0;
            break;
        case IRCBATTLEMATCH_RESULT_QUIT:
            *retWk = 1;
            break;
        case IRCBATTLEMATCH_RESULT_BACK:
            *retWk = 2;
            break;
        default:
            *retWk = 1;
            break;
        }
        break;
    case 410:
        Misc_AddMetTrainer(getHollow_RivalData(save), bsw->partner.id);
        break;
    case 411:
        *retWk = func_0203ffc4() == FALSE ? FieldPlayer_GetObjCodeByForme(FieldPlayer_GetSex(Field_GetPlayer(field)), 0)
                                          : func_ov050_021e6fd4(&bsw->partner);
        break;
    case 412:
        *retWk = func_0203ffc4() == FALSE
                     ? func_ov050_021e6fd4(&bsw->partner)
                     : FieldPlayer_GetObjCodeByForme(FieldPlayer_GetSex(Field_GetPlayer(field)), 0);
        break;
    case 413:
        GFL_NetErrMarkShown();
        break;
    case 414:
        func_02016b0c(gsys, 1);
        break;
    case 415:
        *retWk = FALSE;
        if (GFL_NetErrCheck()) {
            *retWk = TRUE;
        }
        break;
    case 416:
        *retWk = FALSE;
        if (bsw->btlSetup != NULL && func_0200c1d0(bsw->btlSetup->unkAD) == TRUE) {
            *retWk = TRUE;
        }
        break;
    case 102:
    case 201:
    case 202:
    case 501:
        break;
    }
    return result;
}

static BOOL func_ov050_021e69a8(VM *vm, void *env) {
    BSubwayScrWork *bsw = func_0201794c(GSYS_GetGameData(ScriptWork_GetGameSystem(FieldScriptEnv_GetScriptWork(env))));

    if (func_ov012_02161a48(bsw) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov050_021e69cc(VM *vm, void *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(ScriptWork_GetGameSystem(work));
    BSubwayScrWork *bsw = func_0201794c(gameData);
    u16 *var = ScriptWork_GetWkAddr(work, gameData, bsw->unk72A);

    if (GFL_NetErrCheck()) {
        *var = 0;
        return TRUE;
    }
    if (func_ov012_021618c8(bsw->unk727) == TRUE) {
        *var = 1;
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov050_021e6a20(VM *vm, void *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *fieldWork = ScriptWork_GetFieldWork(work);
    GameData *gameData = GSYS_GetGameData(ScriptWork_GetGameSystem(work));
    SaveControl *save = GameData_GetSaveControl(gameData);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    BSubwayScrWork *bsw = func_0201794c(gameData);
    u32 mode = BSubwayPlayData_GetData(SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_PLAY), 0, NULL);
    BSubwayScoreData *score = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_SCORE);
    u32 answer = func_ov036_021c3f98(bsw->unk71C);

    if (answer != 0) {
        func_ov036_021c3eb4(bsw->unk71C);
        bsw->unk71C = NULL;
        switch (answer) {
        case 1:
            *bsw->resultVar = 0;
            return TRUE;
        case 2:
            *bsw->resultVar = 1;
            return TRUE;
        case 4:
        default:
            *bsw->resultVar = 2;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL func_ov050_021e6ab4(VM *vm, void *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *fieldWork = ScriptWork_GetFieldWork(work);
    GameData *gameData = GSYS_GetGameData(ScriptWork_GetGameSystem(work));
    SaveControl *save = GameData_GetSaveControl(gameData);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    BSubwayScrWork *bsw = func_0201794c(gameData);
    u32 mode = BSubwayPlayData_GetData(SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_PLAY), 0, NULL);
    BSubwayScoreData *score = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_SCORE);
    u32 answer = func_ov036_021c3f98(bsw->unk71C);

    if (answer != 0) {
        func_ov036_021c3eb4(bsw->unk71C);
        bsw->unk71C = NULL;
        switch (answer) {
        case 1:
            *bsw->resultVar = 0;
            return TRUE;
        case 2:
        default:
            *bsw->resultVar = 1;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL func_ov050_021e6b38(VM *vm, void *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(ScriptWork_GetGameSystem(work));
    BSubwayScrWork *bsw = func_0201794c(gameData);

    if (func_ov012_02161a94(bsw, ScriptWork_GetWkAddr(work, gameData, bsw->unk72A)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static GameEventReturnCode func_ov050_021e6b78(GameEvent *event, u32 *state, void *data) {
    BSubwayScrWork **bsw = data;

    switch (*state) {
    case 0:
        if (func_02042bc4() == TRUE) {
            func_02042e9c(FALSE);
        }
        if ((*bsw)->unkC_9 == TRUE) {
            func_ov012_021618ac(*bsw);
        }
        (*state)++;
        break;
    case 1:
        if (func_02042bc4() == TRUE) {
            if (func_02042a78() <= 1) {
                func_02042860(0);
                (*state)++;
            }
        } else {
            func_02042860(0);
            (*state)++;
        }
        break;
    case 2:
        if (func_020427a4() == TRUE) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *func_ov050_021e6be0(GameSystem *gsys, BSubwayScrWork *bsw) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov050_021e6b78, sizeof(BSubwayScrWork *));
    BSubwayScrWork **data = GameEvent_GetData(event);

    *data = bsw;
    return event;
}

static GameEventReturnCode func_ov050_021e6c00(GameEvent *event, u32 *state, void *data) {
    BSubwayMsgEvent *wk = data;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(wk->env);

    switch (*state) {
    case 0:
        FieldScriptEnv_SetWaitCounter(wk->env, 1);
        (*state)++;
    case 1:
        if (FieldScriptEnv_UpdateWaitCounter(wk->env) == TRUE) {
            wk->window = func_ov036_021880d4(Field_GetMsgBGSys(GSYS_GetField(FieldScriptEnv_GetGameSystem(wk->env))),
                                             wk->message);
            (*state)++;
        }
        break;
    case 2:
        switch (func_ov036_0218816c(wk->window)) {
        case 0:
            *wk->var = 0;
            (*state)++;
            break;
        case 2:
            break;
        default:
            *wk->var = 1;
            (*state)++;
            break;
        }
        break;
    case 3:
        func_ov036_02187ea0(wk->window);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *func_ov050_021e6c88(GameSystem *gsys, FieldScriptEnv *env, u16 *var, u32 message) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov050_021e6c00, sizeof(BSubwayMsgEvent));
    BSubwayMsgEvent *wk = GameEvent_GetData(event);

    wk->env = env;
    wk->var = var;
    wk->message = message;
    return event;
}

typedef struct {
    u8 unk0[0x20];
    u8 unk20_0 : 1;
    u8 female : 1;
    u8 unk20_2 : 6;
    u8 unk21;
} BSubwayRecord;

// Places the subway's passengers: in mode 4 the players of the records in block 0x3a, and otherwise the actors of the
// stage the player has reached
static void func_ov050_021e6cb0(BSubwayScrWork *bsw, GameData *gameData, MMSys *mmSys, Field *field) {
    int i = 0;
    u32 zoneId = Field_GetPlayerStateZoneID(field);
    u16 id = 0x80;
    u8 mode = bsw->playMode;

    if (mode == 4) {
        const BSubwayActorPos *pos = sActorPositions;
        BSubwayRecord *list = BSubwayWifiData_AllocLeaders(
            SaveControl_GetBlockPtr(GameData_GetSaveControl(gameData), SAVE_BLOCK_BSUBWAY_3A), HEAPID_GAMEEVENT);
        int count = BSubwayWifiLeader_CountList(list);
        BSubwayRecord *record;
        FieldActor *actor;
        u32 index;
        u16 objCode;

        if (count != 0) {
            record = &list[count - 1];
            while (count != 0 && i < 5) {
                index = BSubwayWifiLeader_GetID(record) % 10;
                if (record->female == FALSE) {
                    objCode = sTable702c[index];
                } else {
                    objCode = sTable7040[index];
                }
                actor = CreateNewActorByParam(mmSys, pos->x, pos->z, i % 4, id, objCode, 2, zoneId);
                SetActorUserParam(actor, count - 1, 0);
                SetActorSCRID(actor, 5);
                i++;
                record--;
                id++;
                count--;
                pos++;
            }
        }
        GFL_HeapFree(list);
        return;
    }
    {
        const BSubwayActor *entry = sActors;
        u16 stage = BSubwayScoreData_GetStage(bsw->unk74, mode);
        FieldActor *actor;

        switch (mode) {
        case 5:
        case 6:
        case 7:
        case 8:
            stage += 3;
            break;
        }
        while (entry->objCode != 0xffff) {
            if (sStages[entry->stage - 1] == stage) {
                actor = CreateNewActorByParam(mmSys, entry->x, entry->z, i % 4, id, entry->objCode, 2, zoneId);
                SetActorUserParam(actor, i, 0);
                SetActorUserParam(actor, entry->unkA, 1);
                SetActorSCRID(actor, 5);
                if (id < 0xff) {
                    id++;
                }
            }
            entry++;
            i++;
        }
    }
}

static u16 func_ov050_021e6e6c(FieldActor *actor) {
    return GetActorUserParam(actor, 1);
}

static BOOL func_ov050_021e6e78(BSubwayScrWork *bsw, u16 index) {
    const BSubwayActor *entry = &sActors[index];

    if (bsw->playMode == 4) {
        return FALSE;
    }
    if (entry->unkE != 0) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov050_021e6e9c(BSubwayScrWork *bsw, u16 index, u16 *a, u16 *b, u16 *c) {
    const BSubwayActor *entry = &sActors[index];

    if (bsw->playMode == 4) {
        *a = 0;
        *b = 0;
        *c = 4;
        return;
    }
    if (entry->unkE == 0) {
        *a = 0;
        *b = 0;
        *c = 4;
        return;
    }
    *a = entry->unkA;
    *b = entry->unkC;
    *c = entry->unkE;
}

static BOOL func_ov050_021e6ee0(VM *vm, void *env) {
    GameData *gameData = GSYS_GetGameData(ScriptWork_GetGameSystem(FieldScriptEnv_GetScriptWork(env)));
    BSubwayScrWork *bsw = func_0201794c(gameData);
    u32 regulation = 20;
    int count = bsw->unk10;
    u32 result;

    switch (bsw->playMode) {
    case 1:
    case 6:
        regulation = 21;
        break;
    case 2:
    case 3:
    case 7:
    case 8:
        regulation = 22;
        break;
    }
    if (count > BSUBWAY_COUNT_MAX) {
        count = BSUBWAY_COUNT_MAX;
    }
    result = func_0200be50(gameData, 4, regulation, count, 0, &bsw->unk7EC, &bsw->unk7EE);
    if (result == 2 || result == 3) {
        return TRUE;
    }
    return FALSE;
}

// The regulation of a play mode, in arc 106
static u32 func_ov050_021e6f60(u32 mode) {
    u32 regulation = 20;

    switch (mode) {
    case 1:
    case 6:
        regulation = 21;
        break;
    case 2:
    case 3:
    case 7:
    case 8:
        regulation = 22;
        break;
    }
    return regulation;
}

static u32 func_ov050_021e6f90(Field *field) {
    u16 zoneId = Field_GetPlayerStateZoneID(field);
    u32 index = 0;

    switch (zoneId) {
    case 67:
        index = 0;
        break;
    case 68:
        index = 5;
        break;
    case 69:
        index = 1;
        break;
    case 70:
        index = 6;
        break;
    case 71:
        index = 2;
        break;
    case 72:
        index = 7;
        break;
    case 73:
        index = 4;
        break;
    }
    return index;
}

static u16 func_ov050_021e6fd4(PlayerInfo *info) {
    u8 unk1B = func_02008bfc(info);
    u32 gender = getTrainerGender(info);

    if (unk1B == 22 || unk1B == 23) {
        if (gender == GENDER_MALE) {
            return 0xe7;
        }
        return 0xf0;
    }
    if (gender == GENDER_MALE) {
        return 1;
    }
    return 4;
}
