#include "app/pms_input.h"
#include "types.h"
#include "app/pms_input_data.h"
#include "app/pms_input_view.h"
#include "app/pmsi_initial_data.h"
#include "app/pmsi_search.h"
#include "app/ui/ui_scene.h"
#include "constants/sound.h"
#include "gfl/button_man.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "system/pms.h"
#include "system/pms_data.h"
#include "system/pmsi_param.h"

// The phrase input: the player writes one word, two words or a sentence, choosing each word from a group of words or
// from the words of an initial, in a window of words, with the keys or the touch screen. The names are ours, guessed


// Where the input is driven from
#define KEY_MODE_KEY 0
#define KEY_MODE_TOUCH 1

// What the cursor on the buttons is on
enum {
    BUTTON_POS_DECIDE,
    BUTTON_POS_CANCEL,
};

// The categories' cursor's places besides a category: the search, erase and back buttons, and no place
#define CATEGORY_POS_SEARCH 0xfc
#define CATEGORY_POS_ERASE 0xfd
#define CATEGORY_POS_BACK 0xfe
#define CATEGORY_POS_NONE 0xff

// The word groups, and the sentence types the input offers
#define GROUP_COUNT 12
#define SENTENCE_TYPE_COUNT 5

// What a key or a touch on the word window does
enum {
    WORDWIN_RESULT_NONE,
    WORDWIN_RESULT_CURSOR,
    WORDWIN_RESULT_SCROLL,
    WORDWIN_RESULT_SCROLL_CURSOR,
    WORDWIN_RESULT_IGNORE,
    WORDWIN_RESULT_DECIDE,
};

// How many words the word window shows, two to a row
#define WORDWIN_VISIBLE_WORDS 14
#define WORDWIN_ROWS 6

typedef BOOL (*PMSInputMainProc)(PMSInputWork *wk, u32 *seq);
typedef void (*PMSInputSubProc)(PMSInputWork *wk, u32 *seq);
typedef void (*PMSInputKTChangeFunc)(PMSInputWork *wk, u32 *seq);
typedef BOOL (*PMSInputCursorFunc)(PMSInputWork *wk);

typedef struct {
    s16 type;
    s8 id;
    s8 count;
} PMSInputSentenceWork;

typedef struct {
    u16 top;
    u16 scrollMax;
    u16 wordMax;
    u8 x;
    u8 y;
    int scrollVector;
    u16 onBack;
    u16 selected;
} PMSInputWordWin;

typedef struct {
    u8 pos;
    u8 max;
} PMSInputMenu;

struct PMSInputWork {
    PMSIParam *param;
    u32 inputMode;
    PMSData sentence;
    u16 words[PMS_SENTENCE_WORD_MAX];
    PMSInputView *vwk;
    PMSInputData *dwk;
    PMSISearch *search;
    int mainSeq;
    PMSInputMainProc mainProc;
    PMSInputMainProc nextProc;
    int subSeq;
    PMSInputSubProc subProc;
    u16 keyTrg;
    u16 keyCont;
    u16 keyRepeat;
    // The mode button's state: 0 while it is touched
    u32 modeButton;
    u32 unk40;
    ButtonMan *bmn;
    u16 buttonPos;
    u16 editPos;
    u16 categoryPos;
    u16 categoryPosPrev;
    u16 categoryPosSaved;
    PMSInputWordWin wordWin;
    PMSInputSentenceWork sentenceWk;
    u8 sentenceEditPosMax;
    u8 categoryMode;
    u8 scrollBarHeld;
    PMSInputMenu menu;
    int keyMode;
    PMSInputKTChangeFunc ktChangeFunc;
    void *tcbMem;
    TCBManager *tcbMgr;
};

static void PMSInput_ButtonCallback(u32 button, u32 event, void *work);
static PMSInputWork *PMSInput_ConstructWork(GameProc *proc, PMSIParam *param);
static void PMSInput_SetupSentenceWork(PMSInputSentenceWork *swk, const PMSData *sentence);
static void PMSInput_SentenceIncrement(PMSInputSentenceWork *swk, PMSData *sentence);
static void PMSInput_SentenceDecrement(PMSInputSentenceWork *swk, PMSData *sentence);
static void PMSInput_SentenceSetDirect(PMSInputSentenceWork *swk, PMSData *sentence, int type);
static void PMSInput_DestructWork(PMSInputWork *wk, GameProc *proc);
static void PMSInput_ChangeMainProc(PMSInputWork *wk, PMSInputMainProc proc);
static BOOL PMSInput_KeyStatusChange(PMSInputWork *wk, u32 *seq);
static void PMSInput_StartWordWin(PMSInputWork *wk);
static BOOL PMSInput_MainProcEditArea(PMSInputWork *wk, u32 *seq);
static void PMSInput_EditAreaKTChange(PMSInputWork *wk, u32 *seq);
static void PMSInput_CategoryKTChange(PMSInputWork *wk, u32 *seq);
static void PMSInput_WordWinKTChange(PMSInputWork *wk, u32 *seq);
static BOOL PMSInput_WordKey(PMSInputWork *wk, u32 *seq);
static s32 PMSInput_WordTouchSingle(PMSInputWork *wk);
static s32 PMSInput_WordTouchDouble(PMSInputWork *wk);
static s32 PMSInput_SentenceTouchArea(PMSInputWork *wk);
static s32 PMSInput_SentenceTouchType(PMSInputWork *wk);
static BOOL PMSInput_WordTouch(PMSInputWork *wk, u32 *seq);
static BOOL PMSInput_MainProcWord(PMSInputWork *wk, u32 *seq);
static BOOL PMSInput_DoubleWordKey(PMSInputWork *wk, u32 *seq);
static BOOL PMSInput_MainProcDoubleWord(PMSInputWork *wk, u32 *seq);
static BOOL PMSInput_SentenceKey(PMSInputWork *wk, u32 *seq);
static BOOL PMSInput_SentenceTouch(PMSInputWork *wk, u32 *seq);
static BOOL PMSInput_MainProcSentence(PMSInputWork *wk, u32 *seq);
static BOOL PMSInput_MainProcCommandButton(PMSInputWork *wk, u32 *seq);
static BOOL PMSInput_MainProcCategory(PMSInputWork *wk, u32 *seq);
static void PMSInput_CategoryKey(PMSInputWork *wk, u32 *seq);
static int PMSInput_CategoryTouchButtons(PMSInputWork *wk);
static s32 PMSInput_TouchReturnButton(PMSInputWork *wk);
static int PMSInput_CategoryTouchGroup(PMSInputWork *wk);
static s32 PMSInput_CategoryTouchSearchButtons(PMSInputWork *wk);
static int PMSInput_CategoryTouchInitial(PMSInputWork *wk);
static void PMSInput_CategoryTouch(PMSInputWork *wk, u32 *seq);
static void PMSInput_CategoryInput(PMSInputWork *wk, u32 *seq);
static BOOL PMSInput_IsCategoryEnabled(PMSInputWork *wk);
static BOOL PMSInput_CategoryMoveCursor(PMSInputWork *wk);
static BOOL PMSInput_CategoryKeyGroup(PMSInputWork *wk);
static BOOL PMSInput_CategoryKeyInitial(PMSInputWork *wk);
static void PMSInput_SetupWordWin(PMSInputWordWin *ww, PMSInputWork *wk);
static u32 PMSInput_WordWinGetCursorPos(const PMSInputWordWin *ww);
static int PMSInput_WordWinGetPos(const PMSInputWordWin *ww);
static int PMSInput_WordWinGetScrollVector(const PMSInputWordWin *ww);
static u16 PMSInput_WordWinGetTop(const PMSInputWordWin *ww);
static u16 PMSInput_WordWinGetScrollMax(const PMSInputWordWin *ww);
static BOOL PMSInput_MainProcWordWin(PMSInputWork *wk, u32 *seq);
static void PMSInput_WordWinKey(PMSInputWork *wk, u32 *seq);
static int PMSInput_WordWinTouchWord(PMSInputWork *wk);
static BOOL PMSInput_WordWinTouchScrollBar(PMSInputWork *wk);
static void PMSInput_WordWinTouch(PMSInputWork *wk, u32 *seq);
static void PMSInput_WordWinInput(PMSInputWork *wk, u32 *seq);
static int PMSInput_WordWinCheckKey(PMSInputWordWin *ww, u16 key);
static int PMSInput_WordWinPageUp(PMSInputWordWin *ww);
static int PMSInput_WordWinPageDown(PMSInputWordWin *ww);
static int PMSInput_WordWinScrollBarUp(PMSInputView *vwk, PMSInputWordWin *ww);
static int PMSInput_WordWinScrollBarDown(PMSInputView *vwk, PMSInputWordWin *ww);
static int PMSInput_WordWinScrollBarMove(PMSInputView *vwk, PMSInputWordWin *ww);
static BOOL PMSInput_SetSelectWord(PMSInputWork *wk);
static BOOL PMSInput_MainProcQuit(PMSInputWork *wk, u32 *seq);
static void PMSInput_SetSubProc(PMSInputWork *wk, PMSInputSubProc proc);
static void PMSInput_QuitSubProc(PMSInputWork *wk);
static void PMSInput_SubProcFadeIn(PMSInputWork *wk, u32 *seq);
static void PMSInput_SubProcCommandOK(PMSInputWork *wk, u32 *seq);
static void PMSInput_SubProcCommandCancel(PMSInputWork *wk, u32 *seq);
static BOOL PMSInput_CheckInputComplete(PMSInputWork *wk);
static void PMSInput_InitMenuState(PMSInputMenu *menu, u8 max, u8 pos);

