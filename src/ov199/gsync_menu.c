// The Game Sync menu: three touch buttons, Game Sync itself (whose button also shows the wireless signal), the Wi-Fi
// settings and return. The name is the ROM's string, from GFL_HeapAllocate's calls. Function names are ours.

#include "types.h"
#include "app/gsync.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
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
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wipe.h"
#include "text/system/0044.h"
#include "text/system/wifi_error.h"

// The menu shares the IR battle menu's heap
#define HEAPID_GSYNC_MENU HEAPID_IRC_BATTLE_MENU

#define GSYNC_MENU_BUTTON_WIN_MAX 4
#define GSYNC_MENU_ACTOR_COUNT 3

// The touch buttons, and the actors drawn on them
enum {
    GSYNC_MENU_BUTTON_GSYNC,
    GSYNC_MENU_BUTTON_WIFI_SETTINGS,
    GSYNC_MENU_BUTTON_RETURN,
};

// The actors' first sequences. The Game Sync button's sequence also shows the wireless signal
#define GSYNC_MENU_SEQ_GSYNC 14
#define GSYNC_MENU_SEQ_WIFI_SETTINGS 13

typedef struct GSyncMenuWork GSyncMenuWork;
typedef void (*GSyncMenuStateFunc)(GSyncMenuWork *wk);
typedef BOOL (*GSyncMenuTouchFunc)(u32 button, GSyncMenuWork *wk);

// A window's rectangle, in tiles
typedef struct {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
} GSyncMenuWinRect;

struct GSyncMenuWork {
    GSyncMenuStateFunc state;
    // What a touched button does
    GSyncMenuTouchFunc touchFunc;
    // The proc's result, a GSYNC_RESULT_*
    u32 result;
    u16 heapId;
    BmpWin *buttonWins[GSYNC_MENU_BUTTON_WIN_MAX];
    ButtonMan *buttonMan;
    MsgData *msgData;
    // The messages for the Wi-Fi settings on a DSi
    MsgData *dsiMsgData;
    u32 unk2C;
    Font *font;
    u32 unk34;
    StrBuf *strBuf;
    // The message window's frame, as LoadCursorImageEndOfHeap gives it
    u32 frameChars;
    // The sub screen BG's characters
    u32 bgChars;
    // The sub screen's first 7 palettes, as loaded and brightened
    u16 palettes[2][7][16];
    u8 unk204[0x20];
    BmpWin *msgWin;
    PrintStream *printStream;
    KeyCursor *keyCursor;
    TCBExManager *tcbMgr;
    PrintQueue *printQueue;
    AppTaskMenu *yesNoMenu;
    AppTaskMenuItem yesNoItems[2];
    AppTaskMenuRes *menuRes;
    EventGameSync *param;
    u32 buttonCount;
    GameData *gameData;
    GameSystem *gsys;
    // The scroll of the sub screen's background
    s32 bgScroll;
    u32 unk26C;
    // The button touched
    u32 selected;
    u32 unk274;
    // The wireless status, from func_02012be4, and the last frame's
    u32 wifiStatus;
    u32 prevWifiStatus;
    // The sub screen's palettes kept while they are dimmed
    u16 *objPalettes;
    u16 *bgPalettes;
    WaitIcon *waitIcon;
    // Set once the menu ends and no longer boots the game's communication
    BOOL ending;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    u32 unk29C;
    ClActor *actors[GSYNC_MENU_ACTOR_COUNT];
    ClActUnit *clUnit;
    TCB *vblankTask;
};

static void GSyncMenu_SetState(GSyncMenuWork *wk, GSyncMenuStateFunc state);
static void GSyncMenu_ChangeState(GSyncMenuWork *wk, GSyncMenuStateFunc state, int line);
static void GSyncMenu_CreateBGs(GSyncMenuWork *wk);
static void GSyncMenu_StartWaitIcon(GSyncMenuWork *wk);
static void GSyncMenu_BrightenPalettes(u16 (*palettes)[16], const u16 *add, int count);
static void GSyncMenu_StateFadeOut(GSyncMenuWork *wk);
static void GSyncMenu_StateWifiSettingsAnswer(GSyncMenuWork *wk);
static void GSyncMenu_StateWifiSettingsYesNo(GSyncMenuWork *wk);
static void GSyncMenu_OnButtonAnimEnd(u32 param, fx32 frame);
static void GSyncMenu_StateWaitButtonAnim(GSyncMenuWork *wk);
static void GSyncMenu_CreateButtons(int count, const u32 *msgIds, GSyncMenuWork *wk, GSyncMenuWinRect *rects,
                                    const s32 *centerX, const s32 *centerY);
