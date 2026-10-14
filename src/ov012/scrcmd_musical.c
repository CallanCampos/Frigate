#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "app/ircbattlematch.h"
#include "constants/version.h"
#include "field/event_poke_status.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/musical.h"
#include "field/musical_dressup_sys.h"
#include "field/pdw_postman.h"
#include "field/scrcmd_musical.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/dream_world.h"
#include "save/musical_savedata.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

// The event that runs a musical
typedef struct {
    GameEvent *event;
    MusicalCommWork *work;
    BOOL online;
} MusicalCallEvent;

typedef struct {
    MusicalShotParam *param;
} MusicalShotHolder;

typedef struct {
    MusicalDressUpParam *param;
} MusicalDressUpEvent;

typedef struct {
    FieldScriptEnv *env;
    MusicalCommWork *work;
} MusicalIrcMatchHolder;

typedef struct {
    u16 *a0;
    u16 *a1;
    u32 unk8;
} MusicalOv020Args;

void func_ov012_02158bfc(ScriptWork *work, GameData *gameData);
void func_ov012_02158c40(MusicalCommWork *work, GameData *gameData);
void func_ov012_02158c6c(MusicalCommWork *work, GameSystem *gsys, GameCommSys *comm, u16 value);
void func_ov012_02158c78(MusicalCommWork *work);
GameEventReturnCode func_ov012_02158c8c(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov012_02158d24(GameEvent *event, u32 *state, void *data);
void func_ov012_02158da0(void *arg);
void func_ov012_02158dac(void *arg);
BOOL func_ov012_02158de8(VM *vm, void *arg);
BOOL func_ov012_02158e44(VM *vm, void *arg);
BOOL func_ov012_02158e9c(VM *vm, void *arg);
BOOL func_ov012_02158ee4(VM *vm, void *arg);
BOOL func_ov012_02158f20(VM *vm, void *arg);
BOOL func_ov012_02158f64(VM *vm, void *arg);
BOOL func_ov012_02158fb8(VM *vm, void *arg);

BOOL func_ov012_021580c4(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    u16 arg;
    u8 kind;
    MusicalCommWork *commWork;
    BOOL online;
    GameEvent *event;
    MusicalCallEvent *data;
    MusicalSave *save;
    MusicalShotHolder *holder;

    ScriptWork_GetFieldWork(work);
    arg = ScriptReadAny(vm, env);
    kind = VM_Read8(vm);
    if (kind <= 1) {
        commWork = func_020179dc(gameData);
        online = kind != 0 ? TRUE : FALSE;
        event = GameEvent_Create(gsys, NULL, func_ov012_02158c8c, sizeof(MusicalCallEvent));
        data = GameEvent_GetData(event);
        sys_memset(data, 0, sizeof(MusicalCallEvent));
        data->event = func_ov012_02150cf8(gsys, gameData, arg, online, commWork);
        data->work = commWork;
        data->online = online;
        ScriptWork_SetPostEvent(work, event);
        GameData_GetFieldSoundSystem(gameData);
        VM_Halt(vm);
        return TRUE;
    }
    save = getMusicalInfoBlkAddress(gameData);
    holder = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MusicalShotHolder), TRUE, "scrcmd_musical.c", 171);
    holder->param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MusicalShotParam), TRUE, "scrcmd_musical.c", 172);
    holder->param->askSave = FALSE;
    holder->param->shot = func_0200ad5c(save);
    holder->param->loadComm = TRUE;
    holder->param->loadData = TRUE;
    ScriptWork_CallEvent(work, EventFieldSubprocessTransition_CreateWithCallback(gsys, field, OVERLAY_ID(209), &MUSICAL_SHOT_PROC_FUNCTIONS, holder->param,
                                             func_ov012_02158da0, holder));
    return TRUE;
}

