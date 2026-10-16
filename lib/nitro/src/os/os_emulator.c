// NitroSDK/TwlSDK OS: the console type and whether the program runs on a DSi. The file name is a guess, from
// OS_IsRunOnEmulator, which the console type has a bit for (the SDK's os_emulator.c); whether the DSi check belongs
// here or in a file of its own that has the last 0x20 bytes of .bss is not known. Built with -ipa file: the file's
// statics are reached from one base.
#include "nitro/os.h"

// SCFG_A9ROM, whose low two bits say that the program runs in the DSi's mode (the DS compatible mode leaves them 0)
#define reg_SCFG_A9ROM (*(vu8 *)0x04004000)

// The console type that OS_GetConsoleType starts with on a DSi and on a DS, and the bits that the way the program was
// booted adds to it (indexed by sys_get_boot_indicator)
#define OS_CONSOLE_TWL_BASE 0x01800004
#define OS_CONSOLE_NITRO_BASE 0x80000001
#define OS_CONSOLE_UNKNOWN 0xffffffff

static u32 func_0207ac60(void);

// The file's words in .data and .bss are more than its code reads: the link removed the functions that used the others
static struct {
    u32 unk_00;
    u32 consoleType;
} sConsole = { 0xffffffff, OS_CONSOLE_UNKNOWN };

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
u32 func_0207ac24(void) {
    if (sConsole.consoleType == OS_CONSOLE_UNKNOWN) {
        if (hw_isDSi()) {
            sConsole.consoleType = OS_CONSOLE_TWL_BASE;
        } else {
            sConsole.consoleType = OS_CONSOLE_NITRO_BASE;
        }
        sConsole.consoleType |= func_0207ac60();
    }
    return sConsole.consoleType;
}

static u32 func_0207ac60(void) {
    return sBootTypeConsoleBits[sys_get_boot_indicator()];
}

// NitroSDK's OS_IsRunOnEmulator
BOOL func_0207ac74(void) {
    if (func_0207ac24() & OS_CONSOLE_ENSATA) {
        return TRUE;
    }
    return FALSE;
}

// TwlSDK's OS_IsRunOnTwl, under swan's name hw_isDSi: worked out once
BOOL hw_isDSi(void) {
    if (!sTwl.isTwlChecked) {
        sTwl.isTwl = (reg_SCFG_A9ROM & 3) == 1;
        sTwl.isTwlChecked = TRUE;
    }
    return sTwl.isTwl;
}

BOOL func_0207acbc(void) {
    return hw_isDSi();
}
