// The Battle Recorder's Random Matchup records screen: a menu of the records on the sub screen, and on the main screen
// the player's ratings or the wins and losses of the Rating and Free modes, by battle type. The name is a guess: the
// ROM gives none, and br_core.c runs it as BR_PROCID_RNDMATCH

#include "types.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_rndmatch_proc.h"
#include "app/battle_recorder/br_util.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "save/records.h"
#include "system/bmp_oam.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The records of the menu, which are what the main screen shows
enum {
    BR_RNDMATCH_PAGE_TOP,
    BR_RNDMATCH_PAGE_RATING,
    BR_RNDMATCH_PAGE_RATE,
    BR_RNDMATCH_PAGE_FREE,
    BR_RNDMATCH_PAGE_MAX,
};

#define BR_RNDMATCH_MENU_MAX 3
#define BR_RNDMATCH_MSGWIN_MAX 18
#define BR_RNDMATCH_PRINT_COLOR 0x3da0

// The battle types of a mode, and where the Rating mode's results are in the Random Matchup records, after the Free
// mode's
#define BR_RNDMATCH_BTL_TYPE_MAX 5
#define BR_RNDMATCH_RECORD_RATE BR_RNDMATCH_BTL_TYPE_MAX

// The fields of a result of the Random Matchup records
enum {
    BR_RNDMATCH_FIELD_RATING,
    BR_RNDMATCH_FIELD_WIN,
    BR_RNDMATCH_FIELD_LOSE,
};

typedef struct {
    BmpOamSys *bmpoam;
    PrintQueue *que;
    BrMsgWin *text;
    BrBtn *btn;
    BrMsgWin *menuWin[BR_RNDMATCH_MENU_MAX];
    BrMsgWin *msgWin[BR_RNDMATCH_MSGWIN_MAX];
    BrBallEffect *ballEff;
    u32 select;
    HeapID heapId;
} BrRndMatchWork;

// Where a window is, in tiles, and its message
typedef struct {
    u8 x;
    u8 y;
    u8 w;
    u8 h;
    u32 msgID;
} BrRndMatchMsgWinData;

typedef struct {
    u32 left;
    u32 top;
    u32 right;
    u32 bottom;
} BrRndMatchRect;

static BOOL BrRndMatch_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrRndMatch_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrRndMatch_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrRndMatch_CreateTop(BrRndMatchWork *wk, BrRndMatchProcParam *p);
static void BrRndMatch_CreateRating(BrRndMatchWork *wk, BrRndMatchProcParam *p);
static void BrRndMatch_CreateRate(BrRndMatchWork *wk, BrRndMatchProcParam *p);
static void BrRndMatch_CreateFree(BrRndMatchWork *wk, BrRndMatchProcParam *p);
static void BrRndMatch_DeletePage(BrRndMatchWork *wk, BrRndMatchProcParam *p);
static void BrRndMatch_CreateMenu(BrRndMatchWork *wk, BrRndMatchProcParam *p);
static void BrRndMatch_DeleteMenu(BrRndMatchWork *wk, BrRndMatchProcParam *p);
static u32 BrRndMatch_GetTrgMenu(BrRndMatchWork *wk, u32 x, u32 y);

const GameProcFunctions BR_RNDMATCH_PROC_FUNCTIONS = {
    BrRndMatch_ProcInit,
    BrRndMatch_ProcMain,
    BrRndMatch_ProcExit,
};

static const BrRndMatchMsgWinData sBrRndMatchMenuWins[BR_RNDMATCH_MENU_MAX] = {
    { 4, 3, 24, 2, 0x92 },
    { 4, 8, 24, 2, 0x93 },
    { 4, 13, 24, 2, 0x94 },
};

// The records of the menu on the sub screen. The first, of no record, is never hit
static const BrRndMatchRect sBrRndMatchMenuRects[BR_RNDMATCH_PAGE_MAX] = {
    { 0, 0, 0, 0 },
    { 24, 16, 232, 40 },
    { 24, 56, 232, 80 },
    { 24, 96, 232, 120 },
};

static const BrRndMatchMsgWinData sBrRndMatchRatingWins[] = {
    { 4, 3, 24, 2, 0x92 },   { 14, 6, 14, 2, 0x9e },  { 3, 9, 11, 2, 0x97 },   { 3, 12, 11, 2, 0x98 },
    { 3, 15, 11, 2, 0x99 },  { 3, 18, 11, 2, 0x9a },  { 3, 21, 11, 2, 0x9b },  { 18, 9, 11, 2, 0x9c },
    { 18, 12, 11, 2, 0x9c }, { 18, 15, 11, 2, 0x9c }, { 18, 18, 11, 2, 0x9c }, { 18, 21, 11, 2, 0x9c },
};