BOOL func_ov012_021581e4(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 slot;
    MusicalSave *save;
    MusicalPoke *poke;
    GameEvent *event;
    MusicalDressUpEvent *data;

    GSYS_GetField(gsys);
    slot = ScriptReadAny(vm, env);
    save = getMusicalInfoBlkAddress(gameData);
    poke = MusicalSystem_InitPokeFromPkm(PokeParty_GetPkm(GameData_GetParty(gameData), slot), HEAPID_GAMEEVENT);
    event = GameEvent_Create(gsys, NULL, func_ov012_02158d24, sizeof(MusicalDressUpEvent));
    data = GameEvent_GetData(event);
    sys_memset(data, 0, sizeof(MusicalDressUpEvent));
    data->param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MusicalDressUpParam), TRUE, "scrcmd_musical.c", 218);
    data->param->poke = poke;
    data->param->save = save;
    data->param->comm = NULL;
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov012_02158280(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    u32 mostIndex;
    u32 fewestIndex;
    u32 firstIndex;
    u8 topIndex;
    u8 secondValue;
    u32 second;
    PokeParty *party;
    u8 kind;
    u16 arg;
    u16 *result;
    MusicalSave *save;
    MusicalEventWork *event;
    MusicalCommWork *commWork;
    u8 i;
    u8 j;
    u8 most;
    u8 count;
    u8 value;
    u8 best;
    MusicalSaveUnk1B0 *entries;

    FieldScriptEnv_GetScriptWork(env);
    kind = VM_Read8(vm);
    arg = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    gameData = FieldScriptEnv_GetGameData(env);
    save = getMusicalInfoBlkAddress(gameData);
    event = NULL;
    if (kind >= 30) {
        commWork = func_020179dc(gameData);
        if (commWork == NULL) {
            *result = 0;
            return FALSE;
        }
        event = commWork->event;
    }
    switch (kind) {
    case 0:
        *result = func_0200ae78(save);
        break;
    case 1:
        *result = func_0200ae9c(save);
        break;
    case 2:
        *result = func_0200aed4(save);
        break;
    case 3:
        best = 0;
        mostIndex = 4;
        for (i = 0; i < 4; i++) {
            value = func_0200aebc(save, i);
            if (value > best) {
                best = value;
                mostIndex = i;
            } else if (value == best) {
                mostIndex = 4;
            }
        }
        *result = mostIndex;
        break;
    case 4:
        *result = func_0200aee4(save);
        break;
    case 5:
        func_0200aef0(save, arg);
        break;
    case 6:
        *result = 0;
        for (i = 0; i < 4; i++) {
            if (func_0200aebc(save, i) != 0) {
                *result = 1;
            }
        }
        break;
    case 7:
        if (func_0200ad5c(save)->month == 0) {
            *result = FALSE;
        } else {
            *result = TRUE;
        }
        break;
    case 8:
        best = 0xff;
        fewestIndex = 4;
        for (i = 0; i < 4; i++) {
            value = func_0200aebc(save, i);
            if (value <= best) {
                best = value;
                fewestIndex = i;
            }
        }
        *result = fewestIndex;
        break;
    case 9:
        best = 0;
        firstIndex = 4;
        for (i = 0; i < 4; i++) {
            value = func_0200aebc(save, i);
            if (value > best) {
                best = value;
                firstIndex = i;
            }
        }
        *result = firstIndex;
        break;
    case 10:
        topIndex = 4;
        secondValue = 0;
        most = 0;
        second = 4;
        for (j = 0; j < 4; j++) {
            value = func_0200aebc(save, j);
            if (value > most) {
                if (secondValue == most) {
                    second = 4;
                } else {
                    secondValue = most;
                    second = topIndex;
                }
                most = value;
                topIndex = j;
            }
        }
        *result = second;
        break;
    case 11:
        count = 0;
        party = GameData_GetParty(gameData);
        value = PokeParty_GetPkmCount(party);
        for (i = 0; i < value; i++) {
            if (MusicalSystem_CanJoin(PokeParty_GetPkm(party, i)) == TRUE) {
                count++;
            }
        }
        *result = count;
        break;
    case 12:
        if (MusicalSystem_CanJoin(PokeParty_GetPkm(GameData_GetParty(gameData), arg)) == TRUE) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case 13:
        entries = func_0200ad44(save);
        *result = 0;
        for (i = 0; i < 8; i++) {
            if (entries[i].unk0 != 10) {
                *result = 1;
            }
        }
        break;
    case 14:
        if (func_0200aefc(save) == TRUE) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case 15:
        if (func_0200ad60(save, arg) == TRUE) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case 30:
        *result = func_ov012_02151b80(event);
        break;
    case 31:
        *result = func_ov012_02151bf4(event);
        break;
    case 32:
        *result = func_ov012_02151c18(event);
        break;
    case 33:
        *result = func_ov012_02151bd4(event, arg);
        break;
    case 35:
        *result = func_ov012_02151b88(event, arg);
        break;
    case 36:
        *result = func_ov012_02151ba8(event);
        break;
    case 37:
        *result = func_ov012_02151c3c(event, arg);
        break;
    case 38:
        *result = func_ov012_02151c44(event, arg);
        break;
    case 39:
        *result = func_ov012_02151c8c(event, arg);
        break;
    }
    return FALSE;
}

