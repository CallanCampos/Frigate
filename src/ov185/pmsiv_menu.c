#include "app/pmsiv_menu.h"
#include "types.h"
#include "app/pms_input.h"
#include "app/pms_input_view.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/pmsi_param.h"

// The phrase input's buttons on the lower screen: task menu buttons for the edit area, the categories and the search,
// a row of buttons for the sentence types whose chosen one pulses in color, two arrows, and the return button. The
// names are ours, guessed

// The message file of the phrase input's texts
#define PMSIV_MENU_MSG_FILE TEXT_BANK_PMS_INPUT

// The sentence type buttons, and the button that goes back from the categories
#define PMSIV_MENU_TYPE_BUTTON_COUNT 8
#define PMSIV_MENU_TYPE_BUTTON_BACK 7
// The sentence types that have a button
#define PMSIV_MENU_SENTENCE_TYPE_COUNT 5

// The return button's states
enum {
    RETURN_SEQ_NONE,
    RETURN_SEQ_START,
    RETURN_SEQ_WAIT,
    RETURN_SEQ_END,
};

// The task menu buttons' colors
#define PMSIV_MENU_TEXT_COLOR 0x39e3
#define PMSIV_MENU_BUTTON_PALETTE 8
#define PMSIV_MENU_BUTTON_PALETTE_OFF 14
#define PMSIV_MENU_BUTTON_COLOR_1 0x3545
#define PMSIV_MENU_BUTTON_COLOR_2 0x7b2c

// The colors that pulse on the chosen sentence type's button
#define PMSIV_MENU_PULSE_COLOR_COUNT 5

struct PMSIVMenu {
    PMSInputView *vwk;
    const PMSInputWork *mwk;
    const PMSInputData *dwk;
    int *keyMode;
    PMSIVObjRes typeButtonRes;
    u32 returnPalette;
    u32 returnChars;
    u32 returnCellAnims;
    ClActor *returnButton;
    int returnSeq;
    MsgData *msgData;
    AppTaskMenuRes *taskMenuRes[3];
    AppTaskMenuItem items[3];
    AppTaskMenuWin *buttons[3];
    ClActor *typeButtons[PMSIV_MENU_TYPE_BUTTON_COUNT];
    ClActor *arrows[2];
    GXRgb pulseFrom[PMSIV_MENU_PULSE_COLOR_COUNT];
    GXRgb pulseTo[PMSIV_MENU_PULSE_COLOR_COUNT];
    GXRgb pulse[PMSIV_MENU_PULSE_COLOR_COUNT];
    int pulseAngle;
};

typedef struct {
    u8 x;
    u8 y;
    u8 anim;
} PMSIVMenuArrow;

static void PMSIVMenu_Clear(PMSIVMenu *wk);
static void PMSIVMenu_StartModeButton(PMSIVMenu *wk);
static BOOL PMSIVMenu_WaitModeButton(PMSIVMenu *wk);
static void PMSIVMenu_StartBackButton(PMSIVMenu *wk);
static BOOL PMSIVMenu_WaitBackButton(PMSIVMenu *wk);
static void PMSIVMenu_EndBackButton(PMSIVMenu *wk);
static void PMSIVMenu_StartSearchButton(PMSIVMenu *wk);
static BOOL PMSIVMenu_WaitSearchButton(PMSIVMenu *wk);
static void PMSIVMenu_StartEraseButton(PMSIVMenu *wk);
static BOOL PMSIVMenu_WaitEraseButton(PMSIVMenu *wk);
static void PMSIVMenu_SetupTypeButtons(PMSIVMenu *wk);
static void PMSIVMenu_FreeTypeButtons(PMSIVMenu *wk);
static void PMSIVMenu_SetTypeButtonAnim(PMSIVMenu *wk, u32 button, BOOL on);
static void PMSIVMenu_SetupArrows(PMSIVMenu *wk);
static void PMSIVMenu_FreeArrows(PMSIVMenu *wk);
static void PMSIVMenu_SetupReturnButton(PMSIVMenu *wk);
static void PMSIVMenu_ShowReturnButton(PMSIVMenu *wk);
static void PMSIVMenu_SetupSearchButtons(PMSIVMenu *wk);
static void PMSIVMenu_SetupColors(PMSIVMenu *wk);
static void PMSIVMenu_FreeColors(PMSIVMenu *wk);
static void PMSIVMenu_UpdateColors(PMSIVMenu *wk);
static void PMSIVMenu_StartTypeButtonPulse(PMSIVMenu *wk, u32 button);
static void PMSIVMenu_EndTypeButtonPulse(PMSIVMenu *wk, u32 button);

