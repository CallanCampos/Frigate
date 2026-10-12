#ifndef POKEBW2_NITRO_MB_PRIVATE_H
#define POKEBW2_NITRO_MB_PRIVATE_H

#include "types.h"
#include <stddef.h>
#include "nitro/fs.h"
#include "nitro/mb.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// What the multiboot library's files share (libmb, the parent's side). The names are descriptive: the library keeps no
// assert strings, so none of its own names survive

#define MB_CHILD_MAX 15
#define MB_FILE_MAX 16
#define MB_SEGMENT_COUNT 3

// The ROM header that the card leaves in main memory, and where the child puts its copy
#define MB_ROM_HEADER_BUF 0x2fffe00
#define MB_CHILD_ROM_HEADER_BUF 0x27ffe00
#define MB_CHILD_ROM_HEADER_BUF_END 0x27fff60
// Where the children put the programs' ARM7 code that was moved, from the end of the ARM9 area on
#define MB_RECV_AREA_START 0x22c0000
// An ARM instruction that returns, which the program's autoload callback is patched to
#define MB_ARM_BX_LR 0xe12fff1e
// A word of the shared memory that changes with time: the beacon period is made of it
#define MB_TIME_SEED (*(vu32 *)0x2fffc3c)

// The kinds of memory a program's segments are loaded to
enum {
    MB_SEGMENT_KIND_ARM9,
    MB_SEGMENT_KIND_ARM7,
    MB_SEGMENT_KIND_HEADER,
};

// The program image that MB_ReadSegment makes: the ROM header, 0x88 bytes that follow it in the ROM, a cache's state
// of 0x70 bytes, a table of 0x10 bytes with the ROM addresses of the ARM9 and ARM7 code, and the code
#define MB_SEGMENT_FIXED_SIZE 0x268
#define MB_SEGMENT_LENGTH_MIN 0x10000

// What a frame carries that the children get from the parent, or send to it
enum {
    MB_FRAME_NONE,
    MB_FRAME_CONNECTED,
    MB_FRAME_KICK,
    MB_FRAME_FILE_INFO,
    MB_FRAME_BLOCK,
    MB_FRAME_BOOT,
    MB_FRAME_MEMBER_FULL,
    MB_FRAME_REQUEST,
    MB_FRAME_DOWNLOAD_REQUEST,
    MB_FRAME_BLOCK_REQUEST,
    MB_FRAME_DOWNLOAD_DONE,
};

// What MBComm_Send* gives when nothing was sent
#define MB_SEND_NOTHING 0x15

// What the parent has to say to a child for the application's answers in MB_CommResponseRequest
enum {
    MB_COMM_RESPONSE_NONE,
    MB_COMM_RESPONSE_DOWNLOAD = 2,
    MB_COMM_RESPONSE_ACCEPT,
    MB_COMM_RESPONSE_KICK,
    MB_COMM_RESPONSE_BOOT,
};

// The numbers of the wireless manager's calls, as WMCallback.apiid gives them
enum {
    MB_WM_API_INITIALIZE = 0,
    MB_WM_API_RESET = 1,
    MB_WM_API_END = 2,
    MB_WM_API_SET_PARENT_PARAM = 7,
    MB_WM_API_START_PARENT = 8,
    MB_WM_API_END_SCAN = 11,
    MB_WM_API_START_CONNECT = 12,
    MB_WM_API_DISCONNECT = 13,
    MB_WM_API_START_MP = 14,
    MB_WM_API_SET_MP_DATA = 15,
    MB_WM_API_PORT = 0x15,
    MB_WM_API_BEACON_INDICATION = 0x19,
    MB_WM_API_SET_LIFETIME = 0x1d,
    MB_WM_API_START_SCAN = 0x26,
    MB_WM_API_INDICATION = 0x80,
};

