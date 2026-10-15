#include "types.h"
#include "app/pms_input.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/mailsys.h"
#include "field/mailtool.h"
#include "field/mailview.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/pms_draw.h"
#include "system/pmsi_param.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wipe.h"
#include "text/system/btl_server_flow_3.h"

// The mail screen's procs (overlay 215's mailsys.c): one runs the screen of the mail, and the sentence input when the
// player picks a sentence to write; the other is the screen itself, whose phases are the keys and touch input

// The record of mails written
#define RECORD_MAILS_WRITTEN 0x1d

// The phases of the screen, which the table runs
enum {
    MAIL_PHASE_NOTICE = 2,
    MAIL_PHASE_CANCEL = 3,
};

// The steps of MailSys_Update
enum {
    MAIL_SEQ_INIT,
    MAIL_SEQ_LOAD,
    MAIL_SEQ_FADE_IN,
    MAIL_SEQ_PHASE,
    MAIL_SEQ_FADE_OUT,
    MAIL_SEQ_UNLOAD,
};

// The work of the screen's proc
typedef struct {
    u32 heapId;
    u32 unk04;
    PMSIParam *pmsiParam;
    u32 unk0C;
    MailEdit *edit;
    PMSData sentence;
    GameProcManager *procMgr;
} MailProcWork;

typedef BOOL (*MailSysPhaseFunc)(MailSysWork *work);

static void MailSys_StartPMSInput(MailProcWork *work, MailParam *param);
static MailEdit *MailSys_CreateEdit(MailData *mail, u32 heapId);
static void MailSys_FreeEdit(MailEdit *edit);
static void MailSys_ApplyEdit(MailData *mail, MailEdit *edit);
static void MailSys_InitPresets(MailSysWork *work, MailEdit *edit);
static BOOL MailSys_EditInit(GameProc *proc, u32 *state, void *param, void *unused);
static BOOL MailSys_EditMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MailSys_EditExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MailSys_Update(MailSysWork *work);
static BOOL MailSys_IsIncomplete(MailSysWork *work);
static void MailSys_ResetBlink(MailSysWork *work);
static BOOL MailSys_CheckInputModeSwitch(MailSysWork *work);
static BOOL MailSys_PhaseView(MailSysWork *work);
static BOOL MailSys_PhaseKeys(MailSysWork *work);
static BOOL MailSys_PhaseTouch(MailSysWork *work);
static BOOL MailSys_PhaseWrite(MailSysWork *work);
static void MailSys_ShowMessage(MailSysWork *work, u32 messageId);
static BOOL MailSys_PhaseNotice(MailSysWork *work);
static void MailSys_OpenConfirmMenu(MailSysWork *work);
static s32 MailSys_GetMenuResult(MailSysWork *work);
static BOOL MailSys_PhaseCancel(MailSysWork *work);
static void MailSys_DisableAllBGs(void);
static void MailSys_VBlank(TCB *tcb, void *data);

const GameProcFunctions MAILSYS_PROC_FUNCTIONS = {
    MailSys_ProcInit,
    MailSys_ProcMain,
    MailSys_ProcExit,
};

static const GameProcFunctions sMailSysEditProc = {
    MailSys_EditInit,
    MailSys_EditMain,
    MailSys_EditExit,
};

static const GameProcFunctions sMailSysPMSInputProc = {
    PMSInput_Init,
    PMSInput_Main,
    PMSInput_Exit,
};

// The VRAM banks of the screen, which mailview.c sets
const BGSysVRAMConfig MailSys_VRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,  GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,  GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const MailSysPhaseFunc sMailSysPhaseFuncs[] = {
    MailSys_PhaseView,
    MailSys_PhaseWrite,
    MailSys_PhaseNotice,
    MailSys_PhaseCancel,
};

// The sentence, of sentence type 5, that a design gives each of its three sentences to start from, or 0xff
static const u8 sMailSysPresetSentences[MAIL_DESIGN_COUNT][MAIL_WORD_COUNT] = {
    { 0, 1, 2 },          { 3, 4, 5 },          { 6, 7, 8 },          { 9, 10, 20 },
    { 11, 12, 13 },       { 14, 15, 16 },       { 17, 18, 19 },       { 0xff, 0xff, 0xff },
    { 0xff, 0xff, 0xff }, { 0xff, 0xff, 0xff }, { 0xff, 0xff, 0xff }, { 0xff, 0xff, 0xff },
};

