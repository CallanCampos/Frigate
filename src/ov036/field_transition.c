// Transitions into and out of maps: the fades, wipes and the building-entrance effect that the field events run
// around a warp. Each creator allocates a TransitionEvent for its callback
#include "types.h"
#include "field/building_enter_effect.h"
#include "field/event_season_banner.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_lens_flare.h"
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

u32 func_ov036_021b88cc(u32 type);
u32 func_ov036_021b88fc(u32 from, u32 to);
u32 func_ov036_021b892c(u32 from, u32 to);
GameEvent *func_ov036_021b895c(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4);
GameEvent *func_ov036_021b899c(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4);
GameEvent *func_ov036_021b89dc(GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b8a0c(GameSystem *gsys, Field *field);
GameEvent *EventGameTransition_SeasonBanner_Create(GameSystem *gsys, Field *field, u8 startSeason, u8 endSeason);
GameEvent *func_ov036_021b8a88(GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b8ab8(GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b8ae8(GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b8b18(GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b8b48(GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b8b78(GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b8ba8(GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b8bd8(GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b8c08(GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b8c38(GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b8c88(GameSystem *gsys, Field *field);
GameEventReturnCode func_ov036_021b8cd8(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b8d24(GameEvent *event, u32 *state, void *data);
GameEventReturnCode setupScreenForEnterBuilding(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b8e10(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventGameTransition_SeasonBanner_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b8f74(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b8fc0(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b9038(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b90b0(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b9128(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b91a0(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b9214(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b9294(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b9314(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b9394(GameEvent *event, u32 *state, void *data);
GameEventReturnCode func_ov036_021b9414(GameEvent *event, u32 *state, void *data);
void FieldG3D_SetLCDConfigKeepBGs(Field *field);
void func_ov036_021b948c(Field *field);

u32 GetOutTransitionTypeBetweenZones(u16 fromZone, u16 toZone) {
    return func_ov036_021b88fc(GetMapTransitionEffectType(fromZone), GetMapTransitionEffectType(toZone));
}

u32 GetInTransitionTypeBetweenZones(u16 fromZone, u16 toZone) {
    return func_ov036_021b892c(GetMapTransitionEffectType(fromZone), GetMapTransitionEffectType(toZone));
}

GameEvent *CallFieldMapEntranceOutTransitionDefault(GameSystem *gsys, Field *field, u32 type, u32 a3) {
    return CallFieldMapEntranceOutTransition(gsys, field, type, a3, -1);
}

GameEvent *CallFieldMapEntranceOutTransition(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4) {
    switch (type) {
    case 2:
    default:
        return func_ov036_021b89dc(gsys, field);
    case 0:
    case 1:
    case 3:
        return func_ov036_021b895c(gsys, field, type, a3, a4);
    case 4:
        return func_ov036_021b8a88(gsys, field);
    case 5:
        return func_ov036_021b8ab8(gsys, field);
    case 6:
        return func_ov036_021b8b18(gsys, field);
    case 7:
        return func_ov036_021b8b78(gsys, field);
    case 8:
        return func_ov036_021b8bd8(gsys, field);
    case 9:
        return func_ov036_021b8c38(gsys, field);
    }
}

GameEvent *CallFieldMapEntranceInTransition(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4, u8 startSeason,
                                            u8 endSeason) {
    switch (type) {
    case 2:
    case 4:
    default:
        return func_ov036_021b8a0c(gsys, field);
    case 0:
    case 1:
        return func_ov036_021b899c(gsys, field, type, a3, a4);
    case 3:
        return EventGameTransition_SeasonBanner_Create(gsys, field, startSeason, endSeason);
    case 5:
        return func_ov036_021b8ae8(gsys, field);
    case 6:
        return func_ov036_021b8b48(gsys, field);
    case 7:
        return func_ov036_021b8ba8(gsys, field);
    case 8:
        return func_ov036_021b8c08(gsys, field);
    case 9:
        return func_ov036_021b8c88(gsys, field);
    }
}

GameEvent *func_ov036_021b8800(GameSystem *gsys, Field *field, u8 startSeason, u8 endSeason,
                               EventSeasonBannerCallback callback, void *callbackArg) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b8f74, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 3;
    data->startSeason = startSeason;
    data->endSeason = endSeason;
    data->fadeMode = func_ov036_021b88cc(data->type);
    data->callback = callback;
    data->callbackArg = callbackArg;
    return event;
}

GameEvent *func_ov036_021b8850(GameSystem *gsys, Field *field, u32 a2, u32 a3, u32 a4) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b9414, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = a2;
    data->skipWait = a3;
    data->fadeMode = func_ov036_021b88cc(a2);
    data->fadeSlowness = a4;
    return event;
}

GameEvent *func_ov036_021b8890(GameSystem *gsys, Field *field, u32 a2, u32 a3) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b8cd8, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = a2;
    data->skipWait = a3;
    data->fadeMode = func_ov036_021b88cc(a2);
    return event;
}

u32 func_ov036_021b88cc(u32 type) {
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

u32 func_ov036_021b88fc(u32 from, u32 to) {
    const u32 table[6][6] = {
        { 9, 2, 9, 4, 9, 0 }, { 0, 0, 0, 4, 0, 0 }, { 0, 0, 2, 0, 0, 0 },
        { 1, 1, 1, 0, 1, 0 }, { 9, 0, 9, 0, 2, 0 }, { 0, 0, 0, 0, 0, 0 },
    };
    return table[from][to];
}

u32 func_ov036_021b892c(u32 from, u32 to) {
    const u32 table[6][6] = {
        { 9, 2, 0, 0, 9, 0 }, { 0, 0, 0, 0, 0, 0 }, { 9, 0, 2, 0, 9, 0 },
        { 1, 1, 1, 0, 1, 0 }, { 9, 0, 0, 0, 2, 0 }, { 0, 0, 0, 0, 0, 0 },
    };
    return table[from][to];
}

GameEvent *func_ov036_021b895c(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b8cd8, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = type;
    data->skipWait = a3;
    data->fadeSlowness = a4;
    data->fadeMode = func_ov036_021b88cc(type);
    return event;
}

GameEvent *func_ov036_021b899c(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b8d24, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = type;
    data->skipWait = a3;
    data->fadeMode = func_ov036_021b88cc(type);
    data->keepLCD = a4;
    return event;
}

GameEvent *func_ov036_021b89dc(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, setupScreenForEnterBuilding, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 2;
    data->fadeMode = func_ov036_021b88cc(2);
    return event;
}

GameEvent *func_ov036_021b8a0c(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b8e10, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 2;
    data->fadeMode = func_ov036_021b88cc(2);
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
    data->fadeMode = func_ov036_021b88cc(data->type);
    data->callback = NULL;
    data->callbackArg = NULL;
    return event;
}

GameEvent *func_ov036_021b8a88(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b8fc0, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 4;
    data->fadeMode = func_ov036_021b88cc(4);
    return event;
}

GameEvent *func_ov036_021b8ab8(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b9038, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 5;
    data->fadeMode = func_ov036_021b88cc(5);
    return event;
}

GameEvent *func_ov036_021b8ae8(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b9214, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 5;
    data->fadeMode = func_ov036_021b88cc(5);
    return event;
}

GameEvent *func_ov036_021b8b18(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b90b0, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 6;
    data->fadeMode = func_ov036_021b88cc(6);
    return event;
}

GameEvent *func_ov036_021b8b48(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b9294, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 6;
    data->fadeMode = func_ov036_021b88cc(6);
    return event;
}

GameEvent *func_ov036_021b8b78(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b9128, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 7;
    data->fadeMode = func_ov036_021b88cc(7);
    return event;
}

GameEvent *func_ov036_021b8ba8(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b9314, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 7;
    data->fadeMode = func_ov036_021b88cc(7);
    return event;
}

GameEvent *func_ov036_021b8bd8(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b91a0, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 8;
    data->fadeMode = func_ov036_021b88cc(8);
    return event;
}

GameEvent *func_ov036_021b8c08(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021b9394, sizeof(TransitionEvent));
    TransitionEvent *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->type = 8;
    data->fadeMode = func_ov036_021b88cc(8);
    return event;
}

GameEvent *func_ov036_021b8c38(GameSystem *gsys, Field *field) {
    switch (FieldPlayer_GetFaceDir(Field_GetPlayer(field))) {
    case 2:
    default:
        return func_ov036_021b8b78(gsys, field);
    case 3:
        return func_ov036_021b8bd8(gsys, field);
    case 0:
        return func_ov036_021b8ab8(gsys, field);
    case 1:
        return func_ov036_021b8b18(gsys, field);
    }
}

GameEvent *func_ov036_021b8c88(GameSystem *gsys, Field *field) {
    switch (FieldPlayer_GetFaceDir(Field_GetPlayer(field))) {
    case 2:
    default:
        return func_ov036_021b8ba8(gsys, field);
    case 3:
        return func_ov036_021b8c08(gsys, field);
    case 0:
        return func_ov036_021b8ae8(gsys, field);
    case 1:
        return func_ov036_021b8b48(gsys, field);
    }
}

GameEventReturnCode func_ov036_021b8cd8(GameEvent *event, u32 *state, void *data) {
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

GameEventReturnCode func_ov036_021b8d24(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    switch (*state) {
    case 0:
        if (work->keepLCD != 0) {
            FieldG3D_SetLCDConfigKeepBGs(work->field);
            func_ov036_021b948c(work->field);
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
        config = data_ov036_021d1624;
        GFL_BGSysSetLCDConfig(&config);
        reg_G2_BG2CNT = (reg_G2_BG2CNT & 0x43) | 0x4084;
        GX_SetBankForBG(8);
        GFL_BGSysSetBGPriority(2, 0);
        GFL_BGSysSetBGEnabled(2, TRUE);
        setBuildingEnterVBlankCallback();
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode func_ov036_021b8e10(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    switch (*state) {
    case 0:
        Field_SetEffectRunningFlag(work->field, TRUE);
        work->alpha = 16;
        GFL_BGSysSetBGPriority(2, 0);
        GFL_BGSysSetBGPriority(0, 1);
        GFL_BGSysSetBGEnabled(0, TRUE);
        G2x_SetBlendAlpha_(0x4000050, 4, 1, work->alpha, 0);
        GFL_FadeSet(work->fadeMode, 16, 0, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        if (work->alpha != 0) {
            u32 alpha = --work->alpha;
            reg_G2_BLDALPHA = alpha | ((16 - alpha) << 8);
            break;
        }
        GFL_BGSysSetBGEnabled(2, FALSE);
        GX_SetBankForLCDC(8);
        MIi_CpuClearFast(0, (void *)0x6800000, 0x20000);
        GX_DisableBankForLCDC();
        GX_SetBankForBG(8);
        resetBuildingEnterVBlankCallback();
        (*state)++;
        break;
    case 2:
        FieldG3D_SetLCDConfigKeepBGs(work->field);
        (*state)++;
        break;
    case 3:
        func_ov036_02180630(work->field);
        (*state)++;
        break;
    case 4: {
        void *msgBGSys = Field_GetMsgBGSys(work->field);
        if (!func_ov036_021878bc(msgBGSys)) {
            break;
        }
        func_ov036_0218779c(msgBGSys, 0);
        (*state)++;
        break;
    }
    case 5:
        func_ov036_0218779c(Field_GetMsgBGSys(work->field), 1);
        (*state)++;
        break;
    case 6:
        func_ov036_0218779c(Field_GetMsgBGSys(work->field), 2);
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

GameEventReturnCode func_ov036_021b8f74(GameEvent *event, u32 *state, void *data) {
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

GameEventReturnCode func_ov036_021b8fc0(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 0, 16, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(3, 8, 8, 0, 6, 1, Field_GetHeapID(field));
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

GameEventReturnCode func_ov036_021b9038(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 0, 16, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(3, 2, 2, 0, 6, 1, Field_GetHeapID(field));
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

GameEventReturnCode func_ov036_021b90b0(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 0, 16, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(3, 4, 4, 0, 6, 1, Field_GetHeapID(field));
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

GameEventReturnCode func_ov036_021b9128(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 0, 16, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(3, 10, 10, 0, 6, 1, Field_GetHeapID(field));
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

GameEventReturnCode func_ov036_021b91a0(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 0, 16, -1);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(3, 6, 6, 0, 6, 1, Field_GetHeapID(field));
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

GameEventReturnCode func_ov036_021b9214(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        FieldG3D_SetLCDConfigKeepBGs(field);
        func_ov036_021b948c(work->field);
        GFL_FadeSet(work->fadeMode, 16, 0, -1);
        FieldLensFlare_GreenlightStart(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(3, 3, 3, 0, 6, 1, Field_GetHeapID(field));
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

GameEventReturnCode func_ov036_021b9294(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        FieldG3D_SetLCDConfigKeepBGs(field);
        func_ov036_021b948c(work->field);
        GFL_FadeSet(work->fadeMode, 16, 0, -1);
        FieldLensFlare_GreenlightStart(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(3, 5, 5, 0, 6, 1, Field_GetHeapID(field));
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

GameEventReturnCode func_ov036_021b9314(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        FieldG3D_SetLCDConfigKeepBGs(field);
        func_ov036_021b948c(work->field);
        GFL_FadeSet(work->fadeMode, 16, 0, -1);
        FieldLensFlare_GreenlightStart(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(3, 7, 7, 0, 6, 1, Field_GetHeapID(field));
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

GameEventReturnCode func_ov036_021b9394(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    Field *field = work->field;
    switch (*state) {
    case 0:
        FieldG3D_SetLCDConfigKeepBGs(field);
        func_ov036_021b948c(work->field);
        GFL_FadeSet(work->fadeMode, 16, 0, -1);
        FieldLensFlare_GreenlightStart(Field_GetLensFlare(work->field));
        (*state)++;
        break;
    case 1:
        GFL_WipeSet(3, 11, 11, 0, 6, 1, Field_GetHeapID(field));
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

GameEventReturnCode func_ov036_021b9414(GameEvent *event, u32 *state, void *data) {
    TransitionEvent *work = data;
    switch (*state) {
    case 0:
        GFL_FadeSet(work->fadeMode, 16, 0, work->fadeSlowness);
        FieldLensFlare_Cancel(Field_GetLensFlare(work->field));
        FieldG3D_SetLCDConfigKeepBGs(work->field);
        func_ov036_021b948c(work->field);
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

void func_ov036_021b948c(Field *field) {
    FieldG2D_Prepare3DSurface(field);
}
