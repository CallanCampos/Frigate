#include "types.h"
#include "app/mb_parent.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

// The script plugin of the Poké Transfer Lab (plugin 4), commands from 1000

enum {
    PAL_PARK_INFO_RESULT,
    PAL_PARK_INFO_HIGH_SCORE,
};

// Runs Poké Transfer, whose DS Download Play parent sends the game to the other system
static BOOL Scrcmd_Palpark_CallMbParent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    MBParentParam *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MBParentParam), FALSE, "scrcmd_palpark.c", 68);

    param->startMenu = FALSE;
    param->gameData = gameData;
    ScriptWork_CallEvent(work, EventFieldSubprocessCall_CreateWithCallback(
                                   gsys, field, OVERLAY_ID(181), &MB_PARENT_PROC_FUNCTIONS, param, NULL, param));
    return TRUE;
}

// Sets a variable to the last Poké Transfer's result or the high score (PAL_PARK_INFO_*, a byte in the script)
static BOOL Scrcmd_Palpark_GetInfo(VM *vm, FieldScriptEnv *env) {
    u8 info = VM_Read8(vm);
    u16 *var = ScriptReadVar(vm, env);
    TrainerGameInfoSave *gameInfo = getTrainerGameInfoAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));

    switch (info) {
    case PAL_PARK_INFO_RESULT:
        *var = TrainerGameInfo_GetPalParkResult(gameInfo);
        break;
    case PAL_PARK_INFO_HIGH_SCORE:
        *var = TrainerGameInfo_GetPalParkHighScore(gameInfo);
        break;
    }
    return FALSE;
}

const FieldScriptCommand PAL_PARK_SCRIPT_COMMANDS[] = {
    Scrcmd_Palpark_CallMbParent,
    Scrcmd_Palpark_GetInfo,
    (FieldScriptCommand)0xFFFFFFFF,
};
