#include "types.h"
#include "app/trainer_card.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "field/trcard_sys.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "save/player_info.h"
#include "save/timesig_savedata.h"
#include "system/app_menu_common.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wipe.h"
#include "system/wordset.h"

// The trainer card screen of overlay 186: the card's front and back on the touch screen, which the player flips,
// scrolls and signs. The ROM has no name for this file; trainer_card.c is a guess

// The proc's states
enum {
    TRCARD_SEQ_FADE_IN,
    TRCARD_SEQ_MAIN,
    TRCARD_SEQ_RETURN,
    TRCARD_SEQ_EXIT,
    TRCARD_SEQ_FADE_OUT,
    TRCARD_SEQ_FLIP,
    TRCARD_SEQ_GREETING,
    TRCARD_SEQ_BADGES,
    TRCARD_SEQ_8,
    TRCARD_SEQ_ZOOM,
    TRCARD_SEQ_MEDALS,
};

// What the input asks for
enum {
    TRCARD_INPUT_NONE,
    TRCARD_INPUT_FLIP = 3,
    TRCARD_INPUT_RETURN,
    TRCARD_INPUT_EXIT,
    TRCARD_INPUT_UNION_VIEW,
    TRCARD_INPUT_PERSONALITY,
    TRCARD_INPUT_ZOOM,
    TRCARD_INPUT_GREETING,
    TRCARD_INPUT_BADGES,
    TRCARD_INPUT_MEDALS,
};

// The card's scale while it flips and before the signature is zoomed
#define TRCARD_SCALE 0x103d

// A union room appearance's picture and palette in ARCID_TRAINER_CARD_UNION_GRA
typedef struct {
    u32 chars;
    u32 palette;
} TrainerCardUnionGra;

// A scroll arrow's animation and palette
typedef struct {
    s16 sequence;
    s16 palette;
} TrainerCardArrowAnime;

static void TrainerCard_NextUnionView(TrainerCardData *data);
static void TrainerCard_PlayPenSE(TrainerCardWork *wk, int count);
static void TrainerCard_LoadMessages(TrainerCardWork *wk);
static void TrainerCard_FreeMessages(TrainerCardWork *wk);
static void TrainerCard_LoadCardPalette(TrainerCardWork *wk, int stars, u32 hasPokedex);
static void TrainerCard_LoadUnionPalette(TrainerCardWork *wk, u32 view);
static void TrainerCard_SetupBG(void);
static u32 TrainerCard_GetFrontCharFile(u32 version);
static u32 TrainerCard_GetFrontScreenFile(u32 version);
static void TrainerCard_LoadGraphics(TrainerCardWork *wk);
static void TrainerCard_ReleaseBG(void);
static void TrainerCard_LoadBackScreen(TrainerCardWork *wk, u32 version);
static void TrainerCard_LoadSideScreen(TrainerCardWork *wk);
static void TrainerCard_DrawSide(TrainerCardWork *wk);
static void TrainerCard_SetButtonsEdit(TrainerCardWork *wk, int side);
static void TrainerCard_SetButtonsLocked(TrainerCardWork *wk, int side);
static void TrainerCard_SetButtonsOther(TrainerCardWork *wk, int side);
static void TrainerCard_SetButtons(TrainerCardWork *wk, int side);
static BOOL TrainerCard_FlipSeq(TrainerCardWork *wk);
static void TrainerCard_SetSignWindow(int side);
static BOOL TrainerCard_ZoomSeq(TrainerCardWork *wk);
static void TrainerCard_UpdateAffine(TrainerCardWork *wk);
static BOOL TrainerCard_CheckInputMode(TrainerCardWork *wk);
static int TrainerCard_KeyInput(TrainerCardWork *wk);
static void TrainerCard_ToggleShortcut(TrainerCardWork *wk);
static void TrainerCard_SetPenButton(TrainerCardWork *wk, int eraser, int visible);
static void TrainerCard_PressPenButton(TrainerCardWork *wk, int eraser);
static void TrainerCard_SetSignBlink(TrainerCardWork *wk, int eraser);
static BOOL TrainerCard_IsLocked(TrainerCardWork *wk, int button);
static int TrainerCard_TouchCard(TrainerCardWork *wk, int button);
static int TrainerCard_TouchSign(TrainerCardWork *wk, int button);
static int TrainerCard_CheckScrollTouch(TrainerCardWork *wk, BOOL held);
static void TrainerCard_ScrollRelease(TrainerCardWork *wk);
static void TrainerCard_ScrollTouch(TrainerCardWork *wk);
static int TrainerCard_TouchInput(TrainerCardWork *wk);
static int TrainerCard_Input(TrainerCardWork *wk);
static void TrainerCard_HandleInput(TrainerCardWork *wk, int input, u32 *state);
static void TrainerCard_VBlank(TCB *tcb, void *data);
static void TrainerCard_ResetAffine(void);
static void TrainerCard_DrawTrainer(TrainerCardWork *wk);
static void TrainerCard_ClearSignArea(TrainerCardWork *wk);
static void TrainerCard_UnpackSign(const u8 *src, u8 *dest);
static void TrainerCard_PackSign(const u8 *src, u8 *dest);
static void TrainerCard_SetSignScreen(u32 bg, int mode);
static void TrainerCard_LoadSign(u32 bg, const u8 *pixels, u8 mode);
static void TrainerCard_UpdateSignBlink(TrainerCardWork *wk);
static void TrainerCard_AnimatePalette(TrainerCardWork *wk);
static void TrainerCard_CheckPenRelease(TrainerCardWork *wk);
static void TrainerCard_ReadPen(TrainerCardWork *wk, int zoomed);
static void TrainerCard_DrawBrush(u8 *pixels, const u8 *brush, int x, int y, int width, int height, int zoomed,
                                  int zoomSide);
static int TrainerCard_DrawLine(u8 *pixels, const u8 *brush, int x, int y, int *lastX, int *lastY, int first, int on,
                                int zoomed, int zoomSide);
static void TrainerCard_CopyPen(TrainerCardPen *src, TrainerCardPen *dest);
static int TrainerCard_DrawPen(u8 *pixels, TrainerCardPen *pen, TrainerCardPen *old, BOOL load, const u8 *loadSrc,
                               u8 zoomed, u8 zoomSide);
static void TrainerCard_SetActorVisible(TrainerCardWork *wk, int actor, BOOL visible);
static void TrainerCard_SetSideActors(TrainerCardWork *wk, int mode, BOOL canEdit);
static void TrainerCard_InitRank(TrainerCardWork *wk);
static void TrainerCard_LoadRankPalette(TrainerCardWork *wk);
static void TrainerCard_AnimateRankPalette(TrainerCardWork *wk);
static void TrainerCard_SetRankAnime(TrainerCardWork *wk, int on);
static void TrainerCard_SetScrollArrows(TrainerCardWork *wk, int mode);

// The card's palette for each number of stars, by the version's color, and the one of a card from another game
static u32 sTrainerCardPalettes[7][2] = {
    { 18, 8 }, { 19, 9 }, { 20, 10 }, { 21, 11 }, { 22, 12 }, { 23, 13 }, { 14, 14 },
};

static const BGSysVRAMConfig sTrainerCardVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const TrainerCardUnionGra sTrainerCardUnionGra[16] = {
    { 16, 0 }, { 17, 1 }, { 18, 2 },  { 19, 3 },  { 20, 4 },  { 21, 5 },  { 22, 6 },  { 23, 7 },
    { 24, 8 }, { 25, 9 }, { 26, 10 }, { 27, 11 }, { 28, 12 }, { 29, 13 }, { 30, 14 }, { 31, 15 },
};

// The rank actor's animation of each rank
static const s16 sTrainerCardRankAnimes[] = { 28, 29, 30, 31, 32, 33 };

