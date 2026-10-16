#include "app/comm_tvt/camera_system.h"
#include "types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/ui.h"
#include "nitro/fs.h"
#include "nitro/os.h"
#include "twl/camera.h"
#include "twl/dsp.h"
#include "twl/mi.h"

// The DSi camera for the Xtransceiver: it captures frames from one of the two cameras into a ring of buffers by NDMA,
// and plays the shutter and video sounds that a DSi must play while it records, through the DSP. The camera's
// interrupt callbacks take no work, so the system is kept in sCameraSystem. Built on TwlSDK's camera demos

// The NDMA channel that receives the frames
#define CAMERA_NDMA_NO 1

// The sounds in the archive
#define ARC_CAMERA_SOUND 209
enum {
    CAMERA_SOUND_SHUTTER,
    CAMERA_SOUND_VIDEO_START,
    CAMERA_SOUND_VIDEO_END,
};

// The frame counter stops here, and a frame is only passed on once the counter is past CAMERA_SETTLE_FRAMES
#define CAMERA_FRAME_COUNT_MAX 30
#define CAMERA_SETTLE_FRAMES 4

// HID flags of the camera, which keep the game from sleeping
#define CAMERA_HID_FLAG 0x40

// The sounds a DSi plays while its camera records, which CameraSystem_UpdateSound plays one step at a time
enum {
    CAMERA_SOUND_STATE_NONE,
    CAMERA_SOUND_STATE_VIDEO_START,
    CAMERA_SOUND_STATE_VIDEO_START_WAIT,
    CAMERA_SOUND_STATE_RECORDING,
    CAMERA_SOUND_STATE_VIDEO_END,
    CAMERA_SOUND_STATE_VIDEO_END_WAIT,
    CAMERA_SOUND_STATE_ENDED,
    CAMERA_SOUND_STATE_SHUTTER,
    CAMERA_SOUND_STATE_SHUTTER_WAIT,
};

struct CameraSystem {
    HeapID heapId;
    void **buffers;
    u8 numBuffers;
    BOOL buffersAllocated;
    BOOL capturing;
    BOOL unk14;
    u8 bufferIndex;
    BOOL startCapture;
    BOOL switchCamera;
    // Set after a buffer error, to drop the frame that the restart cut short
    BOOL skipFrame;
    u32 frameCount;
    CAMERASelect camera;
    CAMERASize size;
    u16 trimLeft;
    u16 trimRight;
    u16 trimTop;
    u16 trimBottom;
    CameraFrameCallback callback;
    void *callbackWork;
    u8 soundState;
    void *soundData;
    u32 soundSize;
};

static void CameraSystem_OnSoftReset(void *work);
static void CameraSystem_StartDma(CameraSystem *sys);
static void CameraSystem_LoadSound(CameraSystem *sys, BOOL start);
static void CameraSystem_FreeSound(CameraSystem *sys);
static void CameraSystem_LoadShutterSound(CameraSystem *sys);
static void CameraSystem_OnBufferError(CAMERAResult result);
static void CameraSystem_OnReboot(CAMERAResult result);
static void CameraSystem_OnVsync(CAMERAResult result);
static void CameraSystem_OnDmaDone(void *arg);
static void CameraSystem_FreeBuffers(CameraSystem *sys);
static void CameraSystem_SetupCapture(CameraSystem *sys);
static u16 CameraSystem_GetWidth(CameraSystem *sys);
static u16 CameraSystem_GetHeight(CameraSystem *sys);
static u16 CameraSystem_SizeToWidth(CameraSystem *sys);
static u16 CameraSystem_SizeToHeight(CameraSystem *sys);

static CameraSystem *sCameraSystem;

