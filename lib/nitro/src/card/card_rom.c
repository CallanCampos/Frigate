// NitroSDK's CARD library, the game card's ROM: the commands to the card through its registers, reading the ROM a page
// at a time with the CPU or with a DMA channel, and the setup of the reads. The file name is a guess, from NitroSDK's
// card_rom.c; the library has no file name string.
#include "nitro/card.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The card's registers are in nitro/hw.h

// The words of a command go to the card in the other byte order
#define MI_SWAP32(x) ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | ((x) << 24))

// The control word of the ROM header (HW_CARD_ROM_HEADER) for the card's normal commands, 0x60 bytes in
#define CARD_ROM_HEADER_NORMAL_CONTROL (*(u32 *)(HW_CARD_ROM_HEADER + 0x60))
// The ID of the card that the system read at boot, of which one bit says that the card has the refresh command
#define HW_CARD_ROM_ID (*(u32 *)0x02fffc00)
#define CARD_ROM_ID_HAS_REFRESH 0x20000000

// The commands to the card, which MCCMD0 takes as the first byte: reading a page, the ID, the status and the refresh.
// A card in the DSi's mode has other commands for the first two
#define CARD_ROM_CMD_READ_PAGE 0xb7
#define CARD_ROM_CMD_READ_PAGE_TWL 0x00
#define CARD_ROM_CMD_READ_ID 0xb8
#define CARD_ROM_CMD_READ_ID_TWL 0x90
#define CARD_ROM_CMD_READ_STATUS 0xd6
#define CARD_ROM_CMD_REFRESH 0xb5

// What the status of the card says: a bit that asks for the refresh, and the one that the card sets when it is done
// (a card without the refresh always has it). Named for what the code does with them
#define CARD_ROM_STATUS_NEED_REFRESH 0x08
#define CARD_ROM_STATUS_REFRESHED 0x20

// A ROM page, the unit that the card reads in
#define CARD_ROM_PAGE_SIZE 0x200

// The memory that DMA can't reach: the instruction TCM, which ends where main memory starts, and the data TCM
#define HW_ITCM_IMAGE 0x01ff8000
#define HW_MAIN_MEM 0x02000000
#define HW_DTCM_SIZE 0x4000

// What reads the ROM, as CARDi_ReadRomWithCPU does: len bytes from the offset src into dst. The first parameter is for
// the argument of a callback, which a reader by DMA gives the read
typedef u32 (*CARDiRomReader)(void *arg, void *dst, u32 src, u32 len);

static void func_0206f8b4(u32 cmd, u32 offset);
static BOOL func_0206f928(void);
static void func_0206f95c(u32 offset);
static u32 func_0206f994(void);
static u32 func_0206f9e4(void);
static void func_0206fa38(u32 mask);
static void func_0206fa7c(void);
static void func_0206fb8c(void);
static void func_0206fc44(void *arg);
static BOOL func_0206fc68(u32 dma, u32 dst, u32 src, u32 len);
static void func_0206fce8(CARDiCommon *p);
static void func_0206fec8(void);

// The file's data. -ipa file reaches the statics of a section from one base, and lays out those of one size in an order
// that follows from the order they are declared in, which is the ROM's with this one: the read by DMA, and the pointer
// that the interrupt works on
static CARDiRomDmaRead sRomDmaRead;
// The offset of the ROM in the card's address space, which is zero (card_common.c clears it)
u32 cardi_rom_base;
static CARDiRomDmaRead *sRomDmaActive;
// How the thread's tasks read the ROM. A read uses DMA only while this is the CPU's reader; the DSi's setup in the LTD
// autoload swaps it for its own
static CARDiRomReader sRomReader;
// Whether a read discards the instruction cache of what it read, which nothing turns on
static BOOL sInvalidateIC;
// The 0x88 bytes that follow the ROM's program, which CARD_Init reads on a ROM boot. The multiboot library sends them
// after the header of the program it sends to the children (mb_segment.c), as its authentication code
static u8 sRomAuthCode[0x88];
// The last page that the CPU read
static u8 sPageBuffer[CARD_ROM_PAGE_SIZE] ATTRIBUTE_ALIGN(32);
// The page that the buffer holds, or 1 for none
static u32 sCachedPage = 1;
// Whether a read by DMA discards the data cache of what it read
static BOOL sInvalidateDC = TRUE;

