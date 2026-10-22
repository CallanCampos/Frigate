// NitroSDK's SND (snd_command.c, as ntrtwl's reconstruction of NitroSDK has it): the queue of commands for the ARM7's
// sound driver. A command is taken from a pool of 256, filled in and pushed on the list of ready commands; flushing
// sends the list as one packet through the PXI FIFO, and when the ARM7 is done with a packet the commands go back to
// the pool.
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "nitro/snd.h"

// The commands of the pool
#define SND_COMMAND_NUM 256
// How long to wait between two looks at what the ARM7 did, in system clock cycles
#define SND_WAIT_CYCLES 50
// The emulator's register that the check for a sound driver writes the query to and reads the answer from
#define reg_SND_EMULATOR_DRIVER (*(vu32 *)0x04fff200)
#define SND_EMULATOR_QUERY 0x10

// The ARM7 reads the pool and shares the work with the ARM9 through the cache, so each starts on a cache line. The
// queue's state and the packets are in nitro_snd_main.c (see snd.h), before these
static SNDCommand sCommandArray[SND_COMMAND_NUM] ATTRIBUTE_ALIGN(32);
static SNDSharedWork sSharedWork ATTRIBUTE_ALIGN(32);

static int SND_CountFreeCommand(void);
static int SND_CountReservedCommand(void);
static int SND_CountWaitingCommand(void);
static void PxiFifoCallback(PXIFifoTag fifoTag, u32 data, BOOL err);
static void InitPXI(void);
static void RequestCommandProc(void);
static SNDCommand *AllocCommand(void);
static BOOL IsCommandAvailable(void);

void SND_CommandInit(void) {
    int i;
    SNDCommand *command;

    InitPXI();
    SNDi_Queue.freeHead = sCommandArray;
    for (i = 0; i < SND_COMMAND_NUM - 1; i++) {
        sCommandArray[i].next = &sCommandArray[i + 1];
    }
    sCommandArray[SND_COMMAND_NUM - 1].next = NULL;
    SNDi_Queue.freeTail = &sCommandArray[SND_COMMAND_NUM - 1];
    SNDi_Queue.readyHead = NULL;
    SNDi_Queue.readyTail = NULL;
    SNDi_Queue.sentCount = 0;
    SNDi_Queue.sentRead = 0;
    SNDi_Queue.sentWrite = 0;
    SNDi_Queue.nextTag = 1;
    SNDi_Queue.receivedTag = 0;
    SNDi_SharedWork = &sSharedWork;
    SNDi_InitSharedWork(SNDi_SharedWork);

    // Tell the ARM7 where the shared work is
    command = SND_AllocCommand(SND_COMMAND_BLOCK);
    if (command) {
        command->id = SND_COMMAND_SHARED_WORK;
        command->arg[0] = (u32)SNDi_SharedWork;
        SND_PushCommand(command);
        SND_FlushCommand(SND_COMMAND_BLOCK);
    }
}

// NitroSDK's SND_RecvCommandReply: takes back the next packet that the ARM7 is done with and returns its commands, or
// returns NULL when there is none. With SND_COMMAND_BLOCK it waits for one
const SNDCommand *SND_RecvCommandReply(u32 flags) {
    OSIntrMode enabled = OS_DisableInterrupts();
    SNDCommand *command;
    SNDCommand *last;

    if (flags & SND_COMMAND_BLOCK) {
        while (SNDi_Queue.receivedTag == SNDi_GetFinishedCommandTag()) {
            OS_RestoreInterrupts(enabled);
            OS_SpinWaitSysCycles(SND_WAIT_CYCLES);
            enabled = OS_DisableInterrupts();
        }
    } else if (SNDi_Queue.receivedTag == SNDi_GetFinishedCommandTag()) {
        OS_RestoreInterrupts(enabled);
        return NULL;
    }

    command = sWaitingCommandListQueue[SNDi_Queue.sentRead];
    SNDi_Queue.sentRead++;
    if (SNDi_Queue.sentRead > SND_PXI_FIFO_MESSAGE_BUFSIZE) {
        SNDi_Queue.sentRead = 0;
    }

    // The commands go back to the end of the pool
    for (last = command; last->next != NULL; last = last->next) {
    }
    if (SNDi_Queue.freeTail != NULL) {
        SNDi_Queue.freeTail->next = command;
    } else {
        SNDi_Queue.freeHead = command;
    }
    SNDi_Queue.freeTail = last;
    SNDi_Queue.sentCount--;
    SNDi_Queue.receivedTag++;
    OS_RestoreInterrupts(enabled);
    return command;
}

