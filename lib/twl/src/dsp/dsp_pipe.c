#include "types.h"
#include "dsp_private.h"
#include "nitro/fs.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "twl/dsp.h"

// TwlSDK's DSP library, the pipes (guessed file name, after the SDK's dsp_pipe.c): the access to the DSP's data memory
// by bytes, the pipes that the ARM9 and the DSP's processes talk through, the interrupt that wakes the readers when the
// DSP writes one, and the file server thread that opens, reads and writes files for the DSP. ARM code in the DSi-only
// LTD autoload. The ROM names none of it; the functions have the SDK's names where twl/dsp.h knows them, and keep their
// default names otherwise.

// What the DSP asks of the file server, as the first half word of a message on a port that has the file flag
#define DSP_FILE_OPEN 0        // mode and a path (its length in half words, then the text): opens the file in the ROM
#define DSP_FILE_CLOSE 1       // closes it
#define DSP_FILE_SEEK 2        // offset and whence: moves, and answers with the new position
#define DSP_FILE_READ 3        // length: answers with the length it has, then the bytes
#define DSP_FILE_WRITE 4       // length, then the bytes
#define DSP_FILE_OPEN_MEMORY 5 // address and length: opens a file over memory

#define DSP_PORT_COUNT 8
#define DSP_FILE_CHUNK_SIZE 0x100
#define DSP_FILE_PATH_SIZE 0x108 // what the thread's stack frame holds for a path; the DSP's length is not checked
#define DSP_FILE_STACK_SIZE 0x1000
#define DSP_FILE_THREAD_PRIORITY 13

// The state of the file server: its thread, the requests waiting for it, and a file for each port
typedef struct DSPFileServer {
    BOOL started;
    OSThread thread;
    OSMessage message;
    OSMessageQueue queue;
    // the ports that have a request, a bit each: the DSP's interrupt sets them (func_02706b60) as the thread reads them
    volatile u32 requests;
    FSFile files[DSP_PORT_COUNT];
    u8 stack[DSP_FILE_STACK_SIZE];
} DSPFileServer;

static void func_02706530(u32 address, u8 value);
static void func_027067b4(void *arg);
static void func_02706b60(int port);
static void func_02706c2c(void *userData, int port, int direction);
static void func_02706cb4(DSPPipe *pipe);
static void func_02706dfc(DSPPipe *pipe);
static void func_02706e18(DSPPipe *pipe);

// The byte address in the DSP's memory of the pipe headers, which the DSP sends when it starts
static u32 sPipeBase;
static OSThreadQueue sPipeQueue; // the threads that wait for a pipe to change
// The callback and its user data of each port. The default for the first port is func_02706c2c
static DSPPipeCallback sPipeCallbacks[DSP_PORT_COUNT];
static void *sPipeUserData[DSP_PORT_COUNT];
static DSPPipe sPipes[DSP_PORT_COUNT][2]; // for the callers that pass no pipe to DSP_LoadPipe
static DSPFileServer sFileServer;

// The access to the DSP's memory, a half word of 16 bits at a time (its address is in half words)
static inline void DSPi_ReadWords(u16 *dst, u16 wordAddress, u16 count) {
    OSIntrMode irq = OS_DisableInterrupts();
    if (OS_IsRunOnTwl() == TRUE) {
        func_02705150(0, dst, wordAddress, count, 0);
    }
    reg_DSP_PCFG &= ~(DSP_PCFG_AUTO_INCREMENT | DSP_PCFG_READ_LENGTH_MASK);
    OS_RestoreInterrupts(irq);
}

static inline void DSPi_WriteWords(u16 wordAddress, const u16 *src, u16 count) {
    OSIntrMode irq = OS_DisableInterrupts();
    if (OS_IsRunOnTwl() == TRUE) {
        func_02705088(0, wordAddress, src, count, 0);
    }
    OS_RestoreInterrupts(irq);
}

static inline u16 DSPi_ReadWord(u16 wordAddress) {
    u16 word;
    DSPi_ReadWords(&word, wordAddress, 1);
    return word;
}

static inline void DSPi_WriteWord(u16 wordAddress, u16 word) {
    DSPi_WriteWords(wordAddress, &word, 1);
}

