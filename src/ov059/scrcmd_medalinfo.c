#include "types.h"
#include "app/medal_info.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/resort.h"
#include "field/scrcmd_resort.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

// The Join Avenue's command that shows a visitor's medals

static void func_ov059_021e7d68(void *arg);
static ResortPerson *func_ov059_021e7d7c(Field *field, FieldScriptEnv *env);

BOOL func_ov059_021e7c70(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    MedalInfoParam *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MedalInfoParam), TRUE, "scrcmd_medalinfo.c", 88);
    ResortPerson *person = func_ov059_021e7d7c(field, env);
    u16 date = func_ov137_021f10e8(person, 42, NULL);
    u16 name[8];
    u16 unkC;
    u16 unkE;
    u16 unk10;

    func_ov137_021f10e8(person, JOIN_AVE_PARAM_NAME, name);
    unkC = func_ov137_021f10e8(person, 41, NULL);
    unkE = func_ov137_021f10e8(person, 43, NULL);
    unk10 = func_ov137_021f10e8(person, 40, NULL);
    param->nameBuf = GFL_StrBufCreate(8, HEAPID_GAMEEVENT);
    GFL_StrBufLoadString(param->nameBuf, name);
    param->name = param->nameBuf;
    param->medal = unkC;
    param->year = (date >> 9) & 0x7f;
    param->month = (date >> 5) & 0xf;
    param->day = date & 0x1f;
    param->medalCount = unkE;
    param->rank = unk10;
    param->parentHeapId = 1;
    param->mode = 0;
    param->gameData = gameData;
    param->gsys = gsys;
    ScriptWork_CallEvent(work, EventFieldSubprocessTransition_CreateWithCallback(gsys, field, OVERLAY_MEDAL_INFO, &MedalInfo_ProcFunctions, param,
                                             func_ov059_021e7d68, param));
    return TRUE;
}

static void func_ov059_021e7d68(void *arg) {
    MedalInfoParam *param = arg;

    GFL_StrBufFree(param->nameBuf);
    GFL_HeapFree(param);
}

static ResortPerson *func_ov059_021e7d7c(Field *field, FieldScriptEnv *env) {
    ResortPeople *people = NULL;

    if (Field_CheckGimmickWorkPassword(field, 0)) {
        people = func_ov137_021f0e74(field);
    } else if (Field_CheckGimmickWorkPassword(field, 1)) {
        people = func_ov137_021eeeac(field);
    }
    return ResortSys_GetScriptPerson(env, people);
}
