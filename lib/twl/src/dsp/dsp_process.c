#include "types.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "nitro/std.h"
#include "twl/dsp.h"

// TwlSDK's DSP library, the DSP's processes (guessed file name, after the SDK's dsp_process.c): a context for each COFF
// image, which this file loads into the shared WRAM B and C and hands to the DSP, the interrupt that calls the
// context's callbacks, and the list of the running processes. ARM code in the DSi-only LTD autoload. The ROM names none
// of it; the functions have the SDK's names where twl/dsp.h knows them, and keep their default names otherwise.

// The COFF image's file header and a section header
typedef struct COFFHeader {
    u16 magic;
    u16 sectionCount;
    u32 timeStamp;
    u32 symbolTable;
    u32 symbolCount;
    u16 optionalSize;
    u16 flags;
} COFFHeader;

typedef struct COFFSection {
    union {
        char chars[8];
        struct {
            u32 zeroes; // 0 when the name is in the string table
            u32 offset; // and this is where
        } longName;
    } name;
    u32 programAddress;
    u32 dataAddress;
    u32 size;
    u32 dataOffset;
    u32 relocOffset;
    u32 lineOffset;
    u16 relocCount;
    u16 lineCount;
    u32 flags;
} COFFSection;

#define COFF_SYMBOL_SIZE 18

// What goes where: the program in WRAM B and the data in WRAM C, in the halves of the DSP's memory that the name ends
// with (@c0 and @c1 for the program, @d0 and @d1 for the data)
#define COFF_SECTION_PROGRAM 0x08
#define COFF_SECTION_DATA 0x10
#define COFF_SECTION_PROGRAM_EMPTY 0x20 // has nothing in the file, only a cleared bank
#define COFF_SECTION_DATA_EMPTY 0x40
#define COFF_SECTION_NOT_LOADED 0x80

#define DSP_BANK_SIZE 0x8000         // a bank of the shared WRAM
#define DSP_MEMORY_HALF_SIZE 0x20000 // the DSP's memory has two halves, @c0 and @c1 or @d0 and @d1
#define DSP_BANK_NONE (-1)
#define DSP_PM_PRIORITY 0x3f2 // of the process's callbacks before the system exits and sleeps

typedef BOOL (*COFFSectionFunc)(DSPProcessContext *context, const COFFHeader *header, const COFFSection *section);

static void func_027052b4(void);
static void func_027052fc(void *arg);
static void func_02705320(void *arg);
static void func_0270532c(void);
static BOOL func_027053d4(DSPProcessContext *context, u32 offset, void *dst, u32 size);
static BOOL func_0270541c(DSPProcessContext *context, MIWramPos wram, int slot, MIWramProc proc);
static BOOL func_027054e4(DSPProcessContext *context, const COFFHeader *header, const COFFSection *section);
static BOOL func_02705980(DSPProcessContext *context, COFFSectionFunc callback);
static BOOL func_02705a4c(DSPProcessContext *context, MIWramPos wram, u32 start, u32 length, MIWramProc proc);
static BOOL func_02705acc(DSPProcessContext *context, FSFile *image, int banksB, int banksC);
static void func_02705c20(void);

// Where a word of the process's memory is in the ARM9's memory
static inline void *DSPi_GetWramAddress(DSPProcessContext *context, MIWramPos wram, int wordAddress) {
    int slot = wordAddress / 0x4000;
    int offset = wordAddress - slot * 0x4000;
    int bank = (wram == MI_WRAM_B) ? context->slotOfSegmentCode[slot] : context->slotOfSegmentData[slot];
    return (u8 *)MI_GetWramMapStart(wram) + bank * DSP_BANK_SIZE + offset * 2;
}

static DSPProcessContext *sProcessList;
static BOOL sExitCallbackSet;
static PMExitCallbackInfo sDspExitCallbackInfo;
static PMSleepCallbackInfo sDspSleepCallbackInfo;
static BOOL sSleepCallbackSet;

// Tells the DSP's OS to quit, and waits for its answer
static void func_027052b4(void) {
    sProcessList->hookFactors = 0;
    DSP_SendData(2, 0x8000);
    DSP_RecvData(2);
}

// Before the system exits: quits every process
static void func_027052fc(void *arg) {
    for (;;) {
        DSPProcessContext *context = DSP_FindProcess(NULL);
        if (context == NULL) {
            return;
        }
        DSP_QuitProcess(context);
    }
}

// Before the system sleeps: ends the game
static void func_02705320(void *arg) {
    OS_Terminate();
}