// NitroSDK's SND_AllocCommand: takes a command from the pool. When the pool is empty it returns NULL, or with
// SND_COMMAND_BLOCK it waits until the ARM7 returns some
SNDCommand *SND_AllocCommand(u32 flags) {
    SNDCommand *command;

    if (!IsCommandAvailable()) {
        return NULL;
    }
    command = AllocCommand();
    if (command != NULL) {
        return command;
    }
    if (!(flags & SND_COMMAND_BLOCK)) {
        return NULL;
    }

    if (SND_CountWaitingCommand() > 0) {
        // Packets are on their way: take back what is there
        while (SND_RecvCommandReply(SND_COMMAND_NOBLOCK) != NULL) {
        }
        command = AllocCommand();
        if (command != NULL) {
            return command;
        }
    } else {
        // Everything is waiting to be sent
        SND_FlushCommand(SND_COMMAND_BLOCK);
    }
    RequestCommandProc();
    do {
        SND_RecvCommandReply(SND_COMMAND_BLOCK);
        command = AllocCommand();
    } while (command == NULL);
    return command;
}

// NitroSDK's SND_PushCommand: adds a command to the ready list
void SND_PushCommand(SNDCommand *command) {
    OSIntrMode enabled = OS_DisableInterrupts();

    if (SNDi_Queue.readyTail == NULL) {
        SNDi_Queue.readyHead = command;
        SNDi_Queue.readyTail = command;
    } else {
        SNDi_Queue.readyTail->next = command;
        SNDi_Queue.readyTail = command;
    }
    command->next = NULL;
    OS_RestoreInterrupts(enabled);
}

// NitroSDK's SND_FlushCommand: sends the ready commands as a packet. It returns FALSE when it could not, because no
// packet could be sent without waiting and SND_COMMAND_BLOCK is not set. SND_COMMAND_IMMEDIATE wakes the ARM7 up
BOOL SND_FlushCommand(u32 flags) {
    OSIntrMode enabled = OS_DisableInterrupts();

    if (!SNDi_Queue.readyHead) {
        OS_RestoreInterrupts(enabled);
        return TRUE;
    }

    if (SNDi_Queue.sentCount >= SND_PXI_FIFO_MESSAGE_BUFSIZE) {
        if (!(flags & SND_COMMAND_BLOCK)) {
            OS_RestoreInterrupts(enabled);
            return FALSE;
        }
        do {
            SND_RecvCommandReply(SND_COMMAND_BLOCK);
        } while (SNDi_Queue.sentCount >= SND_PXI_FIFO_MESSAGE_BUFSIZE);
        if (!SNDi_Queue.readyHead) {
            OS_RestoreInterrupts(enabled);
            return TRUE;
        }
    }

    DC_FlushRange(sCommandArray, sizeof(sCommandArray));
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_SOUND, (u32)SNDi_Queue.readyHead, FALSE) < 0) {
        if (!(flags & SND_COMMAND_BLOCK)) {
            OS_RestoreInterrupts(enabled);
            return FALSE;
        }
        while (SNDi_Queue.sentCount >= SND_PXI_FIFO_MESSAGE_BUFSIZE ||
               PXI_SendWordByFifo(PXI_FIFO_TAG_SOUND, (u32)SNDi_Queue.readyHead, FALSE) < 0) {
            OS_RestoreInterrupts(enabled);
            SND_RecvCommandReply(SND_COMMAND_NOBLOCK);
            enabled = OS_DisableInterrupts();
            DC_FlushRange(sCommandArray, sizeof(sCommandArray));
            if (!SNDi_Queue.readyHead) {
                OS_RestoreInterrupts(enabled);
                return TRUE;
            }
        }
    }

    sWaitingCommandListQueue[SNDi_Queue.sentWrite] = SNDi_Queue.readyHead;
    SNDi_Queue.sentWrite++;
    if (SNDi_Queue.sentWrite > SND_PXI_FIFO_MESSAGE_BUFSIZE) {
        SNDi_Queue.sentWrite = 0;
    }
    SNDi_Queue.readyHead = NULL;
    SNDi_Queue.readyTail = NULL;
    SNDi_Queue.sentCount++;
    SNDi_Queue.nextTag++;
    OS_RestoreInterrupts(enabled);

    if (flags & SND_COMMAND_IMMEDIATE) {
        RequestCommandProc();
    }
    return TRUE;
}

// NitroSDK's SND_WaitForCommandProc: waits until the ARM7 is done with the packet that has the tag
void SND_WaitForCommandProc(u32 tag) {
    if (SND_IsFinishedCommandTag(tag)) {
        return;
    }
    while (SND_RecvCommandReply(SND_COMMAND_NOBLOCK) != NULL) {
    }
    if (SND_IsFinishedCommandTag(tag)) {
        return;
    }
    RequestCommandProc();
    while (!SND_IsFinishedCommandTag(tag)) {
        SND_RecvCommandReply(SND_COMMAND_BLOCK);
    }
}

