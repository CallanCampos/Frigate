#ifndef POKEBW2_NITRO_CARD_H
#define POKEBW2_NITRO_CARD_H

#include "types.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// NitroSDK's access to the card's backup memory, and the rest of the card library that other files use

// Where a ROM header's program is loaded in RAM, and its size. NitroSDK's multiboot library reads the pair through a
// pointer to it
typedef struct ROMHeaderLoad {
    u32 ramAddr;
    u32 size;
} ROMHeaderLoad;

// A range of the ROM image: where it starts, and how long it is
typedef struct ROMHeaderRegion {
    u32 offset;
    u32 size;
} ROMHeaderRegion;

// The ROM header of the running program, which the card puts at the end of main memory. Only the fields that
// NitroSDK's multiboot library and file system read are named
typedef struct ROMHeader {
    u8 unk_00[0x12];
    u8 unitCode;
    u8 unk_13;
    u8 deviceSize; // the card's capacity: 128 KB shifted left by it
    u8 unk_15[0xb];
    u32 arm9RomOffset;
    u32 arm9Entry;
    ROMHeaderLoad arm9;
    u32 arm7RomOffset;
    u32 arm7Entry;
    ROMHeaderLoad arm7;
    ROMHeaderRegion fnt;         // the file name table
    ROMHeaderRegion fat;         // the file allocation table
    ROMHeaderRegion arm9Overlay; // the table of the ARM9's overlay headers
    ROMHeaderRegion arm7Overlay; // and the ARM7's
    u32 normalCommandControl;    // what the card's control register (MCCNT1) gets for the normal commands
    u8 unk_64[0x1c];
    u32 romSize;
    u8 unk_84[0xe];
    u16 twlRegionStart; // where the part of the ROM that only the DSi mode reads starts, in units of 0x80000 bytes
    u8 unk_94[0xcc];
} ROMHeader;

// The ROM header in main memory (card_common.c), the TWL header that the DSi's system puts there, and the copy of 0x88
// bytes that the card read after the ROM header (card_rom.c)
const ROMHeader *CARD_GetOwnRomHeader(void); // swan's rom_get_header
const void *CARD_GetOwnRomHeaderTWL(void);
const void *CARDi_GetRomAuthCode(void);

typedef enum {
    CARD_RESULT_SUCCESS,
    CARD_RESULT_FAILURE,
    CARD_RESULT_INVALID_PARAM,
    CARD_RESULT_UNSUPPORTED,
    CARD_RESULT_TIMEOUT,
    CARD_RESULT_ERROR,
    CARD_RESULT_NO_RESPONSE,
    CARD_RESULT_CANCELED,
} CARDResult;

// The commands that the ARM7 executes for the ARM9, by the number that CARDi_Request sends it
typedef enum {
    CARD_REQ_INIT = 0,
    CARD_REQ_IDENTIFY = 2,
    CARD_REQ_READ_BACKUP = 6,
    CARD_REQ_WRITE_BACKUP = 7,
    CARD_REQ_PROGRAM_BACKUP = 8,
    CARD_REQ_VERIFY_BACKUP = 9,
    CARD_REQ_ERASE_PAGE_BACKUP = 10,
    CARD_REQ_ERASE_SECTOR_BACKUP = 11,
    CARD_REQ_ERASE_CHIP_BACKUP = 12,
    CARD_REQ_WRITE_STATUS = 14,
    CARD_REQ_ERASE_SUBSECTOR_BACKUP = 15,
} CARDRequest;

// The bit of CARDBackupSpec.caps for a request that the backup memory can execute
#define CARD_CAPS(request) (1 << (request))

// How a backup command moves its data between the caller's memory and the page buffer. RECV reads into the buffer
// and copies out, SEND copies into the buffer and writes it, SEND_VERIFY adds a verify command after the write and
// SPECIAL hands the caller's addresses on, with no buffer
typedef enum {
    CARD_REQUEST_MODE_RECV,
    CARD_REQUEST_MODE_SEND,
    CARD_REQUEST_MODE_SEND_VERIFY,
    CARD_REQUEST_MODE_SPECIAL,
} CARDRequestMode;

#define CARD_RETRY_COUNT_MAX 10