const GameProcFunctions PMS_INPUT_PROC_FUNCTIONS = {
    PMSInput_Init,
    PMSInput_Main,
    PMSInput_Exit,
};

BOOL PMSInput_Init(GameProc *proc, u32 *state, void *param, void *work) {
    PMSInputWork *wk;

    switch (*state) {
    case 0:
        GFL_OvlLoad(OVERLAY_APP_UI);
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_PMS_INPUT_SYS, 0x18000);
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_PMS_INPUT, 0x32000);
        wk = PMSInput_ConstructWork(proc, param);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_INIT);
        (*state)++;
        break;
    case 1:
        wk = work;
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            if (wk->inputMode == PMSI_MODE_SENTENCE) {
                wk->sentenceEditPosMax = PMSIView_GetSentenceEditPosMax(wk->vwk);
            } else {
                wk->sentenceEditPosMax = 0;
            }
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_KTCHANGE_EDITAREA);
            (*state)++;
        }
        break;
    case 2:
        wk = work;
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcEditArea);
            func_02042ba8(TRUE, HEAPID_PMS_INPUT);
            return TRUE;
        }
        break;
    }
    GFL_TCBMgrUpdate(wk->tcbMgr);
    return FALSE;
}

BOOL PMSInput_Main(GameProc *proc, u32 *state, void *param, void *work) {
    PMSInputWork *wk = work;
    BOOL ret;

    wk->keyTrg = GCTX_HIDGetPressedKeys();
    wk->keyCont = GCTX_HIDGetHeldKeys();
    wk->keyRepeat = GCTX_HIDGetTypedKeys();
    GFL_BMN_Main(wk->bmn);
    if (wk->subProc != NULL) {
        wk->subProc(wk, &wk->subSeq);
        ret = FALSE;
    } else {
        ret = wk->mainProc(wk, &wk->mainSeq);
    }
    GFL_TCBMgrUpdate(wk->tcbMgr);
    return ret;
}

static void PMSInput_ButtonCallback(u32 button, u32 event, void *work) {
    PMSInputWork *wk = work;

    switch (event) {
    case BMN_EVENT_TOUCH:
        wk->modeButton = button;
        break;
    case BMN_EVENT_RELEASE:
    case BMN_EVENT_SLIDEOUT:
        wk->modeButton = 1;
        break;
    case BMN_EVENT_HOLD:
        break;
    default:
        wk->modeButton = 1;
        break;
    }
}

BOOL PMSInput_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    PMSInput_DestructWork(work, proc);
    GFL_HeapDelete(HEAPID_PMS_INPUT_SYS);
    GFL_HeapDelete(HEAPID_PMS_INPUT);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static PMSInputWork *PMSInput_ConstructWork(GameProc *proc, PMSIParam *param) {
    // The mode button, whose touch the button manager follows
    static const TouchRect sPMSInputModeButtonRects[] = {
        { 168, 192, 0, 48 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };

    PMSInputWork *wk = GFL_ProcInitSubsystem(proc, sizeof(PMSInputWork), HEAPID_PMS_INPUT_SYS);

    wk->param = param;
    wk->inputMode = PMSIParam_GetMode(param);
    wk->keyMode = func_02029b40(wk->param);
    PMSIParam_GetResult(wk->param, wk->words, &wk->sentence);
    if (wk->inputMode == PMSI_MODE_SENTENCE) {
        PMSInput_SetupSentenceWork(&wk->sentenceWk, &wk->sentence);
    }
    wk->tcbMem = GFL_HeapAllocate(HEAPID_PMS_INPUT_SYS, GFL_TCBMgrCalcAllocSize(5), FALSE, "pms_input.c", 595);
    wk->tcbMgr = GFL_TCBMgrCreate(5, wk->tcbMem);
    wk->dwk = PMSIData_Create(HEAPID_PMS_INPUT_SYS, wk->param);
    wk->vwk = PMSIView_Create(wk, wk->dwk);
    wk->bmn = GFL_BMN_Create(sPMSInputModeButtonRects, PMSInput_ButtonCallback, wk, HEAPID_PMS_INPUT_SYS);
    wk->categoryMode = 0;
    wk->subProc = NULL;
    wk->subSeq = 0;
    wk->editPos = 0;
    wk->buttonPos = BUTTON_POS_DECIDE;
    wk->modeButton = 1;
    wk->search = PMSISearch_Create(wk, wk->dwk, HEAPID_PMS_INPUT_SYS);
    PMSInput_SetSubProc(wk, PMSInput_SubProcFadeIn);
    return wk;
}

static void PMSInput_SetupSentenceWork(PMSInputSentenceWork *swk, const PMSData *sentence) {
    swk->type = PMSData_GetType(sentence);
    swk->count = PMSData_GetSentenceCount(swk->type);
    swk->id = PMSData_GetID(sentence);
}

static void PMSInput_SentenceIncrement(PMSInputSentenceWork *swk, PMSData *sentence) {
    swk->id++;
    if (swk->id >= swk->count) {
        swk->id = 0;
        swk->type++;
        if (swk->type >= SENTENCE_TYPE_COUNT) {
            swk->type = 0;
        }
        swk->count = PMSData_GetSentenceCount(swk->type);
    }
    PMSData_SetSentence(sentence, swk->type, swk->id);
}

static void PMSInput_SentenceDecrement(PMSInputSentenceWork *swk, PMSData *sentence) {
    swk->id--;
    if (swk->id < 0) {
        swk->type--;
        if (swk->type < 0) {
            swk->type = SENTENCE_TYPE_COUNT - 1;
        }
        swk->count = PMSData_GetSentenceCount(swk->type);
        swk->id = swk->count - 1;
    }
    PMSData_SetSentence(sentence, swk->type, swk->id);
}

static void PMSInput_SentenceSetDirect(PMSInputSentenceWork *swk, PMSData *sentence, int type) {
    swk->id = 0;
    swk->type = type;
    swk->count = PMSData_GetSentenceCount(swk->type);
    PMSData_SetSentence(sentence, swk->type, swk->id);
}

static void PMSInput_DestructWork(PMSInputWork *wk, GameProc *proc) {
    func_02029b48(wk->param, wk->keyMode);
    GFL_BMN_Delete(wk->bmn);
    PMSIView_Delete(wk->vwk);
    func_0203a610(wk->tcbMgr);
    GFL_HeapFree(wk->tcbMem);
    PMSIData_Delete(wk->dwk);
    PMSISearch_Delete(wk->search);
    GFL_ProcReleaseSubsystem(proc);
}

static void PMSInput_ChangeMainProc(PMSInputWork *wk, PMSInputMainProc proc) {
    wk->mainProc = proc;
    wk->mainSeq = 0;
    if (proc == PMSInput_MainProcEditArea) {
        PMSIView_ChangeKTEditArea(wk->vwk, wk, wk->dwk);
        wk->ktChangeFunc = PMSInput_EditAreaKTChange;
    } else if (proc == PMSInput_MainProcCategory) {
        PMSIView_ChangeKTCategory(wk->vwk, wk, wk->dwk);
        wk->ktChangeFunc = PMSInput_CategoryKTChange;
    } else if (proc == PMSInput_MainProcWordWin) {
        PMSIView_ChangeKTWordWin(wk->vwk, wk, wk->dwk);
        wk->ktChangeFunc = PMSInput_WordWinKTChange;
    } else {
        wk->ktChangeFunc = NULL;
    }
}

static BOOL PMSInput_KeyStatusChange(PMSInputWork *wk, u32 *seq) {
    if (wk->keyMode == KEY_MODE_TOUCH) {
        if (func_0203da2c()) {
            return FALSE;
        }
        if (GCTX_HIDGetHeldKeys()) {
            wk->keyMode = KEY_MODE_KEY;
            if (wk->ktChangeFunc) {
                wk->ktChangeFunc(wk, seq);
            }
            return TRUE;
        }
    } else {
        if (GCTX_HIDGetHeldKeys()) {
            return FALSE;
        }
        if (func_0203da2c()) {
            wk->keyMode = KEY_MODE_TOUCH;
            if (wk->ktChangeFunc) {
                wk->ktChangeFunc(wk, seq);
            }
            return TRUE;
        }
    }
    return FALSE;
}

static void PMSInput_StartWordWin(PMSInputWork *wk) {
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    PMSInput_SetupWordWin(&wk->wordWin, wk);
    wk->nextProc = PMSInput_MainProcWordWin;
    PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CATEGORY_TO_WORDWIN);
}

static BOOL PMSInput_MainProcEditArea(PMSInputWork *wk, u32 *seq) {
    static const PMSInputMainProc sPMSInputEditAreaProcs[] = {
        PMSInput_MainProcWord,
        PMSInput_MainProcDoubleWord,
        PMSInput_MainProcSentence,
    };

    return sPMSInputEditAreaProcs[wk->inputMode](wk, seq);
}

static void PMSInput_EditAreaKTChange(PMSInputWork *wk, u32 *seq) {
    PMSIView_ChangeKTEditArea(wk->vwk, wk, wk->dwk);
}

static void PMSInput_CategoryKTChange(PMSInputWork *wk, u32 *seq) {
    PMSIView_ChangeKTCategory(wk->vwk, wk, wk->dwk);
}

static void PMSInput_WordWinKTChange(PMSInputWork *wk, u32 *seq) {
    PMSIView_ChangeKTWordWin(wk->vwk, wk, wk->dwk);
}

