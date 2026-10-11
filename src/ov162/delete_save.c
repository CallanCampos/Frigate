#include "types.h"
#include "app/delete_save.h"
#include "app/title.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
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
#include "nitro/gx.h"
#include "nitro/os.h"
#include "save/save_control.h"
#include "save/save_outside.h"
#include "system/app_keycursor.h"
#include "system/bmp_menu.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wipe.h"

// Deleting the save data, from the title screen with Up, Select and B: two questions, then every save block is
// cleared and the game restarts

enum {
    STATE_INIT,
    STATE_WAIT_WIPE_IN,
    STATE_PRINT_QUESTION,
    STATE_ASK,
    STATE_PRINT_WARNING,
    STATE_ASK_AGAIN,
    STATE_DELETE,
    STATE_WAIT_WIPE_OUT,
    STATE_END,
};

// The messages of system message file 4
enum {
    MSG_DELETE_ALL,
    MSG_NO_WAY_TO_RECOVER,
    MSG_DELETING,
};

// The save blocks that deleting clears
#define SAVE_BLOCK_COUNT 21

typedef struct {
    TCB *vblankTask;
    Font *font;
    MsgData *msgData;
    u8 langId;
    StrBuf *strbuf;
    PrintStream *printStream;
    TCBExManager *tcbManager;
    BmpWin *window;
    BmpMenu *dialog;
    KeyCursor *keyCursor;
    // Whether the message was already told to go on past its pause
    BOOL continued;
} DeleteSaveWork;

static BOOL DeleteSave_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL DeleteSave_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL DeleteSave_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void DeleteSave_Start(DeleteSaveWork *wk, u32 *state);
static void DeleteSave_WaitWipeIn(DeleteSaveWork *wk, u32 *state);
static void DeleteSave_PrintQuestion(DeleteSaveWork *wk, u32 *state);
static void DeleteSave_Ask(DeleteSaveWork *wk, u32 *state);
static void DeleteSave_PrintWarning(DeleteSaveWork *wk, u32 *state);
static void DeleteSave_AskAgain(DeleteSaveWork *wk, u32 *state);
static void DeleteSave_Delete(DeleteSaveWork *wk, u32 *state);
static void DeleteSave_WaitWipeOut(DeleteSaveWork *wk, u32 *state);
static void DeleteSave_InitVRAM(void);
static void DeleteSave_InitBG(void);
static void DeleteSave_FreeBG(void);
static void DeleteSave_InitMsg(DeleteSaveWork *wk);
static void DeleteSave_FreeMsg(DeleteSaveWork *wk);
static void DeleteSave_InitWindow(DeleteSaveWork *wk);
static void DeleteSave_FreeWindow(DeleteSaveWork *wk);
static void DeleteSave_AddVBlankTask(DeleteSaveWork *wk);
static void DeleteSave_RemoveVBlankTask(DeleteSaveWork *wk);
static void DeleteSave_Print(DeleteSaveWork *wk, u32 messageId);
static BOOL DeleteSave_UpdatePrint(DeleteSaveWork *wk);
static void DeleteSave_OpenYesNo(DeleteSaveWork *wk);
static void DeleteSave_WipeIn(void);
static void DeleteSave_WipeOut(void);

const GameProcFunctions DELETE_SAVE_PROC_FUNCTIONS = { DeleteSave_Init, DeleteSave_Main, DeleteSave_Exit };

// Stepped at VBlank while the save is deleted, as the VBlank tasks do not run then
static WaitIcon *sWaitIcon;

static BOOL DeleteSave_Init(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_SAVEDATA_DELETE, 0x30000);
    sys_memset(GFL_ProcInitSubsystem(proc, sizeof(DeleteSaveWork), HEAPID_SAVEDATA_DELETE), 0,
               sizeof(DeleteSaveWork));
    return TRUE;
}

static BOOL DeleteSave_Main(GameProc *proc, u32 *state, void *param, void *work) {
    switch (*state) {
    case STATE_INIT:
        DeleteSave_Start(work, state);
        break;
    case STATE_WAIT_WIPE_IN:
        DeleteSave_WaitWipeIn(work, state);
        break;
    case STATE_PRINT_QUESTION:
        DeleteSave_PrintQuestion(work, state);
        break;
    case STATE_ASK:
        DeleteSave_Ask(work, state);
        break;
    case STATE_PRINT_WARNING:
        DeleteSave_PrintWarning(work, state);
        break;
    case STATE_ASK_AGAIN:
        DeleteSave_AskAgain(work, state);
        break;
    case STATE_DELETE:
        DeleteSave_Delete(work, state);
        break;
    case STATE_WAIT_WIPE_OUT:
        DeleteSave_WaitWipeOut(work, state);
        break;
    case STATE_END:
        return TRUE;
    }
    return FALSE;
}