// The three sentences, the word, and the two buttons, the cancel one last
static const TouchRect sMailSysTouchRects[] = {
    { 0x18, 0x38, 0x18, 0xe8 }, { 0x40, 0x60, 0x18, 0xe8 }, { 0x68, 0x88, 0x18, 0xe8 },
    { 0x98, 0xa8, 0x40, 0x98 }, { 0xa8, 0xc0, 0xa8, 0xf8 }, { 0x90, 0xa8, 0xa8, 0xf8 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

BOOL MailSys_ProcInit(GameProc *proc, u32 *state, void *param, void *unused) {
    MailParam *mailParam = param;
    MailProcWork *work;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MAIL, 0x1000);
    work = GFL_ProcInitSubsystem(proc, sizeof(MailProcWork), HEAPID_MAIL);
    sys_memset32_fast(0, work, sizeof(MailProcWork));
    work->heapId = HEAPID_MAIL;
    work->edit = MailSys_CreateEdit(mailParam->mail, HEAPID_MAIL);
    if (mailParam->mode == MAIL_PARAM_MODE_WRITE) {
        work->edit->design = mailParam->design;
    }
    if (work->edit->design >= MAIL_DESIGN_COUNT) {
        work->edit->design = 0;
    }
    work->edit->mode = mailParam->mode;
    work->procMgr = CreateGameProcManager(HEAPID_MAIL);
    return TRUE;
}

BOOL MailSys_ProcMain(GameProc *proc, u32 *state, void *param, void *workPtr) {
    MailParam *mailParam = param;
    MailProcWork *work = workPtr;
    BOOL running = GFL_ProcMgrUpdate(work->procMgr);

    switch (*state) {
    case 0:
        work->edit->mode = mailParam->mode;
        QueueGameProc(work->procMgr, OVERLAY_NONE, &sMailSysEditProc, work->edit);
        *state = 1;
        break;
    case 1:
        if (running == TRUE) {
            break;
        }
        switch (work->edit->mode) {
        case MAIL_EDIT_CANCEL:
            *state = 2;
            break;
        case MAIL_EDIT_DONE:
            *state = 2;
            break;
        default:
            *state = 3;
            break;
        }
        break;
    case 2:
        if (mailParam->mode == MAIL_PARAM_MODE_WRITE) {
            if (work->edit->mode == MAIL_EDIT_DONE) {
                MailSys_ApplyEdit(mailParam->mail, work->edit);
                RecordAddOne(getTrainerCardInfoBlkAddress(mailParam->save), RECORD_MAILS_WRITTEN);
                mailParam->written = TRUE;
            } else {
                mailParam->written = FALSE;
            }
        }
        return TRUE;
    case 3:
        GFL_OvlLoad(OVERLAY_ID(185));
        MailSys_StartPMSInput(work, mailParam);
        QueueGameProc(work->procMgr, OVERLAY_NONE, &sMailSysPMSInputProc, work->pmsiParam);
        *state = 4;
        break;
    case 4:
        if (running == TRUE) {
            break;
        }
        if (!PMSIParam_IsCanceled(work->pmsiParam)) {
            if (work->edit->selected < MAIL_WORD_COUNT) {
                PMSIParam_GetSentence(work->pmsiParam, &work->edit->words[work->edit->selected]);
            } else {
                work->edit->word = PMSIParam_GetWord(work->pmsiParam);
            }
        }
        PMSIParam_Free(work->pmsiParam);
        GFL_OvlUnload(OVERLAY_ID(185));
        *state = 0;
        break;
    }
    return FALSE;
}

BOOL MailSys_ProcExit(GameProc *proc, u32 *state, void *param, void *workPtr) {
    MailProcWork *work = workPtr;
    HeapID heapId;

    FreeGameProcManager(work->procMgr);
    MailSys_FreeEdit(work->edit);
    heapId = work->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapStatusValidate(heapId);
    GFL_HeapDelete(heapId);
    return TRUE;
}

static void MailSys_StartPMSInput(MailProcWork *work, MailParam *param) {
    if (work->edit->selected == MAIL_WORD_COUNT) {
        PMSData_Clear(&work->sentence);
        work->sentence.type = 6;
        work->sentence.id = 0;
        work->sentence.words[0] = work->edit->word;
        work->pmsiParam = PMSIParam_Create(PMSI_MODE_WORD, 0, NULL, TRUE, param->save, work->heapId);
        PMSIParam_SetWord(work->pmsiParam, work->edit->word);
    } else if (work->edit->hasPreset[work->edit->selected] == TRUE) {
        PMSData_Copy(&work->sentence, &work->edit->words[work->edit->selected]);
        work->pmsiParam = PMSIParam_Create(PMSI_MODE_SENTENCE, 0, &work->sentence, TRUE, param->save, work->heapId);
    } else {
        work->pmsiParam = PMSIParam_Create(PMSI_MODE_SENTENCE, 0, NULL, TRUE, param->save, work->heapId);
        if (PMSData_IsNotEmpty(&work->edit->words[work->edit->selected])) {
            PMSData_Copy(&work->sentence, &work->edit->words[work->edit->selected]);
        } else {
            PMSData_Init(&work->sentence, 0);
        }
        PMSIParam_SetSentence(work->pmsiParam, &work->sentence);
    }
}

static MailEdit *MailSys_CreateEdit(MailData *mail, u32 heapId) {
    MailEdit *edit = GFL_HeapAllocate(heapId, sizeof(MailEdit), FALSE, "mailsys.c", 0x147);
    u16 i;

    sys_memset32_fast(0, edit, sizeof(MailEdit));
    edit->mode = 0;
    edit->trainerId = func_02009730(mail);
    edit->name = GFL_StrBufCreate(16, heapId);
    GFL_StrBufLoadString(edit->name, func_02009734(mail));
    edit->design = func_02009750(mail);
    edit->unk09 = func_0200975c(mail);
    edit->unk0A = func_02009760(mail);
    edit->word = func_02009764(mail);
    for (i = 0; i < MAIL_WORD_COUNT; i++) {
        PMSData_Copy(&edit->words[i], func_0200976c(mail, i));
    }
    return edit;
}

static void MailSys_FreeEdit(MailEdit *edit) {
    if (edit->name != NULL) {
        GFL_StrBufFree(edit->name);
    }
    GFL_HeapFree(edit);
}

static void MailSys_ApplyEdit(MailData *mail, MailEdit *edit) {
    u16 i;

    for (i = 0; i < MAIL_WORD_COUNT; i++) {
        func_0200977c(mail, &edit->words[i], i);
    }
    func_02009768(mail, edit->word);
    func_02009754(mail, edit->design);
}

static void MailSys_InitPresets(MailSysWork *work, MailEdit *edit) {
    int i;

    for (i = 0; i < MAIL_WORD_COUNT; i++) {
        if (!PMSData_IsNotEmpty(&edit->words[i]) && sMailSysPresetSentences[edit->design][i] != 0xff) {
            PMSData_InitWithSentence(&edit->words[i], 5, sMailSysPresetSentences[edit->design][i]);
        }
        if (sMailSysPresetSentences[edit->design][i] == 0xff) {
            edit->hasPreset[i] = FALSE;
        } else {
            edit->hasPreset[i] = TRUE;
        }
    }
}

static BOOL MailSys_EditInit(GameProc *proc, u32 *state, void *param, void *unused) {
    MailEdit *edit = param;
    MailSysWork *work;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MAIL_SCREEN, 0x20000);
    work = GFL_ProcInitSubsystem(proc, sizeof(MailSysWork), HEAPID_MAIL_SCREEN);
    sys_memset32_fast(0, work, sizeof(MailSysWork));
    work->heapId = HEAPID_MAIL_SCREEN;
    work->edit = edit;
    work->phase = edit->mode;
    work->initialPhase = work->phase;
    work->cursor = edit->selected;
    work->savedCursor = edit->cursor;
    work->blinkCurrent = work->blinkTarget = work->cursor;
    edit->mode = MAIL_EDIT_CANCEL;
    work->textSpeed = func_02017bcc();
    MailSys_InitPresets(work, work->edit);
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, work->heapId);
    work->tcbEx = GFL_TCBExMgrCreate(work->heapId, work->heapId, 32, 32);
    work->keyMode = TRUE;
    GFL_BGSysCreate(work->heapId);
    return TRUE;
}

