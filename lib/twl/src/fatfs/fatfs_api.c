// TwlSDK's FATFS, the DSi's file system, ARM9 side (library code in the DSi-only LTD autoload). The ARM9 hands
// requests to the ARM7, which runs the file system, by sending the address of a 0x800-byte buffer over the PXI tag
// 0x14, and waits until the ARM7 has answered and the request is in the list of the done ones. Only the code that the
// game keeps is here: getting a request buffer, sending it, waiting for it and giving it back, and the one request
// that the exit code sends. The ROM names none of it; the file name and those of the types and data are descriptive
// guesses, and the functions keep their default names (twl/fatfs.h says what the public ones do).

#include "types.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "nitro/svc.h"
#include "twl/fatfs.h"

// The part of a request that the ARM7 sees, which the ARM9 flushes from its cache before it sends the address
#define FATFS_REQUEST_SIZE 0x800

// The request that the exit code sends, which the ARM7 answers with 0 for success
#define FATFS_COMMAND_1A 0x1a
#define FATFS_RESULT_SUCCESS 0

// What a request holds until the ARM7 answers; its meaning is a guess
#define FATFS_RESULT_UNANSWERED 2

// The two values that the library gives a request's kind, whose names are guesses: one that was reserved for a thread,
// and one that is shared
#define FATFS_KIND_SHARED 0
#define FATFS_KIND_RESERVED 2

// The ARM9's processor mode in an interrupt handler, and what a thread's id is there (no thread runs)
#define FATFS_IRQ_MODE 0x12
#define FATFS_IRQ_THREAD ((OSThread *)0x80000000)

// How long a wait in an interrupt handler spins between two looks at the FIFO
#define FATFS_IRQ_SPIN 10000

// A request: its command, the result that the ARM7 writes, the link to the next of the done requests, and after the
// 0x800 bytes that the ARM7 sees the thread that has the request reserved and the link of the list of those. The
// list of the done requests is volatile because the PXI callback, which runs in an interrupt, adds to it while a
// thread looks through it
typedef struct FatfsRequest {
    u32 command;                        // 0x000
    u32 result;                         // 0x004
    u32 kind;                           // 0x008: FATFS_KIND_*
    struct FatfsRequest *volatile next; // 0x00c: in the list of the done requests
    u8 unk_10[FATFS_REQUEST_SIZE - 0x10];
    OSThread *owner;                   // 0x800
    u8 unk_804[8];                     // 0x804
    struct FatfsRequest *nextReserved; // 0x80c: in the list of the requests reserved for threads
} FatfsRequest;

// What the library knows of a thread that sends requests: the request that it has now
typedef struct FatfsThread {
    struct FatfsThread *next;
    OSThread *thread;
    FatfsRequest *request;
} FatfsThread;

// The library's state: the threads and the requests reserved for them, the queue of the threads that wait for a
// request to be done and the list of the done ones, the one request that all other threads share (the mutex is theirs)
// and the one for an interrupt handler
typedef struct FatfsState {
    u8 unk_00[4];
    FatfsThread *threads;           // 0x04
    u8 unk_08[0x18];                // 0x08
    FatfsRequest *reserved;         // 0x20
    u8 unk_24[0x1c];                // 0x24
    OSThreadQueue waitQueue;        // 0x40
    FatfsRequest *volatile done;    // 0x48
    OSMutex mutex;                  // 0x4c
    FatfsRequest *sharedRequest;    // 0x64
    FatfsRequest *interruptRequest; // 0x68
} FatfsState;

static void func_027040b4(FatfsRequest *request);
static OSThread *func_027040ec(void);
static FatfsRequest *func_0270410c(u32 command);
static FatfsRequest *func_027041e8(u32 command);
static void func_02704230(FatfsRequest *request);
static void func_02704258(FatfsRequest *request);
static void func_02704264(FatfsRequest *request);

static BOOL sFatfsInitialized;
static FatfsState sFatfs;

// Sends the ARM7 the request: its address in units of 32 bytes, the size of a cache line
static void func_027040b4(FatfsRequest *request) {
    u32 addr;

    DC_FlushRange(request, FATFS_REQUEST_SIZE);
    addr = (u32)request >> 5;
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_FATFS, addr, FALSE) < 0) {
    }
}

