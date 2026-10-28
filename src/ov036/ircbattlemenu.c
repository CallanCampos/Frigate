// The IR battle menu: the touch screen menu of the infrared (and Wi-Fi) battle start. The name is the ROM's string,
// from GFL_HeapAllocate's calls. It is the older sibling of gsync_menu.c.
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/event_irc.h"
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
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/join_avenue.h"
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
#include "text/system/startmenu_2.h"

#define IRC_MENU_BUTTON_WIN_MAX 4

// Whether a touch was handled
typedef enum {
    IRC_MENU_TOUCH_HANDLED = 1,
} IrcMenuTouchResult;

typedef struct IrcMenuWork IrcMenuWork;
typedef void (*IrcMenuStateFunc)(IrcMenuWork *wk);
typedef BOOL (*IrcMenuTouchFunc)(u32 button, IrcMenuWork *wk);

// A window's rectangle, in tiles
typedef struct {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
} IrcMenuWinRect;

struct IrcMenuWork {
    IrcMenuStateFunc state;
    // What a touched button does
    IrcMenuTouchFunc touchFunc;
    int mode;
    u32 choice;
    u16 heapId;
    BmpWin *buttonWins[IRC_MENU_BUTTON_WIN_MAX];
    ButtonMan *buttonMan;
    MsgData *msgData;
    Font *font;
    GameData *gameData;
    u32 unk34;
    StrBuf *strBuf;
    // The message window's frame, as LoadCursorImageEndOfHeap gives it
    u32 frameChars;
    // The sub screen BG's characters
    u32 bgChars;
    u32 mainBgChars;
    // The sub screen's first 8 palettes, as loaded and brightened
    u16 palettes[2][8][16];
    u8 unk248[0x20];
    TCB *vblankTask;
    ClActUnit *clUnit;
    u32 clRes[6];
    u32 unk288;
    ClActor *actors2[2];
    ClActor *actors[5];
    KeyCursor *keyCursor;
    // The scroll of the sub screen's background
    s32 bgScroll;
    BmpWin *msgWin;
    PrintStream *printStream;
    TCBExManager *tcbMgr;
    PrintQueue *printQueue;
    AppTaskMenu *yesNoMenu;
    AppTaskMenuItem yesNoItems[2];
    AppTaskMenuRes *menuRes;
    AppTaskMenuWin *menuWin;
    EventIRCWork *param;
    WaitIcon *waitIcon;
    // The sub screen's palettes kept while they are dimmed
    u16 *objPalettes;
    u16 *bgPalettes;
    u32 buttonCount;
    u32 unk2F8;
    u32 unk2FC;
    // The button touched
    u32 selected;
    u8 unk304[2];
    u8 ending;
    u8 unk307;
};

static void IrcMenu_SetState(IrcMenuWork *wk, IrcMenuStateFunc state);
// Sets the state, from a line that the debug build reported
static void IrcMenu_SetStateAtLine(IrcMenuWork *wk, IrcMenuStateFunc state, int line);
static void IrcMenu_CreateBGs(IrcMenuWork *wk);
static void IrcMenu_ShowMainMenu(IrcMenuWork *wk);
static void IrcMenu_ShowPlayerCountMenu(IrcMenuWork *wk);
static void IrcMenu_ShowBattleTypeMenu(IrcMenuWork *wk);
static void IrcMenu_ShowBattleBoxMenu(IrcMenuWork *wk);
static void IrcMenu_CreateMainButtons(IrcMenuWork *wk);
static void IrcMenu_CreatePlayerCountButtons(IrcMenuWork *wk);
static void IrcMenu_CreateBattleTypeButtons(IrcMenuWork *wk);
static void IrcMenu_CreateBattleBoxButtons(IrcMenuWork *wk);
static void IrcMenu_MainUpdate(IrcMenuWork *wk);
static void IrcMenu_SubmenuUpdate(IrcMenuWork *wk);
static void IrcMenu_BattleBoxUpdate(IrcMenuWork *wk);
static void IrcMenu_StartExit(IrcMenuWork *wk);
static void IrcMenu_AskSave(IrcMenuWork *wk);
static void IrcMenu_WaitSaveQuestion(IrcMenuWork *wk);
static BOOL IrcMenu_MainTouch(u32 button, IrcMenuWork *wk);
static BOOL IrcMenu_PlayerCountTouch(u32 button, IrcMenuWork *wk);
static BOOL IrcMenu_BattleTypeTouch(u32 button, IrcMenuWork *wk);
static BOOL IrcMenu_BattleBoxTouch(u32 button, IrcMenuWork *wk);
static BOOL IrcMenu_UpdateMessage(IrcMenuWork *wk);
static void IrcMenu_PrintMessage(IrcMenuWork *wk, BOOL instant);

// The first menu's button windows, where its messages are centered in them, and its touch rectangles
static IrcMenuWinRect sWinRectsA[] = {
    { 6, 6, 10, 4 },
    { 18, 6, 10, 4 },
    { 12, 11, 10, 4 },
    { 6, 19, 16, 4 },
};

static const s32 sCenterX[] = { 0x20, 0x21, 0x28, 0x2a };
static const s32 sCenterY[] = { 8, 8, 16, 8 };

static const TouchRect sTouchRectsA[] = {
    { 0x20, 0x50, 0x10, 0x80 }, { 0x20, 0x50, 0x80, 0xf0 }, { 0x50, 0x80, 0x40, 0xc0 },
    { 0x80, 0xb0, 0x18, 0x48 }, { 0xa0, 0xc0, 0xc0, 0xff }, { 0x50, 0x80, 0x08, 0x40 },
    { 0x50, 0x80, 0xc0, 0xf8 }, { 0x90, 0xb0, 0x48, 0x90 }, { TOUCH_RECT_END, 0, 0, 0 },
};

// The two-button menus' windows, and the first two-button menu's touch rectangles
static IrcMenuWinRect sWinRects2[] = {
    { 8, 8, 24, 4 },
    { 8, 14, 24, 4 },
};

