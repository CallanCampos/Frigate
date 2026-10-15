#include "types.h"
#include "app/bag.h"
#include "app/chihou_zukan_award.h"
#include "app/mailbox.h"
#include "app/marine_tube_board.h"
#include "app/monolith.h"
#include "app/psel.h"
#include "app/subway_map.h"
#include "app/zenkoku_zukan_award.h"
#include "app/zukan_award.h"
#include "field/entree_forest.h"
#include "field/event_3d_demo.h"
#include "field/event_battle_video.h"
#include "field/event_wifibattlematch.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/game_beacon_set.h"
#include "field/gimmick_state.h"
#include "field/intrude_work.h"
#include "field/scrcmd_proc.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "pml/move_reminder.h"
#include "pml/poke_party.h"
#include "save/high_link.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

struct BagScriptResult {
    u16 *hasSelection;
    u16 *item;
};


BOOL s014A_FieldOpen(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork_CallEvent(work, EventFieldOpen_CreateHeadless(gsys));
    return TRUE;
}

BOOL s014B_FieldClose(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    ScriptWork_CallEvent(work, CreateFieldCloseEvent(gsys, field));
    return TRUE;
}

void CreateScrCmdOverlayProcess(VM *vm, FieldScriptEnv *env, s32 overlayId, const GameProcFunctions *functions,
                                void *resource, void (*cleanup)(ScriptOverlayWork *), void *data) {
    GameSystem *gsys;
    ScriptWork *scriptWork;
    void **heapPtr;
    ScriptOverlayWork *work;

    gsys = FieldScriptEnv_GetGameSystem(env);
    scriptWork = FieldScriptEnv_GetScriptWork(env);
    heapPtr = ScriptWork_GetUserHeapPtr(scriptWork);
    work = GFL_HeapAllocate(4, sizeof(ScriptOverlayWork), TRUE, "scrcmd_proc.c", 0x8b);
    work->resource = resource;
    work->data = data;
    work->cleanup = cleanup;
    GSYS_QueueProc(gsys, overlayId, functions, resource);
    *heapPtr = work;
    VM_SetNativeCallback(vm, func_ov012_02157554);
}

BOOL func_ov012_02157554(VM *vm, void *data) {
    FieldScriptEnv *env = data;
    GameSystem *gsys;
    ScriptWork *scriptWork;
    ScriptOverlayWork *work;

    gsys = FieldScriptEnv_GetGameSystem(env);
    scriptWork = FieldScriptEnv_GetScriptWork(env);
    work = ScriptWork_GetUserHeap(scriptWork);
    if (GSYS_GetProcMgrState(gsys) != 0) {
        return FALSE;
    }
    if (work->cleanup != NULL) {
        work->cleanup(work);
    } else {
        if (work->resource != NULL) {
            GFL_HeapFree(work->resource);
        }
        if (work->data != NULL) {
            GFL_HeapFree(work->data);
        }
    }
    ScriptWork_FreeUserHeap(scriptWork);
    return TRUE;
}

BOOL s014C_RTFreeUserHeap(VM *vm, FieldScriptEnv *env) {
    ScriptWork_FreeUserHeap(FieldScriptEnv_GetScriptWork(env));
    return TRUE;
}

void func_ov012_021575b8(ScriptOverlayWork *work) {
    BagProcessData *bag = work->resource;
    BagScriptResult *result = work->data;
    if (bag->result == 0) {
        *result->hasSelection = FALSE;
    } else {
        *result->hasSelection = TRUE;
    }
    *result->item = bag->item;
    GFL_HeapFree(work->data);
    GFL_HeapFree(work->resource);
}

BOOL s014E_CallBag(VM *vm, FieldScriptEnv *env) {
    u16 mode;
    u16 *hasSelection;
    u16 *item;
    BagProcessData *bag;
    BagScriptResult *result;

    FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetScriptWork(env);
    mode = ScriptReadAny(vm, env);
    hasSelection = ScriptReadVar(vm, env);
    item = ScriptReadVar(vm, env);
    if (mode == 0) {
        mode = 4;
    } else if (mode == 1) {
        mode = 5;
    } else {
        mode = 0;
    }
    bag = BagParam_Create(FieldScriptEnv_GetGameData(env), NULL, mode, HEAPID_GAMEEVENT);
    result = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(BagScriptResult), TRUE, "scrcmd_proc.c", 280);
    result->hasSelection = hasSelection;
    result->item = item;
    CreateScrCmdOverlayProcess(vm, env, OVERLAY_BAG, &BAG_PROC_FUNCTIONS, bag, func_ov012_021575b8, result);
    return TRUE;
}

void func_ov012_0215767c(void *arg) {
    ScriptProcCallbackWork *work = arg;
    MailboxProcessData *mailbox = work->resource;

    if (mailbox->result == 1) {
        *(u16 *)work->data = TRUE;
    } else {
        *(u16 *)work->data = FALSE;
    }
    GFL_HeapFree(work->resource);
}

