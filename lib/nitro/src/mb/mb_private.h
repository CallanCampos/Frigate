#ifndef POKEBW2_NITRO_MB_PRIVATE_H
#define POKEBW2_NITRO_MB_PRIVATE_H

#include "types.h"
#include "nitro/card.h"
#include "nitro/fs.h"
#include "nitro/mb.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include <stddef.h>

// What the multiboot library's files share (libmb, the parent's side). The library keeps no assert strings, so the ROM
// names none of it; the names are NitroSDK's, as ntrtwl's reconstruction of NitroSDK has them, where it has the code,
// and ours, in the SDK's style, elsewhere

#define MB_MAX_CHILD 15
#define MB_MAX_FILE 16
#define MB_DL_SEGMENT_NUM 3

// The ROM header that the card leaves in main memory, and where the child puts its copy
#define MB_ROM_HEADER_ADDRESS 0x27ffe00
#define MB_CHILD_ROM_HEADER_BUF_END 0x27fff60
// Where the children put the programs' ARM7 code that was moved, from the end of the ARM9 area on
#define MB_ARM7_STATIC_RECV_BUFFER 0x22c0000
// An ARM instruction that returns, which the program's autoload callback is patched to
#define MB_ARM_BX_LR 0xe12fff1e
// A word of the shared memory that changes with time: the beacon period is made of it
#define MB_TIME_SEED (*(vu32 *)0x2fffc3c)

// The kinds of memory a program's segments are loaded to
enum {
    MB_SEG_ARM9STATIC,
    MB_SEG_ARM7STATIC,
    MB_SEG_ROMHEADER,
};

// The program image that MB_ReadSegment makes: the ROM header, 0x88 bytes that follow it in the ROM, a cache's state
// of 0x70 bytes, a table of 0x10 bytes with the ROM addresses of the ARM9 and ARM7 code, and the code
#define MB_SEGMENT_FIXED_SIZE 0x268
#define MB_SEGMENT_BUFFER_MIN 0x10000

// What a frame carries that the children get from the parent, or send to it
enum {
    MB_COMM_TYPE_DUMMY,
    MB_COMM_TYPE_PARENT_SENDSTART,
    MB_COMM_TYPE_PARENT_KICKREQ,
    MB_COMM_TYPE_PARENT_DL_FILEINFO,
    MB_COMM_TYPE_PARENT_DATA,
    MB_COMM_TYPE_PARENT_BOOTREQ,
    MB_COMM_TYPE_PARENT_MEMBER_FULL,
    MB_COMM_TYPE_CHILD_FILEREQ,
    MB_COMM_TYPE_CHILD_ACCEPT_FILEINFO,
    MB_COMM_TYPE_CHILD_CONTINUE,
    MB_COMM_TYPE_CHILD_STOPREQ,
};

// What MBComm_Send* gives when nothing was sent
#define MB_SENDFUNC_STATE_ERR 0x15

// What the parent has to say to a child for the application's answers in MB_CommResponseRequest
enum {
    MB_COMM_USER_REQ_NONE,
    MB_COMM_USER_REQ_SEND_START = 2,
    MB_COMM_USER_REQ_ACCEPT,
    MB_COMM_USER_REQ_KICK,
    MB_COMM_USER_REQ_BOOT,
};

// What the wireless manager's callbacks pass on to the communication layer's callback (MBi_CommParentCallback)
enum {
    MB_CALLBACK_CHILD_CONNECTED = 0,
    MB_CALLBACK_CHILD_DISCONNECTED = 1,
    MB_CALLBACK_MP_PARENT_SENT = 2,
    MB_CALLBACK_MP_PARENT_RECV = 3,
    MB_CALLBACK_PARENT_FOUND = 4,
    MB_CALLBACK_PARENT_NOT_FOUND = 5,
    MB_CALLBACK_CONNECTED_TO_PARENT = 6,
    MB_CALLBACK_MP_CHILD_SENT = 8,
    MB_CALLBACK_MP_CHILD_RECV = 9,
    MB_CALLBACK_DISCONNECTED_FROM_PARENT = 10,
    MB_CALLBACK_CONNECT_FAILED = 11,
    MB_CALLBACK_END_COMPLETE = 0x11,
    MB_CALLBACK_MP_CHILD_SENT_ERR = 0x12,
    MB_CALLBACK_MP_PARENT_SENT_ERR = 0x13,
    MB_CALLBACK_INIT_COMPLETE = 0x15,
    MB_CALLBACK_MP_SEND_ENABLE = 0x19,
    MB_CALLBACK_BEACON_SENT = 0x1c,
    MB_CALLBACK_BEACON_RECV = 0x1d,
    MB_CALLBACK_DISASSOCIATE = 0x1f,
    MB_CALLBACK_REASSOCIATE = 0x20,
    MB_CALLBACK_AUTHENTICATE = 0x21,
    MB_CALLBACK_MP_CHILD_SENT_TIMEOUT = 0x29,
    MB_CALLBACK_SEND_QUEUE_FULL_ERR = 0x2a,
    MB_CALLBACK_API_ERROR = 0xff,
    MB_CALLBACK_ERROR = 0x100,
};

