#ifndef POKEBW2_NITRO_OS_H
#define POKEBW2_NITRO_OS_H

#include "types.h"
#include "nitro/cp.h"
#include "nitro/hw.h"
#include "nitro/std.h"
#include <stdarg.h>

// The tick count of the system timer (NitroSDK's OS_GetTick), which swan names clock, and its low 16 bits
// (OS_GetTickLo)
u64 clock(void);
u16 func_0207bb84(void);
// Whether the system timer was started (NitroSDK's OS_IsTickAvailable), and starting it (OS_InitTick)
BOOL clock_is_inited(void);
void clock_init(void);
// NitroSDK's OSi_TickCounter, the number of times that the system timer has wrapped, which os_entropy.c reads
extern volatile u64 g_SystemClockBaseCounter;
// NitroSDK's OSi_SetTimerReserved: marks a hardware timer as taken by the OS
void func_0207ba64(int timerNo);
// The system clock, and NitroSDK's OS_TicksToSeconds: the system timer ticks every 64 cycles of it
#define OS_SYSTEM_CLOCK 33514000
#define OS_TicksToSeconds(tick) (((tick) * 64) / OS_SYSTEM_CLOCK)
#define OS_TicksToMilliSeconds(tick) (((tick) * 64) / (OS_SYSTEM_CLOCK / 1000))
#define OS_TicksToMicroSeconds(tick) (((tick) * 64 * 1000) / (OS_SYSTEM_CLOCK / 1000))
#define OS_MilliSecondsToTicks(msec) ((u64)(((OS_SYSTEM_CLOCK / 1000) * (u64)(msec)) / 64))
// Does nothing but be called with a library's signature ("[SDK+...]"), keeping the signature in the link
void OSi_ReferSymbol(const char *signature);
// Restarts the game, which swan names sys_reset (NitroSDK's OS_ResetSystem). The parameter is left in the shared memory
// for the next program. A program that runs from the NAND of a DSi terminates instead
void sys_reset(u32 parameter);
// Stops the game after a fatal error, calling the registered handler first (NitroSDK's OS_Terminate)
void sys_exit(void);
// NitroSDK's OS_InitReset
void func_0207c248(void);
// The loop that sys_exit ends in, which disables the interrupts and halts the CPU for good. The name is swan's
void abort(void);

// The boot type that the card's boot code leaves in the shared memory (NitroSDK's OS_GetBootType): 1 when the game
// booted from the ROM of a cartridge, 2 when it was downloaded as a DS Download Play child
#define HW_BOOT_CHECK_INFO_BUF 0x02fffc40
#define OS_BOOTTYPE_ROM 1
#define OS_BOOTTYPE_DOWNLOAD_MB 2
// The word that OS_ResetSystem leaves for the next program
#define HW_RESET_PARAMETER_BUF 0x02fffc20
// The commands that the ARM7 and the ARM9 send each other with the OS's tag of the PXI FIFO (PXI_FIFO_TAG_OS), in the
// bits 8 to 14
#define OS_PXI_COMMAND_MASK 0x7f00
#define OS_PXI_COMMAND_SHIFT 8
#define OS_PXI_COMMAND_RESET 0x10
#define OS_PXI_COMMAND_TERMINATE 0x20
// The ITCM function that OS_ResetSystem ends in (NitroSDK's OSi_DoResetSystem)
void func_01ff8fac(void);
void WaitByLoop(u32 cycles);
// Stops the CPU until an interrupt, which swan names cp15_halt. NitroSDK's OS_Halt
void cp15_halt(void);
// Writes a range of the data cache back to memory, before DMA reads it. NitroSDK's DC_FlushRange
void cp15_flushDC(const void *addr, u32 size);
// Discards a range of the data cache, after DMA writes it. NitroSDK's DC_InvalidateRange
void cp15_invalidateDC(void *addr, u32 size);
// Writes a range of the data cache back to memory, keeping it cached. NitroSDK's DC_StoreRange
void cp15_cleanDC(const void *addr, u32 size);
// Waits until the write buffer is empty
void cp15_drainWriteBuffer(void);
// Writes the whole data cache back to memory and discards it. NitroSDK's DC_FlushAll
void cp15_invalidateDCAll(void);
// Discards a range of the instruction cache, or all of it. NitroSDK's IC_InvalidateRange and IC_InvalidateAll
void cp15_invalidateIC(void *addr, u32 size);
void cp15_invalidateICAll(void);
// NitroSDK's OS_EnableProtectionUnit and OS_DisableProtectionUnit, under swan's names: switch the memory protection
// unit
void cp15_mmu_enable(void);
void cp15_mmu_disable(void);
// NitroSDK's OS_GetDTCMAddress: where the data TCM is mapped
u32 func_0207b79c(void);
// NitroSDK's OS_SetProtectionRegion, under swan's name (os_protectionRegion.c): sets the address and size of a region
// of the memory protection unit, and enables it. The size is the size field of the region's register in place (bits 1
// to 5, n for 2^(n + 1) bytes): 4 KB is n = 11 and 4 MB is n = 21
#define OS_PROTECTION_SIZE_4KB (11 << 1)
#define OS_PROTECTION_SIZE_4MB (21 << 1)
void SetMemoryProtectionRegion(int regionNo, u32 address, u32 size);