// The button of each sentence type
static const u8 sPMSIVMenuTypeButtons[PMSIV_MENU_SENTENCE_TYPE_COUNT] = { 1, 2, 3, 4, 5 };

static const PMSIVMenuArrow sPMSIVMenuArrows[2] = {
    { 8, 20, 17 },
    { 240, 20, 18 },
};

// The animations of each sentence type button, off and on
static const u8 sPMSIVMenuTypeButtonAnims[PMSIV_MENU_TYPE_BUTTON_COUNT][2] = {
    { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 8, 9 }, { 10, 11 }, { 12, 13 }, { 14, 15 },
};

PMSIVMenu *PMSIVMenu_Create(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk) {
    PMSIVMenu *wk = GFL_HeapAllocate(HEAPID_PMS_INPUT, sizeof(PMSIVMenu), TRUE, "pmsiv_menu.c", 309);
    int i;

    wk->vwk = vwk;
    wk->mwk = mwk;
    wk->dwk = dwk;
    wk->keyMode = PMSInput_GetKeyModePtr(mwk);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, PMSIV_MENU_MSG_FILE, HEAPID_PMS_INPUT);
    for (i = 0; i < 3; i++) {
        wk->taskMenuRes[i] = AppTaskMenuRes_Create(0, PMSIV_MENU_BUTTON_PALETTE, PMSIView_GetFont(wk->vwk),
                                                   PMSIView_GetPrintQueue(wk->vwk), HEAPID_PMS_INPUT);
    }
    GFL_BGSysLoadNCLRDefault(ARCID_PMSI, 9, 0, 0x1c0, 0x40, HEAPID_PMS_INPUT);
    PMSIVMenu_SetupReturnButton(wk);
    PMSIVMenu_SetupTypeButtons(wk);
    PMSIVMenu_SetupArrows(wk);
    PMSIVMenu_SetupColors(wk);
    return wk;
}

void PMSIVMenu_Delete(PMSIVMenu *wk) {
    int i;

    PMSIVMenu_Clear(wk);
    func_0204c108(wk->returnButton);
    func_0204bcd0(wk->returnPalette);
    func_0204b98c(wk->returnChars);
    func_0204be64(wk->returnCellAnims);
    for (i = 0; i < 3; i++) {
        AppTaskMenuRes_Free(wk->taskMenuRes[i]);
    }
    PMSIVMenu_FreeColors(wk);
    PMSIVMenu_FreeArrows(wk);
    PMSIVMenu_FreeTypeButtons(wk);
    GFL_MsgDataFree(wk->msgData);
    GFL_HeapFree(wk);
}

void PMSIVMenu_Main(PMSIVMenu *wk) {
    int i;

    for (i = 0; i < 3; i++) {
        if (wk->buttons[i] != NULL) {
            AppTaskMenuWin_Update(wk->buttons[i]);
        }
    }
    switch (wk->returnSeq) {
    case RETURN_SEQ_NONE:
        break;
    case RETURN_SEQ_START:
        wk->returnSeq = RETURN_SEQ_WAIT;
        // fallthrough
    case RETURN_SEQ_WAIT:
        if (!func_0204c560(wk->returnButton)) {
            wk->returnSeq = RETURN_SEQ_END;
        }
        break;
    case RETURN_SEQ_END:
        wk->returnSeq = RETURN_SEQ_NONE;
        break;
    }
    PMSIVMenu_UpdateColors(wk);
}