static BOOL PMSInput_WordKey(PMSInputWork *wk, u32 *seq) {
    switch (*seq) {
    case 0:
        if (!PMSIView_WaitCommandAll(wk->vwk)) {
            break;
        }
        if (wk->keyTrg & PAD_KEY_UP) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->buttonPos = BUTTON_POS_CANCEL;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_BUTTON);
            *seq = 1;
        } else if (wk->keyTrg & (PAD_KEY_DOWN | PAD_BUTTON_START)) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->buttonPos = BUTTON_POS_DECIDE;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_BUTTON);
            *seq = 1;
        } else if (wk->keyTrg & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PMSInput_SetSubProc(wk, PMSInput_SubProcCommandCancel);
        } else if (wk->keyTrg & PAD_BUTTON_A) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PMSInput_ResetCategoryPos(wk);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_CATEGORY);
            *seq = 5;
        }
        break;
    case 1:
        if (!PMSIView_WaitCommandAll(wk->vwk)) {
            break;
        }
        if (wk->keyTrg & PAD_KEY_UP) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (wk->buttonPos != BUTTON_POS_DECIDE) {
                wk->buttonPos ^= 1;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
            } else {
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_BUTTON_TO_EDITAREA_SELECT);
                *seq = 0;
            }
        } else if (wk->keyTrg & PAD_KEY_DOWN) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (wk->buttonPos != BUTTON_POS_DECIDE) {
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_BUTTON_TO_EDITAREA_SELECT);
                *seq = 0;
            } else {
                wk->buttonPos ^= 1;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
            }
        } else if (wk->keyTrg & PAD_BUTTON_START) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->buttonPos = BUTTON_POS_DECIDE;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
        } else if (wk->keyTrg & PAD_BUTTON_A) {
            if (wk->buttonPos == BUTTON_POS_DECIDE) {
                if (PMSInput_CheckInputComplete(wk)) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                } else {
                    GFL_SndSEPlay(SEQ_SE_BEEP);
                }
                *seq = 2;
            } else {
                GFL_SndSEPlay(SEQ_SE_CANCEL1);
                *seq = 3;
            }
        } else if (wk->keyTrg & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            *seq = 3;
        }
        break;
    case 2:
        *seq = 4;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandOK);
        break;
    case 3:
        *seq = 4;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandCancel);
        break;
    case 4:
        *seq = 1;
        break;
    case 6:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcCommandButton);
        }
        break;
    case 5:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcCategory);
        }
        break;
    }
    return FALSE;
}

static s32 PMSInput_WordTouchSingle(PMSInputWork *wk) {
    // The edit area's OK and quit buttons, and its words
    static const TouchRect sPMSInputWordSingleRects[] = {
        { 144, 168, 184, 255 },
        { 168, 192, 184, 255 },
        { 16, 32, 80, 176 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };

    return func_0203da0c(sPMSInputWordSingleRects);
}

static s32 PMSInput_WordTouchDouble(PMSInputWork *wk) {
    static const TouchRect sPMSInputWordDoubleRects[] = {
        { 144, 168, 184, 255 }, { 168, 192, 184, 255 },      { 16, 32, 24, 120 },
        { 16, 32, 136, 232 },   { TOUCH_RECT_END, 0, 0, 0 },
    };

    return func_0203da0c(sPMSInputWordDoubleRects);
}

static s32 PMSInput_SentenceTouchArea(PMSInputWork *wk) {
    s32 hit;
    int count, i;
    u32 x, y;
    TouchRect rects[2] = {
        { TOUCH_RECT_END, 0, 0, 0 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };
    // The OK and quit buttons, the arrows that change the sentence on each side, and the sentence's arrows on the upper
    // screen
    static const TouchRect sPMSInputSentenceRects[] = {
        { 144, 168, 184, 255 }, { 168, 192, 184, 255 }, { 168, 192, 0, 24 },         { 168, 192, 144, 168 },
        { 8, 39, 0, 19 },       { 8, 39, 236, 255 },    { TOUCH_RECT_END, 0, 0, 0 },
    };

    if (!func_0203da48()) {
        return -1;
    }
    hit = func_0203da0c(sPMSInputSentenceRects);
    if (hit != -1) {
        if (PMSIParam_HasStartSentence(wk->param)) {
            if (hit == 2 || hit == 3 || hit == 4 || hit == 5) {
                hit = -1;
            }
            return hit;
        }
        return hit;
    }
    count = PMSIView_GetSentenceEditPosMax(wk->vwk);
    func_0203dac8(&x, &y);
    for (i = 0; i < count; i++) {
        PMSIView_GetSentenceWordArea(wk->vwk, rects, i);
        if (func_0203dadc(rects, x, y) != -1) {
            return i + 6;
        }
    }
    return -1;
}

static s32 PMSInput_SentenceTouchType(PMSInputWork *wk) {
    u32 x, y;
    s32 type;

    if (PMSIParam_HasStartSentence(wk->param)) {
        return -1;
    }
    if (!func_0203dac8(&x, &y)) {
        return -1;
    }
    if (y <= 168 || (type = x / 24 - 1) < 0 || type >= SENTENCE_TYPE_COUNT) {
        type = -1;
    }
    return type;
}

static BOOL PMSInput_WordTouch(PMSInputWork *wk, u32 *seq) {
    s32 hit;

    switch (*seq) {
    case 0:
    case 1:
        if (wk->inputMode == PMSI_MODE_WORD) {
            hit = PMSInput_WordTouchSingle(wk);
        } else {
            hit = PMSInput_WordTouchDouble(wk);
        }
        switch (hit) {
        case 0:
            if (PMSInput_CheckInputComplete(wk)) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            } else {
                GFL_SndSEPlay(SEQ_SE_BEEP);
            }
            *seq = 2;
            break;
        case 1:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            *seq = 3;
            break;
        case 2:
        case 3:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PMSInput_ResetCategoryPos(wk);
            wk->editPos = hit - 2;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_CATEGORY);
            *seq = 5;
            break;
        }
        break;
    case 2:
        *seq = 4;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandOK);
        break;
    case 3:
        *seq = 4;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandCancel);
        break;
    case 4:
        *seq = 0;
        break;
    case 5:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcCategory);
        }
        break;
    }
    return FALSE;
}

static BOOL PMSInput_MainProcWord(PMSInputWork *wk, u32 *seq) {
    if (*seq <= 1 && PMSInput_KeyStatusChange(wk, seq) && wk->keyMode == KEY_MODE_KEY) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return FALSE;
    }
    if (wk->keyMode == KEY_MODE_KEY) {
        return PMSInput_WordKey(wk, seq);
    }
    return PMSInput_WordTouch(wk, seq);
}

static BOOL PMSInput_DoubleWordKey(PMSInputWork *wk, u32 *seq) {
    switch (*seq) {
    case 0:
        if (!PMSIView_WaitCommandAll(wk->vwk)) {
            break;
        }
        if (wk->keyTrg & PAD_KEY_UP) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->buttonPos = BUTTON_POS_CANCEL;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_BUTTON);
            *seq = 1;
        } else if (wk->keyTrg & (PAD_KEY_DOWN | PAD_BUTTON_START)) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->buttonPos = BUTTON_POS_DECIDE;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_BUTTON);
            *seq = 1;
        } else if (wk->keyTrg & PAD_KEY_LEFT) {
            if (wk->editPos != 0) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
                wk->editPos = 0;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_EDITAREA_CURSOR);
            }
        } else if (wk->keyTrg & PAD_KEY_RIGHT) {
            if (wk->editPos == 0) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
                wk->editPos = 1;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_EDITAREA_CURSOR);
            }
        } else if (wk->keyTrg & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PMSInput_SetSubProc(wk, PMSInput_SubProcCommandCancel);
        } else if (wk->keyTrg & PAD_BUTTON_A) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PMSInput_ResetCategoryPos(wk);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_CATEGORY);
            *seq = 5;
        }
        break;
    case 1:
        if (!PMSIView_WaitCommandAll(wk->vwk)) {
            break;
        }
        if (wk->keyTrg & PAD_KEY_UP) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (wk->buttonPos != BUTTON_POS_DECIDE) {
                wk->buttonPos ^= 1;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
            } else {
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_BUTTON_TO_EDITAREA_SELECT);
                *seq = 0;
            }
        } else if (wk->keyTrg & PAD_KEY_DOWN) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (wk->buttonPos != BUTTON_POS_DECIDE) {
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_BUTTON_TO_EDITAREA_SELECT);
                *seq = 0;
            } else {
                wk->buttonPos ^= 1;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
            }
        } else if (wk->keyTrg & PAD_BUTTON_START) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->buttonPos = BUTTON_POS_DECIDE;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
        } else if (wk->keyTrg & PAD_BUTTON_A) {
            if (wk->buttonPos == BUTTON_POS_DECIDE) {
                if (PMSInput_CheckInputComplete(wk)) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                } else {
                    GFL_SndSEPlay(SEQ_SE_BEEP);
                }
                *seq = 2;
            } else {
                GFL_SndSEPlay(SEQ_SE_CANCEL1);
                *seq = 3;
            }
        } else if (wk->keyTrg & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            *seq = 3;
        }
        break;
    case 2:
        *seq = 4;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandOK);
        break;
    case 3:
        *seq = 4;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandCancel);
        break;
    case 4:
        *seq = 1;
        break;
    case 6:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcCommandButton);
        }
        break;
    case 5:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcCategory);
        }
        break;
    }
    return FALSE;
}

static BOOL PMSInput_MainProcDoubleWord(PMSInputWork *wk, u32 *seq) {
    if (*seq <= 1 && PMSInput_KeyStatusChange(wk, seq) && wk->keyMode == KEY_MODE_KEY) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return FALSE;
    }
    if (wk->keyMode == KEY_MODE_KEY) {
        return PMSInput_DoubleWordKey(wk, seq);
    }
    return PMSInput_WordTouch(wk, seq);
}