static const BrRndMatchMsgWinData sBrRndMatchFreeWins[BR_RNDMATCH_MSGWIN_MAX] = {
    { 4, 3, 24, 2, 0x94 },  { 17, 6, 5, 2, 0x95 },  { 24, 6, 5, 2, 0x96 },  { 3, 9, 11, 2, 0x97 },
    { 3, 12, 11, 2, 0x98 }, { 3, 15, 11, 2, 0x99 }, { 3, 18, 11, 2, 0x9a }, { 3, 21, 11, 2, 0x9b },
    { 17, 9, 5, 2, 0x9d },  { 17, 12, 5, 2, 0x9d }, { 17, 15, 5, 2, 0x9d }, { 17, 18, 5, 2, 0x9d },
    { 17, 21, 5, 2, 0x9d }, { 24, 9, 5, 2, 0x9d },  { 24, 12, 5, 2, 0x9d }, { 24, 15, 5, 2, 0x9d },
    { 24, 18, 5, 2, 0x9d }, { 24, 21, 5, 2, 0x9d },
};

static const BrRndMatchMsgWinData sBrRndMatchRateWins[BR_RNDMATCH_MSGWIN_MAX] = {
    { 4, 3, 24, 2, 0x93 },  { 17, 6, 5, 2, 0x95 },  { 24, 6, 5, 2, 0x96 },  { 3, 9, 11, 2, 0x97 },
    { 3, 12, 11, 2, 0x98 }, { 3, 15, 11, 2, 0x99 }, { 3, 18, 11, 2, 0x9a }, { 3, 21, 11, 2, 0x9b },
    { 17, 9, 5, 2, 0x9d },  { 17, 12, 5, 2, 0x9d }, { 17, 15, 5, 2, 0x9d }, { 17, 18, 5, 2, 0x9d },
    { 17, 21, 5, 2, 0x9d }, { 24, 9, 5, 2, 0x9d },  { 24, 12, 5, 2, 0x9d }, { 24, 15, 5, 2, 0x9d },
    { 24, 18, 5, 2, 0x9d }, { 24, 21, 5, 2, 0x9d },
};

static BOOL BrRndMatch_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrRndMatchProcParam *p = param;
    BrRndMatchWork *wk;

    wk = GFL_ProcInitSubsystem(proc, sizeof(BrRndMatchWork), BrProcSys_GetHeapID(p->procSys));
    sys_memset(wk, 0, sizeof(BrRndMatchWork));
    wk->heapId = BrProcSys_GetHeapID(p->procSys);

    wk->bmpoam = BmpOam_Init(wk->heapId, p->unit);
    wk->que = func_02021998(wk->heapId);
    wk->ballEff = BrBallEff_Init(p->unit, p->res, CLACT_SURFACE_SUB, wk->heapId);

    BrRndMatch_CreateTop(wk, p);
    BrRndMatch_CreateMenu(wk, p);
    return TRUE;
}

