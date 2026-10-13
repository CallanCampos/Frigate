// The Battle Recorder's Battle Subway records screen: a menu of the Subway's lines on the sub screen, and the streaks
// of the chosen line with the player's Battle Points on the main screen. The name is a guess: the ROM gives none, and
// br_core.c runs it as BR_PROCID_BTLSUBWAY

#include "types.h"
#include "app/battle_recorder/br_btlsubway_proc.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "constants/flags.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "save/bsubway_save.h"
#include "save/event_work.h"
#include "system/bmp_oam.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The lines of the menu, which are what the main screen shows
enum {
    BR_BTLSUBWAY_PAGE_TOP,
    BR_BTLSUBWAY_PAGE_SINGLE,
    BR_BTLSUBWAY_PAGE_DOUBLE,
    BR_BTLSUBWAY_PAGE_MULTI,
    BR_BTLSUBWAY_PAGE_WIFI,
    BR_BTLSUBWAY_PAGE_MAX,
};

#define BR_BTLSUBWAY_MENU_MAX 4
#define BR_BTLSUBWAY_MSGWIN_MAX 16
#define BR_BTLSUBWAY_BP_DIGITS 4
#define BR_BTLSUBWAY_STREAK_MAX 9999
#define BR_BTLSUBWAY_PRINT_COLOR 0x3da0

// The graphics of the Battle Points, from br_res.c
#define BR_RES_OBJ_BTLSUBWAY_BP 13

// The player's Battle Points: the icon and the digits
typedef struct {
    ClActor *icon;
    ClActor *digit[BR_BTLSUBWAY_BP_DIGITS];
} BrBtlSubwayBp;

typedef struct {
    BmpOamSys *bmpoam;
    PrintQueue *que;
    BrBtlSubwayBp bp;
    BrMsgWin *text;
    BrBtn *btn;
    BrMsgWin *menuWin[BR_BTLSUBWAY_MENU_MAX];
    BrMsgWin *msgWin[BR_BTLSUBWAY_MSGWIN_MAX];
    BrBallEffect *ballEff;
    u32 select;
    HeapID heapId;
} BrBtlSubwayWork;

// Where a window is, in tiles, and its message
typedef struct {
    u8 x;
    u8 y;
    u8 w;
    u8 h;
    u32 msgID;
} BrBtlSubwayMsgWinData;

typedef struct {
    u32 left;
    u32 top;
    u32 right;
    u32 bottom;
} BrBtlSubwayRect;

static BOOL BrBtlSubway_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBtlSubway_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBtlSubway_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrBtlSubway_CreateTop(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p);
static void BrBtlSubway_CreateSingle(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p);
static void BrBtlSubway_CreateDouble(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p);
static void BrBtlSubway_CreateMulti(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p);
static void BrBtlSubway_CreateWifi(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p);
static void BrBtlSubway_DeletePage(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p);
static void BrBtlSubway_CreateMenu(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p);
static void BrBtlSubway_DeleteMenu(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p);
static u32 BrBtlSubway_GetTrgMenu(BrBtlSubwayWork *wk, u32 x, u32 y);
static BOOL BrBtlSubway_IsSuperSingle(BSubwayScoreData *score, GameData *gameData);
static BOOL BrBtlSubway_IsSuperDouble(BSubwayScoreData *score, GameData *gameData);
static BOOL BrBtlSubway_IsSuperMulti(BSubwayScoreData *score, GameData *gameData);
static void BrBtlSubwayBp_Init(BrBtlSubwayBp *bp, ClActUnit *unit, const BrResObjData *obj, u32 value, s16 x, s16 y,
                               HeapID heapId);
static void BrBtlSubwayBp_Exit(BrBtlSubwayBp *bp);
static void BrBtlSubwayBp_SetValue(BrBtlSubwayBp *bp, u32 value);

static const BrBtlSubwayMsgWinData sBrBtlSubwayTopWins[] = {
    { 4, 6, 24, 2, 0x82 },
};

static const BrBtlSubwayMsgWinData sBrBtlSubwayMenuWins[BR_BTLSUBWAY_MENU_MAX] = {
    { 4, 2, 24, 2, 0x7e },
    { 4, 6, 24, 2, 0x7f },
    { 4, 10, 24, 2, 0x80 },
    { 4, 14, 24, 2, 0x81 },
};