// Reads length bytes at a byte address in the DSP's memory
static void func_0270633c(u32 address, u8 *dst, u32 length) {
    if (length == 0) {
        return;
    }
    if (address & 1) {
        *dst++ = DSPi_ReadWord(address >> 1) >> ((address & 1) * 8);
        address++;
        length--;
    }
    if (((u32)dst & 1) == 0) {
        u32 even = length & ~1;
        if (even != 0) {
            DSPi_ReadWords((u16 *)dst, address >> 1, even >> 1);
            dst += even;
            address += even;
            length -= even;
        }
    } else {
        while (length >= 2) {
            u16 word = DSPi_ReadWord(address >> 1);
            dst[0] = word;
            dst[1] = word >> 8;
            length -= 2;
            dst += 2;
            address += 2;
        }
    }
    if (length != 0) {
        *dst = DSPi_ReadWord(address >> 1) >> ((address & 1) << 3);
    }
}

// Writes one byte at a byte address, by writing the half word it is in
static void func_02706530(u32 address, u8 value) {
    if ((address & 1) == 0) {
        u16 word = DSPi_ReadWord(address >> 1);
        DSPi_WriteWord(address >> 1, (word & 0xff00) | (value & 0xff));
    } else {
        u16 word = DSPi_ReadWord(address >> 1);
        DSPi_WriteWord(address >> 1, (word & 0xff) | ((value << 8) & 0xff00));
    }
}

// Writes length bytes at a byte address in the DSP's memory
static void func_02706694(u32 address, const u8 *src, u32 length) {
    if (length == 0) {
        return;
    }
    if (address & 1) {
        func_02706530(address, *src++);
        address++;
        length--;
    }
    if (((u32)src & 1) == 0) {
        u32 even = length & ~1;
        if (even != 0) {
            DSPi_WriteWords(address >> 1, (const u16 *)src, even >> 1);
            src += even;
            address += even;
            length -= even;
        }
    } else {
        while (length >= 2) {
            DSPi_WriteWord(address >> 1, src[0] | (src[1] << 8));
            length -= 2;
            src += 2;
            address += 2;
        }
    }
    if (length != 0) {
        func_02706530(address, *src);
    }
}

// The file server's thread: answers the requests that the DSP writes on its pipes, port by port
static void func_027067b4(void *arg) {
    DSPFileServer *server = arg;

    for (;;) {
        OSIntrMode irq = OS_DisableInterrupts();
        int port;
        OSMessage message;
        DSPPipe readPipe;
        DSPPipe writePipe;
        FSFile *file;

        for (;;) {
            u32 requests = server->requests;
            port = 32 - MATH_CountLeadingZeros(~requests & (requests - 1));
            if (port < DSP_PORT_COUNT) {
                break;
            }
            OS_ReceiveMessage(&server->queue, &message, OS_MESSAGE_BLOCK);
        }
        server->requests &= ~(1 << port);
        OS_RestoreInterrupts(irq);

        file = &server->files[port];
        DSP_LoadPipe(&readPipe, port, 0);
        DSP_LoadPipe(&writePipe, port, 1);
        while (DSP_GetPipeReadableSize(&readPipe) >= 2) {
            u16 command;
            DSP_ReadPipe(&readPipe, &command, 2);
            switch (command) {
            case DSP_FILE_OPEN: {
                u16 mode;
                u16 length;
                u16 result;
                char path[DSP_FILE_PATH_SIZE];
                DSP_ReadPipe(&readPipe, &mode, 2);
                DSP_ReadPipe(&readPipe, &length, 2);
                length *= 2;
                DSP_ReadPipe(&readPipe, path, length);
                path[length] = '\0';
                FS_OpenFileEx(file, path, mode);
                result = FS_IsFile(file) ? TRUE : FALSE;
                DSP_WritePipe(&writePipe, &result, 2);
                break;
            }
            case DSP_FILE_OPEN_MEMORY: {
                u32 address;
                u32 length;
                u16 result;
                DSP_ReadPipe(&readPipe, &address, 4);
                DSP_ReadPipe(&readPipe, &length, 4);
                address = DSPi_SwapHalves(address);
                length = DSPi_SwapHalves(length) * 2;
                FS_CreateFileFromMemory(file, (void *)address, length);
                result = FS_IsFile(file) ? TRUE : FALSE;
                DSP_WritePipe(&writePipe, &result, 2);
                break;
            }
            case DSP_FILE_CLOSE: {
                u16 result;
                FS_CloseFile(file);
                result = TRUE;
                DSP_WritePipe(&writePipe, &result, 2);
                break;
            }
            case DSP_FILE_SEEK: {
                u32 offset;
                u16 whence;
                u32 position;
                DSP_ReadPipe(&readPipe, &offset, 4);
                DSP_ReadPipe(&readPipe, &whence, 2);
                offset = DSPi_SwapHalves(offset);
                FS_SeekFile(file, offset, whence);
                position = DSPi_SwapHalves(FS_GetFilePosition(file));
                DSP_WritePipe(&writePipe, &position, 4);
                break;
            }
            case DSP_FILE_READ: {
                u16 length;
                u16 words;
                u8 buffer[DSP_FILE_CHUNK_SIZE];
                DSP_ReadPipe(&readPipe, &length, 2);
                length *= 2;
                {
                    u32 left = FS_GetFileLength(file) - FS_GetFilePosition(file);
                    u32 size = length;
                    if (size > left) {
                        size = left;
                    }
                    length = size;
                }
                words = length >> 1;
                DSP_WritePipe(&writePipe, &words, 2);
                while (length != 0) {
                    u16 chunk = (length > DSP_FILE_CHUNK_SIZE) ? DSP_FILE_CHUNK_SIZE : length;
                    FS_ReadFile(file, buffer, chunk);
                    DSP_WritePipe(&writePipe, buffer, chunk);
                    length -= chunk;
                }
                break;
            }
            case DSP_FILE_WRITE: {
                u16 length;
                u8 buffer[DSP_FILE_CHUNK_SIZE];
                DSP_ReadPipe(&readPipe, &length, 2);
                length *= 2;
                while (length != 0) {
                    u16 chunk = (length > DSP_FILE_CHUNK_SIZE) ? DSP_FILE_CHUNK_SIZE : length;
                    DSP_ReadPipe(&readPipe, buffer, chunk);
                    FS_WriteFile(file, buffer, chunk);
                    length -= chunk;
                }
                break;
            }
            }
        }
    }
}