CameraSystem *CameraSystem_Create(HeapID heapId) {
    FSFile file;
    CAMERAResult result;
    CameraSystem *sys = GFL_HeapAllocate(heapId, sizeof(CameraSystem), FALSE, "camera_system.c", 116);

    sCameraSystem = sys;
    sys->heapId = heapId;
    sys->buffers = NULL;
    sys->bufferIndex = 0;
    sys->numBuffers = 0;
    sys->buffersAllocated = FALSE;
    sys->capturing = FALSE;
    sys->unk14 = FALSE;
    sys->camera = CAMERA_SELECT_IN;
    sys->size = CAMERA_SIZE_DS_LCD;
    sys->startCapture = FALSE;
    sys->switchCamera = FALSE;
    sys->skipFrame = FALSE;
    sys->callback = NULL;
    sys->callbackWork = NULL;
    GCTX_HIDBlockSleep(CAMERA_HID_FLAG);

    result = CAMERA_Init();
    if (result == CAMERA_RESULT_FATAL_ERROR) {
        sys_exit();
    }
    if (result == CAMERA_RESULT_ILLEGAL_STATUS) {
        return NULL;
    }
    CAMERA_I2CEffectEx(sys->camera, CAMERA_CONTEXT_BOTH, CAMERA_EFFECT_NONE);
    CAMERA_I2CAutoExposure(sys->camera, TRUE);
    CAMERA_I2CAutoWhiteBalance(sys->camera, TRUE);
    result = CAMERA_I2CActivate(sys->camera);
    if (result == CAMERA_RESULT_FATAL_ERROR) {
        sys_exit();
    }
    if (result == CAMERA_RESULT_ILLEGAL_STATUS) {
        return NULL;
    }

    sys->frameCount = 0;
    CPU_EnableInterrupts(OS_IE_NDMA1);
    CAMERA_SetVsyncCallback(CameraSystem_OnVsync);
    CAMERA_SetBufferErrorCallback(CameraSystem_OnBufferError);
    CAMERA_SetRebootCallback(CameraSystem_OnReboot);
    CameraSystem_SetupCapture(sys);
    CAMERA_SetOutputFormat(CAMERA_OUTPUT_RGB);

    sys->soundData = NULL;
    sys->soundState = CAMERA_SOUND_STATE_NONE;
    func_02768cf8(MI_WRAM_B, MI_WRAM_ARM9);
    func_02769080(MI_WRAM_B, MI_WRAM_ARM9);
    func_02769080(MI_WRAM_B, MI_WRAM_ARM7);
    func_02768cf8(MI_WRAM_C, MI_WRAM_ARM9);
    func_02769080(MI_WRAM_C, MI_WRAM_ARM9);
    func_02769080(MI_WRAM_C, MI_WRAM_ARM7);
    DSP_OpenStaticComponentG711(&file);
    if (!DSP_LoadG711(&file, 0xff, 0xff)) {
        sys_exit();
    }
    GCTX_HIDSetSoftResetCallback(CameraSystem_OnSoftReset, sys);
    return sys;
}

static void CameraSystem_OnSoftReset(void *work) {
    CameraSystem *sys = work;

    while (DSP_IsShutterSoundPlaying() == TRUE) {
        func_0207aa04(10);
    }
    DSP_UnloadG711();
    CameraSystem_Stop(sys);
    CAMERA_I2CStop();
    func_02768270(CAMERA_NDMA_NO);
    CAMERA_End();
}

void CameraSystem_Delete(CameraSystem *sys) {
    GCTX_HIDSetSoftResetCallback(NULL, NULL);
    DSP_StopSound();
    DSP_UnloadG711();
    CameraSystem_Stop(sys);
    CAMERA_I2CStop();
    func_02768270(CAMERA_NDMA_NO);
    CAMERA_End();
    CameraSystem_FreeSound(sys);
    CameraSystem_FreeBuffers(sys);
    GCTX_HIDUnblockSleep(CAMERA_HID_FLAG);
    GFL_HeapFree(sys);
    sCameraSystem = NULL;
}

static void CameraSystem_StartDma(CameraSystem *sys) {
    u16 width = CameraSystem_GetWidth(sys);
    u16 height = CameraSystem_GetHeight(sys);

    CAMERA_DmaRecvAsync(CAMERA_NDMA_NO, sys->buffers[sys->bufferIndex], CAMERA_GetBytesAtOnce(width),
                        (width << 1) * height, CameraSystem_OnDmaDone, NULL);
}

void CameraSystem_UpdateSound(CameraSystem *sys) {
    switch (sys->soundState) {
    case CAMERA_SOUND_STATE_VIDEO_START:
        CameraSystem_LoadSound(sys, TRUE);
        DSP_PlayShutterSound(sys->soundData, sys->soundSize);
        sys->soundState = CAMERA_SOUND_STATE_VIDEO_START_WAIT;
        break;
    case CAMERA_SOUND_STATE_VIDEO_START_WAIT:
        if (DSP_IsShutterSoundPlaying() == FALSE) {
            CameraSystem_FreeSound(sys);
            sys->soundState = CAMERA_SOUND_STATE_RECORDING;
        }
        break;
    case CAMERA_SOUND_STATE_VIDEO_END:
        CameraSystem_LoadSound(sys, FALSE);
        DSP_PlayShutterSound(sys->soundData, sys->soundSize);
        sys->soundState = CAMERA_SOUND_STATE_VIDEO_END_WAIT;
        break;
    case CAMERA_SOUND_STATE_VIDEO_END_WAIT:
        if (DSP_IsShutterSoundPlaying() == FALSE) {
            CameraSystem_FreeSound(sys);
            sys->soundState = CAMERA_SOUND_STATE_ENDED;
        }
        break;
    case CAMERA_SOUND_STATE_SHUTTER:
        CameraSystem_LoadShutterSound(sys);
        DSP_PlayShutterSound(sys->soundData, sys->soundSize);
        sys->soundState = CAMERA_SOUND_STATE_SHUTTER_WAIT;
        break;
    case CAMERA_SOUND_STATE_SHUTTER_WAIT:
        if (DSP_IsShutterSoundPlaying() == FALSE) {
            CameraSystem_FreeSound(sys);
            sys->soundState = CAMERA_SOUND_STATE_RECORDING;
        }
        break;
    }
}

