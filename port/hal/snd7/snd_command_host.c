// NitroSDK's queue of commands for the sound driver on the host: the decomp's lib/nitro/src/snd/snd_command.c,
// compiled unchanged through this file (port/hal/snd7/replaced.txt keeps it out of the game's own sources), with
// SND_WaitForCommandProc renamed out of the way and wrapped.
//
// The host's ARM7 runs a packet's commands when the ARM9 sends it (snd7.c), so SND_WaitForCommandProc always finds the
// packet done once it has taken back the replies, and never wakes the driver. The DS's driver runs a packet only at its
// next run, so right after a flush the DS's SND_WaitForCommandProc finds it waiting and wakes the driver, which makes a
// run that isn't periodic and draws from SND_CalcRandom's generator. The wrapper sends that wake-up word when the DS's
// would: when the packet isn't taken back yet and the DS's driver would still hold it (snd7_holds_packet), that is
// when no run of the driver came between the flush and the wait. It wakes the driver before the replies are taken
// back, where the DS's wakes it after; neither side sees the difference. A wait for a tag that the ARM9 took back
// before the DS's driver would have run its packet doesn't wake the driver here, where the DS's would.
//
// The file's casts of pointers to u32 (the packet's word and the shared work's address) cut them, as the game's own
// build lets it; snd7.c takes both from the ARM9's variables after a check of the bits.
#define SND_WaitForCommandProc SNDi_DsWaitForCommandProc
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-to-int-cast"
#include "lib/nitro/src/snd/snd_command.c"
#pragma clang diagnostic pop
#undef SND_WaitForCommandProc

#include "hal/snd7/snd7_internal.h"

// snd.h's prototype was renamed with the function
void SND_WaitForCommandProc(u32 tag);

void SND_WaitForCommandProc(u32 tag) {
    if (!SND_IsFinishedCommandTag(tag) && snd7_holds_packet(tag)) {
        SNDi_WakeArm7();
    }
    SNDi_DsWaitForCommandProc(tag);
}
