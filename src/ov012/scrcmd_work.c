#include "types.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_script.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "system/bmp_menulist.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/wordset.h"

ScriptSubwork *InitScriptSubwork(ScriptWork *work, HeapID heapId) {
    ScriptSubwork *subwork = GFL_HeapAllocate(heapId, sizeof(ScriptSubwork), TRUE, "scrcmd_work.c", 0x7b);

    subwork->work = work;
    subwork->gsys = ScriptWork_GetGameSystem(work);
    subwork->gameData = GSYS_GetGameData(subwork->gsys);
    subwork->mmSys = GameData_GetMMSys(subwork->gameData);
    subwork->actorMsgPosActual = 7;
    return subwork;
}

void func_ov012_021550e4(void *subwork) {
    GFL_HeapFree(subwork);
}

FieldScriptEnv *CreateFieldScriptEnv(const FieldScriptEnvArgs *args, HeapID heapId) {
    FieldScriptEnv *env =
        GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(FieldScriptEnv), TRUE, "scrcmd_work.c", 0x97);

    env->heapId = heapId;
    env->args = *args;
    env->subwork = ScriptWork_GetSubwork(args->work);
    return env;
}

void FreeFieldScriptEnv(FieldScriptEnv *env) {
    if (env->ownedHeap != NULL) {
        GFL_HeapFree(env->ownedHeap);
    }
    if (env->msgData != NULL) {
        GFL_MsgDataFree(env->msgData);
    }
    func_ov012_021552c8(env);
    GFL_HeapFree(env);
}

HeapID FieldScriptEnv_GetHeapID(FieldScriptEnv *env) {
    return env->heapId;
}

u16 GetScriptEnvZoneID(FieldScriptEnv *env) {
    return env->args.zoneId;
}

u32 FieldScriptEnv_IsReducedFeatureLevel(FieldScriptEnv *env) {
    return env->args.reducedFeatureLevel;
}

u32 FieldScriptEnv_GetFeatureLevel(FieldScriptEnv *env) {
    return env->args.featureLevel;
}

GameSystem *FieldScriptEnv_GetGameSystem(FieldScriptEnv *env) {
    return env->subwork->gsys;
}

GameData *FieldScriptEnv_GetGameData(FieldScriptEnv *env) {
    return env->subwork->gameData;
}

MMSys *GetScrEnvMMdlSys(FieldScriptEnv *env) {
    return env->subwork->mmSys;
}

ScriptWork *FieldScriptEnv_GetScriptWork(FieldScriptEnv *env) {
    return env->subwork->work;
}

void *func_ov012_0215518c(FieldScriptEnv *env) {
    return *(void **)ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
}

MsgData *GetFieldScriptMsgData(FieldScriptEnv *env) {
    return env->msgData;
}

u16 GetFieldScriptMsgFileNo(FieldScriptEnv *env) {
    return env->msgFileNo;
}

void setMapDisplayInfoPtr(FieldScriptEnv *env, void *info) {
    env->subwork->mapDisplayInfo = info;
}

void *getMapDisplayInfoPtr(FieldScriptEnv *env) {
    return env->subwork->mapDisplayInfo;
}

void SetSpecialMessageIconPtr(FieldScriptEnv *env, void *icon) {
    env->subwork->specialMessageIcon = icon;
}

void *func_ov012_021551c0(FieldScriptEnv *env) {
    return env->subwork->specialMessageIcon;
}

void *GetFieldScriptActorWk(FieldScriptEnv *env) {
    return env->subwork->actorWork;
}

void FieldScriptEnv_SetPlayerGridEventTCB(FieldScriptEnv *env, void *task) {
    env->subwork->playerGridEventTCB = task;
}

void *FieldScriptEnv_GetPlayerGridEventTCB(FieldScriptEnv *env) {
    return env->subwork->playerGridEventTCB;
}

u8 ActorMsgWin_GetPosActual(FieldScriptEnv *env) {
    return env->subwork->actorMsgPosActual;
}

void ActorMsgWin_SetPosActual(FieldScriptEnv *env, u8 pos) {
    env->subwork->actorMsgPosActual = pos;
}

u8 ActorMsgWin_GetPos(FieldScriptEnv *env) {
    return env->subwork->actorMsgPos;
}