// The start of DTCM, where the linker places the DTCM module
extern u32 SDK_AUTOLOAD_DTCM_START[];
#define HW_DTCM ((u32)SDK_AUTOLOAD_DTCM_START)
// The interrupt flags that OS_WaitIrq checks, at the end of DTCM
#define HW_INTR_CHECK_BUF (HW_DTCM + 0x3ff8)
// The IRQ stack in DTCM: its top, and its end where it would overflow
#define HW_IRQ_STACK (HW_DTCM + 0x3f80)
// The stack sizes are absolute symbols of the linker script, read from the literal pool: the IRQ stack's, and the
// system stack's (SDK_SYS_STACKSIZE: its size when positive; when negative, the stack takes the DTCM that is left
// over after that many bytes at the start of it)
extern u8 SDK_IRQ_STACKSIZE[];
extern u8 SDK_SYS_STACKSIZE[];
#define HW_IRQ_STACK_SIZE ((u32)SDK_IRQ_STACKSIZE)
#define HW_IRQ_STACK_END (HW_IRQ_STACK - HW_IRQ_STACK_SIZE)
#define HW_SYS_STACK_SIZE ((int)SDK_SYS_STACKSIZE)
// The end of the part of DTCM that the system keeps for itself (the table of interrupt handlers, the queue of threads
// that wait for one and the callbacks' words), where the data of the game would start
#define HW_DTCM_SYSRV_END 0x02fe00c0
// Where each processor's thread info (NitroSDK's OSi_ThreadInfo) is announced in the shared main memory
#define HW_THREADINFO_MAIN 0x02ffffa0

// NitroSDK's OSSystemWork, the system area at the end of main memory that the two processors share. Only the words
// that the code reads are laid out
typedef struct OSSystemWork {
    u8 unk_000[0x3c];
    u32 vblankCount;
    u8 unk_040[0x40];
    u8 nvramUserInfo[0x80];
    u8 unk_100[0xe8];
    u32 realTimeClock[2];
    u8 unk_1f0[0x198];
    u32 pxiSignalParam[2]; // one for each processor, with a bit for each PXI tag that it has a callback for
    u32 micLastAddress;
    u16 micSampling;
    u16 unk_396;
    u16 unk_398;
    u8 unk_39a[0x10];
    u16 touchPanelX;
    u16 touchPanelY;
} OSSystemWork;

#define HW_MAIN_MEM_SHARED_WORK 0x02fffc00
#define OS_GetSystemWork() ((OSSystemWork *)HW_MAIN_MEM_SHARED_WORK)

#define OS_IE_V_BLANK 0x1
#define OS_IE_H_BLANK 0x2
#define OS_IE_V_COUNT 0x4
#define OS_IE_TIMER0 0x8
#define OS_IE_TIMER1 0x10
// The timers' bits start here, and the DMA channels' and the DSi's new DMA channels', each a bit after the last
#define OS_IE_TIMER0_SHIFT 3
#define OS_IE_DMA0_SHIFT 8
#define OS_IE_NDMA0_SHIFT 28
// The FIFO to the other processor has a word to receive
#define OS_IE_FIFO_RECV 0x40000
#define OS_IE_CARD_DATA 0x80000
// The geometry engine's command FIFO
#define OS_IE_GXFIFO 0x200000
// The DSi's DSP and camera
#define OS_IE_DSP 0x01000000
#define OS_IE_CAMERA 0x02000000
// The DSi's new DMA channel 1
#define OS_IE_NDMA1 0x20000000

static inline u32 OS_GetVBlankCount(void) {
    return *(vu32 *)HW_VBLANK_COUNT_BUF;
}

// Waits for one of the interrupts, after clearing their flags if clear is set, by halting the CPU until it happens.
// NitroSDK's OS_WaitInterrupt, under swan's name (OS_WaitIrq, which sleeps the thread instead, is irq_waitFor)
void CPU_WaitIntrBit(BOOL clear, u32 interrupts);

static inline void OS_SetIrqCheckFlag(u32 interrupts) {
    *(vu32 *)HW_INTR_CHECK_BUF |= interrupts;
}

// Memory arenas and OS heaps, under swan's names: OS_GetArenaLo, OS_GetArenaHi, OS_SetArenaLo, OS_SetArenaHi,
// OS_AllocFromArenaLo, OS_InitAlloc, OS_CreateHeap, OS_AllocFromHeap and OS_FreeToHeap
//
// The arenas are numbered as in the table of their limits in the shared memory, which has room for nine. OS_InitArena
// sets up six of them: the main memory, the extended main memory (OS_InitArenaEx), the ITCM and DTCM, the shared
// memory and the shared WRAM
#define OS_ARENA_MAIN 0
#define OS_ARENA_MAINEX 2
#define OS_ARENA_ITCM 3
#define OS_ARENA_DTCM 4
#define OS_ARENA_SHARED 5
#define OS_ARENA_WRAM_MAIN 6
#define OS_ARENA_MAX 9
#define OS_HEAP_INVALID (-1)