// The id of the running thread
static OSThread *func_027040ec(void) {
    if (OS_GetProcMode() == FATFS_IRQ_MODE) {
        return FATFS_IRQ_THREAD;
    }
    return OS_GetCurrentThread();
}

// The request that was reserved for the running thread, which it takes, or NULL
static inline FatfsRequest *TakeReservedRequest(void) {
    FatfsRequest *request = NULL;
    FatfsRequest *candidate;
    OSThread *current;
    u32 irq;

    irq = OS_DisableInterrupts();
    current = func_027040ec();
    for (candidate = sFatfs.reserved; candidate != NULL; candidate = candidate->nextReserved) {
        if (candidate->owner == current) {
            candidate->owner = NULL;
            request = candidate;
            candidate->kind = FATFS_KIND_RESERVED;
            break;
        }
    }
    OS_RestoreInterrupts(irq);
    return request;
}

// Takes a request for the command: the one that was reserved for the thread, or else the shared one, after waiting for
// the thread that has it. The thread's entry in the library's list gets it
static FatfsRequest *func_0270410c(u32 command) {
    FatfsRequest *request;
    FatfsThread *thread;
    OSThread *current;
    u32 irq;

    request = TakeReservedRequest();
    if (request == NULL) {
        OS_LockMutex(&sFatfs.mutex);
        request = sFatfs.sharedRequest;
        request->kind = FATFS_KIND_SHARED;
    }

    irq = OS_DisableInterrupts();
    current = func_027040ec();
    for (thread = sFatfs.threads; thread != NULL; thread = thread->next) {
        if (thread->thread == current) {
            thread->request = request;
            break;
        }
    }
    OS_RestoreInterrupts(irq);

    request->command = command;
    request->result = FATFS_RESULT_UNANSWERED;
    request->next = NULL;
    return request;
}

// The same for any context: an interrupt handler gets its own request
static FatfsRequest *func_027041e8(u32 command) {
    FatfsRequest *request;

    if (OS_GetProcMode() != FATFS_IRQ_MODE) {
        return func_0270410c(command);
    }
    request = sFatfs.interruptRequest;
    request->command = command;
    request->result = FATFS_RESULT_UNANSWERED;
    request->next = NULL;
    request->kind = FATFS_KIND_SHARED;
    return request;
}

// Gives back the request: only the shared one is under the mutex
static void func_02704230(FatfsRequest *request) {
    if (request == sFatfs.sharedRequest) {
        OS_UnlockMutex(&sFatfs.mutex);
    }
}

// Sends the request
static void func_02704258(FatfsRequest *request) {
    func_027040b4(request);
}

// Waits until the request is in the list of the done ones and takes it out. A thread sleeps meanwhile; an interrupt
// handler takes the words from the FIFO and spins
static void func_02704264(FatfsRequest *request) {
    BOOL waiting = TRUE;

    while (waiting) {
        // The PXI callback adds to the list of done requests from its interrupt
        FatfsRequest *volatile *link;
        u32 irq = OS_DisableInterrupts();

        for (link = &sFatfs.done; *link != NULL; link = &(*link)->next) {
            if (*link == request) {
                waiting = FALSE;
                *link = (*link)->next;
                request->next = NULL;
                break;
            }
        }
        if (waiting) {
            if (OS_GetProcMode() == FATFS_IRQ_MODE) {
                PXIi_HandlerRecvFifoNotEmpty();
                SVC_WaitByLoop(FATFS_IRQ_SPIN);
            } else {
                OS_SleepThread(&sFatfs.waitQueue);
            }
        }
        OS_RestoreInterrupts(irq);
    }
}

BOOL func_02704318(void) {
    return sFatfsInitialized;
}

BOOL func_02704328(void) {
    FatfsRequest *request = func_027041e8(FATFS_COMMAND_1A);
    BOOL success;

    func_02704258(request);
    func_02704264(request);
    success = request->result == FATFS_RESULT_SUCCESS;
    func_02704230(request);
    return success;
}
