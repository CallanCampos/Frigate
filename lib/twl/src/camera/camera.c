// TwlSDK's camera library (libcamera), the camera interface's registers, in the DSi's LTD autoload: resetting the
// interface and its clock, starting and stopping a capture, and the interface's control bits (the transfer size, the
// trimming, the output format, the interrupt). ARM code. The ROM names none of it; the file's name is a guess, after
// the SDK's camera.h, whose Core names twl/camera.h gives. The functions have those names where they are known, and
// keep their default names otherwise.
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
void CAMERA_ResetCore(void) {
    reg_SCFG_CLK |= SCFG_CLK_CAMERA_INTERFACE;
    reg_CAMERA_MCNT = 0;
    OS_SpinWaitSysCycles(0x1e);
    reg_SCFG_CLK |= SCFG_CLK_CAMERA;
    OS_SpinWaitSysCycles(0x1e);
    reg_CAMERA_MCNT = CAMERA_MCNT_RESET;
    OS_SpinWaitSysCycles(0x2008);
    reg_SCFG_CLK &= ~SCFG_CLK_CAMERA;
}

BOOL CAMERA_IsBusyCore(void) {
    return (reg_CAMERA_CNT & CAMERA_CNT_CAPTURE) >> 15;
}

void CAMERA_StartCaptureCore(void) {
    u32 enabled = OS_DisableInterrupts();
    reg_CAMERA_CNT |= CAMERA_CNT_CAPTURE;
    OS_RestoreInterrupts(enabled);
}

void CAMERA_StopCaptureCore(void) {
    u32 enabled = OS_DisableInterrupts();
    reg_CAMERA_CNT &= ~CAMERA_CNT_CAPTURE;
    OS_RestoreInterrupts(enabled);
}

void CAMERA_SetTrimmingCore(BOOL enabled) {
    u32 irq = OS_DisableInterrupts();
    u32 cnt = reg_CAMERA_CNT;
    if (enabled) {
        cnt |= CAMERA_CNT_TRIMMING;
    } else {
        cnt &= ~CAMERA_CNT_TRIMMING;
    }
    reg_CAMERA_CNT = cnt;
    OS_RestoreInterrupts(irq);
}

void CAMERA_SetOutputFormatCore(CAMERAOutput output) {
    u32 irq = OS_DisableInterrupts();
    u32 cnt = reg_CAMERA_CNT;
    switch (output) {
    case CAMERA_OUTPUT_YUV:
        reg_CAMERA_CNT = cnt & ~CAMERA_CNT_FORMAT_RGB;
        break;
    case CAMERA_OUTPUT_RGB:
        reg_CAMERA_CNT = cnt | CAMERA_CNT_FORMAT_RGB;
        break;
    }
    OS_RestoreInterrupts(irq);
}

// Whether the interface reports a buffer error
BOOL CAMERA_IsBufferErrorCore(void) {
    return (reg_CAMERA_CNT & CAMERA_CNT_BUFFER_ERROR) >> 4;
}

void CAMERA_ClearBufferCore(void) {
    u32 enabled = OS_DisableInterrupts();
    reg_CAMERA_CNT |= CAMERA_CNT_CLEAR_BUFFER;
    OS_RestoreInterrupts(enabled);
}

// Sets or clears the interface's interrupt enable
void CAMERA_SetInterruptEnableCore(BOOL on) {
    u32 irq = OS_DisableInterrupts();
    u32 cnt = reg_CAMERA_CNT;
    if (on) {
        cnt |= CAMERA_CNT_INTR_ENABLE;
    } else {
        cnt &= ~CAMERA_CNT_INTR_ENABLE;
    }
    reg_CAMERA_CNT = cnt;
    OS_RestoreInterrupts(irq);
}

// Replaces the two-bit field of bits 8 and 9 of the control word
void func_0270223c(u32 bits) {
    u32 enabled = OS_DisableInterrupts();
    reg_CAMERA_CNT = (reg_CAMERA_CNT & ~CAMERA_CNT_UNK_MASK) | bits;
    OS_RestoreInterrupts(enabled);
}

// Sets or clears bit 10 of the control word
void func_02702268(BOOL on) {
    u32 irq = OS_DisableInterrupts();
    u32 cnt = reg_CAMERA_CNT;
    if (on) {
        cnt |= CAMERA_CNT_UNK_BIT10;
    } else {
        cnt &= ~CAMERA_CNT_UNK_BIT10;
    }
    reg_CAMERA_CNT = cnt;
    OS_RestoreInterrupts(irq);
}

// CAMERA_SetTransferLinesCore, which takes 1 to 16 lines and ignores any other
void CAMERA_SetTransferLinesCore(int lines) {
    u32 enabled;
    if (lines < 1 || lines > 16) {
        return;
    }
    enabled = OS_DisableInterrupts();
    reg_CAMERA_CNT = (reg_CAMERA_CNT & ~CAMERA_CNT_LINES_MASK) | (u16)(lines - 1);
    OS_RestoreInterrupts(enabled);
}

// The number of lines per transfer that the control word holds
static u32 func_027022e0(void) {
    return (reg_CAMERA_CNT & CAMERA_CNT_LINES_MASK) + 1;
}

// The most lines, up to 16, with at most 1024 pixels in all, that divide the height
int CAMERA_GetMaxLinesRoundCore(u16 width, u16 height) {
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

// The bytes of one transfer, two per pixel of a line
u32 CAMERA_GetBytesAtOnceCore(u16 width) {
    return width * 2 * func_027022e0();
}

u16 CAMERA_SizeToWidthCore(CAMERASize size) {
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

u16 CAMERA_SizeToHeightCore(CAMERASize size) {
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
