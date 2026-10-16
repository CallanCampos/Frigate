#include "types.h"
#include "twl/dsp.h"

// TwlSDK's DSP library, the graphics component (guessed file name, after the SDK's dsp_graphics.c): the DSP process
// that scales the camera's pictures, as a COFF image in the LTD autoload's .rodata. The ARM9 tells it what to scale
// through the pipe of port 3 and the DSP answers on reply 1, which the ARM9 either waits for or has func_0270734c
// handle from the DSP's interrupt. ARM code in the DSi-only LTD autoload. The ROM names none of it; the functions keep
// their default names, and the SDK's are in twl/dsp.h.

// What the DSP gets to scale one picture, as its pipe carries it
typedef struct DSPScalingRequest {
    const void *src;
    void *dst;
    u16 mode;
    u16 imageWidth;
    u16 imageHeight;
    u16 scaleX; // the scale in thousandths
    u16 scaleY;
    u16 x; // the area to scale
    u16 y;
    u16 width;
    u16 height;
} DSPScalingRequest;

#define DSP_GRAPHICS_REPLY 1 // the reply on which the DSP answers
#define DSP_GRAPHICS_PIPE_PORT 3
#define DSP_GRAPHICS_RESULT_OK 1

static void func_0270734c(void *arg);

// The component's state. The two flags are volatile because func_0270734c clears them in the DSP's interrupt while
// func_02707494 may be looking at them
static DSPPipe sGraphicsPipe;
static u16 sGraphicsResult;
static volatile BOOL sGraphicsAsync;
static void (*sGraphicsCallback)(void);
static volatile BOOL sGraphicsBusy;
static BOOL sGraphicsLoaded;
static DSPProcessContext sGraphicsContext;

// Opens the component's image, as a file of memory, in file
BOOL func_02707310(FSFile *file) {
    if (!fs_init_check()) {
        fs_init(FS_DMA_NOT_USE);
    }
    finit(file);
    return extfs_fopen(file, DSP_GraphicsImage, DSP_COMPONENT_IMAGE_SIZE);
}

// The DSP's answer to an asynchronous scaling: whether it worked or not, the scaling is over, and only a success is
// called back
static void func_0270734c(void *arg) {
    if (sGraphicsAsync) {
        sGraphicsResult = DSPi_RecvData(DSP_GRAPHICS_REPLY);
        if (sGraphicsResult == DSP_GRAPHICS_RESULT_OK) {
            sGraphicsBusy = FALSE;
            sGraphicsAsync = FALSE;
            if (sGraphicsCallback != NULL) {
                sGraphicsCallback();
            }
        } else {
            sGraphicsBusy = FALSE;
            sGraphicsAsync = FALSE;
        }
    }
}

// Starts the process from the image in file, into the banks of WRAM B and C that slotB and slotC allow
BOOL func_027073b8(FSFile *file, int slotB, int slotC) {
    if (!sGraphicsLoaded) {
        sGraphicsBusy = FALSE;
        sGraphicsAsync = FALSE;
        func_027058d8(&sGraphicsContext, "graphics");
        sGraphicsContext.flags |= DSP_PROCESS_USING_OS;
        func_02705b70(&sGraphicsContext, 1 << (DSP_EVENT_REPLY_SHIFT + DSP_GRAPHICS_REPLY), func_0270734c, NULL);
        if (func_02705c24(&sGraphicsContext, file, slotB, slotC)) {
            sGraphicsLoaded = TRUE;
        }
        func_02706d44(&sGraphicsPipe, DSP_GRAPHICS_PIPE_PORT, 1);
    }
    return sGraphicsLoaded;
}

void func_02707464(void) {
    if (sGraphicsLoaded) {
        func_02705e04(&sGraphicsContext);
        sGraphicsLoaded = FALSE;
    }
}

// Scales the area at (x, y) of a width by height image from src to dst. If async, returns TRUE at once and calls
// callback when the DSP is done; if not, waits for the DSP and returns whether it scaled the image
BOOL func_02707494(const void *src, void *dst, u16 imageWidth, u16 imageHeight, f32 rx, f32 ry, u16 mode, u16 x, u16 y,
                   u16 width, u16 height, void (*callback)(void), BOOL async) {
    DSPScalingRequest request;
    if (sGraphicsBusy) {
        return FALSE;
    }
    sGraphicsBusy = TRUE;
    sGraphicsCallback = callback;
    sGraphicsAsync = async;
    if (async) {
        func_02705b70(&sGraphicsContext, 1 << (DSP_EVENT_REPLY_SHIFT + DSP_GRAPHICS_REPLY), func_0270734c, NULL);
    } else {
        func_02705b70(&sGraphicsContext, 1 << (DSP_EVENT_REPLY_SHIFT + DSP_GRAPHICS_REPLY), NULL, NULL);
    }
    DSPi_SendData(0, 1);
    request.src = src;
    request.dst = dst;
    request.mode = mode;
    request.imageWidth = imageWidth;
    request.imageHeight = imageHeight;
    request.scaleX = 1000.0f * rx;
    request.scaleY = 1000.0f * ry;
    request.x = x;
    request.y = y;
    request.width = width;
    request.height = height;
    func_02706fcc(&sGraphicsPipe, &request, sizeof(request));
    if (sGraphicsAsync) {
        return TRUE;
    }
    while (!DSPi_RecvDataIsReady(DSP_GRAPHICS_REPLY)) {
        func_0207aa04(1);
    }
    sGraphicsResult = DSPi_RecvData(DSP_GRAPHICS_REPLY);
    sGraphicsBusy = FALSE;
    if (sGraphicsResult == DSP_GRAPHICS_RESULT_OK) {
        return TRUE;
    }
    return FALSE;
}