static BOOL BrRndMatch_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrRndMatchProcParam *p = param;
    BrRndMatchWork *wk = work;

    BrRndMatch_DeletePage(wk, p);
    BrRndMatch_DeleteMenu(wk, p);
    GFL_BGSysLoadScr(4);
    GFL_BGSysLoadScr(1);
    BrBallEff_Exit(wk->ballEff);
    func_02021a18(wk->que);
    BmpOam_Exit(wk->bmpoam);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BrRndMatch_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    enum {
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_MAIN,
        SEQ_CHANGE_FADEOUT_START,
        SEQ_CHANGE_FADEOUT_WAIT,
        SEQ_CHANGE,
        SEQ_CHANGE_FADEIN_WAIT,
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrRndMatchProcParam *p = param;
    BrRndMatchWork *wk = work;
    int i;

    switch (*state) {
    case SEQ_FADEIN_START:
        BrFade_StartFade(p->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *state = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p->fade)) {
            *state = SEQ_MAIN;
        }
        break;
    case SEQ_MAIN: {
        u32 x;
        u32 y;

        if (func_0203dac8(&x, &y)) {
            wk->select = BrRndMatch_GetTrgMenu(wk, x, y);
            if (wk->select != BR_RNDMATCH_PAGE_TOP) {
                *state = SEQ_CHANGE_FADEOUT_START;
            }
            if (BrBtn_GetTrg(wk->btn, x, y)) {
                BrPoint pos;

                pos.x = x;
                pos.y = y;
                BrBallEff_Start(wk->ballEff, BR_BALL_EFFECT_SPREAD, &pos);
                *state = SEQ_FADEOUT_START;
            }
        }
        break;
    }
    case SEQ_CHANGE_FADEOUT_START:
        BrFade_StartFade(p->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_MAIN, BR_FADE_DIR_OUT);
        (*state)++;
        break;
    case SEQ_CHANGE_FADEOUT_WAIT:
        if (BrFade_IsEnd(p->fade)) {
            (*state)++;
        }
        break;
    case SEQ_CHANGE:
        BrRndMatch_DeletePage(wk, p);
        switch (wk->select) {
        case BR_RNDMATCH_PAGE_RATING:
            BrRndMatch_CreateRating(wk, p);
            break;
        case BR_RNDMATCH_PAGE_RATE:
            BrRndMatch_CreateRate(wk, p);
            break;
        case BR_RNDMATCH_PAGE_FREE:
            BrRndMatch_CreateFree(wk, p);
            break;
        }
        BrFade_StartFade(p->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_MAIN, BR_FADE_DIR_IN);
        (*state)++;
        break;
    case SEQ_CHANGE_FADEIN_WAIT:
        if (BrFade_IsEnd(p->fade)) {
            *state = SEQ_MAIN;
        }
        break;
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *state = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p->fade)) {
            *state = SEQ_END;
        }
        break;
    case SEQ_END:
        BrProcSys_Pop(p->procSys);
        return TRUE;
    }

    for (i = 0; i < BR_RNDMATCH_MENU_MAX; i++) {
        if (wk->menuWin[i] != NULL) {
            BrMsgWin_Main(wk->menuWin[i]);
        }
    }
    for (i = 0; i < BR_RNDMATCH_MSGWIN_MAX; i++) {
        if (wk->msgWin[i] != NULL) {
            BrMsgWin_Main(wk->msgWin[i]);
        }
    }
    func_02021a3c(wk->que);
    BrBallEff_Main(wk->ballEff);
    if (wk->text != NULL) {
        BrText_Main(wk->text);
    }
    return FALSE;
}

// The main screen before a record is chosen: the explanation
static void BrRndMatch_CreateTop(BrRndMatchWork *wk, BrRndMatchProcParam *p) {
    wk->text = BrText_Init(p->res, wk->que, wk->heapId);
    BrText_Print(wk->text, p->res, 0x86);
}

// The player's rating in each battle type
static void BrRndMatch_CreateRating(BrRndMatchWork *wk, BrRndMatchProcParam *p) {
    Font *font;
    MsgData *msg;
    WordSet *wordSet;
    StrBuf *str;
    int i;

    font = BrRes_GetFont(p->res);
    msg = BrRes_GetMsgData(p->res);
    wordSet = BrRes_GetWordSet(p->res);
    BrRes_LoadBG(p->res, 0x10, wk->heapId);

    for (i = 0; i < 12; i++) {
        BOOL isFormat = TRUE;
        const BrRndMatchMsgWinData *data = &sBrRndMatchRatingWins[i];

        wk->msgWin[i] = BrMsgWin_Init(1, data->x, data->y, data->w, data->h, 14, wk->que, wk->heapId);
        BrMsgWin_SetPos(wk->msgWin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
        switch (i) {
        case 0:
            copyVarForText(wordSet, 0, GetGameDataPlayerInfo(p->gameData));
            break;
        case 7:
            WordSetNumber(wordSet, 0,
                          RndMatchData_GetParam(p->record, BR_RNDMATCH_RECORD_RATE + 0, BR_RNDMATCH_FIELD_RATING), 4, 1,
                          TRUE);
            break;
        case 8:
            WordSetNumber(wordSet, 0,
                          RndMatchData_GetParam(p->record, BR_RNDMATCH_RECORD_RATE + 1, BR_RNDMATCH_FIELD_RATING), 4, 1,
                          TRUE);
            break;
        case 9:
            WordSetNumber(wordSet, 0,
                          RndMatchData_GetParam(p->record, BR_RNDMATCH_RECORD_RATE + 2, BR_RNDMATCH_FIELD_RATING), 4, 1,
                          TRUE);
            break;
        case 10:
            WordSetNumber(wordSet, 0,
                          RndMatchData_GetParam(p->record, BR_RNDMATCH_RECORD_RATE + 3, BR_RNDMATCH_FIELD_RATING), 4, 1,
                          TRUE);
            break;
        case 11:
            WordSetNumber(wordSet, 0,
                          RndMatchData_GetParam(p->record, BR_RNDMATCH_RECORD_RATE + 4, BR_RNDMATCH_FIELD_RATING), 4, 1,
                          TRUE);
            break;
        default:
            str = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            isFormat = FALSE;
            break;
        }
        if (isFormat) {
            StrBuf *src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);

            str = GFL_StrBufCreate(0x80, wk->heapId);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
        }
        BrMsgWin_PrintBuf(wk->msgWin[i], str, font, BR_RNDMATCH_PRINT_COLOR);
        GFL_StrBufFree(str);
    }
}

