#include "types.h"
#include "app/zukan_detail.h"
#include "constants/arc.h"
#include "constants/species.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "pml/poke_graphic.h"
#include "pml/species_names.h"
#include "save/pokedex.h"
#include "system/game_data.h"
#include "system/printsys.h"

// The detail screen's cry page, zukan_detail_voice.c by the ROM's own string: the Pokémon on the top screen, which
// stretches while its cry plays, with the cry's length and its wave scrolling by on the touch screen

// The Pokédex's shortcut for the Y button, which the touch bar's check box registers
#define SHORTCUT_POKEDEX_VOICE 22

enum {
    VOICE_SEQ_INIT,
    VOICE_SEQ_FADE_IN,
    VOICE_SEQ_START_PALFADE,
    VOICE_SEQ_WAIT_FADE_IN,
    VOICE_SEQ_MAIN,
    VOICE_SEQ_WAIT_FADE_OUT,
    VOICE_SEQ_FADE_OUT,
    VOICE_SEQ_EXIT,
};

// How the page ends
enum {
    VOICE_EXIT_NONE,
    VOICE_EXIT_PAGE,
    VOICE_EXIT_SCREEN,
};

// The background music fades out while the page is shown, so that the cries can be heard
enum {
    BGM_PLAYING,
    BGM_FADING_OUT,
    BGM_PAUSED,
    BGM_RESTORED,
};

// The Pokémon stretches up as its cry starts and back down once it ends
enum {
    STRETCH_NONE,
    STRETCH_GROWING,
    STRETCH_GROWN,
    STRETCH_SHRINKING,
};

// The resources of the actors, for each screen
enum {
    RES_MAIN_CHARS,
    RES_MAIN_PALETTE,
    RES_MAIN_CELL_ANIMS,
    RES_SUB_CHARS,
    RES_SUB_PALETTE,
    RES_SUB_CELL_ANIMS,
    RES_COUNT,
};

// The four digits of the time the cry has played, its point and its unit, and an animated icon on the top screen
#define ACTOR_COUNT 7
#define DIGIT_COUNT 4
#define ACTOR_ICON 6

// The longest length shown, 9.99 seconds
#define TIME_MAX 9990

// The cry's wave, drawn a column for every sample's half frame into a bitmap, and scrolled into a window of 31 by 15
// tiles of BG 3 two columns a frame
#define WAVE_BG 3
#define WAVE_WINDOW_WIDTH 31
#define WAVE_WINDOW_HEIGHT 15
#define WAVE_WIDTH (WAVE_WINDOW_WIDTH * 8)
// How much of the window shows, from its left edge, where the new columns are drawn
#define WAVE_SHOWN_WIDTH (WAVE_WIDTH - 10)
#define WAVE_HEIGHT (WAVE_WINDOW_HEIGHT * 8)
#define WAVE_COLUMNS 960
#define WAVE_STEP 2
// The middle line of the wave and the farthest it reaches either way
#define WAVE_CENTER 55
#define WAVE_COLOR 15

typedef struct {
    u8 x;
    u8 y;
    u8 anim;
    u8 priority;
    u8 bgPriority;
    // RES_*
    u8 chars;
    u8 palette;
    u8 cellAnims;
    u8 surface;
} ActorData;

typedef struct {
    ClActUnit *unit;
    Font *font;
    PrintQueue *printQueue;
    BmpWin *nameWindow;
    BOOL nameFlushPending;
    // The wave's window's screen, its scroll within a column and the column at its left edge
    u16 *waveScreen;
    s16 waveScroll;
    s16 waveColumn;
    BOOL waveScreenPending;
    BmpWin *waveWindow;
    u8 waveWait;
    // The Pokémon whose cry the wave shows
    u16 waveSpecies;
    u16 waveForm;
    // The whole wave, its width in columns, and how many columns of it have scrolled into the window
    GFLBitmap *waveBitmap;
    s16 waveWidth;
    s16 waveShown;
    const s8 *samples;
    // The cry's length in milliseconds
    u32 duration;
    // Two Pokémon, the one shown and the one loaded when the list moves
    u32 pokemonPalettes[2];
    u32 pokemonChars[2];
    u32 pokemonCellAnims[2];
    ClActor *pokemonActors[2];
    int pokemon;
    int stretch;
    u32 voice;
    BOOL playing;
    u64 startTime;
    // The time played in milliseconds, and the time that the digits show
    u16 time;
    u16 shownTime;
    u32 bgChars;
    ZukanDetailBackground *backgroundMain;
    ZukanDetailBackground *backgroundSub;
    u32 resources[RES_COUNT];
    ClActor *actors[ACTOR_COUNT];
    int bgm;
    TCB *vblankTcb;
    ZukanDetailBlend *blendMain;
    ZukanDetailBlend *blendSub;
    ZukanDetailPalFade *palFade;
    int exit;
    BOOL inputEnabled;
} ZukanDetailVoiceWork;

// Not referenced: eight bytes and eighteen floats, which may belong to an unused 3D setup
typedef struct {
    u8 unk0[4];
    u16 unk4;
    u16 unk6;
    f32 unk8[6][3];
} ZukanDetailVoiceUnkData;