BOOL func_ov012_02158550(VM *vm, FieldScriptEnv *env) {
    u16 arg = ScriptReadAny(vm, env);
    u8 kind = VM_Read8(vm);
    u16 *result = ScriptReadVar(vm, env);
    MusicalSaveUnk1E0 *entry = func_0200ae6c(getMusicalInfoBlkAddress(FieldScriptEnv_GetGameData(env)), arg);

    switch (kind) {
    case 0:
        switch (entry->unk0) {
        case 0:
            *result = 24;
            break;
        case 1:
            *result = 15;
            break;
        case 2:
            *result = 45;
            break;
        case 3:
            *result = 12;
            break;
        case 4:
            *result = 0;
            break;
        }
        break;
    case 1:
        *result = arg + 5;
        break;
    case 2:
        *result = arg + 15;
        break;
    case 3:
        *result = entry->unk1;
        break;
    case 4:
        *result = entry->unk2;
        break;
    }
    return FALSE;
}

BOOL func_ov012_021585e0(VM *vm, FieldScriptEnv *env) {
    u16 prop = ScriptReadAny(vm, env);

    func_0200add8(getMusicalInfoBlkAddress(FieldScriptEnv_GetGameData(env)), prop);
    return FALSE;
}

BOOL s005D_WordSetMusicalInfo(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u8 kind = VM_Read8(vm);
    u8 wordIndex = VM_Read8(vm);
    u16 arg = ScriptReadAny(vm, env);
    MusicalSave *save;
    MusicalSave *musical;
    MsgData *msgData;
    StrBuf *str;
    u8 i;
    u8 j;
    u8 next;
    u8 tmp;
    u16 messages[3];
    u8 order[4];
    u8 points[4];

    switch (kind) {
    case 0:
        save = getMusicalInfoBlkAddress(FieldScriptEnv_GetGameData(env));
        str = GFL_StrBufCreate(0x26, heapId);
        GFL_StrBufLoadFixedString(str, func_0200af14(save), 0x26);
        func_0202437c(wordSet, wordIndex, str, 0, 1, 2);
        GFL_StrBufFree(str);
        break;
    case 1:
    case 8:
        if (kind == 1) {
            msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_13, heapId);
        } else {
            msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0484, heapId);
        }
        str = GFL_MsgDataLoadStrbufNew(msgData, arg + 3);
        func_0202437c(wordSet, wordIndex, str, 0, 1, 2);
        GFL_StrBufFree(str);
        GFL_MsgDataFree(msgData);
        break;
    case 2:
        str = func_ov012_02151bb4(func_020179dc(gameData)->event, heapId);
        func_0202437c(wordSet, wordIndex, str, 0, 1, 2);
        GFL_StrBufFree(str);
        break;
    case 3:
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_S005D_WORD_SET_MUSICAL_INFO, heapId);
        str = GFL_MsgDataLoadStrbufNew(msgData, arg);
        func_0202437c(wordSet, wordIndex, str, 0, 1, 2);
        GFL_StrBufFree(str);
        GFL_MsgDataFree(msgData);
        break;
    case 4:
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_S005D_WORD_SET_MUSICAL_INFO, heapId);
        str = GFL_MsgDataLoadStrbufNew(msgData, arg + 4);
        func_0202437c(wordSet, wordIndex, str, 0, 1, 2);
        GFL_StrBufFree(str);
        GFL_MsgDataFree(msgData);
        break;
    case 5:
        func_ov012_02151cd4(func_020179dc(gameData)->event, arg, wordSet, wordIndex);
        break;
    case 6:
        func_ov012_02151d6c(func_020179dc(gameData)->event, arg, wordSet, wordIndex);
        break;
    case 7:
        musical = getMusicalInfoBlkAddress(FieldScriptEnv_GetGameData(env));
        for (i = 0; i < 4; i++) {
            order[i] = i;
            points[i] = func_0200aebc(musical, i);
        }
        for (i = 1; i < 4; i++) {
            for (j = 0; j < 4 - i; j++) {
                next = j + 1;
                if (points[j] < points[next]) {
                    tmp = points[j];
                    points[j] = points[next];
                    points[next] = tmp;
                    tmp = order[j];
                    order[j] = order[next];
                    order[next] = tmp;
                }
            }
        }
        if (points[0] == points[1] && points[0] == points[2]) {
            messages[0] = 9;
            messages[1] = 8;
        } else {
            messages[0] = order[0] + 4;
            if (points[1] == points[2]) {
                messages[1] = 8;
            } else {
                messages[1] = order[1] + 4;
            }
        }
        if (points[1] == points[3]) {
            messages[2] = order[1] + 4;
        } else if (points[2] == points[3]) {
            messages[2] = order[2] + 4;
        } else {
            messages[2] = order[3] + 4;
        }
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_S005D_WORD_SET_MUSICAL_INFO, heapId);
        for (i = 0; i < 3; i++) {
            str = GFL_MsgDataLoadStrbufNew(msgData, messages[i]);
            func_0202437c(wordSet, i, str, 0, 1, 2);
            GFL_StrBufFree(str);
        }
        GFL_MsgDataFree(msgData);
        break;
    }
    return FALSE;
}