BOOL TrainerCard_Init(GameProc *proc, u32 *state, void *param, void *work) {
    TrainerCardWork *wk;
    TrainerCardParam *cardParam = param;
    int i;
    u16 bit;

    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    Wipe_SetScreenCovered(0, 0);
    Wipe_SetScreenCovered(1, 0);
    Wipe_HideWindows(0);
    Wipe_HideWindows(1);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_TRAINER_CARD_SCREEN, 0x50000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(TrainerCardWork), HEAPID_TRAINER_CARD_SCREEN);
    sys_memset(wk, 0, sizeof(TrainerCardWork));
    wk->heapId = HEAPID_TRAINER_CARD_SCREEN;
    wk->obj.heapId = HEAPID_TRAINER_CARD_SCREEN;
    wk->param = cardParam;
    wk->data = cardParam->shownData;
    wk->ownData = cardParam->data;
    wk->touchMode = func_0203d554();
    wk->msgSpeed = func_02017bcc();
    wk->frameType = 0;
    if (wk->data->clearMonth != 0) {
        wk->isCleared = TRUE;
    } else {
        wk->isCleared = FALSE;
    }
    bit = 1;
    for (i = 0; i < 16; i++) {
        if (wk->data->badges & bit) {
            wk->badges[i] = 1;
        } else {
            wk->badges[i] = 0;
        }
        bit <<= 1;
    }
    if (func_02042a78() > 0) {
        wk->isOnline = TRUE;
    }
    GFL_BGSysCreate(wk->heapId);
    TrainerCard_LoadMessages(wk);
    GFL_BGSysSetVRAMBanks(&sTrainerCardVRAMConfig);
    TrainerCard_SetupBG();
    TrainerCard_LoadGraphics(wk);
    TrCardObj_Init(&wk->obj, &sTrainerCardVRAMConfig);
    TrCardBmp_InitRows(wk, wk->param->shownData, wk->param->data);
    TrCardObj_AddActors(&wk->obj);
    TrCardBmp_Init(wk);
    wk->pmsDraw =
        PMSDraw_Create(wk->obj.unit, CLACT_VRAM_SUB, wk->printQueue, wk->font, 8, 1, HEAPID_TRAINER_CARD_SCREEN);
    TrCardBmp_Stub(wk, wk->win[TRCARD_WIN_MONEY], 1, wk->colonBuf);
    if (wk->param->appParam == 2) {
        wk->param->appParam = 0;
        wk->side = 1;
    } else {
        wk->side = 0;
    }
    wk->param->next = 0;
    wk->unk32E8 = 0;
    wk->unk31E0 = 0;
    if (wk->data->unk04_4) {
        wk->blinkMode = 0;
    } else {
        wk->blinkMode = 2;
    }
    wk->centerX = 128;
    wk->centerY = 96;
    wk->offsetX = 0;
    wk->offsetY = 0;
    TrainerCard_ResetAffine();
    wk->rankFlags = 0;
    wk->rank = 0;
    wk->scrolled = 0;
    TrainerCard_InitRank(wk);
    TrainerCard_LoadRankPalette(wk);
    TrainerCard_DrawSide(wk);
    TrainerCard_SetSideActors(wk, wk->side, wk->param->canEdit);
    wk->vblankTask = GFL_VBlankTCBAdd(TrainerCard_VBlank, wk, 1);
    wk->tcbl = GFL_TCBExMgrCreate(wk->heapId, wk->heapId, 3, 0);
    func_02042ba8(TRUE, wk->heapId);
    GFL_WipeSet(1, 3, 3, 0, 6, 1, wk->heapId);
    return TRUE;
}

// Steps the union room appearance to the next of the trainer's gender
static void TrainerCard_NextUnionView(TrainerCardData *data) {
    data->unk06++;
    if (data->unk06 > (data->gender + 1) * 8 - 1) {
        data->unk06 = data->gender * 8;
    }
}

static void TrainerCard_PlayPenSE(TrainerCardWork *wk, int count) {
    if (wk->seWait != 0) {
        wk->seWait--;
        return;
    }
    if (count > 5) {
        wk->seWait = 5;
        if (wk->brush == 0) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_08);
        } else {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_09);
        }
    }
}

BOOL TrainerCard_Main(GameProc *proc, u32 *state, void *param, void *work) {
    TrainerCardWork *wk = work;

    switch (*state) {
    case TRCARD_SEQ_FADE_IN:
        if (GFL_WipeIsFinished()) {
            *state = TRCARD_SEQ_MAIN;
        }
        break;
    case TRCARD_SEQ_MAIN:
        TrainerCard_HandleInput(wk, TrainerCard_Input(wk), state);
        TrainerCard_UpdateSignBlink(wk);
        TrainerCard_AnimateRankPalette(wk);
        if (wk->param->shownData->canEdit) {
            TrainerCard_PlayPenSE(wk, TrainerCard_DrawPen(wk->signPixels, &wk->penNew, &wk->penOld, TRUE,
                                                          wk->signPixels, wk->zoomed, wk->zoomSide));
        }
        break;
    case TRCARD_SEQ_RETURN:
        if (!func_0204c560(wk->obj.actors[TRCARD_ACTOR_RETURN])) {
            GFL_WipeSet(2, 4, 4, 0, 6, 1, wk->heapId);
            *state = TRCARD_SEQ_FADE_OUT;
        }
        break;
    case TRCARD_SEQ_EXIT:
        if (!func_0204c560(wk->obj.actors[TRCARD_ACTOR_EXIT])) {
            GFL_WipeSet(2, 4, 4, 0, 6, 1, wk->heapId);
            *state = TRCARD_SEQ_FADE_OUT;
        }
        break;
    case TRCARD_SEQ_FADE_OUT:
        if (GFL_WipeIsFinished()) {
            return TRUE;
        }
        break;
    case TRCARD_SEQ_FLIP:
        if (TrainerCard_FlipSeq(wk)) {
            *state = TRCARD_SEQ_MAIN;
        }
        break;
    case TRCARD_SEQ_ZOOM:
        if (TrainerCard_ZoomSeq(wk)) {
            *state = TRCARD_SEQ_MAIN;
        }
        break;
    case TRCARD_SEQ_GREETING:
        GFL_WipeSet(2, 4, 4, 0, 6, 1, wk->heapId);
        wk->param->next = 1;
        *state = TRCARD_SEQ_FADE_OUT;
        break;
    case TRCARD_SEQ_BADGES:
        if (!func_0204c560(wk->obj.actors[TRCARD_ACTOR_BADGES])) {
            GFL_WipeSet(2, 4, 4, 0, 6, 1, wk->heapId);
            wk->param->next = 2;
            *state = TRCARD_SEQ_FADE_OUT;
        }
        break;
    case TRCARD_SEQ_MEDALS:
        if (!func_0204c560(wk->obj.actors[TRCARD_ACTOR_RANK])) {
            GFL_WipeSet(2, 4, 4, 0, 6, 1, wk->heapId);
            wk->param->next = 4;
            *state = TRCARD_SEQ_FADE_OUT;
        }
        break;
    case TRCARD_SEQ_8:
        break;
    }
    wk->frame++;
    if (wk->frame >= 0x100) {
        wk->frame = 0;
    }
    func_0204b794();
    PMSDraw_Main(wk->pmsDraw);
    func_02021a3c(wk->printQueue);
    if (wk->tcbl != NULL) {
        GFL_TCBExMgrUpdate(wk->tcbl);
    }
    return FALSE;
}

BOOL TrainerCard_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    TrainerCardWork *wk = work;
    TrainerCardParam *cardParam = param;
    PlayerInfo *info;
    TimeSigSave *timeSig;

    if (!wk->data->unk04_5) {
        TrainerCard_PackSign(wk->signPixels, wk->data->unk74);
        sys_memcpy(wk->data->unk74, func_020091a8(getTimeSigSaveBlock(wk->param->gameData)), TRCARD_SIGN_BYTES);
    }
    if (cardParam->canEdit) {
        info = GetGameDataPlayerInfo(wk->param->gameData);
        timeSig = getTimeSigSaveBlock(wk->param->gameData);
        func_02008bf8(info, wk->data->unk06);
        func_020091f0(timeSig, wk->data->unk48);
        func_020091f8(timeSig, wk->data->unk04_4);
    }
    PMSDraw_Delete(wk->pmsDraw);
    TrainerCard_ResetAffine();
    TrainerCard_FreeMessages(wk);
    TrCardObj_Exit(&wk->obj);
    GFL_HeapFree(wk->charRes);
    GFL_HeapFree(wk->scrRes);
    GFL_HeapFree(wk->palRes);
    TrCardBmp_Exit(wk);
    TrainerCard_ReleaseBG();
    GFL_TCBRemove(wk->vblankTask);
    GFL_TCBExMgrFree(wk->tcbl);
    func_0203d564(wk->touchMode);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_TRAINER_CARD_SCREEN);
    return TRUE;
}

static void TrainerCard_LoadMessages(TrainerCardWork *wk) {
    int i;

    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_SERVER_FLOW_4, wk->heapId);
    wk->numBuf = GFL_StrBufCreate(4, wk->heapId);
    wk->nameBuf = GFL_StrBufCreate(32, wk->heapId);
    wk->tmpBuf = GFL_StrBufCreate(33, wk->heapId);
    wk->colonBuf = GFL_StrBufCreate(32, wk->heapId);
    wk->noClearBuf = GFL_StrBufCreate(64, wk->heapId);
    GFL_MsgDataLoadStrbuf(wk->msgData, 27, wk->colonBuf);
    GFL_MsgDataLoadStrbuf(wk->msgData, 28, wk->noClearBuf);
    for (i = 0; i < 3; i++) {
        wk->unk3234[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, i + 30);
    }
    for (i = 0; i < 2; i++) {
        wk->unk3240[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, i + 33);
    }
    for (i = 0; i < 27; i++) {
        wk->labels[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, i);
    }
    for (i = 0; i < 5; i++) {
        wk->formats[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, i + 60);
    }
    wk->wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
}

static void TrainerCard_FreeMessages(TrainerCardWork *wk) {
    int i;

    GFL_WordSetSystemFree(wk->wordSet);
    for (i = 0; i < 5; i++) {
        GFL_StrBufFree(wk->formats[i]);
    }
    for (i = 0; i < 27; i++) {
        GFL_StrBufFree(wk->labels[i]);
    }
    GFL_StrBufFree(wk->unk3240[1]);
    GFL_StrBufFree(wk->unk3240[0]);
    GFL_StrBufFree(wk->unk3234[2]);
    GFL_StrBufFree(wk->unk3234[1]);
    GFL_StrBufFree(wk->unk3234[0]);
    GFL_StrBufFree(wk->noClearBuf);
    GFL_StrBufFree(wk->colonBuf);
    GFL_StrBufFree(wk->tmpBuf);
    GFL_StrBufFree(wk->nameBuf);
    GFL_StrBufFree(wk->numBuf);
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->font);
}