void ActorMsgWin_SetPos(FieldScriptEnv *env, u8 pos) {
    env->subwork->actorMsgPos = pos;
}

void FieldScriptEnv_SetWaitCounter(FieldScriptEnv *env, u16 frames) {
    env->subwork->waitCounter = frames;
}

BOOL FieldScriptEnv_UpdateWaitCounter(FieldScriptEnv *env) {
    if (env->subwork->waitCounter == 0) {
        return TRUE;
    }
    env->subwork->waitCounter--;
    return FALSE;
}

u32 GetScrEnvNowPkmVoice(FieldScriptEnv *env) {
    return env->subwork->nowPkmVoice;
}

void SetScrEnvNowPkmVoice(FieldScriptEnv *env, u32 voice) {
    env->subwork->nowPkmVoice = voice;
}

void *FieldScriptEnv_GetElevatorTable(FieldScriptEnv *env) {
    return env->subwork->elevatorTable;
}

void FieldScriptEnv_SetElevatorTable(FieldScriptEnv *env, void *table) {
    env->subwork->elevatorTable = table;
}

void FieldScriptEnv_AddAcmdTask(FieldScriptEnv *env, FieldAcmdTCB *task) {
    int i;

    for (i = 0; i < 8; i++) {
        if (env->subwork->acmdTasks[i] == NULL) {
            env->subwork->acmdTasks[i] = task;
            return;
        }
    }
}

BOOL FieldScriptEnv_CheckAcmdQueueRunning(FieldScriptEnv *env) {
    int i;
    BOOL running = FALSE;

    for (i = 0; i < 8; i++) {
        if (env->subwork->acmdTasks[i] != NULL) {
            if (FieldAcmdTCB_CheckEnded(env->subwork->acmdTasks[i]) == TRUE) {
                FieldAcmdTCB_Remove(env->subwork->acmdTasks[i]);
                env->subwork->acmdTasks[i] = NULL;
            } else {
                running = TRUE;
            }
        }
    }

    return running;
}

void func_ov012_021552c8(FieldScriptEnv *env) {
    int i;

    for (i = 0; i < 8; i++) {
        if (env->subwork->acmdTasks[i] != NULL) {
            FieldAcmdTCB_Remove(env->subwork->acmdTasks[i]);
            env->subwork->acmdTasks[i] = NULL;
        }
    }
}

void SetFieldScriptEnvMsgData(FieldScriptEnv *env, u32 arcId, u32 fileNo) {
    env->msgData = GFL_MsgSysLoadData(FALSE, (u16)arcId, (u16)fileNo, env->heapId);
    env->msgFileNo = (u16)fileNo;
}

// The list menu's window
static const ListMenuRequest sListMenuRequest = { 1, 6, 0, 0xd, 0x1000, 0x2f, 0, 1, 0, 1, 0xc, 0xd, 0, 0, 0, 0, 0 };

void InitListMenu(FieldScriptEnv *env, u16 x, u16 y, u16 cursor, u16 flags, u32 align, u16 *result, WordSet *wordSet,
                  MsgData *msgData) {
    ScriptSubwork *subwork = env->subwork;
    ScriptListMenu *menu = &subwork->listMenu;

    sys_memset(menu, 0, sizeof(ScriptListMenu));
    subwork->listMenu.x = x;
    menu->y = y;
    menu->cursor = cursor;
    menu->flags = flags;
    menu->align = align;
    menu->result = result;
    menu->wordSet = wordSet;
    menu->msgData = msgData;
    sys_memset32(0, menu->descriptions, sizeof(menu->descriptions));
    if (menu->msgData == NULL) {
        menu->ownsMsgData = TRUE;
        menu->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_0410, env->heapId);
    }
    menu->options = InitListMenuOptionHeap(32, env->heapId);
}

void AddItemToListMenu(FieldScriptEnv *env, u32 messageId, u32 descriptionId, u32 value, StrBuf *expanded,
                       StrBuf *temp) {
    ScriptListMenu *menu = &env->subwork->listMenu;
    u32 index;

    if (descriptionId != 0xffff) {
        index = ListMenuCore_GetFirstFreeIndex(menu->options);
        if (index >= 32) {
            return;
        }
        GFL_MsgDataLoadStrbuf(menu->msgData, descriptionId, temp);
        GFL_WordSetFormatStrbuf(menu->wordSet, expanded, temp);
        menu->descriptions[index] = GFL_StrBufClone(expanded, env->heapId);
    }
    GFL_MsgDataLoadStrbuf(menu->msgData, messageId, temp);
    GFL_WordSetFormatStrbuf(menu->wordSet, expanded, temp);
    AppendListMenuOption(menu->options, expanded, value, env->heapId);
}

