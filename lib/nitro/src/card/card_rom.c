// NitroSDK's CARD library, the game card's ROM: the commands to the card through its registers, reading the ROM a page
// at a time with the CPU or with a DMA channel, and the setup of the reads. The file name is NitroSDK's (card_rom.c, as
// ntrtwl's reconstruction of NitroSDK has it); the library has no file name string. The file has TwlSDK's DSi part
// too, the secure ROM reader, which reads through the card's hash table, in TwlSDK's LTDMAIN section, which the LTD
// autoload's .ltdmain holds: dsd lists it as lib/twl/src/card/card_rom_twl.c, which config/merged_sources.toml links
// from this file's object. The DSi part's CARDi_InitRomHash reaches sRomReader from the base of the main part's pooled
// .bss, as only code of the same object does. The functions have TwlSDK's names, which aletheia's match of Black 1's
// code to TwlSDK's card_rom.o (libcard.TWL.HYB.a) gives in the same order, as do CARDiDmaReadInfo, CARDiOwnSignature,
// CARDi_cache_buf and CARDiEnableCacheInvalidationDC. CARD_WaitRomAsync and CARDi_GetRomCacheBuffer rest on weaker
// evidence: their eight bytes match card_pullout.o's too. The other statics have names of ours.
#include "nitro/card.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "twl/card.h"
#include "twl/mi.h"

// The card's registers are in nitro/hw.h

// The control word of the ROM header for the card's normal commands
#define CARD_ROM_HEADER_NORMAL_CONTROL (((const CARDRomHeader *)HW_CARD_ROM_HEADER)->normalCommandControl)
// The bit of the boot card's ID that says that the card has the refresh command
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

// The boot code leaves the card security flag in shared memory
#define CARD_TWL_BOOT_FLAGS (*(vu8 *)0x02ffe01c)

static void CARDi_SetRomOp(u32 cmd, u32 offset);
static BOOL CARDi_IsNormalMode(void);
static void CARDi_StartRomPageTransfer(u32 offset);
static u32 CARDi_ReadRomIDCore(void);
static u32 CARDi_ReadRomStatusCore(void);
static void CARDi_RefreshRom(u32 mask);
static void CARDi_RefreshRomCore(void);
static void CARDi_DmaReadPageCallback(void);
static void CARDi_DmaReadDone(void *arg);
static BOOL CARDi_IsRomDmaAvailable(u32 dma, u32 dst, u32 src, u32 len);
static void CARDi_ReadRomSyncCore(CARDiCommon *p);
static void CARD_WaitRomAsync(void);
static u32 CARDi_ReadCardWithHash(void *arg, void *dst, u32 src, u32 len);
static u32 CARDi_ReadCardWithHashInternalAsync(void *arg, void *dst, u32 src, u32 len);

// The file's data. -ipa file reaches the statics of a section from one base, and lays out those of one size in an order
// that follows from the order they are declared in, which is the ROM's with this one: the read by DMA, and the pointer
// that the interrupt works on
static CARDiRomDmaRead CARDiDmaReadInfo;
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
static u8 CARDiOwnSignature[0x88];
// The last page that the CPU read
static u8 CARDi_cache_buf[CARD_ROM_PAGE_SIZE] ATTRIBUTE_ALIGN(32);
// Whether a read by DMA discards the data cache of what it read
static BOOL CARDiEnableCacheInvalidationDC = TRUE;
// The page that the buffer holds, or 1 for none
static u32 sCachedPage = 1;

// Starts a command to the card with its command byte and the ROM offset it works on
static void CARDi_SetRomOp(u32 cmd, u32 offset) {
    u32 hi = (offset >> 8) | (cmd << 24);
    u32 lo = offset << 24;
    while ((reg_MI_MCCNT1 & REG_MI_MCCNT1_START_MASK) != 0) {
    }
    reg_MI_MCCNT0 = (reg_MI_MCCNT0 & ~REG_MI_MCCNT0_SEL_MASK) | REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_I_MASK;
    reg_MI_MCCMD0 = MI_SwapEndian32(hi);
    reg_MI_MCCMD1 = MI_SwapEndian32(lo);
}