// The wins and losses of the Rating mode in each battle type
static void BrRndMatch_CreateRate(BrRndMatchWork *wk, BrRndMatchProcParam *p) {
    Font *font;
    MsgData *msg;
    WordSet *wordSet;
    StrBuf *str;
    int i;
    u32 value;
    BOOL isFormat;

    font = BrRes_GetFont(p->res);
    msg = BrRes_GetMsgData(p->res);
    wordSet = BrRes_GetWordSet(p->res);
    BrRes_LoadBG(p->res, 0x11, wk->heapId);

    for (i = 0; i < BR_RNDMATCH_MSGWIN_MAX; i++) {
        const BrRndMatchMsgWinData *data = &sBrRndMatchRateWins[i];

        wk->msgWin[i] = BrMsgWin_Init(1, data->x, data->y, data->w, data->h, 14, wk->que, wk->heapId);
        BrMsgWin_SetPos(wk->msgWin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
        if (8 <= i && i <= 12) {
            value = RndMatchData_GetParam(p->record, i - 8 + BR_RNDMATCH_RECORD_RATE, BR_RNDMATCH_FIELD_WIN);
            isFormat = TRUE;
        } else if (13 <= i && i <= 17) {
            value = RndMatchData_GetParam(p->record, i - 13 + BR_RNDMATCH_RECORD_RATE, BR_RNDMATCH_FIELD_LOSE);
            isFormat = TRUE;
        } else {
            str = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            isFormat = FALSE;
        }
        if (isFormat) {
            StrBuf *src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);

            str = GFL_StrBufCreate(0x80, wk->heapId);
            WordSetNumber(wordSet, 0, value, 5, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
        }
        BrMsgWin_PrintBuf(wk->msgWin[i], str, font, BR_RNDMATCH_PRINT_COLOR);
        GFL_StrBufFree(str);
    }
}

// The wins and losses of the Free mode in each battle type
static void BrRndMatch_CreateFree(BrRndMatchWork *wk, BrRndMatchProcParam *p) {
    Font *font;
    MsgData *msg;
    WordSet *wordSet;
    StrBuf *str;
    int i;
    u32 value;
    BOOL isFormat;

    font = BrRes_GetFont(p->res);
    msg = BrRes_GetMsgData(p->res);
    wordSet = BrRes_GetWordSet(p->res);
    BrRes_LoadBG(p->res, 0x11, wk->heapId);

    for (i = 0; i < BR_RNDMATCH_MSGWIN_MAX; i++) {
        const BrRndMatchMsgWinData *data = &sBrRndMatchFreeWins[i];

        wk->msgWin[i] = BrMsgWin_Init(1, data->x, data->y, data->w, data->h, 14, wk->que, wk->heapId);
        BrMsgWin_SetPos(wk->msgWin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
        if (8 <= i && i <= 12) {
            value = RndMatchData_GetParam(p->record, i - 8, BR_RNDMATCH_FIELD_WIN);
            isFormat = TRUE;
        } else if (13 <= i && i <= 17) {
            value = RndMatchData_GetParam(p->record, i - 13, BR_RNDMATCH_FIELD_LOSE);
            isFormat = TRUE;
        } else {
            str = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            isFormat = FALSE;
        }
        if (isFormat) {
            StrBuf *src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);

            str = GFL_StrBufCreate(0x80, wk->heapId);
            WordSetNumber(wordSet, 0, value, 5, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
        }
        BrMsgWin_PrintBuf(wk->msgWin[i], str, font, BR_RNDMATCH_PRINT_COLOR);
        GFL_StrBufFree(str);
    }
}

static void BrRndMatch_DeletePage(BrRndMatchWork *wk, BrRndMatchProcParam *p) {
    int i;

    for (i = 0; i < BR_RNDMATCH_MSGWIN_MAX; i++) {
        if (wk->msgWin[i] != NULL) {
            BrMsgWin_Exit(wk->msgWin[i]);
            wk->msgWin[i] = NULL;
        }
    }
    BrRes_UnloadBG(p->res, 0x11);
    if (wk->text != NULL) {
        BrText_Exit(wk->text, p->res);
        wk->text = NULL;
    }
}

// Makes the menu of the records and the return button on the sub screen
static void BrRndMatch_CreateMenu(BrRndMatchWork *wk, BrRndMatchProcParam *p) {
    Font *font;
    WordSet *wordSet;
    MsgData *msg;
    ClActorSetup setup;
    BrResObjData obj;
    StrBuf *src;
    StrBuf *str;
    PlayerInfo *player;
    int i;

    BrRes_LoadOBJ(p->res, 6, wk->heapId);
    BrRes_LoadBG(p->res, 0xf, wk->heapId);
    font = BrRes_GetFont(p->res);
    msg = BrRes_GetMsgData(p->res);
    wordSet = BrRes_GetWordSet(p->res);

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 80;
    setup.y = 168;
    setup.sequence = 0;
    setup.priority = 1;
    BrRes_GetObjData(p->res, 6, &obj);
    wk->btn = BrBtn_InitEx(&setup, 4, 96, CLACT_SURFACE_SUB, p->unit, wk->bmpoam, font, msg, &obj, wk->heapId);

    src = GFL_StrBufCreate(0x80, wk->heapId);
    str = GFL_StrBufCreate(0x80, wk->heapId);
    player = GetGameDataPlayerInfo(p->gameData);

    for (i = 0; i < BR_RNDMATCH_MENU_MAX; i++) {
        const BrRndMatchMsgWinData *data = &sBrRndMatchMenuWins[i];

        wk->menuWin[i] = BrMsgWin_Init(4, data->x, data->y, data->w, data->h, 14, wk->que, wk->heapId);
        if (i == 0) {
            // The player's name
            GFL_MsgDataLoadStrbuf(msg, data->msgID, src);
            copyVarForText(wordSet, 0, player);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
        } else {
            GFL_MsgDataLoadStrbuf(msg, data->msgID, str);
        }
        BrMsgWin_PrintBuf(wk->menuWin[i], str, font, BR_RNDMATCH_PRINT_COLOR);
    }

    GFL_StrBufFree(src);
    GFL_StrBufFree(str);
}

static void BrRndMatch_DeleteMenu(BrRndMatchWork *wk, BrRndMatchProcParam *p) {
    int i;

    for (i = 0; i < BR_RNDMATCH_MENU_MAX; i++) {
        if (wk->menuWin[i] != NULL) {
            BrMsgWin_Exit(wk->menuWin[i]);
            wk->menuWin[i] = NULL;
        }
    }
    if (wk->btn != NULL) {
        BrBtn_Exit(wk->btn);
        wk->btn = NULL;
    }
    BrRes_UnloadOBJ(p->res, 6);
    BrRes_UnloadBG(p->res, 0xf);
}

// The record of the menu that the touch is on, other than the one shown, or BR_RNDMATCH_PAGE_TOP
static u32 BrRndMatch_GetTrgMenu(BrRndMatchWork *wk, u32 x, u32 y) {
    u32 select = wk->select;
    int i;

    for (i = 0; i < BR_RNDMATCH_PAGE_MAX; i++) {
        if (i != select) {
            const BrRndMatchRect *rect = &sBrRndMatchMenuRects[i];

            if ((u32)(x - rect->left) <= (u32)(rect->right - rect->left) &&
                (u32)(y - rect->top) <= (u32)(rect->bottom - rect->top)) {
                BrPoint pos;

                pos.x = x;
                pos.y = y;
                BrBallEff_Start(wk->ballEff, BR_BALL_EFFECT_SPREAD, &pos);
                GFL_SndSEPlay(SEQ_SE_BREC_06);
                return i;
            }
        }
    }
    return BR_RNDMATCH_PAGE_TOP;
}