void FieldScriptEnv_ShowListMenu(FieldScriptEnv *env) {
    u32 width;
    ScriptListMenu *menu = &env->subwork->listMenu;
    ListMenuRequest request = sListMenuRequest;
    void *msgBGSys = func_ov012_0215518c(env);
    u32 count;
    u32 rows;
    BOOL scrolls;
    u32 height;

    if (menu->flags & 0x80) {
        menu->flags &= 0x7f;
        request.rowHeight = 14;
    }
    width = CalcListMenuWidth(msgBGSys, menu->options, request.unk10, request.unk0A_0);
    if (menu->align == 1) {
        menu->x -= (u16)width;
    }
    count = ListMenuCore_GetOptionCount(menu->options);
    rows = count > 6 ? 6 : count;
    scrolls = count > 6 ? TRUE : FALSE;
    height = CalcListMenuHeight(rows, request.rowHeight, request.unk0A_3, scrolls);
    ListMenuRequest_Set(&request, ListMenuCore_GetOptionCount(menu->options), menu->x, menu->y, width, height);
    menu->ui = ListMenuUI_Create(msgBGSys, &request, menu->options, func_ov012_02155568, env, 0, menu->cursor,
                                 menu->flags);
}


void FreeListMenuWork(ScriptListMenu *menu) {
    s32 i;

    func_ov036_02187ea0(menu->ui);
    if (menu->ownsMsgData == TRUE) {
        GFL_MsgDataFree(menu->msgData);
    }
    for (i = 0; i < 32; i++) {
        if (menu->descriptions[i] != NULL) {
            GFL_StrBufFree(menu->descriptions[i]);
            menu->descriptions[i] = NULL;
        }
    }
}

BOOL FieldScriptEnv_UpdateListMenu(FieldScriptEnv *env) {
    ScriptListMenu *menu = &env->subwork->listMenu;
    s32 result = ListMenuUI_Update(menu->ui);

    if (result == BMPMENULIST_NULL) {
        return FALSE;
    }
    FreeListMenuWork(menu);
    if (result != BMPMENULIST_CANCEL) {
        *menu->result = result;
    } else {
        *menu->result = 0xfffe;
    }
    return TRUE;
}

BOOL FieldScriptEnv_UpdateListMenuEx(FieldScriptEnv *env) {
    BOOL done;
    ScriptListMenu *menu = &env->subwork->listMenu;

    done = FieldScriptEnv_UpdateListMenu(env);

    if (!done && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X)) {
        FreeListMenuWork(menu);
        *menu->result = 0xfffd;
        done = TRUE;
    }
    return done;
}

void func_ov012_02155568(BmpMenuList *list, s32 value, u8 a2) {
    u16 index = 0;
    FieldScriptEnv *env = BmpMenuList_GetWork(list);
    ScriptListMenu *menu = &env->subwork->listMenu;
    void *window;

    BmpMenuList_GetCursorIndex(list, &index);
    if (menu->descriptions[index] == NULL) {
        return;
    }
    if (FieldScriptSubEvent_IsRegistered(1)) {
        window = getMapDisplayInfoPtr(env);
        func_ov036_02188660(window);
        func_ov036_02188680(window, 0, 0, menu->descriptions[index]);
    } else if (FieldScriptSubEvent_IsRegistered(2)) {
    } else if (FieldScriptSubEvent_IsRegistered(3)) {
    }
}

void FieldScriptEnv_Save(FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    env->ownedHeap = ScriptWork_CreateVarCopy(work);
}

void FieldScriptEnv_Restore(FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptWork_RestoreVarCopy(work, env->ownedHeap);
    env->ownedHeap = NULL;
}

void SetScrEnvVMIndex(FieldScriptEnv *env, u32 index) {
    env->vmIndex = index;
}

u8 FieldScriptEnv_GetVMIndex(FieldScriptEnv *env) {
    return env->vmIndex;
}