static const TouchRect sTouchRects2[] = {
    { 0x30, 0x58, 0x08, 0xf8 },
    { 0x60, 0x88, 0x08, 0xf8 },
    { 0xa0, 0xc0, 0xc0, 0x00 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The four-button menu's windows and touch rectangles
static IrcMenuWinRect sWinRects3[] = {
    { 8, 2, 24, 4 },
    { 8, 7, 24, 4 },
    { 8, 12, 24, 4 },
    { 8, 17, 24, 4 },
};

static const TouchRect sTouchRects3[] = {
    { 0x00, 0x20, 0x08, 0xf8 }, { 0x28, 0x48, 0x08, 0xf8 }, { 0x50, 0x70, 0x08, 0xf8 },
    { 0x78, 0x98, 0x08, 0xf8 }, { 0xa0, 0xc0, 0xc0, 0x00 }, { TOUCH_RECT_END, 0, 0, 0 },
};

// The touch rectangles of the two-button menu whose choice becomes the event's useBattleBox
static const TouchRect sBattleBoxTouchRects[] = {
    { 0x38, 0x58, 0x08, 0xf8 },
    { 0x68, 0x88, 0x08, 0xf8 },
    { 0xa0, 0xc0, 0xc0, 0x00 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static void IrcMenu_SetState(IrcMenuWork *wk, IrcMenuStateFunc state) {
    wk->state = state;
}

static void IrcMenu_SetStateAtLine(IrcMenuWork *wk, IrcMenuStateFunc state, int line) {
    IrcMenu_SetState(wk, state);
}

static void IrcMenu_CreateBGs(IrcMenuWork *wk) {
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x512,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearBG(0);
        GFL_BGSysLoadScr(0);
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
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysClearBG(1);
        GFL_BGSysLoadScr(1);
        GFL_BGSysSetBGEnabled(1, TRUE);
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
        GFL_BGSysSetBGEnabled(5, TRUE);
        GFL_BGSysFillChar(5, 0, 1, 0);
        GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(5);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xc800),
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
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(7, TRUE);
        GFL_BGSysLoadScr(7);
    }
}

// Dims the first count palettes of the sub screen's BG or OBJ palettes. buffer is both the PaletteFade buffer and the
// VRAM it loads from, whose IDs are the same for the sub screen
static void IrcMenu_DimPalettes(IrcMenuWork *wk, u16 buffer, int count) {
    PaletteFade *fade = PaletteFade_Create(wk->heapId);
    u16 *faded;

    PaletteFade_AllocBuffer(fade, buffer, 0x200, wk->heapId);
    PaletteFade_LoadFromVRAM(fade, buffer, 0, count * 32);
    PaletteFade_BlendBuffer(fade, buffer, 0, count * 16, 6, 0);
    faded = PaletteFade_GetFadedBuffer(fade, buffer);
    DC_FlushRange(faded, count * 32);
    switch (buffer) {
    case PALFADE_BUFFER_SUB_OBJ:
        GXS_LoadOBJPltt(faded, 0, count * 32);
        break;
    case PALFADE_BUFFER_SUB_BG:
        GXS_LoadBGPltt(faded, 0, count * 32);
        break;
    }
    PaletteFade_FreeBuffer(fade, buffer);
    PaletteFade_Free(fade);
}

// Dims the sub screen behind a message, or restores it
static void IrcMenu_SetDimmed(IrcMenuWork *wk, BOOL dim) {
    if (dim) {
        MI_CpuCopy8((void *)HW_DB_OBJ_PLTT, wk->objPalettes, 0x200);
        MI_CpuCopy8((void *)HW_DB_BG_PLTT, wk->bgPalettes, 0x200);
        IrcMenu_DimPalettes(wk, PALFADE_BUFFER_SUB_OBJ, 14);
        IrcMenu_DimPalettes(wk, PALFADE_BUFFER_SUB_BG, 9);
    } else {
        GXS_LoadOBJPltt(wk->objPalettes, 0, 14 * 32);
        GXS_LoadBGPltt(wk->bgPalettes, 0, 9 * 32);
    }
}

static void IrcMenu_CreateWaitIcon(IrcMenuWork *wk) {
    if (wk->waitIcon != NULL) {
        WaitIcon_Free(wk->waitIcon);
        wk->waitIcon = NULL;
    }
    wk->waitIcon = WaitIcon_CreateTCBEx(wk->tcbMgr, wk->msgWin, 15, 16, wk->heapId);
}

static void IrcMenu_ClearMessageWindow(IrcMenuWork *wk) {
    if (wk->waitIcon != NULL) {
        WaitIcon_Free(wk->waitIcon);
        wk->waitIcon = NULL;
    }
    BmpWin_ClearFrame(wk->msgWin, 2);
    BmpWin_ClearScreen(wk->msgWin);
    GFL_BGSysQueueScrLoad(5);
}

static void IrcMenu_FreeMessageWindow(IrcMenuWork *wk) {
    IrcMenu_ClearMessageWindow(wk);
    BmpWin_Free(wk->msgWin);
    wk->msgWin = NULL;
}

// Creates the buttons' windows, with their messages centered on centerX and centerY where they are given
static void IrcMenu_CreateButtonWindows(int count, const u32 *msgIds, IrcMenuWork *wk, IrcMenuWinRect *rects,
                                        const s32 *centerX, const s32 *centerY) {
    int i;

    GFL_BGSysClearScr(7);
    wk->buttonCount = count;
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 8 * 0x20, 0x20, wk->heapId);
    for (i = 0; i < IRC_MENU_BUTTON_WIN_MAX; i++) {
        if (wk->buttonWins[i] != NULL) {
            BmpWin_ClearScreen(wk->buttonWins[i]);
            BmpWin_ClearFrame(wk->buttonWins[i], 2);
            BmpWin_Free(wk->buttonWins[i]);
        }
        wk->buttonWins[i] = NULL;
    }
    for (i = 0; i < count; i++) {
        s32 x;
        s32 y;

        GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
        wk->buttonWins[i] = BmpWin_CreateDynamic(7, rects[i].x, rects[i].y, rects[i].width, rects[i].height, 8, FALSE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->buttonWins[i]), 0);
        GFL_MsgDataLoadStrbuf(wk->msgData, msgIds[i], wk->strBuf);
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
    GFL_BGSysQueueScrLoad(7);
    GFL_BGSysSetBGEnabled(7, TRUE);
}

static void IrcMenu_FreeButtonWindows(IrcMenuWork *wk) {
    int i;

    if (wk->buttonMan != NULL) {
        GFL_BMN_Delete(wk->buttonMan);
    }
    wk->buttonMan = NULL;
    for (i = 0; i < IRC_MENU_BUTTON_WIN_MAX; i++) {
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

static void IrcMenu_ButtonCallback(u32 button, u32 event, void *work) {
    IrcMenuWork *wk = work;

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

// Adds add's amount to each color's components, up to 31
static void IrcMenu_BrightenPalettes(u16 (*palettes)[16], const u16 *add, int count) {
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

// What the sub screen's palettes are brightened by in each color
static u16 sIrcMenuBrightenAmounts[16] = { 0, 5, 15, 15, 15, 15, 15, 5, 5, 0, 0, 0, 0, 15, 15, 15 };

static void IrcMenu_LoadGraphics(IrcMenuWork *wk) {
    ArcTool *arc;
    void *file;
    NNSG2dPaletteData *palette;

    wk->unk2F8 = 0;
    wk->strBuf = GFL_StrBufCreate(300, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_STARTMENU_2, wk->heapId);

    arc = GFL_ArcSysCreateFileHandle(ARCID_IRCBATTLE, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_MAIN_BG, 0, 0, wk->heapId);
    wk->mainBgChars = GFL_BGSysLoadArcNCGRDynamic(arc, 6, 0, 0, FALSE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 13, 0, 0, CHAR_POS(wk->mainBgChars), 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(ARCID_GSYNC_MENU, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 2, PALTYPE_SUB_BG, 0, 0, wk->heapId);
    file = GFL_G2DIOReadNCLRArc(arc, 2, &palette, wk->heapId);
    MI_CpuCopy8(palette->pRawData, wk->palettes[0], sizeof(wk->palettes[0]));
    MI_CpuCopy8(palette->pRawData, wk->palettes[1], sizeof(wk->palettes[1]));
    IrcMenu_BrightenPalettes(wk->palettes[1], sIrcMenuBrightenAmounts, 8);
    GFL_HeapFree(file);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 0, wk->heapId);
    wk->bgChars = GFL_BGSysLoadArcNCGRDynamic(arc, 4, 4, 0, FALSE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 9, 4, 0, CHAR_POS(wk->bgChars), 0, FALSE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 11, 6, 0, CHAR_POS(wk->bgChars), 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);

    wk->frameChars = LoadCursorImageEndOfHeap(5, 12, 0, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 11 * 0x20, 0x20, wk->heapId);
}

static void IrcMenu_LoadSubscreenLayout(IrcMenuWork *wk, u32 fileId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_GSYNC_MENU, wk->heapId);

    GFL_G2DIOLoadNSCRSync(arc, fileId, 6, 0, CHAR_POS(wk->bgChars), 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysSetBGEnabled(6, TRUE);
}

static void IrcMenu_ShowMainMenu(IrcMenuWork *wk) {
    u32 msgIds[] = { Startmenu2_Text_Battle, Startmenu2_Text_Trade, Startmenu2_Text_FeelingCheck,
                     Startmenu2_Text_FriendCode };

    IrcMenu_LoadSubscreenLayout(wk, 11);
    IrcMenu_CreateMainButtons(wk);
    IrcMenu_CreateButtonWindows(4, msgIds, wk, sWinRectsA, sCenterX, sCenterY);
    wk->buttonMan = GFL_BMN_Create(sTouchRectsA, IrcMenu_ButtonCallback, wk, wk->heapId);
    wk->touchFunc = IrcMenu_MainTouch;
    IrcMenu_SetStateAtLine(wk, IrcMenu_MainUpdate, 833);
}

static void IrcMenu_FreeGraphics(IrcMenuWork *wk) {
    GFL_TextRndResetColorIndexLUT();
    IrcMenu_FreeButtonWindows(wk);
    GFL_BGSysFreeFilledChar(5, 1, 0);
    GFL_BGSysFreeCharMemory(5, CHAR_POS(wk->frameChars), CHAR_SIZE(wk->frameChars));
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->font);
    GFL_StrBufFree(wk->strBuf);
    GFL_BGSysSetBGEnabled(5, FALSE);
}

static void IrcMenu_WaitExit(IrcMenuWork *wk) {
    if (GFL_WipeIsFinished()) {
        if (wk->mode != 12) {
            if (!GameCommSys_BootCheck(GSYS_GetGameCommSystem(EventIRC_GetGameSystem(wk->param)))) {
                IrcMenu_SetStateAtLine(wk, NULL, 866);
            }
        } else {
            IrcMenu_SetStateAtLine(wk, NULL, 870);
        }
    }
}

static void IrcMenu_StartExit(IrcMenuWork *wk) {
    wk->ending = TRUE;
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
    if (wk->mode != 12) {
        if (GameCommSys_BootCheck(GSYS_GetGameCommSystem(EventIRC_GetGameSystem(wk->param)))) {
            GameCommSys_ExitReq(GSYS_GetGameCommSystem(EventIRC_GetGameSystem(wk->param)));
        }
    }
    if (wk->mode >= 1 && wk->mode <= 8) {
        JoinAvenue_RecordActivity(30);
    }
    IrcMenu_SetStateAtLine(wk, IrcMenu_WaitExit, 901);
}

// Called by the touched button's actor once its animation ends
static void IrcMenu_OnMainButtonAnimEnd(uptr param, fx32 frame) {
    IrcMenuWork *wk = (IrcMenuWork *)param;

    if (wk->mode == 12) {
        IrcMenu_SetStateAtLine(wk, IrcMenu_StartExit, 917);
        return;
    }
    if (Config_GetSaveBeforeIR((const Config *)getTrainerDataBlkAddress(EventIRC_GetSaveControl(wk->param))) == 1) {
        IrcMenu_SetStateAtLine(wk, IrcMenu_AskSave, 920);
        return;
    }
    if ((u32)(wk->mode - 6) <= 2) {
        IrcMenu_SetStateAtLine(wk, IrcMenu_StartExit, 925);
        return;
    }
    IrcMenu_SetStateAtLine(wk, IrcMenu_ShowPlayerCountMenu, 928);
}

static void IrcMenu_WaitMainButtonAnim(IrcMenuWork *wk) {
}

static void IrcMenu_PressMainButton(IrcMenuWork *wk) {
    ClActorCallback callback;
    int index = wk->selected;

    callback.type = CLACT_CALLBACK_LAST_FRAME;
    callback.param = (uptr)wk;
    callback.func = IrcMenu_OnMainButtonAnimEnd;
    if (index == 5) {
        index = 0;
    } else if (index == 6) {
        index = 1;
    } else if (index == 7) {
        index = 3;
    }
    ClActor_SetAutoAnim(wk->actors[index], TRUE);
    ClActor_SetAnimCallback(wk->actors[index], &callback);
    ClActor_RestartAnim(wk->actors[index]);
    IrcMenu_SetStateAtLine(wk, IrcMenu_WaitMainButtonAnim, 968);
}

static void IrcMenu_WaitMessageTouch(IrcMenuWork *wk) {
    if (IrcMenu_UpdateMessage(wk) && GFL_HIDIsTouchPressed()) {
        IrcMenu_FreeMessageWindow(wk);
        IrcMenu_SetStateAtLine(wk, IrcMenu_MainUpdate, 980);
    }
}

static BOOL IrcMenu_MainTouch(u32 button, IrcMenuWork *wk) {
    BOOL result = FALSE;

    wk->selected = button;
    switch (button) {
    case 0:
    case 5:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressMainButton, 999);
        wk->mode = 1;
        result = TRUE;
        break;
    case 1:
    case 6:
        if (howManyPartyPokesAreNotEggs(GameData_GetParty(wk->gameData)) < 2) {
            GFL_MsgDataLoadStrbuf(wk->msgData, Startmenu2_Text_NeedLeastTwoPokemon, wk->strBuf);
            IrcMenu_PrintMessage(wk, FALSE);
            IrcMenu_SetStateAtLine(wk, IrcMenu_WaitMessageTouch, 1011);
        } else {
            GFL_SndSEPlay(SEQ_SE_SYS_69);
            wk->mode = 6;
            IrcMenu_SetStateAtLine(wk, IrcMenu_PressMainButton, 1017);
            result = TRUE;
        }
        break;
    case 2:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        wk->mode = 8;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressMainButton, 1023);
        result = TRUE;
        break;
    case 3:
    case 7:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        wk->mode = 7;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressMainButton, 1030);
        result = TRUE;
        break;
    case 4:
        GFL_SndSEPlay(SEQ_SE_SYS_70);
        wk->mode = 12;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressMainButton, 1037);
        break;
    }
    return result;
}