static BOOL PMSInput_SentenceKey(PMSInputWork *wk, u32 *seq) {
    switch (*seq) {
    case 0:
        wk->sentenceEditPosMax = PMSIView_GetSentenceEditPosMax(wk->vwk);
        *seq = 1;
        // fallthrough
    case 1:
        if (PMSIParam_HasStartSentence(wk->param)) {
            if (wk->keyRepeat & PAD_KEY_LEFT) {
                break;
            }
            if (wk->keyRepeat & PAD_KEY_RIGHT) {
                break;
            }
        }
        if (wk->keyTrg & PAD_KEY_UP) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (wk->editPos == 0) {
                wk->buttonPos = BUTTON_POS_CANCEL;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_BUTTON);
                *seq = 2;
            } else if (wk->sentenceEditPosMax > 1 && wk->editPos != 0) {
                wk->editPos--;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_EDITAREA_CURSOR);
            }
        } else if (wk->keyTrg & PAD_KEY_DOWN) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (wk->sentenceEditPosMax > 1 && wk->editPos < wk->sentenceEditPosMax - 1) {
                wk->editPos++;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_EDITAREA_CURSOR);
            } else {
                wk->buttonPos = BUTTON_POS_DECIDE;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_BUTTON);
                *seq = 2;
            }
        } else if ((wk->keyTrg & PAD_BUTTON_START) || ((wk->keyTrg & PAD_BUTTON_A) && wk->sentenceEditPosMax == 0)) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->buttonPos = BUTTON_POS_DECIDE;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_BUTTON);
            *seq = 2;
        } else if (wk->keyRepeat & PAD_KEY_LEFT) {
            GFL_SndSEPlay(SEQ_SE_SYS_37);
            wk->editPos = 0;
            PMSInput_SentenceDecrement(&wk->sentenceWk, &wk->sentence);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_UPDATE_EDITAREA);
            PMSIView_SetMenuButton(wk->vwk, 0);
            *seq = 3;
        } else if (wk->keyRepeat & PAD_KEY_RIGHT) {
            GFL_SndSEPlay(SEQ_SE_SYS_37);
            wk->editPos = 0;
            PMSInput_SentenceIncrement(&wk->sentenceWk, &wk->sentence);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_UPDATE_EDITAREA);
            PMSIView_SetMenuButton(wk->vwk, 1);
            *seq = 3;
        } else if (wk->keyTrg & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PMSInput_SetSubProc(wk, PMSInput_SubProcCommandCancel);
        } else if (wk->keyTrg & PAD_BUTTON_A) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PMSInput_ResetCategoryPos(wk);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_CATEGORY);
            *seq = 8;
        }
        break;
    case 3:
        if (PMSIView_WaitCommand(wk->vwk, PMSIV_CMD_UPDATE_EDITAREA)) {
            *seq = 0;
        }
        break;
    case 2:
        if (!PMSIView_WaitCommandAll(wk->vwk)) {
            break;
        }
        if (wk->keyTrg & PAD_KEY_UP) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (wk->buttonPos != BUTTON_POS_DECIDE) {
                wk->buttonPos ^= 1;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
            } else {
                wk->editPos = wk->sentenceEditPosMax != 0 ? wk->sentenceEditPosMax - 1 : 0;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_BUTTON_TO_EDITAREA_SELECT);
                *seq = 0;
            }
        } else if (wk->keyTrg & PAD_KEY_DOWN) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (wk->buttonPos != BUTTON_POS_DECIDE) {
                wk->editPos = 0;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_BUTTON_TO_EDITAREA_SELECT);
                *seq = 0;
            } else {
                wk->buttonPos ^= 1;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
            }
        } else if (wk->keyTrg & PAD_BUTTON_START) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->buttonPos = BUTTON_POS_DECIDE;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
        } else if (wk->keyTrg & PAD_BUTTON_A) {
            if (wk->buttonPos == BUTTON_POS_DECIDE) {
                if (PMSInput_CheckInputComplete(wk)) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                } else {
                    GFL_SndSEPlay(SEQ_SE_BEEP);
                }
                *seq = 4;
            } else {
                GFL_SndSEPlay(SEQ_SE_CANCEL1);
                *seq = 5;
            }
        } else if (wk->keyTrg & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            *seq = 5;
        }
        break;
    case 4:
        *seq = 6;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandOK);
        break;
    case 5:
        *seq = 6;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandCancel);
        break;
    case 6:
        *seq = 2;
        break;
    case 7:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcCommandButton);
        }
        break;
    case 8:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcCategory);
        }
        break;
    }
    return FALSE;
}

static BOOL PMSInput_SentenceTouch(PMSInputWork *wk, u32 *seq) {
    s32 hit;

    switch (*seq) {
    case 0:
        wk->sentenceEditPosMax = PMSIView_GetSentenceEditPosMax(wk->vwk);
        *seq = 1;
        // fallthrough
    case 1:
    case 2:
        hit = PMSInput_SentenceTouchType(wk);
        if (hit > -1) {
            wk->editPos = 0;
            PMSInput_SentenceSetDirect(&wk->sentenceWk, &wk->sentence, hit);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_UPDATE_EDITAREA);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            *seq = 3;
            break;
        }
        hit = PMSInput_SentenceTouchArea(wk);
        switch (hit) {
        case 0:
            if (PMSInput_CheckInputComplete(wk)) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            } else {
                GFL_SndSEPlay(SEQ_SE_BEEP);
            }
            wk->buttonPos = BUTTON_POS_DECIDE;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_BUTTON_TO_EDITAREA);
            *seq = hit + 4;
            break;
        case 1:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            wk->buttonPos = BUTTON_POS_CANCEL;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_BUTTON_TO_EDITAREA);
            *seq = hit + 4;
            break;
        case 2:
        case 4:
            GFL_SndSEPlay(SEQ_SE_SYS_37);
            wk->editPos = 0;
            PMSInput_SentenceDecrement(&wk->sentenceWk, &wk->sentence);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_UPDATE_EDITAREA);
            PMSIView_SetMenuButton(wk->vwk, 0);
            *seq = 3;
            break;
        case 3:
        case 5:
            GFL_SndSEPlay(SEQ_SE_SYS_37);
            wk->editPos = 0;
            PMSInput_SentenceIncrement(&wk->sentenceWk, &wk->sentence);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_UPDATE_EDITAREA);
            PMSIView_SetMenuButton(wk->vwk, 1);
            *seq = 3;
            break;
        case 6:
        case 7:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PMSInput_ResetCategoryPos(wk);
            wk->editPos = hit - 6;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_EDITAREA_TO_CATEGORY);
            *seq = 8;
            break;
        }
        break;
    case 3:
        if (PMSIView_WaitCommand(wk->vwk, PMSIV_CMD_UPDATE_EDITAREA)) {
            *seq = 0;
        }
        break;
    case 4:
        *seq = 6;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandOK);
        break;
    case 5:
        *seq = 6;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandCancel);
        break;
    case 6:
        *seq = 2;
        break;
    case 7:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcCommandButton);
        }
        break;
    case 8:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcCategory);
        }
        break;
    }
    return FALSE;
}

static BOOL PMSInput_MainProcSentence(PMSInputWork *wk, u32 *seq) {
    if (*seq <= 2 && PMSInput_KeyStatusChange(wk, seq) && wk->keyMode == KEY_MODE_KEY) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return FALSE;
    }
    if (wk->keyMode == KEY_MODE_KEY) {
        return PMSInput_SentenceKey(wk, seq);
    }
    return PMSInput_SentenceTouch(wk, seq);
}

static BOOL PMSInput_MainProcCommandButton(PMSInputWork *wk, u32 *seq) {
    switch (*seq) {
    case 0:
        if (!PMSIView_WaitCommandAll(wk->vwk)) {
            break;
        }
        if (wk->keyTrg & PAD_KEY_UP) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (wk->buttonPos != BUTTON_POS_DECIDE) {
                wk->buttonPos ^= 1;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
            } else {
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_BUTTON_TO_EDITAREA_SELECT);
                *seq = 0;
            }
        } else if (wk->keyTrg & PAD_KEY_DOWN) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (wk->buttonPos != BUTTON_POS_DECIDE) {
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_BUTTON_TO_EDITAREA_SELECT);
                *seq = 0;
            } else {
                wk->buttonPos ^= 1;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
            }
        } else if (wk->keyTrg & PAD_BUTTON_START) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->buttonPos = BUTTON_POS_DECIDE;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_BUTTON_CURSOR);
        } else if (wk->keyTrg & PAD_BUTTON_A) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            if (wk->buttonPos == BUTTON_POS_DECIDE) {
                *seq = 1;
            } else {
                *seq = 2;
            }
        } else if (wk->keyTrg & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            *seq = 2;
        }
        break;
    case 1:
        *seq = 3;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandOK);
        break;
    case 2:
        *seq = 4;
        PMSInput_SetSubProc(wk, PMSInput_SubProcCommandCancel);
        break;
    case 3:
    case 4:
        *seq = 0;
        break;
    case 5:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcEditArea);
        }
        break;
    }
    return FALSE;
}

