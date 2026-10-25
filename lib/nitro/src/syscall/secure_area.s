// NitroSDK's system call library (libsyscall.a) as this game has it: the 2 KB at the start of ARM9 main, which the
// card's secure area holds. The library is hand-written assembly: BIOS call stubs in Thumb, each an SWI and a return,
// which mwasmarm assembles (library.toml's asm_flags). Nintendo gives each game its own build of it, with the stubs at
// places of their own among filler bytes, so Black 2 and White 2 have them in different orders and with other filler.
// The file name is descriptive, since the library's object name isn't known; the names are NitroSDK's, with swan's in
// nitro/svc.h.
//
// The filler is the base ROM's, which no source gives and which isn't committed: the build writes each version's from
// its extracted ROM to rom/syscall/secure_area_<version>.inc of the generated headers (configure.py's ROM_RANGES,
// tools/build/rom_blob.py) as the macro FILLER, which gives the bytes from one offset of the secure area to another.
// Without a ROM, compile_all.py writes zeros instead. The stubs start at the ROM's addresses, 11 of them in Black 2 and
// 7 in White 2 at 2 bytes past a multiple of 4, which a C asm function can't, since MWCC aligns each function to 4.

	.text
	.thumb

// FILLER start, end: the base ROM's bytes from offset start to offset end of the secure area
#ifdef BLACK2

#include "rom/syscall/secure_area_b2_us.inc"

	FILLER 0x000, 0x064

// SVC_UnpackBits: BIOS call 0x10 (0x02004064)
	.global SVC_UnpackBits
	.type SVC_UnpackBits, @function
SVC_UnpackBits:
	swi 0x10
	bx lr
	.size SVC_UnpackBits, .-SVC_UnpackBits
	FILLER 0x068, 0x0d0

// SVC_UncompressLZ8: BIOS call 0x11 (0x020040d0)
	.global SVC_UncompressLZ8
	.type SVC_UncompressLZ8, @function
SVC_UncompressLZ8:
	swi 0x11
	bx lr
	.size SVC_UncompressLZ8, .-SVC_UncompressLZ8
	FILLER 0x0d4, 0x13a

// SVC_GetCRC16: BIOS call 0x0e (0x0200413a)
	.global SVC_GetCRC16
	.type SVC_GetCRC16, @function
SVC_GetCRC16:
	swi 0xe
	bx lr
	.size SVC_GetCRC16, .-SVC_GetCRC16
	FILLER 0x13e, 0x190

// SVC_CpuSet: BIOS call 0x0b (0x02004190)
	.global SVC_CpuSet
	.type SVC_CpuSet, @function
SVC_CpuSet:
	swi 0xb
	bx lr
	.size SVC_CpuSet, .-SVC_CpuSet
	FILLER 0x194, 0x1f0

// SVC_UncompressRL8: BIOS call 0x14 (0x020041f0)
	.global SVC_UncompressRL8
	.type SVC_UncompressRL8, @function
SVC_UncompressRL8:
	swi 0x14
	bx lr
	.size SVC_UncompressRL8, .-SVC_UncompressRL8
	FILLER 0x1f4, 0x26e

// SVC_UncompressLZ16FromDevice: BIOS call 0x12 (0x0200426e)
	.global SVC_UncompressLZ16FromDevice
	.type SVC_UncompressLZ16FromDevice, @function
SVC_UncompressLZ16FromDevice:
	swi 0x12
	bx lr
	.size SVC_UncompressLZ16FromDevice, .-SVC_UncompressLZ16FromDevice
	FILLER 0x272, 0x2e4

// SVC_Div: BIOS call 0x09, which leaves the quotient in r0 (0x020042e4)
	.global SVC_Div
	.type SVC_Div, @function
SVC_Div:
	swi 0x9
	bx lr
	.size SVC_Div, .-SVC_Div
	FILLER 0x2e8, 0x352

// SVC_WaitVBlankIntr: BIOS call 0x05, with r2 = 0 (0x02004352)
	.global SVC_WaitVBlankIntr
	.type SVC_WaitVBlankIntr, @function
SVC_WaitVBlankIntr:
	mov r2, #0x0
	swi 0x5
	bx lr
	.size SVC_WaitVBlankIntr, .-SVC_WaitVBlankIntr
	FILLER 0x358, 0x3ca

// SVC_IsMmemExpanded: BIOS call 0x0f (0x020043ca)
	.global SVC_IsMmemExpanded
	.type SVC_IsMmemExpanded, @function
SVC_IsMmemExpanded:
	swi 0xf
	bx lr
	.size SVC_IsMmemExpanded, .-SVC_IsMmemExpanded
	FILLER 0x3ce, 0x424

// SVC_UncompressHuffmanFromDevice: BIOS call 0x13 (0x02004424)
	.global SVC_UncompressHuffmanFromDevice
	.type SVC_UncompressHuffmanFromDevice, @function