// What the wireless manager's callbacks pass on to the communication layer's callback (MBComm_OnWmEvent)
enum {
    MB_WM_EVENT_CONNECTED = 0,
    MB_WM_EVENT_DISCONNECTED = 1,
    MB_WM_EVENT_MP_SENT_OK = 2,
    MB_WM_EVENT_MP_RECEIVED = 3,
    MB_WM_EVENT_SCAN_FOUND = 4,
    MB_WM_EVENT_SCAN_NONE = 5,
    MB_WM_EVENT_CONNECT_PARENT = 6,
    MB_WM_EVENT_CHILD_MP_SENT_OK = 8,
    MB_WM_EVENT_PORT_RECV = 9,
    MB_WM_EVENT_CONNECT_LOST = 10,
    MB_WM_EVENT_CONNECT_FAILED = 11,
    MB_WM_EVENT_END = 0x11,
    MB_WM_EVENT_CHILD_MP_SEND_FAILED = 0x12,
    MB_WM_EVENT_MP_SEND_FAILED = 0x13,
    MB_WM_EVENT_INIT_COMPLETE = 0x15,
    MB_WM_EVENT_MP_READY = 0x19,
    MB_WM_EVENT_BEACON_SENT = 0x1c,
    MB_WM_EVENT_IND_0 = 0x1d,
    MB_WM_EVENT_IND_1 = 0x1f,
    MB_WM_EVENT_IND_2 = 0x20,
    MB_WM_EVENT_IND_3 = 0x21,
    MB_WM_EVENT_CHILD_MP_SEND_BUSY = 0x29,
    MB_WM_EVENT_MP_SEND_BUSY = 0x2a,
    MB_WM_EVENT_ERROR = 0xff,
    MB_WM_EVENT_WM_ERROR = 0x100,
};

// The attributes of a beacon: the game offers a multiboot download
#define MB_BEACON_ATTR_MULTIBOOT 2
// A beacon that says there is no game to offer
#define MB_BEACON_ATTR_NO_GAME 8

// A WM_ERRCODE that the wireless manager gives when it is busy
#define WM_ERRCODE_BUSY 4

// The digest areas of the TWL header: the offsets and sizes of the regions of the ROM
typedef struct {
    u8 unk_000[0x1e0];
    u32 ntrRegionOffset;
    u32 ntrRegionSize;
    u32 twlRegionOffset;
    u32 twlRegionSize;
} MBTwlRomHeader;

typedef struct MBCommFile MBCommFile;
typedef struct MBGameInfoRecord MBGameInfoRecord;
typedef struct MBTask MBTask;

// A job for the library's thread
struct MBTask {
    MBTask *next;
    u32 busy : 1;
    u32 priority : 31;
    void (*func)(MBTask *task);
    void (*done)(MBTask *task);
    void *arg0;
    void *arg1;
    u8 unk_18[8];
};

// The size of the memory that the library's thread has: the thread, a queue of jobs and the stack
#define MB_TASK_THREAD_SIZE 0x800

typedef struct {
    OSThread thread;
    // The queue's first job, which the task thread and the threads adding jobs both change
    MBTask *volatile head;
    // The job that ends the thread
    MBTask exitTask;
    u8 stack[MB_TASK_THREAD_SIZE - sizeof(OSThread) - sizeof(MBTask *) - sizeof(MBTask)];
} MBTaskThread;

// The states of the parts of a program that the cache holds
enum {
    MB_CACHE_ENTRY_EMPTY,
    MB_CACHE_ENTRY_LOADING,
    MB_CACHE_ENTRY_READY,
    MB_CACHE_ENTRY_KEPT,
};

// The size of a part of the ARM9 code that MB_ReadSegment reads when the buffer is too small for the whole
#define MB_CACHE_BLOCK_SIZE 0x4400
#define MB_CACHE_ENTRY_COUNT 4

typedef struct {
    u32 offset;
    u32 size;
    void *buf;
    u32 state;
} MBCacheEntry;

// The parts of a program that are in memory, and the archive the rest is read from
typedef struct {
    u32 missCounter;
    u8 unk_04[0xc];
    char archiveName[4];
    u32 nameLength;
    FSArchive *archive;
    u8 unk_1c[0x14];
    MBCacheEntry entries[MB_CACHE_ENTRY_COUNT];
} MBCache;

