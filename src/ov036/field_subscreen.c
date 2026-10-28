// The field's touch screen: the table of its screens (the C-Gear, the field menu, the report, ...), changing between
// them behind a wipe, and the glue that loads each screen's overlay and calls it. The name is the ROM's own, from
// GFL_HeapAllocate's file argument. The struct's layout, the screen IDs, the steps of a change and the names of the
// exported functions that have one are swan's (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), and so are the
// names of the C-Gear's and the Dowsing MCHN's callbacks except FieldSubscreen_CGear_Draw. The other callbacks' names,
// FieldSubscreen_ResetReturnSubscreen, sFieldSubscreenDefs and sFieldMenuBGSetup are ours. First written by mariano54
// on their parallel-decomp branch
#include "types.h"
#include "constants/arc.h"
#include "field/beacon_view.h"
#include "field/beacon_view_common.h"
#include "field/c_gear_power_onoff.h"
#include "field/c_gear_sw.h"
#include "field/dowsing.h"
#include "field/field.h"
#include "field/field_disp_control.h"
#include "field/field_menu.h"
#include "field/fmission_view.h"
#include "field/inf_dungeon_retire.h"
#include "field/no_gear.h"
#include "field/player_state.h"
#include "field/report.h"
#include "field/subscreen.h"
#include "field/union_main.h"
#include "field/union_subdisp.h"
#include "field/wbt_subscreen.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "nitro/gx.h"
#include "save/c_gear_savedata.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/infowin.h"
#include "system/wipe.h"

// The previous screen that FieldSubscreen_Create gives the first screen's init. It is no screen, past
// FLD_SUBSCREEN_ID_MAX, so it stays out of swan's FieldSubscreenID
#define FLD_SUBSCREEN_ID_NONE 14

// The steps of changing the screen, swan's FieldSubscreenState. swan has no step 3, which nothing sets, and so numbers
// the last two one lower. FLD_SUBSCREEN_STATE_UNUSED is ours
enum FieldSubscreenState {
    FLD_SUBSCREEN_STATE_IDLE,
    FLD_SUBSCREEN_STATE_FADE_OUT_SET,
    FLD_SUBSCREEN_STATE_SWAP,
    FLD_SUBSCREEN_STATE_UNUSED,
    FLD_SUBSCREEN_STATE_FADE_IN_SET,
    FLD_SUBSCREEN_STATE_NEW_ENTER_WAIT,
};

struct FieldSubscreen {
    u32 screenId;
    u32 nextScreenId;
    u32 state;
    HeapID heapId;
    u32 returnScreenId;
    Field *field;
    // Called once the C-Gear shows after a FieldSubscreen_ReqChangeEx to FLD_SUBSCREEN_ID_CGEAR_WELCOME
    void (*callback)(void *work);
    void *callbackWork;
    // The current screen's work
    void *work;
    // Allocated but never used
    u32 unused[2];
};

typedef struct {
    u32 screenId;
    void (*init)(FieldSubscreen *subscreen, u32 previousScreenId);
    // active is whether no event is running
    void (*update)(FieldSubscreen *subscreen, BOOL active);
    void (*updateAltFrame)(FieldSubscreen *subscreen, BOOL active);
    GameEvent *(*checkEvent)(FieldSubscreen *subscreen, BOOL enabled);
    void (*terminate)(FieldSubscreen *subscreen);
    void (*draw)(FieldSubscreen *subscreen);
} FieldSubscreenDef;