// Asks the file server to look at the pipes of a port, and starts it if it is not running yet
static void func_02706b60(int port) {
    DSPFileServer *server = &sFileServer;
    OSIntrMode irq = OS_DisableInterrupts();

    if (!server->started) {
        int i;
        for (i = 0; i < DSP_PORT_COUNT; i++) {
            FS_InitFile(&server->files[i]);
        }
        OS_InitMessageQueue(&server->queue, &server->message, 1);
        server->requests = 0;
        server->started = TRUE;
        OS_CreateThread(&server->thread, func_027067b4, server, server->stack + DSP_FILE_STACK_SIZE,
                        DSP_FILE_STACK_SIZE, DSP_FILE_THREAD_PRIORITY);
        OS_WakeupThreadDirect(&server->thread);
    }
    server->requests |= 1 << port;
    OS_SendMessage(&server->queue, (OSMessage)port, OS_MESSAGE_NOBLOCK);
    OS_RestoreInterrupts(irq);
}

// The callback of the first port: reads what the DSP wrote there, and drops it
static void func_02706c2c(void *userData, int port, int direction) {
    DSPPipe pipe;
    u8 buffer[0x80];
    u16 length;
    u16 done;

    if (direction != 0) {
        return;
    }
    DSP_LoadPipe(&pipe, port, direction);
    length = DSP_GetPipeReadableSize(&pipe);
    done = 0;
    if (length == 0) {
        return;
    }
    while (done < length) {
        u16 chunk = (length - done >= 0x80) ? 0x80 : length - done;
        DSP_ReadPipe(&pipe, buffer, chunk);
        done += chunk;
    }
}

// Waits for the DSP to change a pipe, and gets its header again
static void func_02706cb4(DSPPipe *pipe) {
    OS_SleepThread(&sPipeQueue);
    func_02706dfc(pipe);
}

void DSP_InitPipe(void) {
    int i;

    sPipeBase = 0;
    OS_InitThreadQueue(&sPipeQueue);
    for (i = 0; i < DSP_PORT_COUNT; i++) {
        sPipeCallbacks[i] = NULL;
        sPipeUserData[i] = NULL;
    }
    sPipeCallbacks[0] = func_02706c2c;
}

void DSP_SetPipeCallback(int port, DSPPipeCallback callback, void *userData) {
    sPipeCallbacks[port] = callback;
    sPipeUserData[port] = userData;
}

DSPPipe *DSP_LoadPipe(DSPPipe *pipe, int port, int direction) {
    OSIntrMode irq = OS_DisableInterrupts();
    while (sPipeBase == 0) {
        OS_SleepThread(&sPipeQueue);
    }
    OS_RestoreInterrupts(irq);
    if (port >= 0 && port < DSP_PORT_COUNT && direction >= 0 && direction < 2) {
        // the DSP keeps the headers at sPipeBase in its memory, in a table laid out like sPipes
        DSPPipe(*headers)[2] = (DSPPipe(*)[2])sPipeBase;
        u32 address = (u32)&headers[port][direction];
        if (pipe == NULL) {
            pipe = &sPipes[port][direction];
        }
        func_0270633c(address, (u8 *)pipe, sizeof(DSPPipe));
    }
    return pipe;
}

