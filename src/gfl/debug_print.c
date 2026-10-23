#include "types.h"
#include "gfl/bmp.h"
#include "gfl/graphics.h"
#include "gfl/std.h"
#include "gfl/textprint.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"

static TextPrintParam sDebugPrint;

// Shows a 256x64 bitmap on BG 0 of the main engine, in white on blue
void GFL_DebugPrintCreateSurface(void) {
    OS_DisableIrq();
    GX_SetVisiblePlane(0);
    GX_ResetBankForBG();
    GX_SetBankForBG(GX_VRAM_BG_128_A);
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_2D);
    G2_BlendNone();
    GX_SetMasterBrightness(0);
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GX_SetVisiblePlane(GX_PLANEMASK_BG0);
    GX_SetBGScrOffset(GX_BGSCROFFSET_0x00000);
    GX_SetBGCharOffset(GX_BGCHAROFFSET_0x00000);
    G2_SetBG0Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x0000, GX_BG_CHARBASE_0x10000,
                     GX_BG_EXTPLTT_01);
    G2_SetBG0Priority(0);
    G2_SetBG0Offset(0, 0);
    ((u16 *)HW_BG_PLTT)[1] = 0x7fff;
    ((u16 *)HW_BG_PLTT)[2] = 0x60e1;
    GFL_TextPrintInit(NULL);
    sDebugPrint.dest = GFL_BitmapCreate(32, 8, 32, HEAPID_SYSTEM);
    sDebugPrint.x = 0;
    sDebugPrint.y = 0;
    sDebugPrint.letterSpacing = 1;
    sDebugPrint.lineSpacing = 1;
    sDebugPrint.fgColor = 1;
    sDebugPrint.bgColor = 2;
    sDebugPrint.pad = 0;
    GFL_BitmapFill(sDebugPrint.dest, 2);
}

// Prints a message, and ends the line if it has no newline
void GFL_DebugPrintOutputMessage(const char *message) {
    BOOL newline;

    GFL_TextPrintDrawString(message, &sDebugPrint);
    newline = FALSE;
    while (*message != '\0') {
        if (*message++ == '\n') {
            newline = TRUE;
            break;
        }
    }
    if (!newline) {
        GFL_TextPrintDrawString("\n", &sDebugPrint);
    }
}

// Shows the bitmap and stops the game
void GFL_DebugPrintCommit(void) {
    u16 *screen = G2_GetBG0ScrPtr();
    u32 tile = 0;
    int x;
    int y;
    u8 *pixels;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 32; x++) {
            screen[x] = tile++;
        }
        screen += 32;
    }
    pixels = GFL_BitmapGetPixelData(sDebugPrint.dest);
    DC_FlushRange(pixels, 0x2000);
    GX_LoadBG0Char(pixels, 0, 0x2000);
    OS_Terminate();
}

void GFL_DebugSetVerboseAssertHandlers(void) {
    GFL_DebugSetAssertHandlers(GFL_DebugPrintCreateSurface, GFL_DebugPrintOutputMessage, GFL_DebugPrintCommit);
}