// CARDi_SetRomOp: starts a command to the card with its command byte and the ROM offset it works on
static void func_0206f8b4(u32 cmd, u32 offset) {
    u32 hi = (offset >> 8) | (cmd << 24);
    u32 lo = offset << 24;
    while ((reg_MI_MCCNT1 & REG_MI_MCCNT1_START_MASK) != 0) {
    }
    reg_MI_MCCNT0 = (reg_MI_MCCNT0 & ~REG_MI_MCCNT0_SEL_MASK) | REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_I_MASK;
    reg_MI_MCCMD0 = MI_SWAP32(hi);
    reg_MI_MCCMD1 = MI_SWAP32(lo);
}

// Whether the card works in the DSi's mode: the game is on a DSi and was not booted from the ROM, and its header wants
// the card powered on and not in the DS's mode
static BOOL func_0206f928(void) {
    const CARDTwlRomHeader *header = func_0206eee8();
    if (hw_isDSi() && sys_get_boot_indicator() != OS_BOOTTYPE_ROM && header->accessControl.gameCardPowerOn &&
        !header->accessControl.gameCardNtrMode) {
        return TRUE;
    }
    return FALSE;
}

// Starts the read of the page at the offset, which the interrupt of the card answers or a loop of the CPU polls
static void func_0206f95c(u32 offset) {
    u32 cmd = CARD_ROM_CMD_READ_PAGE;
    if (func_0206f928()) {
        cmd = CARD_ROM_CMD_READ_PAGE_TWL;
    }
    func_0206f8b4(cmd, offset);
    reg_MI_MCCNT1 = (CARD_ROM_HEADER_NORMAL_CONTROL & ~REG_MI_MCCNT1_PC_MASK) | REG_MI_MCCNT1_START_MASK |
                    REG_MI_MCCNT1_RESB_MASK | REG_MI_MCCNT1_PC_512B;
}

// CARDi_ReadRomIDCore: reads the card's ID
static u32 func_0206f994(void) {
    u32 cmd = CARD_ROM_CMD_READ_ID;
    if (func_0206f928()) {
        cmd = CARD_ROM_CMD_READ_ID_TWL;
    }
    func_0206f8b4(cmd, 0);
    reg_MI_MCCNT1 = ((CARD_ROM_HEADER_NORMAL_CONTROL & ~REG_MI_MCCNT1_PC_MASK) | REG_MI_MCCNT1_START_MASK |
                     REG_MI_MCCNT1_RESB_MASK | REG_MI_MCCNT1_PC_4B) &
                    ~REG_MI_MCCNT1_L1_MASK;
    while ((reg_MI_MCCNT1 & REG_MI_MCCNT1_READY_MASK) == 0) {
    }
    return reg_MI_MCD1;
}

// Reads the card's status. A card whose ID says that it has no refresh is always done
static u32 func_0206f9e4(void) {
    if ((HW_CARD_ROM_ID & CARD_ROM_ID_HAS_REFRESH) == 0) {
        return CARD_ROM_STATUS_REFRESHED;
    }
    func_0206f8b4(CARD_ROM_CMD_READ_STATUS, 0);
    reg_MI_MCCNT1 = ((CARD_ROM_HEADER_NORMAL_CONTROL & ~REG_MI_MCCNT1_PC_MASK) | REG_MI_MCCNT1_START_MASK |
                     REG_MI_MCCNT1_RESB_MASK | REG_MI_MCCNT1_PC_4B) &
                    ~REG_MI_MCCNT1_L1_MASK;
    while ((reg_MI_MCCNT1 & REG_MI_MCCNT1_READY_MASK) == 0) {
    }
    return reg_MI_MCD1;
}

// If the status has a bit of the mask, refreshes the card and waits for the end, sleeping between polls if the threads
// run
static void func_0206fa38(u32 mask) {
    if ((func_0206f9e4() & mask) != 0) {
        func_0206fa7c();
        if ((func_0206f9e4() & CARD_ROM_STATUS_REFRESHED) == 0) {
            do {
                if (data_0214c21c != 0 && OS_IsAlarmAvailable()) {
                    func_0207aa04(1);
                }
            } while ((func_0206f9e4() & CARD_ROM_STATUS_REFRESHED) == 0);
        }
    }
}

// Sends the refresh command and waits for the transfer to end
static void func_0206fa7c(void) {
    func_0206f8b4(CARD_ROM_CMD_REFRESH, 0);
    reg_MI_MCCNT1 = ((CARD_ROM_HEADER_NORMAL_CONTROL & ~REG_MI_MCCNT1_PC_MASK) | REG_MI_MCCNT1_START_MASK |
                     REG_MI_MCCNT1_RESB_MASK) &
                    ~REG_MI_MCCNT1_L1_MASK;
    while ((reg_MI_MCCNT1 & REG_MI_MCCNT1_START_MASK) != 0) {
    }
}