static void IrcMenu_MainUpdate(IrcMenuWork *wk) {
    if (GFL_WipeIsFinished()) {
        GFL_BMN_Main(wk->buttonMan);
    }
}

static void IrcMenu_ShowPlayerCountMenu(IrcMenuWork *wk) {
    u32 msgIds[] = { Startmenu2_Text_BattlesTwo, Startmenu2_Text_BattlesFour };

    IrcMenu_CreatePlayerCountButtons(wk);
    IrcMenu_LoadSubscreenLayout(wk, 12);
    IrcMenu_CreateButtonWindows(2, msgIds, wk, sWinRects2, NULL, NULL);
    wk->buttonMan = GFL_BMN_Create(sTouchRects2, IrcMenu_ButtonCallback, wk, wk->heapId);
    wk->touchFunc = IrcMenu_PlayerCountTouch;
    IrcMenu_SetStateAtLine(wk, IrcMenu_SubmenuUpdate, 1083);
}

static void IrcMenu_OnPlayerCountButtonAnimEnd(uptr param, fx32 frame) {
    IrcMenuWork *wk = (IrcMenuWork *)param;

    if (wk->mode == 5) {
        if (BattleBoxSave_HasPkm(getBattleBox(wk->param->save))) {
            IrcMenu_SetStateAtLine(wk, IrcMenu_ShowBattleBoxMenu, 1100);
        } else {
            IrcMenu_SetStateAtLine(wk, IrcMenu_StartExit, 1103);
        }
    } else {
        IrcMenu_SetStateAtLine(wk, IrcMenu_ShowBattleTypeMenu, 1110);
    }
}

