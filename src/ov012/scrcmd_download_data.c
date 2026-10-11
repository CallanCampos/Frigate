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

BOOL func_ov012_0216a950(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *data = func_020074d8(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = func_02010b90(data, index);
    return FALSE;
}

BOOL func_ov012_0216a994(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *data = func_020074d8(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = func_02010bc0(data, index);
    return FALSE;
}

BOOL func_ov012_0216a9d8(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *data = func_020074d8(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = func_02010bcc(data, index);
    return FALSE;
}

// Puts the name of the entry in the buffer
BOOL func_ov012_0216aa1c(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    void *data = func_020074d8(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 index = ScriptReadAny(vm, env);
    u16 bufIndex = ScriptReadAny(vm, env);
    u8 value = func_02010bd8(data, index);
    const u16 *name = func_02010be4(data, index);
    StrBuf *strbuf = GFL_StrBufCreate(64, HEAPID_TAIL(heapId));

    GFL_StrBufLoadFixedString(strbuf, name, 8);
    func_0202437c(wordSet, bufIndex, strbuf, value, 1, 2);
    GFL_StrBufFree(strbuf);
    return FALSE;
}

// Mode 0: whether the downloaded props were added. Mode 1: whether the musical's program is at least the downloaded one
BOOL func_ov012_0216aabc(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    MusicalSave *musical = getAddressOfMusicalDataInfo(save);
    void *data = func_020074d8(save);
    u16 mode = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    switch (mode) {
    case 0:
        *result = func_0200af5c(musical);
        break;
    case 1: {
        u16 current = FALSE;

        if (func_02010b0c(data, 0, NULL) <= func_0200af38(musical)) {
            current = TRUE;
        }
        *result = current;
        break;
    }
    }
    return FALSE;
}

// Adds the downloaded props the musical doesn't have, and sets the result when it added any
BOOL func_ov012_0216ab30(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    void *data = func_020074d8(save);
    MusicalSave *musical = getAddressOfMusicalDataInfo(save);
    u16 *result = ScriptReadVar(vm, env);
    const u8 *props = (const u8 *)func_02010b0c(data, 0xb, NULL);
    int i;

    *result = FALSE;
    for (i = 0; i < 100; i++) {
        if (props[i / 8] & (1 << (i % 8))) {
            if (!func_0200ad60(musical, i)) {
                func_0200add8(musical, i);
                *result = TRUE;
            }
        }
    }
    func_0200af64(musical, TRUE);
    return FALSE;
}