static BOOL PMSInput_MainProcCategory(PMSInputWork *wk, u32 *seq) {
    switch (*seq) {
    case 0:
        if (PMSIView_WaitCommand(wk->vwk, PMSIV_CMD_MOVE_CATEGORY_CURSOR) &&
            PMSIView_WaitCommand(wk->vwk, PMSIV_CMD_MOVE_CATEGORY) &&
            PMSIView_WaitCommand(wk->vwk, PMSIV_CMD_SHOW_MENU)) {
            PMSInput_CategoryInput(wk, seq);
        }
        break;
    case 1:
        if (PMSIView_WaitCommand(wk->vwk, PMSIV_CMD_SHOW_MENU)) {
            if (wk->categoryPosSaved != CATEGORY_POS_ERASE) {
                wk->categoryPos = wk->categoryPosSaved;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_CATEGORY_CURSOR);
            }
            *seq = 0;
        }
        break;
    case 2:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            if (wk->nextProc == PMSInput_MainProcEditArea) {
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_KTCHANGE_EDITAREA);
            }
            *seq = 3;
        }
        break;
    case 3:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, wk->nextProc);
        }
        break;
    case 4:
        if (PMSIView_WaitCommand(wk->vwk, PMSIV_CMD_CHANGE_CATEGORY_MODE_ENABLE)) {
            *seq = 0;
        }
        break;
    }
    return FALSE;
}

static void PMSInput_CategoryKey(PMSInputWork *wk, u32 *seq) {
    if ((wk->keyTrg & PAD_BUTTON_START) && wk->categoryMode == 1) {
        wk->categoryPos = CATEGORY_POS_SEARCH;
        if (PMSISearch_Search(wk->search)) {
            PMSInput_StartWordWin(wk);
            *seq = 2;
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_CATEGORY_CURSOR);
        }
        return;
    }
    if (wk->modeButton == 0 || (wk->keyTrg & PAD_BUTTON_SELECT)) {
        GFL_SndSEPlay(SEQ_SE_SELECT4);
        wk->categoryMode ^= 1;
        PMSInput_ResetCategoryPos(wk);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CHANGE_CATEGORY_MODE_ENABLE);
        *seq = 4;
        return;
    }
    if (wk->keyTrg & PAD_BUTTON_B) {
        if (wk->categoryMode == 1 && PMSISearch_DelChar(wk->search)) {
            wk->categoryPosSaved = wk->categoryPos;
            *seq = 1;
            if (wk->categoryPos != CATEGORY_POS_ERASE) {
                wk->categoryPos = CATEGORY_POS_ERASE;
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_CATEGORY_CURSOR);
            }
            GFL_SndSEPlay(SEQ_SE_CANCEL3);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_SHOW_MENU);
            PMSISearch_Search(wk->search);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CHANGE_CATEGORY_MODE);
            return;
        }
        if (wk->categoryMode == 1 && wk->categoryPos != CATEGORY_POS_BACK) {
            wk->categoryPos = CATEGORY_POS_BACK;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_CATEGORY_CURSOR);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CATEGORY_TO_EDITAREA);
        wk->nextProc = PMSInput_MainProcEditArea;
        *seq = 2;
        return;
    }
    if (wk->keyTrg & PAD_BUTTON_A) {
        if (wk->categoryPos == CATEGORY_POS_SEARCH) {
            if (PMSISearch_Search(wk->search)) {
                PMSInput_StartWordWin(wk);
                *seq = 2;
            } else {
                GFL_SndSEPlay(SEQ_SE_BEEP);
            }
        } else if (wk->categoryPos == CATEGORY_POS_ERASE) {
            if (wk->categoryMode == 1 && PMSISearch_DelChar(wk->search)) {
                wk->categoryPosSaved = wk->categoryPos;
                *seq = 1;
                GFL_SndSEPlay(SEQ_SE_CANCEL3);
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_SHOW_MENU);
                PMSISearch_Search(wk->search);
                PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CHANGE_CATEGORY_MODE);
            } else {
                GFL_SndSEPlay(SEQ_SE_BEEP);
            }
        } else if (wk->categoryPos == CATEGORY_POS_BACK) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CATEGORY_TO_EDITAREA);
            wk->nextProc = PMSInput_MainProcEditArea;
            *seq = 2;
        } else if (wk->categoryMode == 0) {
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_CATEGORY);
            if (PMSInput_IsCategoryEnabled(wk)) {
                PMSInput_StartWordWin(wk);
                *seq = 2;
            } else {
                GFL_SndSEPlay(SEQ_SE_BEEP);
            }
        } else {
            PMSISearch_AddChar(wk->search, wk->categoryPos);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PMSISearch_Search(wk->search);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CHANGE_CATEGORY_MODE);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_CATEGORY);
        }
        return;
    }
    if (PMSInput_CategoryMoveCursor(wk)) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_CATEGORY_CURSOR);
    }
}

static int PMSInput_CategoryTouchButtons(PMSInputWork *wk) {
    // The categories' back and mode buttons
    static const TouchRect sPMSInputCategoryButtonRects[] = {
        { 168, 192, 232, 0 },
        { 168, 192, 0, 40 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };

    s32 hit = func_0203da0c(sPMSInputCategoryButtonRects);

    if (hit == -1) {
        return 0;
    }
    switch (hit) {
    case 0:
        return 1;
    case 1:
        if (wk->modeButton == 0) {
            return 2;
        }
        break;
    }
    return 0;
}

static s32 PMSInput_TouchReturnButton(PMSInputWork *wk) {
    static const TouchRect sPMSInputReturnButtonRects[] = {
        { 168, 191, 232, 255 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };

    return func_0203da0c(sPMSInputReturnButtonRects);
}

static int PMSInput_CategoryTouchGroup(PMSInputWork *wk) {
    u32 x, y;
    int row, col;
    TouchRect rects[2] = {
        { TOUCH_RECT_END, 0, 0, 0 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };

    if (!func_0203da48()) {
        return -1;
    }
    func_0203dac8(&x, &y);
    for (row = 0; row < 4; row++) {
        rects[0].top = row * 24 + 56;
        rects[0].bottom = rects[0].top + 16;
        for (col = 0; col < 3; col++) {
            rects[0].left = col * 80 + 12;
            rects[0].right = rects[0].left + 72;
            if (func_0203dadc(rects, x, y) != -1) {
                return row * 3 + col;
            }
        }
    }
    return -1;
}

static s32 PMSInput_CategoryTouchSearchButtons(PMSInputWork *wk) {
    TouchRect rects[4] = {
        { 168, 192, 40, 112 },
        { 168, 192, 112, 184 },
        { 168, 192, 184, 0 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };

    return func_0203da0c(rects);
}

static int PMSInput_CategoryTouchInitial(PMSInputWork *wk) {
    int count = PMSIInitial_GetCount();
    int i;

    for (i = 0; i < count; i++) {
        TouchRect rects[2] = {
            { 0, 0, 0, 0 },
            { TOUCH_RECT_END, 0, 0, 0 },
        };
        u32 x, y;

        PMSIInitial_GetPos(i, &x, &y);
        rects[0].top = y + 48;
        rects[0].bottom = y + 64;
        rects[0].left = x + 8;
        rects[0].right = x + 24;
        if (func_0203da0c(rects) != -1) {
            return i;
        }
    }
    return -1;
}

static void PMSInput_CategoryTouch(PMSInputWork *wk, u32 *seq) {
    int pos;

    switch (PMSInput_CategoryTouchButtons(wk)) {
    case 1:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CATEGORY_TO_EDITAREA);
        wk->nextProc = PMSInput_MainProcEditArea;
        *seq = 2;
        return;
    case 2:
        GFL_SndSEPlay(SEQ_SE_SELECT4);
        wk->categoryMode ^= 1;
        PMSInput_ResetCategoryPos(wk);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CHANGE_CATEGORY_MODE_ENABLE);
        *seq = 4;
        return;
    }
    if (wk->categoryMode == 0) {
        pos = PMSInput_CategoryTouchGroup(wk);
        if (pos < 0) {
            return;
        }
        wk->categoryPos = pos;
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_CATEGORY);
        if (pos >= GROUP_COUNT || PMSIData_GetCategoryWordCount(wk->dwk, wk->categoryPos) == 0) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            return;
        }
        PMSInput_StartWordWin(wk);
        *seq = 2;
        return;
    }
    pos = PMSInput_CategoryTouchInitial(wk);
    if (pos > -1) {
        wk->categoryPos = pos;
        PMSISearch_AddChar(wk->search, wk->categoryPos);
        PMSISearch_Search(wk->search);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CHANGE_CATEGORY_MODE);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_CATEGORY);
        return;
    }
    switch (PMSInput_CategoryTouchSearchButtons(wk)) {
    case -1:
        break;
    case 0:
        wk->categoryPos = CATEGORY_POS_SEARCH;
        if (PMSISearch_Search(wk->search)) {
            PMSInput_StartWordWin(wk);
            *seq = 2;
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        break;
    case 1:
        wk->categoryPos = CATEGORY_POS_ERASE;
        if (PMSISearch_DelChar(wk->search)) {
            wk->categoryPosSaved = wk->categoryPos;
            *seq = 1;
            GFL_SndSEPlay(SEQ_SE_CANCEL3);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_SHOW_MENU);
            PMSISearch_Search(wk->search);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CHANGE_CATEGORY_MODE);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        break;
    case 2:
        wk->categoryPos = CATEGORY_POS_BACK;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_CATEGORY_TO_EDITAREA);
        wk->nextProc = PMSInput_MainProcEditArea;
        *seq = 2;
        break;
    }
}

static void PMSInput_CategoryInput(PMSInputWork *wk, u32 *seq) {
    if (PMSInput_KeyStatusChange(wk, seq)) {
        if (wk->categoryMode == 0) {
            if (wk->keyMode == KEY_MODE_KEY && !(GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
                return;
            }
        } else if (wk->keyMode == KEY_MODE_KEY) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return;
        }
    }
    if (PMSIView_WaitCommand(wk->vwk, PMSIV_CMD_CHANGE_CATEGORY_MODE)) {
        if (wk->keyMode == KEY_MODE_KEY) {
            PMSInput_CategoryKey(wk, seq);
        } else {
            PMSInput_CategoryTouch(wk, seq);
        }
    }
}