// Where the main arena begins on a DS: the SDK's SDK_MAIN_ARENA_LO, the end of the last overlay in main memory, which
// is the end of overlay 290 here. The linker script has no symbol of its own for it
extern u8 OV290_BSS_END[];
// Where it begins on a DSi, the end of the LTD module (the SDK's SDK_LTDAUTOLOAD_LTDMAIN_BSS_END), and the ITCM
// arena (SDK_AUTOLOAD_ITCM_END): the linker script's symbols for the ends of those. The LTD module, the DSi mode's part
// of the program, starts at LTD_AUTOLOAD_0_TEXT_START (func_02700000 for the symbol table)
extern u8 LTD_AUTOLOAD_0_TEXT_START[];
extern u8 LTD_AUTOLOAD_0_BSS_END[];
extern u8 ITCM_BSS_END[];
// The start of the extended main memory arena on a DSi, which is an absolute symbol of the SDK's linker script
extern u8 SDK_MAINEX_ARENA_LO[];

void *GetUserMemRegionStart(int arena);
void *GetUserMemRegionEnd(int arena);
void SetUserMemRegionStart(int arena, void *start);
void SetUserMemRegionEnd(int arena, void *end);
void *mem_alloc_direct(int arena, u32 size, u32 alignment);
void *mem_init_alloc_area(int arena, void *start, void *end, int maxHeaps);
int mem_bind_range(int arena, void *start, void *end);
void *malloc_device(int arena, int heap, s32 size);
void free_device(int arena, int heap, void *ptr);
// NitroSDK's OS_InitArena, which sets the limits of the arenas, and OS_InitArenaEx, which on a DSi also protects the
// main memory that a DS does not have
void OS_InitArena(void);
void func_0207b23c(void);
// On a DS, makes a heap of the 16 KB at the start of the main memory in the arena's first free heap, once, and gives
// its number (-1 on a DSi or when there is no room)
int mem_bind_static_ram_alloc(int arena);
// The usable size of an allocated block (the library's pointer-size function, which MSL's __sys_pointer_size calls)
u32 func_0207b730(int arena, void *ptr);

// NitroSDK's OS_Init, which only starts the function that does the work, and the parts of it that other files define:
// OS_InitException, OS_InitVAlarm, OSi_InitVramExclusive, OS_InitReset and a function of os_application_jump.c that it
// calls last, on a DSi
void os_init(void);
void exceptions_init(void);
void OS_InitVAlarm(void);
// NitroSDK's OSi_InitVramExclusive, and OSi_UnlockVram: gives back the banks of the mask (a bit for each of A to I)
// that the lock ID has
void func_0207c4fc(void);
void func_0207c51c(u16 bank, u16 lockId);
// Takes the arguments that the last program left and keeps the ID of its sender, or drops both (os_application_jump.c)
void func_0207c888(void);

// Whether an address is in the extended main memory (0x0d000000 to 0x0e000000 on a DSi, 0x02400000 to 0x02800000
// otherwise) or in the video memory (the palettes, VRAM and OAM)
BOOL func_0207b744(u32 addr);
BOOL func_0207b780(u32 addr);

// NitroSDK's spin locks, which the two processors share through words of the shared main memory. A lock word has the
// flag that OS_LockByWord swaps the locker's ID into and the ID of the one who has it. The cartridge and the card have
// one each, and their ctrl functions give the bus to the ARM9 when it locks and back to the ARM7 when it unlocks
typedef struct OSLockWord {
    vu32 lockFlag;
    u16 ownerID;
    u16 extension;
} OSLockWord;

#define HW_INIT_LOCK_BUF 0x02ffffb0
#define HW_CARD_LOCK_BUF 0x02ffffe0
#define HW_CTRDG_LOCK_BUF 0x02ffffe8

#define OS_UNLOCK_SUCCESS 0
#define OS_UNLOCK_ERROR (-2)
#define OS_LOCK_ID_ERROR (-3)

// The ID of the lock ID pool (NitroSDK's OS_GetLockID and OS_ReleaseLockID, under swan's names)
s32 cart_key_create(void);
void cart_key_release(u16 lockId);
// NitroSDK's OS_InitLock: sets up the lock words and gives the card and the cartridge to the ARM7, then synchronizes
// with it
void func_02079fe4(void);
// NitroSDK's OSi_SyncWithOtherProc: the handshake of OS_InitLock, through the four bytes at p of shared memory. The
// other processor runs the opposite half (c is 0 or 1)
void func_02079f5c(int c, vu8 *p);
// NitroSDK's OS_LockByWord, OS_UnlockByWord and OS_TryLockByWord: lock the word with the ID (waiting, with ctrl run
// once it is locked), unlock it (OS_UNLOCK_ERROR if the ID is not its owner) or try to lock it, giving the previous
// flag
s32 func_0207a06c(u16 lockID, OSLockWord *lockp, void (*ctrl)(void));
s32 func_0207a0c0(u16 lockID, OSLockWord *lockp, void (*ctrl)(void));
s32 func_0207a114(u16 lockID, OSLockWord *lockp, void (*ctrl)(void));
// NitroSDK's OS_UnlockCartridge, OS_UnLockCartridge (the same under another spelling) and OS_TryLockCartridge
s32 func_0207a120(u16 lockID);
s32 func_0207a138(u16 lockID);
s32 func_0207a140(u16 lockID);
// NitroSDK's OS_LockCard, OS_UnlockCard, OS_UnLockCard and OS_TryLockCard
s32 func_0207a178(u16 lockID);
s32 func_0207a18c(u16 lockID);
s32 func_0207a1a0(u16 lockID);
s32 func_0207a1a8(u16 lockID);
// NitroSDK's OS_ReadOwnerOfLockWord: the ID of the one who has the lock
u16 cart_mutex_get_status(const OSLockWord *lockp);

