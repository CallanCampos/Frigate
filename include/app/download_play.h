#ifndef POKEBW2_APP_DOWNLOAD_PLAY_H
#define POKEBW2_APP_DOWNLOAD_PLAY_H

#include "types.h"
#include "nitro/mb.h"

// The DS Download Play parent of overlay 182 (src/ov182/download_play.c), an adapted copy of src/ov181/mbp.c. The
// names are ours

// How many children may connect
#define DOWNLOAD_PLAY_CHILD_MAX 1
// The size of the file buffer that holds the child program's segment
#define DOWNLOAD_PLAY_FILE_BUF_SIZE 0x10000

// The parent's states
enum {
    DOWNLOAD_PLAY_STATE_STOP,
    DOWNLOAD_PLAY_STATE_IDLE,
    DOWNLOAD_PLAY_STATE_ENTRY,
    DOWNLOAD_PLAY_STATE_DATASENDING,
    DOWNLOAD_PLAY_STATE_REBOOTING,
    DOWNLOAD_PLAY_STATE_COMPLETE,
    DOWNLOAD_PLAY_STATE_CANCEL,
    DOWNLOAD_PLAY_STATE_ERROR,
};

// A child as the parent tracks it
typedef struct {
    MBUserInfo user;
    u8 macAddress[6];
    u16 playerNo;
} DownloadPlayChildInfo;

// The parent's state: the states the six bitmaps hold the children of
typedef struct {
    u16 state;
    // The children in each state, by AID
    u16 connectChildBmp;
    u16 requestChildBmp;
    u16 entryChildBmp;
    u16 downloadChildBmp;
    u16 bootableChildBmp;
    u16 rebootChildBmp;
} DownloadPlayState;

// The parent's work, which its caller allocates and DownloadPlay_SetWork hands over: the state and the children, the
// segment buffer, the library's work with the two pointers the end of the library clears, and then the context of the
// file that follows this one in the overlay, which this file only passes on
typedef struct {
    DownloadPlayState state;
    DownloadPlayChildInfo childInfo[DOWNLOAD_PLAY_CHILD_MAX];
    // The segment the child program is read into
    u8 fileBuf[DOWNLOAD_PLAY_FILE_BUF_SIZE];
    // What follows the buffer and precedes the library work
    u8 unk_1002C[0x14];
    u8 mbWork[MB_SYSTEM_BUF_SIZE];
    void *mbWorkp;
    void *fileBufp;
    u8 unk_1B148[0x18];
    // The overlay's next file owns what follows; this file only passes the address on, and 0x13b4 bytes is what that
    // file's init writes to
    u8 loaderContext[0x13B4];
} DownloadPlayWork;

void DownloadPlay_SetWork(DownloadPlayWork *work);
void DownloadPlay_Init(u32 ggid, u32 tgid);
void DownloadPlay_Start(const MBGameRegistry *gameInfo, u16 channel);
void DownloadPlay_StartSending(void);
BOOL DownloadPlay_IsBootableAll(void);
void DownloadPlay_StartRebootAll(void);
void DownloadPlay_Cancel(void);
u16 DownloadPlay_GetState(void);
u16 DownloadPlay_GetChildBmp(int bmpType);
DownloadPlayChildInfo *DownloadPlay_GetChildInfo(u16 childAid);
u16 DownloadPlay_GetPlayerNoByMac(const u8 *macAddress);

// The overlay's next file's (src/ov182/download_play_wih.c), which this file hands the work's tail to: it takes that
// work, and it measures how busy each channel is
void func_ov182_021b9bbc(void *loaderContext);
BOOL func_ov182_021ba1a8(void);

#endif // POKEBW2_APP_DOWNLOAD_PLAY_H