static void IrcMenu_WaitPlayerCountButtonAnim(IrcMenuWork *wk) {
}

static void IrcMenu_PressPlayerCountButton(IrcMenuWork *wk) {
    ClActorCallback callback;
    int index = 0;

    if (wk->mode == 5) {
        index = 1;
    }
    callback.type = CLACT_CALLBACK_LAST_FRAME;
    callback.param = (uptr)wk;
    callback.func = IrcMenu_OnPlayerCountButtonAnimEnd;
    ClActor_SetAutoAnim(wk->actors[index], TRUE);
    ClActor_SetAnimCallback(wk->actors[index], &callback);
    ClActor_RestartAnim(wk->actors[index]);
    IrcMenu_SetStateAtLine(wk, IrcMenu_WaitPlayerCountButtonAnim, 1146);
}

static void IrcMenu_OnPlayerCountBackAnimEnd(uptr param, fx32 frame) {
    IrcMenuWork *wk = (IrcMenuWork *)param;

    IrcMenu_SetStateAtLine(wk, IrcMenu_ShowMainMenu, 1155);
}

static void IrcMenu_WaitPlayerCountBackAnim(IrcMenuWork *wk) {
}

static void IrcMenu_PressPlayerCountBack(IrcMenuWork *wk) {
    ClActorCallback callback;

    callback.type = CLACT_CALLBACK_LAST_FRAME;
    callback.param = (uptr)wk;
    callback.func = IrcMenu_OnPlayerCountBackAnimEnd;
    ClActor_SetAutoAnim(wk->actors[wk->selected], TRUE);
    ClActor_SetAnimCallback(wk->actors[wk->selected], &callback);
    ClActor_RestartAnim(wk->actors[wk->selected]);
    IrcMenu_SetStateAtLine(wk, IrcMenu_WaitPlayerCountBackAnim, 1179);
}

