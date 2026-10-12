#ifndef POKEBW2_APP_WIFI_P2PMATCH_H
#define POKEBW2_APP_WIFI_P2PMATCH_H

// The Wi-Fi Club's match board (overlay 173). The ROM has no name for the file: it follows the older games'
// wifi_p2pmatch.c, which is not written yet. This header only has what the overlay's other files need of it; its proc
// table, WIFICLUB_PROC_FUNCTIONS, is in app/wificlub.h

#include "types.h"
#include "struct_decls.h"

// The handlers of the match board's commands (wifi_p2pmatch_net.c has the table), with the work as the match board's
void func_ov173_021a4c64(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov173_021a4ca8(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov173_021a4cac(int netId, int size, const void *data, void *work, NetHandle *handle);
// Where the data of func_ov173_021a4ca8's command is received
void *func_ov173_021a4cc8(int netId, void *work, int size);
void func_ov173_021a4cd8(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov173_021a4d10(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov173_021a4d2c(int netId, int size, const void *data, void *work, NetHandle *handle);

#endif // POKEBW2_APP_WIFI_P2PMATCH_H
