#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/event_pokewood.h"
#include "field/field_script.h"
#include "field/pokewood_system.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "save/pokewood.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

// The script plugin of Pokéstar Studios (plugin 10), commands from 1000

#define POKEWOOD_MSG_FILE TEXT_BANK_POKEWOOD_LINES

static u16 func_ov062_021e5800(u16 value) {
    static const struct {
        u16 key;
        u16 value;
    } sTable[] = {
        { 0x4c, 0x2c }, { 0x2e, 0x4a },  { 0x7c, 0x100 }, { 0x34, 0x49 }, { 0xdf, 0x15a },
        { 0x57, 0x41 }, { 0x36, 0x5b },  { 0x32, 0x1e },  { 0x31, 0x1f }, { 0x3a, 0x45 },
        { 0x30, 0x35 }, { 0xe0, 0x15a }, { 0x0e, 0x9a },  { 0x43, 0x34 }, { 0x1a, 0x9b },
    };
    u32 i;

    for (i = 0; i < NELEMS(sTable); i++) {
        if (value == sTable[i].key) {
            return sTable[i].value;
        }
    }
    return 11;
}

static u16 func_ov062_021e582c(u8 index) {
    static const u16 sTable[] = { 0x1c, 0x53, 0x19, 0x2c, 0x12f };

    if (index >= NELEMS(sTable)) {
        index = 0;
    }
    return sTable[index];
}

static PokewoodMovie *Pokewood_LoadMovie(HeapID heapId, u32 movie) {
    return GFL_ArcSysReadHeapNew(ARCID_POKEWOOD_MOVIE, movie, heapId);
}

static u16 Pokewood_GetMovieField(u32 movie, u32 field) {
    u16 value = 0;
    PokewoodMovie *data = Pokewood_LoadMovie(HEAPID_GAMEEVENT, movie);

    switch (field) {
    case 0:
        value = data->unk8;
        break;
    case 1:
        value = data->unk18;
        break;
    case 2:
        value = data->unk1c;
        break;
    case 3:
        value = data->unk4;
        break;
    }
    GFL_HeapFree(data);
    return value;
}

static PokewoodSave *Pokewood_GetSave(FieldScriptEnv *env) {
    return PokewoodSave_Get(FieldScriptEnv_GetGameData(env));
}

static PokewoodSystem *Pokewood_GetSystem(FieldScriptEnv *env) {
    return *GameData_GetPokewoodSystemPtr(FieldScriptEnv_GetGameData(env));
}

static BOOL Scrcmd_Pokewood_Create(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PokewoodSave *save = PokewoodSave_Get(gameData);
    PokewoodSystem **sys = GameData_GetPokewoodSystemPtr(gameData);

    *sys = PokewoodSystem_Create(HEAPID_GAMEEVENT);
    GFL_OvlLoad(OVERLAY_EVENT_POKEWOOD);
    EventPokewood_UnlockMovies(save, gameData, 0, 0, 0);
    GFL_OvlUnload(OVERLAY_EVENT_POKEWOOD);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_Free(VM *vm, FieldScriptEnv *env) {
    PokewoodSystem **sys = GameData_GetPokewoodSystemPtr(FieldScriptEnv_GetGameData(env));

    PokewoodSystem_Free(*sys);
    *sys = NULL;
    return FALSE;
}

static BOOL Scrcmd_Pokewood_Film(VM *vm, FieldScriptEnv *env) {
    PokewoodSystem *sys = Pokewood_GetSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u32 args[2];

    args[0] = ScriptReadAny(vm, env);
    args[1] = ScriptReadAny(vm, env);
    PokewoodSystem_SetResultVar(sys, ScriptReadVar(vm, env));
    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateFilmEvent, args));
    return TRUE;
}

static BOOL Scrcmd_Pokewood_SelectCast(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = ScriptWork_GetGameSystem(work);
    u16 *var = ScriptReadVar(vm, env);
    PokewoodSystem *sys = Pokewood_GetSystem(env);

    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateCastSelectEvent, sys));
    PokewoodSystem_SetResultVar(sys, var);
    return TRUE;
}