// CARDi_ReadRomWithCPU: reads a page at a time through the page buffer, which keeps the last page for the next read.
// A whole page for an aligned destination is read straight into it. Returns the length
u32 func_0206fab4(void *arg, void *dst, u32 src, u32 len) {
    u32 total = len;
    u32 cachedPage = sCachedPage;
    while (len != 0) {
        u32 *buffer = dst;
        u32 size = CARD_ROM_PAGE_SIZE;
        u32 page = src & ~(CARD_ROM_PAGE_SIZE - 1);
        if (page == cachedPage) {
            buffer = (u32 *)sPageBuffer;
        } else {
            u32 control;
            u32 i;
            if (page != src || ((u32)dst & 3) != 0 || len < size) {
                buffer = (u32 *)sPageBuffer;
                cachedPage = page;
            }
            func_0206f95c(page);
            i = 0;
            do {
                control = reg_MI_MCCNT1;
                if ((control & REG_MI_MCCNT1_READY_MASK) != 0) {
                    u32 word = reg_MI_MCD1;
                    if (i < CARD_ROM_PAGE_SIZE / 4) {
                        buffer[i++] = word;
                    }
                }
            } while ((control & REG_MI_MCCNT1_START_MASK) != 0);
        }
        if (buffer == (u32 *)sPageBuffer) {
            u32 offset = src - page;
            size = CARD_ROM_PAGE_SIZE - offset;
            if (len <= size) {
                size = len;
            }
            sys_memcpy(sPageBuffer + offset, dst, size);
        }
        len -= size;
        dst = (u8 *)dst + size;
        src += size;
    }
    func_0206ffcc(func_0206f994());
    sCachedPage = cachedPage;
    return total;
}

// The interrupt for a page that the DMA channel received: starts the next page, or ends the read
static void func_0206fb8c(void) {
    CARDiRomDmaRead *read = sRomDmaActive;
    if (read != NULL) {
        read->src += CARD_ROM_PAGE_SIZE;
        read->dst += CARD_ROM_PAGE_SIZE;
        read->len -= CARD_ROM_PAGE_SIZE;
        if (read->len != 0) {
            func_0206f95c(read->src);
            return;
        }
        cardi_common.dmaInterface->Stop(cardi_common.dma);
        CPU_DisableInterrupts(OS_IE_CARD_DATA);
        CPU_AcknowledgeInterrupts(OS_IE_CARD_DATA);
        sRomDmaActive = NULL;
        func_0206ffcc(func_0206f994());
        if (read->callback != NULL) {
            read->callback(read->arg);
        }
    }
}

// CARDi_ReadRomWithDMA: has the DMA channel receive the first page, and leaves the rest to the interrupt
void func_0206fbf0(CARDiRomDmaRead *read) {
    u32 irq = CPU_IRQDisable();
    sRomDmaActive = read;
    CPU_SetIRQFunc(OS_IE_CARD_DATA, func_0206fb8c);
    CPU_AcknowledgeInterrupts(OS_IE_CARD_DATA);
    CPU_EnableInterrupts(OS_IE_CARD_DATA);
    CPU_SetIRQMask(irq);
    cardi_common.dmaInterface->Recv(cardi_common.dma, (void *)&reg_MI_MCD1, (void *)read->dst, CARD_ROM_PAGE_SIZE);
    func_0206f95c(read->src);
}

// The callback of a read by DMA: ends the command
static void func_0206fc44(void *arg) {
    func_0206ffcc(func_0206f994());
    func_0206fa38(CARD_ROM_STATUS_NEED_REFRESH);
    cardi_common.cmd->result = CARD_RESULT_SUCCESS;
    func_0206f224(&cardi_common);
}

