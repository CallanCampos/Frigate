#include "types.h"
#include "constants/arc.h"
#include "field/event_season_banner.h"
#include "field/field.h"
#include "field/field_lens_flare.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "struct_decls.h"
#include "system/game_event.h"
#include "system/season.h"

typedef enum {
    SEASON_BANNER_TYPE_FIELD,
    SEASON_BANNER_TYPE_STANDALONE,
} EventSeasonBannerType;

typedef enum {
    SEASON_BANNER_STATE_INIT,
    SEASON_BANNER_STATE_LOAD,
    SEASON_BANNER_STATE_FADE_IN,
    SEASON_BANNER_STATE_STAY,
    SEASON_BANNER_STATE_FADE_OUT,
    SEASON_BANNER_STATE_END,
} EventSeasonBannerState;

struct EventSeasonBanner {
    GameSystem *gsys;
    Field *field;
    HeapID heapId;
    u32 type;
    BOOL cancelled;
    u8 endSeason;
    u8 season;
    u32 timer;
    u32 duration;
    EventSeasonBannerCallback callback;
    void *callbackArg;
};

#define SEASON_BANNER_BG 3

GameEventReturnCode EventSeasonBanner_Callback(GameEvent *event, u32 *state, void *data);
void EventSeasonBanner_InitRenderer(EventSeasonBanner *wk);
void EventSeasonBanner_InitRendererFieldOpen(EventSeasonBanner *wk);
void EventSeasonBanner_InitRendererStandalone(EventSeasonBanner *wk);
void EventSeasonBanner_FreeRenderer(EventSeasonBanner *wk);
void EventSeasonBanner_FreeRendererFieldOpen(EventSeasonBanner *wk);
void EventSeasonBanner_FreeRendererStandalone(EventSeasonBanner *wk);
void EventSeasonBanner_LoadGraphics(u8 season, HeapID heapId);
void EventSeasonBanner_UpdateRenderFX(EventSeasonBanner *wk, u32 state);
void EventSeasonBanner_UpdateRenderFXFieldOpen(const EventSeasonBanner *wk, u32 state);
void EventSeasonBanner_UpdateRenderFXStandalone(EventSeasonBanner *wk, u32 state);
u32 EventSeasonBanner_GetUpdatedState(EventSeasonBanner *wk, u32 state);
void EventSeasonBanner_ProcessState(EventSeasonBanner *wk, u32 *state, u32 newState);
void EventSeasonBanner_Update(EventSeasonBanner *wk, u32 *state);
void EventSeasonBanner_SetFieldBannerFlag(EventSeasonBanner *wk);
void EventSeasonBanner_End(EventSeasonBanner *wk);
u32 EventSeasonBanner_GetType(EventSeasonBanner *wk);
BOOL EventSeasonBanner_CheckUserSkipped(EventSeasonBanner *wk);
void EventSeasonBanner_SetCancelled(EventSeasonBanner *wk);
BOOL EventSeasonBanner_CheckTimerEnd(EventSeasonBanner *wk);
u32 EventSeasonBanner_GetFadeInTime(EventSeasonBanner *wk);
u32 EventSeasonBanner_GetFadeOutTime(EventSeasonBanner *wk);
u32 EventSeasonBanner_GetStayTime(EventSeasonBanner *wk);
void EventSeasonBanner_InvokeCallback(EventSeasonBanner *wk);

// Declared in reverse, as the compiler emits them in reverse order
static const BGSysVRAMConfig SEASON_BANNER_VRAM_CONFIG = {
    GX_VRAM_BG_128_D,    GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_32_H,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,    GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_012_ABC, GX_VRAM_TEXPLTT_0_G,     GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
};
static const BGSetup SEASON_BANNER_BG3_SETUP = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x0800),
    GX_BG_CHARBASE(0x04000),
    0x8000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSysLCDConfig SEASON_BANNER_LCD_CONFIG = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

