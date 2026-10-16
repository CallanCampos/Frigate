#ifndef POKEBW2_TWL_DSP_H
#define POKEBW2_TWL_DSP_H

#include "types.h"
#include "nitro/fs.h"
#include "nitro/os.h"
#include "twl/mi.h"
#include "twl/pm.h"

// TwlSDK's DSP library, in the DSi's LTD autoload, which plays the camera's shutter and video sounds through the
// G.711 component. As with the camera, the SDK's inline functions only call it on a DSi, and its functions keep their
// default names until the LTD autoload is analyzed.

void func_027062c8(void);                               // stops the sound playing
BOOL func_02706174(const void *src, u32 length);        // DSP_PlayShutterSoundCore
BOOL func_02706324(void);                               // DSP_IsShutterSoundPlayingCore
BOOL func_0270723c(FSFile *file);                       // DSP_OpenStaticComponentG711Core
BOOL func_02707278(FSFile *file, int slotB, int slotC); // DSP_LoadG711Core
void func_027072e0(void);                               // DSP_UnloadG711Core

// The COFF images of the G.711 and the graphics component, in the LTD autoload's .rodata after its code, which no
// source file owns yet. The openers hand them to the file system as memory files with this size, more than they are
#define DSP_COMPONENT_IMAGE_SIZE 0x1000000
extern const u8 DSP_G711Image[];
extern const u8 DSP_GraphicsImage[];

static inline void DSP_StopSound(void) {
    if (hw_isDSi()) {
        func_027062c8();
    }
}

static inline BOOL DSP_PlayShutterSound(const void *src, u32 length) {
    if (hw_isDSi()) {
        return func_02706174(src, length);
    }
    return FALSE;
}

static inline BOOL DSP_IsShutterSoundPlaying(void) {
    BOOL playing = FALSE;
    if (hw_isDSi()) {
        playing = func_02706324();
    }
    return playing;
}

static inline BOOL DSP_OpenStaticComponentG711(FSFile *file) {
    if (hw_isDSi()) {
        return func_0270723c(file);
    }
    return FALSE;
}

static inline BOOL DSP_LoadG711(FSFile *file, int slotB, int slotC) {
    if (hw_isDSi()) {
        return func_02707278(file, slotB, slotC);
    }
    return FALSE;
}

static inline void DSP_UnloadG711(void) {
    if (hw_isDSi()) {
        func_027072e0();
    }
}

// The graphics component, which the minigames' camera uses to scale its pictures
BOOL func_02707310(FSFile *file);                       // DSP_OpenStaticComponentGraphicsCore
BOOL func_027073b8(FSFile *file, int slotB, int slotC); // DSP_LoadGraphicsCore
void func_02707464(void);                               // DSP_UnloadGraphicsCore
// DSPi_ScalingCore: scales the area at (x, y) of a width by height image by rx and ry. If async, returns TRUE at once
// and calls callback when the DSP is done; if not, waits for the DSP and returns whether it scaled the image. Returns
// FALSE when a scaling is in progress
BOOL func_02707494(const void *src, void *dst, u16 imageWidth, u16 imageHeight, f32 rx, f32 ry, u16 mode, u16 x, u16 y,
                   u16 width, u16 height, void (*callback)(void), BOOL async);

static inline BOOL DSP_OpenStaticComponentGraphics(FSFile *file) {
    if (hw_isDSi()) {
        return func_02707310(file);
    }
    return FALSE;
}

static inline BOOL DSP_LoadGraphics(FSFile *file, int slotB, int slotC) {
    if (hw_isDSi() == TRUE) {
        return func_027073b8(file, slotB, slotC);
    }
    return FALSE;
}

static inline void DSP_UnloadGraphics(void) {
    if (hw_isDSi()) {
        func_02707464();
    }
}

// The DSP's register interface (dsp_if.c). The SDK's names, REG_SCFG_* and REG_DSP_* in its headers, are written
// reg_* here as elsewhere in the library.

// SCFG's switches for the DSP, its clock and its reset line, are in twl/scfg.h

#define reg_DSP_PDATA (*(vu16 *)0x04004300) // the data port of the DSP's memory
#define reg_DSP_PADR (*(vu16 *)0x04004304)  // the word address in it
#define reg_DSP_PCFG (*(vu16 *)0x04004308)  // the port's configuration
#define reg_DSP_PSTS (*(vu16 *)0x0400430c)  // the port's status
#define reg_DSP_PSEM (*(vu16 *)0x04004310)  // the semaphore towards the DSP
#define reg_DSP_PMASK (*(vu16 *)0x04004314) // which of the DSP's semaphore bits interrupt
#define reg_DSP_PCLEAR (*(vu16 *)0x04004318)
#define reg_DSP_SEM (*(vu16 *)0x0400431c) // the semaphore from the DSP
// The three command and reply registers, in pairs 8 bytes apart
#define reg_DSP_CMD(n) (*(vu16 *)(0x04004320 + (n) * 8))
#define reg_DSP_REP(n) (*(vu16 *)(0x04004324 + (n) * 8))

