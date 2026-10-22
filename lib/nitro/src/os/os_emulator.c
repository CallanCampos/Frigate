// NitroSDK/TwlSDK OS: the console type and whether the program runs on a DSi. The file name is a guess, from
// OS_IsRunOnEmulator, which the console type has a bit for (the SDK's os_emulator.c); whether the DSi check belongs
// here or in a file of its own that has the last 0x20 bytes of .bss is not known. Built with -ipa file: the file's
// statics are reached from one base.
#include "nitro/os.h"

// SCFG_A9ROM, whose low two bits say that the program runs in the DSi's mode (the DS compatible mode leaves them 0)
#define reg_SCFG_A9ROM (*(vu8 *)0x04004000)

// The console type that OS_GetConsoleType starts with on a DSi and on a DS, and the bits that the way the program was
// booted adds to it (indexed by OS_GetBootType)
#define OS_CONSOLE_TWL_BASE 0x01800004
#define OS_CONSOLE_NITRO_BASE 0x80000001
#define OSi_CONSOLE_NOT_DETECT 0xffffffff

static u32 func_0207ac60(void);

// The file's words in .data and .bss are more than its code reads: the link removed the functions that used the others
static struct {
    u32 unk_00;
    u32 consoleType;
} sConsole = { 0xffffffff, OSi_CONSOLE_NOT_DETECT };

static struct {
    u32 unk_00;
    BOOL isTwl;
    u32 unk_08[5];
    BOOL isTwlChecked;
} sTwl;

static const u32 sBootTypeConsoleBits[5] = {
    0, 0x00020000, 0x00004000, 0x00040000, 0x00008000,
};

// NitroSDK's OS_GetConsoleType: the console type, worked out once
u32 OS_GetConsoleType(void) {
    if (sConsole.consoleType == OSi_CONSOLE_NOT_DETECT) {
        if (OS_IsRunOnTwl()) {
            sConsole.consoleType = OS_CONSOLE_TWL_BASE;
        } else {
            sConsole.consoleType = OS_CONSOLE_NITRO_BASE;
        }
        sConsole.consoleType |= func_0207ac60();
    }
    return sConsole.consoleType;
}

static u32 func_0207ac60(void) {
    return sBootTypeConsoleBits[OS_GetBootType()];
}

BOOL OS_IsRunOnEmulator(void) {
    if (OS_GetConsoleType() & OS_CONSOLE_ENSATA) {
        return TRUE;
    }
    return FALSE;
}

// TwlSDK's OS_IsRunOnTwl (swan's hw_isDSi): worked out once
BOOL OS_IsRunOnTwl(void) {
    if (!sTwl.isTwlChecked) {
        sTwl.isTwl = (reg_SCFG_A9ROM & 3) == 1;
        sTwl.isTwlChecked = TRUE;
    }
    return sTwl.isTwl;
}

BOOL func_0207acbc(void) {
    return OS_IsRunOnTwl();
}
