// TwlSDK OS: the arguments that a program leaves for the one that starts next (os_deliverArg.c, a guessed name after
// TwlSDK's DeliverArg API, which the code resembles). A 0x300-byte block at the start of the main memory, which
// survives the reset, holds the ID and maker code of the program that wrote it and a CRC16 of the block.
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/svc.h"

// Where the block is left for the next program, in the main memory
#define HW_DELIVER_ARG_BUF 0x02000000
// The maker code of the ROM header that the system puts in the main memory (the TWL ROM header's, at 0x10)
#define HW_TWL_MAKER_CODE 0x02ffe010
// The state of the buffer: bit 0 is set while it is set up and can be sent, or has the block that was taken
// (OS_InitDeliverArg sets bits 0 and 1). The flags of the block: bit 1 is set in a block that is left for the next
// program, bit 0 as well, until the receiver takes the block and clears it
#define OSi_DELIVER_ARG_STATE_READY 0x1
#define OSi_DELIVER_ARG_STATE_SET_UP 0x3
#define OSi_DELIVER_ARG_FLAGS_DELIVERED 0x2
#define OSi_DELIVER_ARG_FLAGS_PENDING 0x1

static BOOL OS_IsValidDeliverArg(void);

// A copy of the title ID of the program that sent the block
static u64 OSi_deliverArgTitleId;

static struct {
    u8 *args;
    u32 state;
    OSDeliverArgInfo *info;
} OSi_deliverArg;

// Sets the buffer up with an empty list of arguments, leaving reserved bytes of it for something else
void OS_InitDeliverArg(OSDeliverArgInfo *info, u32 reserved) {
    OSi_deliverArg.info = info;
    MI_CpuFill8(info, 0, sizeof(OSDeliverArgInfo));
    OSi_deliverArg.state = OSi_DELIVER_ARG_STATE_SET_UP;
    OSi_deliverArg.info->argSpace = sizeof(info->args) - reserved;
    OSi_deliverArg.info->argCount = 0;
    OSi_deliverArg.args = OSi_deliverArg.info->args;
    OSi_deliverArg.args[0] = 0;
    OSi_deliverArg.args[1] = 0;
}

// Puts the buffer in the block for the next program, with this program's title and maker code, and clears it. Gives -1
// when the buffer was not set up for it
int OS_EncodeDeliverArg(void) {
    OSDeliverArgInfo *info;

    if (!(OSi_deliverArg.state & OSi_DELIVER_ARG_STATE_READY)) {
        return -1;
    }
    OSi_deliverArg.info->makerCode = *(u16 *)HW_TWL_MAKER_CODE;
    info = OSi_deliverArg.info;
    info->titleId = *(u64 *)HW_TWL_TITLE_ID;
    info->flags = OSi_DELIVER_ARG_FLAGS_DELIVERED | OSi_DELIVER_ARG_FLAGS_PENDING;
    OSi_deliverArgTitleId = OSi_deliverArg.info->titleId;
    OSi_deliverArg.info->crc16 = 0;
    OSi_deliverArg.info->crc16 = SVC_GetCRC16(0xffff, OSi_deliverArg.info, sizeof(OSDeliverArgInfo));
    MI_CpuCopy8(OSi_deliverArg.info, (void *)HW_DELIVER_ARG_BUF, sizeof(OSDeliverArgInfo));
    MI_CpuFill8(OSi_deliverArg.info, 0, sizeof(OSDeliverArgInfo));
    OSi_deliverArg.state = 0;
    return 0;
}

// Takes the block that the last program left into the buffer, if there is one and its CRC16 is right. Gives -1 when
// there is no block or no buffer
int OS_DecodeDeliverArg(void) {
    u16 crc;

    if (!OS_IsValidDeliverArg()) {
        return -1;
    }
    if (OSi_deliverArg.info == NULL) {
        return -1;
    }
    MI_CpuCopy8((void *)HW_DELIVER_ARG_BUF, OSi_deliverArg.info, sizeof(OSDeliverArgInfo));
    crc = OSi_deliverArg.info->crc16;
    OSi_deliverArg.info->crc16 = 0;
    OSi_deliverArg.info->crc16 = SVC_GetCRC16(0xffff, OSi_deliverArg.info, sizeof(OSDeliverArgInfo));
    if (crc != OSi_deliverArg.info->crc16) {
        MI_CpuFill8(OSi_deliverArg.info, 0, sizeof(OSDeliverArgInfo));
        OS_FinalizeDeliverArg();
        return 0;
    }
    OSi_deliverArg.info->flags &= ~OSi_DELIVER_ARG_FLAGS_PENDING;
    OSi_deliverArg.state = OSi_DELIVER_ARG_STATE_READY;
    return 0;
}

// Forgets the buffer, so that nothing is sent or read
void OS_FinalizeDeliverArg(void) {
    OSi_deliverArg.state = 0;
}

// The title ID of the program that sent the block, or 0 when it was not taken
u64 OS_GetTitleIdFromDeliverArg(void) {
    if (OSi_deliverArg.state & OSi_DELIVER_ARG_STATE_READY) {
        return OSi_deliverArg.info->titleId;
    }
    return 0;
}

// Whether a program left a block for this one
static BOOL OS_IsValidDeliverArg(void) {
    OSDeliverArgInfo *info = (OSDeliverArgInfo *)HW_DELIVER_ARG_BUF;

    return (info->flags & OSi_DELIVER_ARG_FLAGS_DELIVERED) != 0;
}