static void TrainerCard_LoadCardPalette(TrainerCardWork *wk, int stars, u32 hasPokedex) {
    int color = 1;
    u32 file;

    if (wk->data->version == VERSION_WHITE2 || wk->data->version == VERSION_WHITE) {
        color = 0;
    } else if (wk->data->version == VERSION_BLACK2 || wk->data->version == VERSION_BLACK) {
        color = 1;
    } else {
        stars = 6;
    }
    file = sTrainerCardPalettes[stars][color];
    GFL_BGSysLoadNCLRDefault(ARCID_TRAINER_CARD_GRA, file, 4, 0, 0x200, wk->heapId);
    wk->palRes = GFL_G2DIOReadNCLR(ARCID_TRAINER_CARD_GRA, file, &wk->palData, wk->heapId);
}

static void TrainerCard_LoadUnionPalette(TrainerCardWork *wk, u32 view) {
    GFL_BGSysLoadNCLRDefault(ARCID_TRAINER_CARD_UNION_GRA, sTrainerCardUnionGra[view].palette, 4, 0x80, 0x20,
                             wk->heapId);
}

static void TrainerCard_SetupBG(void) {
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_5, FALSE };

        GFL_BGSysSetLCDConfig(&lcdConfig);
    }
    {
        BGSetup setup = { 0, 0, 0x800, 0, 1, 1, 15, 4, 0x10000, 0, 1, 0, 0 };

        GFL_BGSysCreateBG(7, &setup, 2);
        GFL_BGSysClearBG(7);
        GFL_BGSysSetBGEnabled(7, TRUE);
    }
    {
        BGSetup setups[2] = {
            { 0, 0, 0x800, 0, 1, 0, 12, 0, 0x6000, 0, 0, 0, 0 },
            { 0, 0, 0x800, 0, 1, 0, 13, 1, 0x6000, 0, 3, 0, 0 },
        };

        GFL_BGSysCreateBG(4, &setups[0], 0);
        GFL_BGSysClearBG(4);
        GFL_BGSysSetBGEnabled(4, TRUE);
        GFL_BGSysCreateBG(5, &setups[1], 0);
        GFL_BGSysClearBG(5);
        GFL_BGSysSetBGEnabled(5, TRUE);
    }
    {
        BGSetup setup = { 0, 0, 0x800, 0, 1, 1, 14, 2, 0x8000, 0, 2, 0, 0 };

        GFL_BGSysCreateBG(6, &setup, 2);
        GFL_BGSysClearBG(6);
        GFL_BGSysSetBGEnabled(6, TRUE);
    }
    {
        BGSetup setup = { 0, 0, 0x800, 0, 1, 0, 28, 0, 0x8000, 0, 2, 0, 0 };

        GFL_BGSysCreateBG(2, &setup, 0);
        GFL_BGSysClearBG(2);
        GFL_BGSysSetBGEnabled(2, FALSE);
    }
    {
        BGSetup setup = { 0, 0, 0x800, 0, 1, 0, 29, 4, 0x8000, 0, 3, 0, 0 };

        GFL_BGSysCreateBG(3, &setup, 0);
        GFL_BGSysClearBG(3);
        GFL_BGSysSetBGEnabled(3, TRUE);
    }
}

static u32 TrainerCard_GetFrontCharFile(u32 version) {
    if (version == VERSION_WHITE || version == VERSION_WHITE2) {
        return 37;
    }
    return 33;
}

static u32 TrainerCard_GetFrontScreenFile(u32 version) {
    if (version == VERSION_WHITE || version == VERSION_WHITE2) {
        return 49;
    }
    return 48;
}

static void TrainerCard_LoadGraphics(TrainerCardWork *wk) {
    TrainerCard_LoadCardPalette(wk, wk->data->stars, wk->data->unk04_2);
    if (!wk->data->unk04_7) {
        wk->charRes = GFL_G2DIOReadBGNCGR(ARCID_TRAINER_CARD_GRA, 36, FALSE, &wk->charData, wk->heapId);
        if (!wk->data->gender) {
            wk->scrRes = GFL_G2DIOReadNSCR(ARCID_TRAINER_CARD_GRA, 50, FALSE, &wk->scrData, wk->heapId);
        } else {
            wk->scrRes = GFL_G2DIOReadNSCR(ARCID_TRAINER_CARD_GRA, 51, FALSE, &wk->scrData, wk->heapId);
        }
    } else {
        wk->charRes = GFL_G2DIOReadBGNCGR(ARCID_TRAINER_CARD_UNION_GRA, sTrainerCardUnionGra[wk->data->unk06].chars,
                                          FALSE, &wk->charData, wk->heapId);
        wk->scrRes = GFL_G2DIOReadNSCR(ARCID_TRAINER_CARD_UNION_GRA, 32, FALSE, &wk->scrData, wk->heapId);
        TrainerCard_LoadUnionPalette(wk, wk->data->unk06);
    }
    TrainerCard_DrawTrainer(wk);
    GFL_BGSysLoadNCGRStatic(ARCID_TRAINER_CARD_GRA, TrainerCard_GetFrontCharFile(wk->data->version), 6, 0, 0, FALSE,
                            wk->heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_TRAINER_CARD_GRA, TrainerCard_GetFrontScreenFile(wk->data->version), 6,
                                           0, 0, FALSE, wk->heapId);
    GFL_BGSysLoadNCGRStatic(ARCID_TRAINER_CARD_GRA, 35, 5, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_TRAINER_CARD_GRA, 47, 5, 0, 0, FALSE, wk->heapId);
    GFL_BGSysLoadNCGRStatic(ARCID_TRAINER_CARD_GRA, 35, 3, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_TRAINER_CARD_GRA, 47, 3, 0, 0, FALSE, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_TRAINER_CARD_GRA, 18, 0, 0, 0x20, wk->heapId);
    TrainerCard_UnpackSign(wk->data->unk74, wk->signPixels);
    GFL_BGSysLoadNCGRStatic(getUINarcIdx(), func_0202d824(), 4, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(getUINarcIdx(), func_0202d828(), 4, 0, 0, FALSE, wk->heapId);
    GFL_BGSysSetScrPaletteNo(4, 0, 21, 32, 3, 12);
    GFL_BGSysLoadScr(4);
}

static void TrainerCard_ReleaseBG(void) {
    GFL_BGSysSetBGEnabledA(0x1f, FALSE);
    GFL_BGSysSetBGEnabledB(0x1f, FALSE);
    GFL_BGSysMoveBG(5, BG_MOVE_SET_X, 0);
    GFL_BGSysMoveBG(5, BG_MOVE_SET_Y, 0);
    GFL_BGSysMoveBG(3, BG_MOVE_SET_X, 0);
    GFL_BGSysMoveBG(3, BG_MOVE_SET_Y, 0);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysFree();
}

// The back's screen, with the pen or the eraser chosen, in the version's color
static void TrainerCard_LoadBackScreen(TrainerCardWork *wk, u32 version) {
    u32 files[2][2] = { { 46, 43 }, { 45, 44 } };

    // The version becomes the screen's color
    if (version == VERSION_WHITE || version == VERSION_WHITE2) {
        version = 0;
    } else {
        version = 1;
    }
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_TRAINER_CARD_GRA, files[wk->data->unk04_4][version], 6, 0, 0, FALSE,
                                           wk->heapId);
}

// The tables below are declared here, and in this order, for the order of the file's .rodata: MWCC's sort of them
// by size depends on where each one is declared

