#ifndef POKEBW2_NITRO_OS_H
#define POKEBW2_NITRO_OS_H

#include "types.h"
#include "nitro/hw.h"

// The tick count of the system timer, which swan names clock
u64 clock(void);
// Whether the system timer was started, and starting it
BOOL clock_is_inited(void);
void clock_init(void);
// The system clock, and NitroSDK's OS_TicksToSeconds: the system timer ticks every 64 cycles of it
#define OS_SYSTEM_CLOCK 33514000
#define OS_TicksToSeconds(tick) (((tick) * 64) / OS_SYSTEM_CLOCK)
#define OS_TicksToMilliSeconds(tick) (((tick) * 64) / (OS_SYSTEM_CLOCK / 1000))
#define OS_TicksToMicroSeconds(tick) (((tick) * 64 * 1000) / (OS_SYSTEM_CLOCK / 1000))
#define OS_MilliSecondsToTicks(msec) ((u64)(((OS_SYSTEM_CLOCK / 1000) * (u64)(msec)) / 64))
// NitroSDK's OSi_ReferSymbol, which does nothing but be called with a library's signature ("[SDK+...]"), keeping the
// signature in the link
void func_02004eb0(const char *signature);
// Restarts the game, which swan names sys_reset
void sys_reset(u32 parameter);
// Stops the game after a fatal error, calling the registered handler first
void sys_exit(void);
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

// The start of DTCM, where the linker places the DTCM module
extern u32 SDK_AUTOLOAD_DTCM_START[];
#define HW_DTCM ((u32)SDK_AUTOLOAD_DTCM_START)
// The interrupt flags that OS_WaitIrq checks, at the end of DTCM
#define HW_INTR_CHECK_BUF (HW_DTCM + 0x3ff8)

#define OS_IE_V_BLANK 0x1
#define OS_IE_CARD_DATA 0x80000
// The DSi's new DMA channel 1
#define OS_IE_NDMA1 0x20000000

static inline u32 OS_GetVBlankCount(void) {
    return *(vu32 *)HW_VBLANK_COUNT_BUF;
}

// Waits for one of the interrupts, after clearing their flags if clear is set. NitroSDK's OS_WaitIrq, under swan's
// name
void CPU_WaitIntrBit(BOOL clear, u32 interrupts);

static inline void OS_SetIrqCheckFlag(u32 interrupts) {
    *(vu32 *)HW_INTR_CHECK_BUF |= interrupts;
}

// Memory arenas and OS heaps, under swan's names: OS_GetArenaLo, OS_GetArenaHi, OS_SetArenaLo, OS_SetArenaHi,
// OS_AllocFromArenaLo, OS_InitAlloc, OS_CreateHeap, OS_AllocFromHeap and OS_FreeToHeap
#define OS_ARENA_MAIN 0
#define OS_ARENA_DTCM 4
#define OS_HEAP_INVALID (-1)

void *GetUserMemRegionStart(int arena);
void *GetUserMemRegionEnd(int arena);
void SetUserMemRegionStart(int arena, void *start);
void SetUserMemRegionEnd(int arena, void *end);
void *mem_alloc_direct(int arena, u32 size, u32 alignment);
void *mem_init_alloc_area(int arena, void *start, void *end, int maxHeaps);
int mem_bind_range(int arena, void *start, void *end);
void *malloc_device(int arena, int heap, u32 size);
void free_device(int arena, int heap, void *ptr);

// NitroSDK's OS_GetLockID and OS_ReleaseLockID, under swan's names
#define OS_LOCK_ID_ERROR (-3)

s32 cart_key_create(void);
void cart_key_release(u16 lockId);
// NitroSDK's OS_LockCard and OS_UnlockCard
s32 func_0207a178(u16 lockId);
s32 func_0207a1a0(u16 lockId);

// NitroSDK's OS_DisableInterrupts and OS_RestoreInterrupts, under swan's names
u32 CPU_IRQDisable(void);
u32 CPU_SetIRQMask(u32 mask);
// IRQ dispatcher; WCM also uses its address as a special mutex-owner marker.
void CPU_IRQInvoke(void);
// NitroSDK's OS_GetProcMode: the processor mode, 0x12 in an IRQ handler
u32 CPU_GetMode(void);
// NitroSDK's OS_EnableIrqMask, under swan's name
u32 CPU_EnableInterrupts(u32 mask);
// NitroSDK's OS_Sleep: waits for some milliseconds, letting other threads run
void func_0207aa04(u32 msec);
void exit(int status);

// Fills 32 bytes with values that differ from run to run
void OS_GetLowEntropyData(u32 buffer[8]);
// NitroSDK's STD_CompareString
int STD_CompareString(const char *a, const char *b);
// NitroSDK's STD_GetStringLength
int NNS_STD_StrLen(const char *str);
// NitroSDK's STD_CopyLString: copies at most size - 1 characters and a NUL, returning the length of src
int func_0207f7cc(char *dst, const char *src, int size);
// NitroSDK's STD_TSPrintf
int func_020800e8(char *dst, const char *fmt, ...);
// TwlSDK's OS_SpinWaitSysCycles: busy-waits for some cycles of the system clock
void func_0207c160(u32 cycles);