// Gets the header of a pipe from the DSP again
static void func_02706dfc(DSPPipe *pipe) {
    DSP_LoadPipe(pipe, (pipe->id & 0xff) >> 1, pipe->id & 1);
}

// Tells the DSP that the ARM9 moved the pipe's read or write position
static void func_02706e18(DSPPipe *pipe) {
    u32 address = sPipeBase + ((pipe->id & 0xff) >> 1) * sizeof(sPipes[0]) + (pipe->id & 1) * sizeof(DSPPipe);

    if ((pipe->id & 1) == 0) {
        func_02706694(address + 4, (u8 *)&pipe->readPos, 2);
    } else {
        func_02706694(address + 6, (u8 *)&pipe->writePos, 2);
    }
    DSP_SendData(2, pipe->id & 0xff);
}

u16 DSP_GetPipeReadableSize(const DSPPipe *pipe) {
    u16 writePos = pipe->writePos;
    u16 readPos = pipe->readPos;
    return (writePos - readPos + (((readPos ^ writePos) < 0x8000) ? 0 : pipe->length)) & ~0x8000;
}

void DSP_ReadPipe(DSPPipe *pipe, void *dst, u16 length) {
    OSIntrMode irq = OS_DisableInterrupts();
    BOOL moved = FALSE;

    func_02706dfc(pipe);
    while (length != 0) {
        u16 writePos = pipe->writePos;
        u16 readPos = pipe->readPos;
        u16 difference = readPos ^ writePos;
        if (difference == 0) {
            if (moved) {
                func_02706e18(pipe);
                moved = FALSE;
            }
            func_02706cb4(pipe);
        } else {
            u16 offset = readPos & ~0x8000;
            u16 end = (difference < 0x8000) ? (writePos & ~0x8000) : pipe->length;
            u16 count = (length < end - offset) ? length : end - offset;
            count = count & ~(DSP_MEMORY_UNIT - 1);
            func_0270633c(pipe->base * 2 + offset, dst, count);
            length -= count;
            dst = (u8 *)dst + count;
            pipe->readPos = (offset + count < pipe->length) ? readPos + count : ~readPos & 0x8000;
            moved = TRUE;
        }
    }
    if (moved) {
        func_02706e18(pipe);
    }
    OS_RestoreInterrupts(irq);
}

void DSP_WritePipe(DSPPipe *pipe, const void *src, u16 length) {
    OSIntrMode irq = OS_DisableInterrupts();
    BOOL moved = FALSE;

    func_02706dfc(pipe);
    while (length != 0) {
        u16 writePos = pipe->writePos;
        u16 readPos = pipe->readPos;
        u16 difference = readPos ^ writePos;
        if (difference == 0x8000) {
            if (moved) {
                func_02706e18(pipe);
                moved = FALSE;
            }
            func_02706cb4(pipe);
        } else {
            u16 offset = writePos & ~0x8000;
            u16 end = (difference < 0x8000) ? pipe->length : (readPos & ~0x8000);
            u16 count = (length < end - offset) ? length : end - offset;
            count = count & ~(DSP_MEMORY_UNIT - 1);
            func_02706694(pipe->base * 2 + offset, src, count);
            length -= count;
            src = (const u8 *)src + count;
            pipe->writePos = (offset + count < pipe->length) ? writePos + count : ~writePos & 0x8000;
            moved = TRUE;
        }
    }
    if (moved) {
        func_02706e18(pipe);
    }
    OS_RestoreInterrupts(irq);
}

// The DSP's interrupt: the first word the DSP sends on the third reply is the pipes' address, and each after it is a
// port and a direction that changed, which goes to the port's callback, or to the file server if it has the file flag
void func_027070dc(void *userData) {
    while (DSP_GetSemaphore() & 0x8000 || DSP_RecvDataIsReady(2)) {
        DSP_ClearSemaphore(0x8000);
        while (DSP_RecvDataIsReady(2)) {
            if (sPipeBase == 0) {
                sPipeBase = DSP_RecvData(2) << 1;
            } else {
                int message = DSP_RecvData(2);
                int port = message >> 1;
                int direction = message & 1;
                if (port >= 0 && port < DSP_PORT_COUNT) {
                    if (sPipeCallbacks[port] != NULL) {
                        sPipeCallbacks[port](sPipeUserData[port], port, direction);
                    } else {
                        DSPPipe pipe;
                        DSP_LoadPipe(&pipe, port, direction);
                        if (direction == 0 && (pipe.id & DSP_PIPE_FILE_FLAG)) {
                            func_02706b60(port);
                        }
                    }
                }
            }
            OS_WakeupThread(&sPipeQueue);
        }
    }
}