static BOOL ZukanDetailVoice_Init(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                  ZukanDetailCommon *common);
static BOOL ZukanDetailVoice_Main(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                  ZukanDetailCommon *common);
static BOOL ZukanDetailVoice_Exit(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                  ZukanDetailCommon *common);
static void ZukanDetailVoice_Command(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                     ZukanDetailCommon *common, int command);
static void ZukanDetailVoice_VBlank(TCB *tcb, void *data);
static void ZukanDetailVoice_FadeOutBGM(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailVoice_RestoreBGM(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailVoice_UpdateBGM(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common);
static void ZukanDetailVoice_CreateActors(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                          ZukanDetailCommon *common);
static void ZukanDetailVoice_FreeActors(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailVoice_SetTimeDigits(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                           ZukanDetailCommon *common, u16 seconds, u16 hundredths);
static void ZukanDetailVoice_CreatePokemon(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                           ZukanDetailCommon *common, int index);
static void ZukanDetailVoice_FreePokemon(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                         ZukanDetailCommon *common, int index);
static void ZukanDetailVoice_UpdateStretch(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                           ZukanDetailCommon *common);
static void ZukanDetailVoice_StartStretch(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                          ZukanDetailCommon *common);
static void ZukanDetailVoice_EndStretch(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailVoice_CreateName(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailVoice_FreeName(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                      ZukanDetailCommon *common);
static void ZukanDetailVoice_PrintName(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common);
static void ZukanDetailVoice_ClearName(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common);
static void ZukanDetailVoice_FlushName(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common);
static void ZukanDetailVoice_CreateTime(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailVoice_FreeTime(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                      ZukanDetailCommon *common);
static void ZukanDetailVoice_PrintTime(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common, u16 seconds, u16 hundredths);
static void ZukanDetailVoice_ClearTime(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common);
static void ZukanDetailVoice_UpdateTime(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailVoice_StopVoice(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common);
static void ZukanDetailVoice_ChangePokemon(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                           ZukanDetailCommon *common);
static void ZukanDetailVoice_CheckInput(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailVoice_CreateWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailVoice_FreeWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                      ZukanDetailCommon *common);
static void ZukanDetailVoice_UpdateWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailVoice_ResetWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common);
static void ZukanDetailVoice_ScrollWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailVoice_DrawWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk, ZukanDetailCommon *common,
                                      u16 species, u16 form);
static void ZukanDetailVoice_LoadWaveScreen(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                            ZukanDetailCommon *common);
static void ZukanDetailVoice_SetActorsOpaque(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                             ZukanDetailCommon *common);
static void ZukanDetailVoice_SetActorsBlended(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                              ZukanDetailCommon *common);

const ZukanDetailProcFuncs ZUKAN_DETAIL_VOICE_PROC_FUNCS = {
    ZukanDetailVoice_Init, ZukanDetailVoice_Main, ZukanDetailVoice_Exit, ZukanDetailVoice_Command, NULL,
};

static const ActorData sZukanDetailVoiceActors[ACTOR_COUNT] = {
    { 200, 120, 28, 0, 1, RES_MAIN_CHARS, RES_MAIN_PALETTE, RES_MAIN_CELL_ANIMS, CLACT_SURFACE_MAIN },
    { 208, 120, 28, 0, 1, RES_MAIN_CHARS, RES_MAIN_PALETTE, RES_MAIN_CELL_ANIMS, CLACT_SURFACE_MAIN },
    { 224, 120, 28, 0, 1, RES_MAIN_CHARS, RES_MAIN_PALETTE, RES_MAIN_CELL_ANIMS, CLACT_SURFACE_MAIN },
    { 232, 120, 28, 0, 1, RES_MAIN_CHARS, RES_MAIN_PALETTE, RES_MAIN_CELL_ANIMS, CLACT_SURFACE_MAIN },
    { 216, 120, 29, 0, 1, RES_MAIN_CHARS, RES_MAIN_PALETTE, RES_MAIN_CELL_ANIMS, CLACT_SURFACE_MAIN },
    { 240, 120, 30, 0, 1, RES_MAIN_CHARS, RES_MAIN_PALETTE, RES_MAIN_CELL_ANIMS, CLACT_SURFACE_MAIN },
    { 128, 96, 0, 0, 3, RES_SUB_CHARS, RES_SUB_PALETTE, RES_SUB_CELL_ANIMS, CLACT_SURFACE_SUB },
};

const ZukanDetailVoiceUnkData ZUKAN_DETAIL_VOICE_UNK_DATA = {
    { 3, 2, 128, 128 },
    1,
    17,
    {
        { 16.0f, 0.0f, 64.0f },
        { -16.0f, -64.0f, 0.0f },
        { -64.0f, 0.0f, -16.0f },
        { 0.0f, 0.0f, 64.0f },
        { 0.0f, 16.0f, 64.0f },
        { 0.0f, 16.0f, 0.0f },
    },
};

void ZukanDetailVoice_InitParam(ZukanDetailVoiceParam *param, HeapID heapId) {
    param->heapId = heapId;
}

static BOOL ZukanDetailVoice_Init(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                  ZukanDetailCommon *common) {
    ZukanDetailVoiceParam *param = param_;
    ZukanDetailVoiceWork *wk = ZukanDetailProcSys_AllocWork(sys, sizeof(ZukanDetailVoiceWork), param->heapId);
    u8 i;

    MI_CpuFill8(wk, 0, sizeof(ZukanDetailVoiceWork));
    wk->unit = ZukanDetailGraphic_GetClActUnit(ZukanDetailCommon_GetGraphic(common));
    wk->font = ZukanDetailCommon_GetFont(common);
    wk->printQueue = ZukanDetailCommon_GetPrintQueue(common);
    wk->waveScreenPending = FALSE;
    wk->playing = FALSE;
    wk->time = 0;
    wk->shownTime = 1;
    wk->bgm = BGM_PLAYING;
    ZukanDetailVoice_FadeOutBGM(param, wk, common);
    wk->vblankTcb = GFL_VBlankTCBAdd(ZukanDetailVoice_VBlank, wk, 1);
    wk->blendMain = ZukanDetailBlend_Create(param->heapId);
    wk->blendSub = ZukanDetailBlend_Create(param->heapId);
    ZukanDetailBlend_InitPlanes(wk->blendMain);
    ZukanDetailBlend_InitPlanes(wk->blendSub);
    ZukanDetailBlend_SetOut(0, wk->blendMain);
    ZukanDetailBlend_SetOut(1, wk->blendSub);
    wk->palFade = ZukanDetailPalFade_Create(param->heapId);
    wk->exit = VOICE_EXIT_NONE;
    wk->inputEnabled = TRUE;
    for (i = 0; i < 2; i++) {
        wk->pokemonActors[i] = NULL;
    }
    wk->pokemon = 0;
    return TRUE;
}

static BOOL ZukanDetailVoice_Exit(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                  ZukanDetailCommon *common) {
    ZukanDetailVoiceParam *param = param_;
    ZukanDetailVoiceWork *wk = work;
    u8 i;

    if (PokeVoice_IsPlayingAny()) {
        PokeVoice_ReleaseAll();
    }
    ZukanDetailVoice_FreeWave(param, wk, common);
    ZukanDetailBackground_Free(wk->backgroundSub);
    ZukanDetailBackground_Free(wk->backgroundMain);
    ZukanDetail_FreeBG(2, wk->bgChars);
    for (i = 0; i < 2; i++) {
        ZukanDetailVoice_FreePokemon(param, wk, common, i);
    }
    ZukanDetailVoice_ClearName(param, wk, common);
    ZukanDetailVoice_FreeName(param, wk, common);
    ZukanDetailVoice_ClearTime(param, wk, common);
    ZukanDetailVoice_FreeTime(param, wk, common);
    ZukanDetailVoice_FreeActors(param, wk, common);
    ZukanDetailPalFade_Free(wk->palFade);
    ZukanDetailBlend_Free(wk->blendSub);
    ZukanDetailBlend_Free(wk->blendMain);
    if (wk->bgm != BGM_RESTORED) {
        ZukanDetailVoice_RestoreBGM(param, wk, common);
    }
    GFL_TCBRemove(wk->vblankTcb);
    ZukanDetailProcSys_FreeWork(sys);
    return TRUE;
}

static BOOL ZukanDetailVoice_Main(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                  ZukanDetailCommon *common) {
    ZukanDetailVoiceParam *param = param_;
    ZukanDetailVoiceWork *wk = work;
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    ZukanDetailHeadbar *headbar = ZukanDetailCommon_GetHeadbar(common);

    switch (*seq) {
    case VOICE_SEQ_INIT: {
        u8 bg;

        *seq = VOICE_SEQ_FADE_IN;
        for (bg = 0; bg <= 7; bg++) {
            if (bg != 1 && bg != 5) {
                GFL_BGSysMoveBG(bg, BG_MOVE_SET_X, 0);
                GFL_BGSysMoveBG(bg, BG_MOVE_SET_Y, 0);
                GFL_BGSysClearBG(bg);
            }
        }
        GFL_BGSysSetBGPriority(2, 1);
        GFL_BGSysSetBGPriority(WAVE_BG, 2);
        GFL_BGSysSetBGPriority(0, 3);
        GFL_BGSysSetBGPriority(6, 1);
        GFL_BGSysSetBGPriority(4, 3);
        ZukanDetailVoice_CreateActors(param, wk, common);
        ZukanDetailVoice_CreatePokemon(param, wk, common, wk->pokemon);
        ClActor_SetObjMode(wk->pokemonActors[wk->pokemon], 1);
        ZukanDetailVoice_CreateName(param, wk, common);
        ZukanDetailVoice_PrintName(param, wk, common);
        ZukanDetailVoice_CreateTime(param, wk, common);
        ZukanDetailVoice_PrintTime(param, wk, common, 0, 0);
        wk->bgChars = ZukanDetail_LoadBG(FALSE, param->heapId, 2, 1, 0, 5, ARCID_ZUKAN_GRA, 1, 11, 43, 0);
        wk->backgroundMain = ZukanDetailBackground_Create(param->heapId, 2, 0, 2, 3);
        wk->backgroundSub = ZukanDetailBackground_Create(param->heapId, 2, 4, 1, 2);
        ZukanDetailVoice_SetTimeDigits(param, wk, common, 0, 0);
        ZukanDetailVoice_CreateWave(param, wk, common);
        ZukanDetailPalFade_ReadPalettes(wk->palFade);
        ZukanDetailPalFade_SetHidden(wk->palFade);
        break;
    }
    case VOICE_SEQ_FADE_IN:
        *seq = VOICE_SEQ_START_PALFADE;
        ZukanDetailBlend_SetIn(0, wk->blendMain);
        ZukanDetailBlend_SetIn(1, wk->blendSub);
        ZukanDetailVoice_SetActorsOpaque(param, wk, common);
        break;
    case VOICE_SEQ_START_PALFADE:
        *seq = VOICE_SEQ_WAIT_FADE_IN;
        ZukanDetailPalFade_StartIn(wk->palFade);
        if (ZukanDetailTouchbar_GetState(touchbar) != ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
            ZukanDetailTouchbar_SetType(touchbar, ZUKAN_DETAIL_TOUCHBAR_GENERAL, ZUKAN_DETAIL_PAGE_VOICE - 1,
                                        ZukanDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
            ZukanDetailTouchbar_Appear(touchbar, 0);
        } else {
            ZukanDetailTouchbar_SetPage(touchbar, ZUKAN_DETAIL_PAGE_VOICE - 1);
        }
        ZukanDetailTouchbar_SetActive(touchbar, FALSE);
        ZukanDetailTouchbar_SetCheck(
            touchbar, GameData_IsShortcutRegistered(ZukanDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_VOICE));
        if (ZukanDetailHeadbar_GetState(headbar) != ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
            ZukanDetailHeadbar_SetTitle(headbar, 2);
            ZukanDetailHeadbar_Appear(headbar);
        }
        break;
    case VOICE_SEQ_WAIT_FADE_IN:
        if (!ZukanDetailPalFade_IsFading(wk->palFade) &&
            ZukanDetailTouchbar_GetState(touchbar) == ZUKAN_DETAIL_TOUCHBAR_SHOWN &&
            ZukanDetailHeadbar_GetState(headbar) == ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
            ZukanDetailTouchbar_Unlock(touchbar);
            ZukanDetailTouchbar_SetActive(touchbar, TRUE);
            *seq = VOICE_SEQ_MAIN;
        }
        break;
    case VOICE_SEQ_MAIN:
        if (wk->exit != VOICE_EXIT_NONE) {
            *seq = VOICE_SEQ_WAIT_FADE_OUT;
            ZukanDetailPalFade_StartOut(wk->palFade);
            ZukanDetailHeadbar_Disappear(headbar);
            if (wk->exit == VOICE_EXIT_SCREEN) {
                ZukanDetailTouchbar_Disappear(touchbar, 0);
            }
            ZukanDetailVoice_RestoreBGM(param, wk, common);
        } else {
            ZukanDetailVoice_CheckInput(param, wk, common);
        }
        break;
    case VOICE_SEQ_WAIT_FADE_OUT: {
        BOOL done = FALSE;

        if (!ZukanDetailPalFade_IsFading(wk->palFade) &&
            ZukanDetailHeadbar_GetState(headbar) == ZUKAN_DETAIL_TOUCHBAR_HIDDEN) {
            if (wk->exit == VOICE_EXIT_SCREEN) {
                if (ZukanDetailTouchbar_GetState(touchbar) == ZUKAN_DETAIL_TOUCHBAR_HIDDEN) {
                    done = TRUE;
                }
            } else {
                done = TRUE;
            }
        }
        if (done) {
            *seq = VOICE_SEQ_FADE_OUT;
        }
        break;
    }
    case VOICE_SEQ_FADE_OUT:
        *seq = VOICE_SEQ_EXIT;
        ZukanDetailVoice_SetActorsBlended(param, wk, common);
        ZukanDetailBlend_SetOut(0, wk->blendMain);
        ZukanDetailBlend_SetOut(1, wk->blendSub);
        break;
    case VOICE_SEQ_EXIT:
        return TRUE;
    }

    if (*seq >= VOICE_SEQ_WAIT_FADE_IN) {
        ZukanDetailVoice_UpdateTime(param, wk, common);
        ZukanDetailVoice_UpdateWave(param, wk, common);
        ZukanDetailVoice_UpdateStretch(param, wk, common);
    }
    if (*seq >= VOICE_SEQ_START_PALFADE) {
        ZukanDetailVoice_FlushName(param, wk, common);
        ZukanDetailBackground_Update(wk->backgroundMain);
        ZukanDetailBackground_Update(wk->backgroundSub);
    }
    ZukanDetailBlend_Update(wk->blendMain, wk->blendSub);
    ZukanDetailPalFade_Update(wk->palFade);
    ZukanDetailVoice_UpdateBGM(param, wk, common);
    return FALSE;
}

static void ZukanDetailVoice_Command(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                     ZukanDetailCommon *common, int command) {
    ZukanDetailVoiceWork *wk = work;

    if (wk != NULL) {
        ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
        BOOL unlock = FALSE;

        // Input waits while the bar's icons play their animations
        switch (command) {
        case ZUKAN_DETAIL_CMD_CLOSE_TOUCH:
        case ZUKAN_DETAIL_CMD_RETURN_TOUCH:
        case ZUKAN_DETAIL_CMD_CUR_D_TOUCH:
        case ZUKAN_DETAIL_CMD_CUR_U_TOUCH:
        case ZUKAN_DETAIL_CMD_CHECK_TOUCH:
        case ZUKAN_DETAIL_CMD_INFO_TOUCH:
        case ZUKAN_DETAIL_CMD_MAP_TOUCH:
        case ZUKAN_DETAIL_CMD_FORM_TOUCH:
            wk->inputEnabled = FALSE;
            break;
        }
        switch (command) {
        case ZUKAN_DETAIL_CMD_CLOSE:
        case ZUKAN_DETAIL_CMD_RETURN:
        case ZUKAN_DETAIL_CMD_CUR_D:
        case ZUKAN_DETAIL_CMD_CUR_U:
        case ZUKAN_DETAIL_CMD_CHECK:
        case ZUKAN_DETAIL_CMD_INFO:
        case ZUKAN_DETAIL_CMD_MAP:
        case ZUKAN_DETAIL_CMD_FORM:
            unlock = TRUE;
            wk->inputEnabled = TRUE;
            break;
        }

        switch (command) {
        case ZUKAN_DETAIL_CMD_CUR_D_TOUCH: {
            u16 species = ZukanDetailCommon_GetSpecies(common);

            ZukanDetailCommon_GoNext(common);
            if (species != ZukanDetailCommon_GetSpecies(common)) {
                ZukanDetailVoice_ChangePokemon(param, wk, common);
            }
            break;
        }
        case ZUKAN_DETAIL_CMD_CUR_U_TOUCH: {
            u16 species = ZukanDetailCommon_GetSpecies(common);

            ZukanDetailCommon_GoPrev(common);
            if (species != ZukanDetailCommon_GetSpecies(common)) {
                ZukanDetailVoice_ChangePokemon(param, wk, common);
            }
            break;
        }
        }

        switch (command) {
        case ZUKAN_DETAIL_CMD_NONE:
            break;
        case ZUKAN_DETAIL_CMD_CLOSE:
        case ZUKAN_DETAIL_CMD_RETURN:
            wk->exit = VOICE_EXIT_SCREEN;
            break;
        case ZUKAN_DETAIL_CMD_INFO:
        case ZUKAN_DETAIL_CMD_MAP:
        case ZUKAN_DETAIL_CMD_FORM:
            wk->exit = VOICE_EXIT_PAGE;
            break;
        case ZUKAN_DETAIL_CMD_CUR_D:
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        case ZUKAN_DETAIL_CMD_CUR_U:
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        case ZUKAN_DETAIL_CMD_CHECK:
            GameData_SetKeyItemRegistration(ZukanDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_VOICE,
                                            ZukanDetailTouchbar_GetCheck(touchbar));
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        default:
            if (unlock) {
                ZukanDetailTouchbar_Unlock(touchbar);
            }
            break;
        }
    }
}

// Scrolls the wave's BG to where its screen was moved to
static void ZukanDetailVoice_VBlank(TCB *tcb, void *data) {
    ZukanDetailVoiceWork *wk = data;

    if (wk->waveScreenPending) {
        GFL_BGSysMoveBG(WAVE_BG, BG_MOVE_SET_X, wk->waveScroll - 1);
        GFL_BGSysMoveBG(WAVE_BG, BG_MOVE_SET_Y, 0);
        GFL_BGSysLoadScr(WAVE_BG);
        wk->waveScreenPending = FALSE;
    }
    ZukanDetailPalFade_VBlank(wk->palFade);
}

static void ZukanDetailVoice_FadeOutBGM(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common) {
    GFL_SndBGMFadeOut(30);
    wk->bgm = BGM_FADING_OUT;
}

static void ZukanDetailVoice_RestoreBGM(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common) {
    if (wk->bgm == BGM_PAUSED) {
        GFL_SndBGMPop();
        GFL_SndBGMSetPaused(FALSE);
    }
    GFL_SndBGMFadeIn(60);
    wk->bgm = BGM_RESTORED;
}

// Pauses the music once it has faded out
static void ZukanDetailVoice_UpdateBGM(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common) {
    if (wk->bgm == BGM_FADING_OUT && !GFL_SndBGMIsFading()) {
        GFL_SndBGMSetPaused(TRUE);
        GFL_SndBGMPush();
        wk->bgm = BGM_PAUSED;
    }
}

static void ZukanDetailVoice_CreateActors(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                          ZukanDetailCommon *common) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, param->heapId);
    ClActorSetup setup;
    u8 i;

    wk->resources[RES_MAIN_PALETTE] = ClActSys_LoadPlttRange(arc, 3, CLACT_VRAM_MAIN, 0, 0, 4, param->heapId);
    wk->resources[RES_MAIN_CHARS] = ClActSys_LoadChars(arc, 13, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->resources[RES_MAIN_CELL_ANIMS] = ClActSys_LoadCellAnims(arc, 28, 45, param->heapId);
    wk->resources[RES_SUB_PALETTE] = ClActSys_LoadPlttRange(arc, 8, CLACT_VRAM_SUB, 2 * 32, 0, 1, param->heapId);
    wk->resources[RES_SUB_CHARS] = ClActSys_LoadChars(arc, 25, FALSE, CLACT_VRAM_SUB, param->heapId);
    wk->resources[RES_SUB_CELL_ANIMS] = ClActSys_LoadCellAnims(arc, 31, 48, param->heapId);
    GFL_ArcToolFree(arc);

    for (i = 0; i < ACTOR_COUNT; i++) {
        MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
        setup.x = sZukanDetailVoiceActors[i].x;
        setup.y = sZukanDetailVoiceActors[i].y;
        setup.sequence = sZukanDetailVoiceActors[i].anim;
        setup.priority = sZukanDetailVoiceActors[i].priority;
        setup.bgPriority = sZukanDetailVoiceActors[i].bgPriority;
        wk->actors[i] = ClActor_Create(wk->unit, wk->resources[sZukanDetailVoiceActors[i].chars],
                                       wk->resources[sZukanDetailVoiceActors[i].palette],
                                       wk->resources[sZukanDetailVoiceActors[i].cellAnims], &setup,
                                       sZukanDetailVoiceActors[i].surface, param->heapId);
        ClActor_SetVisible(wk->actors[i], TRUE);
        ClActor_SetObjMode(wk->actors[i], 1);
    }
    func_0204c520(wk->actors[ACTOR_ICON], TRUE);
}

static void ZukanDetailVoice_FreeActors(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        ClActor_Delete(wk->actors[i]);
    }
    ClActSys_FreePltt(wk->resources[RES_MAIN_PALETTE]);
    ClActSys_FreeChars(wk->resources[RES_MAIN_CHARS]);
    ClActSys_FreeCellAnims(wk->resources[RES_MAIN_CELL_ANIMS]);
    ClActSys_FreePltt(wk->resources[RES_SUB_PALETTE]);
    ClActSys_FreeChars(wk->resources[RES_SUB_CHARS]);
    ClActSys_FreeCellAnims(wk->resources[RES_SUB_CELL_ANIMS]);
}

// Shows the time on the digits' frames, up to 99.99
static void ZukanDetailVoice_SetTimeDigits(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                           ZukanDetailCommon *common, u16 seconds, u16 hundredths) {
    u8 actors[DIGIT_COUNT] = { 0, 1, 2, 3 };
    u16 digits[DIGIT_COUNT];
    u8 i;

    digits[0] = seconds / 10;
    if (digits[0] > 9) {
        digits[0] = 9;
    }
    digits[1] = seconds % 10;
    digits[2] = hundredths / 10;
    if (digits[2] > 9) {
        digits[2] = 9;
    }
    digits[3] = hundredths % 10;
    for (i = 0; i < DIGIT_COUNT; i++) {
        ClActor_SetAnimFrame(wk->actors[actors[i]], digits[i]);
    }
}

// Loads the Pokémon's sprite into one of the two slots, its palette into the palette fade's OBJ palette of the slot
static void ZukanDetailVoice_CreatePokemon(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                           ZukanDetailCommon *common, int index) {
    u16 species = ZukanDetailCommon_GetSpecies(common);
    u32 personality = 0;
    PokeDexSave *pokedex = GameData_GetPokedex(ZukanDetailCommon_GetGameData(common));
    u32 sex;
    u32 rare;
    u32 form;
    ArcTool *arc;
    ClActorSetupEx setup;

    PokeDex_GetShownSexRareForm(pokedex, species, &sex, &rare, &form, param->heapId);
    if (species == SPECIES_SPINDA) {
        personality = func_0200da18(pokedex, personality);
    }
    arc = MakePokeGraArcHandle(param->heapId);
    wk->pokemonChars[index] =
        PokeGra_LoadClActChars(arc, species, form, sex, rare, 0, 0, personality, CLACT_VRAM_SUB, param->heapId);
    wk->pokemonPalettes[index] =
        PokeGra_LoadClActPalette(arc, species, form, sex, rare, 0, 0, CLACT_VRAM_SUB, index * 32, param->heapId);
    wk->pokemonCellAnims[index] =
        PokeGra_LoadClActCellAnims(species, form, sex, rare, 0, 0, 2, CLACT_VRAM_SUB, param->heapId);
    ZukanDetailPalFade_LoadPalette(wk->palFade, arc,
                                   GetPokemonPaletteDataNo(GetPokemonGraphicsARCID(), species, form, sex, rare, 0, 0),
                                   param->heapId, 3, 32, index * 16, 0);
    GFL_ArcToolFree(arc);

    MI_CpuFill8(&setup, 0, sizeof(ClActorSetupEx));
    setup.base.x = 128;
    setup.base.y = 128;
    setup.base.sequence = 0;
    setup.base.priority = 0;
    setup.base.bgPriority = 0;
    setup.affineCenter.x = 0;
    setup.affineCenter.y = 0;
    setup.scaleX = FX32_ONE;
    setup.scaleY = FX32_ONE;
    setup.rotation = 0;
    setup.affineMode = 2;
    wk->pokemonActors[index] = ClActor_CreateEx(wk->unit, wk->pokemonChars[index], wk->pokemonPalettes[index],
                                                wk->pokemonCellAnims[index], &setup, CLACT_SURFACE_SUB, param->heapId);
    ClActor_SetObjMode(wk->pokemonActors[index], 0);
    wk->stretch = STRETCH_NONE;
}

static void ZukanDetailVoice_FreePokemon(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                         ZukanDetailCommon *common, int index) {
    if (wk->pokemonActors[index] != NULL) {
        ClActor_Delete(wk->pokemonActors[index]);
        wk->pokemonActors[index] = NULL;
        ClActSys_FreePltt(wk->pokemonPalettes[index]);
        ClActSys_FreeChars(wk->pokemonChars[index]);
        ClActSys_FreeCellAnims(wk->pokemonCellAnims[index]);
    }
}

// Stretches the Pokémon up by a hundredth a frame to 1.1 times its height, or back down, keeping its feet in place
static void ZukanDetailVoice_UpdateStretch(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                           ZukanDetailCommon *common) {
    ClActorPos pos;
    ClActorScale scale;
    f32 scaleY;

    switch (wk->stretch) {
    case STRETCH_NONE:
        break;
    case STRETCH_GROWING:
        ClActor_GetSurfacePos(wk->pokemonActors[wk->pokemon], &pos, CLACT_SURFACE_SUB);
        ClActor_GetScale(wk->pokemonActors[wk->pokemon], &scale);
        scaleY = scale.y / (f32)FX32_ONE;
        scaleY += 0.01f;
        if (scaleY >= 1.1f) {
            scaleY = 1.1f;
            wk->stretch = STRETCH_GROWN;
        }
        pos.y = 128 - (int)((scaleY - 1.0f) * 96.0f / 2.0f);
        scale.y = FX32_CONST(scaleY);
        ClActor_SetSurfacePos(wk->pokemonActors[wk->pokemon], &pos, CLACT_SURFACE_SUB);
        ClActor_SetScale(wk->pokemonActors[wk->pokemon], &scale);
        break;
    case STRETCH_GROWN:
        break;
    case STRETCH_SHRINKING:
        ClActor_GetSurfacePos(wk->pokemonActors[wk->pokemon], &pos, CLACT_SURFACE_SUB);
        ClActor_GetScale(wk->pokemonActors[wk->pokemon], &scale);
        scaleY = scale.y / (f32)FX32_ONE;
        scaleY -= 0.01f;
        if (scaleY <= 1.0f) {
            scaleY = 1.0f;
            wk->stretch = STRETCH_NONE;
            pos.y = 128;
        } else {
            pos.y = 128 - (int)((scaleY - 1.0f) * 96.0f / 2.0f);
        }
        scale.y = FX32_CONST(scaleY);
        ClActor_SetSurfacePos(wk->pokemonActors[wk->pokemon], &pos, CLACT_SURFACE_SUB);
        ClActor_SetScale(wk->pokemonActors[wk->pokemon], &scale);
        break;
    }
}

static void ZukanDetailVoice_StartStretch(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                          ZukanDetailCommon *common) {
    ClActorPos pos;
    ClActorScale scale;

    pos.x = 128;
    pos.y = 128;
    scale.x = FX32_ONE;
    scale.y = FX32_ONE;
    ClActor_SetSurfacePos(wk->pokemonActors[wk->pokemon], &pos, CLACT_SURFACE_SUB);
    ClActor_SetScale(wk->pokemonActors[wk->pokemon], &scale);
    wk->stretch = STRETCH_GROWING;
}

static void ZukanDetailVoice_EndStretch(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common) {
    if (wk->stretch == STRETCH_GROWING || wk->stretch == STRETCH_GROWN) {
        wk->stretch = STRETCH_SHRINKING;
    }
}

static void ZukanDetailVoice_CreateName(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common) {
    GFL_G2DIOLoadNCLR(ARCID_FONT, 5, PALTYPE_SUB_BG, 0, 0, 0x20, param->heapId);
    wk->nameWindow = BmpWin_CreateDynamic(6, 10, 5, 12, 4, 0, 1);
    wk->nameFlushPending = FALSE;
    ZukanDetailVoice_ClearName(param, wk, common);
    BmpWin_FlushChar(wk->nameWindow);
}

static void ZukanDetailVoice_FreeName(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                      ZukanDetailCommon *common) {
    wk->nameFlushPending = FALSE;
    PrintQueue_Clear(wk->printQueue);
    BmpWin_Free(wk->nameWindow);
}

static void ZukanDetailVoice_PrintName(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common) {
    StrBuf *name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, ZukanDetailCommon_GetSpecies(common));
    u16 width = GFL_FontGetBlockWidth(name, wk->font, 0);
    int windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->nameWindow));
    u16 x = (windowWidth - width) / 2;

    PrintQueue_PrintColor(wk->printQueue, BmpWin_GetBitmap(wk->nameWindow), x, 1, name, wk->font,
                          PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(name);
    wk->nameFlushPending = TRUE;
    ZukanDetailVoice_FlushName(param, wk, common);
}

static void ZukanDetailVoice_ClearName(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common) {
    wk->nameFlushPending = FALSE;
    PrintQueue_Clear(wk->printQueue);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->nameWindow), 0);
}