GameEventReturnCode EventSeasonBanner_Callback(GameEvent *event, u32 *state, void *data) {
    EventSeasonBanner *wk = data;

    switch (*state) {
    case SEASON_BANNER_STATE_INIT:
        EventSeasonBanner_SetFieldBannerFlag(wk);
        break;
    case SEASON_BANNER_STATE_FADE_IN:
        EventSeasonBanner_UpdateRenderFX(wk, *state);
        break;
    case SEASON_BANNER_STATE_FADE_OUT:
        EventSeasonBanner_UpdateRenderFX(wk, *state);
        break;
    case SEASON_BANNER_STATE_END:
        EventSeasonBanner_End(wk);
        return GAMEEVENT_DONE;
    }

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        if (EventSeasonBanner_GetType(wk) == SEASON_BANNER_TYPE_FIELD) {
            EventSeasonBanner_SetCancelled(wk);
        }
    }
    wk->timer++;
    EventSeasonBanner_Update(wk, state);
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventSeasonBanner_CreateFieldTransition(GameSystem *gsys, Field *field, u8 startSeason, u8 endSeason) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventSeasonBanner_Callback, sizeof(EventSeasonBanner));
    EventSeasonBanner *wk = GameEvent_GetData(event);
    u32 *state = GameEvent_GetStatePtr(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->heapId = Field_GetHeapID(field);
    wk->type = SEASON_BANNER_TYPE_FIELD;
    wk->cancelled = FALSE;
    wk->endSeason = endSeason;
    wk->season = Season_GetPrevious(startSeason);
    wk->callback = NULL;
    wk->callbackArg = NULL;
    FieldLensFlare_Cancel(Field_GetLensFlare(wk->field));
    EventSeasonBanner_ProcessState(wk, state, SEASON_BANNER_STATE_INIT);
    return event;
}

GameEvent *EventSeasonBanner_CreateFieldTransitionEx(GameSystem *gsys, Field *field, u8 startSeason, u8 endSeason,
                                                     EventSeasonBannerCallback callback, void *callbackArg) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventSeasonBanner_Callback, sizeof(EventSeasonBanner));
    EventSeasonBanner *wk = GameEvent_GetData(event);
    u32 *state = GameEvent_GetStatePtr(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->heapId = Field_GetHeapID(field);
    wk->type = SEASON_BANNER_TYPE_FIELD;
    wk->cancelled = FALSE;
    wk->endSeason = endSeason;
    wk->season = Season_GetPrevious(startSeason);
    wk->callback = callback;
    wk->callbackArg = callbackArg;
    EventSeasonBanner_ProcessState(wk, state, SEASON_BANNER_STATE_INIT);
    return event;
}

GameEvent *EventSeasonBanner_CreateStandalone(GameSystem *gsys, u8 startSeason, u8 endSeason) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventSeasonBanner_Callback, sizeof(EventSeasonBanner));
    EventSeasonBanner *wk = GameEvent_GetData(event);
    u32 *state = GameEvent_GetStatePtr(event);

    wk->gsys = gsys;
    wk->field = NULL;
    wk->heapId = 4;
    wk->type = SEASON_BANNER_TYPE_STANDALONE;
    wk->cancelled = FALSE;
    wk->endSeason = endSeason;
    wk->season = Season_GetPrevious(startSeason);
    wk->callback = NULL;
    wk->callbackArg = NULL;
    EventSeasonBanner_ProcessState(wk, state, SEASON_BANNER_STATE_INIT);
    return event;
}

void EventSeasonBanner_InitRenderer(EventSeasonBanner *wk) {
    switch (EventSeasonBanner_GetType(wk)) {
    case SEASON_BANNER_TYPE_FIELD:
        EventSeasonBanner_InitRendererFieldOpen(wk);
        break;
    case SEASON_BANNER_TYPE_STANDALONE:
        EventSeasonBanner_InitRendererStandalone(wk);
        break;
    }
}

void EventSeasonBanner_InitRendererFieldOpen(EventSeasonBanner *wk) {
    u32 enabled = GFL_BGSysGetEnabledBGsA();

    FieldG2D_SetLCDConfig();
    GFL_BGSysSetEnabledBGsA(enabled);
    GFL_BGSysSetBGPriority(0, 1);
    GFL_BGSysSetBGPriority(SEASON_BANNER_BG, 0);
    GFL_BGSysSetBGPriority(1, 3);
    GFL_BGSysSetBGPriority(2, 3);
    GFL_BGSysClearBG(SEASON_BANNER_BG);
}

void EventSeasonBanner_InitRendererStandalone(EventSeasonBanner *wk) {
    GFL_BGSysSetVRAMBanks(&SEASON_BANNER_VRAM_CONFIG);
    GFL_BGSysCreate(wk->heapId);
    GFL_BGSysSetLCDConfig(&SEASON_BANNER_LCD_CONFIG);
    GFL_BGSysCreateBG(SEASON_BANNER_BG, &SEASON_BANNER_BG3_SETUP, BGMODE_TEXT);
    GFL_BGSysSetBGPriority(0, 1);
    GFL_BGSysSetBGPriority(SEASON_BANNER_BG, 0);
    GFL_BGSysSetBGPriority(1, 3);
    GFL_BGSysSetBGPriority(2, 3);
    GFL_BGSysClearBG(SEASON_BANNER_BG);
}

