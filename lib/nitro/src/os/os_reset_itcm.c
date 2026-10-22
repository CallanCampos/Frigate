// The ITCM part of NitroSDK's os_reset.c: the boot code that checks the card's images with the DSi's SHA-1 calls, loads
// them and jumps to the program's entry. The SDK wrote some of it in assembly, so those functions are MWCC asm
// functions.
#include "nitro/card.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nitro/pxi.h"

// The ROM header that the card read leaves at the top of main memory
#define HW_ROM_HEADER ((const CARDRomHeader *)HW_ROM_HEADER_BUF)

// clang-format off
#pragma thumb on
// OSi_VerifyHmac (our name), in assembly: the HMAC-SHA-1 of size bytes at data under the key that follows its code,
// compared with the 20-byte hash. Thumb. The DSi's BIOS calls are SWIs, which C cannot emit, and the key is written
// inside the function's own code, between it and the literal pool that holds its address, so the SDK wrote the whole
// function out
asm BOOL OSi_VerifyHmac(const void *data, u32 size, const void *hash) {
    push {r3, r4, r5, r6, r7, lr}
    sub sp, #0xb8
    mov r4, r1
    mov r5, r0
    mov r7, r2
    add r0, sp, #0x54
    mov r3, #0x60
    add r3, r0
    mov r2, #0x0
    str r2, [r3, #0x0]
    swi 0x24               // SHA-1 init
    ldr r0, =@key
    add r1, sp, #0x14
    mov r2, #0x0
    mov r3, #0x36
@ipad:
    ldrb r6, [r0, r2]
    eor r6, r3
    strb r6, [r1, r2]
    add r2, r2, #0x1
    cmp r2, #0x40
    blt @ipad
    add r0, sp, #0x54
    swi 0x25               // SHA-1 update with the key block
    add r0, sp, #0x54
    mov r1, r5
    mov r2, r4
    swi 0x25               // and with the data
    add r0, sp, #0x0
    add r1, sp, #0x54
    swi 0x26               // SHA-1 get hash
    add r0, sp, #0x54
    mov r3, #0x60
    add r3, r0
    mov r2, #0x0
    str r2, [r3, #0x0]
    swi 0x24
    ldr r0, =@key
    add r1, sp, #0x14
    mov r2, #0x0
    mov r3, #0x5c
@opad:
    ldrb r6, [r0, r2]
    eor r6, r3
    strb r6, [r1, r2]
    add r2, r2, #0x1
    cmp r2, #0x40
    blt @opad
    add r0, sp, #0x54
    swi 0x25
    add r0, sp, #0x54
    add r1, sp, #0x0
    mov r2, #0x14
    swi 0x25
    add r0, sp, #0x0
    add r1, sp, #0x54
    swi 0x26
    add r0, sp, #0x0
    mov r1, r7
    swi 0x28               // the compare of the digest with the hash
    cmp r0, #0x0
    beq @done
    mov r0, #0x1
@done:
    add sp, #0xb8
    pop {r3, r4, r5, r6, r7, pc}
    nop                         // aligns the key that follows to a word
    // The key of the hashes, inside the code of the function, and the pool that holds its address after it
@key:
    dcd 0xdec00621
    dcd 0x3fce98ba
    dcd 0x9de392a6
    dcd 0x01edf246
    dcd 0x08cce376
    dcd 0xfa632356
    dcd 0xdfecd4ca
    dcd 0x3478629a
    dcd 0x3c636d8f
    dcd 0x92ca22fe
    dcd 0x23978820
    dcd 0xc2aecfd2
    dcd 0xfe8d6732
    dcd 0x986483ca
    dcd 0x373efdac
    dcd 0x24584687
}
#pragma thumb off

// clang-format on

// OSi_VerifyRomBlocks (our name): checks the keyed hashes of the four ranges that the DSi header gives, and
// hangs if one of them does not match, so that a program whose images were damaged never runs
void OSi_VerifyRomBlocks(void) {
    const CARDTwlRomHeader *header = (const CARDTwlRomHeader *)HW_TWL_ROM_HEADER_BUF;
    const ROMHeaderLoad *blocks[4];
    const u8 *hashes[4];
    BOOL verified = TRUE;
    u32 address;
    u32 size;
    int i;

    if (header->bootFlags & 1) {
        blocks[0] = &header->arm9;
        blocks[1] = &header->arm7;
        blocks[2] = &header->arm9i;
        blocks[3] = &header->arm7i;
        hashes[0] = header->arm9Hash;
        hashes[1] = header->arm7Hash;
        hashes[2] = header->arm9iHash;
        hashes[3] = header->arm7iHash;
        for (i = 0; i < 4; i++) {
            address = blocks[i]->ramAddr;
            size = blocks[i]->size;
            if (i == 0) {
                // The first 0x4000 bytes of the first image are not part of the hash
                address += 0x4000;
                size -= 0x4000;
            }
            verified &= OSi_VerifyHmac((const void *)address, size, hashes[i]);
        }
    }
    while (verified == FALSE) {
    }
}

// NitroSDK's OSi_DoResetSystem, in ITCM: waits until the reset has happened, turns the interrupts off, and has the boot
// code load the card's program and jump to it
void OSi_DoResetSystem(void) {
    while (!OS_IsResetOccurred()) {
    }
    reg_OS_IME = 0;
    if (OS_IsRunOnTwl() == TRUE) {
        OSi_LoadProgram(TRUE);
        OSi_VerifyRomBlocks();
    } else {
        OSi_LoadProgram(FALSE);
    }
    OSi_DoBoot();
}

// clang-format off
// OSi_DoBoot (our name), in ITCM, in assembly: it turns the interrupts off, takes the program that the card read
// left in the handoff area at the top of main memory, gives the area back to the program and jumps to the entry that
// the ROM header gives, with the registers that the area holds. Neither the jump nor the register load is a C call
asm void OSi_DoBoot(void) {
    mov ip, #0x4000000
    str ip, [ip, #0x208]        // IME: its low half is 0, so the interrupts go off
    ldr r1, =SDK_AUTOLOAD_DTCM_START
    add r1, r1, #0x3fc0
    add r1, r1, #0x3c
    mov r0, #0
    str r0, [r1, #0x0]          // clears the word that the ARM7 reads the request in
    ldr r1, =REG_SUBPINTF_ADDR
@waitArm7:
    ldrh r0, [r1, #0x0]
    and r0, r0, #0xf
    cmp r0, #0x1
    bne @waitArm7
    mov r0, #0x100
    strh r0, [r1, #0x0]
    mov r0, #0
    ldr r3, =0x2fffd9c
    ldr r4, [r3, #0x0]
    ldr r1, =0x2fffd80
    mov r2, #0x80
    bl OSi_CpuClear32
    str r4, [r3, #0x0]
    ldr r1, =0x2ffff80
    mov r2, #0x18
    bl OSi_CpuClear32
    ldr r1, =0x2ffff98
    strh r0, [r1, #0x0]
    ldr r1, =0x2ffff9c
    mov r2, #0x64
    bl OSi_CpuClear32
    ldr r1, =REG_SUBPINTF_ADDR
@waitArm7Again:
    ldrh r0, [r1, #0x0]
    and r0, r0, #0xf
    cmp r0, #0x1
    beq @waitArm7Again
    mov r0, #0
    strh r0, [r1, #0x0]
    ldr r3, =HW_ROM_HEADER_BUF
    ldr ip, [r3, #0x24]
    mov lr, ip
    ldr r11, =0x2ffff80
    ldmia r11, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10}
    mov r11, #0
    bx ip
}

// OSi_CpuClear32 (our name): the boot code's word fill, which OSi_DoBoot uses. It is instruction for instruction
// NitroSDK's MIi_CpuClear32, which the SDK wrote in assembly in mi_memory.c and copied into the ITCM image as a
// function of its own
asm void OSi_CpuClear32(u32 value, void *dest, u32 size) {
    add r12, r1, r2
@loop:
    cmp r1, r12
    stmltia r1!, {r0}
    blt @loop
    bx lr
}

// clang-format on

// clang-format off
// OSi_LoadProgram (our name), in assembly: reads the card's ROM header and the program's images with the ranges that
// the header gives, then on a DSi reads its own two images too and discards the caches, so that the program that was
// just loaded runs from memory. A read of the secure area starts at 0x8000 instead, and the two handshakes with the
// ARM7 bump the ARM9's stage counter (0x02fffc24) until the ARM7's (0x02fffc26) changes
asm void OSi_LoadProgram(BOOL twl) {
    stmdb sp!, {r4, lr}
    mov r4, r0
    cmp r4, #0x0
    beq @load
    ldr r0, =0x2fffc28
    mov r1, #0x1
    strh r1, [r0, #0x0]         // the stage of the load
    ldr r0, =0x2fffc24
    ldr r1, =0x2fffc26
    ldrh r2, [r1, #0x0]
    ldrh r3, [r0, #0x0]
@handshake:
    add r3, r3, #0x1
    strh r3, [r0, #0x0]
    ldrh ip, [r1, #0x0]
    cmp r2, ip
    beq @handshake
    add r3, r3, #0x1
    strh r3, [r0, #0x0]
@load:
    cmp r4, #0x0
    movne r0, #0x0
    ldreq r0, =0x2fffc2c
    ldreq r0, [r0, #0x0]
    stmdb sp!, {r0}
    cmp r0, #0x8000
    ldrhs r1, =HW_ROM_HEADER_BUF
    movhs r2, #0x160
    blhs OSi_ReadCardRom32
    ldr ip, =HW_ROM_HEADER_BUF
    ldr r0, [ip, #0x20]
    ldr r1, [ip, #0x28]
    ldr r2, [ip, #0x2c]
    ldr r3, [sp, #0x0]
    add r0, r0, r3
    subs r3, r0, #0x8000
    movlt r0, #0x8000
    sublt r1, r1, r3
    addlt r2, r2, r3
    cmp r2, #0x0
    blgt OSi_ReadCardRom32
    ldr ip, =HW_ROM_HEADER_BUF
    ldr r0, [ip, #0x30]
    ldr r1, [ip, #0x38]
    ldr r2, [ip, #0x3c]
    ldr r3, [sp, #0x0]
    add r0, r0, r3
    cmp r2, #0x0
    blgt OSi_ReadCardRom32
    ldmia sp!, {r0}
    cmp r4, #0x0
    beq @discardCaches
    ldr ip, =HW_TWL_ROM_HEADER_BUF
    ldr r0, [ip, #0x1c0]
    ldr r1, [ip, #0x1c8]
    ldr r2, [ip, #0x1cc]
    add r0, r0, #0x4000
    add r1, r1, #0x4000
    sub r2, r2, #0x4000
    subs r3, r0, #0x8000
    movlt r0, #0x8000
    sublt r1, r1, r3
    addlt r2, r2, r3
    cmp r2, #0x0
    blgt OSi_ReadCardRom32
    ldr ip, =HW_TWL_ROM_HEADER_BUF
    ldr r0, [ip, #0x1d0]
    ldr r1, [ip, #0x1d8]
    ldr r2, [ip, #0x1dc]
    add r0, r0, #0x4000
    add r1, r1, #0x4000
    sub r2, r2, #0x4000
    subs r3, r0, #0x8000
    movlt r0, #0x8000
    sublt r1, r1, r3
    addlt r2, r2, r3
    cmp r2, #0x0
    blgt OSi_ReadCardRom32
@discardCaches:
    mov r1, #0x0
@segment:
    mov r0, #0x0
@line:
    orr r2, r1, r0
    mcr p15, 0, r2, c7, c10, 2  // cleans the line to memory
    add r0, r0, #0x20
    cmp r0, #0x400
    blt @line
    adds r1, r1, #0x40000000
    bne @segment
    mov r0, #0x0
    mcr p15, 0, r0, c7, c6, 0   // discards the data cache
    mcr p15, 0, r0, c7, c5, 0   // and the instruction cache
    mcr p15, 0, r0, c7, c10, 4  // and drains the write buffer
    cmp r4, #0x0
    beq @done
    mov r3, #0x2
    ldr r0, =0x2fffc28
    strh r3, [r0, #0x0]
    ldr r0, =0x2fffc24
    ldr r1, =0x2fffc26
    ldrh r2, [r1, #0x0]
    ldrh r3, [r0, #0x0]
@handshakeAgain:
    add r3, r3, #0x1
    strh r3, [r0, #0x0]
    ldrh ip, [r1, #0x0]
    cmp r2, ip
    beq @handshakeAgain
    add r3, r3, #0x1
    strh r3, [r0, #0x0]
@done:
    ldmia sp!, {r4, pc}
}

// clang-format on

// OSi_ReadCardRom32 (our name): reads size bytes of the card's ROM at address into dest, a page of 0x200 bytes at a
// time. Each page's request is a command (0xb7) and the page's address, and the words that come back are copied to dest
void OSi_ReadCardRom32(u32 address, void *dest, u32 size) {
    // The card's command bytes, reached from the high byte of MCCNT0: the enable bit at 0x040001a1, MCCMD0's command
    // then address bytes at +7 to +0xa, and MCCMD1's at +0xb to +0xe
    vu8 *card = (vu8 *)0x040001a1;
    u32 control;
    u32 status;
    u32 word;
    s32 offset;

    control = (HW_ROM_HEADER->normalCommandControl & ~REG_MI_MCCNT1_PC_MASK) | REG_MI_MCCNT1_START_MASK |
              REG_MI_MCCNT1_RESB_MASK | REG_MI_MCCNT1_PC_512B;
    offset = -(s32)(address & 0x1ff);
    while (reg_MI_MCCNT1 & REG_MI_MCCNT1_START_MASK) {
    }
    card[0] = 0x80;
    address += offset;
    if (offset >= (s32)size) {
        return;
    }
    do {
        card[0x7] = 0xb7;
        card[0x8] = address >> 24;
        card[0x9] = address >> 16;
        card[0xa] = address >> 8;
        card[0xb] = address;
        card[0xc] = 0;
        card[0xd] = 0;
        card[0xe] = 0;
        reg_MI_MCCNT1 = control;
        do {
            status = reg_MI_MCCNT1;
            if (status & 0x800000) {
                word = reg_MI_MCD1;
                if (offset >= 0 && offset < (s32)size) {
                    // The page's words land at its offset in dest, which starts negative to skip the bytes of the
                    // page before the requested address; style-ok: byte-offset, dest is an untyped byte buffer
                    *(u32 *)((u8 *)dest + offset) = word;
                }
                offset += 4;
            }
        } while (status & REG_MI_MCCNT1_START_MASK);
        address += 0x200;
    } while (offset < (s32)size);
}