// Goes back to the title screen
static BOOL DeleteSave_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_SAVEDATA_DELETE);
    GCTX_ProcMgrReplaceProc(OVERLAY_ID(162), &TITLE_PROC_FUNCTIONS, NULL);
    return TRUE;
}

static void DeleteSave_Start(DeleteSaveWork *wk, u32 *state) {
    DeleteSave_InitVRAM();
    DeleteSave_InitBG();
    DeleteSave_InitMsg(wk);
    DeleteSave_InitWindow(wk);
    DeleteSave_AddVBlankTask(wk);
    DeleteSave_WipeIn();
    *state = STATE_WAIT_WIPE_IN;
}

static void DeleteSave_WaitWipeIn(DeleteSaveWork *wk, u32 *state) {
    if (GFL_WipeIsFinished() == TRUE) {
        DeleteSave_Print(wk, MSG_DELETE_ALL);
        *state = STATE_PRINT_QUESTION;
    }
}

static void DeleteSave_PrintQuestion(DeleteSaveWork *wk, u32 *state) {
    if (DeleteSave_UpdatePrint(wk) == FALSE) {
        DeleteSave_OpenYesNo(wk);
        *state = STATE_ASK;
    }
}

static void DeleteSave_Ask(DeleteSaveWork *wk, u32 *state) {
    switch (ConfirmDialog_Update(wk->dialog)) {
    case 0:
        DeleteSave_Print(wk, MSG_NO_WAY_TO_RECOVER);
        *state = STATE_PRINT_WARNING;
        break;
    case BMPMENU_CANCEL:
        DeleteSave_WipeOut();
        *state = STATE_WAIT_WIPE_OUT;
        break;
    }
}

static void DeleteSave_PrintWarning(DeleteSaveWork *wk, u32 *state) {
    if (DeleteSave_UpdatePrint(wk) == FALSE) {
        DeleteSave_OpenYesNo(wk);
        *state = STATE_ASK_AGAIN;
    }
}

static void DeleteSave_AskAgain(DeleteSaveWork *wk, u32 *state) {
    switch (ConfirmDialog_Update(wk->dialog)) {
    case 0:
        DeleteSave_Print(wk, MSG_DELETING);
        *state = STATE_DELETE;
        break;
    case BMPMENU_CANCEL:
        DeleteSave_WipeOut();
        *state = STATE_WAIT_WIPE_OUT;
        break;
    }
}

static void DeleteSave_VBlank(void *data) {
    WaitIcon_Main(sWaitIcon);
}

// Once the last message is shown, clears the save and restarts the game
static void DeleteSave_Delete(DeleteSaveWork *wk, u32 *state) {
    SaveControl *save;
    u32 i;

    if (DeleteSave_UpdatePrint(wk) == FALSE) {
        sWaitIcon = WaitIcon_Create(NULL, wk->window, 15, 16, HEAPID_SAVEDATA_DELETE);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->window));
        GFL_VBlankSetCallback(DeleteSave_VBlank, NULL);
        save = SaveControl_GetInstance();
        GFL_OvlLoad(OVERLAY_ID(331));
        SaveOutside_Erase(HEAPID_SAVEDATA_DELETE);
        GFL_OvlUnload(OVERLAY_ID(331));
        func_02011558(HEAPID_SAVEDATA_DELETE);
        func_020074ac(save);
        for (i = 0; i < SAVE_BLOCK_COUNT; i++) {
            if (func_020074ec(save, i, HEAPID_SAVEDATA_DELETE) == TRUE) {
                func_020076a4(save, i, HEAPID_SAVEDATA_DELETE);
            }
            freeIntermediateSaveExtraBlksAfterLoad(save, i);
        }
        GFL_VBlankResetCallback();
        sys_reset(0);
    }
}

static void DeleteSave_WaitWipeOut(DeleteSaveWork *wk, u32 *state) {
    if (GFL_WipeIsFinished() == TRUE) {
        DeleteSave_RemoveVBlankTask(wk);
        DeleteSave_FreeWindow(wk);
        DeleteSave_FreeMsg(wk);
        DeleteSave_FreeBG();
        *state = STATE_END;
    }
}

static void DeleteSave_InitVRAM(void) {
    BGSysVRAMConfig vramConfig = {
        GX_VRAM_BG_128_A, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
        GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_NONE, GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
    };

    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&vramConfig);
}