static BOOL MailSys_EditMain(GameProc *proc, u32 *state, void *param, void *work) {
    if (MailSys_Update(work)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MailSys_EditExit(GameProc *proc, u32 *state, void *param, void *workPtr) {
    MailSysWork *work = workPtr;
    u32 heapId;

    GFL_BGSysFree();
    GFL_TCBExMgrFree(work->tcbEx);
    GFL_FontFree(work->font);
    heapId = work->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapStatusValidate(heapId);
    GFL_HeapDelete(heapId);
    return TRUE;
}

static BOOL MailSys_Update(MailSysWork *work) {
    BOOL done = FALSE;

    switch (work->seq) {
    case MAIL_SEQ_INIT:
        MailSys_DisableAllBGs();
        Wipe_SetScreenCovered(0, 0);
        Wipe_SetScreenCovered(1, 0);
        Wipe_HideWindows(0);
        Wipe_HideWindows(1);
        work->seq++;
        break;
    case MAIL_SEQ_LOAD:
        if (MailView_Load(work)) {
            work->vblankTask = GFL_VBlankTCBAdd(MailSys_VBlank, work, 16);
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
            killBrightnessEitherEngine(0);
            gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 28, 4);
            PaletteFade_StartFade(work->palFade, 5, 0xffff, -1, 16, 0, 0, GFL_VBlankGetTCBMgr());
            work->seq++;
        }
        break;
    case MAIL_SEQ_FADE_IN:
        if (PaletteFade_GetActiveMask(work->palFade) == 0) {
            work->seq++;
        }
        PMSDraw_Main(work->pmsDraw);
        break;
    case MAIL_SEQ_PHASE:
        if (sMailSysPhaseFuncs[work->phase](work) == TRUE) {
            work->seq++;
            PaletteFade_StartFade(work->palFade, 5, 0xffff, -1, 0, 16, 0, GFL_VBlankGetTCBMgr());
        }
        PMSDraw_Main(work->pmsDraw);
        if (work->confirmButton != NULL) {
            AppTaskMenuWin_Update(work->confirmButton);
            AppTaskMenuWin_Update(work->cancelButton);
        }
        break;
    case MAIL_SEQ_FADE_OUT:
        if (PaletteFade_GetActiveMask(work->palFade) == 0) {
            Wipe_SetScreenCovered(0, 0);
            Wipe_SetScreenCovered(1, 0);
            GFL_TCBRemove(work->vblankTask);
            MailSys_DisableAllBGs();
            work->seq++;
        }
        PMSDraw_Main(work->pmsDraw);
        break;
    case MAIL_SEQ_UNLOAD:
        if (MailView_Unload(work)) {
            done = TRUE;
        }
        break;
    }
    if (work->clUnit != NULL) {
        func_0204b794();
    }
    if (work->tcbMgr != NULL) {
        GFL_TCBMgrUpdate(work->tcbMgr);
    }
    if (work->tcbEx != NULL) {
        GFL_TCBExMgrUpdate(work->tcbEx);
    }
    if (work->printQueue != NULL) {
        func_02021a3c(work->printQueue);
    }
    if (work->pmsQueue != NULL) {
        func_02021a3c(work->pmsQueue);
    }
    return done;
}

// Whether the mail can't be kept yet: the sentences that a design presets need all their words, and the others need
// one of them
static BOOL MailSys_IsIncomplete(MailSysWork *work) {
    int required = 1;
    int presetCount = 0;
    int freeFilled = 0;
    int presetComplete = 0;
    int i;

    for (i = 0; i < MAIL_WORD_COUNT; i++) {
        if (sMailSysPresetSentences[work->edit->design][i] != 0xff) {
            presetCount++;
        }
    }
    if (presetCount == MAIL_WORD_COUNT) {
        required = 0;
    }
    for (i = 0; i < MAIL_WORD_COUNT; i++) {
        if (sMailSysPresetSentences[work->edit->design][i] != 0xff) {
            if (PMSData_IsComplete(&work->edit->words[i], 100)) {
                presetComplete++;
            }
        } else if (PMSData_IsNotEmpty(&work->edit->words[i])) {
            freeFilled++;
        }
    }
    if (freeFilled < required || presetComplete < presetCount) {
        return TRUE;
    }
    return FALSE;
}

static void MailSys_ResetBlink(MailSysWork *work) {
    PaletteFade_BlendBuffer(work->palFade, 0, work->blinkCurrent + 0x21, 1, 0, 0x7fff);
    work->blinkFalling = 0;
    work->blinkLevel = 0;
}

BOOL MailSys_OnTouchMode(MailSysWork *work) {
    MailSys_ResetBlink(work);
    return FALSE;
}

BOOL MailSys_OnKeyMode(MailSysWork *work) {
    return FALSE;
}

// Switches between the keys and the touch screen when the other one is used, and returns whether the keys took over
static BOOL MailSys_CheckInputModeSwitch(MailSysWork *work) {
    if (work->keyMode == FALSE) {
        if (func_0203da2c()) {
            return FALSE;
        }
        if (GCTX_HIDGetHeldKeys() != 0) {
            if (work->onKeyMode != NULL) {
                work->onKeyMode(work);
            }
            work->keyMode = TRUE;
            return TRUE;
        }
    } else {
        if (GCTX_HIDGetHeldKeys() != 0) {
            return FALSE;
        }
        if (func_0203da2c()) {
            if (work->onTouchMode != NULL) {
                work->onTouchMode(work);
            }
            work->keyMode = FALSE;
            return FALSE;
        }
    }
    return FALSE;
}

static BOOL MailSys_PhaseView(MailSysWork *work) {
    if (GCTX_HIDGetPressedKeys() != 0) {
        work->keyMode = TRUE;
    } else if (func_0203da48()) {
        work->keyMode = FALSE;
    } else {
        return FALSE;
    }
    GFL_SndSEPlay(SEQ_SE_DECIDE3);
    return TRUE;
}

static BOOL MailSys_PhaseKeys(MailSysWork *work) {
    BOOL moved;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        if (work->cursor == 4) {
            if (MailSys_IsIncomplete(work)) {
                GFL_SndSEPlay(SEQ_SE_CANCEL1);
                work->phase = MAIL_PHASE_NOTICE;
                return FALSE;
            }
            work->edit->mode = MAIL_EDIT_DONE;
            GFL_SndSEPlay(SEQ_SE_DECIDE3);
            work->edit->selected = 0;
            work->edit->cursor = 0;
            AppTaskMenuWin_SetFlashing(work->confirmButton, TRUE);
            return TRUE;
        } else if (work->cursor == 5) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            work->phase = MAIL_PHASE_CANCEL;
            return FALSE;
        } else {
            work->edit->selected = work->cursor;
            work->edit->mode = work->edit->selected;
            work->edit->cursor = work->savedCursor;
            GFL_SndSEPlay(SEQ_SE_DECIDE2);
            return TRUE;
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        work->cursor = 5;
        work->blinkTarget = work->cursor;
        work->phase = MAIL_PHASE_CANCEL;
        return FALSE;
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        work->cursor = 4;
        work->savedCursor = 0;
        moved = TRUE;
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_DOWN) {
        work->cursor = (work->cursor + 1) % 6;
        moved = TRUE;
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_UP) {
        work->cursor = (work->cursor + 5) % 6;
        moved = TRUE;
    } else {
        return FALSE;
    }
    if (!moved) {
        return FALSE;
    }
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    if (work->cursor < 4) {
        AppTaskMenuWin_SetActive(work->confirmButton, FALSE);
        AppTaskMenuWin_SetActive(work->cancelButton, FALSE);
    } else if (work->cursor == 4) {
        AppTaskMenuWin_SetActive(work->confirmButton, TRUE);
        AppTaskMenuWin_SetActive(work->cancelButton, FALSE);
    } else if (work->cursor == 5) {
        AppTaskMenuWin_SetActive(work->confirmButton, FALSE);
        AppTaskMenuWin_SetActive(work->cancelButton, TRUE);
    }
    work->blinkTarget = work->cursor;
    return FALSE;
}

