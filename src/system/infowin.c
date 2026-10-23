#include "types.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/infowin.h"

// The information bar along the top of a screen: the clock, the wireless and Wi-Fi icons, the signal strength and the
// battery. Update reads their state and sets a flag for each part that changed, and a VBlank task draws the flagged
// parts into the BG's screen. The names are ours

// The parts to draw, which the VBlank task clears by subtracting them once drawn. 0x2 is set with the rest but
// draws nothing
#define INFOWIN_DRAW_CLOCK 0x1
#define INFOWIN_DRAW_WIRELESS 0x4
#define INFOWIN_DRAW_WIFI 0x8
#define INFOWIN_DRAW_SIGNAL 0x10
#define INFOWIN_DRAW_BATTERY 0x20
#define INFOWIN_DRAW_ALL 0x3f
// The state the icons show
#define INFOWIN_WIRELESS_ON 0x200
#define INFOWIN_SIGNAL_ON 0x400
#define INFOWIN_SIGNAL_BLINK 0x800
#define INFOWIN_BEACON_FOUND 0x1000

// The tiles the bar takes at the end of the BG's characters
#define INFOWIN_CHAR_SIZE 0x1000

typedef struct {
    u32 charBase;
    u16 flags;
    u8 bg;
    u8 palette;
    // The frame of the signal icon's blinking, 5 cycles of 16 frames, the last of them dark
    u8 blinkFrame;
    WifiList *wifiList;
    // Whether the bar shows other players' beacons in the signal icon
    BOOL showBeacons;
    // Whether the colon of the clock is lit, which it is every other second
    BOOL colon;
    BOOL pm;
    u8 minute;
    u8 hour;
    u8 batteryTimer;
    u8 batteryLevel;
    // Which of the Wi-Fi icon's tiles and colors it shows
    int wifiIcon;
    TCB *vblankTask;
} InfoWin;

static void InfoWin_VBlankTask(TCB *tcb, void *data);
static void InfoWin_LoadScreen(u16 tile, u8 palette, u8 x, u8 y, u8 width, u8 height);
static void InfoWin_LoadPalette(u16 color, const u16 *colors, u16 count);
static void InfoWin_LoadGraphics(u8 bg, u8 palette, HeapID heapId);
static void InfoWin_UpdateClock(void);

static const u16 sBatteryColor[] = { 0x7fff, 0x0020 };
static const u16 sWirelessTiles[] = { 0x48, 0x48 };
static const u16 sWirelessColors[][2] = {
    { 0x18c6, 0x2529 },
    { 0x4e73, 0x7fff },
};
static const u16 sSignalColors[][2] = {
    { 0x18c6, 0x2529 },
    { 0x4e73, 0x7fff },
};
static const u16 sWifiTiles[] = { 0x28, 0x28, 0x28, 0x28, 0x28 };
static const u16 sWifiColors[][2] = {
    { 0x18c6, 0x2529 }, { 0x4e73, 0x7fff }, { 0x0850, 0x109f }, { 0x0170, 0x02df }, { 0x4100, 0x7e00 },
};

static InfoWin *sInfoWin;

static inline BOOL InfoWin_IsFlagSet(u16 flag) {
    return (sInfoWin->flags & flag) ? TRUE : FALSE;
}

static inline const u16 *InfoWin_GetSignalColors(u8 on) {
    return sSignalColors[on];
}

void InfoWin_Init(u8 bg, u8 palette, GameData *gameData, HeapID heapId) {
    sInfoWin = GFL_HeapAllocate(heapId, sizeof(InfoWin), FALSE, "infowin.c", 259);
    MI_CpuFill8(sInfoWin, 0, sizeof(InfoWin));
    sInfoWin->bg = bg;
    sInfoWin->palette = palette;
    InfoWin_LoadGraphics(bg, palette, heapId);
    sInfoWin->vblankTask = GFL_VBlankTCBAdd(InfoWin_VBlankTask, sInfoWin, 1);
    sInfoWin->wifiIcon = 0;
    sInfoWin->batteryLevel = GCTX_HIDGetBatteryLevel();
    if (gameData != NULL) {
        sInfoWin->wifiList = GameData_GetWifiList(gameData);
        sInfoWin->showBeacons = GameData_IsCGearOn(gameData);
    } else {
        sInfoWin->wifiList = NULL;
        sInfoWin->showBeacons = FALSE;
    }
    sInfoWin->blinkFrame = 0;
    InfoWin_Update();
    sInfoWin->flags = INFOWIN_DRAW_ALL;
}

