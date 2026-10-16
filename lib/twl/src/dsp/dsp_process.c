#include "types.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "twl/dsp.h"

// TwlSDK's DSP library, the DSP's processes (guessed file name, after the SDK's dsp_process.c): a context for each COFF
// image, which this file loads into the shared WRAM B and C and hands to the DSP, the interrupt that calls the
// context's callbacks, and the list of the running processes. ARM code in the DSi-only LTD autoload. The ROM names none
// of it; the functions keep their default names, and the SDK's are in twl/dsp.h.

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
static BOOL func_0270541c(DSPProcessContext *context, MIWram wram, int slot, MIWramProc proc);
static BOOL func_027054e4(DSPProcessContext *context, const COFFHeader *header, const COFFSection *section);
static BOOL func_02705980(DSPProcessContext *context, COFFSectionFunc callback);
static BOOL func_02705a4c(DSPProcessContext *context, MIWram wram, u32 start, u32 length, MIWramProc proc);
static BOOL func_02705acc(DSPProcessContext *context, FSFile *image, int banksB, int banksC);
static void func_02705c20(void);

// Where a word of the process's memory is in the ARM9's memory
static inline void *DSPi_GetWramAddress(DSPProcessContext *context, MIWram wram, int wordAddress) {
    int slot = wordAddress / 0x4000;
    int offset = wordAddress - slot * 0x4000;
    int bank = (wram == MI_WRAM_B) ? context->bankB[slot] : context->bankC[slot];
    return (u8 *)func_027687d8(wram) + bank * DSP_BANK_SIZE + offset * 2;
}

static DSPProcessContext *sProcessList;
static BOOL sExitCallbackSet;
static PMExitCallbackInfo sExitCallbackInfo;
static PMSleepCallbackInfo sSleepCallbackInfo;
static BOOL sSleepCallbackSet;

// Tells the DSP's OS to quit, and waits for its answer
static void func_027052b4(void) {
    sProcessList->events = 0;
    if (hw_isDSi() == TRUE) {
        func_0270502c(2, 0x8000);
    }
    if (hw_isDSi() == TRUE) {
        func_0270505c(2);
    }
}

// Before the system exits: quits every process
static void func_027052fc(void *arg) {
    for (;;) {
        DSPProcessContext *context = func_02705ed0(NULL);
        if (context == NULL) {
            return;
        }
        func_02705e04(context);
    }
}

// Before the system sleeps: ends the game
static void func_02705320(void *arg) {
    sys_exit();
}

// The DSP's interrupt: calls the callback of each event that the first process has a callback for, until none is left
static void func_0270532c(void) {
    DSPProcessContext *context = sProcessList;
    if (context != NULL) {
        for (;;) {
            int signaled = reg_DSP_SEM | (((reg_DSP_PSTS >> DSP_PSTS_REPLY_SHIFT) & 7) << DSP_EVENT_REPLY_SHIFT);
            int events = signaled & context->events;
            if (events == 0) {
                break;
            }
            while (events != 0) {
                int event = 32 - MATH_CountLeadingZeros(~events & (events - 1));
                events &= ~context->group[event];
                context->callback[event](context->userData[event]);
            }
        }
    }
    OS_SetIrqCheckFlag(OS_IE_DSP);
}

// Reads size bytes of the image from offset
static BOOL func_027053d4(DSPProcessContext *context, u32 offset, void *dst, u32 size) {
    BOOL ok = FALSE;
    if (romfs_fseek(context->image, offset, FS_SEEK_SET)) {
        ok = (size == romfs_fread(context->image, dst, size));
    }
    return ok;
}