static void PMSIVMenu_Clear(PMSIVMenu *wk) {
    int i;

    for (i = 0; i < 3; i++) {
        if (wk->items[i].str != NULL) {
            GFL_StrBufFree(wk->items[i].str);
            wk->items[i].str = NULL;
        }
        if (wk->buttons[i] != NULL) {
            AppTaskMenuWin_Free(wk->buttons[i]);
            wk->buttons[i] = NULL;
        }
    }
    for (i = 0; i < PMSIV_MENU_TYPE_BUTTON_COUNT; i++) {
        func_0204c124(wk->typeButtons[i], FALSE);
    }
    for (i = 0; i < 2; i++) {
        func_0204c124(wk->arrows[i], FALSE);
    }
    GFL_BGSysLoadScr(0);
    func_0204c124(wk->returnButton, FALSE);
}

void PMSIVMenu_SetupEditButtons(PMSIVMenu *wk) {
    int i;

    PMSIVMenu_Clear(wk);
    for (i = 0; i < 2; i++) {
        AppTaskMenuItem *item = &wk->items[i];

        wk->items[i].str = GFL_MsgDataLoadStrbufNew(wk->msgData, i + 14);
        wk->items[i].color = PMSIV_MENU_TEXT_COLOR;
        wk->items[i].type = i == 1 ? APP_TASKMENU_ITEM_RETURN : 0;
        wk->buttons[i] = AppTaskMenuWin_Create(wk->taskMenuRes[i], item, 23, i * 3 + 18, 9, HEAPID_PMS_INPUT);
        AppTaskMenuWin_SetPalette(wk->buttons[i], wk->taskMenuRes[i], PMSIV_MENU_BUTTON_PALETTE,
                                  PMSIV_MENU_BUTTON_COLOR_1, PMSIV_MENU_BUTTON_COLOR_2);
    }
    PMSIVMenu_UpdateEditButtons(wk);
    GFL_BGSysLoadScr(0);
    if (!PMSInput_HasStartSentence(wk->mwk) && PMSInput_GetInputMode(wk->mwk) == PMSI_MODE_SENTENCE) {
        func_0204c124(wk->typeButtons[0], TRUE);
        func_0204c124(wk->typeButtons[1], TRUE);
        func_0204c124(wk->typeButtons[2], TRUE);
        func_0204c124(wk->typeButtons[3], TRUE);
        func_0204c124(wk->typeButtons[4], TRUE);
        func_0204c124(wk->typeButtons[5], TRUE);
        func_0204c124(wk->typeButtons[6], TRUE);
        for (i = 0; i < 2; i++) {
            func_0204c124(wk->arrows[i], TRUE);
        }
    }
    PMSIVMenu_UpdateSentenceType(wk);
}

void PMSIVMenu_SetupCategoryButtons(PMSIVMenu *wk) {
    PMSIVMenu_Clear(wk);
    func_0204c124(wk->typeButtons[PMSIV_MENU_TYPE_BUTTON_BACK], TRUE);
    if (PMSInput_GetCategoryMode(wk->mwk) == 0) {
        PMSIVMenu_ShowReturnButton(wk);
    } else {
        PMSIVMenu_SetupSearchButtons(wk);
    }
}

void PMSIVMenu_SetupWordWinButtons(PMSIVMenu *wk) {
    PMSIVMenu_Clear(wk);
    func_0204c124(wk->returnButton, TRUE);
    func_0204c488(wk->returnButton, 1);
    wk->returnSeq = RETURN_SEQ_NONE;
}

void PMSIVMenu_UpdateSentenceType(PMSIVMenu *wk) {
    int prev = -1;
    int cur = -1;
    u32 type = PMSInput_GetSentenceType(wk->mwk);
    int i;
    u16 anim;

    for (i = 1; i <= PMSIV_MENU_SENTENCE_TYPE_COUNT; i++) {
        anim = func_0204c4a0(wk->typeButtons[i]);
        if (anim == sPMSIVMenuTypeButtonAnims[i][1]) {
            prev = i;
            break;
        }
    }
    for (i = 0; i < PMSIV_MENU_SENTENCE_TYPE_COUNT; i++) {
        if (i == type) {
            cur = sPMSIVMenuTypeButtons[i];
            break;
        }
    }
    if (prev >= 0 && prev != cur) {
        PMSIVMenu_EndTypeButtonPulse(wk, prev);
    }
    for (i = 0; i < PMSIV_MENU_SENTENCE_TYPE_COUNT; i++) {
        PMSIVMenu_SetTypeButtonAnim(wk, sPMSIVMenuTypeButtons[i], i == type ? TRUE : FALSE);
    }
    if (cur >= 0 && prev != cur) {
        PMSIVMenu_StartTypeButtonPulse(wk, cur);
    }
}