static void GSyncMenu_FreeButtons(GSyncMenuWork *wk);
static void GSyncMenu_ButtonCallback(u32 button, u32 event, void *work);
static void GSyncMenu_CreateActors(GSyncMenuWork *wk);
static void GSyncMenu_LoadGraphics(GSyncMenuWork *wk);
static void GSyncMenu_StateShowButtons(GSyncMenuWork *wk);
static void GSyncMenu_FreeGraphics(GSyncMenuWork *wk);
static void GSyncMenu_StateWaitFadeOut(GSyncMenuWork *wk);
static void GSyncMenu_StateWaitMessageTouch(GSyncMenuWork *wk);
static void GSyncMenu_StatePressButton(GSyncMenuWork *wk);
static BOOL GSyncMenu_OnTouch(u32 button, GSyncMenuWork *wk);
static void GSyncMenu_SetSignalSequence(GSyncMenuWork *wk, int sequence);
static void GSyncMenu_UpdateSignal(GSyncMenuWork *wk);
static void GSyncMenu_StateSelect(GSyncMenuWork *wk);
static void GSyncMenu_DimPalettes(GSyncMenuWork *wk, u16 buffer, int count);
static void GSyncMenu_DimSubScreen(GSyncMenuWork *wk, BOOL dim);
static void GSyncMenu_CreateYesNo(GSyncMenuWork *wk);
static void GSyncMenu_ClearMessage(GSyncMenuWork *wk);
static void GSyncMenu_FreeMessage(GSyncMenuWork *wk);
static BOOL GSyncMenu_IsMessageFinished(GSyncMenuWork *wk);
static void GSyncMenu_PrintMessage(GSyncMenuWork *wk, int height, BOOL stream);
static void GSyncMenu_PrintMessageStream(GSyncMenuWork *wk);
static void GSyncMenu_StateSaveAsk(GSyncMenuWork *wk);
static void GSyncMenu_StateWaitSave(GSyncMenuWork *wk);
static void GSyncMenu_StateStartSave(GSyncMenuWork *wk);
static void GSyncMenu_StateSaveAnswer(GSyncMenuWork *wk);
static void GSyncMenu_StateSaveYesNo(GSyncMenuWork *wk);
static void GSyncMenu_VBlank(TCB *task, void *work);
static BOOL GSyncMenu_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL GSyncMenu_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL GSyncMenu_ProcExit(GameProc *proc, u32 *state, void *param, void *work);

// What the Game Sync button's palettes are brightened by in each color
static u16 sBrightenAmounts[16] = { 0, 5, 15, 15, 15, 15, 15, 5, 5, 0, 0, 0, 0, 15, 15, 15 };

static BGSysVRAMConfig sGSyncMenuVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const ClActSysSetup sGSyncMenuClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 100, 100, 100, 100, 16, 16 };

static void GSyncMenu_SetState(GSyncMenuWork *wk, GSyncMenuStateFunc state) {
    wk->state = state;
}

// Changes the state, from a line that the debug build reported
static void GSyncMenu_ChangeState(GSyncMenuWork *wk, GSyncMenuStateFunc state, int line) {
    GSyncMenu_SetState(wk, state);
}

static void GSyncMenu_CreateBGs(GSyncMenuWork *wk) {
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x6800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearBG(0);
        GFL_BGSysLoadScr(0);
        GFL_BGSysSetBGPriority(0, 0);
        GFL_BGSysSetBGEnabled(0, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(4, TRUE);
        GFL_BGSysLoadScr(4);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysFillChar(5, 0, 1, 0);
        GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(5);
        GFL_BGSysSetBGEnabled(5, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(6, TRUE);
        GFL_BGSysLoadScr(6);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(7, TRUE);
        GFL_BGSysLoadScr(7);
    }
}

static void GSyncMenu_StartWaitIcon(GSyncMenuWork *wk) {
    if (wk->waitIcon != NULL) {
        WaitIcon_Free(wk->waitIcon);
        wk->waitIcon = NULL;
    }
    wk->waitIcon = WaitIcon_CreateTCBEx(wk->tcbMgr, wk->msgWin, 15, 16, wk->heapId);
}

// Adds add's amount to each color's components, up to 31
static void GSyncMenu_BrightenPalettes(u16 (*palettes)[16], const u16 *add, int count) {
    int i, j;

    for (i = 0; i < count; i++) {
        u16 *palette = palettes[i];

        for (j = 0; j < 16; j++) {
            u16 r = palette[j] & 0x1f;
            u16 g = (palette[j] & 0x3e0) >> 5;
            u16 b = (palette[j] & 0x7c00) >> 10;

            r += add[j];
            g += add[j];
            b += add[j];
            if (r > 31) {
                r = 31;
            }
            if (g > 31) {
                g = 31;
            }
            if (b > 31) {
                b = 31;
            }
            palette[j] = r + (g << 5) + (b << 10);
        }
    }
}

static void GSyncMenu_StateFadeOut(GSyncMenuWork *wk) {
    wk->ending = TRUE;
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
    switch (wk->result) {
    case GSYNC_RESULT_CONNECT:
    case GSYNC_RESULT_WIFI_SETTINGS:
        if (func_02042788()) {
            GameCommSys_ExitReq(GSYS_GetGameCommSystem(wk->gsys));
        }
        break;
    }
    GSyncMenu_ChangeState(wk, GSyncMenu_StateWaitFadeOut, 426);
}

static void GSyncMenu_StateWifiSettingsAnswer(GSyncMenuWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->yesNoMenu)) {
        if (AppTaskMenu_GetCursorPos(wk->yesNoMenu) == 0) {
            GSyncMenu_ChangeState(wk, GSyncMenu_StateSaveAsk, 438);
        } else {
            GFL_BGSysClearScr(5);
            wk->result = GSYNC_RESULT_NONE;
            GSyncMenu_ChangeState(wk, GSyncMenu_StateShowButtons, 443);
        }
        AppTaskMenu_Free(wk->yesNoMenu);
        wk->yesNoMenu = NULL;
        GSyncMenu_DimSubScreen(wk, FALSE);
    }
}