// The attributes of a beacon: the game offers a multiboot download
#define WM_ATTR_FLAG_MB 2
// A beacon that says there is no game to offer
#define WM_ATTR_FLAG_CS 8

// The digest areas of the TWL header: the offsets and sizes of the regions of the ROM
typedef struct {
    u8 unk_000[0x1e0];
    u32 ntrRegionOffset;
    u32 ntrRegionSize;
    u32 twlRegionOffset;
    u32 twlRegionSize;
} MBTwlRomHeader;

typedef struct MBCommFile MBCommFile;
typedef struct MBGameInfo MBGameInfo;
typedef struct MBiTaskInfo MBiTaskInfo;

// A job for the library's thread
struct MBiTaskInfo {
    MBiTaskInfo *next;
    u32 busy : 1;
    u32 priority : 31;
    void (*task)(MBiTaskInfo *task);
    void (*callback)(MBiTaskInfo *task);
    void *arg0;
    void *arg1;
    u8 unk_18[8];
};

// The size of the memory that the library's thread has: the thread, a queue of jobs and the stack
#define MB_TASK_THREAD_SIZE 0x800

typedef struct {
    OSThread th;
    // The queue's first job, which the task thread and the threads adding jobs both change
    MBiTaskInfo *volatile list;
    // The job that ends the thread
    MBiTaskInfo end_task;
    u8 stack[MB_TASK_THREAD_SIZE - sizeof(OSThread) - sizeof(MBiTaskInfo *) - sizeof(MBiTaskInfo)];
} MBiTaskWork;

// The states of the parts of a program that the cache holds
enum {
    MB_CACHE_STATE_EMPTY,
    MB_CACHE_STATE_BUSY,
    MB_CACHE_STATE_READY,
    MB_CACHE_STATE_LOCKED,
};

// The size of a part of the ARM9 code that MB_ReadSegment reads when the buffer is too small for the whole
#define MB_CACHE_PAGE_SIZE 0x4400
#define MB_CACHE_INFO_MAX 4

typedef struct {
    u32 src;
    u32 len;
    void *ptr;
    u32 state;
} MBiCacheInfo;

// The parts of a program that are in memory, and the archive the rest is read from
typedef struct {
    u32 lifetime;
    u8 unk_04[0xc];
    char arc_name[4];
    u32 arc_name_len;
    FSArchive *arc_pointer;
    u8 reserved[0x14];
    MBiCacheInfo list[MB_CACHE_INFO_MAX];
} MBiCacheList;

// Where a segment of a program is loaded in the child
typedef struct {
    u32 recv_addr;
    u32 load_addr;
    u32 size;
    u32 target : 1;
    u32 rsv : 31;
} MbSegmentInfo;

// Where each segment starts in the program's image, and its first block
typedef struct {
    u32 seg_src_offset[MB_DL_SEGMENT_NUM];
    u16 seg_head_blockno[MB_DL_SEGMENT_NUM];
    u16 block_num;
} MB_BlockInfoTable;

// A block of a program: where the child puts it, how long it is, where it is in the image and from which segment
typedef struct {
    u32 child_address;
    u32 size;
    u32 offset;
    u8 segment_no;
} MB_BlockInfo;

typedef struct {
    u32 rom_src;
    u32 mem_src;
    u32 mem_dst;
    u32 len;
} MBiSegmentHeaderInfo;

// The table in the program's image with the ROM addresses of its code
typedef struct {
    u32 segment[MB_DL_SEGMENT_NUM + 1];
} MBRomOffsets;

// The image that MB_ReadSegment makes
typedef struct {
    CARDRomHeader header;
    u8 headerExtra[0x88];
    MBiCacheList cache;
    MBRomOffsets romOffsets;
    u8 code[];
} MBSegmentImage;

// What a program offers in the beacon: its picture, name and description, the players and who has seen them
struct MBGameInfo {
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
    MBUserInfo members[MB_MAX_CHILD];
    u8 params[8];
    u16 broadcastedPlayerFlag;
    u8 dataAttr;
    u8 seqNoFixed;
    u8 seqNoVolat;
    u8 fileNo;
    u8 pad[2];
    u32 ggid;
    MBGameInfo *nextp;
};

// The game info of the beacon: a piece of a record, or the players
#define MB_BEACON_FIXED_DATA_SIZE 0x62
// The part of the beacon that its checksum covers, from the checksum on
#define MB_BEACON_DATA_SIZE 0x68
enum {
    MB_BEACON_DATA_ATTR_VOLAT = 2,
};