static u32 FieldSubscreen_GetOverrideScreen(FieldSubscreen *subscreen, GameSystem *gsys, GameData *gameData);
static void FieldSubscreen_CGear_Init(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_CGear_InitPowerOn(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_CGear_Terminate(FieldSubscreen *subscreen);
static void FieldSubscreen_CGear_Draw(FieldSubscreen *subscreen);
static void FieldSubscreen_CGear_Update(FieldSubscreen *subscreen, BOOL active);
static GameEvent *FieldSubscreen_CGear_CheckEvent(FieldSubscreen *subscreen, BOOL enabled);
static void FieldSubscreen_NoGear_Init(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_NoGear_Terminate(FieldSubscreen *subscreen);
static void FieldSubscreen_NoGear_Draw(FieldSubscreen *subscreen);
static void FieldSubscreen_NoGear_Update(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_Wbt_Init(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_Wbt_Terminate(FieldSubscreen *subscreen);
static void FieldSubscreen_Wbt_Draw(FieldSubscreen *subscreen);
static void FieldSubscreen_Wbt_Update(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_FieldMenu_Init(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_FieldMenu_Terminate(FieldSubscreen *subscreen);
static void FieldSubscreen_FieldMenu_Update(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_FieldMenu_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_Union_Init(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_Union_Terminate(FieldSubscreen *subscreen);
static void FieldSubscreen_Union_Update(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_Union_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_BeaconView_Init(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_BeaconView_Terminate(FieldSubscreen *subscreen);
static void FieldSubscreen_BeaconView_Update(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_BeaconView_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active);
static GameEvent *FieldSubscreen_BeaconView_CheckEvent(FieldSubscreen *subscreen, BOOL enabled);
static void FieldSubscreen_FMission_Init(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_FMission_Terminate(FieldSubscreen *subscreen);
static void FieldSubscreen_FMission_Update(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_FMission_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active);
static GameEvent *FieldSubscreen_FMission_CheckEvent(FieldSubscreen *subscreen, BOOL enabled);
static void FieldSubscreen_Dowsing_Init(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_Dowsing_Terminate(FieldSubscreen *subscreen);
static void FieldSubscreen_Dowsing_Update(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_Dowsing_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_Report_Init(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_Report_Terminate(FieldSubscreen *subscreen);
static void FieldSubscreen_Report_Update(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_Report_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_CGearPowerOnOff_Init(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_CGearPowerOnOff_Update(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_CGearPowerOnOff_Terminate(FieldSubscreen *subscreen);
static void FieldSubscreen_InfDungeonRetire_Init(FieldSubscreen *subscreen, u32 previousScreenId);
static void FieldSubscreen_InfDungeonRetire_Update(FieldSubscreen *subscreen, BOOL active);
static void FieldSubscreen_InfDungeonRetire_Terminate(FieldSubscreen *subscreen);

static const BGSetup sFieldMenuBGSetup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x5800),
    GX_BG_CHARBASE(0),
    0x5800,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const FieldSubscreenDef sFieldSubscreenDefs[FLD_SUBSCREEN_ID_MAX] = {
    { FLD_SUBSCREEN_ID_DEFAULT, FieldSubscreen_CGear_Init, FieldSubscreen_CGear_Update, NULL,
      FieldSubscreen_CGear_CheckEvent, FieldSubscreen_CGear_Terminate, FieldSubscreen_CGear_Draw },
    { FLD_SUBSCREEN_ID_FIELD_MENU, FieldSubscreen_FieldMenu_Init, FieldSubscreen_FieldMenu_Update,
      FieldSubscreen_FieldMenu_UpdateAltFrame, NULL, FieldSubscreen_FieldMenu_Terminate, NULL },
    { FLD_SUBSCREEN_ID_UNION_ROOM_STANDBY, FieldSubscreen_Union_Init, FieldSubscreen_Union_Update,
      FieldSubscreen_Union_UpdateAltFrame, NULL, FieldSubscreen_Union_Terminate, NULL },
    { FLD_SUBSCREEN_ID_BLANK_STANDBY, FieldSubscreen_NoGear_Init, FieldSubscreen_NoGear_Update, NULL, NULL,
      FieldSubscreen_NoGear_Terminate, FieldSubscreen_NoGear_Draw },
    { FLD_SUBSCREEN_ID_ENTRALINK, FieldSubscreen_BeaconView_Init, FieldSubscreen_BeaconView_Update,
      FieldSubscreen_BeaconView_UpdateAltFrame, FieldSubscreen_BeaconView_CheckEvent,
      FieldSubscreen_BeaconView_Terminate, NULL },
    { FLD_SUBSCREEN_ID_BLANK_STANDBY_2, FieldSubscreen_NoGear_Init, FieldSubscreen_NoGear_Update, NULL, NULL,
      FieldSubscreen_NoGear_Terminate, FieldSubscreen_NoGear_Draw },
    { FLD_SUBSCREEN_ID_DOWSING, FieldSubscreen_Dowsing_Init, FieldSubscreen_Dowsing_Update,
      FieldSubscreen_Dowsing_UpdateAltFrame, NULL, FieldSubscreen_Dowsing_Terminate, NULL },
    { FLD_SUBSCREEN_ID_SAVE_REPORT, FieldSubscreen_Report_Init, FieldSubscreen_Report_Update,
      FieldSubscreen_Report_UpdateAltFrame, NULL, FieldSubscreen_Report_Terminate, NULL },
    { FLD_SUBSCREEN_ID_CGEAR_WELCOME, FieldSubscreen_CGear_InitPowerOn, FieldSubscreen_CGear_Update, NULL,
      FieldSubscreen_CGear_CheckEvent, FieldSubscreen_CGear_Terminate, FieldSubscreen_CGear_Draw },
    { FLD_SUBSCREEN_ID_CGEAR_POWEROFF, FieldSubscreen_CGearPowerOnOff_Init, FieldSubscreen_CGearPowerOnOff_Update, NULL,
      NULL, FieldSubscreen_CGearPowerOnOff_Terminate, NULL },
    { FLD_SUBSCREEN_ID_FUNFEST, FieldSubscreen_FMission_Init, FieldSubscreen_FMission_Update,
      FieldSubscreen_FMission_UpdateAltFrame, FieldSubscreen_FMission_CheckEvent, FieldSubscreen_FMission_Terminate,
      NULL },
    { FLD_SUBSCREEN_ID_PWT, FieldSubscreen_Wbt_Init, FieldSubscreen_Wbt_Update, NULL, NULL,
      FieldSubscreen_Wbt_Terminate, FieldSubscreen_Wbt_Draw },
    { FLD_SUBSCREEN_ID_INF_DUNGEON_RETIRE, FieldSubscreen_InfDungeonRetire_Init, FieldSubscreen_InfDungeonRetire_Update,
      NULL, NULL, FieldSubscreen_InfDungeonRetire_Terminate, NULL },
};

FieldSubscreen *FieldSubscreen_Create(u32 heapId, Field *field, u32 mode) {
    FieldSubscreen *subscreen = GFL_HeapAllocate(heapId, sizeof(FieldSubscreen), TRUE, "field_subscreen.c", 382);
    u32 screenId;

    subscreen->state = FLD_SUBSCREEN_STATE_IDLE;
    subscreen->heapId = heapId;
    subscreen->work = NULL;
    subscreen->field = field;
    screenId = FieldSubscreen_GetIDForChange(subscreen, mode);
    subscreen->screenId = screenId;
    subscreen->nextScreenId = screenId;
    sFieldSubscreenDefs[screenId].init(subscreen, FLD_SUBSCREEN_ID_NONE);
    return subscreen;
}

u8 FieldSubscreen_Free(FieldSubscreen *subscreen) {
    u8 screenId = subscreen->screenId;
    GameSystem *gsys = Field_GetGameSystem(subscreen->field);

    sFieldSubscreenDefs[subscreen->screenId].terminate(subscreen);
    if (screenId == FLD_SUBSCREEN_ID_FUNFEST && getStatusOfFesMission(GSYS_GetLinkFestival(gsys)) == 0) {
        screenId = FLD_SUBSCREEN_ID_DEFAULT;
    }
    GFL_HeapFree(subscreen);
    return screenId;
}

void FieldSubscreen_Update(FieldSubscreen *subscreen) {
    switch (subscreen->state) {
    case FLD_SUBSCREEN_STATE_IDLE:
        sFieldSubscreenDefs[subscreen->screenId].update(subscreen, !Field_IsEventRunning(subscreen->field));
        break;
    case FLD_SUBSCREEN_STATE_FADE_OUT_SET:
        GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 1, 1, subscreen->heapId);
        subscreen->state = FLD_SUBSCREEN_STATE_SWAP;
        break;
    case FLD_SUBSCREEN_STATE_SWAP:
        if (GFL_WipeIsFinished() == TRUE) {
            sFieldSubscreenDefs[subscreen->screenId].terminate(subscreen);
            sFieldSubscreenDefs[subscreen->nextScreenId].init(subscreen, subscreen->screenId);
            subscreen->screenId = subscreen->nextScreenId;
            subscreen->state = FLD_SUBSCREEN_STATE_FADE_IN_SET;
        }
        break;
    case FLD_SUBSCREEN_STATE_FADE_IN_SET:
        GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 1, 1, subscreen->heapId);
        subscreen->state = FLD_SUBSCREEN_STATE_NEW_ENTER_WAIT;
        break;
    case FLD_SUBSCREEN_STATE_NEW_ENTER_WAIT:
        if (GFL_WipeIsFinished() == TRUE) {
            subscreen->state = FLD_SUBSCREEN_STATE_IDLE;
        }
        sFieldSubscreenDefs[subscreen->screenId].update(subscreen, !Field_IsEventRunning(subscreen->field));
        break;
    }
}

void FieldSubscreen_UpdateAltFrame(FieldSubscreen *subscreen) {
    switch (subscreen->state) {
    case FLD_SUBSCREEN_STATE_IDLE:
        if (sFieldSubscreenDefs[subscreen->screenId].updateAltFrame != NULL) {
            sFieldSubscreenDefs[subscreen->screenId].updateAltFrame(subscreen, !Field_IsEventRunning(subscreen->field));
        }
        break;
    case FLD_SUBSCREEN_STATE_FADE_OUT_SET:
    case FLD_SUBSCREEN_STATE_SWAP:
    case FLD_SUBSCREEN_STATE_UNUSED:
    case FLD_SUBSCREEN_STATE_FADE_IN_SET:
    case FLD_SUBSCREEN_STATE_NEW_ENTER_WAIT:
        break;
    }
}

GameEvent *FieldSubscreen_CheckGameEvents(FieldSubscreen *subscreen, BOOL enabled) {
    if (subscreen->state == FLD_SUBSCREEN_STATE_IDLE && sFieldSubscreenDefs[subscreen->screenId].checkEvent != NULL) {
        return sFieldSubscreenDefs[subscreen->screenId].checkEvent(subscreen, enabled);
    }
    return NULL;
}

BOOL FieldSubscreen_IsReady(FieldSubscreen *subscreen) {
    if (subscreen->state == FLD_SUBSCREEN_STATE_IDLE) {
        return TRUE;
    }
    return FALSE;
}

// The screen the place or the game's state forces, or FLD_SUBSCREEN_ID_MAX if none does
static u32 FieldSubscreen_GetOverrideScreen(FieldSubscreen *subscreen, GameSystem *gsys, GameData *gameData) {
    u16 zoneId = Field_GetPlayerStateZoneID(subscreen->field);

    if (IsZoneRoyalUnova(zoneId) || IsZoneLEQ76(zoneId) || IsZoneEntralinkHub(zoneId) ||
        GetZoneIsEntreeForest(zoneId)) {
        return FLD_SUBSCREEN_ID_BLANK_STANDBY_2;
    }
    if (CGearSettings_IsOn(CGearSettings_GetFromSave(GameData_GetSaveControl(gameData))) == 0) {
        return FLD_SUBSCREEN_ID_BLANK_STANDBY_2;
    }
    if (getStatusOfFesMission(GSYS_GetLinkFestival(gsys)) != 0) {
        return FLD_SUBSCREEN_ID_FUNFEST;
    }
    return FLD_SUBSCREEN_ID_MAX;
}

u32 FieldSubscreen_GetIDForChange(FieldSubscreen *subscreen, u32 param) {
    GameSystem *gsys = Field_GetGameSystem(subscreen->field);
    GameData *gameData = GSYS_GetGameData(gsys);
    u32 screenId = FieldSubscreen_GetOverrideScreen(subscreen, gsys, gameData);

    if (param == FLD_SUBSCREEN_ID_DEFAULT) {
        u8 powerRequest = GameData_GetCGearPowerRequest(gameData);
        if (screenId != FLD_SUBSCREEN_ID_MAX) {
            return screenId;
        }
        if (powerRequest == 1) {
            if (GSYS_IsCGearOn(gsys) != 0) {
                return FLD_SUBSCREEN_ID_ENTRALINK;
            }
            GameData_SetCGearPowerRequest(gameData, 0);
        }
    } else if (param == FLD_SUBSCREEN_ID_ENTRALINK && screenId != FLD_SUBSCREEN_ID_MAX) {
        return screenId;
    }
    return param;
}

void FieldSubscreen_ReqChange(FieldSubscreen *subscreen, u32 mode) {
    subscreen->nextScreenId = FieldSubscreen_GetIDForChange(subscreen, mode);
    subscreen->state = FLD_SUBSCREEN_STATE_FADE_OUT_SET;
}

void FieldSubscreen_ChangeImm(FieldSubscreen *subscreen, u32 mode) {
    u32 screenId = FieldSubscreen_GetIDForChange(subscreen, mode);

    sFieldSubscreenDefs[subscreen->screenId].terminate(subscreen);
    subscreen->nextScreenId = screenId;
    sFieldSubscreenDefs[screenId].init(subscreen, subscreen->screenId);
    subscreen->screenId = subscreen->nextScreenId;
    subscreen->state = FLD_SUBSCREEN_STATE_IDLE;
}

void FieldSubscreen_ReqChangeEx(FieldSubscreen *subscreen, u32 mode, void (*callback)(void *work), void *work) {
    subscreen->callback = callback;
    subscreen->callbackWork = work;
    subscreen->nextScreenId = FieldSubscreen_GetIDForChange(subscreen, mode);
    subscreen->state = FLD_SUBSCREEN_STATE_FADE_OUT_SET;
}

u32 FieldSubscreen_GetScreenID(FieldSubscreen *subscreen) {
    return subscreen->screenId;
}

u32 FieldSubscreen_GetReturnSubscreen(FieldSubscreen *subscreen) {
    if (subscreen != NULL) {
        return subscreen->returnScreenId;
    }
    return FLD_SUBSCREEN_ID_DEFAULT;
}

void FieldSubscreen_SaveReturnSubscreen(FieldSubscreen *subscreen, u32 screenId) {
    if (subscreen != NULL) {
        subscreen->returnScreenId = screenId;
    }
}

void FieldSubscreen_ResetReturnSubscreen(FieldSubscreen *subscreen) {
    FieldSubscreen_SaveReturnSubscreen(subscreen, FLD_SUBSCREEN_ID_DEFAULT);
}

void FieldSubscreen_ResumeField(FieldSubscreen *subscreen, HeapID heapId) {
    FieldDispControl_ReqSetAllA(FieldG2D_GetDispControl(subscreen->field), 1, 0, 0, 0, 0);
    setAlphaBlend_wrapper(FALSE);
    GFL_BGSysClearScr(1);
    MsgBGPrinter_LoadWindowGraphics(1, heapId);
}

static void FieldSubscreen_CGear_Init(FieldSubscreen *subscreen, u32 previousScreenId) {
    GameSystem *gsys = Field_GetGameSystem(subscreen->field);
    CGearSettings *settings = CGearSettings_GetFromSave(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    u8 wasOn;
    u32 isOn;

    GFL_OvlLoad(OVERLAY_C_GEAR_SW);
    // The settings keep whether the C-Gear was on when it last showed, so that it plays its startup when it has been
    // turned on since
    wasOn = CGearSettings_GetWasOn(settings);
    isOn = GSYS_IsCGearOn(gsys);
    subscreen->work = CGearSw_Create(settings, subscreen, gsys, isOn == TRUE && wasOn == FALSE);
    CGearSettings_SetWasOn(settings, isOn);
    if (GSYS_IsLowBatteryCGearOff(gsys)) {
        GSYS_ClearLowBatteryCGearOff(gsys);
    }
}

static void FieldSubscreen_CGear_InitPowerOn(FieldSubscreen *subscreen, u32 previousScreenId) {
    GameSystem *gsys = Field_GetGameSystem(subscreen->field);
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    u32 isOn;
    CGearSettings *settings;

    GFL_OvlLoad(OVERLAY_C_GEAR_SW);
    isOn = GSYS_IsCGearOn(gsys);
    settings = CGearSettings_GetFromSave(save);
    subscreen->work = CGearSw_CreateWithCallback(settings, subscreen, Field_GetGameSystem(subscreen->field),
                                                 subscreen->callback, subscreen->callbackWork, isOn);
    CGearSettings_SetWasOn(settings, isOn);
    if (GSYS_IsLowBatteryCGearOff(gsys)) {
        GSYS_ClearLowBatteryCGearOff(gsys);
    }
    subscreen->screenId = FLD_SUBSCREEN_ID_DEFAULT;
    subscreen->nextScreenId = FLD_SUBSCREEN_ID_DEFAULT;
}

static void FieldSubscreen_CGear_Terminate(FieldSubscreen *subscreen) {
    CGearSw_Free(subscreen->work);
    GFL_OvlUnload(OVERLAY_C_GEAR_SW);
}

static void FieldSubscreen_CGear_Draw(FieldSubscreen *subscreen) {
    CGearSw_Draw(subscreen->work);
}

static void FieldSubscreen_CGear_Update(FieldSubscreen *subscreen, BOOL active) {
    if (active) {
        GameSystem *gsys = Field_GetGameSystem(subscreen->field);
        CGearSettings *settings = CGearSettings_GetFromSave(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
        if (GSYS_IsLowBatteryCGearOff(gsys)) {
            if (GSYS_IsCGearOn(gsys) != 0) {
                GSYS_ClearLowBatteryCGearOff(gsys);
            } else if (CGearSw_Reload(subscreen->work)) {
                CGearSettings_SetWasOn(settings, FALSE);
                GSYS_ClearLowBatteryCGearOff(gsys);
            }
        }
    }
    CGearSw_Update(subscreen->work, active);
}

static GameEvent *FieldSubscreen_CGear_CheckEvent(FieldSubscreen *subscreen, BOOL enabled) {
    return CGearSw_CheckEvent(subscreen->work, enabled, subscreen);
}

static void FieldSubscreen_NoGear_Init(FieldSubscreen *subscreen, u32 previousScreenId) {
    CGearSettings *settings;

    GFL_OvlLoad(OVERLAY_NO_GEAR);
    settings =
        CGearSettings_GetFromSave(GameData_GetSaveControl(GSYS_GetGameData(Field_GetGameSystem(subscreen->field))));
    subscreen->work = NoGear_Create(settings, subscreen, Field_GetGameSystem(subscreen->field));
}

static void FieldSubscreen_NoGear_Terminate(FieldSubscreen *subscreen) {
    NoGear_Free(subscreen->work);
    GFL_OvlUnload(OVERLAY_NO_GEAR);
}

static void FieldSubscreen_NoGear_Draw(FieldSubscreen *subscreen) {
    NoGear_Draw(subscreen->work);
}

static void FieldSubscreen_NoGear_Update(FieldSubscreen *subscreen, BOOL active) {
    NoGear_Update(subscreen->work);
}

static void FieldSubscreen_Wbt_Init(FieldSubscreen *subscreen, u32 previousScreenId) {
    CGearSettings *settings;

    GFL_OvlLoad(OVERLAY_WBT_SUBSCREEN);
    settings =
        CGearSettings_GetFromSave(GameData_GetSaveControl(GSYS_GetGameData(Field_GetGameSystem(subscreen->field))));
    subscreen->work = WbtSubscreen_Create(settings, subscreen, Field_GetGameSystem(subscreen->field));
}

static void FieldSubscreen_Wbt_Terminate(FieldSubscreen *subscreen) {
    WbtSubscreen_Free(subscreen->work);
    GFL_OvlUnload(OVERLAY_WBT_SUBSCREEN);
}

static void FieldSubscreen_Wbt_Draw(FieldSubscreen *subscreen) {
    WbtSubscreen_Draw(subscreen->work);
}

static void FieldSubscreen_Wbt_Update(FieldSubscreen *subscreen, BOOL active) {
    WbtSubscreen_Update(subscreen->work);
}

static void FieldSubscreen_FieldMenu_Init(FieldSubscreen *subscreen, u32 previousScreenId) {
    GameSystem *gsys = Field_GetGameSystem(subscreen->field);
    u32 zoneId = PlayerState_GetZoneID(GSYS_GetPlayerState(gsys));
    GameData *gameData = GSYS_GetGameData(gsys);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    BOOL open = FALSE;

    if (previousScreenId == FLD_SUBSCREEN_ID_DEFAULT || previousScreenId == FLD_SUBSCREEN_ID_BLANK_STANDBY_2 ||
        previousScreenId == FLD_SUBSCREEN_ID_UNION_ROOM_STANDBY || previousScreenId == FLD_SUBSCREEN_ID_BLANK_STANDBY ||
        previousScreenId == FLD_SUBSCREEN_ID_ENTRALINK || previousScreenId == FLD_SUBSCREEN_ID_FUNFEST ||
        previousScreenId == FLD_SUBSCREEN_ID_DOWSING) {
        open = TRUE;
    }
    GFL_BGSysCreateBG(BGSYS_BG_SUB, &sFieldMenuBGSetup, BGMODE_TEXT);
    GFL_BGSysSetBGEnabled(BGSYS_BG_SUB, TRUE);
    GFL_BGSysClearBG(BGSYS_BG_SUB);
    GFL_BGSysFillScr(BGSYS_BG_SUB, 0);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    subscreen->work = FieldMenu_Create(HEAPID_FIELD_MENU, subscreen->heapId, subscreen, subscreen->field, open);
    if (FieldMenu_GetMenuType(gameData, eventWork, zoneId) != FIELD_MENU_TYPE_UNION_ROOM) {
        InfoWin_Init(BGSYS_BG_SUB, 14, gameData, subscreen->heapId);
    }
    FieldMenu_LoadBar(subscreen->heapId, open);
}

static void FieldSubscreen_FieldMenu_Terminate(FieldSubscreen *subscreen) {
    GameSystem *gsys = Field_GetGameSystem(subscreen->field);
    u32 zoneId = PlayerState_GetZoneID(GSYS_GetPlayerState(gsys));
    GameData *gameData = GSYS_GetGameData(gsys);

    if (FieldMenu_GetMenuType(gameData, GameData_GetEventWork(gameData), zoneId) != FIELD_MENU_TYPE_UNION_ROOM) {
        InfoWin_Exit();
    }
    FieldMenu_Free(subscreen->work);
    GFL_BGSysReleaseBG(BGSYS_BG_SUB);
}

static void FieldSubscreen_FieldMenu_Update(FieldSubscreen *subscreen, BOOL active) {
    FieldMenu_Update(subscreen->work);
    InfoWin_Update();
}

static void FieldSubscreen_FieldMenu_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active) {
    FieldMenu_UpdateScroll(subscreen->work);
}

u32 FieldSubscreen_GetMenuSelectedButton(FieldSubscreen *subscreen) {
    if (FieldSubscreen_GetScreenID(subscreen) != FLD_SUBSCREEN_ID_FIELD_MENU) {
        return 0;
    }
    return FieldMenu_GetSelectedItem(subscreen->work);
}

void FieldSubscreen_SetMenuSelectedButton(FieldSubscreen *subscreen, u32 param) {
    if (FieldSubscreen_GetScreenID(subscreen) == FLD_SUBSCREEN_ID_FIELD_MENU) {
        FieldMenu_SetCursorItem(subscreen->work, param);
    }
}

void FieldSubscreen_LoadBackdrop(FieldSubscreen *subscreen, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_FIELD_MENU, heapId);

    GFL_G2DIOLoadArcNCLR(arc, 0, PALTYPE_MAIN_BG, 0x140, 0x140, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, 1, 100, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 17, 1, 0, 100, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    FieldDispControl_ReqSetAllA(FieldG2D_GetDispControl(subscreen->field), 1, 0, 0, 0, -6);
    FieldDispControl_ReqSetBGEnabled(FieldG2D_GetDispControl(subscreen->field), 1, TRUE);
}

CGearSw *FieldSubscreen_GetCGear(FieldSubscreen *subscreen) {
    if (subscreen->screenId == FLD_SUBSCREEN_ID_DEFAULT || subscreen->screenId == FLD_SUBSCREEN_ID_CGEAR_WELCOME) {
        return subscreen->work;
    }
    return NULL;
}

static void FieldSubscreen_Union_Init(FieldSubscreen *subscreen, u32 previousScreenId) {
    GFL_OvlLoad(OVERLAY_UNION_SUBDISP);
    subscreen->work = UnionSubdisp_Create(Field_GetGameSystem(subscreen->field));
}

static void FieldSubscreen_Union_Terminate(FieldSubscreen *subscreen) {
    UnionSubdisp_Free(subscreen->work);
    GFL_OvlUnload(OVERLAY_UNION_SUBDISP);
}

static void FieldSubscreen_Union_Update(FieldSubscreen *subscreen, BOOL active) {
    GameSystem *gsys = Field_GetGameSystem(subscreen->field);

    if (active == TRUE && !UnionMain_IsIdle(gsys)) {
        active = FALSE;
    }
    UnionSubdisp_Update(subscreen->work, active);
}

static void FieldSubscreen_Union_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active) {
    GameSystem *gsys = Field_GetGameSystem(subscreen->field);

    if (active == TRUE && !UnionMain_IsIdle(gsys)) {
        active = FALSE;
    }
    UnionSubdisp_UpdateAltFrame(subscreen->work, active);
}

static void FieldSubscreen_BeaconView_Init(FieldSubscreen *subscreen, u32 previousScreenId) {
    GFL_OvlLoad(OVERLAY_BEACON_VIEW_COMMON);
    GFL_OvlLoad(OVERLAY_BEACON_VIEW);
    subscreen->work = BeaconView_Init(Field_GetGameSystem(subscreen->field), subscreen);
}

static void FieldSubscreen_BeaconView_Terminate(FieldSubscreen *subscreen) {
    BeaconView_Exit(subscreen->work);
    GFL_OvlUnload(OVERLAY_BEACON_VIEW);
    GFL_OvlUnload(OVERLAY_BEACON_VIEW_COMMON);
}

static void FieldSubscreen_BeaconView_Update(FieldSubscreen *subscreen, BOOL active) {
    BeaconView_Update(subscreen->work, active);
}

// Prints the view's queued text, whether or not the queue empties
static void FieldSubscreen_BeaconView_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active) {
    BeaconView_Draw(subscreen->work);
}

static GameEvent *FieldSubscreen_BeaconView_CheckEvent(FieldSubscreen *subscreen, BOOL enabled) {
    return BeaconView_CheckEvent(subscreen->work, enabled);
}

static void FieldSubscreen_FMission_Init(FieldSubscreen *subscreen, u32 previousScreenId) {
    GFL_OvlLoad(OVERLAY_BEACON_VIEW_COMMON);
    GFL_OvlLoad(OVERLAY_FMISSION_VIEW);
    subscreen->work = FMissionView_Create(Field_GetGameSystem(subscreen->field), subscreen);
}

static void FieldSubscreen_FMission_Terminate(FieldSubscreen *subscreen) {
    FMissionView_Free(subscreen->work);
    GFL_OvlUnload(OVERLAY_FMISSION_VIEW);
    GFL_OvlUnload(OVERLAY_BEACON_VIEW_COMMON);
}

static void FieldSubscreen_FMission_Update(FieldSubscreen *subscreen, BOOL active) {
    FMissionView_Update(subscreen->work, active);
}

// Prints the view's queued text, whether or not the queue empties
static void FieldSubscreen_FMission_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active) {
    FMissionView_IsPrinting(subscreen->work);
}

static GameEvent *FieldSubscreen_FMission_CheckEvent(FieldSubscreen *subscreen, BOOL enabled) {
    return FMissionView_CheckEvent(subscreen->work, enabled);
}

static void FieldSubscreen_Dowsing_Init(FieldSubscreen *subscreen, u32 previousScreenId) {
    GFL_OvlLoad(OVERLAY_DOWSING);
    subscreen->work = Dowsing_Create(HEAPID_FIELD_MENU, subscreen->heapId, subscreen, subscreen->field);
}

static void FieldSubscreen_Dowsing_Terminate(FieldSubscreen *subscreen) {
    Dowsing_Free(subscreen->work);
    GFL_OvlUnload(OVERLAY_DOWSING);
}

static void FieldSubscreen_Dowsing_Update(FieldSubscreen *subscreen, BOOL active) {
    Dowsing_Update(subscreen->work, active);
}

static void FieldSubscreen_Dowsing_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active) {
    Dowsing_UpdateAltFrame(subscreen->work, active);
}

static void FieldSubscreen_Report_Init(FieldSubscreen *subscreen, u32 previousScreenId) {
    GFL_OvlLoad(OVERLAY_REPORT);
    subscreen->work = Report_Create(Field_GetGameSystem(subscreen->field), subscreen->heapId);
}

static void FieldSubscreen_Report_Terminate(FieldSubscreen *subscreen) {
    Report_Free(subscreen->work);
    GFL_OvlUnload(OVERLAY_REPORT);
}

static void FieldSubscreen_Report_Update(FieldSubscreen *subscreen, BOOL active) {
    Report_Update(subscreen->work);
}

static void FieldSubscreen_Report_UpdateAltFrame(FieldSubscreen *subscreen, BOOL active) {
    Report_UpdateAltFrame(subscreen->work);
}

BOOL FieldSubscreen_Report_IsReady(FieldSubscreen *subscreen) {
    return Report_IsReady(subscreen->work);
}

void FieldSubscreen_Report_InitSave(FieldSubscreen *subscreen) {
    Report_InitSave(subscreen->work);
}

void FieldSubscreen_Report_StartBar(FieldSubscreen *subscreen) {
    Report_StartBar(subscreen->work);
}

BOOL FieldSubscreen_Report_IsBarFull(FieldSubscreen *subscreen) {
    return Report_IsBarFull(subscreen->work);
}

void FieldSubscreen_Report_StopBar(FieldSubscreen *subscreen) {
    Report_StopBar(subscreen->work);
}

BOOL FieldSubscreen_Report_IsLargeSave(FieldSubscreen *subscreen) {
    return Report_IsLargeSave(subscreen->work);
}

static void FieldSubscreen_CGearPowerOnOff_Init(FieldSubscreen *subscreen, u32 previousScreenId) {
    GFL_OvlLoad(OVERLAY_C_GEAR_POWER_ONOFF);
    subscreen->work = CGearPowerOnOff_Create(subscreen, Field_GetGameSystem(subscreen->field), HEAPID_FIELD_MENU);
}

static void FieldSubscreen_CGearPowerOnOff_Update(FieldSubscreen *subscreen, BOOL active) {
    CGearPowerOnOff_Update(subscreen->work, active);
}

static void FieldSubscreen_CGearPowerOnOff_Terminate(FieldSubscreen *subscreen) {
    CGearPowerOnOff_Free(subscreen->work);
    GFL_OvlUnload(OVERLAY_C_GEAR_POWER_ONOFF);
}

static void FieldSubscreen_InfDungeonRetire_Init(FieldSubscreen *subscreen, u32 previousScreenId) {
    subscreen->work = InfDungeonRetire_Create(subscreen, Field_GetGameSystem(subscreen->field), HEAPID_FIELD_MENU);
}

static void FieldSubscreen_InfDungeonRetire_Update(FieldSubscreen *subscreen, BOOL active) {
    InfDungeonRetire_Update(subscreen->work);
}

static void FieldSubscreen_InfDungeonRetire_Terminate(FieldSubscreen *subscreen) {
    InfDungeonRetire_Free(subscreen->work);
}