static void GSyncMenu_StateWifiSettingsYesNo(GSyncMenuWork *wk) {
    if (GSyncMenu_IsMessageFinished(wk)) {
        GSyncMenu_CreateYesNo(wk);
        GSyncMenu_ChangeState(wk, GSyncMenu_StateWifiSettingsAnswer, 457);
    }
}

// Called by the touched button's actor once its animation ends
static void GSyncMenu_OnButtonAnimEnd(u32 param, fx32 frame) {
    GSyncMenuWork *wk = (GSyncMenuWork *)param;

    func_0204c5bc(wk->actors[wk->selected]);
    func_0204c520(wk->actors[wk->selected], FALSE);
    if (wk->result == GSYNC_RESULT_WIFI_SETTINGS) {
        if (wk->msgWin != NULL) {
            BmpWin_Free(wk->msgWin);
            wk->msgWin = NULL;
        }
        if (hw_isDSi()) {
            // A DSi's own settings can't be opened from here
            GFL_MsgDataLoadStrbuf(wk->dsiMsgData, WifiError_Text_PleaseConfigureConnectionSettings, wk->strBuf);
            GSyncMenu_PrintMessage(wk, 10, FALSE);
            GSyncMenu_DimSubScreen(wk, TRUE);
            GSyncMenu_ChangeState(wk, GSyncMenu_StateWaitMessageTouch, 479);
        } else {
            GFL_MsgDataLoadStrbuf(wk->msgData, Bank0044_Text_WantChooseNintendoWi, wk->strBuf);
            GSyncMenu_PrintMessageStream(wk);
            GSyncMenu_DimSubScreen(wk, TRUE);
            GSyncMenu_ChangeState(wk, GSyncMenu_StateWifiSettingsYesNo, 486);
        }
    } else if (wk->result == GSYNC_RESULT_CONNECT || wk->result == GSYNC_RESULT_WIFI_SETTINGS) {
        GSyncMenu_ChangeState(wk, GSyncMenu_StateSaveAsk, 498);
    } else {
        GSyncMenu_ChangeState(wk, GSyncMenu_StateFadeOut, 501);
    }
}

static void GSyncMenu_StateWaitButtonAnim(GSyncMenuWork *wk) {
}

// Creates the buttons' windows, with their messages centered on centerX and centerY where they are given
static void GSyncMenu_CreateButtons(int count, const u32 *msgIds, GSyncMenuWork *wk, GSyncMenuWinRect *rects,
                                    const s32 *centerX, const s32 *centerY) {
    int i;

    wk->buttonCount = count;
    func_020232d8();
    for (i = 0; i < GSYNC_MENU_BUTTON_WIN_MAX; i++) {
        if (wk->buttonWins[i] != NULL) {
            BmpWin_ClearScreen(wk->buttonWins[i]);
            BmpWin_ClearFrame(wk->buttonWins[i], 2);
            BmpWin_Free(wk->buttonWins[i]);
        }
        wk->buttonWins[i] = NULL;
    }
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 8 * 0x20, 0x20, wk->heapId);
    for (i = 0; i < count; i++) {
        s32 x;
        s32 y;

        wk->buttonWins[i] = BmpWin_CreateDynamic(7, rects[i].x, rects[i].y, rects[i].width, rects[i].height, 8, FALSE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->buttonWins[i]), 0);
        GFL_MsgDataLoadStrbuf(wk->msgData, msgIds[i], wk->strBuf);
        GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
        if (centerX != NULL) {
            x = centerX[i] - GFL_FontGetBlockWidth(wk->strBuf, wk->font, 0) / 2;
        } else {
            x = 0;
        }
        if (centerY != NULL) {
            y = centerY[i] - GFL_FontGetBlockHeight(wk->strBuf, wk->font) / 2;
        } else {
            y = 0;
        }
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->buttonWins[i]), x, y, wk->strBuf, wk->font);
        BmpWin_FlushChar(wk->buttonWins[i]);
        BmpWin_FlushMap(wk->buttonWins[i]);
    }
    if (wk->buttonMan != NULL) {
        GFL_BMN_Delete(wk->buttonMan);
    }
    wk->buttonMan = NULL;
    GFL_BGSysLoadScr(7);
}

