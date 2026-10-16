#ifndef POKEBW2_NITRO_CTRDG_H
#define POKEBW2_NITRO_CTRDG_H

#include "types.h"

// NitroSDK's cartridge library (libctrdg): the slot 2 of the DS, for GBA cartridges and other devices. The functions
// keep their default names; the comments give the SDK functions they appear to be

// What the console's start-up code noted about the cartridge, in the part of main memory the DSi mirrors at this
// address (the DS has it at 0x027ffc30): the halfword of the cartridge's ROM that identifies it, the flags and the
// game code. Only the fields that the library reads are named
#define HW_CTRDG_MODULE_INFO_BUF 0x02fffc30

typedef struct {
    u16 romCheck; // The halfword that the ROM had at the address that the library checks
    u8 unk_02[3];
    u8 isAgbCartridge : 1; // The ROM has the header of a GBA cartridge, so its game code can be compared
    u8 pulledOut : 1;      // The library found that the cartridge was pulled out or exchanged
    u8 unk_05 : 6;
    u8 unk_06[2];
    u32 gameCode; // The cartridge's game code
} CTRDGModuleInfo;

#define CTRDG_MODULE_INFO ((CTRDGModuleInfo *)HW_CTRDG_MODULE_INFO_BUF)

// ctrdg.c
// CTRDGi_InitCommon: gets the lock ID for the cartridge bus
void func_02082624(void);
// CTRDG_IsExisting: whether the cartridge that was there at the start is still there
BOOL func_0208264c(void);
// CTRDGi_SendtoPxi: sends a word to the ARM7 with the tag of the cartridge, until the FIFO takes it
void func_020827a0(u32 data);

// ctrdg_proc.c
// CTRDG_DummyInit: the library's start-up as the game does it: takes the lock ID, replaces the ARM7's tag handler by
// one that ignores its words, and sends the ARM7 the first word. The game calls it once, at the start
void func_020827d0(void);

// CTRDG_Init, which OS_Init calls: a weak function of the SDK, which the game replaces (system/machine_use.c) with one
// that calls CTRDG_DummyInit and leaves the slot unused
void func_0200548c(void);

#endif // POKEBW2_NITRO_CTRDG_H
