#include "app/pmsiv_edit.h"
#include "types.h"
#include "app/pms_input.h"
#include "app/pms_input_view.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "system/bgwinfrm.h"
#include "system/bmp_cursor.h"
#include "system/gf_font.h"
#include "system/pms_data.h"
#include "system/pms_word.h"
#include "system/pmsi_param.h"
#include "system/printsys.h"

// The phrase input's edit area: the sentence or the words being written, drawn on BG0 of the upper screen and BG4 of
// the lower one, with a cursor on each screen and number words drawn as actors. The names are ours, guessed

// What the parser of the sentence's text finds next: text, a word's command, a new line, or the end
enum {
    PARSE_TEXT,
    PARSE_WORD,
    PARSE_NEWLINE,
    PARSE_END,
};

// The characters that end a run of text
#define CHAR_COMMAND 0xf000
#define CHAR_NEWLINE 0xfffe
#define CHAR_EOS 0xffff

// The message file of the phrase input's texts
#define PMSIV_EDIT_MSG_FILE TEXT_BANK_PMS_INPUT

typedef struct {
    StrBuf *str;
    const u16 *cur;
    u32 state;
    u16 param;
} PMSIVEditParser;

struct PMSIVEdit {
    PMSInputView *vwk;
    const PMSInputWork *mwk;
    const PMSInputData *dwk;
    // The area's window on each screen
    BmpWin *win[2];
    ClActor *cursor[2];
    // The actors that show number words, for each word place on each screen
    ClActor *numberAct[2][2];
    MsgData *msgData;
    StrBuf *str;
    BmpCursor *bmpCursor;
    ClActorPos wordPos[2];
    u16 wordIndex[2];
    u32 wordCount;
    PMSIVEditParser parser;
    u16 palette[160];
    s16 frameY;
    s16 scrollY;
    u8 scrollCount;
    u8 scrollUp;
    int *keyMode;
    TCB *hblankTask;
    u32 unk1A8;
    u32 unk1AC;
    TCB *vblankTask;
    void *frames;
    BOOL framesDirty;
};

static void PMSIVEdit_HBlankTask(TCB *tcb, void *data);
static void PMSIVEdit_VBlankTask(TCB *tcb, void *data);
static void PMSIVEdit_SetupPalette(PMSIVEdit *wk, ArcTool *arc);
static void PMSIVEdit_UpdatePalette(PMSIVEdit *wk);
static void PMSIVEdit_SetupWordPos(PMSIVEdit *wk);
static void PMSIVEdit_SetupWordActors(PMSIVEdit *wk);
static void PMSIVEdit_SetupCursorActors(PMSIVEdit *wk);
static u32 PMSIVEdit_PrintSentence(PMSIVEdit *wk, BmpWin *win, u8 lcd);
static void PMSIVEdit_ParserInit(PMSIVEditParser *parser, PMSIVEdit *wk);
static void PMSIVEdit_ParserEnd(PMSIVEditParser *parser);
static u32 PMSIVEdit_ParserNext(PMSIVEditParser *parser, StrBuf *buf);
static void PMSIVEdit_GetWordRectPos(const ClActorPos *pos, ClActorPos *out);
static void PMSIVEdit_GetWordCursorPos(const ClActorPos *pos, ClActorPos *out);
static void PMSIVEdit_ClearWord(BmpWin *win, const ClActorPos *pos);
static void PMSIVEdit_PrintWord(PMSIVEdit *wk, BmpWin *win, const ClActorPos *pos, u16 word, ClActor *numberAct);
static void PMSIVEdit_SetCursorActive(PMSIVEdit *wk, BOOL active);
static u32 PMSIVEdit_GetBGEngine(u32 bg);