BOOL func_ov012_02158858(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);

    func_0200ae6c(getMusicalInfoBlkAddress(FieldScriptEnv_GetGameData(env)), index)->unk1 = 0;
    return FALSE;
}

BOOL func_ov012_0215887c(VM *vm, FieldScriptEnv *env) {
    u16 *a0 = ScriptReadVar(vm, env);
    u16 *a1 = ScriptReadVar(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    MusicalOv020Args args;

    sys_memset(&args, 0, sizeof(MusicalOv020Args));
    args.a0 = a0;
    args.a1 = a1;
    ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKE_STATUS, EventMusicalPokeSelect_Create, &args));
    return TRUE;
}

BOOL func_ov012_021588d4(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    u16 kind = ScriptReadAny(vm, env);
    u16 arg = ScriptReadAny(vm, env);
    u16 *result;
    BOOL lost;
    MusicalCommWork *commWork;
    BOOL allowed;
    s32 i;
    PlayerInfo *info;
    MusicalIrcMatchHolder *holder;

    ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    lost = FALSE;
    commWork = NULL;
    if (kind != 0 && kind != 21) {
        commWork = func_020179dc(gameData);
        if (commWork->event != NULL) {
            func_ov012_02151e44(commWork->event);
            if (func_ov012_02151e64(commWork->event) == TRUE) {
                lost = TRUE;
            }
        }
    }
    switch (kind) {
    case 0:
        func_ov012_02158bfc(work, gameData);
        break;
    case 1:
        func_ov012_02158c40(commWork, gameData);
        break;
    case 10:
        func_ov012_02158c6c(commWork, gsys, GSYS_GetGameCommSystem(gsys), arg);
        break;
    case 11:
        func_ov012_02158c78(commWork);
        VM_SetNativeCallback(vm, func_ov012_02158f64);
        return TRUE;
    case 12:
        commWork->result = result;
        commWork->menu = func_ov036_021c3d9c(GetGameDataPlayerInfo(gameData), field, 2, 4, HEAPID_GAMEEVENT, 0, 1, 0);
        VM_SetNativeCallback(vm, func_ov012_02158de8);
        return TRUE;
    case 13:
        commWork->result = result;
        commWork->menu = func_ov036_021c3d9c(GetGameDataPlayerInfo(gameData), field, 2, 4, HEAPID_GAMEEVENT, 1, 1, 0);
        VM_SetNativeCallback(vm, func_ov012_02158e44);
        return TRUE;
    case 14:
        allowed = TRUE;
        if (commWork->comm != NULL && arg == 0x8e) {
            for (i = 0; i < 4; i++) {
                info = func_ov211_021ef9c4(commWork->comm, i);
                if (info != NULL && (u32)(func_02008bfc(info) - VERSION_WHITE) <= VERSION_BLACK - VERSION_WHITE) {
                    allowed = FALSE;
                }
            }
        }
        if (commWork->comm == NULL || lost) {
            break;
        }
        if (allowed) {
            commWork->value = arg;
            func_ov211_021ef988(commWork->comm, commWork->value);
            VM_SetNativeCallback(vm, func_ov012_02158e9c);
            return TRUE;
        }
        break;
    case 15:
        if (commWork->comm != NULL && !lost) {
            VM_SetNativeCallback(vm, func_ov012_02158ee4);
            return TRUE;
        }
        break;
    case 16:
        VM_SetNativeCallback(vm, func_ov012_02158f20);
        return TRUE;
    case 17:
        if (commWork->comm != NULL && !lost) {
            VM_SetNativeCallback(vm, func_ov012_02158fb8);
            return TRUE;
        }
        break;
    case 18:
        holder = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_GAMEEVENT), sizeof(MusicalIrcMatchHolder), TRUE,
                                  "scrcmd_musical.c", 1004);
        holder->env = env;
        holder->work = commWork;
        commWork->result = result;
        commWork->ircMatch.gameData = gameData;
        commWork->ircMatch.musicalNetInit = func_ov211_021f0608(gameData);
        commWork->ircMatch.netError = 0;
        if (arg == 0) {
            commWork->ircMatch.result = IRCBATTLEMATCH_MODE_MUSICAL_ARG0;
        } else {
            commWork->ircMatch.result = IRCBATTLEMATCH_MODE_MUSICAL_ARG1;
        }
        ScriptWork_CallEvent(work, EventFieldSubprocessTransition_CreateWithCallback(gsys, field, OVERLAY_IRCBATTLEMATCH, &IRCBATTLEMATCH_PROC_FUNCTIONS,
                                                 &commWork->ircMatch, func_ov012_02158dac, holder));
        return TRUE;
    case 19:
        if (commWork->comm != NULL && lost == TRUE) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case 20:
        if (commWork->comm != NULL) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case 21:
    case 22:
        if (GFL_NetErrCheck()) {
            func_02016b0c(gsys, 1);
            *result = 1;
        } else if (commWork != NULL && commWork->ircMatch.netError == 1) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case 100:
        break;
    }
    return FALSE;
}

