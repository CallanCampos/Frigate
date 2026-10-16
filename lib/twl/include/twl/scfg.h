#ifndef POKEBW2_TWL_SCFG_H
#define POKEBW2_TWL_SCFG_H

#include "types.h"

// TwlSDK's SCFG library (libscfg): the DSi's system configuration registers, of which this build keeps the ARM9 clock
// switch and the PXI service that collects configuration data from the ARM7

// SCFG_CLK, the DSi's clock register: bit 0 doubles the ARM9's clock (the DS's 67.03 MHz to the DSi's 134.06 MHz),
// bit 1 runs the DSP, bit 2 the camera's interface and bit 8 the camera
#define reg_SCFG_CLK (*(vu16 *)0x04004004)
#define SCFG_CLK_CPU_2X 0x0001
#define DSP_SCFG_CLK_ENABLE 0x0002
#define SCFG_CLK_CAMERA_INTERFACE 0x0004
#define SCFG_CLK_CAMERA 0x0100
// SCFG_RST: bit 0 takes the DSP out of its reset, which it is held in while the bit is low
#define reg_SCFG_RST (*(vu16 *)0x04004006)
#define DSP_SCFG_RST_RELEASE 0x0001

typedef enum SCFGCpuSpeed {
    SCFG_CPU_SPEED_1X = 0,
    SCFG_CPU_SPEED_2X = 1,
} SCFGCpuSpeed;

// Called with the ARM7's answer to the request that registered it: the 64 bits of data that it sent in four words, or
// a single 16-bit value in the low bits
typedef void (*SCFGCallback)(u64 response, void *arg);

// Takes the PXI words of the SCFG service
void SCFG_Init(void);
// Switches the ARM9 to the DS or the DSi clock, with the interrupts off
void SCFG_SetCpuSpeed(SCFGCpuSpeed speed);

// SCFGi_SwitchCpuSpeed, in ITCM (the last function of it): writes bit 0 of SCFG_CLK and waits for the clock to settle
void SCFGi_SwitchCpuSpeed(SCFGCpuSpeed speed);

#endif // POKEBW2_TWL_SCFG_H