static BOOL MailSys_PhaseTouch(MailSysWork *work) {
    s32 touched = func_0203da0c(sMailSysTouchRects);

    if (touched == TOUCH_RECT_NONE) {
        return FALSE;
    }
    if (touched == 4) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        work->phase = MAIL_PHASE_CANCEL;
        return FALSE;
    } else if (touched < 4) {
        work->edit->selected = touched;
        work->edit->mode = work->edit->selected;
        work->edit->cursor = work->savedCursor;
        GFL_SndSEPlay(SEQ_SE_DECIDE2);
        return TRUE;
    }
    if (MailSys_IsIncomplete(work)) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        work->phase = MAIL_PHASE_NOTICE;
        return FALSE;
    }
    work->edit->mode = MAIL_EDIT_DONE;
    GFL_SndSEPlay(SEQ_SE_DECIDE3);
    work->edit->selected = 0;
    work->edit->cursor = 0;
    AppTaskMenuWin_SetFlashing(work->confirmButton, TRUE);
    AppTaskMenuWin_SetActive(work->cancelButton, FALSE);
    return TRUE;
}

static BOOL MailSys_PhaseWrite(MailSysWork *work) {
    if (MailSys_CheckInputModeSwitch(work)) {
        return FALSE;
    }
    if (work->keyMode == TRUE) {
        return MailSys_PhaseKeys(work);
    }
    return MailSys_PhaseTouch(work);
}

