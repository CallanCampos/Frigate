#include "types.h"
#include "twl/dsp.h"

// TwlSDK's DSP library, the graphics component (guessed file name, after the SDK's dsp_graphics.c): the DSP process
// that scales the camera's pictures, as a COFF image in the LTD autoload's .rodata. The ARM9 tells it what to scale
// through the pipe of port 3 and the DSP answers on reply 1, which the ARM9 either waits for or has func_0270734c
// handle from the DSP's interrupt. ARM code in the DSi-only LTD autoload. The ROM names none of it; the functions have
// the SDK's names where twl/dsp.h knows them, and keep their default names otherwise.

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
// DSPi_ScalingCore may be looking at them
static DSPPipe sGraphicsPipe;
static u16 sGraphicsResult;
static volatile BOOL sGraphicsAsync;
static void (*sGraphicsCallback)(void);
// Cleared by func_0270734c in the DSP's interrupt
static volatile BOOL sGraphicsBusy;
static BOOL sGraphicsLoaded;
static DSPProcessContext sGraphicsContext;

// Opens the component's image, as a file of memory, in file
BOOL DSP_OpenStaticComponentGraphicsCore(FSFile *file) {
    if (!FS_IsAvailable()) {
        FS_Init(FS_DMA_NOT_USE);
    }
    FS_InitFile(file);
    return FS_CreateFileFromMemory(file, DSP_GraphicsImage, DSP_COMPONENT_IMAGE_SIZE);
}

// The DSP's answer to an asynchronous scaling: whether it worked or not, the scaling is over, and only a success is
// called back
static void func_0270734c(void *arg) {
    if (sGraphicsAsync) {
        sGraphicsResult = DSP_RecvData(DSP_GRAPHICS_REPLY);
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
BOOL DSP_LoadGraphicsCore(FSFile *file, int slotB, int slotC) {
    if (!sGraphicsLoaded) {
        sGraphicsBusy = FALSE;
        sGraphicsAsync = FALSE;
        DSP_InitProcessContext(&sGraphicsContext, "graphics");
        sGraphicsContext.flags |= DSP_PROCESS_USING_OS;
        DSP_SetCallback(&sGraphicsContext, 1 << (DSP_EVENT_REPLY_SHIFT + DSP_GRAPHICS_REPLY), func_0270734c, NULL);
        if (DSP_ExecuteProcess(&sGraphicsContext, file, slotB, slotC)) {
            sGraphicsLoaded = TRUE;
        }
        DSP_LoadPipe(&sGraphicsPipe, DSP_GRAPHICS_PIPE_PORT, 1);
    }
    return sGraphicsLoaded;
}

void DSP_UnloadGraphicsCore(void) {
    if (sGraphicsLoaded) {
        DSP_QuitProcess(&sGraphicsContext);
        sGraphicsLoaded = FALSE;
    }
}

// Scales the area at (x, y) of a width by height image from src to dst. If async, returns TRUE at once and calls
// callback when the DSP is done; if not, waits for the DSP and returns whether it scaled the image
BOOL DSPi_ScalingCore(const void *src, void *dst, u16 imageWidth, u16 imageHeight, f32 rx, f32 ry, u16 mode, u16 x,
                      u16 y, u16 width, u16 height, void (*callback)(void), BOOL async) {
    DSPScalingRequest request;
    if (sGraphicsBusy) {
        return FALSE;
    }
    sGraphicsBusy = TRUE;
    sGraphicsCallback = callback;
    sGraphicsAsync = async;
    if (async) {
        DSP_SetCallback(&sGraphicsContext, 1 << (DSP_EVENT_REPLY_SHIFT + DSP_GRAPHICS_REPLY), func_0270734c, NULL);
    } else {
        DSP_SetCallback(&sGraphicsContext, 1 << (DSP_EVENT_REPLY_SHIFT + DSP_GRAPHICS_REPLY), NULL, NULL);
    }
    DSP_SendData(0, 1);
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
    DSP_WritePipe(&sGraphicsPipe, &request, sizeof(request));
    if (sGraphicsAsync) {
        return TRUE;
    }
    while (!DSP_RecvDataIsReady(DSP_GRAPHICS_REPLY)) {
        OS_Sleep(1);
    }
    sGraphicsResult = DSP_RecvData(DSP_GRAPHICS_REPLY);
    sGraphicsBusy = FALSE;
    if (sGraphicsResult == DSP_GRAPHICS_RESULT_OK) {
        return TRUE;
    }
    return FALSE;
}