static const TouchRect sTrainerCardListRect[] = {
    { 16, 80, 16, 232 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sTrainerCardArrowRects[] = {
    { 16, 32, 232, 248 },
    { 72, 88, 232, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The buttons of the player's card, and of its zoomed signature
static const TouchRect sTrainerCardRects[] = {
    { 168, 192, 228, 252 }, { 168, 192, 204, 228 },      { 168, 192, 100, 164 }, { 168, 192, 4, 28 },
    { 168, 192, 36, 60 },   { 168, 192, 68, 92 },        { 168, 192, 180, 204 }, { 48, 64, 24, 160 },
    { 64, 80, 24, 160 },    { 120, 151, 16, 239 },       { 120, 151, 16, 239 },  { 120, 151, 16, 239 },
    { 80, 104, 224, 248 },  { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sTrainerCardZoomedRects[] = {
    { 168, 192, 228, 252 }, { 168, 192, 204, 228 },      { 168, 192, 100, 164 }, { 168, 192, 4, 28 },
    { 168, 192, 36, 60 },   { 168, 192, 68, 92 },        { 168, 192, 180, 204 }, { 48, 64, 24, 160 },
    { 64, 80, 24, 160 },    { 120, 151, 16, 239 },       { 0, 168, 231, 255 },   { 0, 168, 0, 24 },
    { 0, 168, 0, 24 },      { TOUCH_RECT_END, 0, 0, 0 },
};

// The buttons of another player's card
static const TouchRect sTrainerCardOtherRects[] = {
    { 168, 192, 228, 252 }, { 168, 192, 228, 252 },      { 168, 192, 100, 164 },
    { 80, 104, 224, 248 },  { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sTrainerCardZoomedSignRelease[] = {
    { 8, 168, 16, 240 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sTrainerCardSignRect[] = {
    { 88, 152, 32, 225 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sTrainerCardZoomedSignRect[] = {
    { 24, 152, 32, 224 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sTrainerCardSignRelease[] = {
    { 72, 168, 16, 240 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The pen and the eraser, normally and on the zoomed signature: 1 draws a pixel and 2 erases it
static const u8 sTrainerCardBrushes[2][2][16] = {
    { { 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0 }, { 0, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 2, 0 } },
    { { 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0 } },
};

// Which of the side's actors show on the front, on the back and while the card moves, for a card that can't be
// edited and for one that can
static const u8 sTrainerCardSideActors[2][3][6] = {
    { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 1, 1, 0 }, { 0, 0, 0, 0, 0, 0 } },
    { { 1, 1, 1, 0, 0, 0 }, { 0, 0, 0, 1, 1, 0 }, { 0, 0, 0, 0, 0, 0 } },
};

// The up and down scroll arrows at rest, held and greyed out
static const TrainerCardArrowAnime sTrainerCardArrowAnimes[2][3] = {
    { { 26, 0 }, { 34, 2 }, { 36, 0 } },
    { { 27, 0 }, { 35, 2 }, { 37, 0 } },
};

static void TrainerCard_LoadSideScreen(TrainerCardWork *wk) {
    if (wk->side == 0) {
        loadBGScrToVramByNarcNoReserveNegAlign(
            ARCID_TRAINER_CARD_GRA, TrainerCard_GetFrontScreenFile(wk->data->version), 6, 0, 0, FALSE, wk->heapId);
        GFL_BGSysClearScr(7);
    } else {
        TrainerCard_LoadBackScreen(wk, wk->data->version);
        TrainerCard_ClearSignArea(wk);
        GFL_BGSysClearScr(7);
    }
}

static void TrainerCard_DrawSide(TrainerCardWork *wk) {
    TrainerCard_LoadSideScreen(wk);
    TrCardBmp_DrawFront(wk, wk->win, wk->data);
    PMSDraw_Main(wk->pmsDraw);
    if (wk->side == 0) {
        TrCardBmp_DrawList(wk, wk->scroll, FALSE, 0);
        TrainerCard_DrawTrainer(wk);
    } else {
        TrCardBmp_DrawBack(wk, wk->win, wk->data);
        TrainerCard_LoadSign(7, wk->signPixels, wk->blinkMode);
    }
    TrainerCard_SetButtons(wk, wk->side);
    TrCardBmp_Flush(wk, wk->win);
    TrainerCard_SetSideActors(wk, 0, wk->param->canEdit);
}

// The buttons of a card that its owner can edit
static void TrainerCard_SetButtonsEdit(TrainerCardWork *wk, int side) {
    if (side == 0) {
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_RETURN, 1, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_ZOOM, 18, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BRUSH, 6, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BADGES, 16, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_SHORTCUT,
                           GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_TRAINER_CARD_FRONT) + 6, TRUE);
        return;
    }
    if (wk->zoomed == 0) {
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_RETURN, 1, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_EXIT, 0, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_FLIP, 22, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_ZOOM, 18, wk->data->unk04_4 ^ 1);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BRUSH, 8 - wk->brush * 2, wk->data->unk04_4 ^ 1);
        TrainerCard_SetPenButton(wk, wk->data->unk04_4, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_SHORTCUT,
                           GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_TRAINER_CARD_BACK) + 6, TRUE);
    } else {
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_RETURN, 15, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_EXIT, 14, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_ZOOM, 20, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BRUSH, 8 - wk->brush * 2, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_FLIP, 22, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BADGES, 10, FALSE);
    }
}

// The buttons of the player's card where it can't be edited
static void TrainerCard_SetButtonsLocked(TrainerCardWork *wk, int side) {
    if (side == 0) {
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_RETURN, 1, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_ZOOM, 18, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BRUSH, 6, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BADGES, 16, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_SHORTCUT,
                           GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_TRAINER_CARD_FRONT) + 6, TRUE);
    } else {
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_RETURN, 1, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_EXIT, 0, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_ZOOM, 18, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_FLIP, 22, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BRUSH, 8 - wk->brush * 2, FALSE);
        TrainerCard_SetPenButton(wk, wk->data->unk04_4, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_SHORTCUT,
                           GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_TRAINER_CARD_BACK) + 6, TRUE);
    }
}

// The buttons of another player's card
static void TrainerCard_SetButtonsOther(TrainerCardWork *wk, int side) {
    if (side == 0) {
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_RETURN, 1, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_EXIT, 14, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BADGES, 16, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BRUSH, 6, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_SHORTCUT, 6, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_ZOOM, 0, FALSE);
    } else {
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_RETURN, 1, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_EXIT, 0, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_ZOOM, 18, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_FLIP, 22, TRUE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BRUSH, 8 - wk->brush * 2, FALSE);
        TrainerCard_SetPenButton(wk, wk->data->unk04_4, FALSE);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_SHORTCUT, 6, FALSE);
    }
}

static void TrainerCard_SetButtons(TrainerCardWork *wk, int side) {
    if (wk->data->unk04_7 == TRUE) {
        TrainerCard_SetButtonsOther(wk, side);
    } else if (wk->param->canEdit == FALSE) {
        TrainerCard_SetButtonsLocked(wk, side);
    } else {
        TrainerCard_SetButtonsEdit(wk, side);
    }
}

// Flips the card over: it narrows to a line, changes side and widens again. TRUE when done
static BOOL TrainerCard_FlipSeq(TrainerCardWork *wk) {
    BOOL done = FALSE;

    switch (wk->subSeq) {
    case 0:
        wk->flipAngle = 0;
        wk->scaleX = TRCARD_SCALE;
        wk->scaleY = TRCARD_SCALE;
        GFL_SndSEPlay(SEQ_SE_SW_CARD_06);
        if (wk->side == 0) {
            PMSDraw_SetIconVisible(wk->pmsDraw, 0, FALSE);
        }
        TrainerCard_SetSideActors(wk, 2, wk->param->canEdit);
        wk->subSeq++;
        break;
    case 1:
        wk->scaleX = FX_Mul(FX_CosIdx(wk->flipAngle), TRCARD_SCALE);
        if (wk->scaleX <= 0x80) {
            wk->scaleX = 0x80;
            wk->subSeq++;
        }
        wk->flipAngle += 0x400;
        if (wk->flipAngle >= 0x4000) {
            wk->flipAngle = 0x4000;
        }
        break;
    case 2:
        wk->side ^= 1;
        TrainerCard_LoadSideScreen(wk);
        TrainerCard_SetButtons(wk, wk->side);
        wk->subSeq++;
        break;
    case 3:
        if (wk->side == 0) {
            TrainerCard_DrawTrainer(wk);
        } else {
            TrainerCard_LoadSign(7, wk->signPixels, wk->blinkMode);
        }
        wk->subSeq++;
        break;
    case 4:
        TrCardBmp_Flush(wk, wk->win);
        wk->subSeq++;
        break;
    case 5:
        wk->flipAngle -= 0x400;
        if (wk->flipAngle <= 0) {
            wk->flipAngle = 0;
        }
        wk->scaleX = FX_Mul(FX_CosIdx(wk->flipAngle), TRCARD_SCALE);
        if (wk->scaleX <= 0x80) {
            wk->scaleX = 0x80;
        }
        if (wk->flipAngle == 0) {
            wk->scaleX = FX32_ONE;
            wk->scaleY = FX32_ONE;
            done = TRUE;
            if (wk->side == 0) {
                PMSDraw_SetVisible(wk->pmsDraw, 0, TRUE);
                PMSDraw_SetIconVisible(wk->pmsDraw, 0, TRUE);
            }
            TrainerCard_SetSideActors(wk, wk->side, wk->param->canEdit);
        }
        break;
    }
    wk->affineDirty = TRUE;
    return done;
}

// Darkens the strip at the edge of the zoomed signature that the other half is reached through: at the right for the
// left half (0), at the left for the right half (1), or nowhere (-1)
static void TrainerCard_SetSignWindow(int side) {
    if (side < 0) {
        G2S_BlendNone();
        GXS_SetVisibleWnd(0);
        return;
    }
    if (side == 0) {
        gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 0xc, -8);
        G2S_SetWnd0Position(224, 0, 0, 168);
        G2S_SetWnd0InsidePlane(0x1c, TRUE);
        G2S_SetWndOutsidePlane(0x1f, FALSE);
        GXS_SetVisibleWnd(1);
    } else if (side == 1) {
        gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 0xc, -8);
        G2S_SetWnd0Position(0, 0, 32, 168);
        G2S_SetWnd0InsidePlane(0x1c, TRUE);
        G2S_SetWndOutsidePlane(0x1f, FALSE);
        GXS_SetVisibleWnd(1);
    }
}