void InfoWin_Update(void) {
    BOOL connected = FALSE;
    u8 connectNum = 0;
    GFLNetInitData *netInit = NULL;
    u32 status;
    int beacon;
    u8 batteryLevel;

    if (sInfoWin == NULL) {
        return;
    }
    InfoWin_UpdateClock();
    sInfoWin->batteryTimer++;
    if (sInfoWin->batteryTimer > 60) {
        batteryLevel = GCTX_HIDGetBatteryLevel();
        if (sInfoWin->batteryLevel != batteryLevel) {
            sInfoWin->batteryLevel = batteryLevel;
            sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_BATTERY;
        }
        sInfoWin->batteryTimer = 0;
    }
    if (GFL_NetIsInitialized() == TRUE) {
        netInit = GFL_NetGetInitData();
        connectNum = GFL_NetGetNegotiatedCount();
        if (GFL_NetIsWifi() == TRUE) {
            connected = TRUE;
        }
    }

    if (connectNum < 2 && connected == FALSE) {
        status = WihDwc_GetStatus(sInfoWin->wifiList);
        beacon = 0;
        if (sInfoWin->showBeacons == TRUE && GameBeaconSys_GetNextNew(&beacon) != 30) {
            sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_SIGNAL;
            sInfoWin->flags = sInfoWin->flags | INFOWIN_BEACON_FOUND;
        } else {
            sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_SIGNAL;
            sInfoWin->flags = sInfoWin->flags & (0xffff ^ INFOWIN_BEACON_FOUND);
        }
        if (!(status & 0x2) && InfoWin_IsFlagSet(INFOWIN_WIRELESS_ON) == TRUE) {
            sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_WIRELESS;
            sInfoWin->flags = sInfoWin->flags & (0xffff ^ INFOWIN_WIRELESS_ON);
        }
        if (status & 0x3c) {
            if (status & 0xc) {
                sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_SIGNAL;
                sInfoWin->flags = sInfoWin->flags | INFOWIN_SIGNAL_ON;
                sInfoWin->flags = sInfoWin->flags | INFOWIN_SIGNAL_BLINK;
            } else if (InfoWin_IsFlagSet(INFOWIN_SIGNAL_ON) == FALSE) {
                sInfoWin->blinkFrame = 0;
                sInfoWin->flags = sInfoWin->flags & (0xffff ^ INFOWIN_SIGNAL_BLINK);
                sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_SIGNAL;
                sInfoWin->flags = sInfoWin->flags | INFOWIN_SIGNAL_ON;
            }
        } else if (InfoWin_IsFlagSet(INFOWIN_SIGNAL_ON) == TRUE) {
            sInfoWin->blinkFrame = 0;
            sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_SIGNAL;
            sInfoWin->flags = sInfoWin->flags & (0xffff ^ INFOWIN_SIGNAL_ON);
            sInfoWin->flags = sInfoWin->flags & (0xffff ^ INFOWIN_SIGNAL_BLINK);
        }
        if (status & 0x3c0) {
            if (sInfoWin->wifiIcon == 0) {
                sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_WIFI;
                sInfoWin->wifiIcon = 1;
            }
        } else if (sInfoWin->wifiIcon != 0) {
            sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_WIFI;
            sInfoWin->wifiIcon = 0;
        }
    } else if (netInit != NULL) {
        if (netInit->bNetType == GFL_NET_TYPE_IRC) {
            if (InfoWin_IsFlagSet(INFOWIN_WIRELESS_ON) == FALSE) {
                sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_WIRELESS;
                sInfoWin->flags = sInfoWin->flags | INFOWIN_WIRELESS_ON;
            }
        } else if (InfoWin_IsFlagSet(INFOWIN_WIRELESS_ON) == TRUE) {
            sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_WIRELESS;
            sInfoWin->flags = sInfoWin->flags & (0xffff ^ INFOWIN_WIRELESS_ON);
        }
        if (netInit->bNetType == GFL_NET_TYPE_WIRELESS || netInit->bNetType == GFL_NET_TYPE_IRC_WIRELESS ||
            netInit->bNetType == GFL_NET_TYPE_WIRELESS_LISTEN) {
            if (InfoWin_IsFlagSet(INFOWIN_SIGNAL_ON) == FALSE) {
                sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_SIGNAL;
                sInfoWin->flags = sInfoWin->flags | INFOWIN_SIGNAL_ON;
                sInfoWin->flags = sInfoWin->flags & (0xffff ^ INFOWIN_SIGNAL_BLINK);
                sInfoWin->blinkFrame = 0;
            }
            if (netInit->gameCommandBase == 0x35) {
                sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_SIGNAL;
                sInfoWin->flags = sInfoWin->flags | INFOWIN_SIGNAL_ON;
                sInfoWin->flags = sInfoWin->flags | INFOWIN_SIGNAL_BLINK;
            }
        } else if (InfoWin_IsFlagSet(INFOWIN_SIGNAL_ON) == TRUE) {
            sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_SIGNAL;
            sInfoWin->flags = sInfoWin->flags & (0xffff ^ INFOWIN_SIGNAL_ON);
            sInfoWin->flags = sInfoWin->flags & (0xffff ^ INFOWIN_SIGNAL_BLINK);
            sInfoWin->blinkFrame = 0;
        }
        if (netInit->bNetType == GFL_NET_TYPE_WIFI || netInit->bNetType == GFL_NET_TYPE_WIFI_LOBBY ||
            netInit->bNetType == GFL_NET_TYPE_WIFI_GTS) {
            if (sInfoWin->wifiIcon == 0) {
                sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_WIFI;
                sInfoWin->wifiIcon = 1;
            }
        } else if (sInfoWin->wifiIcon != 0) {
            sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_WIFI;
            sInfoWin->wifiIcon = 0;
        }
    }
}