static const BrBtlSubwayMsgWinData sBrBtlSubwayWifiWins[] = {
    { 3, 3, 12, 2, 0x7c },
    { 4, 6, 24, 2, 0x90 },
    { 4, 10, 13, 2, 0x8f },
    { 21, 10, 5, 2, 0x8e },
    { 27, 10, 2, 2, 0x89 },
};

const GameProcFunctions BR_BTLSUBWAY_PROC_FUNCTIONS = {
    BrBtlSubway_ProcInit,
    BrBtlSubway_ProcMain,
    BrBtlSubway_ProcExit,
};

static const BrBtlSubwayMsgWinData sBrBtlSubwaySingleWins[] = {
    { 3, 3, 12, 2, 0x7c },
    { 4, 6, 24, 2, 0x7e },
    { 4, 10, 7, 2, 0x87 },
    { 14, 10, 7, 2, 0x89 },
    { 21, 10, 7, 2, 0x8b },
    { 4, 14, 7, 2, 0x88 },
    { 14, 14, 7, 2, 0x89 },
    { 21, 14, 7, 2, 0x8b },
};

static const BrBtlSubwayMsgWinData sBrBtlSubwayDoubleWins[] = {
    { 3, 3, 12, 2, 0x7c },
    { 4, 6, 24, 2, 0x7f },
    { 4, 10, 7, 2, 0x87 },
    { 14, 10, 7, 2, 0x89 },
    { 21, 10, 7, 2, 0x8b },
    { 4, 14, 7, 2, 0x88 },
    { 14, 14, 7, 2, 0x89 },
    { 21, 14, 7, 2, 0x8b },
};

// The lines of the menu on the sub screen. The first, of no line, is never hit
static const BrBtlSubwayRect sBrBtlSubwayMenuRects[BR_BTLSUBWAY_PAGE_MAX] = {
    { 0, 0, 0, 0 },
    { 24, 8, 232, 32 },
    { 24, 40, 232, 64 },
    { 24, 72, 232, 96 },
    { 24, 104, 232, 128 },
};

static const BrBtlSubwayMsgWinData sBrBtlSubwayMultiWins[BR_BTLSUBWAY_MSGWIN_MAX] = {
    { 3, 3, 12, 2, 0x7c },
    { 4, 6, 24, 2, 0x80 },
    { 4, 9, 15, 2, 0x8c },
    { 4, 12, 7, 2, 0x87 },
    { 14, 12, 7, 2, 0x89 },
    { 21, 12, 7, 2, 0x8b },
    { 4, 14, 7, 2, 0x88 },
    { 14, 14, 7, 2, 0x89 },
    { 21, 14, 7, 2, 0x8b },
    { 4, 17, 15, 2, 0x8d },
    { 4, 20, 7, 2, 0x87 },
    { 14, 20, 7, 2, 0x89 },
    { 21, 20, 7, 2, 0x8b },
    { 4, 22, 7, 2, 0x88 },
    { 14, 22, 7, 2, 0x89 },
    { 21, 22, 7, 2, 0x8b },
};

static BOOL BrBtlSubway_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBtlSubwayProcParam *p = param;
    BrBtlSubwayWork *wk;

    wk = GFL_ProcInitSubsystem(proc, sizeof(BrBtlSubwayWork), BrProcSys_GetHeapID(p->procSys));
    sys_memset(wk, 0, sizeof(BrBtlSubwayWork));
    wk->heapId = BrProcSys_GetHeapID(p->procSys);

    wk->bmpoam = BmpOam_Init(wk->heapId, p->unit);
    wk->que = func_02021998(wk->heapId);
    wk->ballEff = BrBallEff_Init(p->unit, p->res, CLACT_SURFACE_SUB, wk->heapId);
    BrRes_LoadOBJ(p->res, BR_RES_OBJ_BTLSUBWAY_BP, wk->heapId);

    BrBtlSubway_CreateMenu(wk, p);
    BrBtlSubway_CreateTop(wk, p);
    return TRUE;
}

