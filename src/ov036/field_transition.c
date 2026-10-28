// Transitions into and out of maps: the fades, wipes and the building-entrance effect that the field events run
// around a warp. Each creator allocates a TransitionEvent for its callback
#include "types.h"
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
GameEvent *EventGameTransition_CreateFadeOutEventForEffect(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4);
GameEvent *EventGameTransition_CreateFadeInEventForEffect(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4);
GameEvent *EventGameTransition_CreateFadeOutEventDefault(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateFadeInEventDefault(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_SeasonBanner_Create(GameSystem *gsys, Field *field, u8 startSeason, u8 endSeason);
GameEvent *EventGameTransition_CreateFadeOutEventEffect4(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateFadeOutEventEffect5(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateFadeInEventEffect5(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateFadeOutEventEffect6(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateFadeInEventEffect6(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateFadeOutEventEffect7(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateFadeInEventEffect7(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateFadeOutEventEffect8(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_CreateFadeInEventEffect8(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_StartFadeOutByFacing(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_StartFadeInByFacing(GameSystem *gsys, Field *field);
GameEventReturnCode EventGameTransition_RunFadeOut(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunFadeIn(GameEvent *event, u32 *state, void *data);
GameEventReturnCode setupScreenForEnterBuilding(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunDefaultFadeIn(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_SeasonBanner_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunSeasonFadeInWithCallback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeOutEffect4(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeOutEffect5(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeOutEffect6(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeOutEffect7(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeOutEffect8(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeInEffect5(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeInEffect6(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeInEffect7(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_RunWipeInEffect8(GameEvent *event, u32 *state, void *data);
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

GameEvent *CallFieldMapEntranceOutTransitionDefault(GameSystem *gsys, Field *field, u32 type, u32 a3) {
    return CallFieldMapEntranceOutTransition(gsys, field, type, a3, -1);
}

GameEvent *CallFieldMapEntranceOutTransition(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4) {
    switch (type) {
    case 2:
    default:
        return EventGameTransition_CreateFadeOutEventDefault(gsys, field);
    case 0:
    case 1:
    case 3:
        return EventGameTransition_CreateFadeOutEventForEffect(gsys, field, type, a3, a4);
    case 4:
        return EventGameTransition_CreateFadeOutEventEffect4(gsys, field);
    case 5:
        return EventGameTransition_CreateFadeOutEventEffect5(gsys, field);
    case 6:
        return EventGameTransition_CreateFadeOutEventEffect6(gsys, field);
    case 7:
        return EventGameTransition_CreateFadeOutEventEffect7(gsys, field);
    case 8:
        return EventGameTransition_CreateFadeOutEventEffect8(gsys, field);
    case 9:
        return EventGameTransition_StartFadeOutByFacing(gsys, field);
    }
}

GameEvent *CallFieldMapEntranceInTransition(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4, u8 startSeason,
                                            u8 endSeason) {
    switch (type) {
    case 2:
    case 4:
    default:
        return EventGameTransition_CreateFadeInEventDefault(gsys, field);
    case 0:
    case 1:
        return EventGameTransition_CreateFadeInEventForEffect(gsys, field, type, a3, a4);
    case 3:
        return EventGameTransition_SeasonBanner_Create(gsys, field, startSeason, endSeason);
    case 5:
        return EventGameTransition_CreateFadeInEventEffect5(gsys, field);
    case 6:
        return EventGameTransition_CreateFadeInEventEffect6(gsys, field);
    case 7:
        return EventGameTransition_CreateFadeInEventEffect7(gsys, field);
    case 8:
        return EventGameTransition_CreateFadeInEventEffect8(gsys, field);
    case 9:
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
    data->type = 3;
    data->startSeason = startSeason;
    data->endSeason = endSeason;
    data->fadeMode = EventGameTransition_GetFadeKind(data->type);
    data->callback = callback;
    data->callbackArg = callbackArg;
    return event;
}

GameEvent *EventGameTransition_CreateFadeInEventWithBgRestore(GameSystem *gsys, Field *field, u32 a2, u32 a3, u32 a4) {
    GameEvent *event =
        GameEvent_Create(gsys, NULL, EventGameTransition_RunFadeInWithBgRestore, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = a2;
    data->skipWait = a3;
    data->fadeMode = EventGameTransition_GetFadeKind(a2);
    data->fadeSlowness = a4;
    return event;
}

GameEvent *EventGameTransition_CreateFadeOutEvent(GameSystem *gsys, Field *field, u32 a2, u32 a3) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunFadeOut, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = a2;
    data->skipWait = a3;
    data->fadeMode = EventGameTransition_GetFadeKind(a2);
    return event;
}

u32 EventGameTransition_GetFadeKind(u32 type) {
    switch (type) {
    case 0:
    default:
        return 3;
    case 1:
        return 12;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        return 2;
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

GameEvent *EventGameTransition_CreateFadeOutEventForEffect(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunFadeOut, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = type;
    data->skipWait = a3;
    data->fadeSlowness = a4;
    data->fadeMode = EventGameTransition_GetFadeKind(type);
    return event;
}

GameEvent *EventGameTransition_CreateFadeInEventForEffect(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunFadeIn, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = type;
    data->skipWait = a3;
    data->fadeMode = EventGameTransition_GetFadeKind(type);
    data->keepLCD = a4;
    return event;
}

GameEvent *EventGameTransition_CreateFadeOutEventDefault(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, setupScreenForEnterBuilding, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 2;
    data->fadeMode = EventGameTransition_GetFadeKind(2);
    return event;
}

GameEvent *EventGameTransition_CreateFadeInEventDefault(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunDefaultFadeIn, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 2;
    data->fadeMode = EventGameTransition_GetFadeKind(2);
    return event;
}

GameEvent *EventGameTransition_SeasonBanner_Create(GameSystem *gsys, Field *field, u8 startSeason, u8 endSeason) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_SeasonBanner_Callback, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 3;
    data->startSeason = startSeason;
    data->endSeason = endSeason;
    data->fadeMode = EventGameTransition_GetFadeKind(data->type);
    data->callback = NULL;
    data->callbackArg = NULL;
    return event;
}

GameEvent *EventGameTransition_CreateFadeOutEventEffect4(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeOutEffect4, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 4;
    data->fadeMode = EventGameTransition_GetFadeKind(4);
    return event;
}

GameEvent *EventGameTransition_CreateFadeOutEventEffect5(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeOutEffect5, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 5;
    data->fadeMode = EventGameTransition_GetFadeKind(5);
    return event;
}

GameEvent *EventGameTransition_CreateFadeInEventEffect5(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeInEffect5, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 5;
    data->fadeMode = EventGameTransition_GetFadeKind(5);
    return event;
}

GameEvent *EventGameTransition_CreateFadeOutEventEffect6(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeOutEffect6, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 6;
    data->fadeMode = EventGameTransition_GetFadeKind(6);
    return event;
}

GameEvent *EventGameTransition_CreateFadeInEventEffect6(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeInEffect6, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 6;
    data->fadeMode = EventGameTransition_GetFadeKind(6);
    return event;
}

GameEvent *EventGameTransition_CreateFadeOutEventEffect7(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeOutEffect7, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 7;
    data->fadeMode = EventGameTransition_GetFadeKind(7);
    return event;
}

GameEvent *EventGameTransition_CreateFadeInEventEffect7(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeInEffect7, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 7;
    data->fadeMode = EventGameTransition_GetFadeKind(7);
    return event;
}

GameEvent *EventGameTransition_CreateFadeOutEventEffect8(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeOutEffect8, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 8;
    data->fadeMode = EventGameTransition_GetFadeKind(8);
    return event;
}

GameEvent *EventGameTransition_CreateFadeInEventEffect8(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameTransition_RunWipeInEffect8, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 8;
    data->fadeMode = EventGameTransition_GetFadeKind(8);
    return event;
}

GameEvent *EventGameTransition_StartFadeOutByFacing(GameSystem *gsys, Field *field) {
    switch (FieldPlayer_GetFaceDir(Field_GetPlayer(field))) {
    case 2:
    default:
        return EventGameTransition_CreateFadeOutEventEffect7(gsys, field);
    case 3:
        return EventGameTransition_CreateFadeOutEventEffect8(gsys, field);
    case 0:
        return EventGameTransition_CreateFadeOutEventEffect5(gsys, field);
    case 1:
        return EventGameTransition_CreateFadeOutEventEffect6(gsys, field);
    }
}

GameEvent *EventGameTransition_StartFadeInByFacing(GameSystem *gsys, Field *field) {
    switch (FieldPlayer_GetFaceDir(Field_GetPlayer(field))) {
    case 2:
    default:
        return EventGameTransition_CreateFadeInEventEffect7(gsys, field);
    case 3:
        return EventGameTransition_CreateFadeInEventEffect8(gsys, field);
    case 0:
        return EventGameTransition_CreateFadeInEventEffect5(gsys, field);
    case 1:
        return EventGameTransition_CreateFadeInEventEffect6(gsys, field);
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
        MIi_CpuClearFast(0, (void *)0x6800000, 0x20000);
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

GameEventReturnCode EventGameTransition_RunWipeOutEffect4(GameEvent *event, u32 *state, void *data) {
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

GameEventReturnCode EventGameTransition_RunWipeOutEffect5(GameEvent *event, u32 *state, void *data) {
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

GameEventReturnCode EventGameTransition_RunWipeOutEffect6(GameEvent *event, u32 *state, void *data) {
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

GameEventReturnCode EventGameTransition_RunWipeOutEffect7(GameEvent *event, u32 *state, void *data) {
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

GameEventReturnCode EventGameTransition_RunWipeOutEffect8(GameEvent *event, u32 *state, void *data) {
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

GameEventReturnCode EventGameTransition_RunWipeInEffect5(GameEvent *event, u32 *state, void *data) {
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

GameEventReturnCode EventGameTransition_RunWipeInEffect6(GameEvent *event, u32 *state, void *data) {
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

GameEventReturnCode EventGameTransition_RunWipeInEffect7(GameEvent *event, u32 *state, void *data) {
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

GameEventReturnCode EventGameTransition_RunWipeInEffect8(GameEvent *event, u32 *state, void *data) {
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
