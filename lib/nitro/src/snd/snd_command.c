// NitroSDK's SND (snd_command.c, a guess from the SDK's file names): the queue of commands for the ARM7's sound driver.
// A command is taken from a pool of 256, filled in and pushed on the list of ready commands; flushing sends the list as
// one packet through the PXI FIFO, and when the ARM7 is done with a packet the commands go back to the pool.
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
static SNDCommand sCommands[SND_COMMAND_NUM] ATTRIBUTE_ALIGN(32);
static SNDSharedWork sSharedWork ATTRIBUTE_ALIGN(32);

static int func_0207da0c(void);
static int func_0207da30(void);
static int func_0207da54(void);
static void func_0207da6c(u32 fifoTag, u32 data, BOOL err);
static void func_0207da84(void);
static void func_0207dac0(void);
static SNDCommand *func_0207dad8(void);
static BOOL func_0207db08(void);

// NitroSDK's SND_CommandInit
void func_0207d6b8(void) {
    int i;
    SNDCommand *command;

    func_0207da84();
    SNDi_Queue.freeHead = sCommands;
    for (i = 0; i < SND_COMMAND_NUM - 1; i++) {
        sCommands[i].next = &sCommands[i + 1];
    }
    sCommands[SND_COMMAND_NUM - 1].next = NULL;
    SNDi_Queue.freeTail = &sCommands[SND_COMMAND_NUM - 1];
    SNDi_Queue.readyHead = NULL;
    SNDi_Queue.readyTail = NULL;
    SNDi_Queue.sentCount = 0;
    SNDi_Queue.sentRead = 0;
    SNDi_Queue.sentWrite = 0;
    SNDi_Queue.nextTag = 1;
    SNDi_Queue.receivedTag = 0;
    SNDi_SharedWork = &sSharedWork;
    func_0207dcdc(SNDi_SharedWork);

    // Tell the ARM7 where the shared work is
    command = sndNewPacket(SND_COMMAND_BLOCK);
    if (command) {
        command->id = SND_COMMAND_SHARED_WORK;
        command->arg[0] = (u32)SNDi_SharedWork;
        sndQueuePacket_(command);
        func_0207d864(SND_COMMAND_BLOCK);
    }
}