// The DSP's interrupt: calls the callback of each event that the first process has a callback for, until none is left
static void func_0270532c(void) {
    DSPProcessContext *context = sProcessList;
    if (context != NULL) {
        for (;;) {
            int signaled = reg_DSP_SEM | (((reg_DSP_PSTS >> DSP_PSTS_REPLY_SHIFT) & 7) << DSP_HOOK_REPLY_SHIFT);
            int events = signaled & context->hookFactors;
            if (events == 0) {
                break;
            }
            while (events != 0) {
                int event = 32 - MATH_CountLeadingZeros(~events & (events - 1));
                events &= ~context->hookGroup[event];
                context->hookFunction[event](context->hookUserdata[event]);
            }
        }
    }
    OS_SetIrqCheckFlag(OS_IE_DSP);
}

// Reads size bytes of the image from offset
static BOOL func_027053d4(DSPProcessContext *context, u32 offset, void *dst, u32 size) {
    BOOL ok = FALSE;
    if (FS_SeekFile(context->image, offset, FS_SEEK_SET)) {
        ok = (size == FS_ReadFile(context->image, dst, size));
    }
    return ok;
}

// Gives the bank behind a slot of the process to a processor
static BOOL func_0270541c(DSPProcessContext *context, MIWramPos wram, int slot, MIWramProc proc) {
    int bank = (wram == MI_WRAM_B) ? context->slotOfSegmentCode[slot] : context->slotOfSegmentData[slot];
    BOOL given = FALSE;
    if (!MI_IsWramSlotUsed(wram, bank) ||
        MI_FreeWramSlot(wram, bank, MI_WRAM_SIZE_32KB, MI_GetWramBankMaster(wram, bank)) > 0) {
        if (MI_AllocWramSlot(wram, bank, MI_WRAM_SIZE_32KB, proc) != 0) {
            vu8 *master = (wram == MI_WRAM_B) ? reg_MBK_B : reg_MBK_C;
            if (proc == MI_WRAM_ARM9) {
                master[bank] = (master[bank] & ~0x1c) | (bank << 2);
            } else if (proc == MI_WRAM_DSP) {
                master[bank] = (master[bank] & ~0x1c) | (slot << 2);
            }
            given = TRUE;
        }
    }
    return given;
}

// Loads one section of the image into the banks of its slots, taking a bank for each slot that has none yet
static BOOL func_027054e4(DSPProcessContext *context, const COFFHeader *header, const COFFSection *section) {
    int empty;
    const char *name = section->name.chars;
    BOOL result = TRUE;
    int count = 0;
    MIWramPos wrams[4];
    u32 addresses[4];
    int empties[4];
    char nameBuffer[128];
    int i;

    if (*(const u32 *)name == 0) {
        u32 stringTable = header->symbolTable + header->symbolCount * COFF_SYMBOL_SIZE;
        u32 offset = stringTable + section->name.longName.offset;
        name = nameBuffer;
        if (!func_027053d4(context, offset, nameBuffer, sizeof(nameBuffer))) {
            return FALSE;
        }
    }
    if (STD_CompareString(name, "SDK_USING_OS@d0") == 0) {
        context->flags |= DSP_PROCESS_FLAG_USING_OS;
    }
    if (section->flags & COFF_SECTION_PROGRAM) {
        u32 address = section->programAddress * 2;
        BOOL sectionEmpty = (section->flags & COFF_SECTION_PROGRAM_EMPTY) != 0;
        if (STD_SearchString(name, "@c0")) {
            wrams[count] = MI_WRAM_B;
            addresses[count] = address;
            empties[count] = sectionEmpty;
            count++;
        }
        if (STD_SearchString(name, "@c1")) {
            wrams[count] = MI_WRAM_B;
            addresses[count] = address + DSP_MEMORY_HALF_SIZE;
            empties[count] = sectionEmpty;
            count++;
        }
    }
    if (section->flags & COFF_SECTION_DATA) {
        u32 address = section->dataAddress * 2;
        BOOL sectionEmpty = (section->flags & COFF_SECTION_DATA_EMPTY) != 0;
        if (STD_SearchString(name, "@d0")) {
            wrams[count] = MI_WRAM_C;
            addresses[count] = address;
            empties[count] = sectionEmpty;
            count++;
        }
        if (STD_SearchString(name, "@d1")) {
            wrams[count] = MI_WRAM_C;
            addresses[count] = address + DSP_MEMORY_HALF_SIZE;
            empties[count] = sectionEmpty;
            count++;
        }
    }
    for (i = 0; i < count; i++) {
        MIWramPos wram = wrams[i];
        int address = addresses[i];
        int size = section->size;
        u32 fileOffset = section->dataOffset;
        empty = empties[i];
        while (size > 0) {
            int chunk = ((address + DSP_BANK_SIZE) & ~(DSP_BANK_SIZE - 1)) - address;

            if (size <= chunk) {
                chunk = size;
            }
            if (((wram == MI_WRAM_B) ? context->slotOfSegmentCode[address / DSP_BANK_SIZE]
                                     : context->slotOfSegmentData[address / DSP_BANK_SIZE]) == DSP_BANK_NONE) {
                int slot = address / DSP_BANK_SIZE;
                u16 *freeMask = (wram == MI_WRAM_B) ? &context->slotB : &context->slotC;
                int *used = (wram == MI_WRAM_B) ? &context->segmentCode : &context->segmentData;
                int *banks = (wram == MI_WRAM_B) ? context->slotOfSegmentCode : context->slotOfSegmentData;
                u32 freeBanks = *freeMask;
                int bank = 32 - MATH_CountLeadingZeros(~freeBanks & (freeBanks - 1));
                if (bank >= 8) {
                    result = FALSE;
                    break;
                }
                banks[slot] = bank;
                *freeMask &= ~(1 << bank);
                *used |= 1 << slot;
                if (!func_0270541c(context, wram, slot, MI_WRAM_ARM9)) {
                    result = FALSE;
                    break;
                }
                MIi_CpuClearFast(0, DSPi_GetWramAddress(context, wram, slot << 14), DSP_BANK_SIZE);
            }
            if (!empty) {
                if (!func_027053d4(context, fileOffset, DSPi_GetWramAddress(context, wram, address / 2), chunk)) {
                    result = FALSE;
                    break;
                }
            }
            fileOffset += chunk;
            address += chunk;
            size -= chunk;
        }
    }
    return result;
}