// A backup type: the device, the log2 of its size and the vendor
typedef int CARDBackupType;
#define CARD_BACKUP_TYPE_DEVICE_EEPROM 1
#define CARD_BACKUP_TYPE_DEVICE_FLASH 2
#define CARD_BACKUP_TYPE_DEVICE_FRAM 3
#define CARD_BACKUP_TYPE_DEFINE(device, sizeShift, vendor) ((device) | ((sizeShift) << 8) | ((vendor) << 16))
#define CARD_BACKUP_TYPE_FLASH_4MBITS_EX CARD_BACKUP_TYPE_DEFINE(CARD_BACKUP_TYPE_DEVICE_FLASH, 19, 0xff)

// The commands' work, which the library's files share. NitroSDK's CARDiCommon is the library's one object, with the
// thread that runs the ROM and backup tasks, the lock of the card's resources, and the state of the command that the
// ARM7 is executing; cardi_arg is the command's block, in memory the ARM7 reads
#define CARD_STAT_INIT 0x01
#define CARD_STAT_INIT_CMD 0x02
#define CARD_STAT_BUSY 0x04
#define CARD_STAT_TASK 0x08
#define CARD_STAT_WAITFOR7ACK 0x20
#define CARD_STAT_CANCEL 0x40

// What a game may do with the card: the access level that CARDi_GetAccessLevel gives it, and the part that a command
// needs
#define CARD_ACCESS_LEVEL_BACKUP_READ 1
#define CARD_ACCESS_LEVEL_BACKUP_WRITE 2
#define CARD_ACCESS_LEVEL_ROM 4
#define CARD_ACCESS_LEVEL_FULL 7

// The resource a lock is taken for
#define CARD_TARGET_ROM 1
#define CARD_TARGET_BACKUP 2

// What the backup memory is: its sizes and timings, which CARDi_IdentifyBackupCore fills in from its type. The times
// are in milliseconds
typedef struct CARDBackupSpec {
    u32 totalSize;
    u32 sectSize;
    u32 subsectSize;
    u32 pageSize;
    u32 addrWidth;
    u32 programPage;
    u32 writePage;
    u32 writePageTotal;
    u32 eraseChip;
    u32 eraseChipTotal;
    u32 eraseSector;
    u32 eraseSectorTotal;
    u32 eraseSubsector;
    u32 eraseSubsectorTotal;
    u32 erasePage;
    u8 initialStatus;
    u32 caps;
    u8 padding[4];
} CARDBackupSpec;

// The command block that the ARM7 reads and answers in, 0x60 bytes
typedef struct CARDiCommandArg {
    CARDResult result;
    CARDBackupType type;
    u32 unk_08;
    u32 src;
    u32 dst;
    u32 len;
    CARDBackupSpec spec;
} CARDiCommandArg;

// What a TWL header gives a game to do with the card: the flags of its access control. The two backup flags are named
// for the access level they grant
typedef struct CARDAccessControl {
    u32 : 5;
    u32 gameCardPowerOn : 1;
    u32 : 2;
    u32 gameCardNtrMode : 1;
    u32 : 6;
    u32 backupRead : 1;
    u32 backupWrite : 1;
    u32 : 15;
} CARDAccessControl;

// The header of a DSi game: the load tables of the DS header in its first 0x60 bytes, then where the DSi loads its own
// two images, and the keyed hashes of the ranges that the boot code verifies before it loads them. Only the fields that
// the boot code and TwlSDK's card_hash.c read are named, and each hash's name says which range it verifies
typedef struct CARDTwlRomHeader {
    u8 unk_000[0x1c];
    // 0x1c: the card's security flag; bit 0 says the boot code checks the images against the hashes. The card library
    // reads the same byte as its CARD_TWL_BOOT_FLAGS
    u8 bootFlags;
    u8 unk_01d[0x3];
    u32 arm9RomOffset;
    u32 arm9Entry;
    ROMHeaderLoad arm9;
    u32 arm7RomOffset;
    u32 arm7Entry;
    ROMHeaderLoad arm7;
    u8 unk_040[0x174];
    CARDAccessControl accessControl;
    u8 unk_1b8[0x8];
    u32 arm9iRomOffset;
    u32 arm9iEntry;
    ROMHeaderLoad arm9i;
    u32 arm7iRomOffset;
    u32 arm7iEntry;
    ROMHeaderLoad arm7i;
    // 0x1e0: the parts of the DS and the DSi images that TwlSDK's card library checks as it reads them, the tables of
    // the sectors' hashes and of the blocks' hashes (a block hashes the hashes of a run of sectors), a sector's size,
    // and the sectors of a block
    ROMHeaderRegion digestNtr;
    ROMHeaderRegion digestTwl;
    ROMHeaderRegion digestSectorHashes;
    ROMHeaderRegion digestBlockHashes;
    u32 digestSectorSize;
    u32 digestSectorsPerBlock;
    u8 unk_208[0x10c];
    u8 arm7Hash[0x14];
    u8 digestBlockHashesHash[0x14];
    u8 unk_33c[0x14];
    u8 arm9iHash[0x14];
    u8 arm7iHash[0x14];
    u8 unk_378[0x28];
    u8 arm9Hash[0x14];
} CARDTwlRomHeader;

