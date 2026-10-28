#include "types.h"
#include "app/local_tvt/local_tvt_callmode.h"
#include "app/local_tvt/local_tvt_chara.h"
#include "app/local_tvt/local_tvt_common.h"
#include "app/local_tvt/local_tvt_graphic.h"
#include "app/local_tvt/local_tvt_message.h"
#include "app/local_tvt/local_tvt_scriptmode.h"
#include "app/local_tvt/local_tvt_util.h"
#include "app/xtransceiver.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The proc of the Xtransceiver's calls with the game's characters, named after the ROM's "local_tvt_sys.c": it makes
// the parts every mode shares and runs the mode the parameters ask for

struct LocalTvtSys {
    u16 heapId;
    LocalTvtScriptMode *scriptMode;
    LocalTvtCallMode *callMode;
    LocalTvtParam *param;
    LocalTvtGraphic *graphic;
    LocalTvtChara *chara;
    LocalTvtMessage *message;
    LocalTvtBG *bg;
    void (*initMode)(LocalTvtSys *sys);
    BOOL (*mainMode)(LocalTvtSys *sys);
    void (*exitMode)(LocalTvtSys *sys);
};

static BOOL LocalTvt_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL LocalTvt_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL LocalTvt_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void LocalTvtSys_SetMode(LocalTvtSys *sys, u32 mode);
static void LocalTvtSys_InitScriptMode(LocalTvtSys *sys);
static BOOL LocalTvtSys_MainScriptMode(LocalTvtSys *sys);
static void LocalTvtSys_ExitScriptMode(LocalTvtSys *sys);
static void LocalTvtSys_InitCallMode(LocalTvtSys *sys);
static void LocalTvtSys_InitCallSecretMode(LocalTvtSys *sys);
static BOOL LocalTvtSys_MainCallMode(LocalTvtSys *sys);
static void LocalTvtSys_ExitCallMode(LocalTvtSys *sys);

const GameProcFunctions XTRANSCEIVER_PROC_FUNCTIONS = {
    LocalTvt_ProcInit,
    LocalTvt_ProcMain,
    LocalTvt_ProcExit,
};

static BOOL LocalTvt_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    LocalTvtSys *sys;
    LocalTvtParam *tvtParam = param;
    LocalTvtCharaSetup charaSetup;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_LOCAL_TVT, 0x40000);
    sys = GFL_ProcInitSubsystem(proc, sizeof(LocalTvtSys), HEAPID_LOCAL_TVT);
    // Without parameters it runs on its own, as a debug start would
    if (tvtParam == NULL) {
        InitZoneDataSystem(HEAPID_USER);
        tvtParam = GFL_HeapAllocate(HEAPID_LOCAL_TVT, sizeof(LocalTvtParam), FALSE, "local_tvt_sys.c", 104);
        tvtParam->gameData = GameData_Create(HEAPID_USER);
        tvtParam->scriptId = 1;
        if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_R) {
            tvtParam->scriptId = 5;
        }
    }
    sys->heapId = HEAPID_LOCAL_TVT;
    sys->param = tvtParam;
    LocalTvtSys_SetMode(sys, tvtParam->mode);
    sys->graphic = LocalTvtGraphic_Create(1, sys->heapId);
    sys->message = LocalTvtMessage_Create(sys->heapId);
    LocalTvtMessage_SetPlayerName(sys->message, sys->param->gameData);
    MI_CpuFill8(&charaSetup, 0, sizeof(charaSetup));
    charaSetup.unit = LocalTvtGraphic_GetClActUnit(sys->graphic);
    charaSetup.playerInfo = GetGameDataPlayerInfo(tvtParam->gameData);
    charaSetup.rival = getHollow_RivalData(GameData_GetSaveControl(tvtParam->gameData));
    charaSetup.message = sys->message;
    charaSetup.timeOfDay = LocalTvtUtil_GetTime(tvtParam->gameData);
    sys->chara = LocalTvtChara_Create(&charaSetup, sys->heapId);
    sys->bg = LocalTvtBG_Create(tvtParam->gameData, sys->heapId, sys->param->swapPalette);
    sys->initMode(sys);
    GFL_NetSetIconScreen(FALSE, sys->heapId);
    return TRUE;
}

