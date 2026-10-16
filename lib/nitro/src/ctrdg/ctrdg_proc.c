// The cartridge library's dummy start-up (libctrdg, TwlSDK's ctrdg/ARM9/src/ctrdg_proc.c by the place of its code in
// the library: the file name is a guess): CTRDG_DummyInit, which the game calls once at the start instead of the
// library's own start-up, and the callback it sets, which ignores the ARM7's words. The static's name is the SDK's
#include "nitro/ctrdg.h"
#include "nitro/pxi.h"

static void CTRDGi_DummyCallback(u32 fifoTag, u32 data, BOOL err);

// CTRDG_DummyInit
void func_020827d0(void) {
    func_02082624();
    func_0207c9fc(PXI_FIFO_TAG_CTRDG, NULL);
    func_0207c9fc(PXI_FIFO_TAG_CTRDG, CTRDGi_DummyCallback);
    func_020827a0(1);
}

static void CTRDGi_DummyCallback(u32 fifoTag, u32 data, BOOL err) {
}