void EventSeasonBanner_FreeRenderer(EventSeasonBanner *wk) {
    switch (EventSeasonBanner_GetType(wk)) {
    case SEASON_BANNER_TYPE_FIELD:
        EventSeasonBanner_FreeRendererFieldOpen(wk);
        break;
    case SEASON_BANNER_TYPE_STANDALONE:
        EventSeasonBanner_FreeRendererStandalone(wk);
        break;
    }
}

void EventSeasonBanner_FreeRendererFieldOpen(EventSeasonBanner *wk) {
}

void EventSeasonBanner_FreeRendererStandalone(EventSeasonBanner *wk) {
    GFL_BGSysReleaseBG(SEASON_BANNER_BG);
    GFL_BGSysFree();
}

void EventSeasonBanner_LoadGraphics(u8 season, HeapID heapId) {
    ArcTool *handle;
    NNSG2dPaletteData *palette;
    NNSG2dCharacterData *character;
    NNSG2dScreenData *screen;
    u32 paletteId, characterId, screenId;
    void *file;

    switch (season) {
    case 0:
    default:
        paletteId = 0;
        characterId = 1;
        screenId = 2;
        break;
    case 1:
        paletteId = 3;
        characterId = 4;
        screenId = 5;
        break;
    case 2:
        paletteId = 6;
        characterId = 7;
        screenId = 8;
        break;
    case 3:
        paletteId = 9;
        characterId = 10;
        screenId = 11;
        break;
    }

    handle = GFL_ArcSysCreateFileHandle(ARCID_SEASON_BANNER, heapId);

    file = GFL_ArcToolReadHeapNew(handle, paletteId, heapId);
    NNS_G2dGetUnpackedPaletteData(file, &palette);
    GFL_BGSysUploadStdPalette(SEASON_BANNER_BG, palette->rawData, 0x20, 0);
    GFL_HeapFree(file);

    file = GFL_ArcToolReadHeapNew(handle, characterId, heapId);
    NNS_G2dGetUnpackedBGCharacterData(file, &character);
    GFL_BGSysLoadChar(SEASON_BANNER_BG, character->rawData, character->size, 0);
    GFL_HeapFree(file);

    file = GFL_ArcToolReadHeapNew(handle, screenId, heapId);
    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrCore(SEASON_BANNER_BG, screen->rawData, screen->size, 0);
    GFL_HeapFree(file);

    GFL_ArcToolFree(handle);
}

void EventSeasonBanner_UpdateRenderFX(EventSeasonBanner *wk, u32 state) {
    switch (EventSeasonBanner_GetType(wk)) {
    case SEASON_BANNER_TYPE_FIELD:
        EventSeasonBanner_UpdateRenderFXFieldOpen(wk, state);
        break;
    case SEASON_BANNER_TYPE_STANDALONE:
        EventSeasonBanner_UpdateRenderFXStandalone(wk, state);
        break;
    }
}

void EventSeasonBanner_UpdateRenderFXFieldOpen(const EventSeasonBanner *wk, u32 state) {
    if (state == SEASON_BANNER_STATE_FADE_IN) {
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, 8, (s32)((f32)wk->timer / (f32)wk->duration * 16.0f + -16.0f));
    } else if (state == SEASON_BANNER_STATE_FADE_OUT) {
        if (wk->season == wk->endSeason) {
            s32 alpha = (s32)((f32)wk->timer / (f32)wk->duration * 16.0f);
            G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 8, 0x11, 16 - alpha, alpha);
            GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR,
                                     (s32)((f32)wk->timer / (f32)wk->duration * 16.0f + -16.0f));
        } else {
            G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, 8, (s32)(0.0f - (f32)wk->timer / (f32)wk->duration * 16.0f));
        }
    }
}

void EventSeasonBanner_UpdateRenderFXStandalone(EventSeasonBanner *wk, u32 state) {
    if (state == SEASON_BANNER_STATE_FADE_IN) {
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, 8, (s32)((f32)wk->timer / (f32)wk->duration * 16.0f + -16.0f));
    } else if (state == SEASON_BANNER_STATE_FADE_OUT) {
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, 8, (s32)(0.0f - (f32)wk->timer / (f32)wk->duration * 16.0f));
    }
}