PMSIVEdit *PMSIVEdit_Create(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk) {
    PMSIVEdit *wk = GFL_HeapAllocate(HEAPID_PMS_INPUT, sizeof(PMSIVEdit), TRUE, "pmsiv_edit.c", 211);

    wk->vwk = vwk;
    wk->mwk = mwk;
    wk->dwk = dwk;
    wk->keyMode = PMSInput_GetKeyModePtr(mwk);
    wk->cursor[0] = NULL;
    wk->cursor[1] = NULL;
    wk->hblankTask = NULL;
    wk->str = GFL_StrBufCreate(128, HEAPID_PMS_INPUT);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, PMSIV_EDIT_MSG_FILE, HEAPID_PMS_INPUT);
    wk->bmpCursor = BmpCursor_Create(HEAPID_PMS_INPUT);
    wk->frameY = 0;
    wk->scrollY = 0;
    wk->unk1A8 = 0;
    wk->unk1AC = 0;
    wk->vblankTask = NULL;
    wk->frames = BGWinFrame_Create(1, 1, HEAPID_PMS_INPUT);
    return wk;
}

void PMSIVEdit_Delete(PMSIVEdit *wk) {
    BGWinFrame_Delete(wk->frames);
    if (wk->vblankTask) {
        GFL_TCBRemove(wk->vblankTask);
    }
    if (wk->hblankTask) {
        GFL_TCBRemove(wk->hblankTask);
    }
    if (wk->bmpCursor) {
        BmpCursor_Free(wk->bmpCursor);
    }
    if (wk->cursor[0]) {
        func_0204c108(wk->cursor[0]);
    }
    if (wk->cursor[1]) {
        func_0204c108(wk->cursor[1]);
    }
    if (wk->msgData) {
        GFL_MsgDataFree(wk->msgData);
    }
    if (wk->str) {
        GFL_StrBufFree(wk->str);
    }
    BmpWin_Free(wk->win[0]);
    BmpWin_Free(wk->win[1]);
    GFL_HeapFree(wk);
}

void PMSIVEdit_SetupGraphicDatas(PMSIVEdit *wk, ArcTool *arc) {
    PMSIVEdit_SetupPalette(wk, arc);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 25, 0, 0, 0, FALSE, HEAPID_PMS_INPUT);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 25, 4, 0, 0, FALSE, HEAPID_PMS_INPUT);
    wk->scrollY = -192;
    GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, wk->scrollY);
    GFL_BGSysLoadArcNCGRStatic(arc, 14, 0, 0, 0, FALSE, HEAPID_PMS_INPUT);
    GFL_BGSysLoadArcNCGRStatic(arc, 14, 4, 0, 0, FALSE, HEAPID_PMS_INPUT);
    wk->win[0] = BmpWin_CreateDynamic(0, 3, 1, 28, 4, 0, TRUE);
    wk->win[1] = BmpWin_CreateDynamic(4, 3, 1, 28, 4, 0, TRUE);
    BmpWin_FlushMap(wk->win[0]);
    BmpWin_FlushMap(wk->win[1]);
    PMSIVEdit_SetupWordPos(wk);
    PMSIVEdit_SetupWordActors(wk);
    PMSIVEdit_UpdateEditArea(wk);
    PMSIVEdit_SetupCursorActors(wk);
    BGWinFrame_InitFrame(wk->frames, 0, 0, 32, 6);
    BGWinFrame_LoadScreenArc(wk->frames, 0, arc, 25, FALSE);
    BGWinFrame_Put(wk->frames, 0, 0, 0);
    BGWinFrame_WriteBmpWin(wk->frames, 0, wk->win[0]);
    BGWinFrame_Show(wk->frames, 0);
    wk->framesDirty = FALSE;
}

static void PMSIVEdit_HBlankTask(TCB *tcb, void *data) {
    GX_GetVCount();
}

static void PMSIVEdit_VBlankTask(TCB *tcb, void *data) {
    PMSIVEdit *wk = data;

    wk->unk1A8 = 0;
    wk->unk1AC = 0;
    if (wk->framesDirty) {
        GFL_BGSysFillScrArea(0, 0, 0, 0, 32, 6, BGSYS_FILL_TILE_PALETTE);
        BGWinFrame_Put(wk->frames, 0, 0, -wk->frameY / 8);
        BGWinFrame_Show(wk->frames, 0);
        wk->framesDirty = FALSE;
    }
}