static BOOL IrcMenu_PlayerCountTouch(u32 button, IrcMenuWork *wk) {
    IrcMenuTouchResult result = IRC_MENU_TOUCH_HANDLED;

    wk->selected = button;
    switch (button) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressPlayerCountButton, 1196);
        break;
    case 1:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        wk->mode = 5;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressPlayerCountButton, 1202);
        break;
    case 2:
        GFL_SndSEPlay(SEQ_SE_SYS_70);
        wk->mode = 12;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressPlayerCountBack, 1208);
        break;
    }
    return result;
}

static void IrcMenu_ShowBattleTypeMenu(IrcMenuWork *wk) {
    u32 msgIds[] = { Startmenu2_Text_SingleBattle, Startmenu2_Text_DoubleBattle, Startmenu2_Text_TripleBattle,
                     Startmenu2_Text_RotationBattle };

    IrcMenu_CreateBattleTypeButtons(wk);
    IrcMenu_LoadSubscreenLayout(wk, 13);
    IrcMenu_CreateButtonWindows(4, msgIds, wk, sWinRects3, NULL, NULL);
    wk->buttonMan = GFL_BMN_Create(sTouchRects3, IrcMenu_ButtonCallback, wk, wk->heapId);
    wk->touchFunc = IrcMenu_BattleTypeTouch;
    IrcMenu_SetStateAtLine(wk, IrcMenu_SubmenuUpdate, 1237);
}

static void IrcMenu_OnBattleBoxButtonAnimEnd(uptr param, fx32 frame) {
    IrcMenuWork *wk = (IrcMenuWork *)param;

    if (wk->choice == 2) {
        if (wk->mode == 5) {
            wk->mode = 1;
            IrcMenu_SetStateAtLine(wk, IrcMenu_ShowPlayerCountMenu, 1254);
        } else {
            IrcMenu_SetStateAtLine(wk, IrcMenu_ShowBattleTypeMenu, 1258);
        }
    } else {
        IrcMenu_SetStateAtLine(wk, IrcMenu_StartExit, 1262);
    }
}

static void IrcMenu_WaitBattleBoxButtonAnim(IrcMenuWork *wk) {
}

static void IrcMenu_PressBattleBoxButton(IrcMenuWork *wk) {
    u8 indices[3] = { 1, 0, 2 };
    ClActorCallback callback;
    int index = indices[wk->choice];

    callback.type = CLACT_CALLBACK_LAST_FRAME;
    callback.param = (uptr)wk;
    callback.func = IrcMenu_OnBattleBoxButtonAnimEnd;
    ClActor_SetAutoAnim(wk->actors[index], TRUE);
    ClActor_SetAnimCallback(wk->actors[index], &callback);
    ClActor_RestartAnim(wk->actors[index]);
    IrcMenu_SetStateAtLine(wk, IrcMenu_WaitBattleBoxButtonAnim, 1293);
}

static void IrcMenu_BattleBoxUpdate(IrcMenuWork *wk) {
    GFL_BMN_Main(wk->buttonMan);
}

static BOOL IrcMenu_BattleBoxTouch(u32 button, IrcMenuWork *wk) {
    BOOL result = FALSE;

    wk->selected = button;
    switch (button) {
    case 1:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        wk->choice = 0;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressBattleBoxButton, 1325);
        result = TRUE;
        break;
    case 0:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        wk->choice = 1;
        result = TRUE;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressBattleBoxButton, 1331);
        break;
    case 2:
        GFL_SndSEPlay(SEQ_SE_SYS_70);
        wk->choice = 2;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressBattleBoxButton, 1337);
        break;
    }
    return result;
}

static void IrcMenu_ShowBattleBoxMenu(IrcMenuWork *wk) {
    u32 msgIds[] = { Startmenu2_Text_BattleUsingBattleBox, Startmenu2_Text_BattleUsingParty };

    IrcMenu_CreateBattleBoxButtons(wk);
    IrcMenu_LoadSubscreenLayout(wk, 13);
    IrcMenu_CreateButtonWindows(2, msgIds, wk, sWinRects2, NULL, NULL);
    wk->buttonMan = GFL_BMN_Create(sBattleBoxTouchRects, IrcMenu_ButtonCallback, wk, wk->heapId);
    wk->touchFunc = IrcMenu_BattleBoxTouch;
    IrcMenu_SetStateAtLine(wk, IrcMenu_BattleBoxUpdate, 1366);
}

static void IrcMenu_OnBattleTypeButtonAnimEnd(uptr param, fx32 frame) {
    IrcMenuWork *wk = (IrcMenuWork *)param;

    if (wk->mode == 12) {
        IrcMenu_SetStateAtLine(wk, IrcMenu_ShowPlayerCountMenu, 1377);
    } else if (BattleBoxSave_HasPkm(getBattleBox(wk->param->save))) {
        IrcMenu_SetStateAtLine(wk, IrcMenu_ShowBattleBoxMenu, 1382);
    } else {
        IrcMenu_SetStateAtLine(wk, IrcMenu_StartExit, 1385);
    }
}

static void IrcMenu_WaitBattleTypeButtonAnim(IrcMenuWork *wk) {
}

static void IrcMenu_PressBattleTypeButton(IrcMenuWork *wk) {
    ClActorCallback callback;
    int index;

    switch (wk->mode) {
    case 1:
        index = 0;
        break;
    case 2:
        index = 1;
        break;
    case 3:
        index = 2;
        break;
    case 4:
        index = 3;
        break;
    default:
        index = 4;
        break;
    }
    callback.type = CLACT_CALLBACK_LAST_FRAME;
    callback.param = (uptr)wk;
    callback.func = IrcMenu_OnBattleTypeButtonAnimEnd;
    ClActor_SetAutoAnim(wk->actors[index], TRUE);
    ClActor_SetAnimCallback(wk->actors[index], &callback);
    ClActor_RestartAnim(wk->actors[index]);
    IrcMenu_SetStateAtLine(wk, IrcMenu_WaitBattleTypeButtonAnim, 1432);
}