static BOOL Scrcmd_Pokewood_SetUsePlayerCast(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    PokewoodSystem *sys = Pokewood_GetSystem(env);

    PokewoodSystem_SetUsePlayerCast(sys, ScriptReadAny(vm, env));
    return TRUE;
}

static BOOL Scrcmd_Pokewood_WatchSlot(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    u32 arg = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    PokewoodSystem *sys = Pokewood_GetSystem(env);

    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateWatchSlotEvent, &arg));
    PokewoodSystem_SetResultVar(sys, var);
    return TRUE;
}

static BOOL Scrcmd_Pokewood_WatchBlock(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    PokewoodSystem *sys = Pokewood_GetSystem(env);

    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateWatchBlockEvent, NULL));
    PokewoodSystem_SetResultVar(sys, NULL);
    return TRUE;
}

static BOOL Scrcmd_Pokewood_PickMovie(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 *var = ScriptReadVar(vm, env);
    PokewoodSystem **sys = GameData_GetPokewoodSystemPtr(gameData);
    PkwdBookParam *param = PokewoodSetup_CreateBookParam(heapId, gsys, var);
    GameEvent *event =
        GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateMoviePickerEvent, param);

    PokewoodSetup_FreeBookParam(param);
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static BOOL Scrcmd_Pokewood_OpenMenu(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 mode = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    PkwdSelectScreenParam *param = PokewoodSetup_CreateMenuParam(heapId, gsys, mode, var);
    GameEvent *event =
        GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateMenuEvent, param);

    PokewoodSetup_FreeMenuParam(param);
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

// Whether the downloaded movie of a slot loads
static BOOL Pokewood_CheckDownloadedMovie(FieldScriptEnv *env, u32 slot) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    BOOL loads = FALSE;

    allocatePokewoodBlk(heapId);
    if (Pokewood_LoadDownloadedMovie(save, heapId, slot) == 1 && Pokewood_IsLoadedMovieValid() == TRUE) {
        loads = TRUE;
    }
    freeAndClearPokewoodBlk();
    return loads;
}

// How many slots have a downloaded movie that loads
static u32 Pokewood_CountDownloadedMovies(FieldScriptEnv *env) {
    u32 slot;
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u32 count = 0;

    allocatePokewoodBlk(heapId);
    for (slot = 0; slot < 8; slot++) {
        if (Pokewood_LoadDownloadedMovie(save, heapId, slot) == 1 && Pokewood_IsLoadedMovieValid() == TRUE) {
            count++;
        }
    }
    freeAndClearPokewoodBlk();
    return count;
}

// A field of the downloaded movie of a slot, and 0 when it doesn't load
static u32 Pokewood_GetDownloadedMovieField(FieldScriptEnv *env, u32 slot, u32 field, u32 a3) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u32 value;

    allocatePokewoodBlk(heapId);
    if (Pokewood_LoadDownloadedMovie(save, heapId, slot) == 1) {
        value = Pokewood_GetLoadedMovieField(field, a3);
    } else {
        value = 0;
    }
    freeAndClearPokewoodBlk();
    return value;
}

