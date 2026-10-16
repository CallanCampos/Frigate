// NitroSDK OS: low-quality random data (os_entropy.c). Collects what differs from one run and console to another: the
// screen's scan line, the ticks, the console's MAC address, the vertical blank count, the clock and the state of the
// buttons, microphone and touch screen that the ARM7 leaves in the shared memory.
#include "nitro/gx.h"
#include "nitro/os.h"

// NitroSDK's OS_GetLowEntropyData
void OS_GetLowEntropyData(u32 buffer[8]) {
    OSSystemWork *work = (OSSystemWork *)HW_MAIN_MEM_SHARED_WORK;
    NVRAMConfig *config = (NVRAMConfig *)work->nvramUserInfo;

    buffer[0] = (GX_GetVCount() << 16) | OS_GetTickLo();
    buffer[1] = (u32)OSi_TickCounter ^ (*(u16 *)&config->macAddress[4] << 16);
    buffer[2] = *(u32 *)&config->macAddress[0] ^ (OSi_TickCounter >> 32) ^ work->vblankCount;
    buffer[2] ^= reg_G3X_GXSTAT;
    buffer[3] = work->realTimeClock[0];
    buffer[4] = work->realTimeClock[1];
    buffer[5] = (work->micSampling << 16) ^ work->micLastAddress;
    buffer[6] = (work->touchPanelX << 16) | work->touchPanelY;
    buffer[7] = reg_PAD_KEYINPUT | *(vu16 *)HW_BUTTON_XY_BUF | (work->unk_398 << 16);
}