static BOOL IrcMenu_BattleTypeTouch(u32 button, IrcMenuWork *wk) {
    BOOL result = FALSE;

    wk->selected = button;
    switch (button) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        result = TRUE;
        wk->mode = 1;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressBattleTypeButton, 1466);
        break;
    case 1:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        wk->mode = 2;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressBattleTypeButton, 1472);
        result = TRUE;
        break;
    case 2:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        wk->mode = 3;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressBattleTypeButton, 1478);
        result = TRUE;
        break;
    case 3:
        GFL_SndSEPlay(SEQ_SE_SYS_69);
        wk->mode = 4;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressBattleTypeButton, 1484);
        result = TRUE;
        break;
    case 4:
        GFL_SndSEPlay(SEQ_SE_SYS_70);
        wk->mode = 12;
        IrcMenu_SetStateAtLine(wk, IrcMenu_PressBattleTypeButton, 1490);
        break;
    }
    return result;
}

static void IrcMenu_SubmenuUpdate(IrcMenuWork *wk) {
    GFL_BMN_Main(wk->buttonMan);
}

static void IrcMenu_LoadButtonResources(IrcMenuWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_GSYNC_MENU, wk->heapId);

    wk->clRes[3] = ClActSys_LoadChars(arc, 5, FALSE, 1, wk->heapId);
    wk->clRes[1] = ClActSys_LoadPlttRange(arc, 3, 1, 0, 0, 10, wk->heapId);
    wk->clRes[5] = ClActSys_LoadCellAnims(arc, 7, 16, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void IrcMenu_DeleteButtonActors(IrcMenuWork *wk) {
    int i;

    for (i = 0; i < 5; i++) {
        if (wk->actors[i] != NULL) {
            ClActor_Delete(wk->actors[i]);
            wk->actors[i] = NULL;
        }
    }
}

static void IrcMenu_CreateMainButtons(IrcMenuWork *wk) {
    u8 x[] = { 128, 128, 128, 128, 224 };
    u8 y[] = { 96, 96, 96, 96, 177 };
    u8 sequences[] = { 2, 1, 3, 4, 0 };
    ClActorSetup setup;
    int i;

    IrcMenu_DeleteButtonActors(wk);
    for (i = 0; i < 5; i++) {
        setup.x = x[i];
        setup.y = y[i];
        setup.sequence = sequences[i];
        setup.priority = 0;
        setup.bgPriority = 1;
        wk->actors[i] = ClActor_Create(wk->clUnit, wk->clRes[3], wk->clRes[1], wk->clRes[5], &setup, 1, wk->heapId);
        ClActor_SetAutoAnim(wk->actors[i], FALSE);
        ClActor_SetVisible(wk->actors[i], TRUE);
        ClActor_SetObjMode(wk->actors[i], 1);
    }
}

static void IrcMenu_CreatePlayerCountButtons(IrcMenuWork *wk) {
    u8 x[] = { 128, 128, 224 };
    u8 y[] = { 72, 120, 177 };
    u8 sequences[] = { 9, 10, 0 };
    ClActorSetup setup;
    int i;

    IrcMenu_DeleteButtonActors(wk);
    for (i = 0; i < 3; i++) {
        setup.x = x[i];
        setup.y = y[i];
        setup.sequence = sequences[i];
        setup.priority = 0;
        setup.bgPriority = 1;
        wk->actors[i] = ClActor_Create(wk->clUnit, wk->clRes[3], wk->clRes[1], wk->clRes[5], &setup, 1, wk->heapId);
        ClActor_SetAutoAnim(wk->actors[i], FALSE);
        ClActor_SetVisible(wk->actors[i], TRUE);
        ClActor_SetObjMode(wk->actors[i], 1);
    }
}

static void IrcMenu_CreateBattleTypeButtons(IrcMenuWork *wk) {
    u8 x[] = { 128, 128, 128, 128, 224 };
    u8 y[] = { 24, 64, 104, 144, 177 };
    u8 sequences[] = { 5, 6, 7, 8, 0 };
    ClActorSetup setup;
    int i;

    IrcMenu_DeleteButtonActors(wk);
    for (i = 0; i < 5; i++) {
        setup.x = x[i];
        setup.y = y[i];
        setup.sequence = sequences[i];
        setup.priority = 0;
        setup.bgPriority = 1;
        wk->actors[i] = ClActor_Create(wk->clUnit, wk->clRes[3], wk->clRes[1], wk->clRes[5], &setup, 1, wk->heapId);
        ClActor_SetAutoAnim(wk->actors[i], FALSE);
        ClActor_SetVisible(wk->actors[i], TRUE);
        ClActor_SetObjMode(wk->actors[i], 1);
    }
}

static void IrcMenu_CreateBattleBoxButtons(IrcMenuWork *wk) {
    u8 x[] = { 128, 128, 224 };
    u8 y[] = { 72, 120, 177 };
    u8 sequences[] = { 17, 17, 0 };
    ClActorSetup setup;
    int i;

    IrcMenu_DeleteButtonActors(wk);
    for (i = 0; i < 3; i++) {
        setup.x = x[i];
        setup.y = y[i];
        setup.sequence = sequences[i];
        setup.priority = 0;
        setup.bgPriority = 1;
        wk->actors[i] = ClActor_Create(wk->clUnit, wk->clRes[3], wk->clRes[1], wk->clRes[5], &setup, 1, wk->heapId);
        ClActor_SetAutoAnim(wk->actors[i], FALSE);
        ClActor_SetVisible(wk->actors[i], TRUE);
        ClActor_SetObjMode(wk->actors[i], 1);
    }
}

static void IrcMenu_CreateDecoration(IrcMenuWork *wk, u32 a, u32 b, u32 c, u32 d) {
}

static void IrcMenu_FreeActorsAndResources(IrcMenuWork *wk) {
    int i;

    for (i = 0; i < 5; i++) {
        if (wk->actors[i] != NULL) {
            ClActor_Delete(wk->actors[i]);
            wk->actors[i] = NULL;
        }
    }
    for (i = 0; i < 2; i++) {
        if (wk->actors2[i] != NULL) {
            ClActor_Delete(wk->actors2[i]);
            wk->actors2[i] = NULL;
        }
    }
    for (i = 0; i < 2; i++) {
        if (wk->clRes[i] != 0) {
            ClActSys_FreePltt(wk->clRes[i]);
        }
    }
    for (; i < 4; i++) {
        if (wk->clRes[i] != 0) {
            ClActSys_FreeChars(wk->clRes[i]);
        }
    }
    for (; i < 6; i++) {
        if (wk->clRes[i] != 0) {
            ClActSys_FreeCellAnims(wk->clRes[i]);
        }
    }
}

static void IrcMenu_CreateYesNoMenu(IrcMenuWork *wk) {
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
    GFL_MsgDataLoadStrbuf(wk->msgData, Startmenu2_Text_Yes, wk->yesNoItems[0].str);
    wk->yesNoItems[0].color = PRINT_COLOR(14, 15, 3);
    wk->yesNoItems[1].str = GFL_StrBufCreate(100, wk->heapId);
    GFL_MsgDataLoadStrbuf(wk->msgData, Startmenu2_Text_No, wk->yesNoItems[1].str);
    wk->yesNoItems[1].color = PRINT_COLOR(14, 15, 3);
    wk->yesNoItems[0].type = 0;
    wk->yesNoItems[1].type = 0;
    wk->yesNoMenu = AppTaskMenu_Create(&init, wk->menuRes);
    AppTaskMenu_SetLocked(wk->yesNoMenu, TRUE);
    GFL_StrBufFree(wk->yesNoItems[0].str);
    GFL_StrBufFree(wk->yesNoItems[1].str);
}

static BOOL IrcMenu_UpdateMessage(IrcMenuWork *wk) {
    if (wk->printStream != NULL) {
        if (wk->msgWin != NULL) {
            KeyCursor_Update(wk->keyCursor, wk->printStream, wk->msgWin);
        }
        switch (PrintStream_GetState(wk->printStream)) {
        case PRINT_STREAM_DONE:
            PrintStream_Delete(wk->printStream);
            wk->printStream = NULL;
            break;
        case PRINT_STREAM_PAUSED:
            if (GFL_HIDIsTouchPressed()) {
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
                PrintStream_Continue(wk->printStream);
            }
            break;
        case PRINT_STREAM_RUNNING:
            if (GFL_HIDIsTouchHeld()) {
                PrintStream_SkipWait(wk->printStream, 0);
            }
            break;
        }
        return FALSE;
    }
    return TRUE;
}

// Prints strBuf in the message window, a character at a time or at once
static void IrcMenu_PrintMessage(IrcMenuWork *wk, BOOL instant) {
    BmpWin *win;

    if (wk->msgWin == NULL) {
        wk->msgWin = BmpWin_CreateDynamic(5, 1, 3, 30, 4, 11, TRUE);
    }
    win = wk->msgWin;
    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    if (!instant) {
        wk->printStream = PrintStream_Create(win, 0, 0, wk->strBuf, wk->font, TextSpeed_GetCurrentWait(), wk->tcbMgr, 2,
                                             wk->heapId, 15);
    } else {
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), 0, 0, wk->strBuf, wk->font);
        wk->printStream = NULL;
    }
    BmpWin_DrawFrame(win, TRUE, wk->frameChars, 12);
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysQueueScrLoad(5);
}