// A function that the library calls with each event, such as the card being pulled out. Linked through next
typedef struct CARDHookContext {
    struct CARDHookContext *next;
    void *userdata;
    void (*callback)(void *userdata, u32 event, u32 param);
} CARDHookContext;

// How a ROM read moves its data: through a DMA channel or the new DMA of a DSi
typedef struct CARDDmaInterface {
    void (*Recv)(u32 dmaNo, const void *src, void *dst, u32 size);
    void (*Stop)(u32 dmaNo);
} CARDDmaInterface;

struct CARDiCommon;
typedef void (*CARDiTask)(struct CARDiCommon *p);

typedef struct CARDiCommon {
    CARDiCommandArg *cmd;
    vu32 flag;
    u32 priority;
    // Transfers of this size and over discard the whole instruction or data cache
    u32 icInvalidateThreshold;
    u32 dcInvalidateThreshold;
    s32 lockOwner;
    s32 lockRef;
    OSThreadQueue lockQueue;
    u32 lockTarget;
    OSThread thread;
    u8 stack[0x400];
    CARDiTask task;
    MIDmaCallback callback;
    void *callbackArg;
    OSThreadQueue busyQueue;
    // The request that the thread is working on: the addresses, the length left, the DMA channel and its interface
    u32 src;
    u32 dst;
    u32 len;
    u32 dma;
    const CARDDmaInterface *dmaInterface;
    int reqType;
    int reqRetry;
    CARDRequestMode reqMode;
    OSThread *curThread;
} CARDiCommon;

extern CARDiCommon cardi_common;

void CARD_LockBackup(u16 lockId);
void CARD_UnlockBackup(u16 lockId);
CARDResult CARD_GetResultCode(void);
BOOL CARDi_RequestStreamCommand(u32 src, u32 dest, u32 size, MIDmaCallback callback, void *arg, BOOL async,
                                CARDRequest request, int retries, CARDRequestMode mode);
BOOL CARD_IdentifyBackup(CARDBackupType type);
BOOL CARD_WaitBackupAsync(void);
// Whether no access to the backup is in progress
BOOL CARD_TryWaitBackupAsync(void);
void CARD_CancelBackupAsync(void);

// What card_common.c and card_task.c give the other files of the library
void CARD_Init(void); // swan's cart_read_thread_init
void CARD_CheckEnabled(void);
const u8 *CARD_GetRomHeader(void);
void CARD_LockRom(u16 lockId);
void CARD_UnlockRom(u16 lockId);
u32 CARDi_GetAccessLevel(void);
BOOL CARDi_WaitAsync(void);
BOOL CARDi_TryWaitAsync(void);
BOOL CARDi_ExecuteOldTypeTask(CARDiTask task, BOOL async);
void CARDi_OldTypeTaskThread(void *arg); // swan's cart_read_thread_proc
BOOL CARDi_WaitForTask(CARDiCommon *p, BOOL start, MIDmaCallback callback, void *arg);
void CARDi_EndTask(CARDiCommon *p);
const CARDDmaInterface *CARDi_GetDmaInterface(u32 dma);
void CARDi_ICInvalidateSmart(void *addr, u32 len, u32 threshold);
void CARDi_DCInvalidateSmart(void *addr, u32 len, u32 threshold);
// Called with each word the ARM7 sends with the card's tag, and sends it its commands. CARDi_OnFifoRecv and
// CARDi_Request
void CARDi_OnFifoRecv(PXIFifoTag fifoTag, u32 data, BOOL err);
BOOL CARDi_Request(CARDiCommon *p, int reqType, int retry);

