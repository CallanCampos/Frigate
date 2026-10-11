#ifndef POKEBW2_APP_NET_COMMON_DOWNLOAD_H
#define POKEBW2_APP_NET_COMMON_DOWNLOAD_H

#include "types.h"
#include "dwc/nd.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A download that overlays share (net_common_download.c, overlay 196): a task that receives a file from the Wi-Fi
// download server, or by local wireless beacons. A caller creates it, starts it, requests a command and calls
// NetCommonDownload_Step and NetCommonDownload_Update each frame. The names are ours

typedef struct NetCommonDownload NetCommonDownload;

typedef struct {
    // 0 or 1: which of the commands a download accepts
    u32 mode;
    GameData *gameData;
    // The size of the buffer that the file is received into
    u32 dataSize;
    HeapID heapId;
} NetCommonDownloadParam;

// The commands of NetCommonDownload_Request
enum {
    // Downloads a file of the server: NetCommonDownloadFileListArgs
    NET_COMMON_DOWNLOAD_FILE_LIST,
    NET_COMMON_DOWNLOAD_COMMAND_1,
    // Asks the download to stop
    NET_COMMON_DOWNLOAD_CANCEL,
    // Receives data from beacons: NetCommonDownloadBeaconArgs
    NET_COMMON_DOWNLOAD_BEACON,
    // Asks the beacon receiving to stop
    NET_COMMON_DOWNLOAD_STOP_BEACON,
};

// Which command the download runs
enum {
    NET_COMMON_DOWNLOAD_KIND_IDLE,
    NET_COMMON_DOWNLOAD_KIND_FILE_LIST,
    NET_COMMON_DOWNLOAD_KIND_FILE,
    NET_COMMON_DOWNLOAD_KIND_BEACON,
};

// How the download is going, as NetCommonDownload_GetStatus gives
enum {
    NET_COMMON_DOWNLOAD_STATUS_NONE,
    NET_COMMON_DOWNLOAD_STATUS_DONE,
    NET_COMMON_DOWNLOAD_STATUS_NO_FILE,
    NET_COMMON_DOWNLOAD_STATUS_TIMEOUT,
    NET_COMMON_DOWNLOAD_STATUS_CANCELLED,
    NET_COMMON_DOWNLOAD_STATUS_ERROR,
};

// What NetCommonDownload_Update gives
enum {
    NET_COMMON_DOWNLOAD_UPDATE_RUNNING,
    // The task ended: reset it
    NET_COMMON_DOWNLOAD_UPDATE_ENDED,
    NET_COMMON_DOWNLOAD_UPDATE_ERROR,
};

// Picks which of count files to download once more than one is offered
typedef u32 (*NetCommonDownloadSelectFunc)(DWCNdFileInfo *files, u32 count);

typedef struct {
    // How many files the list holds
    u32 fileCount;
    // The server's attributes that the files must have
    const char *attr1;
    const char *attr2;
    const char *attr3;
    NetCommonDownloadSelectFunc select;
    u32 unk14;
} NetCommonDownloadFileListArgs;

typedef struct {
    // The high byte of the network commands, the region and version mask of the beacons, and the size of the data
    u32 code;
    u32 region;
    u32 mask;
    u32 dataSize;
} NetCommonDownloadBeaconArgs;

NetCommonDownload *NetCommonDownload_Create(const NetCommonDownloadParam *param);
void NetCommonDownload_Free(NetCommonDownload *download);
void NetCommonDownload_Start(NetCommonDownload *download);
// Runs the command once, if the download was started
void NetCommonDownload_Step(NetCommonDownload *download);
// Starts a command, which needs the download started and the command allowed: returns whether it started
BOOL NetCommonDownload_Request(NetCommonDownload *download, u32 command, const void *args);
u32 NetCommonDownload_GetKind(NetCommonDownload *download);
u32 NetCommonDownload_GetStatus(NetCommonDownload *download);
// The buffer that the file goes into
void *NetCommonDownload_GetData(NetCommonDownload *download);
u32 NetCommonDownload_Update(NetCommonDownload *download);
void NetCommonDownload_ResetUpdate(NetCommonDownload *download);

#endif // POKEBW2_APP_NET_COMMON_DOWNLOAD_H
