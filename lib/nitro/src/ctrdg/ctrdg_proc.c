// The cartridge library's dummy start-up (libctrdg, TwlSDK's ctrdg/ARM9/src/ctrdg_proc.c by the place of its code in
// the library: the file name is a guess): CTRDG_DummyInit, which the game calls once at the start instead of the
// library's own start-up, and the callback it sets, which ignores the ARM7's words. The static's name is the SDK's
#include "nitro/ctrdg.h"
#include "nitro/pxi.h"

static void CTRDGi_DummyCallback(u32 fifoTag, u32 data, BOOL err);

void CTRDG_DummyInit(void) {
    CTRDGi_InitCommon();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_CTRDG, NULL);
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_CTRDG, CTRDGi_DummyCallback);
    CTRDGi_SendtoPxi(1);
}

static void CTRDGi_DummyCallback(u32 fifoTag, u32 data, BOOL err) {
}