BOOL s0150_CallMailbox(VM *vm, FieldScriptEnv *env) {
    ScriptWork *scriptWork;
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    MailboxProcessData *mailbox;
    ScriptProcCallbackWork *work;

    scriptWork = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = GSYS_GetGameData(gsys);
    field = GSYS_GetField(gsys);
    mailbox = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MailboxProcessData), FALSE, "scrcmd_proc.c", 335);
    work = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(ScriptProcCallbackWork), FALSE, "scrcmd_proc.c", 336);
    mailbox->gameData = gameData;
    work->resource = mailbox;
    work->data = ScriptReadVar(vm, env);
    ScriptWork_CallEvent(scriptWork, EventFieldSubprocessCall_CreateWithCallback(gsys, field, OVERLAY_MAILBOX,
                                                                               &MAILBOX_PROC_FUNCTIONS, mailbox,
                                                                               func_ov012_0215767c, work));
    return TRUE;
}

void func_ov012_02157728(void *arg) {
    ScriptProcCallbackWork *work = arg;
    MoveReminderProcessData *data;
    u16 result;

    data = work->resource;
    switch (data->status) {
    case 0:
    default:
        result = FALSE;
        break;
    case 1:
        result = TRUE;
        break;
    }
    *(u16 *)work->data = result;
    GFL_HeapFree(((MoveReminderProcessData *)work->resource)->moves);
    func_ov012_02169ca4(work->resource);
}

BOOL s01D6_MoveReminderCallMoveSelect(VM *vm, FieldScriptEnv *env) {
    ScriptWork *scriptWork;
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    u16 *result;
    u16 slot;
    PartyPkm *pkm;
    PlayerInfo *playerInfo;
    u16 *moves;
    MoveReminderProcessData *data;
    ScriptProcCallbackWork *work;
    // An enum variable, so that MWCC loads the 1 again for the return value
    MoveReminderMode mode = MOVE_REMINDER_MODE_REMEMBER;

    scriptWork = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = GSYS_GetGameData(gsys);
    field = GSYS_GetField(gsys);
    result = ScriptReadVar(vm, env);
    slot = ScriptReadAny(vm, env);
    pkm = PokeParty_GetPkm(GameData_GetParty(gameData), slot);
    playerInfo = GetGameDataPlayerInfo(gameData);
    moves = PokeParty_GetRememberableMoves(pkm, HEAPID_GAMEEVENT);
    data = func_ov012_02169c7c(HEAPID_GAMEEVENT);
    data->pkm = pkm;
    data->playerInfo = playerInfo;
    data->trainerData = NULL;
    data->gsys = gsys;
    data->moves = moves;
    data->mode = mode;
    work = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(ScriptProcCallbackWork), FALSE, "scrcmd_proc.c", 411);
    work->resource = data;
    work->data = result;
    ScriptWork_CallEvent(scriptWork, EventFieldSubprocessTransition_CreateWithCallback(gsys, field, OVERLAY_MOVE_REMINDER, &WazaOshieProcFunctions, data,
                                                   func_ov012_02157728, work));
    return TRUE;
}

BOOL func_ov012_02157814(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameCommSys *commSys;
    GameData *gameData;
    MonolithParam *param;
    u8 *saveBytes;
    HighLinkSave *highLink;

    gsys = FieldScriptEnv_GetGameSystem(env);
    commSys = GSYS_GetGameCommSystem(gsys);
    gameData = GSYS_GetGameData(gsys);
    FieldScriptEnv_GetScriptWork(env);
    func_ov012_02153608(commSys);
    param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MonolithParam), TRUE, "scrcmd_proc.c", 444);
    saveBytes = param->powerFlags;
    highLink = getHighLinkBlockAddress(GameData_GetSaveControl(gameData));
    sys_memset(saveBytes, 0, sizeof(param->powerFlags));
    func_0200c6d8(highLink, saveBytes, 2);
    saveBytes[2] = 1;
    param->unk31 = 0;
    param->gsys = gsys;
    param->netId = 0;
    CreateScrCmdOverlayProcess(vm, env, OVERLAY_MONOLITH, &MONOLITH_PROC_FUNCTIONS, param, NULL, NULL);
    return TRUE;
}