// Whether the card is in what the SDK calls its normal mode, in which it takes the DSi's commands: the game is on a DSi
// and was not booted from the ROM, and its header wants the card powered on and not in the DS's mode
static BOOL CARDi_IsNormalMode(void) {
    const CARDTwlRomHeader *header = CARD_GetOwnRomHeaderTWL();
    if (OS_IsRunOnTwl() && OS_GetBootType() != OS_BOOTTYPE_ROM && header->accessControl.gameCardPowerOn &&
        !header->accessControl.gameCardNtrMode) {
        return TRUE;
    }
    return FALSE;
}

// Starts the read of the page at the offset, which the interrupt of the card answers or a loop of the CPU polls
static void CARDi_StartRomPageTransfer(u32 offset) {
    u32 cmd = CARD_ROM_CMD_READ_PAGE;
    if (CARDi_IsNormalMode()) {
        cmd = CARD_ROM_CMD_READ_PAGE_TWL;
    }
    CARDi_SetRomOp(cmd, offset);
    reg_MI_MCCNT1 = (CARD_ROM_HEADER_NORMAL_CONTROL & ~REG_MI_MCCNT1_PC_MASK) | REG_MI_MCCNT1_START_MASK |
                    REG_MI_MCCNT1_RESB_MASK | REG_MI_MCCNT1_PC_512B;
}

// Reads the card's ID
static u32 CARDi_ReadRomIDCore(void) {
    u32 cmd = CARD_ROM_CMD_READ_ID;
    if (CARDi_IsNormalMode()) {
        cmd = CARD_ROM_CMD_READ_ID_TWL;
    }
    CARDi_SetRomOp(cmd, 0);
    reg_MI_MCCNT1 = ((CARD_ROM_HEADER_NORMAL_CONTROL & ~REG_MI_MCCNT1_PC_MASK) | REG_MI_MCCNT1_START_MASK |
                     REG_MI_MCCNT1_RESB_MASK | REG_MI_MCCNT1_PC_4B) &
                    ~REG_MI_MCCNT1_L1_MASK;
    while ((reg_MI_MCCNT1 & REG_MI_MCCNT1_RDY_MASK) == 0) {
    }
    return reg_MI_MCD1;
}

// Reads the card's status. A card whose ID says that it has no refresh is always done
static u32 CARDi_ReadRomStatusCore(void) {
    if ((OS_GetSystemWork()->bootCardId & CARD_ROM_ID_HAS_REFRESH) == 0) {
        return CARD_ROM_STATUS_REFRESHED;
    }
    CARDi_SetRomOp(CARD_ROM_CMD_READ_STATUS, 0);
    reg_MI_MCCNT1 = ((CARD_ROM_HEADER_NORMAL_CONTROL & ~REG_MI_MCCNT1_PC_MASK) | REG_MI_MCCNT1_START_MASK |
                     REG_MI_MCCNT1_RESB_MASK | REG_MI_MCCNT1_PC_4B) &
                    ~REG_MI_MCCNT1_L1_MASK;
    while ((reg_MI_MCCNT1 & REG_MI_MCCNT1_RDY_MASK) == 0) {
    }
    return reg_MI_MCD1;
}

// If the status has a bit of the mask, refreshes the card and waits for the end, sleeping between polls if the threads
// run
static void CARDi_RefreshRom(u32 mask) {
    if ((CARDi_ReadRomStatusCore() & mask) != 0) {
        CARDi_RefreshRomCore();
        while ((CARDi_ReadRomStatusCore() & CARD_ROM_STATUS_REFRESHED) == 0) {
            if (OSi_IsThreadInitialized != 0 && OS_IsAlarmAvailable()) {
                OS_Sleep(1);
            }
        }
    }
}

// Sends the refresh command and waits for the transfer to end
static void CARDi_RefreshRomCore(void) {
    CARDi_SetRomOp(CARD_ROM_CMD_REFRESH, 0);
    reg_MI_MCCNT1 = ((CARD_ROM_HEADER_NORMAL_CONTROL & ~REG_MI_MCCNT1_PC_MASK) | REG_MI_MCCNT1_START_MASK |
                     REG_MI_MCCNT1_RESB_MASK) &
                    ~REG_MI_MCCNT1_L1_MASK;
    while ((reg_MI_MCCNT1 & REG_MI_MCCNT1_START_MASK) != 0) {
    }
}