void DSP_InitProcessContext(DSPProcessContext *context, const char *name) {
    int i;
    MI_CpuFill8(context, 0, sizeof(*context));
    context->next = NULL;
    context->flags = 0;
    STD_CopyString(context->name, name != NULL ? name : "");
    context->image = NULL;
    context->segmentCode = 0;
    context->segmentData = 0;
    for (i = 0; i < 8; i++) {
        context->slotOfSegmentCode[i] = DSP_BANK_NONE;
    }
    for (i = 0; i < 8; i++) {
        context->slotOfSegmentData[i] = DSP_BANK_NONE;
    }
    context->hookFactors = 0;
    for (i = 0; i < DSP_HOOK_MAX; i++) {
        context->hookFunction[i] = NULL;
        context->hookUserdata[i] = NULL;
        context->hookGroup[i] = 0;
    }
}

// Calls callback with each section of the image that has contents to load
static BOOL func_02705980(DSPProcessContext *context, COFFSectionFunc callback) {
    COFFHeader header;
    COFFSection section;
    BOOL result = FALSE;
    if (func_027053d4(context, 0, &header, sizeof(header))) {
        u32 offset = header.optionalSize + sizeof(header);
        int i;
        for (i = 0; i < header.sectionCount; i++) {
            if (!func_027053d4(context, offset + i * sizeof(section), &section, sizeof(section))) {
                break;
            }
            if (!(section.flags & COFF_SECTION_NOT_LOADED) && section.size != 0 && callback != NULL) {
                if (!callback(context, &header, &section)) {
                    break;
                }
            }
        }
        result = (i >= header.sectionCount);
    }
    return result;
}