void PMSIVMenu_UpdateEditButtons(PMSIVMenu *wk) {
    if (PMSInput_IsEditComplete(wk->mwk)) {
        AppTaskMenuWin_SetPalette(wk->buttons[0], wk->taskMenuRes[0], PMSIV_MENU_BUTTON_PALETTE,
                                  PMSIV_MENU_BUTTON_COLOR_1, PMSIV_MENU_BUTTON_COLOR_2);
    } else {
        AppTaskMenuWin_SetPalette(wk->buttons[0], wk->taskMenuRes[0], PMSIV_MENU_BUTTON_PALETTE_OFF,
                                  PMSIV_MENU_BUTTON_COLOR_1, PMSIV_MENU_BUTTON_COLOR_2);
    }
}

void PMSIVMenu_UpdateSearchButtons(PMSIVMenu *wk) {
    if (PMSInput_GetSearchResultCount(wk->mwk)) {
        AppTaskMenuWin_SetPalette(wk->buttons[0], wk->taskMenuRes[0], PMSIV_MENU_BUTTON_PALETTE,
                                  PMSIV_MENU_BUTTON_COLOR_1, PMSIV_MENU_BUTTON_COLOR_2);
    } else {
        AppTaskMenuWin_SetPalette(wk->buttons[0], wk->taskMenuRes[0], PMSIV_MENU_BUTTON_PALETTE_OFF,
                                  PMSIV_MENU_BUTTON_COLOR_1, PMSIV_MENU_BUTTON_COLOR_2);
    }
    if (PMSInput_GetSearchInputLen(wk->mwk)) {
        AppTaskMenuWin_SetPalette(wk->buttons[1], wk->taskMenuRes[1], PMSIV_MENU_BUTTON_PALETTE,
                                  PMSIV_MENU_BUTTON_COLOR_1, PMSIV_MENU_BUTTON_COLOR_2);
    } else {
        AppTaskMenuWin_SetPalette(wk->buttons[1], wk->taskMenuRes[1], PMSIV_MENU_BUTTON_PALETTE_OFF,
                                  PMSIV_MENU_BUTTON_COLOR_1, PMSIV_MENU_BUTTON_COLOR_2);
    }
}

static void PMSIVMenu_StartModeButton(PMSIVMenu *wk) {
    if (PMSInput_GetCategoryMode(wk->mwk) == 1) {
        PMSIVMenu_FlashButton(wk, 2, TRUE);
    }
}