// CARDi_ReadRomWithCPU: reads a page at a time through the page buffer, which keeps the last page for the next read.
// A whole page for an aligned destination is read straight into it. Returns the length
u32 CARDi_ReadRomWithCPU(void *arg, void *dst, u32 src, u32 len) {
    u32 total = len;
    u32 cachedPage = sCachedPage;
    while (len != 0) {
        u32 *buffer = dst;
        u32 size = CARD_ROM_PAGE_SIZE;
        u32 page = src & ~(CARD_ROM_PAGE_SIZE - 1);
        if (page == cachedPage) {
            buffer = (u32 *)CARDi_cache_buf;
        } else {
            u32 control;
            u32 i;
            if (page != src || ((u32)dst & 3) != 0 || len < size) {
                buffer = (u32 *)CARDi_cache_buf;
                cachedPage = page;
            }
            CARDi_StartRomPageTransfer(page);
            i = 0;
            do {
                control = reg_MI_MCCNT1;
                if ((control & REG_MI_MCCNT1_RDY_MASK) != 0) {
                    u32 word = reg_MI_MCD1;
                    if (i < CARD_ROM_PAGE_SIZE / 4) {
                        buffer[i++] = word;
                    }
                }
            } while ((control & REG_MI_MCCNT1_START_MASK) != 0);
        }
        if (buffer == (u32 *)CARDi_cache_buf) {
            u32 offset = src - page;
            size = CARD_ROM_PAGE_SIZE - offset;
            if (len <= size) {
                size = len;
            }
            MI_CpuCopy8(CARDi_cache_buf + offset, dst, size);
        }
        len -= size;
        dst = (u8 *)dst + size;
        src += size;
    }
    CARDi_CheckPulledOutCore(CARDi_ReadRomIDCore());
    sCachedPage = cachedPage;
    return total;
}

// The interrupt for a page that the DMA channel received: starts the next page, or ends the read
static void CARDi_DmaReadPageCallback(void) {
    CARDiRomDmaRead *read = sRomDmaActive;
    if (read != NULL) {
        read->src += CARD_ROM_PAGE_SIZE;
        read->dst += CARD_ROM_PAGE_SIZE;
        read->len -= CARD_ROM_PAGE_SIZE;
        if (read->len != 0) {
            CARDi_StartRomPageTransfer(read->src);
            return;
        }
        cardi_common.dmaInterface->Stop(cardi_common.dma);
        OS_DisableIrqMask(OS_IE_CARD_DATA);
        OS_ResetRequestIrqMask(OS_IE_CARD_DATA);
        sRomDmaActive = NULL;
        CARDi_CheckPulledOutCore(CARDi_ReadRomIDCore());
        if (read->callback != NULL) {
            read->callback(read->arg);
        }
    }
}

// Has the DMA channel receive the first page, and leaves the rest to the interrupt
void CARDi_ReadRomWithDMA(CARDiRomDmaRead *read) {
    OSIntrMode irq = OS_DisableInterrupts();
    sRomDmaActive = read;
    OS_SetIrqFunction(OS_IE_CARD_DATA, CARDi_DmaReadPageCallback);
    OS_ResetRequestIrqMask(OS_IE_CARD_DATA);
    OS_EnableIrqMask(OS_IE_CARD_DATA);
    OS_RestoreInterrupts(irq);
    cardi_common.dmaInterface->Recv(cardi_common.dma, (void *)&reg_MI_MCD1, (void *)read->dst, CARD_ROM_PAGE_SIZE);
    CARDi_StartRomPageTransfer(read->src);
}

// The callback of a read by DMA: ends the command
static void CARDi_DmaReadDone(void *arg) {
    CARDi_CheckPulledOutCore(CARDi_ReadRomIDCore());
    CARDi_RefreshRom(CARD_ROM_STATUS_NEED_REFRESH);
    cardi_common.cmd->result = CARD_RESULT_SUCCESS;
    CARDi_EndTask(&cardi_common);
}

// Whether a range is outside the instruction TCM, whose image ends where main memory starts
static inline BOOL CARDi_IsOutsideItcm(u32 dst, u32 len) {
    BOOL result = TRUE;
    if (dst + len > HW_ITCM_IMAGE && dst < HW_MAIN_MEM) {
        result = FALSE;
    }
    return result;
}

