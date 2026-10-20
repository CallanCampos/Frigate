// TwlSDK's DSi card ROM setup in the LTD autoload. The file name is inferred from NitroSDK's card_rom.c.
#include "nitro/os.h"
#include "twl/card.h"
#include "twl/mi.h"

// The boot code leaves the card security flag in shared memory.
#define CARD_TWL_BOOT_FLAGS (*(vu8 *)0x02ffe01c)
#define CARD_SECURE_WORK_SIZE 0x70

static u32 CARDi_ReadSecureRom(void *arg, void *dst, u32 src, u32 len);
static u32 CARDi_ReadSecureRomWithDMA(void *arg, void *dst, u32 src, u32 len);

const CARDDmaInterface CARDi_NdmaInterface = { func_02768370, MI_StopNDma };

static u32 sSecureAreaStart;
static u32 sSecureAreaSize;
static CARDiRomDmaRead sSecureRead;
static u8 sSecureWork[CARD_SECURE_WORK_SIZE];

static u32 CARDi_ReadSecureRom(void *arg, void *dst, u32 src, u32 len) {
    func_02767e84(sSecureWork, dst, src, len);
    return len;
}

static u32 CARDi_ReadSecureRomWithDMA(void *arg, void *dst, u32 src, u32 len) {
    if (cardi_common.dma == (u32)-1) {
        len = 0;
    } else {
        DC_FlushRange(dst, len);
        sSecureRead.callback = func_0276787c;
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
    u32 arenaLo = ((u32)OS_GetArenaLo(OS_ARENA_MAIN) + 31) & ~31;
    u32 arenaHi = (u32)OS_GetArenaHi(OS_ARENA_MAIN);
    const CARDTwlRomHeader *header = CARD_GetOwnRomHeaderTWL();
    u32 workSize = func_027676b4(header);

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
        func_02767a14(sSecureWork, header, sSecureAreaStart, sSecureAreaSize, CARDi_ReadRomWithCPU,
                      CARDi_ReadSecureRomWithDMA, sSecureWork);

        sSecureAreaStart = 0;
        sSecureAreaSize = 0;
        CARD_UnlockRom(lockId);
        OS_ReleaseLockID(lockId);
    }
}