// The middleware signature of the backup part of the library, "[SDK+NINTENDO:BACKUP]", which lies in the version
// strings at the start of the program
extern const char CARDi_BackupSignature[];

// What card_rom.c gives card_common.c: the object that holds the ROM's base address, and the setup of the ROM reads
extern u32 cardi_rom_base;
// The ROM reader switches to TwlSDK's secure reader during DSi card initialization.
typedef u32 (*CARDiRomReader)(void *arg, void *dst, u32 src, u32 len);
extern CARDiRomReader sRomReader;
void CARDi_InitRom(void);

// A ROM read by DMA, which the card-data interrupt moves on a page at a time. The callback is called with the argument
// once it is done. TwlSDK's part of the library, in the LTD autoload, makes one of its own
typedef struct CARDiRomDmaRead {
    u32 unk_00;
    void (*callback)(void *arg);
    void *arg;
    u32 src;
    u32 dst;
    u32 len;
    u32 unk_18;
} CARDiRomDmaRead;

// Reads len bytes of the ROM at the offset src into dst, with a DMA channel of the number dma if it can, and calls the
// callback with the argument once done. Waits for the end unless async
void CARDi_ReadRom(u32 dma, const void *src, void *dst, u32 len, MIDmaCallback callback, void *arg, BOOL async);

// The SDK's two forms of it, which most callers use
static inline void CARD_ReadRomAsync(u32 dma, const void *src, void *dst, u32 len, MIDmaCallback callback, void *arg) {
    CARDi_ReadRom(dma, src, dst, len, callback, arg, TRUE);
}

static inline void CARD_ReadRom(u32 dma, const void *src, void *dst, u32 len) {
    CARDi_ReadRom(dma, src, dst, len, NULL, NULL, FALSE);
}

// NitroSDK's CARDi_ReadRomWithCPU and CARDi_ReadRomWithDMA, which TwlSDK's part of the library in the LTD autoload
// calls. The first reads len bytes from src into dst and returns len; its first parameter is for the argument of a
// callback that it has no use for
u32 CARDi_ReadRomWithCPU(void *arg, void *dst, u32 src, u32 len);
void CARDi_ReadRomWithDMA(CARDiRomDmaRead *read);

// Called when the card is pulled out; returns whether to stop the system
typedef BOOL (*CARDPulledOutCallback)(void);

void CARD_SetPulledOutCallback(CARDPulledOutCallback callback);
// The events that CARDi_NotifyEvent gives the hooks: the card was pulled out, or the slot was reset after one was put
// back. Named for what card_pull_out.c sends them on
#define CARD_EVENT_PULLED_OUT 1
#define CARD_EVENT_SLOT_RESET 2
void CARDi_NotifyEvent(u32 command, u32 value);
void CARD_InitPulledOutCallback(void);
BOOL CARD_IsPulledOut(void);
void CARDi_CheckPulledOutCore(u32 cardId);

static inline BOOL CARD_ReadBackup(u32 src, void *dest, u32 size) {
    return CARDi_RequestStreamCommand(src, (u32)dest, size, NULL, NULL, FALSE, CARD_REQ_READ_BACKUP, 1,
                                      CARD_REQUEST_MODE_RECV);
}

static inline BOOL CARD_ReadBackupAsync(u32 src, void *dest, u32 size, MIDmaCallback callback, void *arg) {
    return CARDi_RequestStreamCommand(src, (u32)dest, size, callback, arg, TRUE, CARD_REQ_READ_BACKUP, 1,
                                      CARD_REQUEST_MODE_RECV);
}

static inline BOOL CARD_WriteAndVerifyBackupAsync(u32 dest, const void *src, u32 size, MIDmaCallback callback,
                                                  void *arg) {
    return CARDi_RequestStreamCommand((u32)src, dest, size, callback, arg, TRUE, CARD_REQ_WRITE_BACKUP,
                                      CARD_RETRY_COUNT_MAX, CARD_REQUEST_MODE_SEND_VERIFY);
}

#endif // POKEBW2_NITRO_CARD_H