static void MailSys_ShowMessage(MailSysWork *work, u32 messageId) {
    BmpWin_DrawFrame(work->windows[6], WINFRAME_TRANSFER_NONE, 0xd, 4);
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[6]), 0xff);
    GFL_MsgDataLoadStrbuf(work->msgData, messageId, work->msgBuf);
    work->printStream = func_02022268(work->windows[6], 0, 0, work->msgBuf, work->font, work->textSpeed,
                                      work->tcbEx, 4, work->heapId, 15);
    BmpWin_TransferNow(work->windows[6]);
}

// Tells that the mail is not finished, once its button has flashed
static BOOL MailSys_PhaseNotice(MailSysWork *work) {
    switch (work->subSeq) {
    case 0:
        AppTaskMenuWin_SetFlashing(work->confirmButton, TRUE);
        AppTaskMenuWin_SetActive(work->cancelButton, FALSE);
        work->subSeq = 1;
        break;
    case 1:
        if (AppTaskMenuWin_IsFlashFinished(work->confirmButton)) {
            AppTaskMenuWin_ResetFlash(work->confirmButton);
            AppTaskMenuWin_SetActive(work->confirmButton, TRUE);
            MailSys_ShowMessage(work, BtlServerFlow3_Text_PleaseEnterPhraseWord);
            work->subSeq = 2;
        }
        break;
    case 2:
        if (func_020223b4(work->printStream) == PRINT_STREAM_DONE) {
            if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) || func_0203da48()) {
                func_020223cc(work->printStream);
                work->subSeq = 3;
            }
        }
        break;
    case 3:
        BmpWin_ClearFrame(work->windows[6], WINFRAME_TRANSFER_NONE);
        BmpWin_ClearScreenNow(work->windows[6]);
        MailSys_ResetBlink(work);
        work->subSeq = 0;
        work->phase = work->initialPhase;
        break;
    }
    return FALSE;
}

