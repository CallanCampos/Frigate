// Script commands 0x2b2 to 0x2b7, which read the data the save keeps at 0x68 of SaveControl (save/download_data.h): its
// flags, values and names, and the musical program and props in it. The name is descriptive; the data looks like what
// the game downloads
#include "types.h"
#include "field/field_script.h"
#include "field/scrcmd_download_data.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "save/download_data.h"
#include "save/musical_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/wordset.h"

BOOL Scrcmd_DownloadData_MemoryLinkCheckFlag(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    MemoryLinkSave *data =
        SaveControl_GetMemoryLink(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = MemoryLinkSave_IsMemoryFlagSet(data, index);
    return FALSE;
}

BOOL Scrcmd_DownloadData_MemoryLinkGetVar(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    MemoryLinkSave *data =
        SaveControl_GetMemoryLink(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = MemoryLinkSave_GetVar(data, index);
    return FALSE;
}

BOOL Scrcmd_DownloadData_MemoryLinkIsPlayerLinked(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    MemoryLinkSave *data =
        SaveControl_GetMemoryLink(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = MemoryLinkSave_IsPlayerLinked(data, index);
    return FALSE;
}

// Puts the name of the entry in the buffer
BOOL Scrcmd_DownloadData_WordSetMemoryLinkPlayerName(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    MemoryLinkSave *data =
        SaveControl_GetMemoryLink(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 index = ScriptReadAny(vm, env);
    u16 bufIndex = ScriptReadAny(vm, env);
    u8 value = MemoryLinkSave_GetPlayerGender(data, index);
    const u16 *name = MemoryLinkSave_GetPlayerName(data, index);
    StrBuf *strbuf = GFL_StrBufCreate(64, HEAPID_TAIL(heapId));

    GFL_StrBufLoadFixedString(strbuf, name, 8);
    GFL_WordSetSetStrbuf(wordSet, bufIndex, strbuf, value, 1, 2);
    GFL_StrBufFree(strbuf);
    return FALSE;
}

// Mode 0: whether the downloaded props were added. Mode 1: whether the musical's program is at least the downloaded one
BOOL Scrcmd_DownloadData_MemoryLinkCheckMusical(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    MusicalSave *musical = getAddressOfMusicalDataInfo(save);
    MemoryLinkSave *data = SaveControl_GetMemoryLink(save);
    u16 mode = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    switch (mode) {
    case 0:
        *result = MusicalSave_GetDownloadedProps(musical);
        break;
    case 1: {
        u16 current = FALSE;

        if (MemoryLinkSave_GetValue(data, MEMORY_LINK_KIND_MUSICAL_TOTAL_POINTS, NULL) <=
            MusicalSave_GetTotalPoints(musical)) {
            current = TRUE;
        }
        *result = current;
        break;
    }
    }
    return FALSE;
}

// Adds the downloaded props the musical doesn't have, and sets the result when it added any
BOOL Scrcmd_DownloadData_MemoryLinkAddMusicalProps(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    MemoryLinkSave *data = SaveControl_GetMemoryLink(save);
    MusicalSave *musical = getAddressOfMusicalDataInfo(save);
    u16 *result = ScriptReadVar(vm, env);
    const u8 *props = (const u8 *)MemoryLinkSave_GetValue(data, MEMORY_LINK_KIND_PROPS, NULL);
    int i;

    *result = FALSE;
    for (i = 0; i < 100; i++) {
        if (props[i / 8] & (1 << (i % 8))) {
            if (!MusicalSave_HasProp(musical, i)) {
                MusicalSave_AddProp(musical, i);
                *result = TRUE;
            }
        }
    }
    MusicalSave_SetDownloadedProps(musical, TRUE);
    return FALSE;
}