// Where a segment of a program is loaded in the child
typedef struct {
    u32 recvAddr;
    u32 ramAddr;
    u32 size;
    u32 isArm7 : 1;
    u32 unk_0c : 31;
} MBSegment;

// Where each segment starts in the program's image, and its first block
typedef struct {
    u32 segOffset[MB_SEGMENT_COUNT];
    u16 blockStart[MB_SEGMENT_COUNT];
    u16 blockTotal;
} MBBlockLayout;

// A block of a program: where the child puts it, how long it is, where it is in the image and from which segment
typedef struct {
    u32 recvAddr;
    u32 size;
    u32 bufAddr;
    u8 segment;
} MBBlockSource;

// The parts of the secure area that a ROM image has, and the window of ROM that a read of it covers
typedef struct {
    u32 start;
    u32 length;
} MBSecureAreaPart;

typedef struct {
    u32 start;
    u32 srcBase;
    u32 dstBase;
    u32 length;
} MBSecureWindow;

// The table in the program's image with the ROM addresses of its code
typedef struct {
    u32 segment[MB_SEGMENT_COUNT + 1];
} MBRomOffsets;

// The image that MB_ReadSegment makes
typedef struct {
    ROMHeader header;
    u8 headerExtra[0x88];
    MBCache cache;
    MBRomOffsets romOffsets;
    u8 code[];
} MBSegmentImage;

// What a program offers in the beacon: its picture, name and description, the players and who has seen them
struct MBGameInfoRecord {
    u8 iconPalette[0x20];
    u8 iconChar[0x200];
    MBUserInfo parentUser;
    u8 maxPlayers;
    u8 unk_237;
    u16 gameName[48];
    u16 gameIntro[96];
    u8 playerCount;
    u8 unk_359;
    u16 memberMask;
    u16 changedMask;
    MBUserInfo members[MB_CHILD_MAX];
    u8 params[8];
    u16 sentMask;
    u8 iconError;
    u8 gameNo;
    u8 updateCount;
    u8 slot;
    u8 unk_4b6[2];
    u32 ggid;
    MBGameInfoRecord *next;
};

// The game info of the beacon: a piece of a record, or the players
#define MB_BEACON_FRAGMENT_SIZE 0x62
// The part of the beacon that its checksum covers, from the checksum on
#define MB_BEACON_CHECKED_SIZE 0x68
enum {
    MB_BEACON_KIND_MEMBERS = 2,
};

typedef struct {
    u32 ggid;
    u8 kind : 2;
    u8 slot : 6;
    u8 gameNo;
    u8 updateCount;
    u8 sequence;
    u16 checksum;
    union {
        struct {
            u8 index;
            u8 count;
            u8 length;
            u8 unk_0d;
            u8 data[MB_BEACON_FRAGMENT_SIZE];
        } fragment;
        struct {
            u8 playerCount;
            u8 unk_0b;
            u16 memberMask;
            u16 changedMask;
            MBUserInfo users[4];
            u8 params[8];
        } members;
    } u;
} MBBeacon;

// The steps of the cycle that sends the beacons of all programs
enum {
    MB_BEACON_STATE_PICK_FIRST,
    MB_BEACON_STATE_PICK,
    MB_BEACON_STATE_START_FRAGMENTS,
    MB_BEACON_STATE_FRAGMENTS,
    MB_BEACON_STATE_START_MEMBERS,
    MB_BEACON_STATE_MEMBERS,
};

typedef struct {
    u8 unk_00[0x10];
    u32 mode;
    void (*callback)(u32 ggid);
    MBGameInfoRecord *head;
    MBGameInfoRecord *current;
    const u8 *source;
    u8 state;
    u8 unk_25;
    u8 updateCount;
    u8 fragmentIndex;
    u8 fragmentCount;
    u8 sequence;
    u8 unk_2a[0x1e];
} MBGameInfoSender;

// A frame that the parent sends to the children: a type and what goes with it
typedef struct {
    u8 type;
    u8 unk_01;
    u16 value;
    u16 value2;
} MBFrame;