static void IrcMenu_AskSave(IrcMenuWork *wk) {
    IrcMenu_SetDimmed(wk, TRUE);
    GFL_BGSysFillScrAsync(5, 0);
    GFL_MsgDataLoadStrbuf(wk->msgData, Startmenu2_Text_WouldLikeSaveGame, wk->strBuf);
    IrcMenu_PrintMessage(wk, FALSE);
    IrcMenu_SetStateAtLine(wk, IrcMenu_WaitSaveQuestion, 0x76a);
}

static void IrcMenu_WaitSave(IrcMenuWork *wk) {
    if (GameData_UpdateSave(wk->gameData) == 2) {
        IrcMenu_SetDimmed(wk, FALSE);
        BmpWin_ClearFrame(wk->msgWin, 2);
        BmpWin_ClearScreen(wk->msgWin);
        GFL_BGSysQueueScrLoad(5);
        if (wk->mode == 1) {
            IrcMenu_SetStateAtLine(wk, IrcMenu_ShowPlayerCountMenu, 0x77f);
        } else {
            IrcMenu_SetStateAtLine(wk, IrcMenu_StartExit, 0x782);
        }
        wk->unk2F8 = 1;
    }
}

static void IrcMenu_StartSave(IrcMenuWork *wk) {
    if (IrcMenu_UpdateMessage(wk)) {
        GameData_StartSave(wk->gameData);
        IrcMenu_SetStateAtLine(wk, IrcMenu_WaitSave, 0x797);
    }
}

static void IrcMenu_WaitSaveErrorDismiss(IrcMenuWork *wk) {
    if (IrcMenu_UpdateMessage(wk) && GFL_HIDIsTouchPressed()) {
        IrcMenu_SetDimmed(wk, FALSE);
        GFL_BGSysClearScr(3);
        IrcMenu_FreeMessageWindow(wk);
        wk->mode = 12;
        IrcMenu_SetStateAtLine(wk, IrcMenu_MainUpdate, 0x7ab);
    }
}

static void IrcMenu_HandleSaveAnswer(IrcMenuWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->yesNoMenu)) {
        if (AppTaskMenu_GetCursorPos(wk->yesNoMenu) == 0) {
            if (SaveControl_IsNewGameOverSave(GameData_GetSaveControl(wk->gameData))) {
                GFL_MsgDataLoadStrbuf(wk->msgData, Startmenu2_Text_ThereAlreadySavedGame, wk->strBuf);
                IrcMenu_PrintMessage(wk, FALSE);
                IrcMenu_SetStateAtLine(wk, IrcMenu_WaitSaveErrorDismiss, 0x7c2);
            } else {
                GFL_MsgDataLoadStrbuf(wk->msgData, Startmenu2_Text_SavingDontTurnOff, wk->strBuf);
                IrcMenu_PrintMessage(wk, TRUE);
                IrcMenu_CreateWaitIcon(wk);
                IrcMenu_SetStateAtLine(wk, IrcMenu_StartSave, 0x7c8);
            }
        } else {
            IrcMenu_SetDimmed(wk, FALSE);
            GFL_BGSysClearScr(3);
            IrcMenu_FreeMessageWindow(wk);
            wk->mode = 12;
            IrcMenu_SetStateAtLine(wk, IrcMenu_MainUpdate, 0x7d0);
        }
        AppTaskMenu_Free(wk->yesNoMenu);
        wk->yesNoMenu = NULL;
    }
}