// NitroSDK's OS_EnableInterrupts, OS_DisableInterrupts and OS_RestoreInterrupts, under swan's names
u32 CPU_IRQEnable(void);
u32 CPU_IRQDisable(void);
u32 CPU_SetIRQMask(u32 mask);
// The same for both IRQ and FIQ (NitroSDK's OS_DisableInterrupts_IrqAndFiq and OS_RestoreInterrupts_IrqAndFiq)
u32 CPU_IRQFIQDisable(void);
u32 CPU_SetIRQFIQMask(u32 mask);
// IRQ dispatcher; WCM also uses its address as a special mutex-owner marker.
void CPU_IRQInvoke(void);
// NitroSDK's OS_GetCpsrIrq: the CPSR's IRQ-disable bit, 0x80 while IRQs are off
u32 CPU_GetIRQMask(void);
// NitroSDK's OS_GetProcMode: the processor mode, 0x12 in an IRQ handler
u32 CPU_GetMode(void);
// The processor mode of an IRQ handler
#define OS_PROCMODE_IRQ 0x12
// NitroSDK's OS_EnableIrqMask, under swan's name
u32 CPU_EnableInterrupts(u32 mask);
// NitroSDK's OS_Sleep: waits for some milliseconds, letting other threads run
void func_0207aa04(u32 msec);
void exit(int status);

// Fills 32 bytes with values that differ from run to run
void OS_GetLowEntropyData(u32 buffer[8]);
// TwlSDK's OS_SpinWaitSysCycles: busy-waits for some cycles of the system clock
void func_0207c160(u32 cycles);

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
    u32 unk_60;
    u8 unk_64[0x1c];
    u32 romSize;
    u8 unk_84[0xe];
    u16 twlRegionStart; // where the part of the ROM that only the DSi mode reads starts, in units of 0x80000 bytes
    u8 unk_94[0xcc];
} ROMHeader;

// Gives the ROM header in main memory
const ROMHeader *rom_get_header(void);
// The TWL header that the DSi's system puts in main memory
const void *func_0206eee8(void);
// A saved copy of 0x88 bytes that the card read after the ROM header
const void *func_0206fed0(void);
// An empty function that the program's autoload step calls
void AutoloadCallback(void);
// Reads the console's MAC address (NitroSDK's OS_GetMacAddress)
void func_0207c33c(u8 *macAddress);
// NitroSDK's OS_GetFavoriteColorTable: the sixteen colors (RGB555) that the favorite color of the settings picks from
const u16 *func_0207c3b4(void);

// NitroSDK's OS_GetBootType, under swan's name: 1 when the game booted from the ROM
u32 sys_get_boot_indicator(void);
// NitroSDK's OS_GetConsoleType and OS_IsRunOnEmulator (the console type has the emulator's bit set), and a function
// that calls hw_isDSi
#define OS_CONSOLE_ENSATA 0x10000000
u32 func_0207ac24(void);
BOOL func_0207ac74(void);
BOOL func_0207acbc(void);

// NitroSDK's OS_WaitIrq and OS_IsRunOnTwl, under swan's names
void irq_waitFor(BOOL clear, u32 interrupts);
BOOL hw_isDSi(void);
// TwlSDK's OSi_IsCodecTwlMode: whether the game runs with the DSi's codec in TWL mode (on a DSi, with the TWL ROM
// header's codec flag set)
BOOL func_0207c210(void);
// The DSi's parental controls, of its settings. The fields' meanings are not known
typedef struct {
    u32 unk0_0 : 1;
    u32 unk0_1 : 4;
    u32 unk0_5 : 1;
    u32 unk0_6 : 26;
} TWLParentalControl;

// The DSi's parental controls, NULL on a DS
TWLParentalControl *func_0207c4b4(void);
// TwlSDK's OS_IsAvailableWireless, TRUE on a DS, and OS_IsAgreeEULA, FALSE on a DS
BOOL func_0207c438(void);
BOOL func_0207c45c(void);
// TwlSDK's OS_GetAgreedEULAVersion (0 on a DS), then a byte of the TWL ROM header (0 on a DS) and, by its role,
// TwlSDK's OS_IsForceDisableWireless (FALSE on a DS)
u8 func_0207c480(void);
u8 func_0207c49c(void);
BOOL func_0207c4d0(void);
// NitroSDK's OS_WaitVBlankIntr
void OS_WaitVBlankIntr(void);