// Zooms in to a half of the signature, out again, and slides between the halves. TRUE when a step that ends one is
// done
static BOOL TrainerCard_ZoomSeq(TrainerCardWork *wk) {
    switch (wk->subSeq) {
    case 0:
        if (wk->zoomSide == 0) {
            wk->centerX = 32;
            wk->centerY = 152;
        } else {
            wk->centerX = 224;
            wk->centerY = 152;
        }
        wk->scaleX = TRCARD_SCALE;
        wk->scaleY = TRCARD_SCALE;
        wk->subSeq = 1;
        TrainerCard_SetSideActors(wk, 2, wk->param->canEdit);
        break;
    case 1:
        wk->scaleX += 0x100;
        wk->scaleY += 0x100;
        if (wk->scaleX >= 2 * FX32_ONE) {
            wk->scaleX = 2 * FX32_ONE;
            wk->scaleY = 2 * FX32_ONE;
            wk->subSeq = 2;
        }
        break;
    case 2:
        wk->zoomed = 1;
        TrainerCard_SetButtons(wk, wk->side);
        TrainerCard_SetSignWindow(wk->zoomSide);
        return TRUE;
    case 3:
        wk->subSeq = 4;
        break;
    case 4:
        wk->scaleX -= 0x100;
        wk->scaleY -= 0x100;
        if (wk->scaleX <= FX32_ONE) {
            wk->scaleX = FX32_ONE;
            wk->scaleY = FX32_ONE;
            wk->subSeq = 5;
        }
        break;
    case 5:
        wk->centerX = 128;
        wk->centerY = 96;
        wk->zoomed = 0;
        TrainerCard_SetButtons(wk, wk->side);
        TrainerCard_SetSignWindow(-1);
        TrainerCard_SetSideActors(wk, 1, wk->param->canEdit);
        return TRUE;
    case 6:
        wk->subSeq = 7;
        break;
    case 7:
        wk->centerX -= 8;
        if (wk->centerX < 32) {
            wk->centerX = 32;
            wk->subSeq = 8;
        }
        break;
    case 8:
        wk->zoomSide ^= 1;
        TrainerCard_SetSignWindow(wk->zoomSide);
        return TRUE;
    case 12:
        wk->centerX = 32;
        wk->subSeq = 8;
        break;
    case 9:
        wk->subSeq = 10;
        break;
    case 10:
        wk->centerX += 8;
        if (wk->centerX > 224) {
            wk->centerX = 224;
            wk->subSeq = 11;
        }
        break;
    case 11:
        wk->zoomSide ^= 1;
        TrainerCard_SetSignWindow(wk->zoomSide);
        return TRUE;
    case 13:
        wk->centerX = 224;
        wk->subSeq = 11;
        break;
    }
    wk->affineDirty = TRUE;
    return FALSE;
}

static void TrainerCard_UpdateAffine(TrainerCardWork *wk) {
    MtxFx22 mtx;

    mtx._00 = FX_Inv(wk->scaleX);
    mtx._01 = 0;
    mtx._10 = 0;
    mtx._11 = FX_Inv(wk->scaleY);
    G2S_SetBG2Affine(&mtx, wk->centerX, wk->centerY, wk->offsetX, wk->offsetY);
    G2S_SetBG3Affine(&mtx, wk->centerX, wk->centerY, wk->offsetX, wk->offsetY);
    wk->affineDirty = FALSE;
}

// Switches between the keys and the touch screen when the other is used
static BOOL TrainerCard_CheckInputMode(TrainerCardWork *wk) {
    if (wk->touchMode == TRUE) {
        if (func_0203da2c()) {
            return FALSE;
        }
        if (GCTX_HIDGetHeldKeys()) {
            wk->touchMode = FALSE;
            return TRUE;
        }
    } else {
        if (GCTX_HIDGetHeldKeys()) {
            return FALSE;
        }
        if (func_0203da2c()) {
            wk->touchMode = TRUE;
            return FALSE;
        }
    }
    return FALSE;
}

static int TrainerCard_KeyInput(TrainerCardWork *wk) {
    u32 keys = GCTX_HIDGetPressedKeys();

    if (keys & PAD_BUTTON_B) {
        if (wk->zoomed == 0) {
            TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_RETURN, 9, TRUE);
            GFL_SndSEPlay(SEQ_SE_SW_CARD_04);
            func_0203d564(FALSE);
            return TRCARD_INPUT_RETURN;
        }
    } else if (keys & PAD_BUTTON_X) {
        if (wk->zoomed == 0 && !wk->data->unk04_7) {
            TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_EXIT, 8, TRUE);
            GFL_SndSEPlay(SEQ_SE_SW_CARD_05);
            return TRCARD_INPUT_EXIT;
        }
    } else if (keys & (PAD_KEY_LEFT | PAD_KEY_RIGHT)) {
        if (wk->zoomed == 0) {
            TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_FLIP, 23, TRUE);
            return TRCARD_INPUT_FLIP;
        }
    } else if (keys & PAD_BUTTON_Y) {
        if (!wk->data->unk04_7) {
            TrainerCard_ToggleShortcut(wk);
        }
    } else if (keys & PAD_BUTTON_A) {
        if (!wk->data->unk04_7 && wk->side == 0) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_01);
            TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BADGES, 17, TRUE);
            return TRCARD_INPUT_BADGES;
        }
    } else if (keys & PAD_BUTTON_L) {
        if (!wk->data->unk04_7 && wk->zoomed == 1 && wk->zoomSide == 1) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_02);
            wk->subSeq = 12;
            return TRCARD_INPUT_ZOOM;
        }
    } else if (keys & PAD_BUTTON_R) {
        if (!wk->data->unk04_7 && wk->zoomed == 1 && wk->zoomSide == 0) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_02);
            wk->subSeq = 13;
            return TRCARD_INPUT_ZOOM;
        }
    }
    return TRCARD_INPUT_NONE;
}

// Registers the side shown with the Y button's shortcuts, or takes it off them
static void TrainerCard_ToggleShortcut(TrainerCardWork *wk) {
    BOOL registered;

    GFL_SndSEPlay(SEQ_SE_SW_CARD_03);
    if (wk->side == 0) {
        registered = GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_TRAINER_CARD_FRONT) ^ 1;
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_SHORTCUT, registered + 6, TRUE);
        GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_TRAINER_CARD_FRONT, registered);
    } else {
        registered = GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_TRAINER_CARD_BACK) ^ 1;
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_SHORTCUT, registered + 6, TRUE);
        GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_TRAINER_CARD_BACK, registered);
    }
}

// The button that switches between the pen and the eraser, which shares its actor with the badge case's on the front
static void TrainerCard_SetPenButton(TrainerCardWork *wk, int eraser, int visible) {
    if (eraser == 0) {
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BADGES, 10, visible);
    } else {
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BADGES, 12, visible);
    }
}

static void TrainerCard_PressPenButton(TrainerCardWork *wk, int eraser) {
    if (eraser == 0) {
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BADGES, 11, TRUE);
        func_0204c124(wk->obj.actors[TRCARD_ACTOR_ZOOM], FALSE);
        func_0204c124(wk->obj.actors[TRCARD_ACTOR_BRUSH], FALSE);
    } else {
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BADGES, 13, TRUE);
        func_0204c124(wk->obj.actors[TRCARD_ACTOR_ZOOM], TRUE);
        func_0204c124(wk->obj.actors[TRCARD_ACTOR_BRUSH], TRUE);
    }
}

static void TrainerCard_SetSignBlink(TrainerCardWork *wk, int eraser) {
    if (eraser) {
        wk->blinkMode = 0;
        wk->blinkCount = 0;
    } else {
        wk->blinkMode = 2;
    }
    TrainerCard_SetSignScreen(7, wk->blinkMode);
}

static BOOL TrainerCard_IsLocked(TrainerCardWork *wk, int button) {
    if (wk->data->unk04_5 && button >= 3 && button <= 9) {
        return TRUE;
    }
    return FALSE;
}

static int TrainerCard_TouchCard(TrainerCardWork *wk, int button) {
    if (TrainerCard_IsLocked(wk, button) == TRUE) {
        return TRCARD_INPUT_NONE;
    }
    switch (button) {
    case 0:
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_RETURN, 9, TRUE);
        GFL_SndSEPlay(SEQ_SE_SW_CARD_04);
        func_0203d564(TRUE);
        return TRCARD_INPUT_RETURN;
    case 1:
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_EXIT, 8, TRUE);
        GFL_SndSEPlay(SEQ_SE_SW_CARD_05);
        return TRCARD_INPUT_EXIT;
    case 2:
        func_0203da84(&wk->touchX, &wk->touchY);
        TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_FLIP, 23, TRUE);
        return TRCARD_INPUT_FLIP;
    case 3:
        if (wk->side != 0) {
            if (wk->param->shownData->canEdit) {
                TrainerCard_PressPenButton(wk, wk->data->unk04_4);
                wk->data->unk04_4 ^= 1;
                TrainerCard_LoadBackScreen(wk, wk->data->version);
                TrainerCard_SetSignBlink(wk, wk->data->unk04_4);
                GFL_SndSEPlay(SEQ_SE_SW_CARD_02);
            }
        } else {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_01);
            TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BADGES, 17, TRUE);
            return TRCARD_INPUT_BADGES;
        }
        break;
    case 4:
        if (wk->side != 0 && !wk->data->unk04_4 && wk->param->shownData->canEdit) {
            if (wk->zoomed == 0) {
                TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_ZOOM, 19, TRUE);
                GFL_SndSEPlay(SEQ_SE_SW_CARD_10);
                wk->subSeq = 0;
            } else {
                TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_ZOOM, 21, TRUE);
                GFL_SndSEPlay(SEQ_SE_SW_CARD_11);
                wk->subSeq = 3;
            }
            return TRCARD_INPUT_ZOOM;
        }
        break;
    case 5:
        if (wk->side != 0 && wk->param->shownData->canEdit && !wk->data->unk04_4) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_02);
            TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BRUSH, 9 - wk->brush * 2, TRUE);
            wk->brush ^= 1;
        }
        break;
    case 6:
        TrainerCard_ToggleShortcut(wk);
        break;
    case 7:
        if (wk->side == 0) {
            return TRCARD_INPUT_UNION_VIEW;
        }
        break;
    case 8:
        if (wk->side == 0) {
            return TRCARD_INPUT_PERSONALITY;
        }
        break;
    case 9:
        if (wk->side == 0) {
            return TRCARD_INPUT_GREETING;
        }
        break;
    case 12:
        if ((wk->rankFlags & 1) && wk->side == 0) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_01);
            TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_RANK, sTrainerCardRankAnimes[wk->rank], TRUE);
            return TRCARD_INPUT_MEDALS;
        }
        break;
    case 10:
    case 11:
        break;
    }
    return TRCARD_INPUT_NONE;
}

