// NitroSDK's CARD library, the game card's ROM, on the host: the decomp's lib/nitro/src/card/card_rom.c (which
// port/hal/card/replaced.txt keeps out of the game's sources) reads the card a page at a time through its registers
// (MCCNT, MCCMD, MCD1), with the CPU or a DMA channel that the card-data interrupt moves on; here a read copies from
// the player's ROM file (port/hal/rom). The API and its order of events stay the SDK's: a read takes the library's task
// (CARDi_WaitForTask), runs on the caller's thread when it waits and on the library's thread (priority 4) when it
// doesn't, and ends the task, which calls the callback. A read that the DS would do by DMA runs as a task too: it ends
// before CARDi_ReadRom returns when the card's thread outranks the caller, as it outranks the game's own threads (the
// launcher's 16, the sound loader's 17), and the FS library takes both orders, as it takes the DS's interrupt arriving
// before the call returns.
//
// What changes besides the registers: the destination stays a host pointer (the DS keeps it in CARDiCommon's 32-bit
// dst, which would cut it), and every read reports the host's card ID (port_rom_card_id), so the pull-out check after
// each read never fires. What a read gives is what a retail card answers (port_rom_card_read): addresses wrap at the
// card's capacity, and the header and secure area below 0x8000 read as the block at 0x8000. NitroSDK's
// CARDi_ReadRomWithDMA, which only TwlSDK's DSi card code in the LTD autoload calls, is left out: its request keeps the
// destination in 32 bits, so a call would stop at its trap stub.
#include "hal/card/port_card.h"
#include "hal/rom/port_rom.h"
#include "nitro/card.h"
#include "nitro/os.h"
#include "twl/card.h"

// The 0x88 bytes that follow the ROM's program (card_rom.c's CARDiOwnSignature)
#define CARD_ROM_AUTH_CODE_SIZE 0x88

static void CARDi_ReadRomTask(CARDiCommon *p);

// The offset of the ROM in the card's address space, which card_common.c clears
u32 cardi_rom_base;
// How the library's tasks read the ROM: CARDi_ReadRomWithCPU, which TwlSDK's DSi setup would swap for its own reader
CARDiRomReader sRomReader;
// The 0x88 bytes after the ROM's program, read at initialization on a boot from the card; the multiboot library sends
// them to its children (mb_segment.c)
static u8 sRomAuthCode[CARD_ROM_AUTH_CODE_SIZE];
// Where the read in progress writes: the host pointer that CARDiCommon's 32-bit dst can't hold
static void *sReadDst;

// NitroSDK's CARDi_ReadRomWithCPU: reads len bytes of the card from the address src into dst, and checks the card's
// ID as the DS does after a read. Returns the length
u32 CARDi_ReadRomWithCPU(void *arg, void *dst, u32 src, u32 len) {
    port_rom_card_read(dst, src, len);
    CARDi_CheckPulledOutCore(port_rom_card_id());
    return len;
}

// The task that reads with the reader, on the thread that runs it
static void CARDi_ReadRomTask(CARDiCommon *p) {
    sRomReader(NULL, sReadDst, p->src, p->len);
    cardi_common.cmd->result = CARD_RESULT_SUCCESS;
}

// NitroSDK's CARDi_ReadRom: reads len bytes of the ROM from the offset src into dst, calling the callback with its
// argument at the end, and waits for the end unless async. The DMA channel's number only chose, on the DS, between a
// read by DMA and one by the CPU; a host read is the same either way
void CARDi_ReadRom(u32 dma, const void *src, void *dst, u32 len, MIDmaCallback callback, void *arg, BOOL async) {
    CARDiCommon *p = &cardi_common;

    CARD_CheckEnabled();
    if ((CARDi_GetAccessLevel() & CARD_ACCESS_LEVEL_ROM) == 0) {
        OS_Terminate();
    }
    CARDi_WaitForTask(p, TRUE, callback, arg);
    p->dmaInterface = CARDi_GetDmaInterface(dma);
    p->dma = p->dmaInterface != NULL ? dma & 3 : (u32)-1;
    // The SDK pointer carries a 32-bit cartridge offset, never a host-memory address.
    p->src = (u32)(uptr)src + cardi_rom_base;
    p->dst = 0;
    p->len = len;
    sReadDst = dst;
    CARDi_ExecuteOldTypeTask(CARDi_ReadRomTask, async);
}

// NitroSDK's CARDi_InitRom: reads with the CPU, and on a boot from the card reads the 0x88 bytes after the program.
// It is the first point of a run where the card needs the ROM and the ARM7, so it makes sure that the ROM is open,
// that the system area holds what the DS's boot leaves there (port_rom_boot) and that the ARM7's card services are
// registered (port_card_boot), in case the program didn't do it before running the game; CARD_Init's copy of the
// header, which came just before, is made again with the system area
void CARDi_InitRom(void) {
    port_rom_boot();
    port_card_boot();
    sRomReader = CARDi_ReadRomWithCPU;
    if (OS_GetBootType() == OS_BOOTTYPE_ROM) {
        if (CARD_GetOwnRomHeader()->rom_size != 0) {
            u16 lockId = OS_GetLockID();
            CARD_LockRom(lockId);
            CARDi_ReadRomWithCPU(NULL, sRomAuthCode, CARD_GetOwnRomHeader()->rom_size, sizeof(sRomAuthCode));
            CARD_UnlockRom(lockId);
            OS_ReleaseLockID(lockId);
        }
    }
    if (OS_IsRunOnTwl()) {
        CARDi_InitRomHash();
    }
}

// The 0x88 bytes that follow the ROM's program, as CARDi_InitRom read them
const void *CARDi_GetRomAuthCode(void) {
    return sRomAuthCode;
}
