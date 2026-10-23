#include "dsprot/dsprot_internal.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// DS Protect's two checks of the machine the game runs on, in overlay 165: whether it is an emulator, which has a
// console's MAC address mixed up and an owner that was never set, and whether the cartridge is a flashcart, which
// reads the game card's data differently from the real thing. The ROM doesn't name this file; it is named for what it
// does.

// Bytes of the first MAC address whose reversed bits an emulator has
static const u8 sEmulatorMac[6] = { 0xff, 0xf6, 0x40, 0xff, 0xff, 0xce };

// Returns a code that says the machine is not an emulator, and crashes the game if it is
u32 DSProt_CheckEmulator(void) {
    u8 mac[6];
    OSOwnerInfo owner;
    int i;
    int j;
    u32 code;

    OS_GetMacAddress(mac);
    for (i = 0; i < 6; i++) {
        if (sEmulatorMac[i] != (mac[i] ^ 0xff)) {
            break;
        }
    }
    OS_GetOwnerInfo(&owner);
    if (i == 6 && owner.birthday.month == 1 && owner.birthday.day == 1 && owner.nickNameLength == 0) {
        DSProt_CallCrash(NULL, NULL);
        code = 0xfb;
    } else {
        for (j = 0; j < 6; j++) {
            if (mac[j] != 0) {
                code = 0xf1;
                // style-ok: goto-return pret's pokeheartgold DS Protect (mac_owner.c) jumps to its exit the same way
                goto end;
            }
        }
        DSProt_CallCrash(NULL, NULL);
        code = 0xfb;
    }
end:
    return code * 0xbf;
}

// The CRC-32 of a buffer, a byte's eight steps written out
u32 DSProt_Crc32(const u8 *data, u32 size) {
    u32 crc = ~0;

    while (size-- != 0) {
        crc ^= *data++;
        DSPROT_CRC32_STEP(crc);
        DSPROT_CRC32_STEP(crc);
        DSPROT_CRC32_STEP(crc);
        DSPROT_CRC32_STEP(crc);
        DSPROT_CRC32_STEP(crc);
        DSPROT_CRC32_STEP(crc);
        DSPROT_CRC32_STEP(crc);
        DSPROT_CRC32_STEP(crc);
    }
    return ~crc;
}

static inline u32 DSProt_GetRomSize(void) {
    const CARDRomHeader *header;
    u32 size;
    header = (const CARDRomHeader *)CARD_GetRomHeader();
    size = 1 << (header->device_size + 17);
    return size;
}

// Reads a byte range at a card address with the card's registers, restoring the command registers and the
// memory controller afterwards. `flag` is the sixth byte of the command. The register block is reached from a base
// built at run time (1 shifted by 26) and a second pointer to 0x04000000 for the command bytes, the way the original's
// constants are built.
static inline void DSProt_ReadCardRaw(void *dest, u32 address, s32 size, u8 flag) {
    u8 savedCommand[8];
    u32 base;
    vu8 *regs;
    u32 header;
    u32 control;
    s32 offset;
    u16 exmemcnt;
    u32 page;
    u32 word;
    int i;

    base = 1;
    base <<= REG_BASE_SHIFT;
    regs = (vu8 *)0x04000000;
    exmemcnt = reg_MI_EXMEMCNT;
    reg_MI_EXMEMCNT &= ~0x0800;
    header = 5;
    header += *(vu8 *)(base + OFFSET_SCFG_A9ROM) & 1;
    header <<= 18;
    header -= 13;
    header <<= 5;
    control = (*(u32 *)header & ~0x07000000) | 0xa1000000;
    offset = -(s32)(address & 0x1ff);
    while (*(vu32 *)(base + OFFSET_ROMCTRL) & 0x80000000) {
    }
    *(vu8 *)(base + OFFSET_AUXSPICNT_H) = 0x80;
    for (i = 0; i < 8; i++) {
        savedCommand[i] = REG_ROMCMD(i);
    }
    page = address + offset;
    while (offset < size) {
        regs[OFFSET_ROMCMD + 0] = 0xb7;
        regs[OFFSET_ROMCMD + 1] = page >> 24;
        regs[OFFSET_ROMCMD + 2] = page >> 16;
        regs[OFFSET_ROMCMD + 3] = page >> 8;
        regs[OFFSET_ROMCMD + 4] = page;
        regs[OFFSET_ROMCMD + 5] = flag;
        regs[OFFSET_ROMCMD + 6] = 0;
        regs[OFFSET_ROMCMD + 7] = 0;
        *(vu32 *)(base + OFFSET_ROMCTRL) = control;
        do {
            if (*(vu32 *)(base + OFFSET_ROMCTRL) & 0x800000) {
                word = *(vu32 *)(base + OFFSET_CARD_DATA);

                if (offset >= 0 && offset < size) {
                    // The address is an integer sum, as in overlay 337's copy (dsprot_detect_multi.c): MWCC compiles
                    // a sum on a u8 pointer differently
                    *(u32 *)((u32)dest + offset) = word;
                }
                offset += 4;
            }
        } while (*(vu32 *)(base + OFFSET_ROMCTRL) & 0x80000000);
        page += 0x200;
    }
    for (i = 0; i < 8; i++) {
        REG_ROMCMD(i) = savedCommand[i];
    }
    *(vu16 *)(base + OFFSET_EXMEMCNT) = exmemcnt;
}