SVC_UncompressHuffmanFromDevice:
	swi 0x13
	bx lr
	.size SVC_UncompressHuffmanFromDevice, .-SVC_UncompressHuffmanFromDevice
	FILLER 0x428, 0x48a

// SVC_SoftReset: BIOS call 0x00 (0x0200448a)
	.global SVC_SoftReset
	.type SVC_SoftReset, @function
SVC_SoftReset:
	swi 0x0
	bx lr
	.size SVC_SoftReset, .-SVC_SoftReset
	FILLER 0x48e, 0x4f6

// SVC_WaitByLoop: BIOS call 0x03 (0x020044f6)
	.global SVC_WaitByLoop
	.type SVC_WaitByLoop, @function
SVC_WaitByLoop:
	swi 0x3
	bx lr
	.size SVC_WaitByLoop, .-SVC_WaitByLoop
	FILLER 0x4fa, 0x582

// SVC_Sqrt: BIOS call 0x0d (0x02004582)
	.global SVC_Sqrt
	.type SVC_Sqrt, @function
SVC_Sqrt:
	swi 0xd
	bx lr
	.size SVC_Sqrt, .-SVC_Sqrt
	FILLER 0x586, 0x5f4

// SVC_CpuSetFast: BIOS call 0x0c (0x020045f4)
	.global SVC_CpuSetFast
	.type SVC_CpuSetFast, @function
SVC_CpuSetFast:
	swi 0xc
	bx lr
	.size SVC_CpuSetFast, .-SVC_CpuSetFast
	FILLER 0x5f8, 0x666

// SVC_Halt: BIOS call 0x06 (0x02004666)
	.global SVC_Halt
	.type SVC_Halt, @function
SVC_Halt:
	swi 0x6
	bx lr
	.size SVC_Halt, .-SVC_Halt
	FILLER 0x66a, 0x6ba

// SVC_DivRem: BIOS call 0x09, returning the remainder that it leaves in r1 (0x020046ba)
	.global SVC_DivRem
	.type SVC_DivRem, @function
SVC_DivRem:
	swi 0x9
	mov r0, r1
	bx lr
	.size SVC_DivRem, .-SVC_DivRem
	FILLER 0x6c0, 0x726

// SVC_UncompressRL16FromDevice: BIOS call 0x15 (0x02004726)
	.global SVC_UncompressRL16FromDevice
	.type SVC_UncompressRL16FromDevice, @function
SVC_UncompressRL16FromDevice:
	swi 0x15
	bx lr
	.size SVC_UncompressRL16FromDevice, .-SVC_UncompressRL16FromDevice
	FILLER 0x72a, 0x78c

// SVC_WaitIntr: BIOS call 0x04 (0x0200478e). Its stub starts 2 bytes earlier, in both versions, with mov r2, #0 as
// SVC_WaitVBlankIntr's does: a first entry, which no symbol names and nothing in the ROM calls
	mov r2, #0x0
	.global SVC_WaitIntr
	.type SVC_WaitIntr, @function
SVC_WaitIntr:
	swi 0x4
	bx lr
	.size SVC_WaitIntr, .-SVC_WaitIntr
	FILLER 0x792, 0x800

#else

#include "rom/syscall/secure_area_w2_us.inc"

	FILLER 0x000, 0x06a

// SVC_GetCRC16: BIOS call 0x0e (0x0200406a)
	.global SVC_GetCRC16
	.type SVC_GetCRC16, @function
SVC_GetCRC16:
	swi 0xe
	bx lr
	.size SVC_GetCRC16, .-SVC_GetCRC16
	FILLER 0x06e, 0x0e2

// SVC_WaitIntr: BIOS call 0x04 (0x020040e4). Its stub starts 2 bytes earlier, in both versions, with mov r2, #0 as
// SVC_WaitVBlankIntr's does: a first entry, which no symbol names and nothing in the ROM calls
	mov r2, #0x0
	.global SVC_WaitIntr
	.type SVC_WaitIntr, @function
SVC_WaitIntr:
	swi 0x4
	bx lr
	.size SVC_WaitIntr, .-SVC_WaitIntr
	FILLER 0x0e8, 0x164

// SVC_Halt: BIOS call 0x06 (0x02004164)
	.global SVC_Halt
	.type SVC_Halt, @function
SVC_Halt:
	swi 0x6
	bx lr
	.size SVC_Halt, .-SVC_Halt
	FILLER 0x168, 0x1cc

// SVC_IsMmemExpanded: BIOS call 0x0f (0x020041cc)
	.global SVC_IsMmemExpanded
	.type SVC_IsMmemExpanded, @function
SVC_IsMmemExpanded:
	swi 0xf
	bx lr
	.size SVC_IsMmemExpanded, .-SVC_IsMmemExpanded
	FILLER 0x1d0, 0x224

// SVC_UncompressRL8: BIOS call 0x14 (0x02004224)
	.global SVC_UncompressRL8
	.type SVC_UncompressRL8, @function
