// NitroSDK's console type and TwlSDK's DSi check (the decomp's lib/nitro/src/os/os_emulator.c) on the host, which is a
// retail DS: not a DSi, not a development unit, with 4 MB of memory. The decomp's file reads the DSi check from
// SCFG_A9ROM through a raw address in the C file, which the host's header translation doesn't reach (ADR 0004); the
// host answers it itself, and the console type is worked out as the SDK does from that and the boot type.
#include "hal/os/os_internal.h"
#include "nitro/os.h"

// The console type that OS_GetConsoleType starts with on a DSi and on a DS, and the bits that the way the program was
// booted adds to it (indexed by OS_GetBootType)
#define OS_CONSOLE_TWL_BASE 0x01800004
#define OS_CONSOLE_NITRO_BASE 0x80000001
#define OS_CONSOLE_UNKNOWN 0xffffffff

static u32 sConsoleType = OS_CONSOLE_UNKNOWN;

void port_osi_reset_console(void) {
    sConsoleType = OS_CONSOLE_UNKNOWN;
}

static const u32 sBootTypeConsoleBits[5] = {
    0, 0x00020000, 0x00004000, 0x00040000, 0x00008000,
};

// NitroSDK's OS_GetConsoleType: the console type, worked out once
u32 OS_GetConsoleType(void) {
    if (sConsoleType == OS_CONSOLE_UNKNOWN) {
        u32 bootType = OS_GetBootType();

        sConsoleType = OS_IsRunOnTwl() ? OS_CONSOLE_TWL_BASE : OS_CONSOLE_NITRO_BASE;
        sConsoleType |= bootType < NELEMS(sBootTypeConsoleBits) ? sBootTypeConsoleBits[bootType] : 0;
    }
    return sConsoleType;
}

BOOL OS_IsRunOnEmulator(void) {
    if (OS_GetConsoleType() & OS_CONSOLE_ENSATA) {
        return TRUE;
    }
    return FALSE;
}

// TwlSDK's OS_IsRunOnTwl (swan's hw_isDSi): the host is a DS
BOOL OS_IsRunOnTwl(void) {
    return FALSE;
}

BOOL OSi_IsRunOnTwl(void) {
    return OS_IsRunOnTwl();
}
