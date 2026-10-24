// NitroSDK's crt0.c, the ARM9's startup code (with TwlSDK's DSi additions), which the ROM header's entry address
// points at: _start sets up the CP15 (the caches, the tightly coupled memories and the protection regions), waits for
// the ARM7, clears the memories it needs, decompresses the static module and copies the autoload blocks to their
// places, clears the static .bss, runs the static initializers and jumps to main. The SDK wrote these functions in
// assembly, so they are MWCC asm functions. The file name is NitroSDK's. _start, INITi_CpuClear32 and NitroStartUp
// have NitroSDK's names, as ntrtwl's reconstruction of NitroSDK has them; AutoloadCallback and BuildInfo keep dsd's
// names for NitroSDK's _start_AutoloadDoneCallback and _start_ModuleParams, since dsd needs them. The rest is
// TwlSDK's split of the start-up, which the reference, older than TwlSDK, doesn't have: those functions keep swan's
// names, or ours, or their addresses.
//
// The module parameters among the code, BuildInfo and the LTD module's, hold the addresses of the two modules'
// autoload lists, which the build writes after each module and dsd's linker script has no symbols for, so the file
// isn't linked yet.
#include "nitro/mi.h"
#include "nitro/os.h"
#include "system/main.h"
// MSL's floating-point and static initializers' set-up, which _start calls before main
#include "../../../msl/src/fp/fp_fastI.h"
#include "../../../msl/src/runtime/call_static_initializers.h"

// The DSi's SCFG registers: which console runs the code (the low bits of REG_SCFG_A9ROM) and the ARM9's clock
#define REG_SCFG_A9ROM_ADDR 0x04004000
#define REG_SCFG_CLK_ADDR 0x04004004

// The entry point, which the ROM header gives
void _start(void);
static void INITi_CpuClear32(u32 value, void *dest, u32 size);
static void boot_InitTCM(void);
static void INITi_InitRegion(void);
static void boot_BootstrapExec(void);
static void INITi_ShelterStaticInitializer(const u32 *functions);
static void INITi_CallStaticInitializers(void);
static void NitroStartUp(void);
static BOOL asm_HWIsDSi(void);
static void asm_MemCpy32Fast(void *dest, const void *src, u32 size);
static void boot_MemSet32Fast(void *dest, u32 value, u32 size);
static void WaitForHalfword(vu16 *address, u16 value);
static void func_02004fd4(vu16 *counter, vu16 *watched);
static BOOL func_02004ffc(BOOL enable, s32 delay);

// The module parameters sit among the code, in .text
#pragma define_section textdata ".text"                                                                                \
                                ".text"                                                                                \
                                ".text" abs32 RX
__declspec(section "textdata") void *const BuildInfo[9];
__declspec(section "textdata") void *const LtdModuleParams[6];