BOOL s02EE_MusicalIsPropOwned(VM *vm, FieldScriptEnv *env) {
    u16 prop = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    MusicalSave *save = getMusicalInfoBlkAddress(gameData);
    *result = func_0200ad60(save, prop);
    return FALSE;
}

BOOL s02EF_MusicalGetOwnedPropCount(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    MusicalSave *save = getMusicalInfoBlkAddress(gameData);
    *result = func_0200ae58(save);
    return FALSE;
}

void func_ov012_02158bfc(ScriptWork *work, GameData *gameData) {
    MusicalCommWork *commWork =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MusicalCommWork), TRUE, "scrcmd_musical.c", 1138);

    commWork->event = NULL;
    commWork->comm = NULL;
    func_020179d4(gameData, commWork);
    GFL_OvlLoad(OVERLAY_ID(210));
    GFL_OvlLoad(OVERLAY_ID(211));
}

void func_ov012_02158c40(MusicalCommWork *work, GameData *gameData) {
    GFL_OvlUnload(OVERLAY_ID(211));
    GFL_OvlUnload(OVERLAY_ID(210));
    GFL_HeapFree(work);
    func_020179d4(gameData, NULL);
}

void func_ov012_02158c6c(MusicalCommWork *work, GameSystem *gsys, GameCommSys *comm, u16 value) {
    func_ov211_021ef1e0(HEAPID_GAMEEVENT, gsys, comm, value);
}