typedef struct {
    u32 ggid;
    u8 dataAttr : 2;
    u8 fileNo : 6;
    u8 seqNoFixed;
    u8 seqNoVolat;
    u8 beaconNo;
    u16 checksum;
    union {
        struct {
            u8 index;
            u8 count;
            u8 length;
            u8 unk_0d;
            u8 data[MB_BEACON_FIXED_DATA_SIZE];
        } fragment;
        struct {
            u8 playerCount;
            u8 unk_0b;
            u16 memberMask;
            u16 changedMask;
            MBUserInfo users[4];
            u8 params[8];
        } members;
    } data;
} MbBeacon;

// The steps of the cycle that sends the beacons of all programs
enum {
    MB_BEACON_STATE_STOP,
    MB_BEACON_STATE_READY,
    MB_BEACON_STATE_FIXED_START,
    MB_BEACON_STATE_FIXED,
    MB_BEACON_STATE_VOLAT_START,
    MB_BEACON_STATE_VOLAT,
};

typedef struct {
    u8 unk_00[0x10];
    u32 mode;
    void (*callback)(u32 ggid);
    // The first program, which registering a program changes while the beacon is sent from the list
    MBGameInfo *volatile gameInfoListTop;
    MBGameInfo *nowGameInfop;
    const u8 *srcp;
    u8 state;
    u8 seqNoFixed;
    u8 seqNoVolat;
    u8 flagmentNo;
    u8 flagmentMaxNum;
    u8 beaconNo;
    u8 unk_2a[0x1e];
} MbBeaconSendStatus;

// A frame that the parent sends to the children: a type and what goes with it
typedef struct {
    u8 type;
    u8 pad1;
    u16 fid;
    u16 seqno;
} MBCommParentBlockHeader;

// A frame that a child sent. A request comes in pieces, with the number of the piece and its data in the place of the
// values
typedef struct {
    u8 type;
    u8 pad1;
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
} MBCommChildBlockHeader;

// What a child asks the parent for when it connects: the game it wants, who it is and the program's number. The
// request is MB_COMM_REQ_DATA_SIZE bytes, the struct's size without its padding
typedef struct {
    u32 ggid;
    MBUserInfo userinfo;
    u16 version;
    u8 fileid;
} MBCommRequestData;

#define MB_COMM_REQ_DATA_SIZE 0x1d

// The pieces of the requests of the children that came so far, and which pieces
typedef struct {
    u8 data_buf[MB_MAX_CHILD][0x20];
    u32 data_bmp[MB_MAX_CHILD];
} MbRequestPieceBuf;

// The parent's state for each program it offers
struct MBCommFile {
    u32 arm9Entry;
    u32 arm7Entry;
    u8 unk_08[4];
    MbSegmentInfo segment[MB_DL_SEGMENT_NUM];
    u8 romHeaderExtra[0x88];
    u8 userParam[MB_DOWNLOAD_PARAMETER_SIZE];
    MBGameInfo record;
    MB_BlockInfoTable layout;
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
    MBiCacheList *cache;
    const u32 *segmentRomOffsets;
};

// The memory the library gets with MB_Init, from its third part on: what a parent and a child both have. MB_Init
// clears this much of it, and a parent's start clears the rest of MB_CommPWork
typedef struct {
    u8 sendbuf[0x400];
    u8 recvbuf[0x1300 - 0x400];
    // The user of this console
    MBUserInfo user;
    u16 isMbInitialized;
    // How many bytes of a program go in a block
    u32 block_size_max;
    u32 start_mp_busy;
    u32 is_started_ex;
    u8 padding[0x1c];
} MB_CommCommonWork;

// The same memory for a parent: the common part, and what the parent knows of its children
typedef struct {
    MB_CommCommonWork common;
    MBUserInfo childUser[MB_MAX_CHILD];
    u16 childversion[MB_MAX_CHILD];
    u32 childggid[MB_MAX_CHILD];
    MBCommPStateCallback parent_callback;
    u32 p_comm_state[MB_MAX_CHILD];
    u8 file_num;
    u8 cur_fileid;
    s8 fileid_of_child[MB_MAX_CHILD];
    u8 child_num;
    u16 child_entry_bmp;
    MbRequestPieceBuf req_data_buf;
    u16 req2child[MB_MAX_CHILD];
    MBUserInfo childUserBuf;
    MBCommFile fileinfo[MB_MAX_FILE];
    u32 useWvrFlag;
    u8 padding2[0x14];
    MBiTaskWork task_work;
    MBiTaskInfo cur_task;
} MB_CommPWork;

