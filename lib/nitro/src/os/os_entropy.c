// NitroSDK OS: low-quality random data (os_entropy.c). Collects what differs from one run and console to another: the
// screen's scan line, the ticks, the console's MAC address, the vertical blank count, the clock and the state of the
// buttons, microphone and touch screen that the ARM7 leaves in the shared memory.
#include "nitro/gx.h"
#include "nitro/os.h"

// NitroSDK's OS_GetLowEntropyData
void OS_GetLowEntropyData(u32 buffer[8]) {
    const OSSystemWork *work = OS_GetSystemWork();
    const u8 *macAddress = ((const NVRAMConfig *)work->nvramUserInfo)->macAddress;

    buffer[0] = (GX_GetVCount() << 16) | OS_GetTickLo();
    buffer[1] = (*(const u16 *)&macAddress[4] << 16) ^ (u32)OSi_TickCounter;
    buffer[2] = (OSi_TickCounter >> 32) ^ *(const u32 *)macAddress ^ work->vblankCount;
    buffer[2] ^= reg_G3X_GXSTAT;
    buffer[3] = work->real_time_clock[0];
    buffer[4] = work->real_time_clock[1];
    buffer[5] = (work->mic_sampling_data << 16) ^ work->mic_last_address;
    buffer[6] = (work->touchPanelX << 16) | work->touchPanelY;
    buffer[7] = reg_PAD_KEYINPUT | *(vu16 *)HW_BUTTON_XY_BUF | (work->wm_rssi_pool << 16);
}
