// The ITCM part of TwlSDK's scfg_proc.c: switch the ARM9 clock, then wait for it to settle.
#include "twl/scfg.h"

// The settling delay is a fixed instruction loop; C eliminates an empty loop and a volatile counter adds memory
// accesses, so the SDK implements this hardware timing in assembly.
// clang-format off
asm void SCFGi_SwitchCpuSpeed(SCFGCpuSpeed speed) {
    ldr r2, =0x04004004
    ldrh r1, [r2]
    bic r1, r1, #1
    orr r1, r0, r1
    strh r1, [r2]
    cmp r0, #0
    moveq r0, #10
    movne r0, #20
@wait:
    subs r0, r0, #4
    bhs @wait
    bx lr
}
// clang-format on