static void GSyncMenu_FreeButtons(GSyncMenuWork *wk) {
    int i;

    if (wk->buttonMan != NULL) {
        GFL_BMN_Delete(wk->buttonMan);
    }
    wk->buttonMan = NULL;
    for (i = 0; i < GSYNC_MENU_BUTTON_WIN_MAX; i++) {
        if (wk->buttonWins[i] != NULL) {
            BmpWin_ClearScreen(wk->buttonWins[i]);
            GFL_BGSysQueueScrLoad(5);
            BmpWin_ClearFrame(wk->buttonWins[i], 2);
            BmpWin_Free(wk->buttonWins[i]);
        }
        wk->buttonWins[i] = NULL;
    }
    wk->buttonCount = 0;
}

static void GSyncMenu_ButtonCallback(u32 button, u32 event, void *work) {
    GSyncMenuWork *wk = work;

    switch (event) {
    case BMN_EVENT_TOUCH:
        if (wk->touchFunc != NULL) {
            if (wk->touchFunc(button, wk)) {
                return;
            }
        }
        break;
    }
}

static void GSyncMenu_CreateActors(GSyncMenuWork *wk) {
    u8 x[] = { 128, 128, 224 };
    u8 y[] = { 96, 96, 177 };
    u8 sequences[] = { GSYNC_MENU_SEQ_GSYNC, GSYNC_MENU_SEQ_WIFI_SETTINGS, 0 };
    ClActorSetup setup;
    int i;

    for (i = 0; i < GSYNC_MENU_ACTOR_COUNT; i++) {
        setup.x = x[i];
        setup.y = y[i];
        setup.sequence = sequences[i];
        setup.priority = 0;
        setup.bgPriority = 2;
        wk->actors[i] =
            func_0204c040(wk->clUnit, wk->chars, wk->palette, wk->cellAnims, &setup, CLACT_SURFACE_SUB, wk->heapId);
        func_0204c520(wk->actors[i], FALSE);
        func_0204c124(wk->actors[i], TRUE);
        func_0204c318(wk->actors[i], 1);
    }
}

static void GSyncMenu_LoadGraphics(GSyncMenuWork *wk) {
    ArcTool *arc;
    void *file;
    NNSG2dPaletteData *palette;

    wk->strBuf = GFL_StrBufCreate(300, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0044, wk->heapId);
    wk->dsiMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_WIFI_ERROR, wk->heapId);

    arc = GFL_ArcSysCreateFileHandle(ARCID_GSYNC_MENU, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_SUB_BG, 0, 0, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 0, wk->heapId);
    file = GFL_G2DIOReadNCLRArc(arc, 1, &palette, wk->heapId);
    sys_memcpy(palette->rawData, wk->palettes[0], sizeof(wk->palettes[0]));
    sys_memcpy(palette->rawData, wk->palettes[1], sizeof(wk->palettes[1]));
    GSyncMenu_BrightenPalettes(wk->palettes[1], sBrightenAmounts, 7);
    GFL_HeapFree(file);
    wk->bgChars = GFL_BGSysLoadArcNCGRDynamic(arc, 4, 4, 0, FALSE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 9, 4, 0, CHAR_POS(wk->bgChars), 0, FALSE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 14, 6, 0, CHAR_POS(wk->bgChars), 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);

    wk->frameChars = LoadCursorImageEndOfHeap(5, 12, 0, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 13 * 0x20, 0x20, wk->heapId);

    arc = GFL_ArcSysCreateFileHandle(ARCID_GSYNC_MENU, wk->heapId);
    wk->chars = func_0204b81c(arc, 5, FALSE, CLACT_VRAM_SUB, wk->heapId);
    wk->palette = func_0204bbb8(arc, 3, CLACT_VRAM_SUB, 0, 0, 10, wk->heapId);
    wk->cellAnims = func_0204bde0(arc, 7, 16, wk->heapId);
    GFL_ArcToolFree(arc);
    GSyncMenu_CreateActors(wk);
}