void PMSIVEdit_ScrollSet(PMSIVEdit *wk, BOOL up) {
    wk->scrollCount = 0;
    wk->scrollUp = up;
    if (wk->hblankTask) {
        GFL_TCBRemove(wk->hblankTask);
    }
    wk->hblankTask = GFL_HBlankTCBAdd(PMSIVEdit_HBlankTask, wk, 1);
    wk->unk1A8 = 0;
    wk->unk1AC = 0;
    if (wk->vblankTask) {
        GFL_TCBRemove(wk->vblankTask);
    }
    wk->vblankTask = GFL_VBlankTCBAdd(PMSIVEdit_VBlankTask, wk, 1);
}

BOOL PMSIVEdit_ScrollWait(PMSIVEdit *wk) {
    int step;
    int i, j;
    s16 y;

    if (wk->scrollCount > 5) {
        return TRUE;
    }
    if (wk->scrollUp) {
        step = -8;
    } else {
        step = 8;
    }
    wk->scrollY = wk->scrollY + step;
    wk->frameY = wk->frameY + step;
    GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, wk->scrollY);
    for (i = 0; i < 2; i++) {
        y = func_0204c1dc(wk->cursor[i], i, 1) - step;
        func_0204c1a8(wk->cursor[i], y, i, 1);
        for (j = 0; j < 2; j++) {
            y = func_0204c1dc(wk->numberAct[j][i], i, 1) - step;
            func_0204c1a8(wk->numberAct[j][i], y, i, 1);
        }
    }
    wk->scrollCount++;
    // The count stops at 6, so the tasks are only removed by the next scroll or by PMSIVEdit_Delete
    if (wk->scrollUp && wk->scrollCount == 7) {
        GFL_TCBRemove(wk->hblankTask);
        wk->hblankTask = NULL;
        GFL_TCBRemove(wk->vblankTask);
        wk->vblankTask = NULL;
    }
    wk->framesDirty = TRUE;
    return FALSE;
}

static void PMSIVEdit_SetupPalette(PMSIVEdit *wk, ArcTool *arc) {
    NNSG2dPaletteData *palette;
    void *buf;

    GFL_G2DIOLoadArcNCLRDefault(arc, 3, 0, 0, 0x1c0, HEAPID_PMS_INPUT);
    GFL_G2DIOLoadArcNCLRDefault(arc, 4, 4, 0, 0xa0, HEAPID_PMS_INPUT);
    buf = GFL_G2DIOReadNCLRArc(arc, 6, &palette, HEAPID_PMS_INPUT);
    sys_memcpy16(palette->rawData, wk->palette, sizeof(wk->palette));
    cp15_flushDC(wk->palette, sizeof(wk->palette));
    GFL_HeapFree(buf);
}

static void PMSIVEdit_UpdatePalette(PMSIVEdit *wk) {
    if (PMSInput_GetInputMode(wk->mwk) == PMSI_MODE_SENTENCE && PMSInput_GetSentenceType(wk->mwk) < 5) {
        u32 offset = (PMSInput_GetSentenceType(wk->mwk) + 1) * 16;

        gfxUploadStdPaletteBGA(&wk->palette[offset], 0, 0x20);
        gfxUploadStdPaletteBGB(&wk->palette[offset], 0, 0x20);
    } else {
        gfxUploadStdPaletteBGA(wk->palette, 0, 0x20);
        gfxUploadStdPaletteBGB(wk->palette, 0, 0x20);
    }
}

static void PMSIVEdit_SetupWordPos(PMSIVEdit *wk) {
    switch (PMSInput_GetInputMode(wk->mwk)) {
    case PMSI_MODE_WORD:
        wk->wordPos[0].x = 104;
        wk->wordPos[0].y = 16;
        wk->wordCount = 1;
        break;
    case PMSI_MODE_DOUBLE_WORD:
        wk->wordPos[0].x = 48;
        wk->wordPos[0].y = 16;
        wk->wordPos[1].x = 160;
        wk->wordPos[1].y = 16;
        wk->wordCount = 2;
        break;
    case PMSI_MODE_SENTENCE:
        wk->wordCount = 0;
        break;
    }
}