// NitroSDK's SND_RecvCommandReply: takes back the next packet that the ARM7 is done with and returns its commands, or
// returns NULL when there is none. With SND_COMMAND_BLOCK it waits for one
SNDCommand *func_0207d73c(u32 flags) {
    u32 enabled = CPU_IRQDisable();
    SNDCommand *command;
    SNDCommand *last;

    if (flags & SND_COMMAND_BLOCK) {
        if (SNDi_Queue.receivedTag == func_0207dcc4()) {
            do {
                CPU_SetIRQMask(enabled);
                func_0207c160(SND_WAIT_CYCLES);
                enabled = CPU_IRQDisable();
            } while (SNDi_Queue.receivedTag == func_0207dcc4());
        }
    } else if (SNDi_Queue.receivedTag == func_0207dcc4()) {
        CPU_SetIRQMask(enabled);
        return NULL;
    }

    command = SNDi_Sent[SNDi_Queue.sentRead];
    SNDi_Queue.sentRead++;
    if (SNDi_Queue.sentRead > SND_PACKET_MAX) {
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
    CPU_SetIRQMask(enabled);
    return command;
}

// NitroSDK's SND_AllocCommand: takes a command from the pool. When the pool is empty it returns NULL, or with
// SND_COMMAND_BLOCK it waits until the ARM7 returns some
SNDCommand *sndNewPacket(u32 flags) {
    SNDCommand *command;

    if (!func_0207db08()) {
        return NULL;
    }
    command = func_0207dad8();
    if (command != NULL) {
        return command;
    }
    if (!(flags & SND_COMMAND_BLOCK)) {
        return NULL;
    }

    if (func_0207da54() > 0) {
        // Packets are on their way: take back what is there
        while (func_0207d73c(SND_COMMAND_NOBLOCK) != NULL) {
        }
        command = func_0207dad8();
        if (command != NULL) {
            return command;
        }
    } else {
        // Everything is waiting to be sent
        func_0207d864(SND_COMMAND_BLOCK);
    }
    func_0207dac0();
    do {
        func_0207d73c(SND_COMMAND_BLOCK);
        command = func_0207dad8();
    } while (command == NULL);
    return command;
}

// NitroSDK's SND_PushCommand: adds a command to the ready list
void sndQueuePacket_(SNDCommand *command) {
    u32 enabled = CPU_IRQDisable();

    if (SNDi_Queue.readyTail == NULL) {
        SNDi_Queue.readyHead = command;
        SNDi_Queue.readyTail = command;
    } else {
        SNDi_Queue.readyTail->next = command;
        SNDi_Queue.readyTail = command;
    }
    command->next = NULL;
    CPU_SetIRQMask(enabled);
}

// NitroSDK's SND_FlushCommand: sends the ready commands as a packet. It returns FALSE when it could not, because no
// packet could be sent without waiting and SND_COMMAND_BLOCK is not set. SND_COMMAND_IMMEDIATE wakes the ARM7 up
BOOL func_0207d864(u32 flags) {
    u32 enabled = CPU_IRQDisable();

    if (!SNDi_Queue.readyHead) {
        CPU_SetIRQMask(enabled);
        return TRUE;
    }

    if (SNDi_Queue.sentCount >= SND_PACKET_MAX) {
        if (!(flags & SND_COMMAND_BLOCK)) {
            CPU_SetIRQMask(enabled);
            return FALSE;
        }
        do {
            func_0207d73c(SND_COMMAND_BLOCK);
        } while (SNDi_Queue.sentCount >= SND_PACKET_MAX);
        if (!SNDi_Queue.readyHead) {
            CPU_SetIRQMask(enabled);
            return TRUE;
        }
    }

    cp15_flushDC(sCommands, sizeof(sCommands));
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_SOUND, (u32)SNDi_Queue.readyHead, FALSE) < 0) {
        if (!(flags & SND_COMMAND_BLOCK)) {
            CPU_SetIRQMask(enabled);
            return FALSE;
        }
        while (SNDi_Queue.sentCount >= SND_PACKET_MAX ||
               PXI_SendWordByFifo(PXI_FIFO_TAG_SOUND, (u32)SNDi_Queue.readyHead, FALSE) < 0) {
            CPU_SetIRQMask(enabled);
            func_0207d73c(SND_COMMAND_NOBLOCK);
            enabled = CPU_IRQDisable();
            cp15_flushDC(sCommands, sizeof(sCommands));
            if (!SNDi_Queue.readyHead) {
                CPU_SetIRQMask(enabled);
                return TRUE;
            }
        }
    }

    SNDi_Sent[SNDi_Queue.sentWrite] = SNDi_Queue.readyHead;
    SNDi_Queue.sentWrite++;
    if (SNDi_Queue.sentWrite > SND_PACKET_MAX) {
        SNDi_Queue.sentWrite = 0;
    }
    SNDi_Queue.readyHead = NULL;
    SNDi_Queue.readyTail = NULL;
    SNDi_Queue.sentCount++;
    SNDi_Queue.nextTag++;
    CPU_SetIRQMask(enabled);

    if (flags & SND_COMMAND_IMMEDIATE) {
        func_0207dac0();
    }
    return TRUE;
}

// NitroSDK's SND_WaitForCommandProc: waits until the ARM7 is done with the packet that has the tag
void func_0207d96c(u32 tag) {
    if (func_0207d9d0(tag)) {
        return;
    }
    while (func_0207d73c(SND_COMMAND_NOBLOCK) != NULL) {
    }
    if (func_0207d9d0(tag)) {
        return;
    }
    func_0207dac0();
    if (func_0207d9d0(tag)) {
        return;
    }
    do {
        func_0207d73c(SND_COMMAND_BLOCK);
    } while (!func_0207d9d0(tag));
}