static void ZukanDetailVoice_FlushName(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common) {
    if (wk->nameFlushPending && !PrintQueue_IsBitmapPending(wk->printQueue, BmpWin_GetBitmap(wk->nameWindow))) {
        BmpWin_Transfer(wk->nameWindow);
        wk->nameFlushPending = FALSE;
    }
}

// The time was once printed as text, which the digits' actors replaced: only the palette is left
static void ZukanDetailVoice_CreateTime(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common) {
    GFL_G2DIOLoadNCLR(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0, 0x20, 0x20, param->heapId);
}

static void ZukanDetailVoice_FreeTime(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                      ZukanDetailCommon *common) {
}

static void ZukanDetailVoice_PrintTime(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common, u16 seconds, u16 hundredths) {
}

static void ZukanDetailVoice_ClearTime(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common) {
}

// Counts the time the cry has played, up to its length, and shows it rounded to hundredths of a second
static void ZukanDetailVoice_UpdateTime(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common) {
    if (wk->playing) {
        u64 now = OS_GetTick();
        u64 elapsed;
        u64 time;

        if (now >= wk->startTime) {
            elapsed = now - wk->startTime;
        } else {
            elapsed = now + (0xffffffffffffffff - wk->startTime);
        }
        time = OS_TicksToMilliSeconds(elapsed);
        if (time > TIME_MAX) {
            time = TIME_MAX;
        }
        wk->time = time;
        if (wk->time >= wk->duration) {
            wk->time = wk->duration;
            wk->playing = FALSE;
            ZukanDetailVoice_EndStretch(param, wk, common);
        }
    }

    if (wk->time != wk->shownTime) {
        u16 seconds = wk->time / 1000;
        u16 milliseconds = wk->time % 1000;
        u16 hundredths = milliseconds / 10 + (milliseconds % 10 >= 5 ? 1 : 0);

        ZukanDetailVoice_ClearTime(param, wk, common);
        ZukanDetailVoice_PrintTime(param, wk, common, seconds, hundredths);
        ZukanDetailVoice_SetTimeDigits(param, wk, common, seconds, hundredths);
        wk->shownTime = wk->time;
    }
}

