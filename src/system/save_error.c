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
#define SAVE_ERROR_READ 0
#define SAVE_ERROR_NO_RESPONSE 1
#define SAVE_ERROR_WRITE 2

// The archive of the window's graphics, and its files
#define ARCID_SAVE_ERROR_GRA 22
#define SAVE_ERROR_GRA_PLTT 0
#define SAVE_ERROR_GRA_CHAR 1
#define SAVE_ERROR_GRA_SCRN 2

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
    if (func_02042788()) {
        func_020428a0();
        func_02042860(NULL);
        func_020429f0();
        do {
            func_020428e0();
        } while (func_020427a4() == FALSE);
    }
    GCTX_HIDUnblockSoftReset(2);
    while (TRUE) {
        func_02005430();
    }
}

static void displaySavegameError(u32 message) {
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    gfxSetHBlankIRQEnabled(FALSE);
    gfxSetVBlankIRQEnabled(FALSE);
    gfxAcquireBGBanksA();
    gfxAcquireBGBanksB();
    gfxSetBGBanksA(GX_VRAM_BG_128_C);
    gfxSetEngineModeA(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_2D);
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
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, 0);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, 0);
}

static void displayLightBlueErrorWindow(void) {
    void *file;
    NNSG2dCharacterData *chars;
    NNSG2dScreenData *screen;
    NNSG2dPaletteData *palette;
    NNSG2dPaletteCompressInfo *compressInfo;

    sys_memset32(0, gfxGetCharAddrBG1A(), 0x4000);

    file = GFL_ArcSysReadHeapNewLZ(ARCID_SAVE_ERROR_GRA, SAVE_ERROR_GRA_CHAR, FALSE, HEAPID_SAVEDATA);
    if (NNS_G2dGetUnpackedBGCharacterData(file, &chars)) {
        cp15_flushDC(chars->rawData, chars->size);
        MI_CpuCopy16(chars->rawData, gfxGetCharAddrBG1A(), chars->size);
    }
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNewLZ(ARCID_SAVE_ERROR_GRA, SAVE_ERROR_GRA_SCRN, FALSE, HEAPID_SAVEDATA);
    if (NNS_G2dGetUnpackedScreenData(file, &screen)) {
        cp15_flushDC(screen->rawData, screen->size);
        MI_CpuCopy16(screen->rawData, gfxGetScreenAddrBG1A(), screen->size);
    }
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNewLZ(ARCID_SAVE_ERROR_GRA, SAVE_ERROR_GRA_PLTT, FALSE, HEAPID_SAVEDATA);
    NNS_G2dGetUnpackedPaletteCompressInfo(file, &compressInfo);
    if (NNS_G2dGetUnpackedPaletteData(file, &palette)) {
        cp15_flushDC(palette->rawData, 0x20);
        MI_CpuCopy16(palette->rawData, (void *)HW_BG_PLTT, 0x20);
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

    screen = gfxGetScreenAddrBG1A();
    tile = FIRST_TEXT_TILE;
    for (y = TEXT_TOP; y < TEXT_TOP + TEXT_HEIGHT; y++) {
        for (x = TEXT_LEFT; x < TEXT_LEFT + TEXT_WIDTH; x++) {
            screen[y * 32 + x] = tile;
            tile++;
        }
    }
    sys_memset16(TEXT_BACKGROUND * 0x1111, (u8 *)gfxGetCharAddrBG1A() + FIRST_TEXT_TILE * TILE_SIZE,
                 TEXT_WIDTH * TEXT_HEIGHT * TILE_SIZE);
    bitmap = GFL_BitmapWrapVRAM((u8 *)gfxGetCharAddrBG1A() + FIRST_TEXT_TILE * TILE_SIZE, TEXT_WIDTH, TEXT_HEIGHT,
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
