#include "app/wfc_user_info_warning.h"
#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/bmp.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "system/dsi.h"
#include "system/gf_font.h"
#include "system/main.h"
#include "system/printsys.h"

// The warning that the Nintendo Wi-Fi Connection user information may have been erased: a window on BG1 of the main
// screen with the message, until A or B is pressed. It draws the screen the way save_error.c does, with the same
// graphics. The file's name is a guess, after pokeheartgold's: the ROM has no string for it

// The messages of the warning in MSG_FILE_WIFI_ERROR, for a DS and for a DSi
#define MSG_USER_INFO_ERASED 16
#define MSG_USER_INFO_ERASED_DSI 42

// The archive of the window's graphics (save_error.c's), and its files
#define ARCID_ERROR_WINDOW_GRA 22
#define ERROR_WINDOW_GRA_PLTT 0
#define ERROR_WINDOW_GRA_CHAR 1
#define ERROR_WINDOW_GRA_SCRN 2

// The message file of Nintendo Wi-Fi Connection's errors in ARCID_SYSTEM_MESSAGE
#define MSG_FILE_WIFI_ERROR TEXT_BANK_WIFI_ERROR

// The text area: the tiles from FIRST_TEXT_TILE, laid out in rows of the screen from TEXT_TOP, TEXT_LEFT
#define TILE_SIZE 0x20
#define FIRST_TEXT_TILE 0x20
#define TEXT_LEFT 1
#define TEXT_TOP 4
#define TEXT_WIDTH 30
#define TEXT_HEIGHT 16
#define TEXT_BACKGROUND 7

// The keys that close the warning
#define CLOSE_KEYS (PAD_BUTTON_A | PAD_BUTTON_B)

#define reg_PAD_KEYINPUT (*(vu16 *)0x04000130)
// The buttons and keys that PAD_Read reports, X and Y among them
#define PAD_ALL_MASK 0x2fff

static void WFCUserInfoWarning_Init(u32 message);
static void WFCUserInfoWarning_LoadWindow(void);
static void WFCUserInfoWarning_PrintMessage(u32 message);
static void WFCUserInfoWarning_Exit(void);

// NitroSDK's PAD_Read, as in key.c
static inline u16 PAD_Read(void) {
    return (u16)(((reg_PAD_KEYINPUT | *(vu16 *)HW_BUTTON_XY_BUF) ^ PAD_ALL_MASK) & PAD_ALL_MASK);
}

// The keys held, up cancelling down and left cancelling right, as key.c reads them
static inline u16 ReadKeys(void) {
    u16 keys = PAD_Read();

    keys = keys & ~((keys & PAD_KEY_UP) << 1) & ~((keys & PAD_KEY_LEFT) >> 1);
    return keys;
}

void WFCUserInfoWarning_Show(void) {
    WFCUserInfoWarning_Init(isRunningOnDSi() ? MSG_USER_INFO_ERASED_DSI : MSG_USER_INFO_ERASED);
    // Wait for the keys to be released, then pressed
    while (ReadKeys() & CLOSE_KEYS) {
        func_02005430();
        GCTX_HIDUpdate();
    }
    while (!(ReadKeys() & CLOSE_KEYS)) {
        func_02005430();
        GCTX_HIDUpdate();
    }
    WFCUserInfoWarning_Exit();
}

static void WFCUserInfoWarning_Init(u32 message) {
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
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
    WFCUserInfoWarning_LoadWindow();
    WFCUserInfoWarning_PrintMessage(message);
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, 0);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, 16);
}

static void WFCUserInfoWarning_LoadWindow(void) {
    void *file;
    NNSG2dCharacterData *chars;
    NNSG2dScreenData *screen;
    NNSG2dPaletteData *palette;
    NNSG2dPaletteCompressInfo *compressInfo;

    sys_memset32(0, gfxGetCharAddrBG1A(), 0x4000);

    file = GFL_ArcSysReadHeapNewLZ(ARCID_ERROR_WINDOW_GRA, ERROR_WINDOW_GRA_CHAR, FALSE, HEAPID_USER);
    if (NNS_G2dGetUnpackedBGCharacterData(file, &chars)) {
        cp15_flushDC(chars->rawData, chars->size);
        MI_CpuCopy16(chars->rawData, gfxGetCharAddrBG1A(), chars->size);
    }
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNewLZ(ARCID_ERROR_WINDOW_GRA, ERROR_WINDOW_GRA_SCRN, FALSE, HEAPID_USER);
    if (NNS_G2dGetUnpackedScreenData(file, &screen)) {
        cp15_flushDC(screen->rawData, screen->size);
        MI_CpuCopy16(screen->rawData, gfxGetScreenAddrBG1A(), screen->size);
    }
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNewLZ(ARCID_ERROR_WINDOW_GRA, ERROR_WINDOW_GRA_PLTT, FALSE, HEAPID_USER);
    NNS_G2dGetUnpackedPaletteCompressInfo(file, &compressInfo);
    if (NNS_G2dGetUnpackedPaletteData(file, &palette)) {
        cp15_flushDC(palette->rawData, 0x20);
        MI_CpuCopy16(palette->rawData, (void *)HW_BG_PLTT, 0x20);
    }
    GFL_HeapFree(file);
}

// Lays the text area's tiles out on the screen, and prints the message in them
static void WFCUserInfoWarning_PrintMessage(u32 message) {
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
                                TILE_SIZE, HEAPID_USER);
    font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_USER);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, MSG_FILE_WIFI_ERROR, HEAPID_USER);
    str = GFL_MsgDataLoadStrbufNew(msgData, message);
    GFL_TextRendererDrawToBitmapEx(bitmap, 0, 0, str, font, PRINT_COLOR(4, 11, TEXT_BACKGROUND));
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
    GFL_FontFree(font);
    GFL_BitmapFree(bitmap);
}

// Clears the window and gives the banks back
static void WFCUserInfoWarning_Exit(void) {
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    GFL_BGSysSetEnabledBGsB(0);
    sys_memset16(0, gfxGetCharAddrBG1A(), 0x4000);
    sys_memset16(0, gfxGetScreenAddrBG1A(), 0x800);
    sys_memset16(GX_RGB(31, 31, 31), (void *)HW_BG_PLTT, 0x20);
    GX_SetVisiblePlane(0);
    gfxAcquireBGBanksB();
    gfxAcquireBGBanksA();
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, 0);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, 0);
}
