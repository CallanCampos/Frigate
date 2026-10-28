// Transitions into and out of maps: the fades, wipes and the building-entrance effect that the field events run
// around a warp. Each creator allocates a TransitionEvent for its callback
#include "types.h"
#include "constants/directions.h"
#include "field/building_enter_effect.h"
#include "field/event_season_banner.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_lens_flare.h"
#include "field/field_msgbg.h"
#include "field/fld_exp_obj.h"
#include "field/zone.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "system/game_event.h"
#include "system/wipe.h"

typedef struct {
    GameSystem *gsys;
    Field *field;
    u32 type;
    u32 fadeMode;
    u32 fadeSlowness;
    u32 skipWait;
    u32 keepLCD;
    u32 alpha;
    u8 startSeason;
    u8 endSeason;
    EventSeasonBannerCallback callback;
    void *callbackArg;
} TransitionEvent;

u32 EventGameTransition_GetFadeKind(u32 type);
u32 EventGameTransition_LookupExitEffect(u32 from, u32 to);
u32 EventGameTransition_LookupEntranceEffect(u32 from, u32 to);
GameEvent *EventGameTransition_CreateFadeOutEventForEffect(GameSystem *gsys, Field *field, u32 type, u32 skipWait,
                                                           u32 fadeSlowness);
GameEvent *EventGameTransition_CreateFadeInEventForEffect(GameSystem *gsys, Field *field, u32 type, u32 skipWait,
                                                          u32 keepLCD);
