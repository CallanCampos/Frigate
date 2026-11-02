#include "types.h"
#include "constants/sound.h"
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
#include "system/zone_weather.h"

static const SweetScentPalette sPalette = { 0, 0x5d5f };

GameEvent *EventSweetScent_Create(Field *field, GameSystem *gsys) {
    return EventSweetScent_CreateFromPartySlot(gsys, field, 0xff);
}

GameEvent *EventSweetScent_CreateFromPartySlot(GameSystem *gsys, Field *field, u8 partySlot) {
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

// The steps of Sweet Scent: the player's form change and the Pokémon's sprite when a Pokémon uses it, then the fade
// in clear weather and a wild battle, or the scripts of no encounter and of the wrong weather
enum {
    SWEET_SCENT_STEP_START,
    SWEET_SCENT_STEP_WAIT_FORM_CHANGE,
    SWEET_SCENT_STEP_WAIT_ANIME,
    SWEET_SCENT_STEP_RETURN_FORM,
    SWEET_SCENT_STEP_WAIT_RETURN_FORM,
    SWEET_SCENT_STEP_CHECK_WEATHER,
    SWEET_SCENT_STEP_ENCOUNTER,
    SWEET_SCENT_STEP_NO_ENCOUNTER,
    SWEET_SCENT_STEP_BAD_WEATHER,
    SWEET_SCENT_STEP_END,
};

GameEventReturnCode EventSweetScent_Callback(GameEvent *event, u32 *state, void *data) {
    SweetScentEventData *work;
    GameEvent *next;

    work = data;
    switch (*state) {
    case SWEET_SCENT_STEP_START:
        if (work->partySlot >= 6) {
            *state = SWEET_SCENT_STEP_CHECK_WEATHER;
            break;
        }
        FieldPlayer_SetSpecialSeq(work->player, 0x80);
        (*state)++;
    case SWEET_SCENT_STEP_WAIT_FORM_CHANGE:
        if (FieldPlayer_StepFormChange(work->player) != TRUE) {
            break;
        }
        FldAct_SetAcmd(FieldPlayer_GetActor(work->player), 0xa2);
        (*state)++;
        break;
    case SWEET_SCENT_STEP_WAIT_ANIME:
        if (FldAct_EndAcmd(FieldPlayer_GetActor(work->player)) != TRUE) {
            break;
        }
        next = EventFieldEffect_CreatePokeSprite(work->gsys, Field_Get3DCi(work->field), work->partySlot);
        if (next != NULL) {
            GameEvent_ChainNext(event, next);
        }
        (*state)++;
        break;
    case SWEET_SCENT_STEP_RETURN_FORM:
        FieldPlayer_SetSpecialSeq(work->player, 8);
        (*state)++;
    case SWEET_SCENT_STEP_WAIT_RETURN_FORM:
        if (FieldPlayer_StepFormChange(work->player) == TRUE) {
            *state = SWEET_SCENT_STEP_CHECK_WEATHER;
        }
        break;
    case SWEET_SCENT_STEP_CHECK_WEATHER:
        if (EventSweetScent_CheckWeatherClear(work) == 0) {
            *state = SWEET_SCENT_STEP_BAD_WEATHER;
            break;
        }
        EventSweetScent_StartFade(event, work->gsys, work->field);
        *state = SWEET_SCENT_STEP_ENCOUNTER;
        break;
    case SWEET_SCENT_STEP_ENCOUNTER:
        next = EventWildBattleCall_CreateRandom(Field_GetEncountSystem(work->field), ENCOUNT_MODE_SWEET_SCENT);
        if (next == NULL) {
            *state = SWEET_SCENT_STEP_NO_ENCOUNTER;
            break;
        }
        GameEvent_ChainNext(event, next);
        *state = SWEET_SCENT_STEP_END;
        break;
    case SWEET_SCENT_STEP_NO_ENCOUNTER:
        EventScriptCall_Start(event, 0x2793, NULL, 0, 0x15);
        *state = SWEET_SCENT_STEP_END;
        break;
    case SWEET_SCENT_STEP_BAD_WEATHER:
        EventScriptCall_Start(event, 0x2794, NULL, 0, 0x15);
        *state = SWEET_SCENT_STEP_END;
        break;
    case SWEET_SCENT_STEP_END:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

BOOL EventSweetScent_CheckWeatherClear(SweetScentEventData *work) {
    if (FieldWeather_GetWeatherId(Field_GetWeatherSystem(work->field)) == WEATHER_SUNNY) {
        return TRUE;
    }
    return FALSE;
}

void EventSweetScent_StartFade(GameEvent *event, GameSystem *gsys, Field *field) {
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

// The steps of the fade: fill a BG with the scent's color, blend it in to 8, hold, blend it out, and clear it
enum {
    SWEET_SCENT_FADE_STEP_SETUP,
    SWEET_SCENT_FADE_STEP_FADE_IN,
    SWEET_SCENT_FADE_STEP_WAIT,
    SWEET_SCENT_FADE_STEP_FADE_OUT,
    SWEET_SCENT_FADE_STEP_CLEAR,
};

GameEventReturnCode EventSweetScent_FadeMain(GameEvent *event, u32 *state, void *data) {
    SweetScentScreenWork *work;

    work = data;
    switch (*state) {
    case SWEET_SCENT_FADE_STEP_SETUP:
        EventSweetScent_FadeSetup(work);
        GFL_SndSEPlay(SEQ_SE_FLD_18);
        (*state)++;
        break;
    case SWEET_SCENT_FADE_STEP_FADE_IN:
        if (work->timer++ >= work->step) {
            work->alpha++;
            FieldDispControl_ReqAdjustAlphaA(work->displayControl, work->alpha, 16 - work->alpha);
            work->timer = 0;
        }
        if (work->alpha >= 8) {
            (*state)++;
        }
        break;
    case SWEET_SCENT_FADE_STEP_WAIT:
        if (work->timer++ < 30) {
            (*state)++;
        }
        break;
    case SWEET_SCENT_FADE_STEP_FADE_OUT:
        if (work->timer++ >= work->step) {
            work->alpha--;
            FieldDispControl_ReqAdjustAlphaA(work->displayControl, work->alpha, 16 - work->alpha);
            work->timer = 0;
        }
        if (work->alpha == 0) {
            (*state)++;
        }
        break;
    case SWEET_SCENT_FADE_STEP_CLEAR:
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