static void ZukanDetailVoice_StopVoice(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common) {
    if (wk->playing) {
        if (PokeVoice_IsPlayingAny()) {
            PokeVoice_ReleaseAll();
        }
        wk->playing = FALSE;
    }
    wk->time = 0;
}

// Loads the Pokémon the list moved to into the other slot
static void ZukanDetailVoice_ChangePokemon(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                           ZukanDetailCommon *common) {
    if (wk->pokemonActors[wk->pokemon] != NULL) {
        ClActor_SetVisible(wk->pokemonActors[wk->pokemon], FALSE);
    }
    wk->pokemon = (wk->pokemon + 1) % 2;
    ZukanDetailVoice_ClearName(param, wk, common);
    ZukanDetailVoice_FreePokemon(param, wk, common, wk->pokemon);
    ZukanDetailVoice_CreatePokemon(param, wk, common, wk->pokemon);
    ZukanDetailVoice_PrintName(param, wk, common);
    ZukanDetailVoice_StopVoice(param, wk, common);
    ZukanDetailVoice_ResetWave(param, wk, common);
}

// A or a touch above the touch bar plays the cry
static void ZukanDetailVoice_CheckInput(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common) {
    BOOL play = FALSE;

    if (wk->inputEnabled) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            GCTX_HIDSetUsingTouch(FALSE);
            play = TRUE;
        } else {
            u32 x;
            u32 y;

            if (GFL_HIDGetTouchPressedPoint(&x, &y) && x < 256 && y < 168) {
                play = TRUE;
                GCTX_HIDSetUsingTouch(TRUE);
            }
        }
    }

    if (play) {
        PokeDexSave *pokedex = GameData_GetPokedex(ZukanDetailCommon_GetGameData(common));
        u16 species = ZukanDetailCommon_GetSpecies(common);
        u32 form;
        u32 sex;
        u32 rare;

        PokeDex_GetShownSexRareForm(pokedex, species, &sex, &rare, &form, param->heapId);
        wk->voice = PokeVoice_Play(species, form, 64, 0, 0, 0, 0, NULL);
        wk->time = 0;
        wk->startTime = OS_GetTick();
        wk->playing = TRUE;
        ZukanDetailVoice_DrawWave(param, wk, common, species, form);
        ZukanDetailVoice_StartStretch(param, wk, common);
    }
}