static BOOL PMSInput_IsCategoryEnabled(PMSInputWork *wk) {
    if (PMSIData_GetCategoryWordCount(wk->dwk, wk->categoryPos)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL PMSInput_CategoryMoveCursor(PMSInputWork *wk) {
    static const PMSInputCursorFunc sPMSInputCategoryCursorFuncs[] = {
        PMSInput_CategoryKeyGroup,
        PMSInput_CategoryKeyInitial,
    };

    return sPMSInputCategoryCursorFuncs[wk->categoryMode](wk);
}

static BOOL PMSInput_CategoryKeyGroup(PMSInputWork *wk) {
    // Where the cursor goes from each group, up, down, left and right
    static const u8 sPMSInputGroupCursorMoves[GROUP_COUNT][4] = {
        { 9, 3, 2, 1 }, { 10, 4, 0, 2 }, { 11, 5, 1, 0 }, { 0, 6, 5, 4 },   { 1, 7, 3, 5 },  { 2, 8, 4, 3 },
        { 3, 9, 8, 7 }, { 4, 10, 6, 8 }, { 5, 11, 7, 6 }, { 6, 0, 11, 10 }, { 7, 1, 9, 11 }, { 8, 2, 10, 9 },
    };

    u16 pos = wk->categoryPos;
    u32 idx = pos;

    if (pos == CATEGORY_POS_BACK) {
        idx = GROUP_COUNT;
    }
    if (pos != CATEGORY_POS_BACK) {
        wk->categoryPosPrev = pos;
        if (wk->keyRepeat & PAD_KEY_UP) {
            wk->categoryPos = sPMSInputGroupCursorMoves[idx][0];
            return TRUE;
        }
        if (wk->keyRepeat & PAD_KEY_DOWN) {
            wk->categoryPos = sPMSInputGroupCursorMoves[idx][1];
            return TRUE;
        }
        if (wk->keyRepeat & PAD_KEY_LEFT) {
            wk->categoryPos = sPMSInputGroupCursorMoves[idx][2];
            return TRUE;
        }
        if (wk->keyRepeat & PAD_KEY_RIGHT) {
            wk->categoryPos = sPMSInputGroupCursorMoves[idx][3];
            return TRUE;
        }
    } else {
        wk->categoryPosPrev = CATEGORY_POS_BACK;
        if (wk->keyRepeat & PAD_KEY_UP) {
            wk->categoryPos = 11;
            return TRUE;
        }
        if (wk->keyRepeat & PAD_KEY_DOWN) {
            wk->categoryPos = 2;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL PMSInput_CategoryKeyInitial(PMSInputWork *wk) {
    u16 pos = wk->categoryPos;
    int next;

    if (pos != CATEGORY_POS_BACK && pos != CATEGORY_POS_SEARCH && pos != CATEGORY_POS_ERASE) {
        next = CATEGORY_POS_NONE;
        if (wk->keyRepeat & PAD_KEY_UP) {
            next = PMSIInitial_GetUp(pos);
        } else if (wk->keyRepeat & PAD_KEY_DOWN) {
            next = PMSIInitial_GetDown(pos);
        } else if (wk->keyRepeat & PAD_KEY_RIGHT) {
            next = PMSIInitial_GetRight(pos);
        } else if (wk->keyRepeat & PAD_KEY_LEFT) {
            next = PMSIInitial_GetLeft(pos);
        }
        if (next != CATEGORY_POS_NONE) {
            wk->categoryPosPrev = wk->categoryPos;
            wk->categoryPos = next;
            return TRUE;
        }
    } else if (wk->keyRepeat & PAD_KEY_UP) {
        next = wk->categoryPosPrev <= PMSI_INITIAL_COUNT - 1 ? PMSIInitial_GetBottom(wk->categoryPosPrev)
                                                             : CATEGORY_POS_NONE;
        if (wk->categoryPos == CATEGORY_POS_SEARCH) {
            if (next >= 20 && next <= 23) {
                wk->categoryPos = next;
            } else {
                wk->categoryPos = 22;
            }
        } else if (wk->categoryPos == CATEGORY_POS_ERASE) {
            if (next == 24 || next == 25 || next == 16) {
                wk->categoryPos = next;
            } else {
                wk->categoryPos = 25;
            }
        } else {
            if (next == 17 || next == 18 || next == 26) {
                wk->categoryPos = next;
            } else {
                wk->categoryPos = 26;
            }
        }
        return TRUE;
    } else if (wk->keyRepeat & PAD_KEY_DOWN) {
        next = wk->categoryPosPrev <= PMSI_INITIAL_COUNT - 1 ? PMSIInitial_GetBottom(wk->categoryPosPrev)
                                                             : CATEGORY_POS_NONE;
        if (wk->categoryPos == CATEGORY_POS_SEARCH) {
            if (next >= 0 && next <= 3) {
                wk->categoryPos = next;
            } else {
                wk->categoryPos = 2;
            }
        } else if (wk->categoryPos == CATEGORY_POS_ERASE) {
            if (next >= 4 && next <= 6) {
                wk->categoryPos = next;
            } else {
                wk->categoryPos = 5;
            }
        } else {
            if (next >= 7 && next <= 9) {
                wk->categoryPos = next;
            } else {
                wk->categoryPos = 8;
            }
        }
        return TRUE;
    } else if (wk->keyRepeat & PAD_KEY_LEFT) {
        switch (pos) {
        case CATEGORY_POS_BACK:
            wk->categoryPos = CATEGORY_POS_ERASE;
            break;
        case CATEGORY_POS_ERASE:
            wk->categoryPos = CATEGORY_POS_SEARCH;
            break;
        case CATEGORY_POS_SEARCH:
            wk->categoryPos = CATEGORY_POS_BACK;
            break;
        }
        return TRUE;
    } else if (wk->keyRepeat & PAD_KEY_RIGHT) {
        switch (pos) {
        case CATEGORY_POS_BACK:
            wk->categoryPos = CATEGORY_POS_SEARCH;
            break;
        case CATEGORY_POS_ERASE:
            wk->categoryPos = CATEGORY_POS_BACK;
            break;
        case CATEGORY_POS_SEARCH:
            wk->categoryPos = CATEGORY_POS_ERASE;
            break;
        }
        return TRUE;
    }
    return FALSE;
}

static void PMSInput_SetupWordWin(PMSInputWordWin *ww, PMSInputWork *wk) {
    ww->top = 0;
    ww->x = 0;
    ww->y = 0;
    ww->wordMax = PMSInput_GetCategoryWordMax(wk);
    ww->scrollVector = 0;
    ww->onBack = 0;
    ww->selected = PMS_WORD_NULL;
    if (ww->wordMax > WORDWIN_VISIBLE_WORDS) {
        ww->scrollMax = (ww->wordMax - WORDWIN_VISIBLE_WORDS) / 2 + (ww->wordMax & 1);
    } else {
        ww->scrollMax = 0;
    }
}

static u32 PMSInput_WordWinGetCursorPos(const PMSInputWordWin *ww) {
    return ww->x + ww->y * 2;
}

static int PMSInput_WordWinGetPos(const PMSInputWordWin *ww) {
    return ww->top * 2 + PMSInput_WordWinGetCursorPos(ww);
}

static int PMSInput_WordWinGetScrollVector(const PMSInputWordWin *ww) {
    return ww->scrollVector;
}

static u16 PMSInput_WordWinGetTop(const PMSInputWordWin *ww) {
    return ww->top;
}

static u16 PMSInput_WordWinGetScrollMax(const PMSInputWordWin *ww) {
    return ww->scrollMax;
}

static BOOL PMSInput_MainProcWordWin(PMSInputWork *wk, u32 *seq) {
    switch (*seq) {
    case 0:
        PMSInput_WordWinInput(wk, seq);
        break;
    case 1:
        if (PMSIView_WaitCommand(wk->vwk, PMSIV_CMD_SCROLL_WORDWIN)) {
            *seq = 0;
        }
        break;
    case 2:
        if (PMSIView_WaitCommand(wk->vwk, PMSIV_CMD_SCROLL_WORDWIN)) {
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_WORDWIN_CURSOR);
            *seq = 0;
        }
        break;
    case 3:
        if (PMSIView_WaitCommand(wk->vwk, PMSIV_CMD_SET_WORDWIN_ARROWS)) {
            *seq = 0;
        }
        break;
    case 4:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_ChangeMainProc(wk, wk->nextProc);
            *seq = 0;
        }
        break;
    }
    return FALSE;
}

static void PMSInput_WordWinKey(PMSInputWork *wk, u32 *seq) {
    switch (PMSInput_WordWinCheckKey(&wk->wordWin, wk->keyRepeat)) {
    case WORDWIN_RESULT_CURSOR:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_WORDWIN_CURSOR);
        return;
    case WORDWIN_RESULT_SCROLL:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_SCROLL_WORDWIN);
        *seq = 1;
        return;
    case WORDWIN_RESULT_SCROLL_CURSOR:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_SCROLL_WORDWIN);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_WORDWIN_CURSOR);
        *seq = 2;
        return;
    }
    if (wk->keyTrg & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_WORDWIN_TO_CATEGORY);
        wk->nextProc = PMSInput_MainProcCategory;
        *seq = 4;
        return;
    }
    if (wk->keyTrg & PAD_BUTTON_A) {
        if (wk->wordWin.onBack) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_WORDWIN_TO_CATEGORY);
            wk->nextProc = PMSInput_MainProcCategory;
            *seq = 4;
            return;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        wk->wordWin.selected = PMS_WORD_NULL;
        PMSInput_SetSelectWord(wk);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_WORDWIN);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_WORDWIN_TO_EDITAREA);
        wk->nextProc = PMSInput_MainProcEditArea;
        *seq = 4;
    }
}

