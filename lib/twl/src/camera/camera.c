// TwlSDK's camera library (libcamera), the camera interface's registers, in the DSi's LTD autoload: resetting the
// interface and its clock, starting and stopping a capture, and the interface's control bits (the transfer size, the
// trimming, the output format, the interrupt). ARM code. The ROM names none of it; the file's name is a guess, after
// the SDK's camera.h, whose Core names twl/camera.h gives. The functions keep their default names.
//
// The interface has two registers in the I/O space after SCFG_CLK: 0x04004200, which the reset writes, and 0x04004202,
// the control word. The functions that change the control word do so with the interrupts off.

#include "types.h"
#include "nitro/os.h"
#include "twl/camera.h"
#include "twl/scfg.h"

#define reg_CAMERA_MCNT (*(vu16 *)0x04004200)
#define reg_CAMERA_CNT (*(vu16 *)0x04004202)

// What the reset leaves in the first register after the camera's clock has been on
#define CAMERA_MCNT_RESET 0x22

// The control word
#define CAMERA_CNT_LINES_MASK 0xf // The number of lines per transfer, less one
#define CAMERA_CNT_BUFFER_ERROR 0x10
#define CAMERA_CNT_CLEAR_BUFFER 0x20
#define CAMERA_CNT_UNK_MASK 0x300    // A field of two bits, which camera_api.c sets to 2 while the camera runs
#define CAMERA_CNT_UNK_BIT10 0x400   // Set while the camera runs
#define CAMERA_CNT_INTR_ENABLE 0x800 // Set while the camera interrupt has a handler
#define CAMERA_CNT_FORMAT_RGB 0x2000
#define CAMERA_CNT_TRIMMING 0x4000
#define CAMERA_CNT_CAPTURE 0x8000 // Set to start a capture, and on until it ends

static u32 func_027022e0(void);

// Resets the interface: the interface's clock on, its first register cleared, the camera's clock on for the reset, and
// then off again, with the SDK's delays in between
void func_02702080(void) {
    reg_SCFG_CLK |= SCFG_CLK_CAMERA_INTERFACE;
    reg_CAMERA_MCNT = 0;
    func_0207c160(0x1e);
    reg_SCFG_CLK |= SCFG_CLK_CAMERA;
    func_0207c160(0x1e);
    reg_CAMERA_MCNT = CAMERA_MCNT_RESET;
    func_0207c160(0x2008);
    reg_SCFG_CLK &= ~SCFG_CLK_CAMERA;
}

// CAMERA_IsBusyCore
BOOL func_027020f0(void) {
    return (reg_CAMERA_CNT & CAMERA_CNT_CAPTURE) >> 15;
}

// CAMERA_StartCaptureCore
void func_02702108(void) {
    u32 enabled = CPU_IRQDisable();
    reg_CAMERA_CNT |= CAMERA_CNT_CAPTURE;
    CPU_SetIRQMask(enabled);
}

// CAMERA_StopCaptureCore
void func_0270212c(void) {
    u32 enabled = CPU_IRQDisable();
    reg_CAMERA_CNT &= ~CAMERA_CNT_CAPTURE;
    CPU_SetIRQMask(enabled);
}

// CAMERA_SetTrimmingCore
void func_02702158(BOOL enabled) {
    u32 irq = CPU_IRQDisable();
    u32 cnt = reg_CAMERA_CNT;
    if (enabled) {
        cnt |= CAMERA_CNT_TRIMMING;
    } else {
        cnt &= ~CAMERA_CNT_TRIMMING;
    }
    reg_CAMERA_CNT = cnt;
    CPU_SetIRQMask(irq);
}

// CAMERA_SetOutputFormatCore
void func_0270218c(CAMERAOutput output) {
    u32 irq = CPU_IRQDisable();
    u32 cnt = reg_CAMERA_CNT;
    switch (output) {
    case CAMERA_OUTPUT_YUV:
        reg_CAMERA_CNT = cnt & ~CAMERA_CNT_FORMAT_RGB;
        break;
    case CAMERA_OUTPUT_RGB:
        reg_CAMERA_CNT = cnt | CAMERA_CNT_FORMAT_RGB;
        break;
    }
    CPU_SetIRQMask(irq);
}