static BOOL Scrcmd_Pokewood_CheckDownloadedMovie(VM *vm, FieldScriptEnv *env) {
    u16 slot = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = Pokewood_CheckDownloadedMovie(env, slot);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_GetDownloadedMovieResult(VM *vm, FieldScriptEnv *env) {
    u16 slot = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    switch (Pokewood_GetDownloadedMovieField(env, slot, 1, 0)) {
    case 0:
    default:
        *var = 0;
        break;
    case 1:
        *var = 1;
        break;
    case 2:
        *var = 2;
        break;
    }
    return FALSE;
}

static BOOL Scrcmd_Pokewood_SetMessageWord(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u8 index = VM_Read8(vm);
    u16 message = ScriptReadAny(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u32 msgId = message + 42;
    StrBuf *strbuf = ScriptWork_GetAltStrBuf(work);
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, POKEWOOD_MSG_FILE, heapId);

    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, msgId, strbuf);
        GFL_MsgDataFree(msgData);
    }
    GFL_WordSetSetStrbuf(wordSet, index, strbuf, 2, 1, 2);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_GetMovieFlag(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 list = ScriptReadAny(vm, env);
    u16 movie = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = PokewoodSave_CheckFlag(save, list, movie);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_SetMovieFlag(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 list = ScriptReadAny(vm, env);
    u16 movie = ScriptReadAny(vm, env);

    PokewoodSave_SetFlag(save, list, movie);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_CountMovieFlags(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 list = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = PokewoodSave_CountFlags(save, list);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_CountDownloadedMovies(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = Pokewood_CountDownloadedMovies(env);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_GetNewUnlockKind(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 movie = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    u16 *doneVar = ScriptReadVar(vm, env);

    *doneVar = FALSE;
    if (PokewoodSave_CheckFlag(save, 0, movie) == TRUE && PokewoodSave_CheckFlag(save, 3, movie) == FALSE) {
        PokewoodSave_SetFlag(save, 3, movie);
        GFL_OvlLoad(OVERLAY_EVENT_POKEWOOD);
        switch (EventPokewood_GetUnlockKind(movie)) {
        case 0:
        case 1:
        default:
            *var = 0;
            break;
        case 2:
            *var = 1;
            break;
        case 3:
            *var = 2;
            break;
        case 4:
            *var = 3;
            break;
        case 5:
            *var = 4;
            break;
        case 6:
            *var = 5;
            break;
        case 7:
            *var = 6;
            break;
        }
        GFL_OvlUnload(OVERLAY_EVENT_POKEWOOD);
        *doneVar = TRUE;
    }
    return FALSE;
}

static BOOL Scrcmd_Pokewood_TestAndSetLevelBit(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);
    PokewoodSave *save = Pokewood_GetSave(env);
    u32 level = PokewoodSave_GetLevelOfPoints(PokewoodSave_GetLevelPoints(save));

    *var = PokewoodSave_GetLevelBit(save, level);
    PokewoodSave_SetLevelBit(save, level, 1);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_GetSlotFlag(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 slot = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = func_020111a0(save, slot);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_SetSlotFlag(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);

    func_020111a8(save, ScriptReadAny(vm, env), 1);
    return FALSE;
}

static BOOL func_ov062_021e5f1c(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 slot = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = Pokewood_GetDownloadedMovieField(env, slot, 9, 0);
    return FALSE;
}

static BOOL func_ov062_021e5f50(VM *vm, FieldScriptEnv *env) {
    u8 sex = getTrainerGender(GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env)));
    u16 movie = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    u16 *var2 = ScriptReadVar(vm, env);

    *var = func_ov062_021e5800(Pokewood_GetMovieField(movie, sex == 0 ? 1 : 2));
    *var2 = func_ov062_021e582c(Pokewood_GetMovieField(movie, 0));
    return FALSE;
}

static BOOL Scrcmd_Pokewood_SetSaveSlot(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PokewoodSystem *sys = Pokewood_GetSystem(env);

    PokewoodSystem_SetSaveSlot(sys, ScriptReadAny(vm, env));
    return TRUE;
}

static BOOL func_ov062_021e5fec(VM *vm, FieldScriptEnv *env) {
    u16 movie = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = Pokewood_GetMovieField(movie, 0);
    return FALSE;
}

static BOOL func_ov062_021e6010(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 *var = ScriptReadVar(vm, env);

    if (PokewoodSave_CountFlags(save, 2) == POKEWOOD_MOVIE_COUNT) {
        *var = TRUE;
    } else {
        *var = FALSE;
    }
    return FALSE;
}

static BOOL Scrcmd_Pokewood_OpenRecordMonitor(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 *var = ScriptReadVar(vm, env);
    PokewoodSystem **sys = GameData_GetPokewoodSystemPtr(gameData);
    PkwdRecordMonitorParam *param = PokewoodSetup_CreateRecordMonitorParam(heapId, gsys);
    GameEvent *event =
        GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateScreenEvent, param);

    PokewoodSetup_FreeRecordMonitorParam(param);
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static BOOL func_ov062_021e60a8(VM *vm, FieldScriptEnv *env) {
    static const u16 sTable6872[][3] = { { 3, 2, 1 }, { 3, 2, 1 }, { 3, 2, 0 }, { 2, 1, 0 } };
    static const u16 sTable68b2[][4] = {
        { 0x23b, 0x2a, 0x20, 0x1e }, { 0x1d, 0x19, 0x2b, 0x11 }, { 0x5c, 0x21, 0x4e, 0x8a },
        { 0x5b, 0x36, 0x1f8, 0x86 }, { 0x17, 0x59, 0x23, 0x22 },
    };
    static const u16 sTable688a[][4] = {
        { 1, 5, 5, 1 }, { 1, 5, 1, 1 }, { 1, 5, 1, 1 }, { 1, 5, 1, 1 }, { 1, 1, 1, 1 },
    };
    static const u16 sTable68da[] = {
        0, 0xff, 0xff, 1, 2, 0xff, 2, 2, 3, 0xff, 0xff, 3, 0xff, 3, 0xff, 0xff, 3, 0xff, 0xff, 0xff, 4, 4,
    };
    u16 slot = ScriptReadAny(vm, env);
    u16 kind = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    u16 *var2 = ScriptReadVar(vm, env);
    u16 a = Pokewood_GetDownloadedMovieField(env, slot, 1, 0);
    u16 b = Pokewood_GetMovieField((u16)Pokewood_GetDownloadedMovieField(env, slot, 0, 0), 3) / 2 - 1;
    u16 column = sTable6872[b][a];
    u16 row = sTable68da[kind];

    if (row != 0xff) {
        *var = sTable68b2[row][column];
        *var2 = sTable688a[row][column];
    } else {
        *var = 0;
        *var2 = 0;
    }
    return FALSE;
}

const FieldScriptCommand POKEWOOD_SCRIPT_COMMANDS[] = {
    Scrcmd_Pokewood_Film,
    Scrcmd_Pokewood_WatchSlot,
    Scrcmd_Pokewood_PickMovie,
    Scrcmd_Pokewood_OpenMenu,
    Scrcmd_Pokewood_SetSaveSlot,
    Scrcmd_Pokewood_SetMessageWord,
    Scrcmd_Pokewood_GetMovieFlag,
    Scrcmd_Pokewood_SetMovieFlag,
    Scrcmd_Pokewood_CountDownloadedMovies,
    Scrcmd_Pokewood_CheckDownloadedMovie,
    Scrcmd_Pokewood_GetDownloadedMovieResult,
    Scrcmd_Pokewood_Create,
    Scrcmd_Pokewood_Free,
    Scrcmd_Pokewood_SelectCast,
    Scrcmd_Pokewood_SetUsePlayerCast,
    Scrcmd_Pokewood_GetNewUnlockKind,
    Scrcmd_Pokewood_TestAndSetLevelBit,
    Scrcmd_Pokewood_GetSlotFlag,
    func_ov062_021e5f1c,
    func_ov062_021e5f50,
    func_ov062_021e5fec,
    func_ov062_021e6010,
    Scrcmd_Pokewood_SetSlotFlag,
    Scrcmd_Pokewood_OpenRecordMonitor,
    func_ov062_021e60a8,
    Scrcmd_Pokewood_CountMovieFlags,
    Scrcmd_Pokewood_WatchBlock,
    (FieldScriptCommand)0xFFFFFFFF,
};