// Gives the banks of the slots that cover length words from start to a processor
static BOOL func_02705a4c(DSPProcessContext *context, MIWramPos wram, u32 start, u32 length, MIWramProc proc) {
    int slot;
    int lastSlot;
    int *used;
    if (length < 1) {
        length = 1;
    }
    start *= 2;
    lastSlot = (start + length * 2 - 1) >> 15;
    used = (wram == MI_WRAM_B) ? &context->segmentCode : &context->segmentData;
    for (slot = start >> 15; slot <= lastSlot; slot++) {
        if (*used & (1 << slot)) {
            if (!func_0270541c(context, wram, slot, proc)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// Loads the image and gives its banks to the DSP
static BOOL func_02705acc(DSPProcessContext *context, FSFile *image, int banksB, int banksC) {
    BOOL ok = FALSE;
    context->image = image;
    context->slotB = banksB;
    context->slotC = banksC;
    if (func_02705980(context, func_027054e4)) {
        DC_FlushRange((void *)MI_GetWramMapStart_B(), 0x40000);
        DC_FlushRange((void *)MI_GetWramMapStart_C(), 0x40000);
        if (func_02705a4c(context, MI_WRAM_B, 0, 0x20000, MI_WRAM_DSP)) {
            if (func_02705a4c(context, MI_WRAM_C, 0, 0x20000, MI_WRAM_DSP)) {
                ok = TRUE;
            }
        }
    }
    return ok;
}

void DSP_SetProcessHook(DSPProcessContext *context, int events, DSPHookFunction callback, void *userData) {
    OSIntrMode irq = OS_DisableInterrupts();
    int i;
    int replies;
    u16 mask;
    u16 pcfg;
    u16 pmask;

    for (i = 0; i < DSP_HOOK_MAX; i++) {
        if (events & (1 << i)) {
            context->hookFunction[i] = callback;
            context->hookUserdata[i] = userData;
            context->hookGroup[i] = events;
        }
    }
    replies = ((events >> DSP_HOOK_REPLY_SHIFT) & 7) << DSP_PCFG_REPLY_IRQ_SHIFT;
    mask = events;
    pcfg = reg_DSP_PCFG;
    pmask = reg_DSP_PMASK;
    if (callback != NULL) {
        reg_DSP_PCFG = pcfg | replies;
        reg_DSP_PMASK = pmask & ~mask;
        context->hookFactors |= events;
    } else {
        reg_DSP_PCFG = pcfg & ~replies;
        reg_DSP_PMASK = pmask | mask;
        context->hookFactors &= ~events;
    }
    OS_RestoreInterrupts(irq);
}

static void func_02705c20(void) {
}

BOOL DSP_ExecuteProcess(DSPProcessContext *context, FSFile *image, int banksB, int banksC) {
    BOOL started = FALSE;
    if (!FS_IsAvailable()) {
        FS_Init(-1);
    }
    DSP_InitPipe();
    OS_SetIrqFunction(OS_IE_DSP, func_0270532c);
    DSP_SetProcessHook(context, 0x48000, func_027070dc, NULL);
    OS_EnableIrqMask(OS_IE_DSP);
    if (func_02705acc(context, image, banksB, banksC)) {
        OSIntrMode irq = OS_DisableInterrupts();
        DSPProcessContext **link = &sProcessList;
        while (*link != NULL && *link != context) {
            link = &(*link)->next;
        }
        *link = context;
        context->image = NULL;
        if (!sExitCallbackSet) {
            PM_SetExitCallbackInfo(&sDspExitCallbackInfo, func_027052fc, NULL);
            PM_InsertPostExitCallback(&sDspExitCallbackInfo, DSP_PM_PRIORITY);
            sExitCallbackSet = TRUE;
        }
        if (!sSleepCallbackSet) {
            PM_SetSleepCallbackInfo(&sDspSleepCallbackInfo, func_02705320, NULL);
            PM_InsertPreSleepCallback(&sDspSleepCallbackInfo, DSP_PM_PRIORITY);
            sSleepCallbackSet = TRUE;
        }
        DSP_PowerOn();
        DSP_ResetOffEx(context->hookFactors >> 16);
        DSP_MaskSemaphore(~context->hookFactors);
        func_02705c20();
        if (context->flags & DSP_PROCESS_FLAG_USING_OS) {
            u16 port;

            for (port = 0; port < 3; port++) {
                // The SDK's own volatile local for the reply; nothing else writes it
                volatile u16 reply;

                do {
                    reply = DSP_RecvDataCore(port);
                } while (reply != 1);
            }
        }
        func_0270532c();
        started = TRUE;
        OS_RestoreInterrupts(irq);
    }
    return started;
}

void DSP_QuitProcess(DSPProcessContext *context) {
    OSIntrMode irq;
    DSPProcessContext **link;
    DSPProcessContext *node;
    func_027052b4();
    irq = OS_DisableInterrupts();
    DSP_ResetOn();
    DSP_PowerOff();
    OS_DisableIrqMask(OS_IE_DSP);
    OS_SetIrqFunction(OS_IE_DSP, NULL);
    MI_FreeWram(MI_WRAM_B, MI_WRAM_DSP);
    MI_FreeWram(MI_WRAM_C, MI_WRAM_DSP);
    link = &sProcessList;
    for (node = *link; node != NULL; node = node->next) {
        if (node == context) {
            *link = node->next;
            break;
        }
        link = &node->next;
    }
    context->next = NULL;
    OS_RestoreInterrupts(irq);
    PM_DeletePreSleepCallback(&sDspSleepCallbackInfo);
    sSleepCallbackSet = FALSE;
}

DSPProcessContext *DSP_FindProcess(const char *name) {
    DSPProcessContext *context;
    OSIntrMode irq = OS_DisableInterrupts();
    context = sProcessList;
    if (name != NULL) {
        while (context != NULL && STD_CompareString(context->name, name) != 0) {
            context = context->next;
        }
    }
    OS_RestoreInterrupts(irq);
    return context;
}

void DSP_WriteProcessPipe(DSPProcessContext *context, int port, const void *src, u32 length) {
    DSPPipe pipe;
    DSP_LoadPipe(&pipe, port, 1);
    DSP_WritePipe(&pipe, src, length);
}