void func_ov012_02158c78(MusicalCommWork *work) {
    if (work->comm != NULL) {
        func_ov211_021ef220(work->comm);
        work->comm = NULL;
    }
}

GameEventReturnCode func_ov012_02158c8c(GameEvent *event, u32 *state, void *arg) {
    MusicalCallEvent *data = arg;
    GameData *gameData = GSYS_GetGameData(GameEvent_GetGameSystem(event));
    GameCommSys *comm;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, data->event);
        (*state)++;
        break;
    case 1:
        GameData_GetFieldSoundSystem(gameData);
        func_ov012_02158c78(data->work);
        (*state)++;
        break;
    case 2:
        comm = GSYS_GetGameCommSystem(GameEvent_GetGameSystem(event));
        if (data->online && GameCommSys_BootCheck(comm)) {
            break;
        }
        (*state)++;
        break;
    case 3:
        if (func_020427a4() == TRUE || !data->online) {
            if (data->online == TRUE) {
                GSYS_TryBootGameComm(GameEvent_GetGameSystem(event));
            }
            func_ov012_02158c40(data->work, gameData);
            (*state)++;
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode func_ov012_02158d24(GameEvent *event, u32 *state, void *arg) {
    MusicalDressUpEvent *data = arg;
    GameSystem *gsys = GameEvent_GetGameSystem(event);

    GSYS_GetGameData(gsys);
    GSYS_GetField(gsys);
    switch (*state) {
    case 0:
        (*state)++;
        break;
    case 1:
        (*state)++;
        break;
    case 2:
        (*state)++;
        break;
    case 3:
        GSYS_QueueProc(gsys, OVERLAY_NONE, &data_ov012_0216dfc4, data->param);
        (*state)++;
        break;
    case 4:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        (*state)++;
        break;
    case 5:
        (*state)++;
        break;
    case 6:
        GFL_HeapFree(data->param->poke);
        GFL_HeapFree(data->param);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void func_ov012_02158da0(void *arg) {
    MusicalShotHolder *holder = arg;

    GFL_HeapFree(holder->param);
}

void func_ov012_02158dac(void *arg) {
    MusicalIrcMatchHolder *holder = arg;
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(holder->env);

    FieldScriptEnv_GetGameData(holder->env);
    GSYS_GetGameCommSystem(gsys);
    if (holder->work->ircMatch.result != IRCBATTLEMATCH_RESULT_QUIT
        && holder->work->ircMatch.result != IRCBATTLEMATCH_RESULT_BACK) {
        *holder->work->result = 1;
        func_ov211_021ef3b8(holder->work->comm);
    } else {
        *holder->work->result = 0;
    }
}


BOOL func_ov012_02158de8(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    MusicalCommWork *commWork;
    BOOL done;
    u32 choice;

    FieldScriptEnv_GetScriptWork(env);
    commWork = func_020179dc(FieldScriptEnv_GetGameData(env));
    choice = func_ov036_021c3f98(commWork->menu);
    done = FALSE;
    switch (choice) {
    case 1:
        *commWork->result = 0;
        func_ov211_021ef3a0(commWork->comm);
        done = TRUE;
        break;
    case 2:
        *commWork->result = 1;
        done = TRUE;
        break;
    case 4:
        *commWork->result = 2;
        done = TRUE;
        break;
    }
    if (done == TRUE) {
        func_ov036_021c3eb4(commWork->menu);
        commWork->menu = NULL;
    }
    return done;
}

BOOL func_ov012_02158e44(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    MusicalCommWork *commWork;
    BOOL done;

    FieldScriptEnv_GetScriptWork(env);
    commWork = func_020179dc(FieldScriptEnv_GetGameData(env));
    done = FALSE;
    switch (func_ov036_021c3f98(commWork->menu)) {
    case 1:
        *commWork->result = 3;
        func_ov211_021ef3a0(commWork->comm);
        done = TRUE;
        break;
    case 2:
        *commWork->result = 4;
        done = TRUE;
        break;
    case 4:
        *commWork->result = 2;
        done = TRUE;
        break;
    }
    if (done == TRUE) {
        func_ov036_021c3eb4(commWork->menu);
        commWork->menu = NULL;
    }
    return done;
}

BOOL func_ov012_02158e9c(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    MusicalCommWork *commWork;

    FieldScriptEnv_GetScriptWork(env);
    commWork = func_020179dc(FieldScriptEnv_GetGameData(env));
    if (commWork->event != NULL) {
        func_ov012_02151e44(commWork->event);
        if (func_ov012_02151e64(commWork->event) == TRUE) {
            return TRUE;
        }
    }
    if (func_ov211_021ef99c(commWork->comm, commWork->value) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_02158ee4(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    MusicalCommWork *commWork;

    FieldScriptEnv_GetScriptWork(env);
    commWork = func_020179dc(FieldScriptEnv_GetGameData(env));
    func_ov012_02151e44(commWork->event);
    if (func_ov012_02151e64(commWork->event) == TRUE) {
        return TRUE;
    }
    if (func_ov211_021f03e0(commWork->comm) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_02158f20(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    GameSystem *gsys;
    GameData *gameData;
    GameCommSys *comm;
    MusicalCommWork *commWork;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    comm = GSYS_GetGameCommSystem(gsys);
    commWork = func_020179dc(gameData);
    if (GameCommSys_BootCheck(comm) == 4) {
        commWork->comm = GameCommSys_GetWork(comm);
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_02158f64(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    GameSystem *gsys;
    GameData *gameData;
    GameCommSys *comm;
    MusicalCommWork *commWork;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    comm = GSYS_GetGameCommSystem(gsys);
    commWork = func_020179dc(gameData);
    if (commWork->event != NULL) {
        func_ov012_02151e44(commWork->event);
        if (func_ov012_02151e64(commWork->event) == TRUE) {
            return TRUE;
        }
    }
    if (GameCommSys_BootCheck(comm) == 0) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_02158fb8(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    MusicalCommWork *commWork;

    FieldScriptEnv_GetScriptWork(env);
    commWork = func_020179dc(FieldScriptEnv_GetGameData(env));
    func_ov012_02151e44(commWork->event);
    if (func_ov012_02151e64(commWork->event) == TRUE) {
        return TRUE;
    }
    if (func_ov211_021f04ac(commWork->comm) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

// The items sent from the Dream World
BOOL func_ov012_02158ff4(VM *vm, FieldScriptEnv *env) {
    u16 kind = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    DreamWorldSave *dreamWorld = getDreamWorldStuffAddress(GameData_GetSaveControl(gameData));
    s32 count = func_02009a78(dreamWorld);
    GameEvent *event;

    if (GSYS_GetField(gsys) == NULL && kind != 0) {
        return FALSE;
    }
    switch (kind) {
    case 0:
        *result = func_02009a78(dreamWorld) > 0 ? TRUE : FALSE;
        break;
    case 1:
        if (func_ov033_02177c8c(gameData, heapId, dreamWorld) < count) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case 2:
        event = func_ov033_02177d28(gsys);
        ScriptWork_CallEvent(FieldScriptEnv_GetScriptWork(env), event);
        return TRUE;
    case 3:
        func_ov033_02177c48(gameData, heapId, dreamWorld);
        break;
    case 5:
        loadItemsNameToStrbuf(ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env)), 0,
                              func_ov033_02177cd4(gameData, heapId, dreamWorld, *result));
        break;
    case 4:
        *result = func_ov033_02177c8c(gameData, heapId, dreamWorld);
        break;
    }
    return FALSE;
}