typedef void (*MBCallbackFunc)(u16 event, void *arg);

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
    u8 reserved1[8];
} ParentInfo;

// The memory the library gets with MB_Init, from its first part on: what the wireless manager is told
typedef struct {
    WMParentParam parentParam;
    u8 unk_38[8];
    u16 sendBuf[(0x500 - 0x40) / sizeof(u16)];
    u16 p_sendlen;
    u16 p_recvlen;
    WMMpRecvBuf *recvBuf;
    WMCallbackFunc callback_ptr;
    u8 mpBusy;
    u8 mbIsStarted;
    u8 reserved0[10];
    u16 sendBufSize;
    u16 recvBufSize;
    MBCallbackFunc callback;
    const WMBssDesc *pInfo;
    u16 mode;
    u16 endReq;
    u16 mpStarted;
    u16 child_bitmap;
    u16 contSend;
    u8 reserved1[2];
    u16 uname[4];
    u16 gameName[16];
    u8 unk_558[0x88];
    u16 found_parent_count;
    u16 my_aid;
    u32 scanning_flag;
    u32 scan_channel_flag;
    u32 last_found_parent_no;
    u8 reserved8[0x10];
    ParentInfo parent_info[16];
} MBiParam;

// The library's communication state, which MB_Init sets and the end of the library clears. The parent's code reaches
// its part through pPwork: the original adds a field's offset to the address of an element of the parent's
// arrays, which only a cast of the common pointer gives (see MB_DisconnectChild)
extern MB_CommCommonWork *mbc;
#define pPwork ((MB_CommPWork *)mbc)

// Takes a child that left out of the players of the program it chose (MB_DisconnectChild, MBi_CommParentCallback)
static inline void MBComm_LeaveFile(u16 aid) {
    if (pPwork->fileid_of_child[aid - 1] != -1) {
        u8 n = (u8)pPwork->fileid_of_child[aid - 1];

        pPwork->fileinfo[n].memberMask &= ~(1 << aid);
        pPwork->fileinfo[n].changedMask |= 1 << aid;
        pPwork->fileid_of_child[aid - 1] = -1;
        pPwork->fileinfo[n].downloadMask &= ~(1 << aid);
    }
}

// mb_comm.c
void MBi_CommParentCallback(u16 event, void *arg);

// mb_segment.c
BOOL MBi_MakeBlockInfoTable(MB_BlockInfoTable *layout, const MBCommFile *file);
BOOL MBi_get_blockinfo(MB_BlockInfo *source, const MB_BlockInfoTable *layout, u16 block, MBCommFile *file);
int MBi_BlockHeaderEnd(u32 length, u16 aidBitmap, void *buf);

// mb_gameinfo.c
void MBi_MakeGameInfo(MBGameInfo *record, const MBGameRegistry *reg, const MBUserInfo *parentUser);
void MB_UpdateGameInfoMember(MBGameInfo *record, const MBUserInfo *users, u16 memberMask, u16 changedMask);
void MB_AddGameInfo(MBGameInfo *record);
void MB_InitSendGameInfoStatus(void);
void MB_SendGameInfoBeacon(u32 ggid, u16 tgid, u8 attr);

// mb_wm.c
int MBi_SendMP(const void *data, u32 size, u32 aidBitmap);
u32 MBi_GetGgid(void);
u16 MBi_GetTgid(void);
u8 MBi_GetAttribute(void);
BOOL MBi_IsStarted(void);

// mb_cache.c
void MBi_InitCache(MBiCacheList *cache);
void MBi_AttachCacheBuffer(MBiCacheList *cache, u32 offset, u32 size, void *buf, u32 state);
BOOL MBi_ReadFromCache(MBiCacheList *cache, u32 offset, void *dst, u32 size);

// mb_task.c
void MBi_InitTaskThread(MBiTaskWork *thread, u32 size);
BOOL MBi_IsTaskAvailable(void);
void MBi_InitTaskInfo(MBiTaskInfo *task);
BOOL MBi_IsTaskBusy(const MBiTaskInfo *task);
void MBi_SetTask(MBiTaskInfo *task, void (*func)(MBiTaskInfo *task), void (*done)(MBiTaskInfo *task), u32 priority);
void MBi_EndTaskThread(void (*done)(MBiTaskInfo *task));

// mb_frame.c
void MBi_SetChildMPMaxSize(int size);
void MBi_SetParentPieceBuffer(MbRequestPieceBuf *buf);
void MBi_ClearParentPieceBuffer(u16 aid);
u8 *MBi_MakeParentSendBuffer(const MBCommParentBlockHeader *frame, u8 *dst);
const void *MBi_SetRecvBufferFromChild(const u8 *src, MBCommChildBlockHeader *out, u16 aid);

#endif // POKEBW2_NITRO_MB_PRIVATE_H
