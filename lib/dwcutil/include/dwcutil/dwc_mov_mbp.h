#ifndef POKEBW2_DWCUTIL_DWC_MOV_MBP_H
#define POKEBW2_DWCUTIL_DWC_MOV_MBP_H

#include "types.h"
#include "nitro/mb.h"

// The DWC utility's DS Download Play parent in overlay 182 (dwc_mov_mbp.c), an adapted copy of src/ov181/mbp.c. The
// functions have the SDK's names (see dwc_mov_mbp.c), the types and the constants are ours, and the states are named
// after the ROM's state names

// How many children may connect
#define MOV_MBP_CHILD_MAX 1
// The size of the file buffer that holds the child program's segment
#define MOV_MBP_FILE_BUF_SIZE 0x10000

// The parent's states
enum {
    DWCi_MOV_MBP_STATE_STOP,
    DWCi_MOV_MBP_STATE_IDLE,
    DWCi_MOV_MBP_STATE_ENTRY,
    DWCi_MOV_MBP_STATE_DATASENDING,
    DWCi_MOV_MBP_STATE_REBOOTING,
    DWCi_MOV_MBP_STATE_COMPLETE,
    DWCi_MOV_MBP_STATE_CANCEL,
    DWCi_MOV_MBP_STATE_ERROR,
};

// A child as the parent tracks it
typedef struct {
    MBUserInfo user;
    u8 macAddress[6];
    u16 playerNo;
} MovMbpChildInfo;

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
} MovMbpState;

// The parent's work, which its caller allocates and DWCi_MOV_MBP_MemInit hands over: the state and the children, the
// segment buffer, the library's work with the two pointers the end of the library clears, and then the context of the
// file that follows this one in the overlay, which this file only passes on
typedef struct {
    MovMbpState state;
    MovMbpChildInfo childInfo[MOV_MBP_CHILD_MAX];
    // The segment the child program is read into
    u8 fileBuf[MOV_MBP_FILE_BUF_SIZE];
    // What follows the buffer and precedes the library work
    u8 unk_1002C[0x14];
    u8 mbWork[MB_SYSTEM_BUF_SIZE];
    void *mbWorkp;
    void *fileBufp;
    u8 unk_1B148[0x18];
    // The overlay's next file owns what follows; this file only passes the address on, and 0x13b4 bytes is what that
    // file's init writes to
    u8 loaderContext[0x13B4];
} MovMbpWork;

void DWCi_MOV_MBP_MemInit(MovMbpWork *work);
void DWCi_MOV_MBP_Init(u32 ggid, u32 tgid);
void DWCi_MOV_MBP_Start(const MBGameRegistry *gameInfo, u16 channel);
void DWCi_MOV_MBP_StartDownloadAll(void);
BOOL DWCi_MOV_MBP_IsBootableAll(void);
void DWCi_MOV_MBP_StartRebootAll(void);
void DWCi_MOV_MBP_Cancel(void);
u16 DWCi_MOV_MBP_GetState(void);
// The children's bitmaps DWCi_MOV_MBP_GetChildBmp reads (MOV_MBP_BMP_*), in the order of MovMbpState's
// bitmaps
typedef enum {
    MOV_MBP_BMP_CONNECT,
    MOV_MBP_BMP_REQUEST,
    MOV_MBP_BMP_ENTRY,
    MOV_MBP_BMP_DOWNLOAD,
    MOV_MBP_BMP_BOOTABLE,
    MOV_MBP_BMP_REBOOT,
} MovMbpBmpType;

u16 DWCi_MOV_MBP_GetChildBmp(MovMbpBmpType bmpType);
MovMbpChildInfo *DWCi_MOV_MBP_GetChildInfo(u16 childAid);
u16 DWCi_MOV_MBP_GetPlayerNo(const u8 *macAddress);

// The overlay's next file's (dwc_mov_wh.c, dwcutil/dwc_mov_wh.h), which this file hands the work's tail to: it takes
// that work, and it measures how busy each channel is
void DWCi_MOV_WH_MemInit(void *loaderContext);
BOOL DWCi_MOV_WH_StartMeasureChannel(void);

#endif // POKEBW2_DWCUTIL_DWC_MOV_MBP_H