static void ZukanDetailVoice_CreateWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common) {
    wk->waveScreen = GFL_HeapAllocate(param->heapId, WAVE_WINDOW_WIDTH * WAVE_WINDOW_HEIGHT * sizeof(u16), TRUE,
                                      "zukan_detail_voice.c", 2209);
    wk->waveWindow = BmpWin_CreateDynamic(WAVE_BG, 1, 2, WAVE_WINDOW_WIDTH, WAVE_WINDOW_HEIGHT, 1, 1);
    ZukanDetailVoice_ResetWave(param, wk, common);
    wk->waveBitmap = GFL_BitmapCreate(WAVE_COLUMNS / 8 + 1, WAVE_WINDOW_HEIGHT, 0x20, param->heapId);
}

static void ZukanDetailVoice_FreeWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                      ZukanDetailCommon *common) {
    GFL_BitmapFree(wk->waveBitmap);
    BmpWin_Free(wk->waveWindow);
    GFL_HeapFree(wk->waveScreen);
}

static void ZukanDetailVoice_UpdateWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common) {
    if (wk->waveWait != 0) {
        wk->waveWait--;
    } else {
        ZukanDetailVoice_ScrollWave(param, wk, common);
        wk->waveWait = 0;
    }
}

// Clears the window to the middle line
static void ZukanDetailVoice_ResetWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                       ZukanDetailCommon *common) {
    GFLBitmap *bitmap;

    wk->waveWait = 0;
    wk->waveScroll = 0;
    wk->waveColumn = 0;
    wk->waveSpecies = 0;
    wk->waveForm = 0;
    wk->waveWidth = 0;
    wk->waveShown = wk->waveWidth;
    wk->duration = 0;
    bitmap = BmpWin_GetBitmap(wk->waveWindow);
    GFL_BitmapFill(bitmap, 0);
    GFL_BitmapFillArea(bitmap, 0, WAVE_CENTER, WAVE_SHOWN_WIDTH, 1, WAVE_COLOR);
    ZukanDetailVoice_LoadWaveScreen(param, wk, common);
}