void InfoWin_Exit(void) {
    if (sInfoWin != NULL) {
        GFL_TCBRemove(sInfoWin->vblankTask);
        GFL_BGSysFreeCharMemory(sInfoWin->bg, sInfoWin->charBase, INFOWIN_CHAR_SIZE);
        GFL_HeapFree(sInfoWin);
        sInfoWin = NULL;
    }
}

static void InfoWin_VBlankTask(TCB *tcb, void *data) {
    BOOL redraw = FALSE;
    BOOL on;
    u8 lit;
    u8 frame, cycle, brightness, level;

    if (sInfoWin == NULL) {
        return;
    }
    if (sInfoWin->flags & INFOWIN_DRAW_CLOCK) {
        u16 clock[10];
        if (sInfoWin->hour < 10) {
            clock[0] = sInfoWin->charBase + ((sInfoWin->palette << 12) + 0xf);
            clock[5] = sInfoWin->charBase + ((sInfoWin->palette << 12) + 0x1f);
        } else {
            clock[0] = sInfoWin->charBase + ((sInfoWin->palette << 12) + (sInfoWin->hour / 10 + 1));
            clock[5] = clock[0] + 0x10;
        }
        clock[1] = sInfoWin->charBase + ((sInfoWin->palette << 12) + (sInfoWin->hour % 10 + 1));
        clock[2] = sInfoWin->charBase + ((sInfoWin->palette << 12) + (sInfoWin->colon ? 0xb : 0xf));
        clock[3] = sInfoWin->charBase + ((sInfoWin->palette << 12) + (sInfoWin->minute / 10 + 1));
        clock[4] = sInfoWin->charBase + ((sInfoWin->palette << 12) + (sInfoWin->minute % 10 + 1));
        clock[6] = clock[1] + 0x10;
        clock[7] = sInfoWin->charBase + ((sInfoWin->palette << 12) + (sInfoWin->colon ? 0x1b : 0x1f));
        clock[8] = clock[3] + 0x10;
        clock[9] = clock[4] + 0x10;
        GFL_BGSysLoadScrArea(sInfoWin->bg, 1, 0, 5, 2, clock, 0, 0, 5, 2);
        redraw = TRUE;
        sInfoWin->flags -= INFOWIN_DRAW_CLOCK;
    }
    if (sInfoWin->flags & INFOWIN_DRAW_WIRELESS) {
        lit = InfoWin_IsFlagSet(INFOWIN_WIRELESS_ON);
        InfoWin_LoadScreen(sWirelessTiles[lit], sInfoWin->palette, 9, 0, 3, 2);
        InfoWin_LoadPalette(6, sWirelessColors[lit], 2);
        redraw = TRUE;
        sInfoWin->flags -= INFOWIN_DRAW_WIRELESS;
    }
    if (sInfoWin->flags & INFOWIN_DRAW_WIFI) {
        InfoWin_LoadScreen(sWifiTiles[sInfoWin->wifiIcon], sInfoWin->palette, 13, 0, 6, 2);
        InfoWin_LoadPalette(8, sWifiColors[sInfoWin->wifiIcon], 2);
        redraw = TRUE;
        sInfoWin->flags -= INFOWIN_DRAW_WIFI;
    }
    if (sInfoWin->flags & INFOWIN_DRAW_SIGNAL) {
        u16 blinkColors[2];
        InfoWin_LoadScreen(0x20, sInfoWin->palette, 19, 0, 7, 2);
        on = FALSE;

        if (sInfoWin->flags & INFOWIN_SIGNAL_BLINK) {
            if (GCTX_HIDGetUpdateRate() == 30) {
                sInfoWin->blinkFrame += 2;
            } else {
                sInfoWin->blinkFrame += 1;
            }
            if (sInfoWin->blinkFrame >= 80) {
                sInfoWin->blinkFrame = 0;
            }
            frame = sInfoWin->blinkFrame % 16;
            cycle = sInfoWin->blinkFrame / 16;
            if (cycle < 4) {
                brightness = 8 - MATH_ABS(frame - 8);
                level = brightness * 13 / 8 + 6;
                blinkColors[0] = GX_RGB(level, level, level);
                level = brightness * 22 / 8 + 9;
                blinkColors[1] = GX_RGB(level, level, level);
                InfoWin_LoadPalette(10, blinkColors, 2);
            } else {
                InfoWin_LoadPalette(10, sSignalColors[0], 2);
            }
        } else {
            if (sInfoWin->flags & (INFOWIN_SIGNAL_ON | INFOWIN_BEACON_FOUND)) {
                on = TRUE;
            }
            InfoWin_LoadPalette(10, InfoWin_GetSignalColors(on), 2);
        }
        redraw = TRUE;
        sInfoWin->flags -= INFOWIN_DRAW_SIGNAL;
    }
    if (sInfoWin->flags & INFOWIN_DRAW_BATTERY) {
        u16 batteryTiles[6] = { 0x40, 0x41, 0x42, 0x43, 0x44, 0x45 };
        u8 dsiBatteryLevels[6] = { 0, 0, 1, 1, 2, 3 };
        u16 battery[4];

        if (OS_IsRunOnTwl()) {
            level = dsiBatteryLevels[sInfoWin->batteryLevel];
        } else if (sInfoWin->batteryLevel == 5) {
            level = 5;
        } else {
            level = 4;
        }
        battery[0] = sInfoWin->charBase + ((sInfoWin->palette << 12) + batteryTiles[level]);
        battery[2] = battery[0] + 0x10;
        battery[1] = battery[0] + 0x400;
        battery[3] = battery[2] + 0x400;
        GFL_BGSysLoadScrArea(sInfoWin->bg, 28, 0, 2, 2, battery, 0, 0, 2, 2);
        InfoWin_LoadPalette(13, sBatteryColor, 1);
        redraw = TRUE;
        sInfoWin->flags -= INFOWIN_DRAW_BATTERY;
    }
    if (redraw == TRUE) {
        GFL_BGSysLoadScr(sInfoWin->bg);
    }
}