static void MailSys_OpenConfirmMenu(MailSysWork *work) {
    AppTaskMenuInit init;

    work->menuItems[0].str = GFL_MsgDataLoadStrbufNew(work->msgData, BtlServerFlow3_Text_Yes);
    work->menuItems[0].color = PRINT_COLOR(14, 15, 3);
    work->menuItems[0].type = 0;
    work->menuItems[1].str = GFL_MsgDataLoadStrbufNew(work->msgData, BtlServerFlow3_Text_No);
    work->menuItems[1].color = PRINT_COLOR(14, 15, 3);
    work->menuItems[1].type = 0;
    init.heapId = work->heapId;
    init.itemCount = 2;
    init.items = work->menuItems;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 23;
    init.y = 6;
    init.width = 8;
    init.height = 3;
    work->menu = AppTaskMenu_Create(&init, work->menuRes);
    PaletteFade_BlendBuffer(work->palFade, 0, 0, 0x30, 10, 0);
    AppTaskMenuWin_ClearScreen(work->confirmButton);
    AppTaskMenuWin_ClearScreen(work->cancelButton);
}

// The item picked in the confirm menu once it has flashed, or -1
static s32 MailSys_GetMenuResult(MailSysWork *work) {
    s32 result = -1;

    if (AppTaskMenu_IsFlashFinished(work->menu)) {
        result = AppTaskMenu_GetCursorPos(work->menu);
        AppTaskMenu_Free(work->menu);
        GFL_StrBufFree(work->menuItems[0].str);
        GFL_StrBufFree(work->menuItems[1].str);
    }
    return result;
}