// The buttons while the signature is zoomed
static int TrainerCard_TouchSign(TrainerCardWork *wk, int button) {
    switch (button) {
    case 4:
        if (wk->side != 0) {
            TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_ZOOM, 21, TRUE);
            GFL_SndSEPlay(SEQ_SE_SW_CARD_11);
            wk->subSeq = 3;
            return TRCARD_INPUT_ZOOM;
        }
        break;
    case 5:
        if (wk->side != 0) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_02);
            TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_BRUSH, 9 - wk->brush * 2, TRUE);
            wk->brush ^= 1;
        }
        break;
    case 6:
        TrainerCard_ToggleShortcut(wk);
        break;
    case 10:
        if (wk->zoomSide == 0) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_02);
            wk->subSeq = 9;
            return TRCARD_INPUT_ZOOM;
        }
        break;
    case 11:
        if (wk->zoomSide == 1) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_02);
            wk->subSeq = 6;
            return TRCARD_INPUT_ZOOM;
        }
        break;
    case 0:
    case 1:
    case 2:
    case 3:
    case 7:
    case 8:
    case 9:
        break;
    }
    return TRCARD_INPUT_NONE;
}

// What of the back's list is touched: 0 the list, 1 the up arrow, 2 the down arrow, or -1
static int TrainerCard_CheckScrollTouch(TrainerCardWork *wk, BOOL held) {
    int result = -1;
    int hit;

    if (!held) {
        if (func_0203da0c(sTrainerCardListRect) == 0) {
            result = 0;
        } else {
            hit = func_0203da0c(sTrainerCardArrowRects);
            if (hit == 0) {
                result = 1;
            } else if (hit == 1) {
                result = 2;
            }
        }
    } else {
        if (func_0203d9c8(sTrainerCardListRect) == 0) {
            result = 0;
        } else {
            hit = func_0203d9c8(sTrainerCardArrowRects);
            if (hit == 0) {
                result = 1;
            } else if (hit == 1) {
                result = 2;
            }
        }
    }
    return result;
}

static void TrainerCard_ScrollRelease(TrainerCardWork *wk) {
    int hit = TrainerCard_CheckScrollTouch(wk, TRUE);

    if (wk->side != 0 && hit == -1 && wk->scrolled == 1) {
        TrCardBmp_DrawList(wk, wk->scroll, TRUE, 0);
        wk->scrolled = 0;
        wk->unk331C = 0;
        wk->scrollTouch = 0;
        TrainerCard_SetScrollArrows(wk, 2);
    }
}

static void TrainerCard_ScrollTouch(TrainerCardWork *wk) {
    int trg = TrainerCard_CheckScrollTouch(wk, FALSE);
    int cont = TrainerCard_CheckScrollTouch(wk, TRUE);
    u32 x;
    u32 y;
    int min;
    int up;

    if (trg != -1) {
        wk->scrollTouch = 1;
    }
    if (wk->scrollTouch == 0 || cont == -1) {
        return;
    }
    if (trg == 0) {
        if (func_0203da48()) {
            func_0203dac8(&x, &y);
            wk->dragY = y;
            wk->dragScroll = wk->scroll;
        }
    } else if (trg == 1) {
        wk->dragY = 16;
        wk->dragScroll = wk->scroll;
    } else if (trg == 2) {
        wk->dragY = 80;
        wk->dragScroll = wk->scroll;
    }
    if (cont == 0) {
        if (func_0203da2c()) {
            func_0203da84(&x, &y);
            wk->scroll = wk->dragScroll - (wk->dragY - y);
        }
    } else {
        if (cont == 1) {
            wk->scroll += 8;
            TrainerCard_SetScrollArrows(wk, 1);
        } else if (cont == 2) {
            wk->scroll -= 8;
            TrainerCard_SetScrollArrows(wk, 0);
        }
        func_0203da84(&x, &y);
        wk->dragY = y;
        wk->dragScroll = wk->scroll;
    }
    min = -(wk->rowCount - 4) * TRCARD_ROW_HEIGHT;
    if (wk->scroll < min) {
        wk->scroll = min;
    } else if (wk->scroll > 0) {
        wk->scroll = 0;
    }
    if (wk->oldScroll != wk->scroll) {
        wk->scrolled = 1;
        TrCardBmp_DrawList(wk, wk->scroll, TRUE, 1);
        GFL_SndSEPlay(SEQ_SE_SW_CARD_07);
        if (cont == 0) {
            up = 0;
            if (wk->scroll < wk->oldScroll) {
                up = 1;
            }
        } else if (cont == 1 || cont == 2) {
            up = 0;
            if (wk->scroll < wk->oldScroll) {
                up = 1;
            }
        }
        TrainerCard_SetScrollArrows(wk, up);
        wk->rankAngle = 0x8000;
    } else {
        TrainerCard_SetScrollArrows(wk, 3);
        wk->rankAngle = 0x8000;
    }
    wk->oldScroll = wk->scroll;
}

static int TrainerCard_TouchInput(TrainerCardWork *wk) {
    int hit;
    int input;

    if (!wk->data->unk04_7) {
        if (wk->zoomed == 0) {
            hit = func_0203da0c(sTrainerCardRects);
        } else {
            hit = func_0203da0c(sTrainerCardZoomedRects);
        }
    } else {
        hit = func_0203da0c(sTrainerCardOtherRects);
        if (hit == 3) {
            hit = 12;
        }
    }
    if (wk->zoomed == 0) {
        input = TrainerCard_TouchCard(wk, hit);
    } else {
        input = TrainerCard_TouchSign(wk, hit);
    }
    if (input == TRCARD_INPUT_NONE) {
        if (wk->side != 0) {
            if (wk->zoomed == 0) {
                TrainerCard_ScrollTouch(wk);
                TrainerCard_ReadPen(wk, 0);
            } else {
                TrainerCard_ReadPen(wk, 1);
            }
        }
        return TRCARD_INPUT_NONE;
    }
    return input;
}

static int TrainerCard_Input(TrainerCardWork *wk) {
    TrainerCard_CheckInputMode(wk);
    TrainerCard_ScrollRelease(wk);
    TrainerCard_CheckPenRelease(wk);
    if (wk->touchMode == TRUE) {
        return TrainerCard_TouchInput(wk);
    }
    return TrainerCard_KeyInput(wk);
}

static void TrainerCard_HandleInput(TrainerCardWork *wk, int input, u32 *state) {
    switch (input) {
    case TRCARD_INPUT_FLIP:
        wk->subSeq = 0;
        *state = TRCARD_SEQ_FLIP;
        break;
    case TRCARD_INPUT_ZOOM:
        if (wk->param->shownData->canEdit) {
            *state = TRCARD_SEQ_ZOOM;
        }
        break;
    case TRCARD_INPUT_GREETING:
        if (wk->param->shownData->canEdit) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_02);
            wk->subSeq = 0;
            *state = TRCARD_SEQ_GREETING;
        }
        break;
    case TRCARD_INPUT_RETURN:
        *state = TRCARD_SEQ_RETURN;
        wk->param->next = 0;
        wk->param->result = 0;
        break;
    case TRCARD_INPUT_EXIT:
        *state = TRCARD_SEQ_EXIT;
        wk->param->next = 0;
        wk->param->result = 1;
        break;
    case TRCARD_INPUT_UNION_VIEW:
        if (wk->param->shownData->canEdit) {
            func_020091dc(getTimeSigSaveBlock(wk->param->gameData));
            GFL_SndSEPlay(SEQ_SE_SW_CARD_02);
            TrainerCard_NextUnionView(wk->data);
            TrCardBmp_DrawUnionView(wk, wk->data->unk06, TRUE);
        }
        break;
    case TRCARD_INPUT_PERSONALITY:
        if (wk->param->shownData->canEdit) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_02);
            wk->data->unk48++;
            if (wk->data->unk48 > 24) {
                wk->data->unk48 = 0;
            }
            TrCardBmp_DrawPersonality(wk, wk->data->unk48, TRUE);
        }
        break;
    case TRCARD_INPUT_BADGES:
        wk->subSeq = 0;
        *state = TRCARD_SEQ_BADGES;
        break;
    case TRCARD_INPUT_MEDALS:
        wk->subSeq = 0;
        *state = TRCARD_SEQ_MEDALS;
        break;
    }
}