static int PMSInput_WordWinTouchWord(PMSInputWork *wk) {
    u32 x, y;
    int row, col;
    u16 pos;
    BOOL done;
    TouchRect rects[2] = {
        { TOUCH_RECT_END, 0, 0, 0 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };
    // The scroll bar of the word window
    static const TouchRect sPMSInputWordWinScrollBarRects[] = {
        { 4, 163, 236, 251 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };

    if (func_0203da0c(sPMSInputWordWinScrollBarRects) == 0) {
        func_0203dac8(&x, &y);
        switch (PMSIView_GetWordWinScrollDir(wk->vwk, x, y)) {
        case 0:
            wk->scrollBarHeld = TRUE;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return WORDWIN_RESULT_NONE;
        case 1:
            wk->scrollBarHeld = TRUE;
            PMSIView_SetWordWinScrollBarY(wk->vwk, y);
            return PMSInput_WordWinScrollBarUp(wk->vwk, &wk->wordWin);
        case 2:
            wk->scrollBarHeld = TRUE;
            PMSIView_SetWordWinScrollBarY(wk->vwk, y);
            return PMSInput_WordWinScrollBarDown(wk->vwk, &wk->wordWin);
        case 3:
            break;
        }
    }
    func_0203dac8(&x, &y);
    pos = wk->wordWin.top * 2;
    done = FALSE;
    for (row = 0; row < WORDWIN_ROWS; row++) {
        rects[0].top = row * 24 + 13;
        rects[0].bottom = rects[0].top + 22;
        for (col = 0; col < 2; col++) {
            rects[0].left = col * 112 + 16;
            rects[0].right = rects[0].left + 102;
            if (func_0203dadc(rects, x, y) != -1) {
                wk->wordWin.selected = pos;
                wk->wordWin.x = col;
                wk->wordWin.y = row;
                return WORDWIN_RESULT_DECIDE;
            }
            pos++;
            if (pos >= wk->wordWin.wordMax - 2) {
                done = TRUE;
                break;
            }
        }
        if (done) {
            break;
        }
    }
    return WORDWIN_RESULT_NONE;
}

static BOOL PMSInput_WordWinTouchScrollBar(PMSInputWork *wk) {
    u32 x, y;

    if (func_0203da84(&x, &y) == TRUE && wk->scrollBarHeld == TRUE) {
        PMSIView_SetWordWinScrollBarY(wk->vwk, y);
        return TRUE;
    }
    wk->scrollBarHeld = FALSE;
    return FALSE;
}

static void PMSInput_WordWinTouch(PMSInputWork *wk, u32 *seq) {
    if (PMSInput_TouchReturnButton(wk) == 0) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_WORDWIN_TO_CATEGORY);
        wk->nextProc = PMSInput_MainProcCategory;
        *seq = 4;
        return;
    }
    if (PMSInput_WordWinTouchScrollBar(wk) == TRUE) {
        switch (PMSInput_WordWinScrollBarMove(wk->vwk, &wk->wordWin)) {
        case WORDWIN_RESULT_SCROLL:
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_SET_WORDWIN_ARROWS);
            *seq = 3;
            return;
        case WORDWIN_RESULT_SCROLL_CURSOR:
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_SET_WORDWIN_ARROWS);
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_WORDWIN_CURSOR);
            *seq = 3;
            return;
        }
        return;
    }
    switch (PMSInput_WordWinTouchWord(wk)) {
    case WORDWIN_RESULT_DECIDE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        PMSInput_SetSelectWord(wk);
        wk->nextProc = PMSInput_MainProcEditArea;
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_WORDWIN);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_WORDWIN_TO_EDITAREA);
        *seq = 4;
        break;
    case WORDWIN_RESULT_CURSOR:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_WORDWIN_CURSOR);
        break;
    case WORDWIN_RESULT_SCROLL:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_SET_WORDWIN_ARROWS);
        *seq = 3;
        break;
    case WORDWIN_RESULT_SCROLL_CURSOR:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_SCROLL_WORDWIN);
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_MOVE_WORDWIN_CURSOR);
        *seq = 2;
        break;
    }
}

static void PMSInput_WordWinInput(PMSInputWork *wk, u32 *seq) {
    if (PMSInput_KeyStatusChange(wk, seq) && wk->keyMode == KEY_MODE_KEY &&
        !(GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return;
    }
    if (wk->keyMode == KEY_MODE_KEY) {
        PMSInput_WordWinKey(wk, seq);
    } else {
        PMSInput_WordWinTouch(wk, seq);
    }
}

static int PMSInput_WordWinCheckKey(PMSInputWordWin *ww, u16 key) {
    u32 pos;

    if (key & PAD_KEY_DOWN) {
        if (ww->onBack) {
            return WORDWIN_RESULT_IGNORE;
        }
        if (ww->y < WORDWIN_ROWS - 1) {
            ww->y++;
            pos = PMSInput_WordWinGetPos(ww);
            if (pos < ww->wordMax - 2) {
                return WORDWIN_RESULT_CURSOR;
            }
            if (pos == ww->wordMax - 2) {
                if (pos & 1) {
                    ww->x = 0;
                    return WORDWIN_RESULT_CURSOR;
                }
                ww->y--;
                return WORDWIN_RESULT_IGNORE;
            }
            ww->y--;
            return WORDWIN_RESULT_IGNORE;
        }
        if (ww->top < ww->scrollMax) {
            ww->scrollVector = 1;
            ww->top++;
            pos = PMSInput_WordWinGetPos(ww);
            if (pos < ww->wordMax - 2) {
                return WORDWIN_RESULT_SCROLL;
            }
            ww->x = 0;
            return WORDWIN_RESULT_SCROLL_CURSOR;
        }
        return WORDWIN_RESULT_IGNORE;
    }
    if (key & PAD_KEY_UP) {
        if (ww->onBack) {
            ww->onBack = 0;
            return WORDWIN_RESULT_CURSOR;
        }
        if (ww->y != 0) {
            ww->y--;
            return WORDWIN_RESULT_CURSOR;
        }
        if (ww->top != 0) {
            ww->scrollVector = -1;
            ww->top--;
            return WORDWIN_RESULT_SCROLL;
        }
        return WORDWIN_RESULT_IGNORE;
    }
    if (key & PAD_KEY_LEFT) {
        if (ww->x == 1) {
            ww->x ^= 1;
            pos = PMSInput_WordWinGetPos(ww);
            if (pos < ww->wordMax - 2) {
                return WORDWIN_RESULT_CURSOR;
            }
            ww->x ^= 1;
        }
        return WORDWIN_RESULT_IGNORE;
    }
    if (key & PAD_KEY_RIGHT) {
        if (ww->x == 0) {
            ww->x ^= 1;
            pos = PMSInput_WordWinGetPos(ww);
            if (pos < ww->wordMax - 2) {
                return WORDWIN_RESULT_CURSOR;
            }
            ww->x ^= 1;
        }
        return WORDWIN_RESULT_IGNORE;
    }
    if (key & PAD_BUTTON_L) {
        return PMSInput_WordWinPageUp(ww);
    }
    if (key & PAD_BUTTON_R) {
        return PMSInput_WordWinPageDown(ww);
    }
    return WORDWIN_RESULT_NONE;
}

static int PMSInput_WordWinPageUp(PMSInputWordWin *ww) {
    if (ww->top != 0) {
        if (ww->top >= WORDWIN_ROWS) {
            ww->top -= WORDWIN_ROWS;
            ww->scrollVector = -WORDWIN_ROWS;
        } else {
            ww->scrollVector = -ww->top;
            ww->top = 0;
        }
        return WORDWIN_RESULT_SCROLL;
    }
    return WORDWIN_RESULT_IGNORE;
}

static int PMSInput_WordWinPageDown(PMSInputWordWin *ww) {
    if (ww->top < ww->scrollMax) {
        int next = ww->top + WORDWIN_ROWS;

        if (next <= ww->scrollMax) {
            ww->scrollVector = WORDWIN_ROWS;
            ww->top = next;
        } else {
            ww->scrollVector = ww->scrollMax - ww->top;
            ww->top = ww->scrollMax;
        }
        if (PMSInput_WordWinGetPos(ww) < ww->wordMax - 2) {
            return WORDWIN_RESULT_SCROLL;
        }
        ww->x = 0;
        return WORDWIN_RESULT_SCROLL_CURSOR;
    }
    return WORDWIN_RESULT_IGNORE;
}

static int PMSInput_WordWinScrollBarUp(PMSInputView *vwk, PMSInputWordWin *ww) {
    if (ww->top != 0) {
        ww->top = PMSIView_GetWordWinScrollBarPos(vwk, ww->scrollMax);
        ww->scrollVector = 0;
        if (PMSInput_WordWinGetPos(ww) < ww->wordMax - 2) {
            return WORDWIN_RESULT_SCROLL;
        }
        ww->x = 0;
        return WORDWIN_RESULT_SCROLL_CURSOR;
    }
    return WORDWIN_RESULT_IGNORE;
}

static int PMSInput_WordWinScrollBarDown(PMSInputView *vwk, PMSInputWordWin *ww) {
    if (ww->top < ww->scrollMax) {
        ww->top = PMSIView_GetWordWinScrollBarPos(vwk, ww->scrollMax);
        ww->scrollVector = 0;
        if (PMSInput_WordWinGetPos(ww) < ww->wordMax - 2) {
            return WORDWIN_RESULT_SCROLL;
        }
        ww->x = 0;
        return WORDWIN_RESULT_SCROLL_CURSOR;
    }
    return WORDWIN_RESULT_IGNORE;
}

static int PMSInput_WordWinScrollBarMove(PMSInputView *vwk, PMSInputWordWin *ww) {
    u16 top = ww->top;

    ww->top = PMSIView_GetWordWinScrollBarPos(vwk, ww->scrollMax);
    ww->scrollVector = 0;
    if (top != ww->top) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    }
    if (PMSInput_WordWinGetPos(ww) < ww->wordMax - 2) {
        return WORDWIN_RESULT_SCROLL;
    }
    ww->x = 0;
    return WORDWIN_RESULT_SCROLL_CURSOR;
}