static void PMSIVEdit_SetupWordActors(PMSIVEdit *wk) {
    PMSIVObjRes res;
    int i, j;

    for (i = 0; i < 2; i++) {
        PMSIView_GetObjRes2(wk->vwk, &res, i);
        for (j = 0; j < 2; j++) {
            wk->numberAct[j][i] = PMSIView_AddActor(wk->vwk, &res, 0, 0, 2, i + 1);
            func_0204c124(wk->numberAct[j][i], FALSE);
        }
    }
}

static void PMSIVEdit_SetupCursorActors(PMSIVEdit *wk) {
    PMSIVObjRes res;
    ClActorPos pos;

    if (wk->wordCount) {
        PMSIVEdit_GetWordCursorPos(&wk->wordPos[0], &pos);
    } else {
        pos.x = 128;
        pos.y = 24;
    }
    PMSIView_GetObjRes(wk->vwk, &res, 1, 0);
    wk->cursor[1] = PMSIView_AddActor(wk->vwk, &res, pos.x, pos.y + 192, 0, NNS_G2D_VRAM_TYPE_2DSUB);
    func_0204c488(wk->cursor[1], 1);
    func_0204c468(wk->cursor[1], 1);
    PMSIView_GetObjRes(wk->vwk, &res, 0, 0);
    wk->cursor[0] = PMSIView_AddActor(wk->vwk, &res, pos.x, pos.y, 0, NNS_G2D_VRAM_TYPE_2DMAIN);
    PMSIVEdit_SetCursorActive(wk, TRUE);
}

void PMSIVEdit_UpdateEditArea(PMSIVEdit *wk) {
    int lcd, i;

    PMSIVEdit_UpdatePalette(wk);
    for (lcd = 0; lcd < 2; lcd++) {
        for (i = 0; i < 2; i++) {
            if (wk->numberAct[i][lcd]) {
                func_0204c124(wk->numberAct[i][lcd], FALSE);
            }
        }
        GFL_BitmapFill(BmpWin_GetBitmap(wk->win[lcd]), 15);
        switch (PMSInput_GetInputMode(wk->mwk)) {
        case PMSI_MODE_WORD:
            PMSIVEdit_ClearWord(wk->win[lcd], &wk->wordPos[0]);
            PMSIVEdit_PrintWord(wk, wk->win[lcd], &wk->wordPos[0], PMSInput_GetEditWord(wk->mwk, 0),
                                wk->numberAct[0][lcd]);
            break;
        case PMSI_MODE_DOUBLE_WORD:
            PMSIVEdit_ClearWord(wk->win[lcd], &wk->wordPos[0]);
            PMSIVEdit_ClearWord(wk->win[lcd], &wk->wordPos[1]);
            PMSIVEdit_PrintWord(wk, wk->win[lcd], &wk->wordPos[0], PMSInput_GetEditWord(wk->mwk, 0),
                                wk->numberAct[0][lcd]);
            PMSIVEdit_PrintWord(wk, wk->win[lcd], &wk->wordPos[1], PMSInput_GetEditWord(wk->mwk, 1),
                                wk->numberAct[1][lcd]);
            break;
        case PMSI_MODE_SENTENCE:
            wk->wordCount = PMSIVEdit_PrintSentence(wk, wk->win[lcd], lcd);
            break;
        }
        BmpWin_FlushChar(wk->win[lcd]);
    }
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(4);
}

void PMSIVEdit_GetWordArea(PMSIVEdit *wk, TouchRect *rect, u32 index) {
    rect->left = wk->wordPos[index].x - 24;
    rect->right = rect->left + 96;
    rect->top = wk->wordPos[index].y;
    rect->bottom = rect->top + 16;
}