static void TrainerCard_VBlank(TCB *tcb, void *data) {
    TrainerCardWork *wk = data;
    int y;

    TrainerCard_AnimatePalette(wk);
    y = -(wk->frame / 2);
    GFL_BGSysMoveBG(5, BG_MOVE_SET_X, y);
    GFL_BGSysMoveBG(5, BG_MOVE_SET_Y, y);
    GFL_BGSysMoveBG(3, BG_MOVE_SET_X, y);
    GFL_BGSysMoveBG(3, BG_MOVE_SET_Y, y);
    if (wk->affineDirty) {
        TrainerCard_UpdateAffine(wk);
    }
    GFL_BGSysUpdate();
    func_0204b7c8();
}

static void TrainerCard_ResetAffine(void) {
    MtxFx22 mtx;
    fx32 x = FX_Inv(FX32_ONE);
    fx32 y = FX_Inv(FX32_ONE);

    mtx._00 = x;
    mtx._01 = 0;
    mtx._10 = 0;
    mtx._11 = y;
    OS_WaitVBlankIntr();
    G2S_SetBG2Affine(&mtx, 128, 96, 0, 0);
    G2S_SetBG3Affine(&mtx, 128, 96, 0, 0);
    OS_WaitVBlankIntr();
    G2_SetBG3Affine(&mtx, 128, 0, 0, 0);
}

// The trainer's picture on the front
static void TrainerCard_DrawTrainer(TrainerCardWork *wk) {
    GFL_BGSysLoadChar(7, wk->charData->rawData, wk->charData->size, 0);
    GFL_BGSysLoadScrArea(7, 21, 2, 10, 11, wk->scrData->rawData, 21, 2, wk->scrData->width / 8,
                         wk->scrData->height / 8);
    GFL_BGSysLoadScr(7);
}

static void TrainerCard_ClearSignArea(TrainerCardWork *wk) {
    GFL_BGSysFillScrArea(7, 0, 20, 6, 6, 9, 16);
}

// The signature of the save, a bit for each pixel, as 8-bit characters
static void TrainerCard_UnpackSign(const u8 *src, u8 *dest) {
    int i;

    for (i = 0; i < TRCARD_SIGN_PIXELS; i++) {
        u8 row = (i / 8) % 8;
        u8 bit = i % 8;

        if ((src[(i / 64) * 8 + row] >> bit) & 1) {
            dest[i] = 7;
        }
    }
}

static void TrainerCard_PackSign(const u8 *src, u8 *dest) {
    int i;
    int bit;
    int byte;

    for (i = 0; i < TRCARD_SIGN_BYTES; i++) {
        byte = 0;
        for (bit = 0; bit < 8; bit++) {
            byte |= (src[i * 8 + bit] & 1) << bit;
        }
        dest[i] = byte;
    }
}

// The screen of the signature's 24x8 characters: all of it (2), or only one of the two frames of the eraser's
// blinking (0 and 1)
static void TrainerCard_SetSignScreen(u32 bg, int mode) {
    u16 *screen = GFL_BGSysIsScrHeapExists(bg);
    u8 x;
    u8 y;
    u16 tile;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 24; x++) {
            screen[(y + 11) * 32 + x + 4] = 0;
        }
    }
    tile = 0;
    switch (mode) {
    case 2:
        for (y = 0; y < 8; y++) {
            for (x = 0; x < 24; x++) {
                tile++;
                screen[(y + 11) * 32 + x + 4] = tile;
            }
        }
        break;
    case 0:
        for (y = 0; y < 8; y++) {
            for (x = 0; x < 12; x++) {
                tile++;
                screen[(y + 11) * 32 + x + 10] = tile;
            }
            tile += 12;
        }
        break;
    case 1:
        tile += 12;
        for (y = 0; y < 8; y++) {
            for (x = 0; x < 12; x++) {
                tile++;
                screen[(y + 11) * 32 + x + 10] = tile;
            }
            tile += 12;
        }
        break;
    }
    GFL_BGSysQueueScrLoad(bg);
}

static void TrainerCard_LoadSign(u32 bg, const u8 *pixels, u8 mode) {
    GFL_BGSysLoadChar(bg, pixels, TRCARD_SIGN_PIXELS, 1);
    TrainerCard_SetSignScreen(bg, mode);
}

static void TrainerCard_UpdateSignBlink(TrainerCardWork *wk) {
    if (wk->side != 0 && wk->data->unk04_4) {
        wk->blinkCount++;
        if (wk->blinkCount > 20) {
            wk->blinkMode ^= 1;
            TrainerCard_SetSignScreen(7, wk->blinkMode);
            wk->blinkCount = 0;
        }
    }
}

static void TrainerCard_AnimatePalette(TrainerCardWork *wk) {
    u16 *colors = wk->palData->rawData;

    wk->palFrame++;
    if (wk->palFrame > 8) {
        wk->palFrame = 0;
    }
    if (wk->palFrame == 0) {
        gfxUploadStdPaletteBGB(colors + 3, 6, 4);
    } else if (wk->palFrame == 4) {
        gfxUploadStdPaletteBGB(colors + 5, 6, 4);
    }
}

// Ends a stroke when the pen leaves the signature's surroundings
static void TrainerCard_CheckPenRelease(TrainerCardWork *wk) {
    if ((wk->side != 0 && wk->zoomed == 0 && func_0203d9c8(sTrainerCardSignRelease) != 0) ||
        (wk->zoomed == 1 && func_0203d9c8(sTrainerCardZoomedSignRelease) != 0)) {
        wk->penTouch = 0;
    }
}

static void TrainerCard_ReadPen(TrainerCardWork *wk, int zoomed) {
    u32 x;
    u32 y;

    if (wk->data->unk04_5 || wk->data->unk04_4 == TRUE) {
        return;
    }
    if (func_0203da84(&x, &y)) {
        if (zoomed == 0) {
            if (func_0203da0c(sTrainerCardSignRect) == 0) {
                wk->penTouch = 1;
            }
            if (wk->penTouch != 0 && func_0203dadc(sTrainerCardSignRect, x, y) == 0) {
                wk->pen.x = x;
                wk->pen.y = y;
                wk->pen.brush = wk->brush;
                wk->pen.on = 1;
                if (x < 128) {
                    wk->zoomSide = 0;
                } else {
                    wk->zoomSide = 1;
                }
            } else {
                wk->pen.on = 0;
            }
        } else {
            if (func_0203da0c(sTrainerCardZoomedSignRect) == 0) {
                wk->penTouch = 1;
            }
            if (wk->penTouch != 0 && func_0203dadc(sTrainerCardZoomedSignRect, x, y) == 0) {
                wk->pen.x = (x - 32 + wk->zoomSide * 192) / 2 + 32;
                wk->pen.y = (y - 24) / 2 + 88;
                wk->pen.brush = wk->brush;
                wk->pen.on = 1;
            } else {
                wk->pen.on = 0;
            }
        }
    } else {
        wk->pen.on = 0;
    }
    wk->penNew.x = wk->pen.x;
    wk->penNew.y = wk->pen.y;
    wk->penNew.brush = wk->pen.brush;
    wk->penNew.on = wk->pen.on;
}

// Stamps a 4x4 brush on the signature's characters. On the zoomed signature only the half shown is drawn on
static void TrainerCard_DrawBrush(u8 *pixels, const u8 *brush, int x, int y, int width, int height, int zoomed,
                                  int zoomSide) {
    int px;
    int py;
    int i;
    int j;
    int n = 0;
    int offset;

    for (j = 0; j < 4; j++) {
        py = y + j;
        for (i = 0; i < 4; i++) {
            px = x + i;
            if (px < 0 || py < 0) {
                n++;
                continue;
            }
            if (px >= width || py >= height) {
                n++;
                continue;
            }
            if (zoomed == 1) {
                if ((zoomSide == 0 && px >= TRCARD_SIGN_WIDTH / 2) ||
                    (zoomSide == 1 && px <= TRCARD_SIGN_WIDTH / 2 - 1)) {
                    n++;
                    continue;
                }
            }
            offset = px % 8 + ((py % 8) * 8 + (px / 8 + (py / 8) * (TRCARD_SIGN_WIDTH / 8)) * 64);
            if (brush[n] == 1) {
                pixels[offset] = 7;
            } else if (brush[n] == 2) {
                pixels[offset] = 0;
            }
            n++;
        }
    }
}