// Scrolls the window by two columns, and draws the wave's next two columns at its right edge, or the middle line once
// the wave has been shown.
static void ZukanDetailVoice_ScrollWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                        ZukanDetailCommon *common) {
    GFLBitmap *bitmap = BmpWin_GetBitmap(wk->waveWindow);
    s16 left = wk->waveScroll + wk->waveColumn * 8;
    s16 x = (left + WAVE_SHOWN_WIDTH) % WAVE_WIDTH;
    s16 drawn = 0;
    s16 end;
    int width;

    wk->waveScroll += WAVE_STEP;
    while (wk->waveScroll >= 8) {
        GFL_BitmapFillArea(bitmap, wk->waveColumn * 8, 0, 8, WAVE_HEIGHT, 0);
        wk->waveColumn = (wk->waveColumn + 1) % WAVE_WINDOW_WIDTH;
        wk->waveScroll -= 8;
    }
    if (wk->waveScroll > 0) {
        GFL_BitmapFillArea(bitmap, wk->waveColumn * 8, 0, wk->waveScroll, WAVE_HEIGHT, 0);
    }

    while (drawn < WAVE_STEP) {
        s16 rest = wk->waveWidth - wk->waveShown;

        end = x + WAVE_STEP - drawn;
        if (rest <= 0) {
            break;
        }
        if (end > WAVE_WIDTH) {
            end = WAVE_WIDTH;
        }
        if (end - x > rest) {
            end = x + rest;
        }
        width = end - x;
        GFL_BitmapCopyArea(wk->waveBitmap, bitmap, wk->waveShown, 0, x, 0, width, WAVE_HEIGHT, GFL_BITMAP_NO_COLOR_KEY);
        drawn += width;
        x = end % WAVE_WIDTH;
#ifdef BUGFIX
        wk->waveShown += width;
#else
        // BUG: Adds all the columns drawn this frame, so a frame whose two columns are split at the window's edge
        // skips a column of the wave
        wk->waveShown += drawn;
#endif
    }
    while (drawn < WAVE_STEP) {
        end = x + WAVE_STEP - drawn;
        if (end > WAVE_WIDTH) {
            end = WAVE_WIDTH;
        }
        width = end - x;
        GFL_BitmapFillArea(bitmap, x, WAVE_CENTER, width, 1, WAVE_COLOR);
        drawn += width;
        x = end % WAVE_WIDTH;
    }
    ZukanDetailVoice_LoadWaveScreen(param, wk, common);
}