// A frame that a child sent. A request comes in pieces, with the number of the piece and its data in the place of the
// values
typedef struct {
    u8 type;
    u8 unk_01;
    union {
        struct {
            u16 value;
            u8 data[0x10];
        };
        struct {
            u8 fragment;
            u8 pieceData[0x10];
        };
    };
} MBChildFrame;

// What a child asks the parent for when it connects: the game it wants, who it is and the program's number. The
// request is MB_CHILD_REQUEST_SIZE bytes, the struct's size without its padding
typedef struct {
    u32 ggid;
    MBUserInfo user;
    u16 unk_1a;
    u8 fileNo;
} MBChildRequest;

#define MB_CHILD_REQUEST_SIZE 0x1d

// The pieces of the requests of the children that came so far, and which pieces
typedef struct {
    u8 slot[MB_CHILD_MAX][0x20];
    u32 received[MB_CHILD_MAX];
} MBFrameBuffer;

// The parent's state for each program it offers
struct MBCommFile {
    u32 arm9Entry;
    u32 arm7Entry;
    u8 unk_08[4];
    MBSegment segment[MB_SEGMENT_COUNT];
    u8 romHeaderExtra[0x88];
    u8 userParam[MB_DOWNLOAD_PARAMETER_SIZE];
    MBGameInfoRecord record;
    MBBlockLayout layout;
    const MBGameRegistry *registry;
    const void *buf;
    // The block that is sent next
    u16 sendCursor;
    // The highest block that a child asked for in this round
    u16 requestedBlock;
    // The children that are downloading it, those that are in the game and those that changed
    u16 downloadMask;
    u16 memberMask;
    u16 changedMask;
    u8 valid;
    u8 unk_5cb;
    MBCache *cache;
    const u32 *segmentRomOffsets;
};

// The memory the library gets with MB_Init, from its third part on: what a parent and a child both have. MB_Init
// clears this much of it, and a parent's start clears the rest of MBCommParentWork
typedef struct {
    u8 sendBuf[0x400];
    u8 recvBuf[0x1300 - 0x400];
    // The user of this console
    MBUserInfo user;
    u16 initialized;
    // How many bytes of a program go in a block
    u32 blockSize;
    u32 mpRequested;
    u32 started;
    u8 unk_1324[0x1c];
} MBCommWork;

// The same memory for a parent: the common part, and what the parent knows of its children
typedef struct {
    MBCommWork common;
    MBUserInfo childUser[MB_CHILD_MAX];
    u16 childUnk148a[MB_CHILD_MAX];
    u32 childRequestGgid[MB_CHILD_MAX];
    MBCommPStateCallback callback;
    u32 childState[MB_CHILD_MAX];
    u8 fileCount;
    u8 fileCursor;
    s8 childFile[MB_CHILD_MAX];
    u8 childCount;
    u16 childBitmap;
    MBFrameBuffer fragmentBuf;
    u16 response[MB_CHILD_MAX];
    MBUserInfo childUserCopy;
    MBCommFile files[MB_FILE_MAX];
    u32 multiChild;
    u8 unk_74cc[0x14];
    MBTaskThread thread;
    MBTask task;
} MBCommParentWork;

typedef void (*MBCommEventCallback)(u16 event, void *arg);

// What the wireless manager is set up with
typedef struct {
    u16 lifeMp;
    u16 lifeCam;
    u16 lifeTable;
    u16 lifeFrame;
    u32 powerOn;
    const u16 *name;
    u32 ggid;
    u32 ggidMask;
} MBWmConfig;

// A parent that the scan found
typedef struct {
    u8 desc[0xc0];
    u8 unk_c0[0xa];
    u8 bssid[6];
    u16 channel;
    u16 linkLevel;
    u8 unk_d4[0x22];
    u16 gameInfoLength;
    WMGameInfo gameInfo;
    u8 unk_178[8];
} MBScanEntry;