// The buttons the ARM7 reads, X, Y and the lid among them, in shared memory. NitroSDK's PAD_DetectFold
#define HW_BUTTON_XY_BUF 0x02ffffa8
#define PAD_DETECT_FOLD_MASK 0x8000
#define PAD_DETECT_FOLD_SHIFT 15

static inline BOOL PAD_DetectFold(void) {
    return (*(vu16 *)HW_BUTTON_XY_BUF & PAD_DETECT_FOLD_MASK) >> PAD_DETECT_FOLD_SHIFT;
}

// The DS's owner settings
#define OS_OWNERINFO_NICKNAME_MAX 10
#define OS_OWNERINFO_COMMENT_MAX 26

typedef struct {
    u8 month;
    u8 day;
} OSBirthday;

typedef struct {
    u8 language;
    u8 favoriteColor;
    OSBirthday birthday;
    u16 nickName[OS_OWNERINFO_NICKNAME_MAX + 1];
    u16 nickNameLength;
    u16 comment[OS_OWNERINFO_COMMENT_MAX + 1];
    u16 commentLength;
} OSOwnerInfo;

// TwlSDK's owner settings, which add the country
typedef struct {
    u8 language;
    u8 favoriteColor;
    OSBirthday birthday;
    u16 nickName[OS_OWNERINFO_NICKNAME_MAX + 1];
    u16 nickNameLength;
    u16 comment[OS_OWNERINFO_COMMENT_MAX + 1];
    u16 commentLength;
    u8 country;
} OSOwnerInfoEx;

// The user settings that the ARM7 reads from the firmware into the shared memory (NitroSDK's NVRAMConfig), with the
// console's MAC address after them. Of the settings, the version, the alarm and the touch screen calibration are not
// named
#define HW_NVRAM_USER_INFO 0x02fffc80

typedef struct NVRAMOwnerInfo {
    u8 favoriteColor : 4;
    u8 : 4;
    OSBirthday birthday;
    u8 unk_03;
    u16 nickName[OS_OWNERINFO_NICKNAME_MAX];
    u8 nickNameLength;
    u8 unk_19;
    u16 comment[OS_OWNERINFO_COMMENT_MAX];
    u8 commentLength;
    u8 unk_4f;
} NVRAMOwnerInfo;

typedef struct NVRAMConfig {
    u16 unk_00;
    NVRAMOwnerInfo owner;
    u8 unk_52[0x12];
    u16 language : 3;
    u16 unk_64_3 : 13;
    u16 unk_66;
    s64 rtcOffset;
    u32 unk_70;
    u8 macAddress[6];
} NVRAMConfig;

void OS_GetOwnerInfo(OSOwnerInfo *info);
// TwlSDK's OS_GetOwnerInfoEx
void func_0207c3bc(OSOwnerInfoEx *info);
// The RTC offset the owner set in the settings, in seconds, from the shared main memory
s64 OS_GetOwnerRtcOffset(void);

// NitroSDK's message queues: a ring of messages, with the threads waiting to send and to receive
typedef void *OSMessage;

#define OS_MESSAGE_NOBLOCK 0

typedef struct OSThreadQueue {
    struct OSThread *head;
    struct OSThread *tail;
} OSThreadQueue;

typedef struct OSMessageQueue {
    OSThreadQueue queueSend;
    OSThreadQueue queueReceive;
    OSMessage *msgArray;
    s32 msgCount;
    s32 firstIndex;
    s32 usedCount;
} OSMessageQueue;

#define OS_MESSAGE_BLOCK 1

// Sets up a queue that holds count messages in the array, by OS_InitMessageQueue's code
void func_0207acc4(OSMessageQueue *queue, OSMessage *messages, s32 count);
BOOL OS_SendMessage(OSMessageQueue *queue, OSMessage message, s32 flags);
BOOL OS_ReceiveMessage(OSMessageQueue *mq, OSMessage *msg, s32 flags);
BOOL OS_ReadMessage(OSMessageQueue *mq, OSMessage *msg, s32 flags); // The first message, left in the queue
// Sends a message to the front of the queue, where the next OS_ReceiveMessage gets it
BOOL OS_JamMessage(OSMessageQueue *mq, OSMessage msg, s32 flags);
// NitroSDK's OS_SPrintf, OS_VSPrintf, OS_SNPrintf and OS_VSNPrintf: write a formatted text to dest, at most size bytes
// of it with a NUL for the last two, and return its length (the length it would have, for those). The ones with a
// va_list take the arguments in it
int func_0207a290(char *dest, const char *format, ...);
int func_0207a2ac(char *dest, const char *format, va_list args);
int func_0207a2c0(char *dest, u32 size, const char *format, ...);
int func_0207a2dc(char *dest, u32 size, const char *format, va_list args);
// Stop and restart thread switching, returning the previous state
u32 OS_DisableScheduler(void);
u32 OS_EnableScheduler(void);

