#include "system/save_error.h"
#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/bmp.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "save/save_control.h"
#include "system/gf_font.h"
#include "system/main.h"
#include "system/printsys.h"

// The screen of a save data error: a window on BG1 of the main screen with the error's message, which stops the game
// until it is reset. The file's name is a guess, from swan's names: the ROM has no string for it

// The messages of the errors, in MSG_FILE_SAVE_ERROR
enum {
    SAVE_ERROR_READ,
    SAVE_ERROR_NO_RESPONSE,
    SAVE_ERROR_WRITE,
};

// The message file of the errors in ARCID_SYSTEM_MESSAGE
#define MSG_FILE_SAVE_ERROR TEXT_BANK_SAVE_ERROR

// The text area: the tiles from FIRST_TEXT_TILE, laid out in rows of the screen from TEXT_TOP, TEXT_LEFT
#define TILE_SIZE 0x20
#define FIRST_TEXT_TILE 0x20
#define TEXT_LEFT 1
#define TEXT_TOP 4
#define TEXT_WIDTH 30
#define TEXT_HEIGHT 16
#define TEXT_BACKGROUND 7

static void playSavegameError(u32 message);
static void displaySavegameError(u32 message);
static void displayLightBlueErrorWindow(void);
static void loadDisplayPositionFont(u32 message);

void showSavegameMainError(void) {
    playSavegameError(SAVE_ERROR_READ);
}

void showSavegameAltError(BOOL noResponse) {
    playSavegameError(noResponse == FALSE ? SAVE_ERROR_WRITE : SAVE_ERROR_NO_RESPONSE);
}

// Shows the error, ends wireless communication and waits for a reset
static void playSavegameError(u32 message) {
    SaveControl_Free();
    displaySavegameError(message);
    if (GFL_NetIsInitialized()) {
        GFL_NetCancelEnd();
        GFL_NetExit(NULL);
        GFL_NetIwFree();
        do {
            GFL_NetMainWhileWaiting();
        } while (GFL_NetIsEnded() == FALSE);
    }
    GCTX_HIDUnblockSoftReset(2);
    while (TRUE) {
        Machine_ProcessExitRequest();
    }
}

static void displaySavegameError(u32 message) {
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    GX_HBlankIntr(FALSE);
    GX_VBlankIntr(FALSE);
    GX_ResetBankForBG();
    GX_ResetBankForSubBG();
    GX_SetBankForBG(GX_VRAM_BG_128_C);
    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_2D);
    GX_SetVisiblePlane(GX_PLANEMASK_BG1);
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    G2_SetBG1Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x0000, GX_BG_CHARBASE_0x04000,
                     GX_BG_EXTPLTT_01);
    G2_BG1Mosaic(FALSE);
    G2_SetBG1Offset(0, 0);
    G2_BlendNone();
    GFL_BGSysSetEnabledBGsB(0);
    GXS_DispOn();
    *(vu16 *)HW_DB_BG_PLTT = GX_RGB(10, 23, 31);
    displayLightBlueErrorWindow();
    loadDisplayPositionFont(message);
    GX_SetMasterBrightness(0);
    GXS_SetMasterBrightness(0);
}

static void displayLightBlueErrorWindow(void) {
    void *file;
    NNSG2dCharacterData *chars;
    NNSG2dScreenData *screen;
    NNSG2dPaletteData *palette;
    NNSG2dPaletteCompressInfo *compressInfo;

    MIi_CpuClear32(0, G2_GetBG1CharPtr(), 0x4000);

    file = GFL_ArcSysReadHeapNewLZ(ARCID_ERROR_WINDOW_GRA, ERROR_WINDOW_GRA_CHAR, FALSE, HEAPID_SAVEDATA);
    if (NNS_G2dGetUnpackedBGCharacterData(file, &chars)) {
        DC_FlushRange(chars->pRawData, chars->szByte);
        MI_CpuCopy16(chars->pRawData, G2_GetBG1CharPtr(), chars->szByte);
    }
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNewLZ(ARCID_ERROR_WINDOW_GRA, ERROR_WINDOW_GRA_SCRN, FALSE, HEAPID_SAVEDATA);
    if (NNS_G2dGetUnpackedScreenData(file, &screen)) {
        DC_FlushRange(screen->rawData, screen->szByte);
        MI_CpuCopy16(screen->rawData, G2_GetBG1ScrPtr(), screen->szByte);
    }
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNewLZ(ARCID_ERROR_WINDOW_GRA, ERROR_WINDOW_GRA_PLTT, FALSE, HEAPID_SAVEDATA);
    NNS_G2dGetUnpackedPaletteCompressInfo(file, &compressInfo);
    if (NNS_G2dGetUnpackedPaletteData(file, &palette)) {
        DC_FlushRange(palette->pRawData, 0x20);
        MI_CpuCopy16(palette->pRawData, (void *)HW_BG_PLTT, 0x20);
    }
    GFL_HeapFree(file);
}

// Lays the text area's tiles out on the screen, and prints the message in them
static void loadDisplayPositionFont(u32 message) {
    u16 *screen;
    int x;
    int y;
    u16 tile;
    GFLBitmap *bitmap;
    Font *font;
    MsgData *msgData;
    StrBuf *str;

    screen = G2_GetBG1ScrPtr();
    tile = FIRST_TEXT_TILE;
    for (y = TEXT_TOP; y < TEXT_TOP + TEXT_HEIGHT; y++) {
        for (x = TEXT_LEFT; x < TEXT_LEFT + TEXT_WIDTH; x++) {
            screen[y * 32 + x] = tile;
            tile++;
        }
    }
    MIi_CpuClear16(TEXT_BACKGROUND * 0x1111, (u8 *)G2_GetBG1CharPtr() + FIRST_TEXT_TILE * TILE_SIZE,
                   TEXT_WIDTH * TEXT_HEIGHT * TILE_SIZE);
    bitmap = GFL_BitmapWrapVRAM((u8 *)G2_GetBG1CharPtr() + FIRST_TEXT_TILE * TILE_SIZE, TEXT_WIDTH, TEXT_HEIGHT,
                                TILE_SIZE, HEAPID_SAVEDATA);
    font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_SAVEDATA);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, MSG_FILE_SAVE_ERROR, HEAPID_SAVEDATA);
    str = GFL_MsgDataLoadStrbufNew(msgData, message);
    GFL_TextRendererDrawToBitmapEx(bitmap, 0, 0, str, font, PRINT_COLOR(4, 11, TEXT_BACKGROUND));
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
    GFL_FontFree(font);
    GFL_BitmapFree(bitmap);
}