void CameraSystem_InitDsp(CameraSystem *sys) {
    FSFile file;

    DSP_OpenStaticComponentG711(&file);
    if (!DSP_LoadG711(&file, 0xff, 0xff)) {
        sys_exit();
    }
    GCTX_HIDSetSoftResetCallback(CameraSystem_OnSoftReset, sys);
}

void CameraSystem_ExitDsp(CameraSystem *sys) {
    GCTX_HIDSetSoftResetCallback(NULL, NULL);
    DSP_StopSound();
    DSP_UnloadG711();
}

static void CameraSystem_LoadSound(CameraSystem *sys, BOOL start) {
    u16 file = start == TRUE ? CAMERA_SOUND_VIDEO_START : CAMERA_SOUND_VIDEO_END;

    CameraSystem_FreeSound(sys);
    sys->soundSize = GFL_ArcSysGetDataLength(ARC_CAMERA_SOUND, file);
    sys->soundData = allocConfigDSSoftwareFeature(sys->heapId, sys->soundSize, "camera_system.c", 359);
    GFL_ArcSysRead(sys->soundData, ARC_CAMERA_SOUND, file);
    cp15_flushDC(sys->soundData, sys->soundSize);
}

static void CameraSystem_FreeSound(CameraSystem *sys) {
    if (sys->soundData != NULL) {
        func_02042ed0(sys->soundData);
        sys->soundData = NULL;
    }
}

static void CameraSystem_LoadShutterSound(CameraSystem *sys) {
    CameraSystem_FreeSound(sys);
    sys->soundSize = GFL_ArcSysGetDataLength(ARC_CAMERA_SOUND, CAMERA_SOUND_SHUTTER);
    sys->soundData = allocConfigDSSoftwareFeature(sys->heapId, sys->soundSize, "camera_system.c", 381);
    GFL_ArcSysRead(sys->soundData, ARC_CAMERA_SOUND, CAMERA_SOUND_SHUTTER);
    cp15_flushDC(sys->soundData, sys->soundSize);
}

static void CameraSystem_OnBufferError(CAMERAResult result) {
    CameraSystem *sys = sCameraSystem;

    CAMERA_StopCapture();
    func_02768270(CAMERA_NDMA_NO);
    CAMERA_ClearBuffer();
    sys->skipFrame = TRUE;
    sys->startCapture = TRUE;
}

static void CameraSystem_OnReboot(CAMERAResult result) {
    if (result != CAMERA_RESULT_FATAL_ERROR) {
        CameraSystem_OnBufferError(result);
    }
}

static void CameraSystem_OnVsync(CAMERAResult result) {
    CameraSystem *sys = sCameraSystem;

    if (sys->capturing) {
        if (sys->frameCount <= CAMERA_FRAME_COUNT_MAX) {
            sys->frameCount++;
        }
        if (sys->switchCamera) {
            if (CAMERA_I2CActivate(sys->camera) == CAMERA_RESULT_FATAL_ERROR) {
                sys_exit();
            }
            sys->frameCount = 0;
            sys->switchCamera = FALSE;
        }
        if (sys->startCapture) {
            CameraSystem_StartDma(sys);
            CAMERA_ClearBuffer();
            CAMERA_StartCapture();
            sys->startCapture = FALSE;
        }
    }
}

static void CameraSystem_OnDmaDone(void *arg) {
    CameraSystem *sys = sCameraSystem;

    func_02768270(CAMERA_NDMA_NO);
    if (CAMERA_IsBusy() == TRUE) {
        if (func_02768234(CAMERA_NDMA_NO)) {
            func_02768270(CAMERA_NDMA_NO);
        }
        if (sys->skipFrame) {
            sys->skipFrame = FALSE;
        } else if (sys->frameCount > CAMERA_SETTLE_FRAMES) {
            if (sys->callback != NULL) {
                sys->callback(sys->buffers[sys->bufferIndex], sys->callbackWork);
            }
            sys->bufferIndex = (sys->bufferIndex + 1) % sys->numBuffers;
        }
        if (sys->capturing == TRUE) {
            CameraSystem_StartDma(sys);
        }
    }
}

void CameraSystem_SetFrameCallback(CameraSystem *sys, CameraFrameCallback callback, void *work) {
    sys->callback = callback;
    sys->callbackWork = work;
}