static u32 PMSIVEdit_PrintSentence(PMSIVEdit *wk, BmpWin *win, u8 lcd) {
    StrBuf *str;
    u16 index;
    BOOL loop;
    Font *font;
    u32 count;
    int y;
    int x;

    str = PMSInput_GetEditSourceString(wk->mwk, HEAPID_PMS_INPUT);
    font = PMSIView_GetFont(wk->vwk);
    loop = TRUE;
    count = 0;
    y = 0;
    x = 0;

    PMSIVEdit_ParserInit(&wk->parser, wk);
    while (loop) {
        switch (PMSIVEdit_ParserNext(&wk->parser, str)) {
        case PARSE_TEXT:
            GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
            GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), x, y, str, font);
            x += GFL_FontGetBlockWidth(str, font, 0);
            break;
        case PARSE_WORD:
            index = wk->parser.param;
            wk->wordIndex[count] = index;
            wk->wordPos[count].x = x + 50;
            wk->wordPos[count].y = y + 8;
            PMSIVEdit_ClearWord(win, &wk->wordPos[count]);
            PMSIVEdit_PrintWord(wk, win, &wk->wordPos[count], PMSInput_GetEditWord(wk->mwk, index),
                                wk->numberAct[count][lcd]);
            count++;
            x += 100;
            break;
        case PARSE_NEWLINE:
            y += 16;
            x = 0;
            break;
        case PARSE_END:
            loop = FALSE;
            break;
        }
    }
    PMSIVEdit_ParserEnd(&wk->parser);
    GFL_StrBufFree(str);
    return count;
}

static void PMSIVEdit_ParserInit(PMSIVEditParser *parser, PMSIVEdit *wk) {
    parser->str = PMSInput_GetEditSourceString(wk->mwk, HEAPID_PMS_INPUT);
    parser->cur = GFL_StrBufGetStringPtr(parser->str);
    if (*parser->cur == CHAR_COMMAND) {
        parser->state = PARSE_WORD;
    } else {
        parser->state = PARSE_TEXT;
    }
}

static void PMSIVEdit_ParserEnd(PMSIVEditParser *parser) {
    GFL_StrBufFree(parser->str);
}

static u32 PMSIVEdit_ParserNext(PMSIVEditParser *parser, StrBuf *buf) {
    const u16 *start = parser->cur;
    u32 type;

    switch (parser->state) {
    case PARSE_TEXT:
        while (parser->state == PARSE_TEXT) {
            switch (*parser->cur) {
            case CHAR_NEWLINE:
                parser->state = PARSE_NEWLINE;
                break;
            case CHAR_EOS:
                parser->state = PARSE_END;
                break;
            case CHAR_COMMAND:
                parser->state = PARSE_WORD;
                break;
            default:
                parser->cur++;
                break;
            }
        }
        GFL_StrBufLoadFixedString(buf, start, parser->cur - start + 1);
        return PARSE_TEXT;
    case PARSE_WORD:
        parser->param = GFL_WordSetGetCommandParameter(start, 0);
        parser->cur = GFL_StrCmdSkipCommand(parser->cur);
        type = PARSE_WORD;
        break;
    case PARSE_NEWLINE:
        parser->cur = start + 1;
        type = PARSE_NEWLINE;
        break;
    case PARSE_END:
    default:
        return PARSE_END;
    }
    switch (*parser->cur) {
    case CHAR_NEWLINE:
        parser->state = PARSE_NEWLINE;
        break;
    case CHAR_EOS:
        parser->state = PARSE_END;
        break;
    case CHAR_COMMAND:
        parser->state = PARSE_WORD;
        break;
    default:
        parser->state = PARSE_TEXT;
        break;
    }
    return type;
}

static void PMSIVEdit_GetWordRectPos(const ClActorPos *pos, ClActorPos *out) {
    out->x = pos->x - 48;
    out->y = pos->y - 8;
}

static void PMSIVEdit_GetWordCursorPos(const ClActorPos *pos, ClActorPos *out) {
    out->x = pos->x + 24;
    out->y = pos->y + 8;
}