// Whether a range is outside the data TCM
static inline BOOL CARDi_IsOutsideDtcm(u32 dst, u32 len) {
    BOOL result = TRUE;
    if (dst + len > OS_GetDTCMAddress() && dst < OS_GetDTCMAddress() + HW_DTCM_SIZE) {
        result = FALSE;
    }
    return result;
}

// Whether a DMA channel of the DS can write a range: it starts a cache line of 32 bytes and is outside the ITCM
static inline BOOL CARDi_IsDmaWritable(u32 dma, u32 dst, u32 len) {
    BOOL result = FALSE;
    if (dma <= 3 && len != 0 && (dst & 0x1f) == 0 && CARDi_IsOutsideItcm(dst, len)) {
        result = TRUE;
    }
    return result;
}

// The same, outside the DTCM too
static inline BOOL CARDi_IsDmaReachable(u32 dma, u32 dst, u32 len) {
    BOOL result = FALSE;
    if (CARDi_IsDmaWritable(dma, dst, len) && CARDi_IsOutsideDtcm(dst, len)) {
        result = TRUE;
    }
    return result;
}

// Whether a read can use DMA: the channel and the destination have to suit it, while the source and the length are
// whole pages
static BOOL CARDi_IsRomDmaAvailable(u32 dma, u32 dst, u32 src, u32 len) {
    BOOL result = FALSE;
    if (CARDi_IsDmaReachable(dma, dst, len) && ((src | len) & (CARD_ROM_PAGE_SIZE - 1)) == 0) {
        result = TRUE;
    }
    return result;
}

// The task that reads with the reader on the thread
static void CARDi_ReadRomSyncCore(CARDiCommon *p) {
    sRomReader(NULL, (void *)p->dst, p->src, p->len);
    CARDi_CheckPulledOutCore(CARDi_ReadRomIDCore());
    CARDi_RefreshRom(CARD_ROM_STATUS_NEED_REFRESH);
    cardi_common.cmd->result = CARD_RESULT_SUCCESS;
}

// CARDi_ReadRom: reads len bytes of the ROM from the offset src, by DMA if the channel and the addresses allow it, else
// with the reader on the thread. Waits for the end unless async
void CARDi_ReadRom(u32 dma, const void *src, void *dst, u32 len, MIDmaCallback callback, void *arg, BOOL async) {
    CARDiCommon *p = &cardi_common;
    u32 dmaNo;
    CARD_CheckEnabled();
    if ((CARDi_GetAccessLevel() & CARD_ACCESS_LEVEL_ROM) == 0) {
        OS_Terminate();
    }
    CARDi_WaitForTask(p, TRUE, callback, arg);
    p->dmaInterface = CARDi_GetDmaInterface(dma);
    if (p->dmaInterface != NULL) {
        dmaNo = dma & 3;
    } else {
        dmaNo = (u32)-1;
    }
    p->dma = dmaNo;
    if (dmaNo <= 3) {
        p->dmaInterface->Stop(dmaNo);
    }
    p->src = (u32)src + cardi_rom_base;
    p->dst = (u32)dst;
    p->len = len;
    CARDiDmaReadInfo.callback = CARDi_DmaReadDone;
    CARDiDmaReadInfo.arg = NULL;
    CARDiDmaReadInfo.src = p->src;
    CARDiDmaReadInfo.dst = p->dst;
    CARDiDmaReadInfo.len = p->len;
    CARDiDmaReadInfo.unk_18 = 0;
    if (sRomReader == CARDi_ReadRomWithCPU && CARDi_IsRomDmaAvailable(p->dma, (u32)dst, p->src, len)) {
        OSIntrMode irq = OS_DisableInterrupts();
        if (sInvalidateIC) {
            CARDi_ICInvalidateSmart((void *)p->dst, p->len, p->icInvalidateThreshold);
        }
        if (CARDiEnableCacheInvalidationDC) {
            CARDi_DCInvalidateSmart((void *)p->dst, p->len, p->dcInvalidateThreshold);
        }
        OS_RestoreInterrupts(irq);
        CARDi_ReadRomWithDMA(&CARDiDmaReadInfo);
        if (!async) {
            CARD_WaitRomAsync();
        }
    } else {
        if (sInvalidateIC) {
            CARDi_ICInvalidateSmart((void *)p->dst, p->len, p->icInvalidateThreshold);
        }
        CARDi_ExecuteOldTypeTask(CARDi_ReadRomSyncCore, async);
    }
}