// NitroSDK's contexts: the registers of a thread that does not run, with the divider's and the square root unit's. They
// are hand-written in assembly (os_context.c): OS_InitContext sets one up to run from newpc with the stack at newsp,
// OS_SaveContext stores the registers (it returns 0, and 1 when the context is loaded) and func_0207abbc, which is
// NitroSDK's OS_LoadContext, continues from one
typedef struct OSContext {
    u32 cpsr;
    u32 r[13];
    u32 sp;
    u32 lr;
    u32 pc_plus4;
    u32 sp_svc;
    CPContext cp_context;
} OSContext;

void OS_InitContext(OSContext *context, u32 newpc, u32 newsp);
BOOL OS_SaveContext(OSContext *context);
void func_0207abbc(OSContext *context);

// NitroSDK's threads, under swan's names: OS_CreateThread, whose stack is its top, OS_SleepThread, which sleeps on
// queue until OS_WakeupThread wakes it, and OS_WakeupThreadDirect, which starts a new thread. The priority is smaller
// for the more urgent threads: 0 to 31, the launcher thread that the game's main runs in has 16 and the idle thread 32
#define OS_THREAD_PRIORITY_MAX 31
#define OS_THREAD_LAUNCHER_PRIORITY 16

typedef enum OSThreadState {
    OS_THREAD_STATE_WAITING = 0,
    OS_THREAD_STATE_READY = 1,
    OS_THREAD_STATE_TERMINATED = 2,
} OSThreadState;

// The neighbors of a thread in the queue it sleeps in, and the list of the mutexes that a thread has
typedef struct OSThreadLink {
    struct OSThread *prev;
    struct OSThread *next;
} OSThreadLink;

typedef struct OSMutexQueue {
    struct OSMutex *head;
    struct OSMutex *tail;
} OSMutexQueue;

// The numbers at the ends of a stack that the stack overflow checks look for: at the beginning of a stack, 8 bytes
// under the address that a thread is given (4 under the IRQ stack's), and at the end where it would overflow
#define OS_STACK_BEGIN_MAGIC 0xfddb597d
#define OS_STACK_END_MAGIC 0x7bf9dd5b

// A thread's destructor runs in the thread, when it ends, with the value that it ended with
typedef void (*OSThreadDestructor)(void *arg);

// stackTop is where the stack ends (its lowest address) and stackBottom where it begins, as in NitroSDK. The thread
// that is being waited for has the waiting threads in joinQueue. The words of specific are for the libraries that keep
// something per thread (NitroWiFi's CPS keeps a thread's socket in the first)
typedef struct OSThread {
    OSContext context;
    OSThreadState state;
    struct OSThread *next;
    u32 id;
    u32 priority;
    void *profiler;
    OSThreadQueue *queue;
    OSThreadLink link;
    struct OSMutex *mutex;
    OSMutexQueue mutexQueue;
    u32 stackTop;
    u32 stackBottom;
    u32 stackWarningOffset;
    OSThreadQueue joinQueue;
    void *specific[3];
    struct OSAlarm *alarmForSleep;
    OSThreadDestructor destructor;
    u8 unk_b8[8];
} OSThread;

static inline void OS_InitThreadQueue(OSThreadQueue *queue) {
    queue->head = queue->tail = NULL;
}

// The scheduler's state, which swan names g_IsThreadSwitchRequested (NitroSDK's OSi_ThreadInfo): whether a thread
// switch was asked for while the scheduler could not do it, in an interrupt handler or while it was locked; the depth
// of the interrupts that are being handled; the thread that runs; the list of all threads, linked through their next,
// sorted by priority; and the function that is called with the old and the new thread at each switch
typedef void (*OSSwitchThreadCallback)(OSThread *from, OSThread *to);

typedef struct OSThreadInfo {
    u16 isNeedRescheduling;
    u16 irqDepth;
    OSThread *current;
    OSThread *list;
    OSSwitchThreadCallback switchCallback;
} OSThreadInfo;

extern OSThreadInfo g_IsThreadSwitchRequested;

// NitroSDK's OS_GetCurrentThread
static inline OSThread *OS_GetCurrentThread(void) {
    return g_IsThreadSwitchRequested.current;
}