static void IrcMenu_WaitSaveQuestion(IrcMenuWork *wk) {
    if (IrcMenu_UpdateMessage(wk)) {
        IrcMenu_CreateYesNoMenu(wk);
        IrcMenu_SetStateAtLine(wk, IrcMenu_HandleSaveAnswer, 0x7e5);
    }
}

static void IrcMenu_VBlank(TCB *task, void *work) {
    ClActSys_VBlank();
}

static BGSysVRAMConfig sIrcMenuVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static BOOL IrcMenu_Init(GameProc *proc, u32 *state, void *param, void *work) {
    IrcMenuWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_IRC_BATTLE_MENU, 0x18000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(IrcMenuWork), HEAPID_IRC_BATTLE_MENU);
    MI_CpuFill8(wk, 0, sizeof(IrcMenuWork));
    wk->heapId = HEAPID_IRC_BATTLE_MENU;
    GFL_BGSysCreate(wk->heapId);
    BmpWin_InitAllocator(wk->heapId);
    GFL_TextRndInitColorIndexLUT();
    GFL_BGSysSetVRAMBanks(&sIrcMenuVRAMConfig);
    {
        BGSysLCDConfig lcd = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
        GFL_BGSysSetLCDConfig(&lcd);
    }
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    ClActSys_Create(&data_02093f08, &sIrcMenuVRAMConfig, wk->heapId);
    wk->clUnit = ClActUnit_Create(40, 0, wk->heapId);
    wk->vblankTask = GFL_VBlankTCBAdd(IrcMenu_VBlank, wk, 0);
    IrcMenu_CreateBGs(wk);
    IrcMenu_LoadGraphics(wk);
    wk->objPalettes = GFL_HeapAllocate(wk->heapId, 0x200, FALSE, "ircbattlemenu.c", 0x832);
    wk->bgPalettes = GFL_HeapAllocate(wk->heapId, 0x200, FALSE, "ircbattlemenu.c", 0x833);
    wk->tcbMgr = GFL_TCBExMgrCreate(wk->heapId, wk->heapId, 2, 0);
    wk->keyCursor = KeyCursor_Create(15, FALSE, TRUE, wk->heapId);
    wk->printQueue = PrintQueue_CreateDefault(wk->heapId);
    wk->menuRes = AppTaskMenuRes_Create(5, 9, wk->font, wk->printQueue, wk->heapId);
    IrcMenu_LoadButtonResources(wk);
    IrcMenu_CreateDecoration(wk, 0x39, 0x9a, 0, 10);
    IrcMenu_CreateDecoration(wk, 0xc8, 0x26, 1, 11);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
    IrcMenu_SetStateAtLine(wk, IrcMenu_ShowMainMenu, 0x844);
    wk->param = param;
    wk->gameData = GSYS_GetGameData(EventIRC_GetGameSystem(wk->param));
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG0, 15, 4);
    GFL_NetSetIconScreen(FALSE, wk->heapId);
    GFL_BGSysSetEnabledBGsA(0x13);
    GFL_BGSysSetEnabledBGsB(0x17);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_OBJ, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1, 9, 14);
    return TRUE;
}

static BOOL IrcMenu_Main(GameProc *proc, u32 *state, void *param, void *work) {
    IrcMenuWork *wk = work;
    IrcMenuStateFunc func = wk->state;
    BOOL done = TRUE;

    WihDwc_GetStatus(GameData_GetWifiList(wk->gameData));
    if (func != NULL) {
        func(wk);
        done = FALSE;
    }
    if (wk->yesNoMenu != NULL) {
        AppTaskMenu_Update(wk->yesNoMenu);
    }
    if (wk->menuWin != NULL) {
        AppTaskMenuWin_Update(wk->menuWin);
    }
    GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, wk->bgScroll);
    wk->bgScroll--;
    GFL_TCBExMgrUpdate(wk->tcbMgr);
    PrintQueue_Main(wk->printQueue);
    ClActSys_Draw();
    if (!wk->ending) {
        GSYS_TryBootGameComm(EventIRC_GetGameSystem(wk->param));
    }
    if (GFL_WipeIsFinished() && GSYS_IsLowBatteryCGearOff(EventIRC_GetGameSystem(wk->param))) {
        wk->mode = 12;
        done = TRUE;
        Wipe_SetScreenCovered(0, WIPE_COLOR_BLACK);
        Wipe_SetScreenCovered(1, WIPE_COLOR_BLACK);
    }
    return done;
}

static BOOL IrcMenu_End(GameProc *proc, u32 *state, void *param, void *work) {
    EventIRCWork *event = param;
    IrcMenuWork *wk = work;

    if (!GFL_WipeIsFinished()) {
        return FALSE;
    }
    IrcMenu_FreeGraphics(wk);
    EventIRC_SetMode(event, wk->mode);
    event->useBattleBox = wk->choice;
    if (wk->waitIcon != NULL) {
        WaitIcon_Free(wk->waitIcon);
        wk->waitIcon = NULL;
    }
    GFL_HeapFree(wk->objPalettes);
    GFL_HeapFree(wk->bgPalettes);
    IrcMenu_FreeActorsAndResources(wk);
    if (wk->printStream != NULL) {
        PrintStream_Delete(wk->printStream);
    }
    KeyCursor_Free(wk->keyCursor);
    GFL_TCBExMgrFree(wk->tcbMgr);
    GFL_BGSysReleaseBG(7);
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    if (wk->msgWin != NULL) {
        BmpWin_Free(wk->msgWin);
    }
    if (wk->yesNoMenu != NULL) {
        AppTaskMenu_Free(wk->yesNoMenu);
        wk->yesNoMenu = NULL;
    }
    if (wk->menuWin != NULL) {
        AppTaskMenuWin_Free(wk->menuWin);
    }
    AppTaskMenuRes_Free(wk->menuRes);
    GFL_TCBRemove(wk->vblankTask);
    ClActUnit_Delete(wk->clUnit);
    ClActSys_Delete();
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_IRC_BATTLE_MENU);
    return TRUE;
}

const GameProcFunctions IRC_MENU_PROC = {
    IrcMenu_Init,
    IrcMenu_Main,
    IrcMenu_End,
};
