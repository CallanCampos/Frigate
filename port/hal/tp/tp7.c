// The host ARM7's touch panel (PXI tag TOUCHPANEL), for the decomp's lib/nitro/src/spi/tp.c, sampling the host's touch
// state (tp_host.h). The protocol, from tp.c: a command's first word has the command in bits 8 to 14 (0 one sample, 1
// start the automatic sampling with the samples a frame in its low byte and the line to start on in the second word, 2
// stop it); the ARM7 writes a sample to the shared memory (OSSystemWork's touchPanelX and touchPanelY: x in bits 0-11,
// y in 12-23, the touch in bit 24, the validity in 25-26) and answers with the last word's END bit, the command and a
// result in the low byte (0 done, 2 bad parameter, 3 not now, 4 busy; tp.c stops the program on any other). While the
// automatic sampling runs, the ARM7 writes each sample and sends a word with command 0x10.
//
// The raw values follow the calibration that the host's user settings carry (port_tp7_calibration): 16 steps a pixel,
// a pixel's raw value in the middle of its steps, so that TP_GetCalibratedPoint gives the pixel back exactly. Samples
// are always valid. The automatic sampling takes the frame's samples at the VBlank, with the touch as the program set
// it for that frame, instead of at the lines the DS spreads them over: the host's input changes once a frame.
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "hal/tp/tp_host.h"
#include "nitro/pxi.h"

#define TP_COMMAND_SAMPLING 0
#define TP_COMMAND_AUTO_ON 1
#define TP_COMMAND_AUTO_OFF 2
#define TP_COMMAND_AUTO_SAMPLE 0x10

#define TP_RESULT_SUCCESS 0
#define TP_RESULT_INVALID_PARAMETER 2
#define TP_RESULT_ILLEGAL_STATUS 3

// The automatic sampling's limits: up to 4 samples a frame (NitroSDK's TP_SAMPLING_FREQUENCY_MAX), from a line of the
// frame's 263
#define TP_AUTO_FREQUENCY_MAX 4
#define TP_LINES_PER_FRAME 263

// The raw values: 16 steps for each pixel, the pixel's value in the middle
#define TP_RAW_STEP 16
#define TP_RAW_VALUE(pixel) ((pixel) * TP_RAW_STEP + TP_RAW_STEP / 2)
#define TP_SAMPLE_TOUCH (1u << 24)

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192

// The host's touch, which outlives the runs: it is the program's input
static struct {
    BOOL touching;
    int x;
    int y;
} sTouch;

static struct {
    PortArm7SpiCommand command;
    BOOL autoSampling;
    u32 frequency;
} sTp;

static int Clamp(int value, int limit) {
    if (value < 0) {
        return 0;
    }
    return value >= limit ? limit - 1 : value;
}

void port_tp_set_touch(int touching, int x, int y) {
    sTouch.touching = touching != 0;
    sTouch.x = Clamp(x, SCREEN_WIDTH);
    sTouch.y = Clamp(y, SCREEN_HEIGHT);
}

void port_tp_get_touch(int *touching, int *x, int *y) {
    *touching = sTouch.touching;
    *x = sTouch.x;
    *y = sTouch.y;
}

// Two points of the screen, a quarter of its width and height in from opposite corners, and their raw values
void port_tp7_calibration(NVRAMTouchCalibration *calibration) {
    calibration->screenX1 = SCREEN_WIDTH / 8;
    calibration->screenY1 = SCREEN_HEIGHT / 8;
    calibration->screenX2 = SCREEN_WIDTH - SCREEN_WIDTH / 8;
    calibration->screenY2 = SCREEN_HEIGHT - SCREEN_HEIGHT / 8;
    calibration->rawX1 = TP_RAW_VALUE(calibration->screenX1);
    calibration->rawY1 = TP_RAW_VALUE(calibration->screenY1);
    calibration->rawX2 = TP_RAW_VALUE(calibration->screenX2);
    calibration->rawY2 = TP_RAW_VALUE(calibration->screenY2);
}

// A sample of the touch now, in the shared memory. Without a touch the coordinates are 0
static void StoreSample(void) {
    OSSystemWork *work = OS_GetSystemWork();
    u32 sample = 0;

    if (sTouch.touching) {
        sample = (u32)TP_RAW_VALUE(sTouch.x) | ((u32)TP_RAW_VALUE(sTouch.y) << 12) | TP_SAMPLE_TOUCH;
    }
    work->touchPanelX = (u16)sample;
    work->touchPanelY = (u16)(sample >> 16);
}

static void Answer(u32 command, u32 result) {
    port_pxi_reply(PXI_FIFO_TAG_TOUCHPANEL, PORT_ARM7_SPI_ANSWER(command, result), FALSE);
}

static void TpService(u32 tag, u32 data, BOOL err) {
    u32 command;

    if (err || !port_arm7_spi_collect(&sTp.command, data)) {
        return;
    }
    command = sTp.command.command;
    switch (command) {
    case TP_COMMAND_SAMPLING:
        StoreSample();
        Answer(command, TP_RESULT_SUCCESS);
        break;
    case TP_COMMAND_AUTO_ON: {
        u32 frequency = sTp.command.data[0];
        u32 line = sTp.command.words > 1 ? sTp.command.data[1] : TP_LINES_PER_FRAME;

        if (sTp.autoSampling) {
            Answer(command, TP_RESULT_ILLEGAL_STATUS);
        } else if (frequency == 0 || frequency > TP_AUTO_FREQUENCY_MAX || line >= TP_LINES_PER_FRAME) {
            Answer(command, TP_RESULT_INVALID_PARAMETER);
        } else {
            sTp.autoSampling = TRUE;
            sTp.frequency = frequency;
            Answer(command, TP_RESULT_SUCCESS);
        }
        break;
    }
    case TP_COMMAND_AUTO_OFF:
        if (!sTp.autoSampling) {
            Answer(command, TP_RESULT_ILLEGAL_STATUS);
        } else {
            sTp.autoSampling = FALSE;
            Answer(command, TP_RESULT_SUCCESS);
        }
        break;
    default:
        Answer(command, TP_RESULT_INVALID_PARAMETER);
        break;
    }
}

void port_tp7_start(void) {
    OSSystemWork *work = OS_GetSystemWork();

    sTp.command.count = 0;
    sTp.autoSampling = FALSE;
    sTp.frequency = 0;
    work->touchPanelX = 0;
    work->touchPanelY = 0;
    port_pxi_register_service(PXI_FIFO_TAG_TOUCHPANEL, TpService);
}

void port_tp7_frame(u64 cycles) {
    u32 i;

    (void)cycles;
    if (!sTp.autoSampling) {
        return;
    }
    for (i = 0; i < sTp.frequency; i++) {
        StoreSample();
        port_pxi_reply(PXI_FIFO_TAG_TOUCHPANEL, PORT_ARM7_SPI_ANSWER(TP_COMMAND_AUTO_SAMPLE, 0), FALSE);
    }
}