static void GSyncMenu_StateShowButtons(GSyncMenuWork *wk) {
    static const s32 sButtonTextX[] = { 72 };
    static const s32 sButtonTextY[] = { 8 };
    static const TouchRect sTouchRects[] = {
        { 72, 136, 40, 216 },
        { 160, 192, 0, 56 },
        { 160, 192, 192, 255 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };
    static GSyncMenuWinRect sButtonWinRects[] = {
        { 7, 12, 18, 5 },
    };
    u32 msgIds[] = { 0 };

    GSyncMenu_CreateButtons(1, msgIds, wk, sButtonWinRects, sButtonTextX, sButtonTextY);
    wk->buttonMan = GFL_BMN_Create(sTouchRects, GSyncMenu_ButtonCallback, wk, wk->heapId);
    wk->touchFunc = GSyncMenu_OnTouch;
    GSyncMenu_ChangeState(wk, GSyncMenu_StateSelect, 801);
}

static void GSyncMenu_FreeGraphics(GSyncMenuWork *wk) {
    func_020232d8();
    GSyncMenu_FreeButtons(wk);
    GFL_BGSysFreeFilledChar(5, 1, 0);
    GFL_BGSysFreeCharMemory(5, CHAR_POS(wk->frameChars), CHAR_SIZE(wk->frameChars));
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_MsgDataFree(wk->msgData);
    GFL_MsgDataFree(wk->dsiMsgData);
    GFL_FontFree(wk->font);
    GFL_StrBufFree(wk->strBuf);
}

static void GSyncMenu_StateWaitFadeOut(GSyncMenuWork *wk) {
    if (GFL_WipeIsFinished()) {
        switch (wk->result) {
        case GSYNC_RESULT_CONNECT:
        case GSYNC_RESULT_WIFI_SETTINGS:
            if (!GameCommSys_BootCheck(GSYS_GetGameCommSystem(wk->gsys))) {
                GSyncMenu_ChangeState(wk, NULL, 837);
            }
            break;
        default:
            GSyncMenu_ChangeState(wk, NULL, 841);
            break;
        }
    }
}

static void GSyncMenu_StateWaitMessageTouch(GSyncMenuWork *wk) {
    if (GSyncMenu_IsMessageFinished(wk) && func_0203da48()) {
        GSyncMenu_FreeMessage(wk);
        wk->prevWifiStatus = 0;
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        GSyncMenu_DimSubScreen(wk, FALSE);
        GSyncMenu_ChangeState(wk, GSyncMenu_StateShowButtons, 864);
    }
}

static void GSyncMenu_StatePressButton(GSyncMenuWork *wk) {
    ClActorCallback callback;

    callback.type = CLACT_CALLBACK_LAST_FRAME;
    callback.param = (u32)wk;
    callback.func = GSyncMenu_OnButtonAnimEnd;
    func_0204c520(wk->actors[wk->selected], TRUE);
    func_0204c5b0(wk->actors[wk->selected], &callback);
    func_0204c56c(wk->actors[wk->selected]);
    GSyncMenu_ChangeState(wk, GSyncMenu_StateWaitButtonAnim, 886);
}

static BOOL GSyncMenu_OnTouch(u32 button, GSyncMenuWork *wk) {
    wk->selected = button;
    switch (button) {
    case GSYNC_MENU_BUTTON_GSYNC:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        func_0204c488(wk->actors[GSYNC_MENU_BUTTON_GSYNC], GSYNC_MENU_SEQ_GSYNC);
        GSyncMenu_ChangeState(wk, GSyncMenu_StatePressButton, 905);
        wk->result = GSYNC_RESULT_CONNECT;
        break;
    case GSYNC_MENU_BUTTON_WIFI_SETTINGS:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        wk->result = GSYNC_RESULT_WIFI_SETTINGS;
        GSyncMenu_ChangeState(wk, GSyncMenu_StatePressButton, 915);
        break;
    case GSYNC_MENU_BUTTON_RETURN:
        GFL_SndSEPlay(SEQ_SE_SYS_70);
        wk->result = GSYNC_RESULT_NONE;
        GSyncMenu_ChangeState(wk, GSyncMenu_StatePressButton, 921);
        break;
    }
    return TRUE;
}

static void GSyncMenu_SetSignalSequence(GSyncMenuWork *wk, int sequence) {
    if (sequence != func_0204c4a0(wk->actors[GSYNC_MENU_BUTTON_GSYNC])) {
        func_0204c520(wk->actors[GSYNC_MENU_BUTTON_GSYNC], TRUE);
        func_0204c488(wk->actors[GSYNC_MENU_BUTTON_GSYNC], sequence);
    }
}

// Shows the wireless signal's strength on the Game Sync button, from the bits of func_02012be4's status that
// are set, strongest first
static void GSyncMenu_UpdateSignal(GSyncMenuWork *wk) {
    if (wk->wifiStatus != wk->prevWifiStatus) {
        if (wk->wifiStatus & 0x40) {
            GSyncMenu_SetSignalSequence(wk, 24);
        } else if (wk->wifiStatus & 0x80) {
            GSyncMenu_SetSignalSequence(wk, 22);
        } else if (wk->wifiStatus & 0x100) {
            GSyncMenu_SetSignalSequence(wk, 21);
        } else if (wk->wifiStatus & 0x200) {
            GSyncMenu_SetSignalSequence(wk, 21);
        } else {
            GSyncMenu_SetSignalSequence(wk, GSYNC_MENU_SEQ_GSYNC);
        }
    }
}

static void GSyncMenu_StateSelect(GSyncMenuWork *wk) {
    if (GFL_WipeIsFinished()) {
        GFL_BMN_Main(wk->buttonMan);
    }
    GSyncMenu_UpdateSignal(wk);
}

// Dims the first count palettes of the sub screen's BG or OBJ palettes. buffer is both the PaletteFade buffer and the
// VRAM it loads from, whose IDs are the same for the sub screen
static void GSyncMenu_DimPalettes(GSyncMenuWork *wk, u16 buffer, int count) {
    PaletteFade *fade = PaletteFade_Create(wk->heapId);
    u16 *faded;

    PaletteFade_AllocBuffer(fade, buffer, 0x200, wk->heapId);
    PaletteFade_LoadFromVRAM(fade, buffer, 0, count * 32);
    PaletteFade_BlendBuffer(fade, buffer, 0, count * 16, 6, 0);
    faded = PaletteFade_GetFadedBuffer(fade, buffer);
    cp15_flushDC(faded, count * 32);
    switch (buffer) {
    case PALFADE_BUFFER_SUB_OBJ:
        gfxUploadStdPaletteObjB(faded, 0, count * 32);
        break;
    case PALFADE_BUFFER_SUB_BG:
        gfxUploadStdPaletteBGB(faded, 0, count * 32);
        break;
    }
    PaletteFade_FreeBuffer(fade, buffer);
    PaletteFade_Free(fade);
}

// Dims the sub screen behind a message, or restores it
static void GSyncMenu_DimSubScreen(GSyncMenuWork *wk, BOOL dim) {
    if (dim) {
        sys_memcpy((void *)HW_DB_OBJ_PLTT, wk->objPalettes, 0x200);
        sys_memcpy((void *)HW_DB_BG_PLTT, wk->bgPalettes, 0x200);
        GSyncMenu_DimPalettes(wk, PALFADE_BUFFER_SUB_OBJ, 14);
        GSyncMenu_DimPalettes(wk, PALFADE_BUFFER_SUB_BG, 9);
    } else {
        gfxUploadStdPaletteObjB(wk->objPalettes, 0, 14 * 32);
        gfxUploadStdPaletteBGB(wk->bgPalettes, 0, 9 * 32);
    }
}

static void GSyncMenu_CreateYesNo(GSyncMenuWork *wk) {
    AppTaskMenuInit init;

    init.heapId = wk->heapId;
    init.itemCount = 2;
    init.items = wk->yesNoItems;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 14;
    init.width = 13;
    init.height = 3;
    wk->yesNoItems[0].str = GFL_StrBufCreate(100, wk->heapId);
    GFL_MsgDataLoadStrbuf(wk->msgData, Bank0044_Text_Yes, wk->yesNoItems[0].str);
    wk->yesNoItems[0].color = PRINT_COLOR(14, 15, 0);
    wk->yesNoItems[1].str = GFL_StrBufCreate(100, wk->heapId);
    GFL_MsgDataLoadStrbuf(wk->msgData, Bank0044_Text_No, wk->yesNoItems[1].str);
    wk->yesNoItems[1].color = PRINT_COLOR(14, 15, 0);
    wk->yesNoMenu = AppTaskMenu_Create(&init, wk->menuRes);
    AppTaskMenu_SetLocked(wk->yesNoMenu, TRUE);
    GFL_StrBufFree(wk->yesNoItems[0].str);
    GFL_StrBufFree(wk->yesNoItems[1].str);
}

static void GSyncMenu_ClearMessage(GSyncMenuWork *wk) {
    if (wk->waitIcon != NULL) {
        WaitIcon_Free(wk->waitIcon);
        wk->waitIcon = NULL;
    }
    BmpWin_ClearFrame(wk->msgWin, 2);
    BmpWin_ClearScreen(wk->msgWin);
    GFL_BGSysQueueScrLoad(5);
}

static void GSyncMenu_FreeMessage(GSyncMenuWork *wk) {
    GSyncMenu_ClearMessage(wk);
    BmpWin_Free(wk->msgWin);
    wk->msgWin = NULL;
}

static BOOL GSyncMenu_IsMessageFinished(GSyncMenuWork *wk) {
    if (wk->printStream != NULL) {
        if (wk->msgWin != NULL) {
            KeyCursor_Update(wk->keyCursor, wk->printStream, wk->msgWin);
        }
        switch (func_020223b4(wk->printStream)) {
        case PRINT_STREAM_DONE:
            func_020223cc(wk->printStream);
            wk->printStream = NULL;
            break;
        case PRINT_STREAM_PAUSED:
            if (func_0203da48()) {
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
                func_020223bc(wk->printStream);
            }
            break;
        case PRINT_STREAM_RUNNING:
            if (func_0203da2c()) {
                func_020223e0(wk->printStream, 0);
            }
            break;
        }
        return FALSE;
    }
    return TRUE;
}

// Prints strBuf in the message window, at once or a character at a time
static void GSyncMenu_PrintMessage(GSyncMenuWork *wk, int height, BOOL stream) {
    BmpWin *win;

    if (wk->msgWin == NULL) {
        wk->msgWin = BmpWin_CreateDynamic(5, 1, 3, 30, height, 13, TRUE);
    }
    win = wk->msgWin;
    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    if (stream) {
        wk->printStream =
            func_02022268(win, 0, 0, wk->strBuf, wk->font, func_02017bcc(), wk->tcbMgr, 2, wk->heapId, 15);
    } else {
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), 0, 0, wk->strBuf, wk->font);
    }
    BmpWin_DrawFrame(win, TRUE, wk->frameChars, 12);
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysQueueScrLoad(5);
}