#define DSP_PCFG_RESET 0x0001
#define DSP_PCFG_AUTO_INCREMENT 0x0002 // the address in PADR moves on after each word
#define DSP_PCFG_READ_LENGTH_SHIFT 2   // how many words a read fills the FIFO with
#define DSP_PCFG_READ_LENGTH_MASK 0x000c
#define DSP_PCFG_READ_1 (0 << DSP_PCFG_READ_LENGTH_SHIFT)
#define DSP_PCFG_READ_8 (1 << DSP_PCFG_READ_LENGTH_SHIFT)
#define DSP_PCFG_READ_16 (2 << DSP_PCFG_READ_LENGTH_SHIFT)
#define DSP_PCFG_READ_CONTINUOUS (3 << DSP_PCFG_READ_LENGTH_SHIFT)
#define DSP_PCFG_READ_START 0x0010
#define DSP_PCFG_REPLY_IRQ_MASK 0x0e00 // the interrupts of the three replies
#define DSP_PCFG_REPLY_IRQ_SHIFT 9
#define DSP_PCFG_MEMORY_MASK 0xf000 // which of the DSP's memories PADR addresses

#define DSP_PSTS_RESETTING 0x0004  // while the DSP's reset is changing
#define DSP_PSTS_READ_READY 0x0040 // PDATA has a word to read
#define DSP_PSTS_WRITE_FULL 0x0080 // PDATA takes no word yet
#define DSP_PSTS_REPLY_SHIFT 10    // reply n has data, on bit n of three
#define DSP_PSTS_COMMAND_SHIFT 13  // command n was not taken yet, on bit n of three

// dsp_if.c. TwlSDK's names are in the comments
void func_02704e90(void);                // DSP_PowerOn: starts the DSP's clock and releases its reset line
void func_02704ee8(void);                // DSP_PowerOff
void func_02704f1c(void);                // DSP_ResetOn: puts the DSP in reset
void func_02704f4c(u16 mask);            // DSP_ResetOffEx: takes it out, with the reply interrupts of mask
BOOL func_02705008(int index);           // DSP_RecvDataIsReady: reply index has data
void func_0270502c(int index, u16 data); // DSP_SendData: waits for command index to be free, then writes it
u16 func_0270505c(int index);            // DSP_RecvData: waits for reply index and reads it
// The SDK's inline functions over the three, which only call them on a DSi
static inline void DSPi_SendData(int index, u16 data) {
    if (hw_isDSi() == TRUE) {
        func_0270502c(index, data);
    }
}

static inline BOOL DSPi_RecvDataIsReady(int index) {
    if (hw_isDSi() == TRUE) {
        return func_02705008(index);
    }
    return FALSE;
}

static inline u16 DSPi_RecvData(int index) {
    if (hw_isDSi() == TRUE) {
        return func_0270505c(index);
    }
    return 0;
}
// Writes count words at the DSP's address dspAddr, memory being the memory's PCFG bits
#define DSP_WRITE_FIXED_SOURCE 0x1  // writes the first word count times
#define DSP_WRITE_FIXED_ADDRESS 0x2 // does not move the DSP's address
void func_02705088(u16 memory, u16 dspAddr, const u16 *src, int count, u16 flags);
// Reads count words from the DSP's address dspAddr
#define DSP_READ_FIXED_ADDRESS 0x1     // does not move the DSP's address
#define DSP_READ_FIXED_DESTINATION 0x2 // stores every word in *dst
#define DSP_READ_BURST_MASK 0x300      // reads 1, 8 or 16 words (instead of count) in one burst of that length
#define DSP_READ_BURST_1 0x100
#define DSP_READ_BURST_8 0x200
#define DSP_READ_BURST_16 0x300
void func_02705150(u16 memory, u16 *dst, u16 dspAddr, int count, u16 flags);
u16 func_02705284(void);      // DSP_GetSemaphore
void func_02705294(u16 mask); // DSP_ClearSemaphore
void func_027052a4(u16 mask); // DSP_MaskSemaphore

// dsp_process.c: a process is a COFF image that the DSP runs. Its program goes to the shared WRAM B and its data to
// WRAM C, in banks of 32 KB that the ARM9 gives it from the ones its context allows, and its events (the 16 bits of the
// semaphore from the DSP and its 3 replies) call the callbacks of its context. The SDK's names are in the comments.
#define DSP_EVENT_COUNT 19
#define DSP_EVENT_REPLY_SHIFT 16 // reply n is event 16 + n