// Has the CPU read the ROM, and on a ROM boot reads the 0x88 bytes after the program
void CARDi_InitRom(void) {
    sRomReader = CARDi_ReadRomWithCPU;
    if (OS_GetBootType() == OS_BOOTTYPE_ROM) {
        if (CARD_GetOwnRomHeader()->rom_size != 0) {
            u16 lockId = OS_GetLockID();
            CARD_LockRom(lockId);
            CARDi_ReadRomWithCPU(NULL, CARDiOwnSignature, CARD_GetOwnRomHeader()->rom_size, sizeof(CARDiOwnSignature));
            CARD_UnlockRom(lockId);
            OS_ReleaseLockID(lockId);
        }
    }
    // The call into the LTD autoload goes through a veneer, which the linker adds after this function
    if (OS_IsRunOnTwl()) {
        CARDi_InitRomHash();
    }
}

// Waits for the last task of the library, which is the one just started
static void CARD_WaitRomAsync(void) {
    CARDi_WaitAsync();
}

// The 0x88 bytes that follow the ROM's program, as CARDi_InitRom read them
const void *CARDi_GetRomCacheBuffer(void) {
    return CARDiOwnSignature;
}

// clang-format off
// TwlSDK's section for the DSi-only code and data that the LTD autoload's .ltdmain holds
#pragma define_section LTDMAIN ".ltdmain" ".ltdmain.bss" ".ltdmain" abs32 RWX
// clang-format on
// The DSi part, in the LTD autoload: the secure ROM reader
#pragma section LTDMAIN begin

const CARDDmaInterface CARDi_NdmaInterface = { MI_Card_NDmaCopy, MI_StopNDma };

static u32 sSecureAreaStart;
static u32 sSecureAreaSize;
static CARDiRomDmaRead sSecureRead;
static CARDRomHashContext sSecureWork;

static u32 CARDi_ReadCardWithHash(void *arg, void *dst, u32 src, u32 len) {
    CARD_ReadRomHashImage(&sSecureWork, dst, src, len);
    return len;
}

static u32 CARDi_ReadCardWithHashInternalAsync(void *arg, void *dst, u32 src, u32 len) {
    if (cardi_common.dma == (u32)-1) {
        len = 0;
    } else {
        DC_FlushRange(dst, len);
        sSecureRead.callback = CARD_NotifyRomHashReadAsync;
        sSecureRead.arg = arg;
        sSecureRead.src = src;
        sSecureRead.dst = (u32)dst;
        sSecureRead.len = len;
        sSecureRead.unk_00 = 0;
        sSecureRead.unk_18 = 0;
        CARDi_ReadRomWithDMA(&sSecureRead);
    }
    return len;
}

void CARDi_InitRomHash(void) {
    u32 arenaLo = ((u32)OS_GetArenaLo(OS_ARENA_MAIN) + 31) & ~31;
    u32 arenaHi = (u32)OS_GetArenaHi(OS_ARENA_MAIN);
    const CARDTwlRomHeader *header = CARD_GetOwnRomHeaderTWL();
    u32 workSize = CARD_CalcRomHashBufferLength(header);

    arenaHi &= ~31;
    if (arenaLo + workSize > arenaHi) {
        OS_Terminate();
        return;
    }

    OS_SetArenaLo(OS_ARENA_MAIN, (void *)(arenaLo + workSize));
    sSecureAreaStart = arenaLo;
    sSecureAreaSize = workSize;

    if (OS_GetBootType() == OS_BOOTTYPE_ROM && (CARD_TWL_BOOT_FLAGS & 1)) {
        cardi_common.dma = (u32)-1;
        sRomReader = CARDi_ReadCardWithHash;

        u16 lockId = OS_GetLockID();
        CARD_LockRom(lockId);

        header = CARD_GetOwnRomHeaderTWL();
        CARD_InitRomHashContext(&sSecureWork, header, (void *)sSecureAreaStart, sSecureAreaSize, CARDi_ReadRomWithCPU,
                                CARDi_ReadCardWithHashInternalAsync, &sSecureWork);

        sSecureAreaStart = 0;
        sSecureAreaSize = 0;
        CARD_UnlockRom(lockId);
        OS_ReleaseLockID(lockId);
    }
}

#pragma section LTDMAIN end