void CameraSystem_AllocBuffers(CameraSystem *sys, int count, HeapID heapId) {
    u16 width = CameraSystem_GetWidth(sys);
    u16 height = CameraSystem_GetHeight(sys);
    u8 i;

    if (sys->buffers == NULL) {
        sys->buffers = GFL_HeapAllocate(sys->heapId, count * sizeof(void *), FALSE, "camera_system.c", 529);
        sys->numBuffers = count;
        for (i = 0; i < count; i++) {
            sys->buffers[i] =
                allocConfigDSSoftwareFeature(heapId, CAMERA_GET_FRAME_BYTES(width, height), "camera_system.c", 534);
        }
        sys->buffersAllocated = TRUE;
    }
}

static void CameraSystem_FreeBuffers(CameraSystem *sys) {
    u8 i;

    if (sys->capturing == FALSE && CAMERA_IsBusy() == FALSE) {
        if (sys->buffersAllocated == TRUE) {
            for (i = 0; i < sys->numBuffers; i++) {
                func_02042ed0(sys->buffers[i]);
            }
        }
        GFL_HeapFree(sys->buffers);
        sys->numBuffers = 0;
        sys->buffers = NULL;
    }
}

void CameraSystem_Start(CameraSystem *sys) {
    if (sys->capturing == FALSE && CAMERA_IsBusy() == FALSE) {
        if (func_02768234(CAMERA_NDMA_NO) == TRUE) {
            func_02768270(CAMERA_NDMA_NO);
        }
        sys->capturing = TRUE;
        sys->startCapture = TRUE;
        sys->frameCount = 0;
    }
}

void CameraSystem_Stop(CameraSystem *sys) {
    if (sys->capturing == TRUE) {
        sys->capturing = FALSE;
        sys->startCapture = FALSE;
        CAMERA_StopCapture();
    }
}

BOOL CameraSystem_IsSoundDone(CameraSystem *sys) {
    if (sys->soundState == CAMERA_SOUND_STATE_ENDED || sys->soundState == CAMERA_SOUND_STATE_NONE) {
        return TRUE;
    }
    return FALSE;
}

static void CameraSystem_SetupCapture(CameraSystem *sys) {
    u16 width, height;
    int lines;

    if (sys->capturing == FALSE && CAMERA_IsBusy() == FALSE) {
        CAMERA_SetTrimming(FALSE);
        sys->trimLeft = 0;
        sys->trimRight = CameraSystem_SizeToWidth(sys);
        sys->trimTop = 0;
        sys->trimBottom = CameraSystem_SizeToHeight(sys);
        width = CameraSystem_GetWidth(sys);
        height = CameraSystem_GetHeight(sys);
        lines = CAMERA_GetMaxLinesRound(width, height);
        CAMERA_SetTransferLines(lines);
        sys->unk14 = FALSE;
    }
}

static u16 CameraSystem_GetWidth(CameraSystem *sys) {
    return sys->trimRight - sys->trimLeft;
}

static u16 CameraSystem_GetHeight(CameraSystem *sys) {
    return sys->trimBottom - sys->trimTop;
}

void CameraSystem_SetSize(CameraSystem *sys, CAMERASize size) {
    CAMERA_I2CSizeEx(CAMERA_SELECT_BOTH, CAMERA_CONTEXT_BOTH, size);
    sys->size = size;
    CameraSystem_SetupCapture(sys);
}

static u16 CameraSystem_SizeToWidth(CameraSystem *sys) {
    return CAMERA_SizeToWidth(sys->size);
}

static u16 CameraSystem_SizeToHeight(CameraSystem *sys) {
    return CAMERA_SizeToHeight(sys->size);
}

void CameraSystem_SwitchCamera(CameraSystem *sys, CAMERASelect camera) {
    if (sys->camera != camera && sys->switchCamera == FALSE) {
        sys->camera = camera;
        sys->switchCamera = TRUE;
    }
}

void CameraSystem_PlayVideoStartSound(CameraSystem *sys) {
    sys->soundState = CAMERA_SOUND_STATE_VIDEO_START;
}

void CameraSystem_PlayVideoEndSound(CameraSystem *sys) {
    sys->soundState = CAMERA_SOUND_STATE_VIDEO_END;
}

void CameraSystem_PlayShutterSound(CameraSystem *sys) {
    sys->soundState = CAMERA_SOUND_STATE_SHUTTER;
}

BOOL CameraSystem_IsShutterSoundPlaying(CameraSystem *sys) {
    if (sys->soundState == CAMERA_SOUND_STATE_SHUTTER || sys->soundState == CAMERA_SOUND_STATE_SHUTTER_WAIT) {
        return TRUE;
    }
    return FALSE;
}
