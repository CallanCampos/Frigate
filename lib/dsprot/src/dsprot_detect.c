#include "dsprot/dsprot_internal.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// DS Protect's two checks of the machine the game runs on, in overlay 165: whether it is an emulator, which has a
// console's MAC address mixed up and an owner that was never set, and whether the cartridge is a flashcart, which
// reads the game card's data differently from the real thing. The ROM doesn't name this file; it is named for what it
// does.

// The card's registers, which the flashcart check reads the ROM through
#define REG_AUXSPICNT_H (*(vu8 *)0x040001a1)
#define REG_ROMCTRL (*(vu32 *)0x040001a4)
#define REG_ROMCMD(n) (*(vu8 *)(0x040001a8 + (n)))
#define REG_CARD_DATA (*(vu32 *)0x04100010)
#define REG_SCFG_A9ROM (*(vu8 *)0x04004000)

// The card's ROM control word that the system copied from the header, at 0x027ffe60, or 0x02fffe60 in TWL mode. DS
// Protect computes the address from the TWL bit instead of choosing between the two.
#define HW_ROM_HEADER_CARD_CTRL(twl) (*(u32 *)((((twl) + 5) << 23) - 0x1a0))

// Bytes of the first MAC address whose reversed bits an emulator has
static const u8 sEmulatorMac[6] = { 0xff, 0xf6, 0x40, 0xff, 0xff, 0xce };

// Returns a code that says the machine is not an emulator, and crashes the game if it is
u32 DSProt_CheckEmulator(void) {
    u8 mac[6];
    OSOwnerInfo owner;
    int i;
    int j;
    u32 code;

    func_0207c33c(mac);
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
                goto end;
            }
        }
        DSProt_CallCrash(NULL, NULL);
        code = 0xfb;
    }
end:
    return code * 0xbf;
}

// The CRC-32 of a buffer, with the polynomial applied to the other half of the values
u32 DSProt_Crc32(const u8 *data, u32 size) {
    u32 crc = ~0;

    while (size-- != 0) {
        u32 value = crc ^ *data++;

        if (value & 1) {
            crc = value >> 1;
        } else {
            crc = (value >> 1) ^ 0xedb88320;
        }
        if (crc & 1) {
            crc = crc >> 1;
        } else {
            crc = (crc >> 1) ^ 0xedb88320;
        }
        if (crc & 1) {
            crc = crc >> 1;
        } else {
            crc = (crc >> 1) ^ 0xedb88320;
        }
        if (crc & 1) {
            crc = crc >> 1;
        } else {
            crc = (crc >> 1) ^ 0xedb88320;
        }
        if (crc & 1) {
            crc = crc >> 1;
        } else {
            crc = (crc >> 1) ^ 0xedb88320;
        }
        if (crc & 1) {
            crc = crc >> 1;
        } else {
            crc = (crc >> 1) ^ 0xedb88320;
        }
        if (crc & 1) {
            crc = crc >> 1;
        } else {
            crc = (crc >> 1) ^ 0xedb88320;
        }
        if (crc & 1) {
            crc = crc >> 1;
        } else {
            crc = (crc >> 1) ^ 0xedb88320;
        }
    }
    return ~crc;
}