static void PMSIVEdit_ClearWord(BmpWin *win, const ClActorPos *pos) {
    ClActorPos rectPos;

    PMSIVEdit_GetWordRectPos(pos, &rectPos);
    GFL_BitmapFillArea(BmpWin_GetBitmap(win), rectPos.x, rectPos.y, 96, 16, 14);
}

static void PMSIVEdit_PrintWord(PMSIVEdit *wk, BmpWin *win, const ClActorPos *pos, u16 word, ClActor *numberAct) {
    Font *font = PMSIView_GetFont(wk->vwk);
    ClActorPos rectPos;
    ClActorPos actPos;

    if (word == PMS_WORD_NULL) {
        return;
    }
    PMSIVEdit_GetWordRectPos(pos, &rectPos);
    if (PMSWord_IsNumber(word)) {
        u32 number = PMSWord_GetNumber(word) - 1;
        u32 engine = PMSIVEdit_GetBGEngine(BmpWin_GetBGIndex(win));

        actPos.x = rectPos.x + BmpWin_GetPosX(win) * 8;
        actPos.y = rectPos.y + BmpWin_GetPosY(win) * 8;
        if (engine == 1) {
            actPos.y += 192;
        }
        func_0204c488(numberAct, number);
        func_0204c140(numberAct, &actPos, engine);
        func_0204c124(numberAct, TRUE);
        func_0204c468(numberAct, GFL_BGSysGetBGPriority(BmpWin_GetBGIndex(win)));
    } else {
        loadSayingToString(word, wk->str, HEAPID_PMS_INPUT);
        GFL_TextRndUpdateColorIndexLUT(3, 4, 14);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), rectPos.x, rectPos.y, wk->str, font);
    }
}

static void PMSIVEdit_SetCursorActive(PMSIVEdit *wk, BOOL active) {
    if (wk->wordCount) {
        if (active) {
            func_0204c488(wk->cursor[0], 0);
        } else {
            func_0204c488(wk->cursor[0], 1);
        }
    } else {
        if (active) {
            func_0204c488(wk->cursor[0], 14);
        } else {
            func_0204c488(wk->cursor[0], 15);
        }
    }
}

static u32 PMSIVEdit_GetBGEngine(u32 bg) {
    if (bg >= BGSYS_BG_SUB) {
        return BGSYS_ENGINE_SUB;
    }
    return BGSYS_ENGINE_MAIN;
}

u32 PMSIVEdit_GetWordCount(PMSIVEdit *wk) {
    return wk->wordCount;
}

u16 PMSIVEdit_GetWordIndex(PMSIVEdit *wk, u32 index) {
    return wk->wordIndex[index];
}

void PMSIVEdit_StopCursor(PMSIVEdit *wk) {
    PMSIVEdit_SetCursorActive(wk, FALSE);
}

void PMSIVEdit_ActiveCursor(PMSIVEdit *wk) {
    PMSIVEdit_SetCursorActive(wk, TRUE);
}

void PMSIVEdit_VisibleCursor(PMSIVEdit *wk, BOOL visible) {
    func_0204c124(wk->cursor[0], visible);
    func_0204c124(wk->cursor[1], visible);
    PMSIVEdit_SetCursorActive(wk, TRUE);
}

void PMSIVEdit_StopArrow(PMSIVEdit *wk) {
}

void PMSIVEdit_ActiveArrow(PMSIVEdit *wk) {
}

void PMSIVEdit_MoveCursor(PMSIVEdit *wk, u32 pos) {
    ClActorPos actPos;
    ClActorPos cursorPos;

    if (wk->wordCount) {
        PMSIVEdit_GetWordCursorPos(&wk->wordPos[pos], &cursorPos);
        actPos.x = cursorPos.x;
        actPos.y = cursorPos.y;
    } else {
        actPos.x = 128;
        actPos.y = 24;
    }
    func_0204c140(wk->cursor[0], &actPos, CLACT_SURFACE_MAIN);
    actPos.y += 192;
    func_0204c140(wk->cursor[1], &actPos, CLACT_SURFACE_SUB);
    PMSIVEdit_SetCursorActive(wk, TRUE);
}
