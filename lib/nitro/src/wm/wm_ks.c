// The wireless manager's key sharing (libwm, TwlSDK's wm/ARM9/src/wm_ks.c, by its place in the library: the file name
// is a guess): the data sharing with two bytes of keys for every console. The names are descriptive
#include "nitro/wm.h"

// WM_StartKeySharing: shares with all the AIDs, in the double mode
int func_02082408(WMDataSharingInfo *dsi, u16 port) {
    return func_02081bd8(dsi, port, 0xffff, 2, TRUE);
}

// WM_EndKeySharing
int func_0208241c(WMDataSharingInfo *dsi) {
    return func_02081dbc(dsi);
}