static void GSyncMenu_PrintMessageStream(GSyncMenuWork *wk) {
    GSyncMenu_PrintMessage(wk, 4, TRUE);
}

static void GSyncMenu_StateSaveAsk(GSyncMenuWork *wk) {
    GSyncMenu_DimSubScreen(wk, TRUE);
    GFL_BGSysFillScrAsync(5, 0);
    GFL_MsgDataLoadStrbuf(wk->msgData, Bank0044_Text_WouldLikeSaveGame, wk->strBuf);
    GSyncMenu_PrintMessageStream(wk);
    GSyncMenu_ChangeState(wk, GSyncMenu_StateSaveYesNo, 1232);
}

static void GSyncMenu_StateWaitSave(GSyncMenuWork *wk) {
    if (func_02017850(wk->gameData) == 2) {
        GSyncMenu_FreeMessage(wk);
        GSyncMenu_DimSubScreen(wk, FALSE);
        GSyncMenu_ChangeState(wk, GSyncMenu_StateFadeOut, 1258);
    }
}

static void GSyncMenu_StateStartSave(GSyncMenuWork *wk) {
    if (GSyncMenu_IsMessageFinished(wk)) {
        func_0201782c(wk->gameData);
        GSyncMenu_ChangeState(wk, GSyncMenu_StateWaitSave, 1274);
    }
}