// Reads the 0x200 bytes at a card address with the card's registers, restoring the command registers and the
// memory controller afterwards. `flag` is the sixth byte of the command.
static inline void DSProt_ReadCardRaw(u32 *dest, u32 address, u32 flag) {
    u16 exmemcnt = reg_MI_EXMEMCNT;
    u8 savedCommand[8];
    u32 control;
    int offset;
    int i;

    reg_MI_EXMEMCNT &= ~0x0800;
    control = (HW_ROM_HEADER_CARD_CTRL(REG_SCFG_A9ROM & 1) & ~0x07000000) | 0xa1000000;
    offset = -(int)(address & 0x1ff);
    while (REG_ROMCTRL & 0x80000000) {
    }
    REG_AUXSPICNT_H = 0x80;
    for (i = 0; i < 8; i++) {
        savedCommand[i] = REG_ROMCMD(i);
    }
    if (offset < 0x200) {
        u32 page = address + offset;

        do {
            REG_ROMCMD(0) = 0xb7;
            REG_ROMCMD(1) = page >> 24;
            REG_ROMCMD(2) = page >> 16;
            REG_ROMCMD(3) = page >> 8;
            REG_ROMCMD(4) = page;
            REG_ROMCMD(5) = flag;
            REG_ROMCMD(6) = 0;
            REG_ROMCMD(7) = 0;
            REG_ROMCTRL = control;
            do {
                if (REG_ROMCTRL & 0x800000) {
                    u32 word = REG_CARD_DATA;

                    if (offset >= 0 && offset < 0x200) {
                        *(u32 *)((u8 *)dest + offset) = word;
                    }
                    offset += 4;
                }
            } while (REG_ROMCTRL & 0x80000000);
            page += 0x200;
        } while (offset < 0x200);
    }
    for (i = 0; i < 8; i++) {
        REG_ROMCMD(i) = savedCommand[i];
    }
    reg_MI_EXMEMCNT = exmemcnt;
}

// Reads the same places of the card with the card's registers and through the SDK, and crashes the game if a
// flashcart gives different data
u32 DSProt_CheckFlashcart(void) {
    u32 crcs[20];
    u32 page[128];
    u32 offset = 0x7000;
    u32 address = 0x1000;
    u32 lockId;
    u32 i;

    lockId = (u16)cart_key_create();
    func_0206eef0(lockId);
    for (i = 0; i < 6; i++) {
        DSProt_ReadCardRaw(page, address + (1 << (((u8 *)func_0206eed8())[0x14] + 17)), 0);
        crcs[i] = DSProt_Crc32((u8 *)page, 0x200);
        func_0206fd24(-1, address + offset, page, 0x200, 0, 0, 0);
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
        DSProt_ReadCardRaw(page, address, 0);
        crcs[i + 6] = DSProt_Crc32((u8 *)page, 0x200);
        func_0206fd24(-1, address, page, 0x200, 0, 0, 0);
        crcs[i + 8] = DSProt_Crc32((u8 *)page, 0x200);
        address += 0x1000;
    }
    address -= 0x1d000;
    for (; i < 10; i++) {
        DSProt_ReadCardRaw(page, address, 0x11);
        crcs[i + 8] = DSProt_Crc32((u8 *)page, 0x200);
        func_0206fd24(-1, address, page, 0x200, 0, 0, 0);
        crcs[i + 10] = DSProt_Crc32((u8 *)page, 0x200);
        address += 0x1000;
    }
    func_0206ef38(lockId);
    cart_key_release(lockId);
    for (i = 0; i < 128; i++) {
        page[i] = i;
    }
    for (i = 0; i < 3; i++) {
        if (crcs[6] != crcs[i]) {
            DSProt_CallCrash(NULL, NULL);
            return 0xb177;
        }
    }
    if (crcs[6] == crcs[7] && crcs[6] == crcs[8]) {
        DSProt_CallCrash(NULL, NULL);
        return 0xb177;
    }
    if (crcs[4] != crcs[10] || crcs[5] != crcs[11]) {
        DSProt_CallCrash(NULL, NULL);
        return 0xb177;
    }
    if (crcs[12] != crcs[14] || crcs[13] != crcs[15]) {
        DSProt_CallCrash(NULL, NULL);
        return 0xb177;
    }
    if (crcs[16] != crcs[18] || crcs[17] != crcs[19]) {
        DSProt_CallCrash(NULL, NULL);
        return 0xb177;
    }
    return 0xaa65;
}

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

// Hand-written, run from .ctor when the overlay is loaded: has DSProt_Decrypt decrypt this file's stubs and its
// checksum function
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

#pragma define_section ctor ".ctor" ".ctor" ".ctor" abs32 RW
__declspec(ctor) static void (*const sDecryptDetect)(void) = DSProt_DecryptDetect;