static BOOL PMSInput_SetSelectWord(PMSInputWork *wk) {
    u32 pos = wk->wordWin.selected;
    u16 word;

    if (pos == PMS_WORD_NULL) {
        pos = PMSInput_WordWinGetPos(&wk->wordWin);
    }
    if (wk->categoryMode == 0) {
        word = PMSIData_GetCategoryWordCode(wk->dwk, wk->categoryPos, pos);
    } else {
        word = PMSISearch_GetResultWord(wk->search, pos);
    }
    switch (wk->inputMode) {
    case PMSI_MODE_WORD:
        wk->words[0] = word;
        break;
    case PMSI_MODE_DOUBLE_WORD:
        wk->words[wk->editPos] = word;
        break;
    case PMSI_MODE_SENTENCE:
        PMSData_SetWord(&wk->sentence, PMSIView_GetSentenceWord(wk->vwk, wk->editPos), word);
        break;
    }
    return PMSInput_CheckInputComplete(wk);
}

static BOOL PMSInput_MainProcQuit(PMSInputWork *wk, u32 *seq) {
    switch (*seq) {
    case 0:
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_QUIT);
        (*seq)++;
        break;
    case 1:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void PMSInput_SetSubProc(PMSInputWork *wk, PMSInputSubProc proc) {
    wk->subProc = proc;
    wk->subSeq = 0;
}

static void PMSInput_QuitSubProc(PMSInputWork *wk) {
    wk->subProc = NULL;
}

static void PMSInput_SubProcFadeIn(PMSInputWork *wk, u32 *seq) {
    switch (*seq) {
    case 0:
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_FADEIN);
        (*seq)++;
        break;
    case 1:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_QuitSubProc(wk);
        }
        break;
    }
}

static void PMSInput_SubProcCommandOK(PMSInputWork *wk, u32 *seq) {
    switch (*seq) {
    case 0:
        if (!PMSIView_WaitCommandAll(wk->vwk)) {
            break;
        }
        if (PMSInput_CheckInputComplete(wk)) {
            PMSInput_InitMenuState(&wk->menu, 1, 0);
            wk->buttonPos = BUTTON_POS_DECIDE;
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_PUSH_BUTTON);
            *seq = 1;
        } else {
            *seq = 3;
        }
        break;
    case 1:
        switch (PMSIView_GetMenuButton(wk->vwk)) {
        case 0:
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_UPDATE_EDITAREA_CURSOR);
            *seq = 3;
            break;
        case 1:
            if (wk->inputMode == PMSI_MODE_SENTENCE) {
                PMSData_ClearUnusedWords(&wk->sentence, HEAPID_PMS_INPUT);
            }
            PMSIParam_SetResult(wk->param, wk->words, &wk->sentence);
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcQuit);
            *seq = 3;
            break;
        }
        break;
    case 2:
        if (wk->keyTrg & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_PLUS_KEY_MASK)) {
            *seq = 3;
        }
        break;
    case 3:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_QuitSubProc(wk);
        }
        break;
    }
}

static void PMSInput_SubProcCommandCancel(PMSInputWork *wk, u32 *seq) {
    switch (*seq) {
    case 0:
        PMSInput_InitMenuState(&wk->menu, 1, 1);
        wk->buttonPos = BUTTON_POS_CANCEL;
        PMSIView_SetCommand(wk->vwk, PMSIV_CMD_PUSH_BUTTON);
        *seq = 1;
        break;
    case 1:
        switch (PMSIView_GetMenuButton(wk->vwk)) {
        case 0:
            PMSIView_SetCommand(wk->vwk, PMSIV_CMD_UPDATE_EDITAREA_CURSOR);
            *seq = 2;
            break;
        case 1:
            PMSInput_ChangeMainProc(wk, PMSInput_MainProcQuit);
            *seq = 2;
            break;
        }
        break;
    case 2:
        if (PMSIView_WaitCommandAll(wk->vwk)) {
            PMSInput_QuitSubProc(wk);
        }
        break;
    }
}

static BOOL PMSInput_CheckInputComplete(PMSInputWork *wk) {
    switch (wk->inputMode) {
    case PMSI_MODE_WORD:
        if (wk->words[0] != PMS_WORD_NULL) {
            return TRUE;
        }
        return FALSE;
    case PMSI_MODE_DOUBLE_WORD:
        if (wk->words[0] != PMS_WORD_NULL && wk->words[1] != PMS_WORD_NULL) {
            return TRUE;
        }
        return FALSE;
    case PMSI_MODE_SENTENCE:
        return PMSData_IsComplete(&wk->sentence, HEAPID_PMS_INPUT);
    }
    return FALSE;
}

static void PMSInput_InitMenuState(PMSInputMenu *menu, u8 max, u8 pos) {
    menu->pos = pos;
    menu->max = max;
}

int *PMSInput_GetKeyModePtr(const PMSInputWork *wk) {
    return (int *)&wk->keyMode;
}

u32 PMSInput_GetInputMode(const PMSInputWork *wk) {
    return wk->inputMode;
}

u32 PMSInput_GetCategoryMode(const PMSInputWork *wk) {
    return wk->categoryMode;
}

u16 PMSInput_GetSentenceType(const PMSInputWork *wk) {
    return PMSData_GetType(&wk->sentence);
}

u16 PMSInput_GetEditWord(const PMSInputWork *wk, u32 index) {
    if (wk->inputMode == PMSI_MODE_SENTENCE) {
        return PMSData_GetWord(&wk->sentence, index);
    }
    return wk->words[index];
}

StrBuf *PMSInput_GetEditSourceString(const PMSInputWork *wk, HeapID heapId) {
    return PMSData_GetSentenceString(&wk->sentence, heapId);
}

u32 PMSInput_GetEditAreaCursorPos(const PMSInputWork *wk) {
    return wk->editPos;
}

u32 PMSInput_GetButtonCursorPos(const PMSInputWork *wk) {
    return wk->buttonPos;
}

u32 PMSInput_GetCategoryCursorPos(const PMSInputWork *wk) {
    return wk->categoryPos;
}

u32 PMSInput_GetCategoryPosSaved(const PMSInputWork *wk) {
    return wk->categoryPosSaved;
}

u32 PMSInput_GetCategoryWordMax(const PMSInputWork *wk) {
    if (wk->categoryMode == 0) {
        return PMSIData_GetCategoryWordCount(wk->dwk, wk->categoryPos) + 2;
    }
    return PMSISearch_GetResultCount(wk->search) + 2;
}

void PMSInput_GetCategoryWord(const PMSInputWork *wk, u32 index, StrBuf *buf) {
    if (wk->categoryMode == 0) {
        PMSIData_GetCategoryWord(wk->dwk, wk->categoryPos, index, buf);
    } else {
        PMSISearch_GetResultStr(wk->search, index, buf);
    }
}

u32 PMSInput_GetWordWinCursorPos(const PMSInputWork *wk) {
    if (wk->wordWin.onBack) {
        return -1;
    }
    return PMSInput_WordWinGetCursorPos(&wk->wordWin);
}

int PMSInput_GetWordWinScrollVector(const PMSInputWork *wk) {
    return PMSInput_WordWinGetScrollVector(&wk->wordWin);
}

BOOL PMSInput_GetWordWinUpArrowVisible(const PMSInputWork *wk) {
    return PMSInput_WordWinGetTop(&wk->wordWin);
}

BOOL PMSInput_GetWordWinDownArrowVisible(const PMSInputWork *wk) {
    return PMSInput_WordWinGetScrollMax(&wk->wordWin);
}

BOOL PMSInput_HasStartSentence(const PMSInputWork *wk) {
    return PMSIParam_HasStartSentence(wk->param);
}

TCBManager *PMSInput_GetTCBManager(const PMSInputWork *wk) {
    return wk->tcbMgr;
}

void PMSInput_GetSearchInputStr(const PMSInputWork *wk, StrBuf *buf) {
    PMSISearch_GetInputStr(wk->search, buf);
}

void PMSInput_ResetSearch(const PMSInputWork *wk) {
    PMSISearch_Reset(wk->search);
}

void PMSInput_GetWordWinScroll(const PMSInputWork *wk, u16 *top, u16 *scrollMax) {
    *top = wk->wordWin.top;
    *scrollMax = wk->wordWin.scrollMax;
}

u32 PMSInput_GetSearchResultCount(const PMSInputWork *wk) {
    return PMSISearch_GetResultCount(wk->search);
}

u32 PMSInput_GetSearchInputLen(const PMSInputWork *wk) {
    return PMSISearch_GetInputLen(wk->search);
}

void PMSInput_GetSearchResultStr(const PMSInputWork *wk, u32 index, StrBuf *buf) {
    PMSISearch_GetResultStr(wk->search, index, buf);
}

BOOL PMSInput_IsEditComplete(const PMSInputWork *wk) {
    return PMSInput_CheckInputComplete((PMSInputWork *)wk);
}

void PMSInput_ResetCategoryPos(PMSInputWork *wk) {
    wk->categoryPos = 0;
    wk->categoryPosPrev = CATEGORY_POS_NONE;
    wk->categoryPosSaved = CATEGORY_POS_ERASE;
}