static BOOL LocalTvt_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    LocalTvtSys *sys = work;

    sys->exitMode(sys);
    LocalTvtBG_Free(sys->bg);
    LocalTvtChara_Free(sys->chara);
    LocalTvtMessage_Free(sys->message);
    LocalTvtGraphic_Free(sys->graphic);
    if (param == NULL) {
        GameData_Free(sys->param->gameData);
        GFL_HeapFree(sys->param);
        FreeZoneDataSystem();
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_LOCAL_TVT);
    return TRUE;
}

static BOOL LocalTvt_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    LocalTvtSys *sys = work;
    BOOL finished = sys->mainMode(sys);

    LocalTvtMessage_Update(sys->message);
    LocalTvtChara_Update(sys->chara);
    LocalTvtGraphic_Update(sys->graphic);
    return finished == TRUE ? TRUE : FALSE;
}

static void LocalTvtSys_SetMode(LocalTvtSys *sys, u32 mode) {
    switch (mode) {
    case LOCAL_TVT_MODE_SCRIPT:
        sys->initMode = LocalTvtSys_InitScriptMode;
        sys->mainMode = LocalTvtSys_MainScriptMode;
        sys->exitMode = LocalTvtSys_ExitScriptMode;
        break;
    case LOCAL_TVT_MODE_CALL:
        sys->initMode = LocalTvtSys_InitCallMode;
        sys->mainMode = LocalTvtSys_MainCallMode;
        sys->exitMode = LocalTvtSys_ExitCallMode;
        break;
    case LOCAL_TVT_MODE_CALL_SECRET:
        sys->initMode = LocalTvtSys_InitCallSecretMode;
        sys->mainMode = LocalTvtSys_MainCallMode;
        sys->exitMode = LocalTvtSys_ExitCallMode;
        break;
    }
}

static void LocalTvtSys_InitScriptMode(LocalTvtSys *sys) {
    sys->scriptMode = LocalTvtScriptMode_Create(sys->heapId, sys->param->gsys, sys->param->gameData,
                                                sys->param->scriptId, sys->chara);
}

static BOOL LocalTvtSys_MainScriptMode(LocalTvtSys *sys) {
    return LocalTvtScriptMode_Main(sys->scriptMode);
}

static void LocalTvtSys_ExitScriptMode(LocalTvtSys *sys) {
    LocalTvtScriptMode_Free(sys->scriptMode);
}

static void LocalTvtSys_InitCallMode(LocalTvtSys *sys) {
    LocalTvtCallModeSetup setup;

    MI_CpuFill8(&setup, 0, sizeof(setup));
    setup.gameData = sys->param->gameData;
    setup.chara = sys->chara;
    setup.message = sys->message;
    setup.unit = LocalTvtGraphic_GetClActUnit(sys->graphic);
    setup.mode = LOCAL_TVT_CALL_MODE_SELECT;
    setup.gsys = sys->param->gsys;
    sys->callMode = LocalTvtCallMode_Create(&setup, sys->heapId);
}

static void LocalTvtSys_InitCallSecretMode(LocalTvtSys *sys) {
    LocalTvtCallModeSetup setup;

    MI_CpuFill8(&setup, 0, sizeof(setup));
    setup.gameData = sys->param->gameData;
    setup.chara = sys->chara;
    setup.message = sys->message;
    setup.unit = LocalTvtGraphic_GetClActUnit(sys->graphic);
    setup.mode = LOCAL_TVT_CALL_MODE_SECRET;
    setup.gsys = sys->param->gsys;
    sys->callMode = LocalTvtCallMode_Create(&setup, sys->heapId);
}

static BOOL LocalTvtSys_MainCallMode(LocalTvtSys *sys) {
    return LocalTvtCallMode_Main(sys->callMode);
}

static void LocalTvtSys_ExitCallMode(LocalTvtSys *sys) {
    LocalTvtCallMode_Free(sys->callMode);
}