// Reads the same places of the card with the card's registers and through the SDK, and crashes the game if a
// flashcart gives different data
static inline u32 DSProt_TestCard(u32 passCode, u32 failCode) {
    u32 crcs[20];
    u32 page[128];
    int i;
    u32 address;
    u32 offset;
    u16 lockId;

    offset = 0x7000;
    address = 0x1000;

    lockId = OS_GetLockID();
    CARD_LockRom(lockId);
    for (i = 0; i < 6; i++) {
        DSProt_ReadCardRaw(page, address + DSProt_GetRomSize(), 0x200, 0);
        crcs[i] = DSProt_Crc32((u8 *)page, 0x200);
        CARD_ReadRom((u32)-1, (const void *)(address + offset), page, 0x200);
        crcs[i + 6] = DSProt_Crc32((u8 *)page, 0x200);
        if (i < 3) {
            address += 0x200;
        } else if (i == 3) {
            offset = 0;
            address = 0xa000;
        } else if (i > 3) {
            address = i * 0x1000 + 0x9000;
        }
    }
    address += 0x1e000;
    for (; i < 8; i++) {
        DSProt_ReadCardRaw(page, address, 0x200, 0);
        crcs[i + 6] = DSProt_Crc32((u8 *)page, 0x200);
        CARD_ReadRom((u32)-1, (const void *)address, page, 0x200);
        crcs[i + 8] = DSProt_Crc32((u8 *)page, 0x200);
        address += 0x1000;
    }
    address -= 0x1d000;
    for (; i < 10; i++) {
        DSProt_ReadCardRaw(page, address, 0x200, 0x11);
        crcs[i + 8] = DSProt_Crc32((u8 *)page, 0x200);
        CARD_ReadRom((u32)-1, (const void *)address, page, 0x200);
        crcs[i + 10] = DSProt_Crc32((u8 *)page, 0x200);
        address += 0x1000;
    }
    CARD_UnlockRom(lockId);
    OS_ReleaseLockID(lockId);
    for (i = 0; i < 128; i++) {
        page[i] = i;
    }
    for (i = 0; i < 3; i++) {
        if (crcs[6] != crcs[i]) {
            DSProt_CallCrash(NULL, NULL);
            return failCode;
        }
    }
    if (crcs[6] == crcs[7] && crcs[6] == crcs[8]) {
        DSProt_CallCrash(NULL, NULL);
        return failCode;
    }
    if (crcs[4] != crcs[10] || crcs[5] != crcs[11]) {
        DSProt_CallCrash(NULL, NULL);
        return failCode;
    }
    if (crcs[12] != crcs[14] || crcs[13] != crcs[15]) {
        DSProt_CallCrash(NULL, NULL);
        return failCode;
    }
    if (crcs[16] != crcs[18] || crcs[17] != crcs[19]) {
        DSProt_CallCrash(NULL, NULL);
        return failCode;
    }
    return passCode;
}

// The two possible results are the check's prime-encoded Boolean values.
u32 DSProt_CheckFlashcart(void) {
    return DSProt_TestCard(0xaa65, 0xb177);
}