GameEvent *EventGameTransition_CreateFadeOutEventDefault(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateFadeInEventDefault(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_SeasonBanner_Create(GameSystem *gsys, Field *field, u8 startSeason, u8 endSeason);
GameEvent *EventGameTransition_CreateWipeOutEvent(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateWipeOutEventUp(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateWipeInEventUp(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateWipeOutEventDown(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateWipeInEventDown(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateWipeOutEventLeft(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateWipeInEventLeft(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateWipeOutEventRight(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateWipeInEventRight(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_StartFadeOutByFacing(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_StartFadeInByFacing(GameSystem *gsys, Field *field);
GameEventReturnCode EventGameTransition_RunFadeOut(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunFadeIn(GameEvent *event, u32 *state, void *data);
GameEventReturnCode setupScreenForEnterBuilding(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunDefaultFadeIn(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_SeasonBanner_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunSeasonFadeInWithCallback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeOut(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeOutUp(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeOutDown(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeOutLeft(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeOutRight(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeInUp(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeInDown(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeInLeft(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeInRight(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunFadeInWithBgRestore(GameEvent *event, u32 *state, void *data);
void FieldG3D_SetLCDConfigKeepBGs(Field *field);
void EventGameTransition_ResetBgAffine(Field *field);

u32 GetOutTransitionTypeBetweenZones(u16 fromZone, u16 toZone) {
    return EventGameTransition_LookupExitEffect(GetMapTransitionEffectType(fromZone),
                                                GetMapTransitionEffectType(toZone));
}

u32 GetInTransitionTypeBetweenZones(u16 fromZone, u16 toZone) {
    return EventGameTransition_LookupEntranceEffect(GetMapTransitionEffectType(fromZone),
                                                    GetMapTransitionEffectType(toZone));
}

GameEvent *CallFieldMapEntranceOutTransitionDefault(GameSystem *gsys, Field *field, u32 type, u32 skipWait) {
    return CallFieldMapEntranceOutTransition(gsys, field, type, skipWait, -1);
}

GameEvent *CallFieldMapEntranceOutTransition(GameSystem *gsys, Field *field, u32 type, u32 skipWait, u32 fadeSlowness) {
    switch (type) {
    case FIELD_TRANSITION_BUILDING:
    default:
        return EventGameTransition_CreateFadeOutEventDefault(gsys, field);
    case FIELD_TRANSITION_FADE_BLACK:
    case FIELD_TRANSITION_FADE_WHITE:
    case FIELD_TRANSITION_SEASON:
        return EventGameTransition_CreateFadeOutEventForEffect(gsys, field, type, skipWait, fadeSlowness);
    case FIELD_TRANSITION_WIPE:
        return EventGameTransition_CreateWipeOutEvent(gsys, field);
    case FIELD_TRANSITION_WIPE_UP:
        return EventGameTransition_CreateWipeOutEventUp(gsys, field);
    case FIELD_TRANSITION_WIPE_DOWN:
        return EventGameTransition_CreateWipeOutEventDown(gsys, field);
    case FIELD_TRANSITION_WIPE_LEFT:
        return EventGameTransition_CreateWipeOutEventLeft(gsys, field);
    case FIELD_TRANSITION_WIPE_RIGHT:
        return EventGameTransition_CreateWipeOutEventRight(gsys, field);
    case FIELD_TRANSITION_WIPE_FACING:
        return EventGameTransition_StartFadeOutByFacing(gsys, field);
    }
}

GameEvent *CallFieldMapEntranceInTransition(GameSystem *gsys, Field *field, u32 type, u32 skipWait, u32 keepLCD,
                                            u8 startSeason, u8 endSeason) {
    switch (type) {
    case FIELD_TRANSITION_BUILDING:
    case FIELD_TRANSITION_WIPE:
    default:
        return EventGameTransition_CreateFadeInEventDefault(gsys, field);
    case FIELD_TRANSITION_FADE_BLACK:
    case FIELD_TRANSITION_FADE_WHITE:
        return EventGameTransition_CreateFadeInEventForEffect(gsys, field, type, skipWait, keepLCD);
    case FIELD_TRANSITION_SEASON:
        return EventGameTransition_SeasonBanner_Create(gsys, field, startSeason, endSeason);
    case FIELD_TRANSITION_WIPE_UP:
        return EventGameTransition_CreateWipeInEventUp(gsys, field);
    case FIELD_TRANSITION_WIPE_DOWN:
        return EventGameTransition_CreateWipeInEventDown(gsys, field);
    case FIELD_TRANSITION_WIPE_LEFT:
        return EventGameTransition_CreateWipeInEventLeft(gsys, field);
    case FIELD_TRANSITION_WIPE_RIGHT:
        return EventGameTransition_CreateWipeInEventRight(gsys, field);
    case FIELD_TRANSITION_WIPE_FACING:
        return EventGameTransition_StartFadeInByFacing(gsys, field);
    }
}

GameEvent *EventGameTransition_CreateSeasonFadeInEventWithCallback(GameSystem *gsys, Field *field, u8 startSeason,
                                                                   u8 endSeason, EventSeasonBannerCallback callback,
                                                                   void *callbackArg) {
    GameEvent *event =
        GameEvent_Create(gsys, NULL, EventGameTransition_RunSeasonFadeInWithCallback, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_SEASON;
    data->startSeason = startSeason;
    data->endSeason = endSeason;
    data->fadeMode = EventGameTransition_GetFadeKind(data->type);
    data->callback = callback;
    data->callbackArg = callbackArg;
    return event;
}

GameEvent *EventGameTransition_CreateFadeInEventWithBgRestore(GameSystem *gsys, Field *field, u32 type, u32 skipWait,
                                                              u32 fadeSlowness) {
    GameEvent *event =
        GameEvent_Create(gsys, NULL, EventGameTransition_RunFadeInWithBgRestore, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = type;
    data->skipWait = skipWait;
    data->fadeMode = EventGameTransition_GetFadeKind(type);
    data->fadeSlowness = fadeSlowness;
    return event;
}

GameEvent *EventGameTransition_CreateFadeOutEvent(GameSystem *gsys, Field *field, u32 type, u32 skipWait) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunFadeOut, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = type;
    data->skipWait = skipWait;
    data->fadeMode = EventGameTransition_GetFadeKind(type);
    return event;
}

u32 EventGameTransition_GetFadeKind(u32 type) {
    switch (type) {
    case FIELD_TRANSITION_FADE_BLACK:
    default:
        return FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK;
    case FIELD_TRANSITION_FADE_WHITE:
        return FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE;
    case FIELD_TRANSITION_BUILDING:
    case FIELD_TRANSITION_SEASON:
    case FIELD_TRANSITION_WIPE:
    case FIELD_TRANSITION_WIPE_UP:
    case FIELD_TRANSITION_WIPE_DOWN:
    case FIELD_TRANSITION_WIPE_LEFT:
    case FIELD_TRANSITION_WIPE_RIGHT:
    case FIELD_TRANSITION_WIPE_FACING:
        return FADE_ENGINE_B_BLACK;
    }
}

u32 EventGameTransition_LookupExitEffect(u32 from, u32 to) {
    const u32 table[6][6] = {
        { 9, 2, 9, 4, 9, 0 }, { 0, 0, 0, 4, 0, 0 }, { 0, 0, 2, 0, 0, 0 },
        { 1, 1, 1, 0, 1, 0 }, { 9, 0, 9, 0, 2, 0 }, { 0, 0, 0, 0, 0, 0 },
    };
    return table[from][to];
}

u32 EventGameTransition_LookupEntranceEffect(u32 from, u32 to) {
    const u32 table[6][6] = {
        { 9, 2, 0, 0, 9, 0 }, { 0, 0, 0, 0, 0, 0 }, { 9, 0, 2, 0, 9, 0 },
        { 1, 1, 1, 0, 1, 0 }, { 9, 0, 0, 0, 2, 0 }, { 0, 0, 0, 0, 0, 0 },
    };
    return table[from][to];
}

GameEvent *EventGameTransition_CreateFadeOutEventForEffect(GameSystem *gsys, Field *field, u32 type, u32 skipWait,
                                                           u32 fadeSlowness) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunFadeOut, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = type;
    data->skipWait = skipWait;
    data->fadeSlowness = fadeSlowness;
    data->fadeMode = EventGameTransition_GetFadeKind(type);
    return event;
}

GameEvent *EventGameTransition_CreateFadeInEventForEffect(GameSystem *gsys, Field *field, u32 type, u32 skipWait,
                                                          u32 keepLCD) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunFadeIn, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = type;
    data->skipWait = skipWait;
    data->fadeMode = EventGameTransition_GetFadeKind(type);
    data->keepLCD = keepLCD;
    return event;
}

GameEvent *EventGameTransition_CreateFadeOutEventDefault(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, setupScreenForEnterBuilding, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_BUILDING;
    data->fadeMode = EventGameTransition_GetFadeKind(FIELD_TRANSITION_BUILDING);
    return event;
}

GameEvent *EventGameTransition_CreateFadeInEventDefault(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunDefaultFadeIn, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_BUILDING;
    data->fadeMode = EventGameTransition_GetFadeKind(FIELD_TRANSITION_BUILDING);
    return event;
}

GameEvent *EventGameTransition_SeasonBanner_Create(GameSystem *gsys, Field *field, u8 startSeason, u8 endSeason) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_SeasonBanner_Callback, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_SEASON;
    data->startSeason = startSeason;
    data->endSeason = endSeason;
    data->fadeMode = EventGameTransition_GetFadeKind(data->type);
    data->callback = NULL;
    data->callbackArg = NULL;
    return event;
}

GameEvent *EventGameTransition_CreateWipeOutEvent(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeOut, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_WIPE;
    data->fadeMode = EventGameTransition_GetFadeKind(FIELD_TRANSITION_WIPE);
    return event;
}

GameEvent *EventGameTransition_CreateWipeOutEventUp(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeOutUp, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_WIPE_UP;
    data->fadeMode = EventGameTransition_GetFadeKind(FIELD_TRANSITION_WIPE_UP);
    return event;
}

GameEvent *EventGameTransition_CreateWipeInEventUp(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeInUp, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_WIPE_UP;
    data->fadeMode = EventGameTransition_GetFadeKind(FIELD_TRANSITION_WIPE_UP);
    return event;
}

GameEvent *EventGameTransition_CreateWipeOutEventDown(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeOutDown, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_WIPE_DOWN;
    data->fadeMode = EventGameTransition_GetFadeKind(FIELD_TRANSITION_WIPE_DOWN);
    return event;
}

GameEvent *EventGameTransition_CreateWipeInEventDown(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeInDown, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_WIPE_DOWN;
    data->fadeMode = EventGameTransition_GetFadeKind(FIELD_TRANSITION_WIPE_DOWN);
    return event;
}

GameEvent *EventGameTransition_CreateWipeOutEventLeft(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeOutLeft, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_WIPE_LEFT;
    data->fadeMode = EventGameTransition_GetFadeKind(FIELD_TRANSITION_WIPE_LEFT);
    return event;
}

GameEvent *EventGameTransition_CreateWipeInEventLeft(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeInLeft, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_WIPE_LEFT;
    data->fadeMode = EventGameTransition_GetFadeKind(FIELD_TRANSITION_WIPE_LEFT);
    return event;
}

GameEvent *EventGameTransition_CreateWipeOutEventRight(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeOutRight, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_WIPE_RIGHT;
    data->fadeMode = EventGameTransition_GetFadeKind(FIELD_TRANSITION_WIPE_RIGHT);
    return event;
}

GameEvent *EventGameTransition_CreateWipeInEventRight(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeInRight, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = FIELD_TRANSITION_WIPE_RIGHT;
    data->fadeMode = EventGameTransition_GetFadeKind(FIELD_TRANSITION_WIPE_RIGHT);
    return event;
}

GameEvent *EventGameTransition_StartFadeOutByFacing(GameSystem *gsys, Field *field) {
    switch (FieldPlayer_GetFaceDir(Field_GetPlayer(field))) {
    case DIR_LEFT:
    default:
        return EventGameTransition_CreateWipeOutEventLeft(gsys, field);
    case DIR_RIGHT:
        return EventGameTransition_CreateWipeOutEventRight(gsys, field);
    case DIR_UP:
        return EventGameTransition_CreateWipeOutEventUp(gsys, field);
    case DIR_DOWN:
        return EventGameTransition_CreateWipeOutEventDown(gsys, field);
    }
}

GameEvent *EventGameTransition_StartFadeInByFacing(GameSystem *gsys, Field *field) {
    switch (FieldPlayer_GetFaceDir(Field_GetPlayer(field))) {
    case DIR_LEFT:
    default:
        return EventGameTransition_CreateWipeInEventLeft(gsys, field);
    case DIR_RIGHT:
        return EventGameTransition_CreateWipeInEventRight(gsys, field);
    case DIR_UP:
        return EventGameTransition_CreateWipeInEventUp(gsys, field);
    case DIR_DOWN:
        return EventGameTransition_CreateWipeInEventDown(gsys, field);
    }
}

GameEventReturnCode EventGameTransition_RunFadeOut(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 0, 16, work->fadeSlowness);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        if (work->skipWait == 1) {
            return GAMEEVENT_DONE;
        }
        if (!GFL_FadeIsRunning()) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunFadeIn(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    switch (*state) {
    case 0:
        if (work->keepLCD != 0) {
            FieldG3D_SetLCDConfigKeepBGs(work->field);
            EventGameTransition_ResetBgAffine(work->field);
        }
        GFL_FadeSet(work->fadeMode, 16, 0, -1);
        FieldLensFlare_GreenlightStart(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        if (work->skipWait == 1) {
            return GAMEEVENT_DONE;
        }
        if (!GFL_FadeIsRunning()) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode setupScreenForEnterBuilding(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    if (*state == 0) {
        BGSysLCDConfig config;
        GFL_FadeSet(work->fadeMode, 0, 16, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        GX_SetBankForLCDC(8);
        reg_GX_DISPCAPCNT = 0x80330010;
        OS_WaitVBlankIntr();
        OS_WaitVBlankIntr();
        config = gEnterBuildingLCDConfig;
        GFL_BGSysSetLCDConfig(&config);
        reg_G2_BG2CNT = (reg_G2_BG2CNT & 0x43) | 0x4084;
        GX_SetBankForBG(GX_VRAM_BG_128_D);
        GFL_BGSysSetBGPriority(2, 0);
        GFL_BGSysSetBGEnabled(2, TRUE);
        setBuildingEnterVBlankCallback();
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunDefaultFadeIn(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    switch (*state) {
    case 0:
        Field_SetEffectRunningFlag(work->field, TRUE);
        work->alpha = 16;
        GFL_BGSysSetBGPriority(2, 0);
        GFL_BGSysSetBGPriority(0, 1);
        GFL_BGSysSetBGEnabled(0, TRUE);
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG0, work->alpha, 0);
        GFL_FadeSet(work->fadeMode, 16, 0, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        if (work->alpha != 0) {
            u32 alpha = --work->alpha;
            G2_ChangeBlendAlpha(alpha, 16 - alpha);
            break;
        }
        GFL_BGSysSetBGEnabled(2, FALSE);
        GX_SetBankForLCDC(8);
        MIi_CpuClearFast(0, (void *)HW_LCDC_VRAM, 0x20000);
        GX_DisableBankForLCDC();
        GX_SetBankForBG(GX_VRAM_BG_128_D);
        resetBuildingEnterVBlankCallback();
        (*state)++;
        break;
    case 2:
        FieldG3D_SetLCDConfigKeepBGs(work->field);
        (*state)++;
        break;
    case 3:
        FieldG2D_Prepare3DSurfaceDelayed(work->field);
        (*state)++;
        break;
    case 4: {
        FieldMsgBG *msgBGSys = Field_GetMsgBGSys(work->field);
        if (!MsgBGPrinter_IsSetupDone(msgBGSys)) {
            break;
        }
        MsgBGPrinter_LoadWindowGraphicsStep(msgBGSys, MSGBG_LOAD_PALETTES);
        (*state)++;
        break;
    }
    case 5:
        MsgBGPrinter_LoadWindowGraphicsStep(Field_GetMsgBGSys(work->field), MSGBG_LOAD_SYSTEM_FRAME);
        (*state)++;
        break;
    case 6:
        MsgBGPrinter_LoadWindowGraphicsStep(Field_GetMsgBGSys(work->field), MSGBG_LOAD_LIST_FRAME);
        (*state)++;
        break;
    case 7:
        (*state)++;
    case 8:
        Field_SetEffectRunningFlag(work->field, FALSE);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_SeasonBanner_Callback(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    GameSystem *gsys = work->gsys;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GameEvent_ChainNext(event,
                            EventSeasonBanner_CreateFieldTransition(gsys, field, work->startSeason, work->endSeason));
        (*state)++;
        break;
    case 1:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunSeasonFadeInWithCallback(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    GameSystem *gsys = work->gsys;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GameEvent_ChainNext(event,
                            EventSeasonBanner_CreateFieldTransitionEx(gsys, field, work->startSeason, work->endSeason,
                                                                      work->callback, work->callbackArg));
        (*state)++;
        break;
    case 1:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunWipeOut(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 0, 16, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(WIPE_MODE_MAIN, 8, 8, WIPE_COLOR_BLACK, 6, 1, Field_GetHeapID(field));
        (*state)++;
        break;
    case 2:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunWipeOutUp(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 0, 16, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(WIPE_MODE_MAIN, 2, 2, WIPE_COLOR_BLACK, 6, 1, Field_GetHeapID(field));
        (*state)++;
        break;
    case 2:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunWipeOutDown(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 0, 16, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(WIPE_MODE_MAIN, 4, 4, WIPE_COLOR_BLACK, 6, 1, Field_GetHeapID(field));
        (*state)++;
        break;
    case 2:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunWipeOutLeft(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 0, 16, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(WIPE_MODE_MAIN, 10, 10, WIPE_COLOR_BLACK, 6, 1, Field_GetHeapID(field));
        (*state)++;
        break;
    case 2:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunWipeOutRight(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 0, 16, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(WIPE_MODE_MAIN, 6, 6, WIPE_COLOR_BLACK, 6, 1, Field_GetHeapID(field));
        (*state)++;
        break;
    case 2:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunWipeInUp(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        FieldG3D_SetLCDConfigKeepBGs(field);
        EventGameTransition_ResetBgAffine(work->field);
        GFL_FadeSet(work->fadeMode, 16, 0, -1);
        FieldLensFlare_GreenlightStart(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(WIPE_MODE_MAIN, 3, 3, WIPE_COLOR_BLACK, 6, 1, Field_GetHeapID(field));
        (*state)++;
        break;
    case 2:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunWipeInDown(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        FieldG3D_SetLCDConfigKeepBGs(field);
        EventGameTransition_ResetBgAffine(work->field);
        GFL_FadeSet(work->fadeMode, 16, 0, -1);
        FieldLensFlare_GreenlightStart(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(WIPE_MODE_MAIN, 5, 5, WIPE_COLOR_BLACK, 6, 1, Field_GetHeapID(field));
        (*state)++;
        break;
    case 2:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunWipeInLeft(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        FieldG3D_SetLCDConfigKeepBGs(field);
        EventGameTransition_ResetBgAffine(work->field);
        GFL_FadeSet(work->fadeMode, 16, 0, -1);
        FieldLensFlare_GreenlightStart(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(WIPE_MODE_MAIN, 7, 7, WIPE_COLOR_BLACK, 6, 1, Field_GetHeapID(field));
        (*state)++;
        break;
    case 2:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunWipeInRight(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        FieldG3D_SetLCDConfigKeepBGs(field);
        EventGameTransition_ResetBgAffine(work->field);
        GFL_FadeSet(work->fadeMode, 16, 0, -1);
        FieldLensFlare_GreenlightStart(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(WIPE_MODE_MAIN, 11, 11, WIPE_COLOR_BLACK, 6, 1, Field_GetHeapID(field));
        (*state)++;
        break;
    case 2:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventGameTransition_RunFadeInWithBgRestore(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 16, 0, work->fadeSlowness);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        FieldG3D_SetLCDConfigKeepBGs(work->field);
        EventGameTransition_ResetBgAffine(work->field);
        (*state)++;
        break;
    case 1:
        if (work->skipWait == 1) {
            return GAMEEVENT_DONE;
        }
        if (!GFL_FadeIsRunning()) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

void FieldG3D_SetLCDConfigKeepBGs(Field *field) {
    u32 enabledA = GFL_BGSysGetEnabledBGsA();
    u32 enabledB = GFL_BGSysGetEnabledBGsB();
    FieldG2D_SetLCDConfig();
    GFL_BGSysSetEnabledBGsA(enabledA);
    GFL_BGSysSetEnabledBGsB(enabledB);
}

void EventGameTransition_ResetBgAffine(Field *field) {
    FieldG2D_Prepare3DSurface(field);
}