// Gives the bank behind a slot of the process to a processor
static BOOL func_0270541c(DSPProcessContext *context, MIWram wram, int slot, MIWramProc proc) {
    int bank = (wram == MI_WRAM_B) ? context->bankB[slot] : context->bankC[slot];
    BOOL given = FALSE;
    if (!func_0276919c(wram, bank) || func_02768d90(wram, bank, 1, func_02768770(wram, bank)) > 0) {
        if (func_02768c04(wram, bank, 1, proc) != 0) {
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
    int mask;
    int empty;
    const char *name = section->name.chars;
    BOOL result = TRUE;
    int count = 0;
    MIWram wrams[4];
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
        context->flags |= DSP_PROCESS_USING_OS;
    }
    if (section->flags & COFF_SECTION_PROGRAM) {
        u32 address = section->programAddress * 2;
        BOOL empty = (section->flags & COFF_SECTION_PROGRAM_EMPTY) != 0;
        if (func_0207f874(name, "@c0")) {
            wrams[count] = MI_WRAM_B;
            addresses[count] = address;
            empties[count] = empty;
            count++;
        }
        if (func_0207f874(name, "@c1")) {
            wrams[count] = MI_WRAM_B;
            addresses[count] = address + DSP_MEMORY_HALF_SIZE;
            empties[count] = empty;
            count++;
        }
    }
    if (section->flags & COFF_SECTION_DATA) {
        u32 address = section->dataAddress * 2;
        BOOL empty = (section->flags & COFF_SECTION_DATA_EMPTY) != 0;
        if (func_0207f874(name, "@d0")) {
            wrams[count] = MI_WRAM_C;
            addresses[count] = address;
            empties[count] = empty;
            count++;
        }
        if (func_0207f874(name, "@d1")) {
            wrams[count] = MI_WRAM_C;
            addresses[count] = address + DSP_MEMORY_HALF_SIZE;
            empties[count] = empty;
            count++;
        }
    }
    for (i = 0; i < count; i++) {
        MIWram wram = wrams[i];
        int address = addresses[i];
        int size = section->size;
        u32 fileOffset = section->dataOffset;
        empty = empties[i];
        while (size > 0) {
            int chunk;
            mask = -DSP_BANK_SIZE;
            chunk = ((address + DSP_BANK_SIZE) & mask) - address;
            if (size <= chunk) {
                chunk = size;
            }
            if (((wram == MI_WRAM_B) ? context->bankB[address / DSP_BANK_SIZE]
                                     : context->bankC[address / DSP_BANK_SIZE]) == DSP_BANK_NONE) {
                int slot = address / DSP_BANK_SIZE;
                u16 *freeMask = (wram == MI_WRAM_B) ? &context->freeBanksB : &context->freeBanksC;
                u32 *used = (wram == MI_WRAM_B) ? &context->usedSlotsB : &context->usedSlotsC;
                int *banks = (wram == MI_WRAM_B) ? context->bankB : context->bankC;
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
                sys_memset32_fast(0, DSPi_GetWramAddress(context, wram, slot << 14), DSP_BANK_SIZE);
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

void func_027058d8(DSPProcessContext *context, const char *name) {
    int i;
    sys_memset(context, 0, sizeof(*context));
    context->next = NULL;
    context->flags = 0;
    func_0207f7a4(context->name, name != NULL ? name : "");
    context->image = NULL;
    context->usedSlotsB = 0;
    context->usedSlotsC = 0;
    for (i = 0; i < 8; i++) {
        context->bankB[i] = DSP_BANK_NONE;
    }
    for (i = 0; i < 8; i++) {
        context->bankC[i] = DSP_BANK_NONE;
    }
    context->events = 0;
    for (i = 0; i < DSP_EVENT_COUNT; i++) {
        context->callback[i] = NULL;
        context->userData[i] = NULL;
        context->group[i] = 0;
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
static BOOL func_02705a4c(DSPProcessContext *context, MIWram wram, u32 start, u32 length, MIWramProc proc) {
    int slot;
    int lastSlot;
    u32 *used;
    if (length < 1) {
        length = 1;
    }
    start *= 2;
    lastSlot = (start + length * 2 - 1) >> 15;
    used = (wram == MI_WRAM_B) ? &context->usedSlotsB : &context->usedSlotsC;
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
    context->freeBanksB = banksB;
    context->freeBanksC = banksC;
    if (func_02705980(context, func_027054e4)) {
        cp15_flushDC(func_027687a0(), 0x40000);
        cp15_flushDC(func_027687bc(), 0x40000);
        if (func_02705a4c(context, MI_WRAM_B, 0, 0x20000, MI_WRAM_DSP)) {
            if (func_02705a4c(context, MI_WRAM_C, 0, 0x20000, MI_WRAM_DSP)) {
                ok = TRUE;
            }
        }
    }
    return ok;
}

void func_02705b70(DSPProcessContext *context, int events, DSPCallback callback, void *userData) {
    u32 irq = CPU_IRQDisable();
    int i;
    for (i = 0; i < DSP_EVENT_COUNT; i++) {
        if (events & (1 << i)) {
            context->callback[i] = callback;
            context->userData[i] = userData;
            context->group[i] = events;
        }
    }
    int replies = ((events >> DSP_EVENT_REPLY_SHIFT) & 7) << DSP_PCFG_REPLY_IRQ_SHIFT;
    u16 mask = events;
    u16 pcfg = reg_DSP_PCFG;
    u16 pmask = reg_DSP_PMASK;
    if (callback != NULL) {
        reg_DSP_PCFG = pcfg | replies;
        reg_DSP_PMASK = pmask & ~mask;
        context->events |= events;
    } else {
        reg_DSP_PCFG = pcfg & ~replies;
        reg_DSP_PMASK = pmask | mask;
        context->events &= ~events;
    }
    CPU_SetIRQMask(irq);
}

static void func_02705c20(void) {
}

BOOL func_02705c24(DSPProcessContext *context, FSFile *image, int banksB, int banksC) {
    BOOL started = FALSE;
    if (!fs_init_check()) {
        fs_init(-1);
    }
    func_02706cd4();
    CPU_SetIRQFunc(OS_IE_DSP, func_0270532c);
    func_02705b70(context, 0x48000, func_027070dc, NULL);
    CPU_EnableInterrupts(OS_IE_DSP);
    if (func_02705acc(context, image, banksB, banksC)) {
        u32 irq = CPU_IRQDisable();
        DSPProcessContext **link = &sProcessList;
        u16 mask;
        while (*link != NULL && *link != context) {
            link = &(*link)->next;
        }
        *link = context;
        context->image = NULL;
        if (!sExitCallbackSet) {
            PM_SetExitCallbackInfo(&sExitCallbackInfo, func_027052fc, NULL);
            func_02769550(&sExitCallbackInfo, DSP_PM_PRIORITY);
            sExitCallbackSet = TRUE;
        }
        if (!sSleepCallbackSet) {
            PM_SetSleepCallbackInfo(&sSleepCallbackInfo, func_02705320, NULL);
            func_0207f614(&sSleepCallbackInfo, DSP_PM_PRIORITY);
            sSleepCallbackSet = TRUE;
        }
        if (hw_isDSi() == TRUE) {
            func_02704e90();
        }
        mask = context->events >> 16;
        if (hw_isDSi() == TRUE) {
            func_02704f4c(mask);
        }
        mask = ~context->events;
        if (hw_isDSi() == TRUE) {
            func_027052a4(mask);
        }
        func_02705c20();
        if (context->flags & DSP_PROCESS_USING_OS) {
            u16 port = 0;
            do {
                // volatile: the ROM stores each reply in a stack variable and reads it back to compare it
                volatile u16 reply;
                do {
                    reply = func_0270505c(port);
                } while (reply != 1);
                port++;
            } while (port < 3);
        }
        func_0270532c();
        started = TRUE;
        CPU_SetIRQMask(irq);
    }
    return started;
}

void func_02705e04(DSPProcessContext *context) {
    u32 irq;
    DSPProcessContext **link;
    DSPProcessContext *node;
    func_027052b4();
    irq = CPU_IRQDisable();
    if (hw_isDSi() == TRUE) {
        func_02704f1c();
    }
    if (hw_isDSi() == TRUE) {
        func_02704ee8();
    }
    CPU_DisableInterrupts(OS_IE_DSP);
    CPU_SetIRQFunc(OS_IE_DSP, NULL);
    func_02768cf8(MI_WRAM_B, MI_WRAM_DSP);
    func_02768cf8(MI_WRAM_C, MI_WRAM_DSP);
    link = &sProcessList;
    for (node = *link; node != NULL; node = node->next) {
        if (node == context) {
            *link = node->next;
            break;
        }
        link = &node->next;
    }
    context->next = NULL;
    CPU_SetIRQMask(irq);
    func_0207f62c(&sSleepCallbackInfo);
    sSleepCallbackSet = FALSE;
}

DSPProcessContext *func_02705ed0(const char *name) {
    DSPProcessContext *context;
    u32 irq = CPU_IRQDisable();
    context = sProcessList;
    if (name != NULL) {
        while (context != NULL && STD_CompareString(context->name, name) != 0) {
            context = context->next;
        }
    }
    CPU_SetIRQMask(irq);
    return context;
}

void func_02705f28(DSPProcessContext *context, int port, const void *src, u32 length) {
    DSPPipe pipe;
    func_02706d44(&pipe, port, 1);
    func_02706fcc(&pipe, src, length);
}
