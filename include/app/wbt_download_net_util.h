#ifndef POKEBW2_APP_WBT_DOWNLOAD_NET_UTIL_H
#define POKEBW2_APP_WBT_DOWNLOAD_NET_UTIL_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// wbt_download_net_util.c, overlay 327: the process that downloads the tournaments from the Wi-Fi server, run by
// wbt_download_sys.c between the login and the logout. The name is the ROM's string, from GFL_HeapAllocate's call.
// Our names

// What wbt_download_sys.c gives the download
typedef struct {
    GameData *gameData;
    // The buffer that the file is received into, 0xe000 bytes
    void *buffer;
    // 0 once a valid file is received, 1 if the network failed (log in again), 2 if no valid file came
    u32 result;
} WbtDownloadNetParam;

extern const GameProcFunctions WBT_DOWNLOAD_NET_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WBT_DOWNLOAD_NET_UTIL_H
