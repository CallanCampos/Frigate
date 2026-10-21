// TwlSDK's DSi card ROM setup in the LTD autoload. The file name is inferred from NitroSDK's card_rom.c.
#include "nitro/os.h"
#include "twl/card.h"
#include "twl/mi.h"

// The boot code leaves the card security flag in shared memory.
#define CARD_TWL_BOOT_FLAGS (*(vu8 *)0x02ffe01c)

static u32 CARDi_ReadSecureRom(void *arg, void *dst, u32 src, u32 len);
static u32 CARDi_ReadSecureRomWithDMA(void *arg, void *dst, u32 src, u32 len);

const CARDDmaInterface CARDi_NdmaInterface = { MIi_CardNDmaRecv, MI_StopNDma };

static uptr sSecureAreaStart;
static u32 sSecureAreaSize;
static CARDiRomDmaRead sSecureRead;
static CARDCheckedReadContext sSecureWork;

static u32 CARDi_ReadSecureRom(void *arg, void *dst, u32 src, u32 len) {
    CARD_ReadRomChecked(&sSecureWork, dst, src, len);
    return len;
}

static u32 CARDi_ReadSecureRomWithDMA(void *arg, void *dst, u32 src, u32 len) {
    if (cardi_common.dma == (u32)-1) {
        len = 0;
    } else {
        DC_FlushRange(dst, len);
        sSecureRead.callback = CARD_OnCheckedReadDone;
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

void CARDi_InitTwlRom(void) {
    uptr arenaLo = ((uptr)OS_GetArenaLo(OS_ARENA_MAIN) + 31) & ~31;
    uptr arenaHi = (uptr)OS_GetArenaHi(OS_ARENA_MAIN);
    const CARDTwlRomHeader *header = CARD_GetOwnRomHeaderTWL();
    u32 workSize = CARD_GetCheckedReadBufferSize(header);

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
        sRomReader = CARDi_ReadSecureRom;

        u16 lockId = OS_GetLockID();
        CARD_LockRom(lockId);

        header = CARD_GetOwnRomHeaderTWL();
        CARD_SetUpCheckedReads(&sSecureWork, header, (void *)sSecureAreaStart, sSecureAreaSize, CARDi_ReadRomWithCPU,
                               CARDi_ReadSecureRomWithDMA, &sSecureWork);

        sSecureAreaStart = 0;
        sSecureAreaSize = 0;
        CARD_UnlockRom(lockId);
        OS_ReleaseLockID(lockId);
    }
}