// Draws the stroke from the last point to (x, y) and returns how many brushes it took
static int TrainerCard_DrawLine(u8 *pixels, const u8 *brush, int x, int y, int *lastX, int *lastY, int first, int on,
                                int zoomed, int zoomSide) {
    int count = 0;
    int x0 = *lastX;
    int y0 = *lastY;
    int endX = x;
    int dx;
    int dy;
    int endY = y;
    int stepY;
    int stepX;
    int error;

    if (first == 0 && on == 0) {
        *lastX = x;
        *lastY = y;
        return count;
    }
    dx = x - x0;
    if (dx < 0) {
        dx = -dx;
    }
    dy = y - y0;
    if (dy < 0) {
        dy = -dy;
    }
    if (dx > dy) {
        if (x0 > x) {
            if (y0 > y) {
                stepY = 1;
            } else {
                stepY = -1;
            }
            endX = x0;
            x0 = x;
            y0 = y;
        } else {
            if (y0 < y) {
                stepY = 1;
            } else {
                stepY = -1;
            }
        }
        TrainerCard_DrawBrush(pixels, brush, x0, y0, TRCARD_SIGN_WIDTH, TRCARD_SIGN_HEIGHT, zoomed, zoomSide);
        count++;
        error = dx >> 1;
        for (x0++; x0 <= endX; x0++) {
            error -= dy;
            if (error < 0) {
                error += dx;
                y0 += stepY;
            }
            TrainerCard_DrawBrush(pixels, brush, x0, y0, TRCARD_SIGN_WIDTH, TRCARD_SIGN_HEIGHT, zoomed, zoomSide);
            count++;
        }
    } else {
        if (y0 > y) {
            if (x0 > x) {
                stepX = 1;
            } else {
                stepX = -1;
            }
            endY = y0;
            y0 = y;
            x0 = x;
        } else {
            if (x0 < x) {
                stepX = 1;
            } else {
                stepX = -1;
            }
        }
        TrainerCard_DrawBrush(pixels, brush, x0, y0, TRCARD_SIGN_WIDTH, TRCARD_SIGN_HEIGHT, zoomed, zoomSide);
        count++;
        error = dy >> 1;
        for (y0++; y0 <= endY; y0++) {
            error -= dx;
            if (error < 0) {
                error += dy;
                x0 += stepX;
            }
            TrainerCard_DrawBrush(pixels, brush, x0, y0, TRCARD_SIGN_WIDTH, TRCARD_SIGN_HEIGHT, zoomed, zoomSide);
            count++;
        }
    }
    *lastX = x;
    *lastY = y;
    return count;
}

static void TrainerCard_CopyPen(TrainerCardPen *src, TrainerCardPen *dest) {
    dest->x = src->x;
    dest->y = src->y;
    dest->brush = src->brush;
    dest->on = src->on;
}

// Draws the stroke up to where the pen is now, and returns how many brushes it took
static int TrainerCard_DrawPen(u8 *pixels, TrainerCardPen *pen, TrainerCardPen *old, BOOL load, const u8 *loadSrc,
                               u8 zoomed, u8 zoomSide) {
    int lastX;
    int lastY;
    BOOL drawn = FALSE;
    int count = 0;
    int offset = zoomed == 0 ? 4 : 2;
    if (pen->on) {
        if (old->on) {
            lastX = old->x - 32 - offset;
            lastY = old->y - 90;
        }
        count = TrainerCard_DrawLine(pixels, sTrainerCardBrushes[zoomed][pen->brush], pen->x - 32 - offset, pen->y - 90,
                                     &lastX, &lastY, 0, old->on, zoomed, zoomSide);
        drawn = TRUE;
    }
    if (drawn && load) {
        GFL_BGSysLoadChar(7, loadSrc, TRCARD_SIGN_PIXELS, 1);
    }
    TrainerCard_CopyPen(pen, old);
    pen->on = 0;
    return count;
}

static void TrainerCard_SetActorVisible(TrainerCardWork *wk, int actor, BOOL visible) {
    TrCardObj_SetVisible(&wk->obj, actor, visible);
}

// Shows the actors of the front (0), of the back (1) or of neither while the card moves (2)
static void TrainerCard_SetSideActors(TrainerCardWork *wk, int mode, BOOL canEdit) {
    int i;
    BOOL anime = FALSE;

    if (mode == 0) {
        for (i = 0; i < 5; i++) {
            TrainerCard_SetActorVisible(wk, TRCARD_ACTOR_SIDE + i, sTrainerCardSideActors[canEdit][mode][i]);
        }
        if (wk->rankFlags & 1) {
            TrainerCard_SetActorVisible(wk, TRCARD_ACTOR_RANK, TRUE);
        }
        anime = TRUE;
    } else if (mode == 1) {
        for (i = 0; i < 6; i++) {
            TrainerCard_SetActorVisible(wk, TRCARD_ACTOR_SIDE + i, sTrainerCardSideActors[canEdit][mode][i]);
        }
        TrainerCard_SetScrollArrows(wk, 3);
        anime = TRUE;
    } else {
        for (i = 0; i < 6; i++) {
            TrainerCard_SetActorVisible(wk, TRCARD_ACTOR_SIDE + i, sTrainerCardSideActors[canEdit][mode][i]);
        }
    }
    TrainerCard_SetRankAnime(wk, anime);
}

// The rank that the medals earned, which a card of the other versions shows one higher at the top
static void TrainerCard_InitRank(TrainerCardWork *wk) {
    int rank;

    if (func_ov012_02169c04(wk->param->data) == 0) {
        wk->rankFlags &= ~1;
        TrainerCard_SetActorVisible(wk, TRCARD_ACTOR_RANK, FALSE);
        return;
    }
    rank = func_ov012_02169c10(wk->param->data);
    if (rank >= 4) {
        rank = 4;
        if (wk->data->version != VERSION_BLACK2) {
            rank = 5;
        }
    }
    wk->rank = rank;
    TrCardObj_SetAnime(&wk->obj, TRCARD_ACTOR_RANK, sTrainerCardRankAnimes[rank], TRUE);
    wk->rankFlags |= 1;
}

static void TrainerCard_LoadRankPalette(TrainerCardWork *wk) {
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLR(ARCID_TRAINER_CARD_GRA, 15, &palette, HEAPID_TRAINER_CARD_SCREEN);
    u16 *colors = palette->rawData;

    sys_memcpy(colors, wk->rankPalette[0], 0x20);
    sys_memcpy(colors + 16, wk->rankPalette[1], 0x20);
    GFL_HeapFree(file);
    wk->rankAngle = 0;
    TrainerCard_SetRankAnime(wk, TRUE);
}

// Fades two colors of the rank's palette between its two versions
static void TrainerCard_AnimateRankPalette(TrainerCardWork *wk) {
    int i;
    s16 ratio;

    if (wk->rankFlags & 2) {
        wk->rankAngle += 0x200;
        if (wk->rankAngle >= 0x10000) {
            wk->rankAngle -= 0x10000;
        }
        ratio = (FX_SinCosTable_[(wk->rankAngle >> 4) * 2 + 1] + 0x1000) / 2;
        for (i = 0; i < 16; i++) {
            u16 from = wk->rankPalette[0][i];
            u16 to = wk->rankPalette[1][i];
            u8 r = from & 0x1f;
            u8 g = (from & 0x3e0) >> 5;
            u8 b = (from & 0x7c00) >> 10;
            u8 tr = to & 0x1f;
            u8 tg = (to & 0x3e0) >> 5;
            u8 tb = (to & 0x7c00) >> 10;

            if (i == 12 || i == 13) {
                r = r + ((tr - r) * ratio >> 12);
                g = g + ((tg - g) * ratio >> 12);
                b = b + ((tb - b) * ratio >> 12);
            }
            wk->rankPaletteOut[i] = GX_RGB(r, g, b);
        }
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_SUB, 0, wk->rankPaletteOut, 0x20);
    }
}

static void TrainerCard_SetRankAnime(TrainerCardWork *wk, int on) {
    if (on == TRUE) {
        wk->rankFlags |= 2;
    } else {
        wk->rankFlags &= ~2;
    }
}

// The scroll arrows: the up arrow held (0), the down arrow held (1), or both at rest (2 and 3). An arrow that can't
// scroll further is greyed out
static void TrainerCard_SetScrollArrows(TrainerCardWork *wk, int mode) {
    int end = 0;
    int anime;

    if (wk->scroll <= -(wk->rowCount - 4) * TRCARD_ROW_HEIGHT) {
        end = 1;
    } else if (wk->scroll >= 0) {
        end = 2;
    }
    if (mode == 0) {
        anime = 2;
        if (end != 2) {
            anime = 1;
        }
        TrCardObj_SetAnimePalette(&wk->obj, TRCARD_ACTOR_SCROLL_UP, sTrainerCardArrowAnimes[0][anime].sequence, TRUE,
                                  sTrainerCardArrowAnimes[0][anime].palette);
    } else if (mode == 1) {
        anime = 2;
        if (end != 1) {
            anime = 1;
        }
        TrCardObj_SetAnimePalette(&wk->obj, TRCARD_ACTOR_SCROLL_DOWN, sTrainerCardArrowAnimes[1][anime].sequence, TRUE,
                                  sTrainerCardArrowAnimes[1][anime].palette);
    } else if (mode == 2 || mode == 3) {
        anime = 2;
        if (end != 2) {
            anime = 0;
        }
        TrCardObj_SetAnimePalette(&wk->obj, TRCARD_ACTOR_SCROLL_UP, sTrainerCardArrowAnimes[0][anime].sequence, TRUE,
                                  sTrainerCardArrowAnimes[0][anime].palette);
        anime = 2;
        if (end != 1) {
            anime = 0;
        }
        TrCardObj_SetAnimePalette(&wk->obj, TRCARD_ACTOR_SCROLL_DOWN, sTrainerCardArrowAnimes[1][anime].sequence, TRUE,
                                  sTrainerCardArrowAnimes[1][anime].palette);
    }
}