// Hand-written, as every DSProt_Call* is: checksums DSProt_Dispatch from the address and word count packed in its
// last word, then has DSProt_Dispatch decrypt DSProt_CheckFlashcart, run it and encrypt it again
// clang-format off
asm u32 DSProt_CallCheckFlashcart(void) {
    orr ip, pc, pc
    ands ip, ip, ip
    moveq ip, #0
    addne ip, ip, #0x8c
    ldr ip, [ip, #0x14]
    sub ip, ip, #0
    stmdb sp!, {r0, r1, r2, r3, r4}
    mov r1, ip, lsr #0x1a
    eor r3, r3, r3
    mov ip, ip, lsl #6
    mov ip, ip, lsr #6
    mov r0, ip
@1:
    ldr r2, [r0, #0]
    mov r4, r2, lsr #0x18
    cmp r4, #0xea
    cmpne r4, #0xeb
    eorne r3, r3, r2, ror #0x11
    addne r3, r3, r2, ror #0x1c
    eorne r3, r3, r2, ror r1
    subs r1, r1, #1
    add r0, r0, #4
    bne @1
    mov r0, #0x2000000
    orr r1, r0, #0x800
    mov r0, #0x4000000
    orr r0, r0, #0x4000
    ldr r0, [r0, #0]
    ands r0, r0, #1
    orrne r1, r1, #0x4000
    cmp ip, r1
    strge r3, [pc, #0x18]
    movlt ip, lr
    ldmgeia sp!, {r0, r1, r2, r3, r4}
    ldmltia sp!, {r5, r6, r7, r8, r9}
    stmgedb sp!, {ip}
    orr ip, pc, pc
    ldmia sp!, {pc}
    dcd DSProt_Work + 1
    dcd DSProt_Work + 3
    dcd DSProt_CheckFlashcart + 0x2200
    dcd DSProt_Work + 0x2200 + 0x6a0
    dcd 0
    dcd DSProt_Dispatch + (26 << 26)
}
// clang-format on

// Hand-written, as every DSProt_Call* is: checksums DSProt_Dispatch from the address and word count packed in its
// last word, then has DSProt_Dispatch decrypt DSProt_CheckEmulator, run it and encrypt it again
// clang-format off
asm u32 DSProt_CallCheckEmulator(void) {
    orr ip, pc, pc
    ands ip, ip, ip
    moveq ip, #0
    addne ip, ip, #0x8c
    ldr ip, [ip, #0x14]
    sub ip, ip, #0
    stmdb sp!, {r0, r1, r2, r3, r4}
    mov r1, ip, lsr #0x1a
    eor r3, r3, r3
    mov ip, ip, lsl #6
    mov ip, ip, lsr #6
    mov r0, ip
@1:
    ldr r2, [r0, #0]
    mov r4, r2, lsr #0x18
    cmp r4, #0xea
    cmpne r4, #0xeb
    eorne r3, r3, r2, ror #0x11
    addne r3, r3, r2, ror #0x1c
    eorne r3, r3, r2, ror r1
    subs r1, r1, #1
    add r0, r0, #4
    bne @1
    mov r0, #0x2000000
    orr r1, r0, #0x800
    mov r0, #0x4000000
    orr r0, r0, #0x4000
    ldr r0, [r0, #0]
    ands r0, r0, #1
    orrne r1, r1, #0x4000
    cmp ip, r1
    strge r3, [pc, #0x18]
    movlt ip, lr
    ldmgeia sp!, {r0, r1, r2, r3, r4}
    ldmltia sp!, {r5, r6, r7, r8, r9}
    stmgedb sp!, {ip}
    orr ip, pc, pc
    ldmia sp!, {pc}
    dcd DSProt_Work + 1
    dcd DSProt_Work + 3
    dcd DSProt_CheckEmulator + 0x2200
    dcd DSProt_Work + 0x2200 + 0xac
    dcd 0
    dcd DSProt_Dispatch + (26 << 26)
}
// clang-format on

// Hand-written, run from .ctor when the overlay is loaded: has DSProt_Decrypt decrypt this file's stubs and its
// checksum function
// clang-format off
asm void DSProt_DecryptDetect(void) {
    orr r0, pc, #0
    adds r0, r0, #4
    bne DSProt_Decrypt
    dcd DSProt_CallCheckFlashcart + 0x2200
    dcd DSProt_Work + 0x2200 + 0x94
    dcd DSProt_CallCheckEmulator + 0x2200
    dcd DSProt_Work + 0x2200 + 0x94
    dcd DSProt_Crc32 + 0x2200
    dcd DSProt_Work + 0x2200 + 0x90
    dcd 0
    dcd 0
}
// clang-format on

#pragma define_section ctor ".ctor"                                                                                    \
                            ".ctor"                                                                                    \
                            ".ctor" abs32 RW
// The loader, rather than C code, references this registration.
__attribute__((used)) __declspec(ctor) static void (*const sDecryptDetect)(void) = DSProt_DecryptDetect;