// NitroSDK's OS_InitThread, which makes the thread that runs the launcher thread and creates the idle thread, and
// OS_IsThreadAvailable, which says whether it has run (a flag that other code reads, data_0214c21c)
void scheduler_init(void);
BOOL func_0207a57c(void);
extern BOOL data_0214c21c;
void scheduler_init_thread(OSThread *thread, void (*func)(void *), void *arg, void *stack, u32 stackSize, u32 prio);
void scheduler_yield(OSThreadQueue *queue);
void OS_WakeupThread(OSThreadQueue *queue);
void scheduler_start_thread(OSThread *thread);
// NitroSDK's OS_SetSwitchThreadCallback, which gives back the one it replaces, and OS_SetThreadDestructor
OSSwitchThreadCallback func_0207aa8c(OSSwitchThreadCallback callback);
void func_0207aafc(OSThread *thread, OSThreadDestructor destructor);
// Sets and gets a thread's priority, and ends the running thread (which does not return)
BOOL func_0207a998(OSThread *thread, u32 priority);
u32 func_0207aa00(const OSThread *thread);
void scheduler_thread_exit_proc(void);
// NitroSDK's OS_IsThreadTerminated, under swan's name, OS_DestroyThread and OS_KillThread (which ends a thread with a
// value for its destructor), by their code
BOOL scheduler_is_thread_ended(const OSThread *thread);
void func_0207a710(OSThread *thread);
void func_0207a768(OSThread *thread, void *arg);
// Waits for a thread's end
void OS_JoinThread(OSThread *thread);
// Runs the highest priority thread that is ready
void OS_RescheduleThread(void);
// Lets the other threads of the same priority run
void OS_YieldThread(void);
// TwlSDK's OS_SleepThreadDirect, by its code: puts a thread to sleep on a queue (the queue may be NULL: it sleeps until
// something starts it)
void scheduler_join_thread(OSThread *thread, OSThreadQueue *queue);

// NitroSDK's interrupts. The table of the 32 interrupt handlers (OS_IRQTable) and the queue of the threads that sleep
// in OS_WaitIrq (OSi_IrqThreadQueue) are in DTCM. The DMA channels, the DSi's new DMA channels and the timers (indexes
// 0 to 3, 4 to 7 and 8 to 11) have a callback and its argument in OSi_IrqCallbackInfo instead: their handlers call
// OSi_IrqCallback with the index. enable says whether the interrupt stays enabled after the callback
typedef void (*OSIrqFunction)(void);
typedef void (*OSIrqCallback)(void *arg);

typedef struct OSIrqCallbackInfo {
    OSIrqCallback func;
    u32 enable;
    void *arg;
} OSIrqCallbackInfo;

#define OSi_IRQCALLBACK_NUM 12
#define OSi_IRQCALLBACK_NO_DMA0 0
#define OSi_IRQCALLBACK_NO_NDMA0 4
#define OSi_IRQCALLBACK_NO_TIMER0 8

extern OSIrqFunction data_02fe0020[32];                      // OS_IRQTable
extern OSThreadQueue data_02fe00a0;                          // OSi_IrqThreadQueue
extern OSIrqCallbackInfo data_0214c178[OSi_IRQCALLBACK_NUM]; // OSi_IrqCallbackInfo

// OS_IrqDummy, and the handlers that DTCM's table has for DMA0 to 3, Timer0 to 3 and NDMA0 to 3 (OSi_IrqDma0 to 3,
// OSi_IrqTimer0 to 3 and OSi_IrqNDma0 to 3), in that order
void func_02079c24(void);
void func_02079c80(void);
void func_02079c8c(void);
void func_02079c98(void);
void func_02079ca4(void);
void func_02079cb0(void);
void func_02079cbc(void);
void func_02079cc8(void);
void func_02079cd4(void);
void func_02079ce0(void);
void func_02079cec(void);
void func_02079cf8(void);
void func_02079d04(void);

// NitroSDK's OS_InitIrqTable, which clears the queue of the threads that wait for an interrupt
void func_02079d10(void);
// NitroSDK's OS_SetIrqFunction, under swan's name: sets the function that runs for each interrupt of the mask
void CPU_SetIRQFunc(u32 interrupts, OSIrqFunction function);
// NitroSDK's OS_GetIrqFunction: the function of the first interrupt of the mask, or NULL
OSIrqFunction func_02079d88(u32 interrupts);
// NitroSDK's OSi_EnterDmaCallback, under swan's name, OSi_EnterNDmaCallback, which tail-calls TwlSDK's body in the LTD
// autoload, and OSi_EnterTimerCallback: set the callback of a DMA channel, new DMA channel or timer, and enable its
// interrupt
void irq_setDMACallback(u32 dmaNo, OSIrqCallback callback, void *arg);
void func_02079e18(u32 ndmaNo, OSIrqCallback callback, void *arg);
void OSi_EnterTimerCallback(int timerNo, OSIrqCallback callback, void *arg);
// NitroSDK's OS_SetIrqMask, OS_EnableIrqMask (declared above), OS_DisableIrqMask and OS_ResetRequestIrqMask, under
// swan's names: change IE or acknowledge bits of IF, returning the previous IE or IF
u32 CPU_SetInterrupts(u32 mask);
u32 CPU_DisableInterrupts(u32 mask);
u32 CPU_AcknowledgeInterrupts(u32 mask);
// Writes the magic numbers at both ends of the IRQ stack, for the exception handler to check
void OS_SetIrqStackChecker(void);

// The part of NitroSDK's reset and terminate handling that is Thumb code here (NitroSDK's OS_IsResetOccurred, the
// callback for the OS tag of the PXI FIFO, and the function that sends the ARM7 a command). The callback gets the
// command in the bits 8 to 14 of the word
BOOL func_02079f04(void);
void func_02079f10(u32 fifoTag, u32 data, BOOL err);
void func_02079f44(u32 command);

