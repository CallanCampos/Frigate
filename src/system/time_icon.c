#include "system/time_icon.h"
#include "types.h"
#include "constants/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"

// The hourglass that turns in the corner of a window while the game is busy. Our names

// The icon's 8 frames, 16x16 each, one above the other in its bitmap
#define TIME_ICON_FRAMES 8
#define TIME_ICON_SIZE 16
// The bitmap's file
#define TIME_ICON_FILE 0

struct WaitIcon {
    TCB *tcb;
    TCBEx *tcbEx;
    // The 2x2 tile window the icon shows in, at the bottom right of the owner's window
    BmpWin *window;
    GFLBitmap *bitmap;
    HeapID heapId;
    u8 clearColor;
    // The frames left before the next frame shows, and the frames each one shows for
    u8 wait;
    u8 interval;
    u8 frame;
    BOOL active;
};

static void WaitIcon_Init(WaitIcon *icon, BmpWin *window, u8 clearColor, u8 interval, HeapID heapId);
static void WaitIcon_TCBFunc(TCB *tcb, void *data);
static void WaitIcon_TCBExFunc(TCBEx *tcbEx, void *data);

WaitIcon *WaitIcon_Create(TCBManager *tcbManager, BmpWin *window, u8 clearColor, u8 interval, HeapID heapId) {
    WaitIcon *icon = GFL_HeapAllocate(heapId, sizeof(WaitIcon), FALSE, "time_icon.c", 73);

    WaitIcon_Init(icon, window, clearColor, interval, heapId);
    if (tcbManager != NULL) {
        icon->tcb = GFL_TCBMgrAddTask(tcbManager, WaitIcon_TCBFunc, icon, 0);
    } else {
        icon->tcb = NULL;
    }
    icon->tcbEx = NULL;
    icon->active = TRUE;
    return icon;
}

WaitIcon *WaitIcon_CreateTCBEx(TCBExManager *tcbManager, BmpWin *window, u8 clearColor, u8 interval, HeapID heapId) {
    TCBEx *tcbEx = GFL_TCBExMgrAddTask(tcbManager, WaitIcon_TCBExFunc, sizeof(WaitIcon), 0);
    WaitIcon *icon = GFL_TCBExGetData(tcbEx);

    WaitIcon_Init(icon, window, clearColor, interval, heapId);
    icon->tcbEx = tcbEx;
    icon->tcb = NULL;
    icon->active = TRUE;
    return icon;
}

static void WaitIcon_Init(WaitIcon *icon, BmpWin *window, u8 clearColor, u8 interval, HeapID heapId) {
    icon->heapId = heapId;
    icon->wait = 0;
    icon->clearColor = clearColor;
    icon->interval = interval;
    icon->frame = GFL_RandomLC(TIME_ICON_FRAMES);
    icon->bitmap = GFL_G2DIOLoadBitmap(ARCID_TIME_ICON, TIME_ICON_FILE, TRUE, icon->heapId);
    icon->window = BmpWin_CreateDynamic(BmpWin_GetBGIndex(window), BmpWin_GetPosX(window) + BmpWin_GetSizeX(window) - 2,
                                        BmpWin_GetPosY(window) + BmpWin_GetSizeY(window) - 2, 2, 2,
                                        BmpWin_GetPalette(window), TRUE);
    BmpWin_FlushMap(icon->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(icon->window));
}

WaitIcon *WaitIcon_Alloc(HeapID heapId) {
    WaitIcon *icon = GFL_HeapAllocate(heapId, sizeof(WaitIcon), FALSE, "time_icon.c", 166);

    icon->heapId = heapId;
    icon->bitmap = GFL_G2DIOLoadBitmap(ARCID_TIME_ICON, TIME_ICON_FILE, TRUE, icon->heapId);
    icon->window = NULL;
    icon->tcb = NULL;
    icon->tcbEx = NULL;
    return icon;
}

void WaitIcon_Start(WaitIcon *icon, TCBManager *tcbManager, BmpWin *window, u8 clearColor, u8 interval) {
    icon->wait = 0;
    icon->clearColor = clearColor;
    icon->interval = interval;
    icon->frame = GFL_RandomLC(TIME_ICON_FRAMES);
    icon->window = BmpWin_CreateDynamic(BmpWin_GetBGIndex(window), BmpWin_GetPosX(window) + BmpWin_GetSizeX(window) - 2,
                                        BmpWin_GetPosY(window) + BmpWin_GetSizeY(window) - 2, 2, 2,
                                        BmpWin_GetPalette(window), TRUE);
    BmpWin_FlushMap(icon->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(icon->window));
    icon->tcbEx = NULL;
    icon->tcb = GFL_TCBMgrAddTask(tcbManager, WaitIcon_TCBFunc, icon, 0);
    icon->active = TRUE;
}

void WaitIcon_Free(WaitIcon *icon) {
    // Only allocated, never started
    if (icon->tcb == NULL && icon->tcbEx == NULL) {
        GFL_BitmapFree(icon->bitmap);
        GFL_HeapFree(icon);
        return;
    }
    icon->active = FALSE;
    GFL_BitmapFill(BmpWin_GetBitmap(icon->window), icon->clearColor);
    BmpWin_FlushChar(icon->window);
    BmpWin_Free(icon->window);
    GFL_BitmapFree(icon->bitmap);
    if (icon->tcb != NULL) {
        GFL_TCBRemove(icon->tcb);
        GFL_HeapFree(icon);
    } else if (icon->tcbEx != NULL) {
        // The icon is the task's data, freed with it
        GFL_TCBExRequestEnd(icon->tcbEx);
    }
}

static void WaitIcon_TCBFunc(TCB *tcb, void *data) {
    WaitIcon_Main(data);
}

static void WaitIcon_TCBExFunc(TCBEx *tcbEx, void *data) {
    WaitIcon_Main(data);
}

void WaitIcon_Main(WaitIcon *icon) {
    if (icon->active) {
        if (icon->wait == 0) {
            GFL_BitmapFill(BmpWin_GetBitmap(icon->window), icon->clearColor);
            GFL_BitmapCopyArea(icon->bitmap, BmpWin_GetBitmap(icon->window), 0, icon->frame * TIME_ICON_SIZE, 0, 0,
                               TIME_ICON_SIZE, TIME_ICON_SIZE, 0);
            BmpWin_FlushChar(icon->window);
            icon->wait = icon->interval;
            icon->frame++;
            if (icon->frame == TIME_ICON_FRAMES) {
                icon->frame = 0;
            }
        } else {
            icon->wait--;
        }
    }
}