BOOL s0154_Call3DDemo(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    u16 demoId;
    u16 param;
    GameEvent *parent;
    GameEvent *event;

    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    demoId = ScriptReadAny(vm, env);
    param = ScriptReadAny(vm, env);
    parent = ScriptWork_GetEvent(work);
    event = Event3DDemo_Create(gsys, parent, demoId, param, 0);
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL s0151_CallPokedexDiploma(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameData *gameData;
    PlayerInfo *playerInfo;
    u16 mode;
    u16 value;
    ZukanAwardParam *award;
    SubwayMapParam *subwayMap;
    MarineTubeBoardParam *board;

    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = GSYS_GetGameData(gsys);
    playerInfo = GetGameDataPlayerInfo(gameData);
    mode = ScriptReadAny(vm, env);
    value = ScriptReadAny(vm, env);
    if (mode <= 1) {
        func_ov012_0215fdbc();
    }
    switch (mode) {
    case 0:
        award = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(ZukanAwardParam), TRUE, "scrcmd_proc.c", 546);
        award->gameData = gameData;
        award->unk04 = value;
        CreateScrCmdOverlayProcess(vm, env, OVERLAY_CHIHOU_ZUKAN_AWARD, &CHIHOU_ZUKAN_AWARD_PROC_FUNCTIONS, award, NULL, NULL);
        break;
    case 1:
        award = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(ZukanAwardParam), TRUE, "scrcmd_proc.c", 554);
        award->gameData = gameData;
        award->unk04 = value;
        CreateScrCmdOverlayProcess(vm, env, OVERLAY_ZENKOKU_ZUKAN_AWARD, &ZENKOKU_ZUKAN_AWARD_PROC_FUNCTIONS, award, NULL, NULL);
        break;
    case 2:
        subwayMap = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(SubwayMapParam), TRUE, "scrcmd_proc.c", 563);
        subwayMap->playerInfo = playerInfo;
        CreateScrCmdOverlayProcess(vm, env, OVERLAY_SUBWAY_MAP, &SUBWAY_MAP_PROC_FUNCTIONS, subwayMap, NULL, NULL);
        break;
    case 3:
        board = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MarineTubeBoardParam), TRUE, "scrcmd_proc.c", 570);
        board->gsys = gsys;
        board->board = value;
        CreateScrCmdOverlayProcess(vm, env, OVERLAY_MARINE_TUBE_BOARD, &MARINE_TUBE_BOARD_PROC_FUNCTIONS, board, NULL,
                                   NULL);
        break;
    }
    return TRUE;
}

BOOL s0153_callPoke3Select(VM *vm, FieldScriptEnv *env) {
    u16 *result;
    PselParam *param;

    result = ScriptReadVar(vm, env);
    param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PselParam), TRUE, "scrcmd_proc.c", 597);
    param->result = result;
    CreateScrCmdOverlayProcess(vm, env, OVERLAY_PSEL, &PselProcFunctions, param, NULL, NULL);
    return TRUE;
}

BOOL func_ov012_02157a78(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    FieldScriptEnv_GetGameSystem(env);
    return TRUE;
}

BOOL s0160_NetConnectWiFiBattle(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    u16 battleType;
    u16 mode;
    u32 type;
    u32 option;
    EventWifiBattleMatchArgs args;
    GameEvent *event;

    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    battleType = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    switch (battleType) {
    case 15:
        type = 0;
        break;
    case 16:
        type = 1;
        break;
    case 17:
        type = 2;
        break;
    case 18:
        type = 3;
        break;
    case 19:
        type = 4;
        break;
    }
    switch (mode) {
    case 0:
        option = 0;
        break;
    case 1:
        option = 1;
        break;
    }
    args.field = GSYS_GetField(gsys);
    args.unk4 = 1;
    args.unk8 = option;
    args.unkC = type;
    event = GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(2), EventWifiBattleMatch_CreateFromArgs, &args);
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL s0161_NetConnectBattleVideo(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    u16 mode;
    EventBattleVideoArgs args;
    GameEvent *event;

    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    mode = ScriptReadAny(vm, env);
    if (mode == 0) {
        mode = 1;
    } else {
        mode = 2;
    }
    args.field = GSYS_GetField(gsys);
    args.mode = mode;
    event = GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(3), EventBattleVideo_CreateFromArgs, &args);
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov012_02157b7c(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 value;

    gsys = FieldScriptEnv_GetGameSystem(env);
    value = ScriptReadAny(vm, env);
    GFL_OvlLoad(OVERLAY_ID(90));
    func_ov090_021eec80(gsys, value);
    GFL_OvlUnload(OVERLAY_ID(90));
    return FALSE;
}

BOOL func_ov012_02157bb4(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;

    gsys = FieldScriptEnv_GetGameSystem(env);
    GFL_OvlLoad(OVERLAY_ID(90));
    func_ov090_021eec98(gsys);
    GFL_OvlUnload(OVERLAY_ID(90));
    return FALSE;
}

BOOL func_ov012_02157bdc(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 *first;
    u16 *second;

    gsys = FieldScriptEnv_GetGameSystem(env);
    first = ScriptReadVar(vm, env);
    second = ScriptReadVar(vm, env);
    GFL_OvlLoad(OVERLAY_ID(90));
    func_ov090_021eecc0(gsys, *first, *second);
    GFL_OvlUnload(OVERLAY_ID(90));
    return FALSE;
}