u32 EventSeasonBanner_GetUpdatedState(EventSeasonBanner *wk, u32 state) {
    switch (state) {
    case SEASON_BANNER_STATE_INIT:
        state = SEASON_BANNER_STATE_LOAD;
        break;
    case SEASON_BANNER_STATE_LOAD:
        state = SEASON_BANNER_STATE_FADE_IN;
        break;
    case SEASON_BANNER_STATE_FADE_IN:
        if (EventSeasonBanner_CheckTimerEnd(wk) == TRUE) {
            state = SEASON_BANNER_STATE_STAY;
        }
        break;
    case SEASON_BANNER_STATE_STAY:
        if (EventSeasonBanner_CheckTimerEnd(wk) == TRUE || EventSeasonBanner_CheckUserSkipped(wk) == TRUE) {
            state = SEASON_BANNER_STATE_FADE_OUT;
        }
        break;
    case SEASON_BANNER_STATE_FADE_OUT:
        if (EventSeasonBanner_CheckTimerEnd(wk) == TRUE) {
            if (wk->endSeason == wk->season) {
                state = SEASON_BANNER_STATE_END;
            } else {
                state = SEASON_BANNER_STATE_LOAD;
            }
        }
        break;
    case SEASON_BANNER_STATE_END:
        break;
    }
    return state;
}

void EventSeasonBanner_ProcessState(EventSeasonBanner *wk, u32 *state, u32 newState) {
    switch (newState) {
    case SEASON_BANNER_STATE_INIT:
        EventSeasonBanner_InitRenderer(wk);
        break;
    case SEASON_BANNER_STATE_LOAD:
        wk->season = Season_GetNext(wk->season);
        EventSeasonBanner_LoadGraphics(wk->season, wk->heapId);
        break;
    case SEASON_BANNER_STATE_FADE_IN:
        GFL_BGSysSetBGEnabled(SEASON_BANNER_BG, TRUE);
        GFL_BGSysSetBGEnabled(0, FALSE);
        GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, 0);
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, 8, -16);
        wk->duration = EventSeasonBanner_GetFadeInTime(wk);
        break;
    case SEASON_BANNER_STATE_STAY:
        wk->duration = EventSeasonBanner_GetStayTime(wk);
        break;
    case SEASON_BANNER_STATE_FADE_OUT:
        wk->duration = EventSeasonBanner_GetFadeOutTime(wk);
        if (EventSeasonBanner_GetType(wk) == SEASON_BANNER_TYPE_FIELD && wk->season == wk->endSeason) {
            FieldG2D_Prepare3DSurface(wk->field);
            GFL_BGSysSetBGEnabled(SEASON_BANNER_BG, TRUE);
            EventSeasonBanner_InvokeCallback(wk);
        }
        break;
    case SEASON_BANNER_STATE_END:
        break;
    }
    *state = newState;
    wk->timer = 0;
}

void EventSeasonBanner_Update(EventSeasonBanner *wk, u32 *state) {
    u32 newState = EventSeasonBanner_GetUpdatedState(wk, *state);

    if (*state != newState) {
        EventSeasonBanner_ProcessState(wk, state, newState);
    }
}

void EventSeasonBanner_SetFieldBannerFlag(EventSeasonBanner *wk) {
    if (wk->field != NULL) {
        Field_SetSeasonBannerOverdrawFlag(wk->field, TRUE);
    }
}

void EventSeasonBanner_End(EventSeasonBanner *wk) {
    GFL_BGSysSetBGEnabled(SEASON_BANNER_BG, FALSE);
    if (wk->field != NULL) {
        Field_SetSeasonBannerOverdrawFlag(wk->field, FALSE);
    }
    EventSeasonBanner_FreeRenderer(wk);
}

u32 EventSeasonBanner_GetType(EventSeasonBanner *wk) {
    return wk->type;
}

BOOL EventSeasonBanner_CheckUserSkipped(EventSeasonBanner *wk) {
    return wk->cancelled;
}

void EventSeasonBanner_SetCancelled(EventSeasonBanner *wk) {
    wk->cancelled = TRUE;
}

BOOL EventSeasonBanner_CheckTimerEnd(EventSeasonBanner *wk) {
    if (wk->duration < wk->timer) {
        return TRUE;
    }
    return FALSE;
}

u32 EventSeasonBanner_GetFadeInTime(EventSeasonBanner *wk) {
    if (EventSeasonBanner_GetType(wk) == SEASON_BANNER_TYPE_STANDALONE) {
        return 30;
    }
    return 10;
}

u32 EventSeasonBanner_GetFadeOutTime(EventSeasonBanner *wk) {
    if (EventSeasonBanner_GetType(wk) == SEASON_BANNER_TYPE_STANDALONE) {
        return 60;
    }
    if (wk->season == wk->endSeason) {
        return 20;
    }
    return 10;
}

u32 EventSeasonBanner_GetStayTime(EventSeasonBanner *wk) {
    if (EventSeasonBanner_GetType(wk) == SEASON_BANNER_TYPE_STANDALONE) {
        return 120;
    }
    if (wk->season == wk->endSeason) {
        return 60;
    }
    return 30;
}

void EventSeasonBanner_InvokeCallback(EventSeasonBanner *wk) {
    if (wk->callback != NULL) {
        wk->callback(wk->callbackArg);
    }
}