// clang-format off
// The entry point. It runs in the system mode with the interrupts off, sets up the IRQ, supervisor and system
// stacks at the end of DTCM, and doesn't return: main is entered with lr at the BIOS
asm void _start(void) {
    // Disable the interrupts (REG_IME) and set up the CP15
    mov ip, #0x4000000
    str ip, [ip, #0x208]
    bl boot_InitTCM
    // Clear the system's shared words at the end of main memory, and mark the boot as from the card if nothing
    // else did
    mov r0, #0x0
    ldr r1, =0x2ffff80
    ldr r2, =0x68
    bl INITi_CpuClear32
    mov r0, #0x0
    ldr r1, =0x2fffff0
    ldr r2, =0x10
    bl INITi_CpuClear32
    ldr r1, =0x2fffc40
    ldrh r2, [r1, #0x0]
    cmp r2, #0x0
    moveq r2, #0x1
    streqh r2, [r1, #0x0]
    // Wait for the ARM7, from the start of ITCM since the ARM7 may be using main memory: first for its word to
    // change, counting in the ARM9's, then for it to become 4
    ldr r1, =func_02004fd4
    ldr r2, =0x1ff8000
    add r3, r2, #0x28
@copy_counter:
    ldr r0, [r1], #0x4
    str r0, [r2], #0x4
    cmp r2, r3
    blt @copy_counter
    ldr r0, =0x2fffc28
    mov r1, #0x1
    strh r1, [r0, #0x0]
    ldr r0, =0x2fffc24
    ldr r1, =0x2fffc26
    ldr r2, =0x1ff8000
    blx r2
    ldr r1, =WaitForHalfword
    ldr r2, =0x1ff8000
    add r3, r2, #0x1c
@copy_wait:
    ldr r0, [r1], #0x4
    str r0, [r2], #0x4
    cmp r2, r3
    blt @copy_wait
    ldr r0, =0x2fffc28
    mov r1, #0x4
    ldr r2, =0x1ff8000
    blx r2
    // On a DSi, set bit 0 of REG_SCFG_CLK, from ITCM too, copied from its end backward
    bl asm_HWIsDSi
    bne @not_dsi
    ldr r1, =func_02004ffc
    ldr r2, =0x1ff8000
    add r2, r2, #0x1c
    mov r3, #0x34
@copy_clock:
    subs r3, r3, #0x4
    ldr r0, [r1, r3]
    str r0, [r2, r3]
    bgt @copy_clock
    mov r0, #0x1
    mov r1, #0x8
    blx r2
@not_dsi:
    bl INITi_InitRegion
    // The IRQ stack below the supervisor's 0x40 bytes at the end of DTCM, then the system stack below the IRQ stack,
    // each 8-byte aligned
    mov r0, #0x13
    msr cpsr_c, r0
    ldr r1, =SDK_AUTOLOAD_DTCM_START
    add r1, r1, #0x4000
    sub sp, r1, #0x40
    sub r1, sp, #0x40
    mov r0, #0x12
    msr cpsr_c, r0
    sub sp, r1, #0x4
    tst sp, #0x4
    subeq sp, sp, #0x4
    ldr r0, =SDK_IRQ_STACKSIZE
    sub r1, r1, r0
    mov r0, #0x1f
    msr cpsr_fsxc, r0
    sub sp, r1, #0x4
    tst sp, #0x4
    subne sp, sp, #0x4
    // Clear DTCM, the palettes and OAM
    mov r0, #0x0
    ldr r1, =SDK_AUTOLOAD_DTCM_START
    mov r2, #0x4000
    bl INITi_CpuClear32
    mov r1, #0x0
    ldr r0, =0x5000000
    mov r2, #0x400
    bl boot_MemSet32Fast
    ldr r0, =0x7000000
    mov r2, #0x400
    bl boot_MemSet32Fast
    // Decompress the module and do the autoloads, then clear the static .bss
    bl boot_BootstrapExec
    mov r1, #0x0
    ldr r3, =BuildInfo
    ldr r0, [r3, #0xc]
    ldr r2, [r3, #0x10]
    subs r2, r2, r0
    blgt boot_MemSet32Fast
    // The IRQ handler's address, in the last word of DTCM
    ldr r1, =SDK_AUTOLOAD_DTCM_START
    add r1, r1, #0x3fc0
    add r1, r1, #0x3c
    ldr r0, =OS_IrqHandler
    str r0, [r1, #0x0]
    bl _fp_init
    bl NitroStartUp
    bl __call_static_initializers
    bl INITi_CallStaticInitializers
    // Start main at the start of a frame (when REG_VCOUNT is 0)
    ldr r1, =0x4000006
@wait_vcount:
    ldrh r0, [r1, #0x0]
    cmp r0, #0x0
    bne @wait_vcount
    ldr r1, =main
    ldr lr, =0xffff0000
    bx r1
}

// NitroSDK's INITi_CpuClear32 (swan's asm_MemSet32): fills size bytes at dest with value, a word at a time
static asm void INITi_CpuClear32(u32 value, void *dest, u32 size) {
    add ip, r1, r2
@loop:
    cmp r1, ip
    strlt r0, [r1], #0x4
    blt @loop
    bx lr
}

// Cleans the data cache if it is on, and sets up the CP15: the caches and the protection unit off, ITCM at 0 and
// DTCM at SDK_AUTOLOAD_DTCM_START (16 KB), both enabled
static asm void boot_InitTCM(void) {
    mrc p15, 0, r0, c1, c0, 0
    tst r0, #0x1
    beq @set_up
    tst r0, #0x4
    beq @drain
    mov r1, #0x0
@clean_segment:
    mov r2, #0x0
@clean_line:
    orr r3, r1, r2
    mcr p15, 0, r3, c7, c10, 2
    add r2, r2, #0x20
    cmp r2, #0x400
    blt @clean_line
    adds r1, r1, #0x40000000
    bne @clean_segment
@drain:
    mov r1, #0x0
    mcr p15, 0, r1, c7, c10, 4
@set_up:
    ldr r1, =0xf9005
    bic r0, r0, r1
    ldr r1, =0x2078
    orr r0, r0, r1
    mcr p15, 0, r0, c1, c0, 0
    mov r1, #0x20
    mcr p15, 0, r1, c9, c1, 1
    ldr r1, =SDK_AUTOLOAD_DTCM_START
    orr r1, r1, #0xa
    mcr p15, 0, r1, c9, c1, 0
    mov r1, #0x50000
    orr r0, r0, r1
    mcr p15, 0, r0, c1, c0, 0
    bx lr
}

// Sets up the protection regions, their cache and write buffer bits and their access, for a DSi's memory map or a
// DS's, and enables the protection unit and the caches
static asm void INITi_InitRegion(void) {
    mov ip, lr
    ldr r0, =0x4000033
    mcr p15, 0, r0, c6, c0, 0
    ldr r0, =SDK_AUTOLOAD_DTCM_START
    orr r0, r0, #0x1b
    mcr p15, 0, r0, c6, c4, 0
    ldr r0, =0x100002f
    mcr p15, 0, r0, c6, c5, 0
    ldr r0, =0xffff001d
    mcr p15, 0, r0, c6, c6, 0
    bl asm_HWIsDSi
    bne @ds
    ldr r0, =0x2000031
    mcr p15, 0, r0, c6, c1, 0
    ldr r0, =0x2f80025
    mcr p15, 0, r0, c6, c2, 0
    ldr r0, =0x8000035
    mcr p15, 0, r0, c6, c3, 0
    ldr r0, =0x2ffc01b
    mcr p15, 0, r0, c6, c7, 0
    mov r0, #0x4a
    mcr p15, 0, r0, c2, c0, 1
    mov r0, #0x4a
    mcr p15, 0, r0, c2, c0, 0
    mov r0, #0xa
    mcr p15, 0, r0, c3, c0, 0
    ldr r0, =0x5101011
    mcr p15, 0, r0, c5, c0, 3
    ldr r0, =0x15111011
    mcr p15, 0, r0, c5, c0, 2
    b @enable
@ds:
    ldr r0, =0x2000031
    mcr p15, 0, r0, c6, c1, 0
    ldr r0, =0x27ff017
    mcr p15, 0, r0, c6, c2, 0
    ldr r0, =0x8000035
    mcr p15, 0, r0, c6, c3, 0
    ldr r0, =0x2fff017
    mcr p15, 0, r0, c6, c7, 0
    mov r0, #0x42
    mcr p15, 0, r0, c2, c0, 1
    mov r0, #0x42
    mcr p15, 0, r0, c2, c0, 0
    mov r0, #0x2
    mcr p15, 0, r0, c3, c0, 0
    ldr r0, =0x5101011
    mcr p15, 0, r0, c5, c0, 3
    ldr r0, =0x15111111
    mcr p15, 0, r0, c5, c0, 2
@enable:
    mrc p15, 0, r0, c1, c0, 0
    ldr r1, =0x5005
    orr r0, r0, r1
    mcr p15, 0, r0, c1, c0, 0
    mov r1, #0x0
    mcr p15, 0, r1, c7, c6, 0
    mcr p15, 0, r1, c7, c5, 0
    bx ip
}

// Decompresses the static module and copies each block of its autoload list ({address, size, .bss size, static
// initializers} after the block's data) to its place, clearing its .bss after it and flushing the caches for it
// unless it is in ITCM or DTCM. On a DSi whose launcher loaded the LTD module, does the same for that module's list.
// Then calls AutoloadCallback with the two modules' parameters
static asm void boot_BootstrapExec(void) {
    stmdb sp!, {lr}
    ldr r1, =BuildInfo
    ldr r0, [r1, #0x14]
    bl MIi_UncompressBackward
    ldr r0, =BuildInfo
    ldr ip, [r0, #0x0]
    ldr r3, [r0, #0x4]
    ldr r1, [r0, #0x8]
@static_block:
    cmp ip, r3
    bge @static_done
    stmdb sp!, {r3}
    ldr r0, [ip], #0x4
    ldr r2, [ip], #0x4
    stmdb sp!, {r0}
    bl asm_MemCpy32Fast
    stmdb sp!, {r0, r1}
    ldr r0, [ip], #0x4
    stmdb sp!, {ip}
    bl INITi_ShelterStaticInitializer
    ldmia sp!, {ip}
    ldmia sp!, {r0}
    mov r1, #0x0
    ldr r2, [ip], #0x4
    bl boot_MemSet32Fast
    ldmia sp!, {r1, r2}
    mov r3, #0x1000000
    cmp r2, r3
    movge r3, #0x2000000
    cmpge r3, r2
    bgt @static_next
    ldr r3, =SDK_AUTOLOAD_DTCM_START
    cmp r2, r3
    addge r3, r3, #0x4000
    cmpge r3, r2
    bgt @static_next
    bic r2, r2, #0x1f
@static_flush:
    cmp r2, r0
    bge @static_next
    mcr p15, 0, r2, c7, c14, 1
    mcr p15, 0, r2, c7, c5, 1
    add r2, r2, #0x20
    b @static_flush
@static_next:
    ldmia sp!, {r3}
    b @static_block
@static_done:
    bl asm_HWIsDSi
    bne @done
    ldr r1, =0x2ffe1cc
    ldr r0, [r1, #0x0]
    cmp r0, #0x0
    beq @done
    ldr r1, =LtdModuleParams
    ldr r0, [r1, #0xc]
    bl MIi_UncompressBackward
    ldr r0, =LtdModuleParams
    ldr ip, [r0, #0x0]
    ldr r3, [r0, #0x4]
    ldr r1, [r0, #0x8]
@ltd_block:
    cmp ip, r3
    bge @done
    stmdb sp!, {r3}
    ldr r0, [ip], #0x4
    ldr r2, [ip], #0x4
    stmdb sp!, {r0}
    bl asm_MemCpy32Fast
    stmdb sp!, {r0, r1}
    ldr r0, [ip], #0x4
    stmdb sp!, {ip}
    bl INITi_ShelterStaticInitializer
    ldmia sp!, {ip}
    ldmia sp!, {r0}
    mov r1, #0x0
    ldr r2, [ip], #0x4
    bl boot_MemSet32Fast
    ldmia sp!, {r1, r2}
    mov r3, #0x1000000
    cmp r2, r3
    movge r3, #0x2000000
    cmpge r3, r2
    bgt @ltd_next
    ldr r3, =SDK_AUTOLOAD_DTCM_START
    cmp r2, r3
    addge r3, r3, #0x4000
    cmpge r3, r2
    bgt @ltd_next
    bic r2, r2, #0x1f
@ltd_flush:
    cmp r2, r0
    bge @ltd_next
    mcr p15, 0, r2, c7, c14, 1
    mcr p15, 0, r2, c7, c5, 1
    add r2, r2, #0x20
    b @ltd_flush
@ltd_next:
    ldmia sp!, {r3}
    b @ltd_block
@done:
    mov r0, #0x0
    mcr p15, 0, r0, c7, c10, 4
    ldr r0, =BuildInfo
    ldr r1, =LtdModuleParams
    ldmia sp!, {lr}
    b AutoloadCallback
}

// Appends a block's zero-terminated list of static initializers to the one kept below the IRQ stack, for
// INITi_CallStaticInitializers
static asm void INITi_ShelterStaticInitializer(const u32 *functions) {
    cmp r0, #0x0
    bxeq lr
    ldr r1, =SDK_AUTOLOAD_DTCM_START
    add r1, r1, #0x4000
    sub r1, r1, #0x40
    sub r1, r1, #0x40
    ldr r2, =SDK_IRQ_STACKSIZE
    sub r1, r1, r2
    add r1, r1, #0x4
@find_end:
    ldr r2, [r1, #0x0]
    cmp r2, #0x0
    addne r1, r1, #0x4
    bne @find_end
@copy:
    ldr r2, [r0], #0x4
    str r2, [r1], #0x4
    cmp r2, #0x0
    bne @copy
    bx lr
}

// Calls each static initializer of the list INITi_ShelterStaticInitializer made, clearing its entry
static asm void INITi_CallStaticInitializers(void) {
    stmdb sp!, {lr}
    ldr r1, =SDK_AUTOLOAD_DTCM_START
    add r1, r1, #0x4000
    sub r1, r1, #0x40
    sub r1, r1, #0x40
    ldr r2, =SDK_IRQ_STACKSIZE
    sub r1, r1, r2
    add r1, r1, #0x4
@call:
    ldr r0, [r1, #0x0]
    cmp r0, #0x0
    beq @done
    stmdb sp!, {r1}
    blx r0
    ldmia sp!, {r1}
    mov r0, #0x0
    str r0, [r1], #0x4
    b @call
@done:
    ldmia sp!, {lr}
    bx lr
}

// Expands the image compressed backward in place that ends at end: the last word gives the size to add to the end and
// the last byte of the word before it the size of this footer, and the code reads flag bytes and blocks backward from
// there, a literal byte or a copy of 3 to 18 bytes from up to 4 KB after. Then flushes the caches for the image
asm void MIi_UncompressBackward(void *end) {
    cmp r0, #0x0
    beq @return
    stmdb sp!, {r4, r5, r6, r7, r8}
    ldmdb r0, {r1, r2}
    add r2, r0, r2
    sub r3, r0, r1, lsr #0x18
    bic r1, r1, #0xff000000
    sub r1, r0, r1
    mov r4, r2
@flag_byte:
    cmp r3, r1
    ble @flush
    ldrb r5, [r3, #-0x1]!
    mov r6, #0x8
@flag_bit:
    subs r6, r6, #0x1
    blt @flag_byte
    tst r5, #0x80
    bne @copy
    ldrb r0, [r3, #-0x1]!
    ldrb r8, [r2, #-0x1]
    strb r0, [r2, #-0x1]!
    b @next_bit
@copy:
    ldrb ip, [r3, #-0x1]!
    ldrb r7, [r3, #-0x1]!
    orr r7, r7, ip, lsl #0x8
    bic r7, r7, #0xf000
    add r7, r7, #0x2
    add ip, ip, #0x20
@copy_byte:
    ldrb r0, [r2, r7]
    ldrb r8, [r2, #-0x1]
    strb r0, [r2, #-0x1]!
    subs ip, ip, #0x10
    bge @copy_byte
@next_bit:
    cmp r3, r1
    mov r5, r5, lsl #0x1
    bgt @flag_bit
@flush:
    mov r0, #0x0
    bic r3, r1, #0x1f
@flush_line:
    mcr p15, 0, r0, c7, c10, 4
    mcr p15, 0, r3, c7, c5, 1
    mcr p15, 0, r3, c7, c14, 1
    add r3, r3, #0x20
    cmp r3, r4
    blt @flush_line
    ldmia sp!, {r4, r5, r6, r7, r8}
@return:
    bx lr
}

// The hook called after the autoloads, empty unless a program defines its own: NitroSDK's
// _start_AutoloadDoneCallback, under dsd's name
asm void AutoloadCallback(void) {
    bx lr
}

// NitroSDK's NitroStartUp: the hook called before the static initializers, empty unless a program defines its own
static asm void NitroStartUp(void) {
    bx lr
}

// Keeps a library's signature in the link: the call is what references it
asm void OSi_ReferSymbol(void *symbol) {
    bx lr
}

// TRUE on a DSi in its own mode, from REG_SCFG_A9ROM
static asm BOOL asm_HWIsDSi(void) {
    ldr r0, =REG_SCFG_A9ROM_ADDR
    ldrb r0, [r0, #0x0]
    and r0, r0, #0x3
    cmp r0, #0x1
    moveq r0, #0x1
    movne r0, #0x0
    bx lr
}

// Copies size bytes (a multiple of 4) from src to dest, 32 bytes at a time while it can. Returns the end of dest in
// r0 and of src in r1, which boot_BootstrapExec goes on from
static asm void asm_MemCpy32Fast(void *dest, const void *src, u32 size) {
    stmdb sp!, {r3, r4, r5, r6, r7, r8, r9, r10, r11}
    bics r3, r2, #0x1f
    beq @tail
    add r3, r0, r3
@block:
    ldmia r1!, {r4, r5, r6, r7, r8, r9, r10, r11}
    stmia r0!, {r4, r5, r6, r7, r8, r9, r10, r11}
    cmp r3, r0
    bgt @block
@tail:
    tst r2, #0x10
    ldmneia r1!, {r4, r5, r6, r7}
    stmneia r0!, {r4, r5, r6, r7}
    tst r2, #0x8
    ldmneia r1!, {r4, r5}
    stmneia r0!, {r4, r5}
    tst r2, #0x4
    ldmneia r1!, {r4}
    stmneia r0!, {r4}
    ldmia sp!, {r3, r4, r5, r6, r7, r8, r9, r10, r11}
    bx lr
}

// Fills size bytes (a multiple of 4) at dest with value, 32 bytes at a time while it can
static asm void boot_MemSet32Fast(void *dest, u32 value, u32 size) {
    stmdb sp!, {r3, r4, r5, r6, r7, r8, r9, r10, r11}
    mov r4, r1
    mov r5, r1
    mov r6, r1
    mov r7, r1
    mov r8, r1
    mov r9, r1
    mov r10, r1
    mov r11, r1
    bics r3, r2, #0x1f
    beq @tail
    add r3, r0, r3
@block:
    stmia r0!, {r4, r5, r6, r7, r8, r9, r10, r11}
    cmp r3, r0
    bgt @block
@tail:
    tst r2, #0x10
    stmneia r0!, {r4, r5, r6, r7}
    tst r2, #0x8
    stmneia r0!, {r4, r5}
    tst r2, #0x4
    stmneia r0!, {r4}
    ldmia sp!, {r3, r4, r5, r6, r7, r8, r9, r10, r11}
    bx lr
}

// clang-format on

// The module parameters of the LTD module, which only a DSi loads: its autoload list and its end, where its blocks
// start, the end of the compressed module (0 until the compressor writes it) and TwlSDK's two magic numbers
__declspec(section "textdata") void *const LtdModuleParams[6] = {
    SDK_LTDAUTOLOAD_LIST, SDK_LTDAUTOLOAD_LIST_END, SDK_LTDAUTOLOAD_START, (void *)0,
    (void *)0xdec01463,   (void *)0x6314c0de,
};

// clang-format off
// Waits until the halfword at address is value, a little while between reads. _start runs it from ITCM
static asm void WaitForHalfword(vu16 *address, u16 value) {
@read:
    ldrh r2, [r0]
    cmp r1, r2
    bxeq lr
    mov r3, #0x10
@delay:
    subs r3, r3, #0x1
    bne @delay
    b @read
}

// clang-format on

// The module parameters, in the code (NitroSDK's _start_ModuleParams, under dsd's name): the static module's autoload
// list and its end, where the autoload blocks start, the static .bss, the end of the compressed module (0 until the
// compressor writes it), the SDK's version and the two magic numbers that the build tools look for
__declspec(section "textdata") void *const BuildInfo[9] = {
    SDK_AUTOLOAD_LIST, SDK_AUTOLOAD_LIST_END, ARM9_DATA_END,      ARM9_BSS_START,     ARM9_BSS_END,
    (void *)0,         (void *)0x0503757c,    (void *)0xdec00621, (void *)0x2106c0de,
};

// clang-format off
// Counts in counter until the halfword at watched changes. _start runs it from ITCM
static asm void func_02004fd4(vu16 *counter, vu16 *watched) {
    ldrh r2, [r1, #0x0]
    ldrh r3, [r0, #0x0]
@count:
    add r3, r3, #0x1
    strh r3, [r0, #0x0]
    ldrh ip, [r1, #0x0]
    cmp r2, ip
    beq @count
    add r3, r3, #0x1
    strh r3, [r0, #0x0]
    bx lr
}

// Sets bit 0 of REG_SCFG_CLK if enable, clears it otherwise, and when that changes it waits for delay / 4 loops.
// Returns the bit as it was. _start runs it from ITCM
static asm BOOL func_02004ffc(BOOL enable, s32 delay) {
    cmp r0, #0x0
    ldr r3, =REG_SCFG_CLK_ADDR
    ldrh r0, [r3, #0x0]
    biceq r2, r0, #0x1
    orrne r2, r0, #0x1
    cmp r0, r2
    and r0, r0, #0x1
    bxeq lr
    strh r2, [r3, #0x0]
@delay:
    subs r1, r1, #0x4
    bge @delay
    bx lr
}

// clang-format on

// Two words that, like the module parameters, end in a pair of magic numbers for the build tools to find them by, on a
// cache line of their own. What they give isn't known, and the name is ours
__declspec(section "textdata") const u32 TaggedParams[4] __attribute__((aligned(32))) = {
    0x900,
    1,
    0xdec08133,
    0x3381c0de,
};