static BOOL PMSIVMenu_WaitModeButton(PMSIVMenu *wk) {
    if (PMSInput_GetCategoryMode(wk->mwk) == 1) {
        if (PMSIVMenu_IsFlashFinished(wk, 2)) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

static void PMSIVMenu_StartBackButton(PMSIVMenu *wk) {
    PMSIVMenu_SetTypeButtonAnim(wk, PMSIV_MENU_TYPE_BUTTON_BACK, TRUE);
}

static BOOL PMSIVMenu_WaitBackButton(PMSIVMenu *wk) {
    if (!func_0204c560(wk->typeButtons[PMSIV_MENU_TYPE_BUTTON_BACK])) {
        return TRUE;
    }
    return FALSE;
}

static void PMSIVMenu_EndBackButton(PMSIVMenu *wk) {
    PMSIVMenu_SetTypeButtonAnim(wk, PMSIV_MENU_TYPE_BUTTON_BACK, FALSE);
}

static void PMSIVMenu_StartSearchButton(PMSIVMenu *wk) {
    if (PMSInput_GetCategoryMode(wk->mwk) == 1) {
        PMSIVMenu_FlashButton(wk, 1, TRUE);
    }
}

static BOOL PMSIVMenu_WaitSearchButton(PMSIVMenu *wk) {
    if (PMSInput_GetCategoryMode(wk->mwk) == 1) {
        if (PMSIVMenu_IsFlashFinished(wk, 1)) {
            AppTaskMenuWin_ResetFlash(wk->buttons[1]);
            return TRUE;
        }
    } else {
        return TRUE;
    }
    return FALSE;
}

static void PMSIVMenu_StartEraseButton(PMSIVMenu *wk) {
    if (PMSInput_GetCategoryMode(wk->mwk) == 1) {
        PMSIVMenu_FlashButton(wk, 0, TRUE);
    }
}

static BOOL PMSIVMenu_WaitEraseButton(PMSIVMenu *wk) {
    if (PMSInput_GetCategoryMode(wk->mwk) == 1) {
        return PMSIVMenu_IsFlashFinished(wk, 0) ? TRUE : FALSE;
    }
    return TRUE;
}

void PMSIVMenu_StartButton(PMSIVMenu *wk, u32 button) {
    switch (button) {
    case PMSIV_MENU_BUTTON_MODE:
        PMSIVMenu_StartModeButton(wk);
        break;
    case PMSIV_MENU_BUTTON_BACK:
        PMSIVMenu_StartBackButton(wk);
        break;
    case PMSIV_MENU_BUTTON_SEARCH:
        PMSIVMenu_StartSearchButton(wk);
        break;
    case PMSIV_MENU_BUTTON_ERASE:
        PMSIVMenu_StartEraseButton(wk);
        break;
    }
}

void PMSIVMenu_SetEditButton(PMSIVMenu *wk, u32 which) {
    switch (which) {
    case 0:
        PMSIVMenu_SetTypeButtonAnim(wk, 0, TRUE);
        break;
    case 1:
        PMSIVMenu_SetTypeButtonAnim(wk, 6, TRUE);
        break;
    }
}

BOOL PMSIVMenu_WaitButton(PMSIVMenu *wk, u32 button) {
    switch (button) {
    case PMSIV_MENU_BUTTON_MODE:
        return PMSIVMenu_WaitModeButton(wk);
    case PMSIV_MENU_BUTTON_BACK:
        return PMSIVMenu_WaitBackButton(wk);
    case PMSIV_MENU_BUTTON_SEARCH:
        return PMSIVMenu_WaitSearchButton(wk);
    case PMSIV_MENU_BUTTON_ERASE:
        return PMSIVMenu_WaitEraseButton(wk);
    }
    return TRUE;
}

void PMSIVMenu_EndButton(PMSIVMenu *wk, u32 button) {
    if (button == PMSIV_MENU_BUTTON_BACK) {
        PMSIVMenu_EndBackButton(wk);
    }
}

void PMSIVMenu_SetCursor(PMSIVMenu *wk, u8 pos, BOOL active) {
    int i;

    if (wk->buttons[pos] != NULL) {
        for (i = 0; i < 3; i++) {
            if (wk->buttons[i] != NULL) {
                AppTaskMenuWin_SetActive(wk->buttons[i], FALSE);
            }
        }
        AppTaskMenuWin_SetActive(wk->buttons[pos], active);
    }
}

void PMSIVMenu_FlashButton(PMSIVMenu *wk, u8 pos, BOOL flash) {
    int i;

    for (i = 0; i < 3; i++) {
        if (wk->buttons[i] != NULL) {
            AppTaskMenuWin_SetActive(wk->buttons[i], FALSE);
        }
    }
    if (wk->buttons[pos] != NULL) {
        AppTaskMenuWin_SetFlashing(wk->buttons[pos], flash);
    }
}

BOOL PMSIVMenu_IsFlashFinished(PMSIVMenu *wk, u8 pos) {
    return AppTaskMenuWin_IsFlashFinished(wk->buttons[pos]);
}

BOOL PMSIVMenu_StartReturn(PMSIVMenu *wk) {
    if (func_0204c138(wk->returnButton) && wk->returnSeq == RETURN_SEQ_NONE) {
        func_0204c488(wk->returnButton, 9);
        wk->returnSeq = RETURN_SEQ_START;
        return TRUE;
    }
    return FALSE;
}

BOOL PMSIVMenu_WaitReturn(PMSIVMenu *wk) {
    if (func_0204c138(wk->returnButton) && wk->returnSeq != RETURN_SEQ_END) {
        return FALSE;
    }
    return TRUE;
}

static void PMSIVMenu_SetupTypeButtons(PMSIVMenu *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PMSI, HEAPID_PMS_INPUT);
    int i;

    PMSIView_GetObjRes(wk->vwk, &wk->typeButtonRes, 0, 0);
    wk->typeButtonRes.chars = func_0204b81c(arc, 13, FALSE, CLACT_VRAM_MAIN, HEAPID_PMS_INPUT);
    wk->typeButtonRes.cellAnims = func_0204bde0(arc, 35, 41, HEAPID_PMS_INPUT);
    GFL_ArcToolFree(arc);
    for (i = 0; i < PMSIV_MENU_TYPE_BUTTON_COUNT; i++) {
        s16 x = i * 24;

        if (i == PMSIV_MENU_TYPE_BUTTON_BACK) {
            x = 0;
        }
        wk->typeButtons[i] = PMSIView_AddActor(wk->vwk, &wk->typeButtonRes, x, 168, 0, NNS_G2D_VRAM_TYPE_2DMAIN);
        PMSIVMenu_SetTypeButtonAnim(wk, i, FALSE);
        func_0204c124(wk->typeButtons[i], FALSE);
    }
}

static void PMSIVMenu_FreeTypeButtons(PMSIVMenu *wk) {
    int i;

    for (i = 0; i < PMSIV_MENU_TYPE_BUTTON_COUNT; i++) {
        func_0204c108(wk->typeButtons[i]);
    }
    func_0204b98c(wk->typeButtonRes.chars);
    func_0204be64(wk->typeButtonRes.cellAnims);
}

static void PMSIVMenu_SetTypeButtonAnim(PMSIVMenu *wk, u32 button, BOOL on) {
    func_0204c488(wk->typeButtons[button], sPMSIVMenuTypeButtonAnims[button][on]);
}

static void PMSIVMenu_SetupArrows(PMSIVMenu *wk) {
    u8 i;

    for (i = 0; i < 2; i++) {
        const PMSIVMenuArrow *arrow = &sPMSIVMenuArrows[i];

        wk->arrows[i] = PMSIView_AddActor(wk->vwk, &wk->typeButtonRes, sPMSIVMenuArrows[i].x, arrow->y, 0,
                                          NNS_G2D_VRAM_TYPE_2DMAIN);
        func_0204c488(wk->arrows[i], arrow->anim);
        func_0204c124(wk->arrows[i], FALSE);
    }
}

static void PMSIVMenu_FreeArrows(PMSIVMenu *wk) {
    u8 i;

    for (i = 0; i < 2; i++) {
        func_0204c108(wk->arrows[i]);
    }
}

static void PMSIVMenu_SetupReturnButton(PMSIVMenu *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_PMS_INPUT);
    ClActorSetup setup;

    wk->returnPalette = func_0204bbb8(arc, func_0202d810(), CLACT_VRAM_MAIN, 0x180, 0, 3, HEAPID_PMS_INPUT);
    wk->returnChars = func_0204b81c(arc, func_0202d814(), FALSE, CLACT_VRAM_MAIN, HEAPID_PMS_INPUT);
    wk->returnCellAnims = func_0204bde0(arc, func_0202d818(1), func_0202d81c(1), HEAPID_PMS_INPUT);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(setup));
    setup.x = 232;
    setup.y = 168;
    setup.sequence = 1;
    setup.priority = 0;
    setup.bgPriority = 3;
    wk->returnButton = func_0204c040(PMSIView_GetActUnit(wk->vwk), wk->returnChars, wk->returnPalette,
                                     wk->returnCellAnims, &setup, CLACT_SURFACE_MAIN, HEAPID_PMS_INPUT);
    func_0204c520(wk->returnButton, TRUE);
    func_0204c124(wk->returnButton, FALSE);
    wk->returnSeq = RETURN_SEQ_NONE;
}