// swan's name for SND_GetCurrentCommandTag: the tag that SND_IsFinishedCommandTag tells for the commands pushed last,
// which is the next packet's while commands wait to be flushed, and else the last packet taken back
u32 sndGetSentPacketCount(void) {
    u32 enabled = CPU_IRQDisable();
    u32 tag;

    if (SNDi_Queue.readyHead == NULL) {
        tag = SNDi_Queue.receivedTag;
    } else {
        tag = SNDi_Queue.nextTag;
    }
    CPU_SetIRQMask(enabled);
    return tag;
}

// NitroSDK's SND_IsFinishedCommandTag: whether the packet with the tag has come back. The tags wrap around
BOOL func_0207d9d0(u32 tag) {
    u32 enabled = CPU_IRQDisable();
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
    CPU_SetIRQMask(enabled);
    return finished;
}

// NitroSDK's SND_CountFreeCommand
static int func_0207da0c(void) {
    u32 enabled = CPU_IRQDisable();
    int count = 0;
    SNDCommand *command;

    for (command = SNDi_Queue.freeHead; command != NULL; command = command->next) {
        count++;
    }
    CPU_SetIRQMask(enabled);
    return count;
}

// NitroSDK's SND_CountReadyCommand
static int func_0207da30(void) {
    u32 enabled = CPU_IRQDisable();
    int count = 0;
    SNDCommand *command;

    for (command = SNDi_Queue.readyHead; command != NULL; command = command->next) {
        count++;
    }
    CPU_SetIRQMask(enabled);
    return count;
}

// NitroSDK's SND_CountWaitingCommand: the commands that are in neither the pool nor the ready list, so with the ARM7
static int func_0207da54(void) {
    return SND_COMMAND_NUM - func_0207da0c() - func_0207da30();
}

// The function that the PXI FIFO calls with the words that the ARM7 sends for the sound tag: the number of an alarm and
// its ID, which are the alarm that went off. NitroSDK's SNDi_PxiFifoCallback (a guess)
static void func_0207da6c(u32 fifoTag, u32 data, BOOL err) {
    u32 enabled = CPU_IRQDisable();

    func_0207db8c(data);
    CPU_SetIRQMask(enabled);
}

// NitroSDK's SNDi_InitPxi: sets the FIFO's callback for the sound tag and waits until the ARM7 has set its own
static void func_0207da84(void) {
    func_0207c9fc(PXI_FIFO_TAG_SOUND, func_0207da6c);
    if (func_0207db08()) {
        while (!func_0207ca48(PXI_FIFO_TAG_SOUND, MI_PROCESSOR_ARM7)) {
            func_0207c160(SND_WAIT_CYCLES);
        }
    }
}

// Sends the ARM7 an empty word, to make it look at the packets
static void func_0207dac0(void) {
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_SOUND, 0, FALSE) < 0) {
    }
}

// Takes a command from the pool, or returns NULL when there is none
static SNDCommand *func_0207dad8(void) {
    u32 enabled = CPU_IRQDisable();
    SNDCommand *command = SNDi_Queue.freeHead;

    if (command == NULL) {
        CPU_SetIRQMask(enabled);
        return NULL;
    }
    SNDi_Queue.freeHead = command->next;
    if (SNDi_Queue.freeHead == NULL) {
        SNDi_Queue.freeTail = NULL;
    }
    CPU_SetIRQMask(enabled);
    return command;
}

// Whether there is a sound driver to send commands to: always on a console, while on the Ensata emulator
// (OS_IsRunOnEmulator) it asks a register of the emulator
static BOOL func_0207db08(void) {
    u32 enabled;
    u32 present;

    if (!func_0207ac74()) {
        return TRUE;
    }
    enabled = CPU_IRQDisable();
    reg_SND_EMULATOR_DRIVER = SND_EMULATOR_QUERY;
    present = reg_SND_EMULATOR_DRIVER;
    CPU_SetIRQMask(enabled);
    return present != 0;
}