static void InfoWin_LoadScreen(u16 tile, u8 palette, u8 x, u8 y, u8 width, u8 height) {
    u16 screen[14];
    u8 i, j;

    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            screen[i + j * width] = sInfoWin->charBase + (tile + (palette << 12) + i + j * 16);
        }
    }
    GFL_BGSysLoadScrArea(sInfoWin->bg, x, y, width, height, screen, 0, 0, width, height);
}

static void InfoWin_LoadPalette(u16 color, const u16 *colors, u16 count) {
    u16 size = count * sizeof(u16);
    u16 offset = (color + sInfoWin->palette * 16) * sizeof(u16);

    GFL_BGSysUploadStdPalette(sInfoWin->bg, colors, size, offset);
}

static void InfoWin_LoadGraphics(u8 bg, u8 palette, HeapID heapId) {
    u32 paletteType = PALTYPE_MAIN_BG;
    ArcTool *arc;

    // BGs 4 to 7 are the sub engine's
    if (bg > 3) {
        paletteType = PALTYPE_SUB_BG;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_INFOWIN, heapId);
    sInfoWin->charBase = GFL_BGSysAllocChar(bg, INFOWIN_CHAR_SIZE, TRUE);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, paletteType, palette * 32, 32, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 1, bg, sInfoWin->charBase, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysFillScrArea(bg, sInfoWin->charBase + 0xf, 0, 0, 32, 1, palette);
    GFL_BGSysFillScrArea(bg, sInfoWin->charBase + 0x1f, 0, 1, 32, 1, palette);
    GFL_BGSysLoadScr(bg);
}

static void InfoWin_UpdateClock(void) {
    RTCTime time;
    BOOL colon;

    RTC_GetCachedTime(&time);
    colon = time.second & 1;
    if (time.hour != sInfoWin->hour) {
        sInfoWin->hour = time.hour;
        if (sInfoWin->hour >= 12) {
            sInfoWin->pm = TRUE;
        } else {
            sInfoWin->pm = FALSE;
        }
        sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_CLOCK;
    }
    if (time.minute != sInfoWin->minute) {
        sInfoWin->minute = time.minute;
        sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_CLOCK;
    }
    if (colon != sInfoWin->colon) {
        sInfoWin->colon = colon;
        sInfoWin->flags = sInfoWin->flags | INFOWIN_DRAW_CLOCK;
    }
}