// NitroSDK's events, by their code: a word of flags, and the threads waiting for some of them to be set
typedef struct OSEvent {
    vu32 flag;
    OSThreadQueue queue;
} OSEvent;

// Whether func_0207c800 waits for all the flags of its pattern or for any of them
#define OS_EVENT_MODE_AND 0
#define OS_EVENT_MODE_OR 1

// Clears an event's flags and its queue (OS_InitEvent's code)
void func_0207c7f4(OSEvent *event);
// Sleeps until all or any (mode) of the flags of pattern are set, then clears the flags of clearBits and returns the
// flags that were set (OS_WaitEventEx's code)
u32 func_0207c800(OSEvent *event, u32 pattern, u32 mode, u32 clearBits);
// Sets flags and wakes the threads waiting on the event (OS_SignalEvent's code)
void func_0207c864(OSEvent *event, u32 setPattern);

// The block of arguments that a program leaves in the main memory for the one that starts next (os_deliverArg.c, after
// TwlSDK's DeliverArg API): who wrote it, a CRC16 of the block, and the arguments as a list of strings that ends with
// an empty one
typedef struct OSDeliverArgInfo {
    u64 titleId;
    u8 unk_08;
    u8 flags;
    u16 makerCode;
    u16 argSpace;
    u16 argCount;
    u16 crc16;
    u8 unk_12[2];
    u8 args[0x2ec];
} OSDeliverArgInfo;

// Sets the buffer up with no arguments (leaving reserved bytes of it unused), sends its content to the next program
// and clears it (-1 when it was not set up), takes the block that the last program left into it (0 whether the CRC16
// was right or not, -1 when there is none or no buffer), forgets the buffer, and gives the title ID of the sender
void func_0207c574(OSDeliverArgInfo *info, u32 reserved);
int func_0207c5b0(void);
int func_0207c624(void);
void func_0207c698(void);
u64 func_0207c6a4(void);

// Alarms: OS_SetAlarm's fires once after the ticks and calls the function with the argument
typedef u64 OSTick;
typedef void (*OSAlarmHandler)(void *arg);

// An alarm: NitroSDK's OSAlarm. A one-shot alarm has no period; a periodic one fires at start plus a multiple of it
typedef struct OSAlarm {
    OSAlarmHandler handler;
    void *arg;
    u32 tag;
    OSTick fire;
    struct OSAlarm *prev;
    struct OSAlarm *next;
    OSTick period;
    OSTick start;
} OSAlarm;

// Starts the alarm module, and whether it has started; WCM starts it before it creates its alarm
void OS_InitAlarm(void);
BOOL OS_IsAlarmAvailable(void);
void OS_CreateAlarm(OSAlarm *alarm);
void OS_SetAlarm(OSAlarm *alarm, OSTick tick, OSAlarmHandler handler, void *arg);
void OS_CancelAlarm(OSAlarm *alarm);

// A V-count alarm: NitroSDK's OSVAlarm. It fires when the LCD scan is at line vcount of the frame, which the alarm's
// frame counts from the first V-count alarm that was set, and up to delay lines later
typedef void (*OSVAlarmHandler)(void *arg);

typedef struct OSVAlarm {
    OSVAlarmHandler handler;
    void *arg;
    u32 tag;
    u32 frame;
    s16 vcount;
    s16 delay;
    struct OSVAlarm *prev;
    struct OSVAlarm *next;
    BOOL periodic;
    u32 unk_20;
    BOOL canceled;
} OSVAlarm;

// NitroSDK's OS_CreateVAlarm and OS_SetVAlarm
void func_0207befc(OSVAlarm *alarm);
void OS_SetVAlarm(OSVAlarm *alarm, s16 vcount, s16 delay, OSVAlarmHandler handler, void *arg);

// NitroSDK's mutexes, which a thread may lock again; OS_LockMutex sleeps until the mutex is free
typedef struct OSMutexLink {
    struct OSMutex *next;
    struct OSMutex *prev;
} OSMutexLink;

// count is the lock count in its low 24 bits and, in TwlSDK, the mutex's type in the top 8 (os_mutex.c)
typedef struct OSMutex {
    OSThreadQueue queue;
    OSThread *thread;
    s32 count;
    OSMutexLink link;
} OSMutex;

// Takes the first mutex off a thread's list of them and gives it (os_thread.c's OSi_RemoveMutexLinkFromQueue)
OSMutex *OSi_RemoveMutexLinkFromQueue(OSMutexQueue *queue);
// NitroSDK's OSi_UnlockAllMutex: unlocks the mutexes that a thread has, waking the threads that wait for them
void func_0207aeb0(OSThread *thread);
void OS_InitMutex(OSMutex *mutex);
void OS_LockMutex(OSMutex *mutex);
// TRUE when it got the mutex, without waiting
BOOL OS_TryLockMutex(OSMutex *mutex);
void OS_UnlockMutex(OSMutex *mutex);

#endif // POKEBW2_NITRO_OS_H
