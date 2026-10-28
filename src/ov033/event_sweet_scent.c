#include "types.h"
#include "field/encounter_effect.h"
#include "field/event_sweet_scent.h"
#include "field/event_wild_battle.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_disp_control.h"
#include "field/field_effect.h"
#include "field/field_environment.h"
#include "field/field_player.h"
#include "field/field_script.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "struct_decls.h"
#include "system/game_event.h"
#include "system/game_system.h"

static const SweetScentPalette sPalette = { 0, 0x5d5f };

GameEvent *EventSweetScent_Create(Field *field, GameSystem *gsys) {
    return func_ov033_021785d4(gsys, field, 0xff);
}

GameEvent *func_ov033_021785d4(GameSystem *gsys, Field *field, u8 partySlot) {
    GameEvent *event;
    SweetScentEventData *data;

    event = GameEvent_Create(gsys, NULL, EventSweetScent_Callback, sizeof(SweetScentEventData));
    data = GameEvent_GetData(event);
    MI_CpuFill8(data, 0, sizeof(SweetScentEventData));
    data->partySlot = partySlot;
    data->gsys = gsys;
    data->field = field;
    data->player = Field_GetPlayer(field);
    data->gameData = GSYS_GetGameData(gsys);
    return event;
}

GameEventReturnCode EventSweetScent_Callback(GameEvent *event, u32 *state, void *data) {
    SweetScentEventData *work;
    GameEvent *next;

    work = data;
    switch (*state) {
    case 0:
        if (work->partySlot >= 6) {
            *state = 5;
            break;
        }
        FieldPlayer_SetSpecialSeq(work->player, 0x80);
        (*state)++;
    case 1:
        if (FieldPlayer_StepFormChange(work->player) != TRUE) {
            break;
        }
        FldAct_SetAcmd(FieldPlayer_GetActor(work->player), 0xa2);
        (*state)++;
        break;
    case 2:
        if (FldAct_EndAcmd(FieldPlayer_GetActor(work->player)) != TRUE) {
            break;
        }
        next = EventFieldEffect_CreatePokeSprite(work->gsys, Field_Get3DCi(work->field), work->partySlot);
        if (next != NULL) {
            GameEvent_ChainNext(event, next);
        }
        (*state)++;
        break;
    case 3:
        FieldPlayer_SetSpecialSeq(work->player, 8);
        (*state)++;
    case 4:
        if (FieldPlayer_StepFormChange(work->player) == TRUE) {
            *state = 5;
        }
        break;
    case 5:
        if (EventSweetScent_CheckWeatherClear(work) == 0) {
            *state = 8;
            break;
        }
        func_ov033_02178748(event, work->gsys, work->field);
        *state = 6;
        break;
    case 6:
        next = EventWildBattleCall_CreateRandom(Field_GetEncountSystem(work->field), ENCOUNT_MODE_SWEET_SCENT);
        if (next == NULL) {
            *state = 7;
            break;
        }
        GameEvent_ChainNext(event, next);
        *state = 9;
        break;
    case 7:
        EventScriptCall_Start(event, 0x2793, NULL, 0, 0x15);
        *state = 9;
        break;
    case 8:
        EventScriptCall_Start(event, 0x2794, NULL, 0, 0x15);
        *state = 9;
        break;
    case 9:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

BOOL EventSweetScent_CheckWeatherClear(SweetScentEventData *work) {
    if (FieldWeather_GetWeatherId(Field_GetWeatherSystem(work->field)) == 0) {
        return TRUE;
    }
    return FALSE;
}

void func_ov033_02178748(GameEvent *event, GameSystem *gsys, Field *field) {
    GameEvent *next;
    SweetScentScreenWork *data;

    next = GameEvent_Create(gsys, NULL, EventSweetScent_FadeMain, sizeof(SweetScentScreenWork));
    data = GameEvent_GetData(next);
    data->bgId = 3;
    data->alpha = 0;
    data->step = 2;
    data->timer = 0;
    data->displayControl = FieldG2D_GetDispControl(field);
    GameEvent_ChainNext(event, next);
}

GameEventReturnCode EventSweetScent_FadeMain(GameEvent *event, u32 *state, void *data) {
    SweetScentScreenWork *work;

    work = data;
    switch (*state) {
    case 0:
        EventSweetScent_FadeSetup(work);
        GFL_SndSEPlay(0x683);
        (*state)++;
        break;
    case 1:
        if (work->timer++ >= work->step) {
            work->alpha++;
            FieldDispControl_ReqAdjustAlphaA(work->displayControl, work->alpha, 16 - work->alpha);
            work->timer = 0;
        }
        if (work->alpha >= 8) {
            (*state)++;
        }
        break;
    case 2:
        if (work->timer++ < 30) {
            (*state)++;
        }
        break;
    case 3:
        if (work->timer++ >= work->step) {
            work->alpha--;
            FieldDispControl_ReqAdjustAlphaA(work->displayControl, work->alpha, 16 - work->alpha);
            work->timer = 0;
        }
        if (work->alpha == 0) {
            (*state)++;
        }
        break;
    case 4:
        FieldDispControl_ReqSetAlphaA(work->displayControl, 0, 0, 0, 0x1f);
        FieldDispControl_ReqSetBGEnabled(work->displayControl, work->bgId, FALSE);
        (*state)++;
        break;
    default:
        EventSweetScent_FadeCleanup(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void EventSweetScent_FadeSetup(SweetScentScreenWork *work) {
    SweetScentPalette palette = sPalette;
    GFL_BGSysSetBGEnabled(work->bgId, FALSE);
    GFL_BGSysUploadStdPalette(work->bgId, &palette, 8, 0x20);
    GFL_BGSysFillChar(work->bgId, 1, 1, 0x100);
    GFL_BGSysFillScrArea(work->bgId, 0x100, 0, 0, 0x20, 0x20, 1);
    GFL_BGSysLoadScr(work->bgId);
    FieldDispControl_ReqSetAlphaA(work->displayControl, 8, 0x37, work->alpha, 16 - work->alpha);
    FieldDispControl_ReqSetBGEnabled(work->displayControl, work->bgId, TRUE);
}

void EventSweetScent_FadeCleanup(SweetScentScreenWork *work) {
    u16 palette[2] = { 0, 0 };

    GFL_BGSysUploadStdPalette(work->bgId, palette, 8, 0x20);
    GFL_BGSysFillScrArea(work->bgId, 0, 0, 0, 0x20, 0x20, 1);
    GFL_BGSysLoadScr(work->bgId);
    GFL_BGSysFreeFilledChar(work->bgId, 1, 0x100);
}