// The message window on BG 0 over a blue backdrop
static void DeleteSave_InitBG(void) {
    GFL_BGSysCreate(HEAPID_SAVEDATA_DELETE);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&lcdConfig);
    }
    {
        BGSetup setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0x00000), 0x8000, GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE };

        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    }
    GFL_BGSysResetStdPalette(0, GX_RGB(12, 12, 31));
    GFL_BGSysResetStdPalette(4, GX_RGB(12, 12, 31));
}

static void DeleteSave_FreeBG(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysFree();
}

// The messages are read in language 0 while the screen is up
static void DeleteSave_InitMsg(DeleteSaveWork *wk) {
    wk->langId = GFL_MsgDataGetDefaultLangID();
    GFL_MsgDataSetDefaultLangID(0);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_DELETE_SAVE, HEAPID_SAVEDATA_DELETE);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, HEAPID_SAVEDATA_DELETE);
    wk->strbuf = GFL_StrBufCreate(1024, HEAPID_SAVEDATA_DELETE);
    wk->tcbManager = GFL_TCBExMgrCreate(HEAPID_SAVEDATA_DELETE, HEAPID_SAVEDATA_DELETE, 1, 4);
    wk->keyCursor = KeyCursor_Create(15, TRUE, FALSE, HEAPID_SAVEDATA_DELETE);
}

static void DeleteSave_FreeMsg(DeleteSaveWork *wk) {
    KeyCursor_Free(wk->keyCursor);
    GFL_TCBExMgrFree(wk->tcbManager);
    GFL_StrBufFree(wk->strbuf);
    GFL_FontFree(wk->font);
    GFL_MsgDataFree(wk->msgData);
    GFL_MsgDataSetDefaultLangID(wk->langId);
}

static void DeleteSave_InitWindow(DeleteSaveWork *wk) {
    BmpWin_InitAllocator(HEAPID_SAVEDATA_DELETE);
    wk->window = BmpWin_CreateDynamic(0, 1, 19, 30, 4, 15, 1);
    LoadSysMsgBox(0, 1, 14, 0, HEAPID_SAVEDATA_DELETE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 15 * 0x20, 0x20, HEAPID_SAVEDATA_DELETE);
}

static void DeleteSave_FreeWindow(DeleteSaveWork *wk) {
    BmpWin_Free(wk->window);
    BmpWin_FreeAllocator();
}

static void DeleteSave_VBlankTask(TCB *tcb, void *data) {
    GFL_BGSysUpdate();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

static void DeleteSave_AddVBlankTask(DeleteSaveWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(DeleteSave_VBlankTask, wk, 0);
}

static void DeleteSave_RemoveVBlankTask(DeleteSaveWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static void DeleteSave_Print(DeleteSaveWork *wk, u32 messageId) {
    GFL_MsgDataLoadStrbuf(wk->msgData, messageId, wk->strbuf);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->window), 15);
    BmpWin_DrawFrame(wk->window, WINFRAME_TRANSFER_NONE, 1, 14);
    wk->printStream = func_02022268(wk->window, 0, 0, wk->strbuf, wk->font, func_02017c50(0), wk->tcbManager, 10,
                                    HEAPID_SAVEDATA_DELETE, 15);
    wk->continued = FALSE;
    BmpWin_Transfer(wk->window);
}

// Returns FALSE once the message has ended
static BOOL DeleteSave_UpdatePrint(DeleteSaveWork *wk) {
    GFL_TCBExMgrUpdate(wk->tcbManager);
    KeyCursor_Update(wk->keyCursor, wk->printStream, wk->window);
    switch (func_020223b4(wk->printStream)) {
    case PRINT_STREAM_RUNNING:
        wk->continued = FALSE;
        break;
    case PRINT_STREAM_PAUSED:
        if (wk->continued == FALSE && (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            func_020223bc(wk->printStream);
            wk->continued = TRUE;
        }
        break;
    case PRINT_STREAM_DONE:
        func_020223cc(wk->printStream);
        wk->continued = FALSE;
        return FALSE;
    }
    return TRUE;
}

// No is chosen at first
static void DeleteSave_OpenYesNo(DeleteSaveWork *wk) {
    ConfirmDialogSetup setup;

    setup.bg = 0;
    setup.x = 24;
    setup.y = 13;
    setup.palette = 15;
    setup.unk4 = 0;
    wk->dialog = ShopUI_CreateConfirmDialog(&setup, 1, 14, 1, HEAPID_SAVEDATA_DELETE);
}

static void DeleteSave_WipeIn(void) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, HEAPID_SAVEDATA_DELETE);
}

static void DeleteSave_WipeOut(void) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, HEAPID_SAVEDATA_DELETE);
}