typedef void (*DSPCallback)(void *userData);

// The image has the section of the DSP's OS, which answers on each of the 3 ports when the process starts
#define DSP_PROCESS_USING_OS 0x1

typedef struct DSPProcessContext {
    struct DSPProcessContext *next; // the list of running processes
    char name[16];
    FSFile *image;  // the COFF file, while it loads
    u16 freeBanksB; // the banks of WRAM B that the process can still take, a bit each
    u16 freeBanksC; // and of WRAM C
    u32 flags;      // DSP_PROCESS_*
    u32 usedSlotsB; // the 32 KB slots of its program memory that have a bank
    u32 usedSlotsC; // and of its data memory
    int bankB[8];   // the bank of WRAM B behind each slot, or -1
    int bankC[8];   // and of WRAM C
    int events;     // the events that have a callback
    DSPCallback callback[DSP_EVENT_COUNT];
    void *userData[DSP_EVENT_COUNT];
    int group[DSP_EVENT_COUNT]; // the events set along with each one, which its callback takes together
} DSPProcessContext;

// DSP_InitProcessContext
void func_027058d8(DSPProcessContext *context, const char *name);
// DSP_SetCallback: calls callback for each of the events (NULL stops that)
void func_02705b70(DSPProcessContext *context, int events, DSPCallback callback, void *userData);
// DSP_ExecuteProcess: loads the image into the banks given by banksB and banksC, one bit each, and starts the DSP
BOOL func_02705c24(DSPProcessContext *context, FSFile *image, int banksB, int banksC);
// DSP_QuitProcess
void func_02705e04(DSPProcessContext *context);
// DSP_FindProcess: the running process of that name, or the first one if name is NULL
DSPProcessContext *func_02705ed0(const char *name);
// DSP_WriteProcessPipe: writes length bytes of src to the pipe of that port (the context is not used)
void func_02705f28(DSPProcessContext *context, int port, const void *src, u32 length);

// dsp_pipe.c: the pipes between the ARM9 and the DSP, ring buffers in the DSP's memory that each have a header of five
// half words, which the DSP shares. A port has two pipes, 0 for what the DSP writes and 1 for what the ARM9 writes,
// and their headers are 0x14 bytes apart for the ports, from the address that the DSP sends when it starts. The
// positions are bytes, with a bit 0x8000 that flips each time one wraps around: equal positions are an empty pipe and
// positions that differ in the bit only, a full one.
typedef struct DSPPipe {
    u16 base;     // the ring buffer's address in the DSP's memory, in half words
    u16 length;   // its length in bytes
    u16 readPos;  // where the reader is
    u16 writePos; // where the writer is
    u16 id;       // port * 2 + direction in the low byte, and the flags
} DSPPipe;

// A flag of a pipe's id: the DSP writes to this port requests for the files (func_02706b60's thread answers them)
#define DSP_PIPE_FILE_FLAG 0x100

// What a pipe's events call: the user data of func_02706d28, the port and the direction
typedef void (*DSPPipeCallback)(void *userData, int port, int direction);

// TwlSDK's names are in the comments. The ports are 0 to 7, and the first has a callback of its own at the start
void func_02706cd4(void); // DSP_InitPipe: forgets the pipes' address and the callbacks, and drops what port 0 gets
// DSP_SetPipeCallback: calls callback for the pipes' events of a port, with userData (NULL stops that)
void func_02706d28(int port, DSPPipeCallback callback, void *userData);
// DSP_LoadPipe: reads the header of the pipe of a port and direction from the DSP into pipe, or into a scratch copy if
// pipe is NULL, and returns it. Waits until the DSP has sent the pipes' address. A port or direction out of range
// leaves pipe alone
DSPPipe *func_02706d44(DSPPipe *pipe, int port, int direction);
// DSP_GetPipeReadableSize: how many bytes the pipe has to read, by the header that was loaded
u16 func_02706e90(const DSPPipe *pipe);
// DSP_ReadPipe: reads length bytes of the pipe into dst, waiting for the DSP when it is empty
void func_02706ec0(DSPPipe *pipe, void *dst, u16 length);
// DSP_WritePipe: writes length bytes of src to the pipe, waiting for the DSP when it is full
void func_02706fcc(DSPPipe *pipe, const void *src, u16 length);
// The callback of the DSP's events (DSPCallback): hands each change of a pipe to its port's callback, and to the file
// server thread when the pipe has the file flag
void func_027070dc(void *userData);

#endif // POKEBW2_TWL_DSP_H
