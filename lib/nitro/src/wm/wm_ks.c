// The wireless manager's key sharing (libwm, TwlSDK's wm/ARM9/src/wm_ks.c, by its place in the library: the file name
// is a guess): the data sharing with two bytes of keys for every console. The names are descriptive
#include "nitro/wm.h"

// Shares with all the AIDs, in the double mode
int WM_StartKeySharing(WMDataSharingInfo *dsi, u16 port) {
    return WM_StartDataSharing(dsi, port, 0xffff, 2, TRUE);
}

int WM_EndKeySharing(WMDataSharingInfo *dsi) {
    return WM_EndDataSharing(dsi);
}