// Whether a read can use DMA: the channel has to exist, and the destination to start at a cache line of 32 bytes and
// to be outside both TCMs, while the source and the length are whole pages
static BOOL func_0206fc68(u32 dma, u32 dst, u32 src, u32 len) {
    BOOL result = FALSE;
    BOOL reachable = FALSE;
    BOOL outsideItcm = FALSE;
    if (dma <= 3 && len != 0 && (dst & 0x1f) == 0) {
        BOOL outside = TRUE;
        if (dst + len > HW_ITCM_IMAGE && dst < HW_MAIN_MEM) {
            outside = FALSE;
        }
        if (outside) {
            outsideItcm = TRUE;
        }
    }
    if (outsideItcm) {
        BOOL outside = TRUE;
        if (dst + len > func_0207b79c() && dst < func_0207b79c() + HW_DTCM_SIZE) {
            outside = FALSE;
        }
        if (outside) {
            reachable = TRUE;
        }
    }
    if (reachable) {
        if (((src | len) & (CARD_ROM_PAGE_SIZE - 1)) == 0) {
            result = TRUE;
        }
    }
    return result;
}

// The task that reads with the reader on the thread
static void func_0206fce8(CARDiCommon *p) {
    sRomReader(NULL, (void *)p->dst, p->src, p->len);
    func_0206ffcc(func_0206f994());
    func_0206fa38(CARD_ROM_STATUS_NEED_REFRESH);
    cardi_common.cmd->result = CARD_RESULT_SUCCESS;
}

// CARDi_ReadRom: reads len bytes of the ROM from the offset src, by DMA if the channel and the addresses allow it, else
// with the reader on the thread. Waits for the end unless async
void func_0206fd24(u32 dma, u32 src, void *dst, u32 len, MIDmaCallback callback, void *arg, BOOL async) {
    CARDiCommon *p = &cardi_common;
    u32 dmaNo;
    func_0206eeb0();
    if ((func_0206f018() & CARD_ACCESS_LEVEL_ROM) == 0) {
        sys_exit();
    }
    func_0206f1c0(p, TRUE, callback, arg);
    p->dmaInterface = func_0206f25c(dma);
    if (p->dmaInterface != NULL) {
        dmaNo = dma & 3;
    } else {
        dmaNo = (u32)-1;
    }
    p->dma = dmaNo;
    if (dmaNo <= 3) {
        p->dmaInterface->Stop(dmaNo);
    }
    p->src = src + cardi_rom_base;
    p->dst = (u32)dst;
    p->len = len;
    sRomDmaRead.callback = func_0206fc44;
    sRomDmaRead.arg = NULL;
    sRomDmaRead.src = p->src;
    sRomDmaRead.dst = p->dst;
    sRomDmaRead.len = p->len;
    sRomDmaRead.unk_18 = 0;
    if (sRomReader == func_0206fab4 && func_0206fc68(p->dma, (u32)dst, p->src, len)) {
        u32 irq = CPU_IRQDisable();
        if (sInvalidateIC) {
            func_0206f298((void *)p->dst, p->len, p->icInvalidateThreshold);
        }
        if (sInvalidateDC) {
            func_0206f2ac((void *)p->dst, p->len, p->dcInvalidateThreshold);
        }
        CPU_SetIRQMask(irq);
        func_0206fbf0(&sRomDmaRead);
        if (!async) {
            func_0206fec8();
        }
    } else {
        if (sInvalidateIC) {
            func_0206f298((void *)p->dst, p->len, p->icInvalidateThreshold);
        }
        func_0206f134(func_0206fce8, async);
    }
}

// CARDi_InitRom: has the CPU read the ROM, and on a ROM boot reads the 0x88 bytes after the program
void func_0206fe54(void) {
    sRomReader = func_0206fab4;
    if (sys_get_boot_indicator() == OS_BOOTTYPE_ROM) {
        if (rom_get_header()->romSize != 0) {
            u16 lockId = cart_key_create();
            func_0206eef0(lockId);
            func_0206fab4(NULL, sRomAuthCode, rom_get_header()->romSize, sizeof(sRomAuthCode));
            func_0206ef38(lockId);
            cart_key_release(lockId);
        }
    }
    if (hw_isDSi()) {
        func_0206feb8();
    }
}

// The linker's veneer from here to the DSi's part of the library in the LTD autoload, out of the Thumb call's range.
// The NOP fills the halfword before the ARM entry point
// clang-format off
asm void func_0206feb8(void) {
    bx pc
    nop
}

#pragma thumb off
asm void func_0206febc(void) {
    ldr ip, [pc, #0]
    bx ip
    dcd 0x02767521
}
#pragma thumb on
// clang-format on

// Waits for the last task of the library, which is the one just started
static void func_0206fec8(void) {
    func_0206f07c();
}

// The 0x88 bytes that follow the ROM's program, as CARDi_InitRom read them
const void *func_0206fed0(void) {
    return sRomAuthCode;
}