static void GSyncMenu_StateSaveAnswer(GSyncMenuWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->yesNoMenu)) {
        if (AppTaskMenu_GetCursorPos(wk->yesNoMenu) == 0) {
            if (func_0200746c(GameData_GetSaveControl(wk->gameData))) {
                GFL_MsgDataLoadStrbuf(wk->msgData, Bank0044_Text_ThereAlreadySavedGame, wk->strBuf);
                GSyncMenu_PrintMessageStream(wk);
                GSyncMenu_ChangeState(wk, GSyncMenu_StateWaitMessageTouch, 1293);
            } else {
                GFL_MsgDataLoadStrbuf(wk->msgData, Bank0044_Text_SavingDontTurnOff, wk->strBuf);
                GSyncMenu_PrintMessage(wk, 4, FALSE);
                GSyncMenu_StartWaitIcon(wk);
                GSyncMenu_ChangeState(wk, GSyncMenu_StateStartSave, 1305);
            }
        } else {
            GFL_BGSysClearScr(5);
            wk->result = GSYNC_RESULT_NONE;
            GSyncMenu_ChangeState(wk, GSyncMenu_StateShowButtons, 1311);
            GSyncMenu_DimSubScreen(wk, FALSE);
        }
        AppTaskMenu_Free(wk->yesNoMenu);
        wk->yesNoMenu = NULL;
    }
}

static void GSyncMenu_StateSaveYesNo(GSyncMenuWork *wk) {
    if (GSyncMenu_IsMessageFinished(wk)) {
        GSyncMenu_CreateYesNo(wk);
        GSyncMenu_ChangeState(wk, GSyncMenu_StateSaveAnswer, 1334);
    }
}

static void GSyncMenu_VBlank(TCB *task, void *work) {
    func_0204b7c8();
}