// Draws the cry's whole wave into the bitmap, a column of the samples' range for every half frame
static void ZukanDetailVoice_DrawWave(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk, ZukanDetailCommon *common,
                                      u16 species, u16 form) {
    u32 column;
    u32 end;
    u32 i;

    wk->samples = PokeVoice_GetSamples(wk->voice);
    wk->waveShown = 0;
    if (species == wk->waveSpecies && form == wk->waveForm) {
        return;
    }
    wk->waveSpecies = species;
    wk->waveForm = form;
    {
        int rate = PokeVoice_GetSampleRate(wk->voice);
        int speed = PokeVoice_GetSpeed(wk->voice);

        wk->duration = PokeVoice_GetSampleCount(wk->voice) * 32768.0f / rate / speed * 1000.0f + 0.5f;
    }

    GFL_BitmapFill(wk->waveBitmap, 0);
    column = 0;
    i = 0;
    while (i < PokeVoice_GetSampleCount(wk->voice)) {
        int speed = PokeVoice_GetSpeed(wk->voice);
        int rate = PokeVoice_GetSampleRate(wk->voice);

        end = (column + 1) * ((f32)rate * speed / 32768.0f / 60.0f / 2.0f);
        if (end > PokeVoice_GetSampleCount(wk->voice)) {
            end = PokeVoice_GetSampleCount(wk->voice);
        }
        if (column < WAVE_COLUMNS) {
            s16 min = 500;
            s16 max = -500;

            for (; i < end; i++) {
                s16 sample = wk->samples[i] * 47 * PokeVoice_GetVolume(wk->voice) / 128 / 128;

                sample = MATH_CLAMP(sample, -500, 500);
                if (sample < min) {
                    min = sample;
                }
                if (sample > max) {
                    max = sample;
                }
            }
            if (min <= max) {
                if (min < -WAVE_CENTER) {
                    min = -WAVE_CENTER;
                }
                if (max > WAVE_CENTER) {
                    max = WAVE_CENTER;
                }
                GFL_BitmapFillArea(wk->waveBitmap, column, WAVE_CENTER - max, 1, max - min + 1, WAVE_COLOR);
            }
        }
        column++;
        i = end + 1;
    }
    if (column > WAVE_COLUMNS) {
        column = WAVE_COLUMNS;
    }
    wk->waveWidth = column;
}