// Whether the interface reports a buffer error (CAMERA_IsBufferErrorCore, perhaps)
BOOL func_027021cc(void) {
    return (reg_CAMERA_CNT & CAMERA_CNT_BUFFER_ERROR) >> 4;
}

// CAMERA_ClearBufferCore
void func_027021e4(void) {
    u32 enabled = CPU_IRQDisable();
    reg_CAMERA_CNT |= CAMERA_CNT_CLEAR_BUFFER;
    CPU_SetIRQMask(enabled);
}

// Sets or clears the interface's interrupt enable (CAMERA_SetInterruptCore, perhaps)
void func_02702208(BOOL on) {
    u32 irq = CPU_IRQDisable();
    u32 cnt = reg_CAMERA_CNT;
    if (on) {
        cnt |= CAMERA_CNT_INTR_ENABLE;
    } else {
        cnt &= ~CAMERA_CNT_INTR_ENABLE;
    }
    reg_CAMERA_CNT = cnt;
    CPU_SetIRQMask(irq);
}

// Replaces the two-bit field of bits 8 and 9 of the control word
void func_0270223c(u32 bits) {
    u32 enabled = CPU_IRQDisable();
    reg_CAMERA_CNT = (reg_CAMERA_CNT & ~CAMERA_CNT_UNK_MASK) | bits;
    CPU_SetIRQMask(enabled);
}

// Sets or clears bit 10 of the control word
void func_02702268(BOOL on) {
    u32 irq = CPU_IRQDisable();
    u32 cnt = reg_CAMERA_CNT;
    if (on) {
        cnt |= CAMERA_CNT_UNK_BIT10;
    } else {
        cnt &= ~CAMERA_CNT_UNK_BIT10;
    }
    reg_CAMERA_CNT = cnt;
    CPU_SetIRQMask(irq);
}

// CAMERA_SetTransferLinesCore, which takes 1 to 16 lines and ignores any other
void func_0270229c(int lines) {
    u32 enabled;
    if (lines < 1 || lines > 16) {
        return;
    }
    enabled = CPU_IRQDisable();
    reg_CAMERA_CNT = (reg_CAMERA_CNT & ~CAMERA_CNT_LINES_MASK) | (u16)(lines - 1);
    CPU_SetIRQMask(enabled);
}

// The number of lines per transfer that the control word holds
static u32 func_027022e0(void) {
    return (reg_CAMERA_CNT & CAMERA_CNT_LINES_MASK) + 1;
}

// CAMERA_GetMaxLinesRoundCore: the most lines, up to 16, with at most 1024 pixels in all, that divide the height
int func_027022f8(u16 width, u16 height) {
    int lines = 0x400 / width;
    if (lines > 16) {
        lines = 16;
    }
    while (lines > 1) {
        if (height % lines == 0) {
            return lines;
        }
        lines--;
    }
    return 1;
}

// CAMERA_GetBytesAtOnceCore: the bytes of one transfer, two per pixel of a line
u32 func_0270234c(u16 width) {
    return width * 2 * func_027022e0();
}

// CAMERA_SizeToWidthCore
u16 func_02702364(CAMERASize size) {
    switch (size) {
    case CAMERA_SIZE_DSI_VGA:
        return 640;
    case CAMERA_SIZE_QVGA:
        return 320;
    case CAMERA_SIZE_QQVGA:
        return 160;
    case CAMERA_SIZE_CIF:
        return 352;
    case CAMERA_SIZE_QCIF:
        return 176;
    case CAMERA_SIZE_DS_LCD:
        return 256;
    case CAMERA_SIZE_DS_LCDX4:
        return 512;
    default:
        return 0;
    }
}

// CAMERA_SizeToHeightCore
u16 func_027023cc(CAMERASize size) {
    switch (size) {
    case CAMERA_SIZE_DSI_VGA:
        return 480;
    case CAMERA_SIZE_QVGA:
        return 240;
    case CAMERA_SIZE_QQVGA:
        return 120;
    case CAMERA_SIZE_CIF:
        return 288;
    case CAMERA_SIZE_QCIF:
        return 144;
    case CAMERA_SIZE_DS_LCD:
        return 192;
    case CAMERA_SIZE_DS_LCDX4:
        return 384;
    default:
        return 0;
    }
}