static void PMSIVMenu_ShowReturnButton(PMSIVMenu *wk) {
    func_0204c124(wk->returnButton, TRUE);
    func_0204c488(wk->returnButton, 1);
    wk->returnSeq = RETURN_SEQ_NONE;
}

static void PMSIVMenu_SetupSearchButtons(PMSIVMenu *wk) {
    int i;

    for (i = 0; i < 3; i++) {
        AppTaskMenuItem *item = &wk->items[i];

        wk->items[i].str = GFL_MsgDataLoadStrbufNew(wk->msgData, i + 18);
        wk->items[i].color = PMSIV_MENU_TEXT_COLOR;
        wk->items[i].type = i == 2 ? APP_TASKMENU_ITEM_RETURN : 0;
        wk->buttons[i] = AppTaskMenuWin_Create(wk->taskMenuRes[i], item, i * 9 + 5, 21, 9, HEAPID_PMS_INPUT);
        AppTaskMenuWin_SetPalette(wk->buttons[i], wk->taskMenuRes[i], PMSIV_MENU_BUTTON_PALETTE,
                                  PMSIV_MENU_BUTTON_COLOR_1, PMSIV_MENU_BUTTON_COLOR_2);
    }
    PMSIVMenu_UpdateSearchButtons(wk);
    GFL_BGSysLoadScr(0);
}