// Points the window's screen at its characters from the column at its left edge on, with BG 3 scrolled at VBlank
static void ZukanDetailVoice_LoadWaveScreen(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                            ZukanDetailCommon *common) {
    u16 chars = BmpWin_GetCharPos(wk->waveWindow);
    u16 n = 0;
    u16 y;
    u16 x;

    for (y = 0; y < WAVE_WINDOW_HEIGHT; y++) {
        for (x = 0; x < WAVE_WINDOW_WIDTH; x++) {
            wk->waveScreen[n] =
                (u16)(chars + y * WAVE_WINDOW_WIDTH + (x + wk->waveColumn) % WAVE_WINDOW_WIDTH) | (1 << 12);
            n++;
        }
    }
    GFL_BGSysLoadScrAreaAll(WAVE_BG, wk->waveScreen, 1, 2, WAVE_WINDOW_WIDTH, WAVE_WINDOW_HEIGHT);
    wk->waveScreenPending = TRUE;
    BmpWin_FlushChar(wk->waveWindow);
}

static void ZukanDetailVoice_SetActorsOpaque(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                             ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (wk->pokemonActors[i] != NULL) {
            ClActor_SetObjMode(wk->pokemonActors[i], 0);
        }
    }
    for (i = 0; i < ACTOR_ICON; i++) {
        ClActor_SetObjMode(wk->actors[i], 0);
    }
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2,
                       GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 |
                           GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                       16, 8);
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, 0,
                       GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 |
                           GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                       12, 4);
}

static void ZukanDetailVoice_SetActorsBlended(ZukanDetailVoiceParam *param, ZukanDetailVoiceWork *wk,
                                              ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (wk->pokemonActors[i] != NULL) {
            ClActor_SetObjMode(wk->pokemonActors[i], 1);
        }
    }
    for (i = 0; i < ACTOR_ICON; i++) {
        ClActor_SetObjMode(wk->actors[i], 1);
    }
    ZukanDetailBlend_InitPlanes(wk->blendMain);
    ZukanDetailBlend_InitPlanes(wk->blendSub);
}