static BOOL GSyncMenu_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    EventGameSync *event = param;
    GSyncMenuWork *wk;
    s32 brightness;
    u32 color;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_GSYNC_MENU, 0x18000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(GSyncMenuWork), HEAPID_GSYNC_MENU);
    sys_memset(wk, 0, sizeof(GSyncMenuWork));
    wk->heapId = HEAPID_GSYNC_MENU;
    wk->gameData = event->gameData;
    wk->gsys = event->gsys;
    GFL_BGSysSetDisplayLayout(1);
    if (event->fromWifiSettings) {
        brightness = 16;
    } else {
        brightness = -16;
    }
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, brightness);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, brightness);
    GXS_DispOn();
    gfxEngineEnableA();
    GFL_BGSysCreate(wk->heapId);
    BmpWin_InitAllocator(wk->heapId);
    func_020232d0();
    GFL_BGSysSetVRAMBanks(&sGSyncMenuVRAMConfig);
    {
        BGSysLCDConfig lcd = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
        GFL_BGSysSetLCDConfig(&lcd);
    }
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    ClActSys_Create(&sGSyncMenuClActSetup, &sGSyncMenuVRAMConfig, wk->heapId);
    wk->clUnit = func_0204bf1c(40, 0, wk->heapId);
    wk->vblankTask = GFL_VBlankTCBAdd(GSyncMenu_VBlank, wk, 0);
    GSyncMenu_CreateBGs(wk);
    GSyncMenu_LoadGraphics(wk);
    wk->objPalettes = GFL_HeapAllocate(wk->heapId, 0x200, FALSE, "gsync_menu.c", 1446);
    wk->bgPalettes = GFL_HeapAllocate(wk->heapId, 0x200, FALSE, "gsync_menu.c", 1447);
    wk->tcbMgr = GFL_TCBExMgrCreate(wk->heapId, wk->heapId, 3, 0);
    wk->keyCursor = KeyCursor_Create(15, FALSE, TRUE, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    wk->menuRes = AppTaskMenuRes_Create(5, 9, wk->font, wk->printQueue, wk->heapId);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG0, 15, 4);
    color = WIPE_COLOR_BLACK;
    if (event->fromWifiSettings) {
        color = WIPE_COLOR_WHITE;
    }
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, color, 6, 1, wk->heapId);
    event->fromWifiSettings = FALSE;
    GFL_BGSysSetEnabledBGsB(GX_PLANEMASK_ALL);
    GSyncMenu_ChangeState(wk, GSyncMenu_StateShowButtons, 1471);
    wk->param = event;
    func_02042ba8(FALSE, wk->heapId);
    func_0203d564(TRUE);
    return TRUE;
}

static BOOL GSyncMenu_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    GSyncMenuWork *wk = work;
    GSyncMenuStateFunc func = wk->state;
    BOOL done = TRUE;

    if (func != NULL) {
        func(wk);
        done = FALSE;
    }
    if (wk->yesNoMenu != NULL) {
        AppTaskMenu_Update(wk->yesNoMenu);
    }
    if (func_02042788()) {
        wk->prevWifiStatus = wk->wifiStatus;
        wk->wifiStatus = func_02012be4(NULL);
    }
    GFL_TCBExMgrUpdate(wk->tcbMgr);
    func_02021a3c(wk->printQueue);
    GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, wk->bgScroll);
    wk->bgScroll--;
    func_0204b794();
    if (!wk->ending) {
        GSYS_TryBootGameComm(wk->gsys);
    }
    if (GFL_WipeIsFinished() && func_02016bec(wk->gsys)) {
        wk->result = GSYNC_RESULT_NONE;
        done = TRUE;
        Wipe_SetScreenCovered(0, WIPE_COLOR_BLACK);
        Wipe_SetScreenCovered(1, WIPE_COLOR_BLACK);
    }
    return done;
}

static BOOL GSyncMenu_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    EventGameSync *event = param;
    GSyncMenuWork *wk = work;
    int i;

    if (!GFL_WipeIsFinished()) {
        return FALSE;
    }
    for (i = 0; i < GSYNC_MENU_ACTOR_COUNT; i++) {
        func_0204c108(wk->actors[i]);
    }
    func_0204bcd0(wk->palette);
    func_0204b98c(wk->chars);
    func_0204be64(wk->cellAnims);
    GFL_TCBRemove(wk->vblankTask);
    func_0204bf98(wk->clUnit);
    func_0204b758();
    if (wk->waitIcon != NULL) {
        WaitIcon_Free(wk->waitIcon);
        wk->waitIcon = NULL;
    }
    if (wk->printStream != NULL) {
        func_020223cc(wk->printStream);
    }
    if (wk->yesNoMenu != NULL) {
        AppTaskMenu_Free(wk->yesNoMenu);
        wk->yesNoMenu = NULL;
    }
    GSyncMenu_FreeGraphics(wk);
    event->gsyncResult = wk->result;
    GFL_HeapFree(wk->objPalettes);
    GFL_HeapFree(wk->bgPalettes);
    KeyCursor_Free(wk->keyCursor);
    GFL_BGSysReleaseBG(5);
    func_02021c44(wk->printQueue);
    func_02021a18(wk->printQueue);
    if (wk->msgWin != NULL) {
        BmpWin_Free(wk->msgWin);
    }
    GFL_TCBExMgrFree(wk->tcbMgr);
    AppTaskMenuRes_Free(wk->menuRes);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_GSYNC_MENU);
    return TRUE;
}

const GameProcFunctions GSYNC_MENU_PROC_FUNCTIONS = {
    GSyncMenu_ProcInit,
    GSyncMenu_ProcMain,
    GSyncMenu_ProcExit,
};