// Asks whether to stop writing the mail
static BOOL MailSys_PhaseCancel(MailSysWork *work) {
    s32 result;

    switch (work->subSeq) {
    case 0:
        AppTaskMenuWin_SetActive(work->confirmButton, FALSE);
        AppTaskMenuWin_SetFlashing(work->cancelButton, TRUE);
        break;
    case 1:
        if (AppTaskMenuWin_IsFlashFinished(work->cancelButton)) {
            AppTaskMenuWin_ResetFlash(work->cancelButton);
            AppTaskMenuWin_SetActive(work->cancelButton, TRUE);
            work->confirming = TRUE;
            MailSys_ShowMessage(work, BtlServerFlow3_Text_StopGivingPokemonMail);
            MailSys_ResetBlink(work);
            work->blinkCurrent = work->blinkTarget;
            break;
        }
        return FALSE;
    case 2:
        if (func_020223b4(work->printStream) == PRINT_STREAM_DONE) {
            func_020223cc(work->printStream);
            MailSys_OpenConfirmMenu(work);
            work->subSeq = 3;
        }
        return FALSE;
    case 3:
        AppTaskMenu_Update(work->menu);
        result = MailSys_GetMenuResult(work);
        if (result < 0) {
            return FALSE;
        }
        BmpWin_ClearFrame(work->windows[6], WINFRAME_TRANSFER_NONE);
        BmpWin_ClearScreenNow(work->windows[6]);
        work->subSeq = 0;
        if (result == 0) {
            work->edit->mode = MAIL_EDIT_CANCEL;
            work->phase = work->initialPhase;
            return TRUE;
        }
        work->phase = work->initialPhase;
        work->confirming = FALSE;
        PaletteFade_BlendBuffer(work->palFade, 0, 0, 0x30, 0, 0);
        AppTaskMenuWin_FlushMap(work->confirmButton);
        AppTaskMenuWin_FlushMap(work->cancelButton);
        return FALSE;
    }
    work->subSeq++;
    return FALSE;
}

static void MailSys_DisableAllBGs(void) {
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG2, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
}

static void MailSys_VBlank(TCB *tcb, void *data) {
    MailSysWork *work = data;

    if (work->palFade != NULL) {
        PaletteFade_Transfer(work->palFade);
    }
    func_0204b7c8();
    GFL_BGSysUpdate();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

void MailSys_UpdateBlink(TCB *tcb, void *data) {
    MailSysWork *work = data;

    if (work->confirming == 0 && work->keyMode != 0) {
        if (work->blinkCurrent != work->blinkTarget) {
            MailSys_ResetBlink(work);
            work->blinkCurrent = work->blinkTarget;
        }
        PaletteFade_BlendBuffer(work->palFade, 0, work->blinkTarget + 0x21, 1, work->blinkLevel >> 1, 0x7fff);
        if (work->blinkFalling) {
            if (work->blinkLevel-- == 1) {
                work->blinkFalling ^= 1;
            }
        } else {
            if (work->blinkLevel++ == 24) {
                work->blinkFalling ^= 1;
            }
        }
    }
}