// The memory the library gets with MB_Init, from its first part on: what the wireless manager is told
typedef struct {
    WMParentParam parentParam;
    u8 unk_38[8];
    u8 sendBuf[0x500 - 0x40];
    u16 parentSendSize;
    u16 childSendSize;
    void *recvBuf;
    WMCallbackFunc wmCallback;
    u8 sending;
    u8 started;
    u8 unk_50e[10];
    u16 sendBufSize;
    u16 recvBufSize;
    MBCommEventCallback commCallback;
    const WMBssDesc *target;
    u16 mode;
    u16 ending;
    u16 mpStarted;
    u16 connectedMask;
    u16 mpFreq;
    u8 unk_52e[2];
    u16 userName[4];
    u16 gameName[16];
    u8 unk_558[0x88];
    u16 scanCount;
    u16 connectedAid;
    u32 scanning;
    u32 scanSwitchChannel;
    u32 lastScanIndex;
    u8 unk_5f0[0x10];
    MBScanEntry scanEntries[16];
} MBWmWork;

// The library's communication state, which MB_Init sets and the end of the library clears. The parent's code reaches
// its part through MB_PARENT_WORK: the original adds a field's offset to the address of an element of the parent's
// arrays, which only a cast of the common pointer gives (see MB_DisconnectChild)
extern MBCommWork *gMBCommWork;
#define MB_PARENT_WORK ((MBCommParentWork *)gMBCommWork)

// Takes a child that left out of the players of the program it chose (MB_DisconnectChild, MBComm_OnWmEvent)
static inline void MBComm_LeaveFile(u16 aid) {
    if (MB_PARENT_WORK->childFile[aid - 1] != -1) {
        u8 n = (u8)MB_PARENT_WORK->childFile[aid - 1];

        MB_PARENT_WORK->files[n].memberMask &= ~(1 << aid);
        MB_PARENT_WORK->files[n].changedMask |= 1 << aid;
        MB_PARENT_WORK->childFile[aid - 1] = -1;
        MB_PARENT_WORK->files[n].downloadMask &= ~(1 << aid);
    }
}

// mb_comm.c
void MBComm_OnWmEvent(u16 event, void *arg);

// mb_segment.c
BOOL MBFile_BuildLayout(MBBlockLayout *layout, const MBCommFile *file);
BOOL MBFile_LocateBlock(MBBlockSource *source, const MBBlockLayout *layout, u16 block, const MBCommFile *file);
int MBComm_FlushAndSend(u32 length, u16 aidBitmap, void *buf);

// mb_gameinfo.c
void MBGameInfo_Build(MBGameInfoRecord *record, const MBGameRegistry *reg, const MBUserInfo *parentUser);
void MBGameInfo_Update(MBGameInfoRecord *record, const MBUserInfo *users, u16 memberMask, u16 changedMask);
void MBGameInfo_Link(MBGameInfoRecord *record);
void MBGameInfo_Init(void);
void MBGameInfo_Send(u32 ggid, u16 tgid, u8 attr);

// mb_wm.c
int MBWm_SendToChildren(const void *data, u32 size, u32 aidBitmap);
u32 MBWm_GetGgid(void);
u16 MBWm_GetTgid(void);
u8 MBWm_GetAttribute(void);
BOOL MBWm_IsStarted(void);

// mb_cache.c
void MBCache_Clear(MBCache *cache);
void MBCache_Add(MBCache *cache, u32 offset, u32 size, void *buf, u32 state);
BOOL MBCache_Read(MBCache *cache, u32 offset, void *dst, u32 size);

// mb_task.c
void MBTask_StartThread(MBTaskThread *thread, u32 size);
BOOL MBTask_IsThreadStarted(void);
void MBTask_Clear(MBTask *task);
BOOL MBTask_IsBusy(const MBTask *task);
void MBTask_Add(MBTask *task, void (*func)(MBTask *task), void (*done)(MBTask *task), u32 priority);
void MBTask_EndThread(void (*done)(MBTask *task));

// mb_frame.c
void MBFrame_SetPayloadSize(int size);
void MBFrame_SetBuffer(MBFrameBuffer *buf);
void MBFrame_ClearChild(u16 aid);
u8 *MBFrame_Encode(const MBFrame *frame, u8 *dst);
const void *MBFrame_Decode(const u8 *src, MBChildFrame *out, u16 aid);

#endif // POKEBW2_NITRO_MB_PRIVATE_H