// NitroSDK's SND_GetCurrentCommandTag: the tag that SND_IsFinishedCommandTag tells for the commands pushed last,
// which is the next packet's while commands wait to be flushed, and else the last packet taken back
u32 SND_GetCurrentCommandTag(void) {
    OSIntrMode enabled = OS_DisableInterrupts();
    u32 tag;

    if (SNDi_Queue.readyHead == NULL) {
        tag = SNDi_Queue.receivedTag;
    } else {
        tag = SNDi_Queue.nextTag;
    }
    OS_RestoreInterrupts(enabled);
    return tag;
}

// NitroSDK's SND_IsFinishedCommandTag: whether the packet with the tag has come back. The tags wrap around
BOOL SND_IsFinishedCommandTag(u32 tag) {
    OSIntrMode enabled = OS_DisableInterrupts();
    BOOL finished;

    if (tag > SNDi_Queue.receivedTag) {
        if (tag - SNDi_Queue.receivedTag < 0x80000000) {
            finished = FALSE;
        } else {
            finished = TRUE;
        }
    } else {
        if (SNDi_Queue.receivedTag - tag < 0x80000000) {
            finished = TRUE;
        } else {
            finished = FALSE;
        }
    }
    OS_RestoreInterrupts(enabled);
    return finished;
}

static int SND_CountFreeCommand(void) {
    OSIntrMode enabled = OS_DisableInterrupts();
    int count = 0;
    SNDCommand *command;

    for (command = SNDi_Queue.freeHead; command != NULL; command = command->next) {
        count++;
    }
    OS_RestoreInterrupts(enabled);
    return count;
}

static int SND_CountReservedCommand(void) {
    OSIntrMode enabled = OS_DisableInterrupts();
    int count = 0;
    SNDCommand *command;

    for (command = SNDi_Queue.readyHead; command != NULL; command = command->next) {
        count++;
    }
    OS_RestoreInterrupts(enabled);
    return count;
}

// NitroSDK's SND_CountWaitingCommand: the commands that are in neither the pool nor the ready list, so with the ARM7
static int SND_CountWaitingCommand(void) {
    return SND_COMMAND_NUM - SND_CountFreeCommand() - SND_CountReservedCommand();
}

// The function that the PXI FIFO calls with the words that the ARM7 sends for the sound tag: the number of an alarm and
// its ID, which are the alarm that went off. NitroSDK's PxiFifoCallback
static void PxiFifoCallback(PXIFifoTag fifoTag, u32 data, BOOL err) {
    OSIntrMode enabled = OS_DisableInterrupts();

    SNDi_CallAlarmHandler(data);
    OS_RestoreInterrupts(enabled);
}

// NitroSDK's InitPXI: sets the FIFO's callback for the sound tag and waits until the ARM7 has set its own
static void InitPXI(void) {
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_SOUND, PxiFifoCallback);
    if (IsCommandAvailable()) {
        while (!PXI_IsCallbackReady(PXI_FIFO_TAG_SOUND, PXI_PROC_ARM7)) {
            OS_SpinWaitSysCycles(SND_WAIT_CYCLES);
        }
    }
}

// Sends the ARM7 an empty word, to make it look at the packets
static void RequestCommandProc(void) {
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_SOUND, 0, FALSE) < 0) {
    }
}

// Takes a command from the pool, or returns NULL when there is none
static SNDCommand *AllocCommand(void) {
    OSIntrMode enabled = OS_DisableInterrupts();
    SNDCommand *command = SNDi_Queue.freeHead;

    if (command == NULL) {
        OS_RestoreInterrupts(enabled);
        return NULL;
    }
    SNDi_Queue.freeHead = command->next;
    if (SNDi_Queue.freeHead == NULL) {
        SNDi_Queue.freeTail = NULL;
    }
    OS_RestoreInterrupts(enabled);
    return command;
}

// Whether there is a sound driver to send commands to: always on a console, while on the Ensata emulator
// (OS_IsRunOnEmulator) it asks a register of the emulator
static BOOL IsCommandAvailable(void) {
    OSIntrMode enabled;
    u32 present;

    if (!OS_IsRunOnEmulator()) {
        return TRUE;
    }
    enabled = OS_DisableInterrupts();
    reg_SND_EMULATOR_DRIVER = SND_EMULATOR_QUERY;
    present = reg_SND_EMULATOR_DRIVER;
    OS_RestoreInterrupts(enabled);
    return present != 0;
}