SVC_UncompressRL8:
	swi 0x14
	bx lr
	.size SVC_UncompressRL8, .-SVC_UncompressRL8
	FILLER 0x228, 0x280

// SVC_WaitByLoop: BIOS call 0x03 (0x02004280)
	.global SVC_WaitByLoop
	.type SVC_WaitByLoop, @function
SVC_WaitByLoop:
	swi 0x3
	bx lr
	.size SVC_WaitByLoop, .-SVC_WaitByLoop
	FILLER 0x284, 0x2e6

// SVC_CpuSet: BIOS call 0x0b (0x020042e6)
	.global SVC_CpuSet
	.type SVC_CpuSet, @function
SVC_CpuSet:
	swi 0xb
	bx lr
	.size SVC_CpuSet, .-SVC_CpuSet
	FILLER 0x2ea, 0x350

// SVC_SoftReset: BIOS call 0x00 (0x02004350)
	.global SVC_SoftReset
	.type SVC_SoftReset, @function
SVC_SoftReset:
	swi 0x0
	bx lr
	.size SVC_SoftReset, .-SVC_SoftReset
	FILLER 0x354, 0x3a8

// SVC_Sqrt: BIOS call 0x0d (0x020043a8)
	.global SVC_Sqrt
	.type SVC_Sqrt, @function
SVC_Sqrt:
	swi 0xd
	bx lr
	.size SVC_Sqrt, .-SVC_Sqrt
	FILLER 0x3ac, 0x414

// SVC_UnpackBits: BIOS call 0x10 (0x02004414)
	.global SVC_UnpackBits
	.type SVC_UnpackBits, @function
SVC_UnpackBits:
	swi 0x10
	bx lr
	.size SVC_UnpackBits, .-SVC_UnpackBits
	FILLER 0x418, 0x484

// SVC_WaitVBlankIntr: BIOS call 0x05, with r2 = 0 (0x02004484)
	.global SVC_WaitVBlankIntr
	.type SVC_WaitVBlankIntr, @function
SVC_WaitVBlankIntr:
	mov r2, #0x0
	swi 0x5
	bx lr
	.size SVC_WaitVBlankIntr, .-SVC_WaitVBlankIntr
	FILLER 0x48a, 0x4ea

// SVC_UncompressLZ16FromDevice: BIOS call 0x12 (0x020044ea)
	.global SVC_UncompressLZ16FromDevice
	.type SVC_UncompressLZ16FromDevice, @function
SVC_UncompressLZ16FromDevice:
	swi 0x12
	bx lr
	.size SVC_UncompressLZ16FromDevice, .-SVC_UncompressLZ16FromDevice
	FILLER 0x4ee, 0x554

// SVC_DivRem: BIOS call 0x09, returning the remainder that it leaves in r1 (0x02004554)
	.global SVC_DivRem
	.type SVC_DivRem, @function
SVC_DivRem:
	swi 0x9
	mov r0, r1
	bx lr
	.size SVC_DivRem, .-SVC_DivRem
	FILLER 0x55a, 0x5ae

// SVC_UncompressRL16FromDevice: BIOS call 0x15 (0x020045ae)
	.global SVC_UncompressRL16FromDevice
	.type SVC_UncompressRL16FromDevice, @function
SVC_UncompressRL16FromDevice:
	swi 0x15
	bx lr
	.size SVC_UncompressRL16FromDevice, .-SVC_UncompressRL16FromDevice
	FILLER 0x5b2, 0x632

// SVC_UncompressHuffmanFromDevice: BIOS call 0x13 (0x02004632)
	.global SVC_UncompressHuffmanFromDevice
	.type SVC_UncompressHuffmanFromDevice, @function
SVC_UncompressHuffmanFromDevice:
	swi 0x13
	bx lr
	.size SVC_UncompressHuffmanFromDevice, .-SVC_UncompressHuffmanFromDevice
	FILLER 0x636, 0x6aa

// SVC_UncompressLZ8: BIOS call 0x11 (0x020046aa)
	.global SVC_UncompressLZ8
	.type SVC_UncompressLZ8, @function
SVC_UncompressLZ8:
	swi 0x11
	bx lr
	.size SVC_UncompressLZ8, .-SVC_UncompressLZ8
	FILLER 0x6ae, 0x712

// SVC_Div: BIOS call 0x09, which leaves the quotient in r0 (0x02004712)
	.global SVC_Div
	.type SVC_Div, @function
SVC_Div:
	swi 0x9
	bx lr
	.size SVC_Div, .-SVC_Div
	FILLER 0x716, 0x778

// SVC_CpuSetFast: BIOS call 0x0c (0x02004778)
	.global SVC_CpuSetFast
	.type SVC_CpuSetFast, @function
SVC_CpuSetFast:
	swi 0xc
	bx lr
	.size SVC_CpuSetFast, .-SVC_CpuSetFast
	FILLER 0x77c, 0x800

#endif
