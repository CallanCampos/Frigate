#include "types.h"
#include "system/net_define.h"
#include "field/net_devirc.h"
#include "gfl/net.h"
#include "gfl/net_devwifi.h"
#include "gfl/net_devwireless.h"
#include "gfl/overlay.h"
#include "gfl/wm_icon.h"

// What the game gives the network library: the Wi-Fi icon's files, the device of each kind of connection with the
// overlays it needs, the network's step of each frame and the kind of the game commands. The ROM has no string for
// the file, so the name is a guess. getWifiIconNarcIdxNum is swan's name.

// The field's overlays 12 and 27 make room for the Wi-Fi library's 11 and 29, and the GTS's 189
#define OVERLAY_DWC_RAP OVERLAY_ID(11)
#define OVERLAY_NET_WIRELESS OVERLAY_ID(30)
#define OVERLAY_NET_IRC OVERLAY_ID(31)

static u8 sSkipFrame;

u32 getWifiIconNarcIdxNum(void) {
    return 0;
}

void func_020116b0(WmIconFiles *files) {
    files->chars[0] = 2;
    files->chars[1] = 4;
    files->pltt = 1;
}

const GFLNetDevTable *func_020116c0(int type) {
    switch (type) {
    case GFL_NET_TYPE_WIFI_GTS:
        GFL_OvlUnload(OVERLAY_ID(12));
        GFL_OvlUnload(OVERLAY_ID(27));
        GFL_OvlLoad(OVERLAY_DWC_RAP);
        GFL_OvlLoad(OVERLAY_ID(29));
        GFL_OvlLoad(OVERLAY_ID(189));
        return NetDevWifi_GetTable();
    case GFL_NET_TYPE_WIFI:
        GFL_OvlUnload(OVERLAY_ID(12));
        GFL_OvlUnload(OVERLAY_ID(27));
        GFL_OvlLoad(OVERLAY_DWC_RAP);
        GFL_OvlLoad(OVERLAY_ID(29));
        return NetDevWifi_GetTable();
    case GFL_NET_TYPE_WIFI_LOBBY:
        GFL_OvlUnload(OVERLAY_ID(12));
        GFL_OvlUnload(OVERLAY_ID(27));
        GFL_OvlLoad(OVERLAY_DWC_RAP);
        GFL_OvlLoad(OVERLAY_ID(29));
        return NetDevWifi_GetTable();
    case 0:
    case 5:
        GFL_OvlLoad(OVERLAY_NET_WIRELESS);
        return NetDevWireless_GetTable();
    case 3:
    case 4:
        GFL_OvlLoad(OVERLAY_NET_IRC);
        return NetDevIrc_GetTable();
    }
    return NULL;
}

void func_02011778(int type) {
    switch (type) {
    case GFL_NET_TYPE_WIFI_GTS:
        GFL_OvlUnload(OVERLAY_ID(189));
        GFL_OvlUnload(OVERLAY_DWC_RAP);
        GFL_OvlUnload(OVERLAY_ID(29));
        GFL_OvlLoad(OVERLAY_ID(12));
        GFL_OvlLoad(OVERLAY_ID(27));
        break;
    case GFL_NET_TYPE_WIFI:
        GFL_OvlUnload(OVERLAY_DWC_RAP);
        GFL_OvlUnload(OVERLAY_ID(29));
        GFL_OvlLoad(OVERLAY_ID(12));
        GFL_OvlLoad(OVERLAY_ID(27));
        break;
    case GFL_NET_TYPE_WIFI_LOBBY:
        GFL_OvlUnload(OVERLAY_DWC_RAP);
        GFL_OvlUnload(OVERLAY_ID(29));
        GFL_OvlLoad(OVERLAY_ID(12));
        GFL_OvlLoad(OVERLAY_ID(27));
        break;
    case 0:
    case 5:
        GFL_OvlUnload(OVERLAY_NET_WIRELESS);
        break;
    case 3:
    case 4:
        GFL_OvlUnload(OVERLAY_NET_IRC);
        break;
    }
}

void func_02011818(void) {
    if (func_02042d34() == 0x35) {
        if (!sSkipFrame) {
            sSkipFrame = func_020428e0();
        } else {
            sSkipFrame = FALSE;
        }
    } else {
        func_020428e0();
    }
}

BOOL func_02011844(void) {
    switch (func_02042d34()) {
    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
    case 0x10:
    case 0x1c:
    case 0x1f:
    case 0x25:
    case 0x26:
    case 0x30:
        return TRUE;
    }
    return FALSE;
}