static void PMSIVMenu_SetupColors(PMSIVMenu *wk) {
    NNSG2dPaletteData *palette;
    void *buf = GFL_G2DIOReadNCLR(ARCID_PMSI, 7, &palette, HEAPID_PMS_INPUT);
    u16 *colors = palette->rawData;

    sys_memcpy(&colors[75], wk->pulseFrom, sizeof(wk->pulseFrom));
    sys_memcpy(&colors[59], wk->pulseTo, sizeof(wk->pulseTo));
    GFL_HeapFree(buf);
    sys_memcpy(wk->pulseFrom, wk->pulse, sizeof(wk->pulse));
    wk->pulseAngle = 0;
}

static void PMSIVMenu_FreeColors(PMSIVMenu *wk) {
}

static void PMSIVMenu_UpdateColors(PMSIVMenu *wk) {
    fx16 ratio;
    u8 i;

    if (wk->pulseAngle + 0x400 >= 0x10000) {
        wk->pulseAngle = wk->pulseAngle - 0xfc00;
    } else {
        wk->pulseAngle = wk->pulseAngle + 0x400;
    }
    ratio = (FX_CosIdx(wk->pulseAngle) + FX32_ONE) / 2;
    for (i = 0; i < PMSIV_MENU_PULSE_COLOR_COUNT; i++) {
        u8 fromR = wk->pulseFrom[i] & GX_RGB_R_MASK;
        u8 fromG = (wk->pulseFrom[i] & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        u8 fromB = (wk->pulseFrom[i] & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        u8 toR = wk->pulseTo[i] & GX_RGB_R_MASK;
        u8 toG = (wk->pulseTo[i] & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        u8 toB = (wk->pulseTo[i] & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        u8 r = fromR + (((toR - fromR) * ratio) >> FX32_SHIFT);
        u8 g = fromG + (((toG - fromG) * ratio) >> FX32_SHIFT);
        u8 b = fromB + (((toB - fromB) * ratio) >> FX32_SHIFT);

        wk->pulse[i] = GX_RGB(r, g, b);
    }
    NNS_GfdRegisterNewVramTransferTask(14, 0x56, wk->pulse, sizeof(wk->pulse));
}

static void PMSIVMenu_StartTypeButtonPulse(PMSIVMenu *wk, u32 button) {
    func_0204c378(wk->typeButtons[button], 2, 1);
    sys_memcpy(wk->pulseFrom, wk->pulse, sizeof(wk->pulse));
    wk->pulseAngle = 0;
}

static void PMSIVMenu_EndTypeButtonPulse(PMSIVMenu *wk, u32 button) {
    func_0204c378(wk->typeButtons[button], 6, 1);
}