static BOOL BrBtlSubway_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBtlSubwayProcParam *p = param;
    BrBtlSubwayWork *wk = work;

    BrBtlSubway_DeleteMenu(wk, p);
    BrBtlSubway_DeletePage(wk, p);
    GFL_BGSysLoadScr(4);
    GFL_BGSysLoadScr(1);
    BrRes_UnloadOBJ(p->res, BR_RES_OBJ_BTLSUBWAY_BP);
    BrBallEff_Exit(wk->ballEff);
    func_02021a18(wk->que);
    BmpOam_Exit(wk->bmpoam);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BrBtlSubway_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
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
    BrBtlSubwayProcParam *p = param;
    BrBtlSubwayWork *wk = work;
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
            wk->select = BrBtlSubway_GetTrgMenu(wk, x, y);
            if (wk->select != BR_BTLSUBWAY_PAGE_TOP) {
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
        BrBtlSubway_DeletePage(wk, p);
        switch (wk->select) {
        case BR_BTLSUBWAY_PAGE_SINGLE:
            BrBtlSubway_CreateSingle(wk, p);
            break;
        case BR_BTLSUBWAY_PAGE_DOUBLE:
            BrBtlSubway_CreateDouble(wk, p);
            break;
        case BR_BTLSUBWAY_PAGE_MULTI:
            BrBtlSubway_CreateMulti(wk, p);
            break;
        case BR_BTLSUBWAY_PAGE_WIFI:
            BrBtlSubway_CreateWifi(wk, p);
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

    for (i = 0; i < BR_BTLSUBWAY_MENU_MAX; i++) {
        if (wk->menuWin[i] != NULL) {
            BrMsgWin_Main(wk->menuWin[i]);
        }
    }
    for (i = 0; i < BR_BTLSUBWAY_MSGWIN_MAX; i++) {
        if (wk->msgWin[i] != NULL) {
            BrMsgWin_Main(wk->msgWin[i]);
        }
    }
    func_02021a3c(wk->que);
    if (wk->text != NULL) {
        BrText_Main(wk->text);
    }
    BrBallEff_Main(wk->ballEff);
    return FALSE;
}

// The main screen before a line is chosen: the explanation and the Battle Points
static void BrBtlSubway_CreateTop(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p) {
    Font *font;
    MsgData *msg;
    int i;
    u32 bp;
    BrResObjData obj;

    font = BrRes_GetFont(p->res);
    msg = BrRes_GetMsgData(p->res);
    BrRes_LoadBG(p->res, 0xe, wk->heapId);

    for (i = 0; i < 1; i++) {
        const BrBtlSubwayMsgWinData *data = &sBrBtlSubwayTopWins[i];

        wk->msgWin[i] = BrMsgWin_Init(1, data->x, data->y, data->w, data->h, 14, wk->que, wk->heapId);
        BrMsgWin_SetPos(wk->msgWin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
        BrMsgWin_Print(wk->msgWin[i], msg, data->msgID, font, BR_BTLSUBWAY_PRINT_COLOR);
    }

    wk->text = BrText_Init(p->res, wk->que, wk->heapId);
    BrText_Print(wk->text, p->res, 0x86);

    bp = BSubwayScoreData_GetBattlePoints(p->score);
    BrRes_GetObjData(p->res, BR_RES_OBJ_BTLSUBWAY_BP, &obj);
    BrBtlSubwayBp_Init(&wk->bp, p->unit, &obj, bp, 104, 88, wk->heapId);
}

// NitroSDK's MATH_IMin, which nitro/math.h doesn't have yet
static inline int MATH_IMin(int a, int b) {
    return (a <= b) ? a : b;
}

// The streaks of the Single Line, or of the Super Single Line once it is open
static void BrBtlSubway_CreateSingle(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p) {
    Font *font;
    MsgData *msg;
    WordSet *wordSet;
    u32 mode;
    int i;
    StrBuf *str;
    u32 bp;
    BrResObjData obj;

    font = BrRes_GetFont(p->res);
    msg = BrRes_GetMsgData(p->res);
    wordSet = BrRes_GetWordSet(p->res);
    BrRes_LoadBG(p->res, 0xa, wk->heapId);

    if (BrBtlSubway_IsSuperSingle(p->score, p->gameData)) {
        mode = 5;
    } else {
        mode = 0;
    }

    for (i = 0; i < 8; i++) {
        const BrBtlSubwayMsgWinData *data = &sBrBtlSubwaySingleWins[i];

        wk->msgWin[i] = BrMsgWin_Init(1, data->x, data->y, data->w, data->h, 14, wk->que, wk->heapId);
        BrMsgWin_SetPos(wk->msgWin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
        switch (i) {
        case 1:
            // The line's name
            str = GFL_MsgDataLoadStrbufNew(msg, BrBtlSubway_IsSuperSingle(p->score, p->gameData) ? 0x83 : 0x7e);
            break;
        case 2:
            str = GFL_MsgDataLoadStrbufNew(msg, BSubwayScoreData_HasStage(p->score, mode) ? 0x91 : 0x87);
            break;
        case 3:
        {
            int streak = MATH_IMin(BSubwayScoreData_GetStreak(p->score, mode), BR_BTLSUBWAY_STREAK_MAX);
            StrBuf *src;

            src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            str = GFL_StrBufCreate(0x80, wk->heapId);
            WordSetNumber(wordSet, 0, streak, 4, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
            break;
        }
        case 6:
        {
            int streak = MATH_IMin(BSubwayScoreData_GetMaxStreak(p->score, mode), BR_BTLSUBWAY_STREAK_MAX);
            StrBuf *src;

            src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            str = GFL_StrBufCreate(0x80, wk->heapId);
            WordSetNumber(wordSet, 0, streak, 4, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
            break;
        }
        default:
            str = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            break;
        }
        BrMsgWin_PrintBuf(wk->msgWin[i], str, font, BR_BTLSUBWAY_PRINT_COLOR);
        GFL_StrBufFree(str);
    }

    bp = BSubwayScoreData_GetBattlePoints(p->score);
    BrRes_GetObjData(p->res, BR_RES_OBJ_BTLSUBWAY_BP, &obj);
    BrBtlSubwayBp_Init(&wk->bp, p->unit, &obj, bp, 160, 32, wk->heapId);
}

// The streaks of the Double Line, or of the Super Double Line
static void BrBtlSubway_CreateDouble(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p) {
    Font *font;
    MsgData *msg;
    WordSet *wordSet;
    u32 mode;
    int i;
    StrBuf *str;
    u32 bp;
    BrResObjData obj;

    font = BrRes_GetFont(p->res);
    msg = BrRes_GetMsgData(p->res);
    wordSet = BrRes_GetWordSet(p->res);
    BrRes_LoadBG(p->res, 0xb, wk->heapId);

    if (BrBtlSubway_IsSuperDouble(p->score, p->gameData)) {
        mode = 6;
    } else {
        mode = 1;
    }

    for (i = 0; i < 8; i++) {
        const BrBtlSubwayMsgWinData *data = &sBrBtlSubwayDoubleWins[i];

        wk->msgWin[i] = BrMsgWin_Init(1, data->x, data->y, data->w, data->h, 14, wk->que, wk->heapId);
        BrMsgWin_SetPos(wk->msgWin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
        switch (i) {
        case 1:
            // The line's name
            str = GFL_MsgDataLoadStrbufNew(msg, BrBtlSubway_IsSuperDouble(p->score, p->gameData) ? 0x84 : 0x7f);
            break;
        case 2:
            str = GFL_MsgDataLoadStrbufNew(msg, BSubwayScoreData_HasStage(p->score, mode) ? 0x91 : 0x87);
            break;
        case 3:
        {
            int streak = MATH_IMin(BSubwayScoreData_GetStreak(p->score, mode), BR_BTLSUBWAY_STREAK_MAX);
            StrBuf *src;

            src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            str = GFL_StrBufCreate(0x80, wk->heapId);
            WordSetNumber(wordSet, 0, streak, 4, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
            break;
        }
        case 6:
        {
            int streak = MATH_IMin(BSubwayScoreData_GetMaxStreak(p->score, mode), BR_BTLSUBWAY_STREAK_MAX);
            StrBuf *src;

            src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            str = GFL_StrBufCreate(0x80, wk->heapId);
            WordSetNumber(wordSet, 0, streak, 4, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
            break;
        }
        default:
            str = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            break;
        }
        BrMsgWin_PrintBuf(wk->msgWin[i], str, font, BR_BTLSUBWAY_PRINT_COLOR);
        GFL_StrBufFree(str);
    }

    bp = BSubwayScoreData_GetBattlePoints(p->score);
    BrRes_GetObjData(p->res, BR_RES_OBJ_BTLSUBWAY_BP, &obj);
    BrBtlSubwayBp_Init(&wk->bp, p->unit, &obj, bp, 160, 32, wk->heapId);
}

// The streaks of the Multi Line, or of the Super Multi Line: with a partner of the game, then with a friend
static void BrBtlSubway_CreateMulti(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p) {
    Font *font;
    MsgData *msg;
    WordSet *wordSet;
    u32 mode;
    u32 commMode;
    int i;
    StrBuf *str;
    u32 bp;
    BrResObjData obj;

    font = BrRes_GetFont(p->res);
    msg = BrRes_GetMsgData(p->res);
    wordSet = BrRes_GetWordSet(p->res);
    BrRes_LoadBG(p->res, 0xc, wk->heapId);

    if (BrBtlSubway_IsSuperMulti(p->score, p->gameData)) {
        mode = 7;
        commMode = 8;
    } else {
        mode = 2;
        commMode = 3;
    }

    for (i = 0; i < BR_BTLSUBWAY_MSGWIN_MAX; i++) {
        const BrBtlSubwayMsgWinData *data = &sBrBtlSubwayMultiWins[i];

        wk->msgWin[i] = BrMsgWin_Init(1, data->x, data->y, data->w, data->h, 14, wk->que, wk->heapId);
        BrMsgWin_SetPos(wk->msgWin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
        switch (i) {
        case 1:
            // The line's name
            str = GFL_MsgDataLoadStrbufNew(msg, BrBtlSubway_IsSuperMulti(p->score, p->gameData) ? 0x85 : 0x80);
            break;
        case 3:
            str = GFL_MsgDataLoadStrbufNew(msg, BSubwayScoreData_HasStage(p->score, mode) ? 0x91 : 0x87);
            break;
        case 4:
        {
            int streak = MATH_IMin(BSubwayScoreData_GetStreak(p->score, mode), BR_BTLSUBWAY_STREAK_MAX);
            StrBuf *src;

            src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            str = GFL_StrBufCreate(0x80, wk->heapId);
            WordSetNumber(wordSet, 0, streak, 4, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
            break;
        }
        case 7:
        {
            int streak = MATH_IMin(BSubwayScoreData_GetMaxStreak(p->score, mode), BR_BTLSUBWAY_STREAK_MAX);
            StrBuf *src;

            src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            str = GFL_StrBufCreate(0x80, wk->heapId);
            WordSetNumber(wordSet, 0, streak, 4, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
            break;
        }
        case 10:
            str = GFL_MsgDataLoadStrbufNew(msg, BSubwayScoreData_HasStage(p->score, commMode) ? 0x91 : 0x87);
            break;
        case 11:
        {
            int streak = MATH_IMin(BSubwayScoreData_GetStreak(p->score, commMode), BR_BTLSUBWAY_STREAK_MAX);
            StrBuf *src;

            src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            str = GFL_StrBufCreate(0x80, wk->heapId);
            WordSetNumber(wordSet, 0, streak, 4, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
            break;
        }
        case 14:
        {
            int streak = MATH_IMin(BSubwayScoreData_GetMaxStreak(p->score, commMode), BR_BTLSUBWAY_STREAK_MAX);
            StrBuf *src;

            src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            str = GFL_StrBufCreate(0x80, wk->heapId);
            WordSetNumber(wordSet, 0, streak, 4, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
            break;
        }
        default:
            str = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            break;
        }
        BrMsgWin_PrintBuf(wk->msgWin[i], str, font, BR_BTLSUBWAY_PRINT_COLOR);
        GFL_StrBufFree(str);
    }

    bp = BSubwayScoreData_GetBattlePoints(p->score);
    BrRes_GetObjData(p->res, BR_RES_OBJ_BTLSUBWAY_BP, &obj);
    BrBtlSubwayBp_Init(&wk->bp, p->unit, &obj, bp, 160, 32, wk->heapId);
}

// The player's rank on the Wi-Fi Line
static void BrBtlSubway_CreateWifi(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p) {
    Font *font;
    MsgData *msg;
    WordSet *wordSet;
    StrBuf *str;
    int i;
    u32 bp;
    BrResObjData obj;

    font = BrRes_GetFont(p->res);
    msg = BrRes_GetMsgData(p->res);
    wordSet = BrRes_GetWordSet(p->res);
    BrRes_LoadBG(p->res, 0xd, wk->heapId);

    for (i = 0; i < 5; i++) {
        const BrBtlSubwayMsgWinData *data = &sBrBtlSubwayWifiWins[i];

        wk->msgWin[i] = BrMsgWin_Init(1, data->x, data->y, data->w, data->h, 14, wk->que, wk->heapId);
        if (i == 4) {
            u32 rank = BSubwayScoreData_GetWifiRank(p->score);
            StrBuf *src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);

            str = GFL_StrBufCreate(0x80, wk->heapId);
            WordSetNumber(wordSet, 0, rank, 2, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(src);
        } else {
            str = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
        }
        BrMsgWin_PrintBuf(wk->msgWin[i], str, font, BR_BTLSUBWAY_PRINT_COLOR);
        GFL_StrBufFree(str);
    }

    bp = BSubwayScoreData_GetBattlePoints(p->score);
    BrRes_GetObjData(p->res, BR_RES_OBJ_BTLSUBWAY_BP, &obj);
    BrBtlSubwayBp_Init(&wk->bp, p->unit, &obj, bp, 160, 32, wk->heapId);
}

static void BrBtlSubway_DeletePage(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p) {
    int i;

    BrBtlSubwayBp_Exit(&wk->bp);
    for (i = 0; i < BR_BTLSUBWAY_MSGWIN_MAX; i++) {
        if (wk->msgWin[i] != NULL) {
            BrMsgWin_Exit(wk->msgWin[i]);
            wk->msgWin[i] = NULL;
        }
    }
    BrRes_UnloadBG(p->res, 0xa);
    if (wk->text != NULL) {
        BrText_Exit(wk->text, p->res);
        wk->text = NULL;
    }
}

// Makes the menu of the lines and the return button on the sub screen
static void BrBtlSubway_CreateMenu(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p) {
    Font *font;
    MsgData *msg;
    ClActorSetup setup;
    BrResObjData obj;
    int i;
    u32 msgID;

    BrRes_LoadOBJ(p->res, 6, wk->heapId);
    BrRes_LoadBG(p->res, 9, wk->heapId);
    font = BrRes_GetFont(p->res);
    msg = BrRes_GetMsgData(p->res);

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 80;
    setup.y = 168;
    setup.sequence = 0;
    setup.priority = 1;
    BrRes_GetObjData(p->res, 6, &obj);
    wk->btn = BrBtn_InitEx(&setup, 4, 96, CLACT_SURFACE_SUB, p->unit, wk->bmpoam, font, msg, &obj, wk->heapId);

    for (i = 0; i < BR_BTLSUBWAY_MENU_MAX; i++) {
        const BrBtlSubwayMsgWinData *data = &sBrBtlSubwayMenuWins[i];

        wk->menuWin[i] = BrMsgWin_Init(4, data->x, data->y, data->w, data->h, 14, wk->que, wk->heapId);
        switch (i) {
        case 0:
            if (BrBtlSubway_IsSuperSingle(p->score, p->gameData)) {
                msgID = 0x83;
            } else {
                msgID = 0x7e;
            }
            break;
        case 1:
            if (BrBtlSubway_IsSuperDouble(p->score, p->gameData)) {
                msgID = 0x84;
            } else {
                msgID = 0x7f;
            }
            break;
        case 2:
            if (BrBtlSubway_IsSuperMulti(p->score, p->gameData)) {
                msgID = 0x85;
            } else {
                msgID = 0x80;
            }
            break;
        case 3:
            msgID = 0x81;
            break;
        }
        BrMsgWin_Print(wk->menuWin[i], msg, msgID, font, BR_BTLSUBWAY_PRINT_COLOR);
    }
}

static void BrBtlSubway_DeleteMenu(BrBtlSubwayWork *wk, BrBtlSubwayProcParam *p) {
    int i;

    for (i = 0; i < BR_BTLSUBWAY_MENU_MAX; i++) {
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
    BrRes_UnloadBG(p->res, 9);
}

// The line of the menu that the touch is on, other than the one shown, or BR_BTLSUBWAY_PAGE_TOP
static u32 BrBtlSubway_GetTrgMenu(BrBtlSubwayWork *wk, u32 x, u32 y) {
    u32 select = wk->select;
    int i;

    for (i = 0; i < BR_BTLSUBWAY_PAGE_MAX; i++) {
        if (i != select) {
            const BrBtlSubwayRect *rect = &sBrBtlSubwayMenuRects[i];

            if ((u32)(x - rect->left) <= (u32)(rect->right - rect->left)
                && (u32)(y - rect->top) <= (u32)(rect->bottom - rect->top)) {
                BrPoint pos;

                pos.x = x;
                pos.y = y;
                BrBallEff_Start(wk->ballEff, BR_BALL_EFFECT_SPREAD, &pos);
                GFL_SndSEPlay(SEQ_SE_BREC_06);
                return i;
            }
        }
    }
    return BR_BTLSUBWAY_PAGE_TOP;
}

// Whether the Super lines are open and the player has taken the line
static BOOL BrBtlSubway_IsSuperSingle(BSubwayScoreData *score, GameData *gameData) {
    BOOL flag = BSubwayScoreData_SetFlag(score, 4, BSUBWAY_FLAG_GET);
    BOOL isOpen = EventWork_FlagGet(GameData_GetEventWork(gameData), EVENT_FLAG_0x0960);

    if (flag && isOpen) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BrBtlSubway_IsSuperDouble(BSubwayScoreData *score, GameData *gameData) {
    BOOL flag = BSubwayScoreData_SetFlag(score, 5, BSUBWAY_FLAG_GET);
    BOOL isOpen = EventWork_FlagGet(GameData_GetEventWork(gameData), EVENT_FLAG_0x0960);

    if (flag && isOpen) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BrBtlSubway_IsSuperMulti(BSubwayScoreData *score, GameData *gameData) {
    BOOL flag = BSubwayScoreData_SetFlag(score, 6, BSUBWAY_FLAG_GET);
    BOOL isOpen = EventWork_FlagGet(GameData_GetEventWork(gameData), EVENT_FLAG_0x0960);

    if (flag && isOpen) {
        return TRUE;
    }
    return FALSE;
}

// Makes the Battle Points: the icon at the position and four digits to its right
static void BrBtlSubwayBp_Init(BrBtlSubwayBp *bp, ClActUnit *unit, const BrResObjData *obj, u32 value, s16 x, s16 y,
                               HeapID heapId) {
    ClActorSetup setup;
    int i;

    sys_memset(bp, 0, sizeof(BrBtlSubwayBp));
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = x;
    setup.y = y;
    setup.sequence = 10;
    bp->icon = func_0204c040(unit, obj->chr, obj->plt, obj->cell, &setup, 0, heapId);
    func_0204c318(bp->icon, 1);

    setup.sequence = 0;
    for (i = 0; i < BR_BTLSUBWAY_BP_DIGITS; i++) {
        setup.x = x + 20 + i * 16;
        setup.y = y;
        bp->digit[i] = func_0204c040(unit, obj->chr, obj->plt, obj->cell, &setup, 0, heapId);
        func_0204c318(bp->digit[i], 1);
    }
    BrBtlSubwayBp_SetValue(bp, value);
}

static void BrBtlSubwayBp_Exit(BrBtlSubwayBp *bp) {
    int i;

    for (i = 0; i < BR_BTLSUBWAY_BP_DIGITS; i++) {
        if (bp->digit[i] != NULL) {
            func_0204c108(bp->digit[i]);
            bp->digit[i] = NULL;
        }
    }
    func_0204c108(bp->icon);
    sys_memset(bp, 0, sizeof(BrBtlSubwayBp));
}

static void BrBtlSubwayBp_SetValue(BrBtlSubwayBp *bp, u32 value) {
    static const u32 sBrBtlSubwayBpDigits[] = { 1, 10, 100, 1000, 10000 };
    u8 rest = value;
    int keta = 0;
    int i;

    // The number of digits, which nothing uses
    if (rest != 0) {
        for (;;) {
            if (rest == 0) {
                break;
            }
            rest /= 10;
            keta++;
        }
    }

    for (i = 0; i < BR_BTLSUBWAY_BP_DIGITS; i++) {
        func_0204c488(bp->digit[i], 0);
    }
    for (i = 0; i < BR_BTLSUBWAY_BP_DIGITS; i++) {
        u32 number = value % sBrBtlSubwayBpDigits[i + 1] / sBrBtlSubwayBpDigits[i];

        func_0204c488(bp->digit[BR_BTLSUBWAY_BP_DIGITS - 1 - i], number);
    }
}