// The ROM header of the running program, which the card puts at the end of main memory. Only the fields that
// NitroSDK's multiboot library reads are named
typedef struct ROMHeader {
    u8 unk_00[0x12];
    u8 unitCode;
    u8 unk_13[0xd];
    u32 arm9RomOffset;
    u32 arm9Entry;
    u32 arm9RamAddr;
    u32 arm9Size;
    u32 arm7RomOffset;
    u32 arm7Entry;
    u32 arm7RamAddr;
    u32 arm7Size;
    u8 unk_40[0x20];
    u32 unk_60;
    u8 unk_64[0x1c];
    u32 romSize;
    u8 unk_84[0xdc];
} ROMHeader;

// Gives the ROM header in main memory
const ROMHeader *rom_get_header(void);
// The TWL header that the DSi's system puts in main memory
const void *func_0206eee8(void);
// A saved copy of 0x88 bytes that the card read after the ROM header
const void *func_0206fed0(void);
// An empty function that the program's autoload step calls
void AutoloadCallback(void);
// Whether a bit of a word of the shared memory is set, which the other processor sets for a FIFO tag once it is
// ready for it
BOOL func_0207ca48(u32 fifoTag, u32 processor);
// Reads the console's MAC address
void func_0207c33c(u8 *mac);

// NitroSDK's OS_WaitIrq and OS_IsRunOnTwl, under swan's names
void irq_waitFor(BOOL clear, u32 interrupts);
BOOL hw_isDSi(void);
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

void OS_GetOwnerInfo(OSOwnerInfo *info);
// The RTC offset the owner set in the settings, in seconds, from the shared main memory
s64 OS_GetOwnerRtcOffset(void);

int OS_SNPrintf(char *dst, u32 len, const char *format, ...);

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
// OS_SNPrintf's role: snprintf
int func_0207a2c0(char *dest, u32 size, const char *format, ...);
// OS_DisableScheduler and OS_EnableScheduler's roles
u32 func_0207aab4(void);
u32 func_0207aad8(void);

// NitroSDK's threads, under swan's names: OS_CreateThread, whose stack is its top, OS_SleepThread, which sleeps on
// queue until OS_WakeupThread wakes it, and OS_WakeupThreadDirect, which starts a new thread. Of the thread's fields
// only the link of the list of all threads and the specific words (NitroWiFi's CPS keeps a thread's socket in the
// first) are known yet
typedef struct OSThread {
    u8 unk_00[0x68];
    struct OSThread *next;
    u8 unk_6c[0x38];
    void *specific[3];
    u8 unk_b0[0x10];
} OSThread;

static inline void OS_InitThreadQueue(OSThreadQueue *queue) {
    queue->head = queue->tail = NULL;
}

void scheduler_init_thread(OSThread *thread, void (*func)(void *), void *arg, void *stack, u32 stackSize, u32 prio);
void scheduler_yield(OSThreadQueue *queue);
void OS_WakeupThread(OSThreadQueue *queue);
void scheduler_start_thread(OSThread *thread);
// Sets and gets a thread's priority, and ends the running thread (which does not return)
BOOL func_0207a998(OSThread *thread, u32 priority);
u32 func_0207aa00(const OSThread *thread);
void scheduler_thread_exit_proc(void);
// NitroSDK's OS_IsThreadTerminated, under swan's name, and OS_DestroyThread, by its code
BOOL scheduler_is_thread_ended(const OSThread *thread);
void func_0207a710(OSThread *thread);
// NitroSDK's OS_JoinThread, by its code: waits for a thread's end
void func_0207a800(OSThread *thread);
// NitroSDK's OS_RescheduleThread, by its place before OS_YieldThread and by its use after OS_EnableScheduler: runs the
// highest priority thread that is ready
void func_0207a918(void);
// NitroSDK's OS_YieldThread, by its place between OS_JoinThread and OS_SetThreadPriority: lets the other threads of the
// same priority run
void func_0207a92c(void);

// The scheduler's state, which swan names g_IsThreadSwitchRequested: the thread that runs is its second word, and the
// list of all threads, linked through their next, its third (NitroSDK's OSThreadInfo)
extern struct {
    int unk_00;
    OSThread *current;
    OSThread *list;
} g_IsThreadSwitchRequested;

// NitroSDK's OS_GetCurrentThread
static inline OSThread *OS_GetCurrentThread(void) {
    return g_IsThreadSwitchRequested.current;
}

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

// NitroSDK's alarms, by their code: OS_CreateAlarm, OS_SetAlarm (the alarm fires once after the ticks and calls the
// function with the argument) and OS_CancelAlarm
typedef struct OSAlarm {
    u8 unk_00[0x2c];
} OSAlarm;
void func_0207bc24(OSAlarm *alarm);
void func_0207bcf4(OSAlarm *alarm, u64 ticks, void (*handler)(void *arg), void *arg);
void func_0207bd3c(OSAlarm *alarm);

// NitroSDK's mutexes, which a thread may lock again; OS_LockMutex sleeps until the mutex is free
typedef struct OSMutexLink {
    struct OSMutex *next;
    struct OSMutex *prev;
} OSMutexLink;

typedef struct OSMutex {
    OSThreadQueue queue;
    OSThread *thread;
    s32 count;
    OSMutexLink link;
} OSMutex;

void OS_InitMutex(OSMutex *mutex);
void OS_LockMutex(OSMutex *mutex);
// NitroSDK's OS_TryLockMutex: TRUE when it got the mutex, without waiting
BOOL func_0207aeec(OSMutex *mutex);
void OS_UnlockMutex(OSMutex *mutex);

#endif // POKEBW2_NITRO_OS_H
