#include "app/comm_tvt/ctvt_camera.h"
#include "types.h"
#include "app/comm_tvt/camera_system.h"
#include "app/comm_tvt/comm_tvt_sys.h"
#include "app/comm_tvt/ctvt_comm.h"
#include "constants/sound.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "system/dsi.h"
#include "twl/camera.h"

// The Xtransceiver's video. Each member of the call has a window on the top screen, BG 3 of the main engine, a
// direct-color bitmap. The own camera's frames are cropped into a buffer and sent to the others, and each member's
// picture is copied into its window once it changes. Without a camera, a picture from the archive stands in. The
// windows slide into place when the layout changes, and the screen opens from the top when the video starts

#define CTVT_CAMERA_HEAP_SIZE 0x80000
// The bitmap of the top screen, 256 x 192 direct colors
#define CTVT_SCREEN_SIZE 0x18000
// A copy of the middle of a camera frame, 128 x 192 direct colors
#define CTVT_FRAME_COPY_SIZE 0xc000

#define CTVT_CAMERA_MEMBERS 4

// The opening of the screen grows by this many lines each frame, up to the screen's height
#define CTVT_OPEN_SPEED 8
#define CTVT_OPEN_END 192

typedef struct {
    u16 x;
    u16 y;
    u16 width;
    u16 height;
} CtvtRect;

typedef struct {
    BOOL moving;
    BOOL slide;
    BOOL playSE;
    CtvtRect cur;
    CtvtRect target;
} CtvtCameraWindow;

struct CtvtCamera {
    HeapID heapId;
    u8 *screen;
    // The pictures for a member without a camera, for the normal and the zoomed screen
    void *noCameraPicture;
    void *noCameraPictureZoomed;
    // The windows to draw again
    u8 dirtyMask;
    BOOL redrawAll;
    BOOL startOpening;
    BOOL updateDisplay;
    BOOL zoomed;
    BOOL recording;
    BOOL recordingEnded;
    u8 redrawFrames;
    u8 openLines;
    BOOL opening;
    CtvtCameraWindow windows[CTVT_CAMERA_MEMBERS];
    CameraSystem *camera;
    void *frameBuffers[2];
    u16 *frameCopy;
};

static void CtvtCamera_OnFrame(void *frame, void *data);
static void CtvtCamera_SetWindowRect(CtvtCameraWindow *window, u16 x, u16 y, u16 width, u16 height);
static void CtvtCamera_SetWindowTarget(CtvtCameraWindow *window, u16 x, u16 y, u16 width, u16 height);
static BOOL CtvtCamera_MoveWindow(CtvtCamera *work, CtvtCameraWindow *window);
static BOOL CtvtCamera_MoveCoord(CtvtCamera *work, u16 *cur, u16 *target);

const u16 data_ov257_021b18c4[4] = { 0, 0x80, 0x40, 0xc0 };

static const u32 sPictureWidths[] = {
    [CTDM_SINGLE] = 256,
    [CTDM_DOUBLE] = 128,
    [CTDM_QUAD] = 128,
};

static const u32 sPictureHeights[] = {
    [CTDM_SINGLE] = 192,
    [CTDM_DOUBLE] = 192,
    [CTDM_QUAD] = 96,
};

static const u32 sPictureSizes[] = {
    [CTDM_SINGLE] = 256 * 192 * 2,
    [CTDM_DOUBLE] = 128 * 192 * 2,
    [CTDM_QUAD] = 128 * 96 * 2,
};

CtvtCamera *CtvtCamera_Create(CommTvtWork *sys, HeapID heapId) {
    u8 i;
    CtvtCamera *work = GFL_HeapAllocate(heapId, sizeof(CtvtCamera), TRUE, "ctvt_camera.c", 103);
    ArcTool *arc = CommTvt_GetArc(sys);

    if (CommTvt_IsCameraEnabled() == TRUE) {
        work->heapId = HEAPID_CTVT_CAMERA;
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_CTVT_CAMERA, CTVT_CAMERA_HEAP_SIZE);
        work->camera = CameraSystem_Create(work->heapId);
        CameraSystem_SetSize(work->camera, CAMERA_SIZE_DS_LCD);
        CameraSystem_SwitchCamera(work->camera, CAMERA_SELECT_IN);
        CameraSystem_AllocBuffers(work->camera, 2, HEAPID_TAIL(work->heapId));
        CameraSystem_SetFrameCallback(work->camera, CtvtCamera_OnFrame, sys);
        CameraSystem_Start(work->camera);
        work->frameBuffers[0] =
            GFL_HeapAllocate(HEAPID_TAIL(work->heapId), CTVT_FRAME_COPY_SIZE, TRUE, "ctvt_camera.c", 118);
        work->frameBuffers[1] =
            GFL_HeapAllocate(HEAPID_TAIL(work->heapId), CTVT_FRAME_COPY_SIZE, TRUE, "ctvt_camera.c", 119);
        work->frameCopy = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), CTVT_FRAME_COPY_SIZE, TRUE, "ctvt_camera.c", 120);
        work->noCameraPicture = GFL_ArcToolReadHeapNewLZ(arc, 28, FALSE, work->heapId);
        work->noCameraPictureZoomed = GFL_ArcToolReadHeapNewLZ(arc, 29, FALSE, work->heapId);
    } else {
        work->noCameraPicture = GFL_ArcToolReadHeapNewLZ(arc, 28, FALSE, heapId);
        work->noCameraPictureZoomed = GFL_ArcToolReadHeapNewLZ(arc, 29, FALSE, heapId);
    }

    work->dirtyMask = 0;
    work->redrawFrames = 0;
    work->redrawAll = TRUE;
    work->openLines = CTVT_OPEN_END;
    work->startOpening = FALSE;
    work->updateDisplay = FALSE;
    work->zoomed = FALSE;
    work->opening = FALSE;
    work->recording = FALSE;
    work->recordingEnded = FALSE;
    work->screen = GFL_HeapAllocate(HEAPID_TAIL(heapId), CTVT_SCREEN_SIZE, TRUE, "ctvt_camera.c", 140);
    for (i = 0; i < CTVT_CAMERA_MEMBERS; i++) {
        CtvtCamera_SetWindowRect(&work->windows[i], 0, 0, 0, 0);
        CtvtCamera_SetWindowTarget(&work->windows[i], 0, 0, 0, 0);
        work->windows[i].moving = FALSE;
        work->windows[i].slide = TRUE;
        work->windows[i].playSE = TRUE;
    }
    return work;
}

void CtvtCamera_Delete(CommTvtWork *sys, CtvtCamera *work) {
    MtxFx22 mtx;

    mtx._00 = FX_Inv(FX32_ONE);
    mtx._11 = FX_Inv(FX32_ONE);
    mtx._01 = 0;
    mtx._10 = 0;
    G2_SetBG3Affine(&mtx, 0, 0, 0, 0);
    GFL_HeapFree(work->screen);
    GFL_HeapFree(work->noCameraPictureZoomed);
    GFL_HeapFree(work->noCameraPicture);
    if (CommTvt_IsCameraEnabled() == TRUE) {
        GFL_HeapFree(work->frameCopy);
        GFL_HeapFree(work->frameBuffers[0]);
        GFL_HeapFree(work->frameBuffers[1]);
        CameraSystem_Delete(work->camera);
        GFL_HeapDelete(HEAPID_CTVT_CAMERA);
    }
    GFL_HeapFree(work);
}

void CtvtCamera_Update(CommTvtWork *sys, CtvtCamera *work) {
    u8 i;

    if (work->redrawAll == FALSE) {
        for (i = 0; i < CTVT_CAMERA_MEMBERS; i++) {
            if (CtvtCamera_MoveWindow(work, &work->windows[i]) == TRUE) {
                work->dirtyMask |= 1 << i;
                break;
            }
        }
    }
    if (CommTvt_IsCameraEnabled() == TRUE) {
        CameraSystem_UpdateSound(work->camera);
    }
    if (work->openLines < CTVT_OPEN_END && work->opening == FALSE) {
        work->openLines += CTVT_OPEN_SPEED;
        G2_SetWnd0Position(0, 0, 255, work->openLines);
        G2_SetWnd1Position(128, 0, 256, work->openLines);
    }
}

void CtvtCamera_Draw(CommTvtWork *sys, CtvtCamera *work) {
    MtxFx22 mtx;
    int displayMode = CommTvt_GetDisplayMode(sys);
    BOOL zoomed = CommTvt_IsZoomed(sys);
    CtvtComm *comm = CommTvt_GetComm(sys);
    u8 i;
    u8 count;
    int pos;
    int width;
    u32 size;
    int row;
    uptr src;
    u16 height;

    CommTvt_GetMode(sys);
    if (CommTvt_GetMemberCount(sys) < 2) {
        return;
    }

    if (displayMode == CTDM_SINGLE) {
        if (work->dirtyMask != 0) {
            DC_FlushRange(work->screen, CTVT_SCREEN_SIZE);
            GX_LoadBG3Scr(work->screen, 0, CTVT_SCREEN_SIZE);
            work->dirtyMask = 0;
        }
    } else {
        if (work->redrawAll == TRUE) {
            count = 0;
            for (i = 0; i < CTVT_CAMERA_MEMBERS; i++) {
                if (work->dirtyMask & (1 << i)) {
                    count++;
                }
            }
            if (count != CommTvt_GetMemberCount(sys)) {
                return;
            }
            if (work->redrawFrames < 2) {
                work->dirtyMask = 0;
                work->redrawFrames++;
                for (i = 0; i < CTVT_CAMERA_MEMBERS; i++) {
                    if (CtvtComm_IsMemberInfoReceived(sys, comm, i) == TRUE &&
                        CtvtComm_HasMemberCamera(sys, comm, i) == FALSE) {
                        work->dirtyMask |= 1 << i;
                    }
                }
                return;
            }
            if (work->openLines < CTVT_OPEN_END) {
                return;
            }
            if (work->startOpening == TRUE) {
                work->openLines = 0;
                work->opening = TRUE;
                work->startOpening = FALSE;
                G2_SetWnd0Position(0, 0, 255, 0);
                G2_SetWnd1Position(128, 0, 256, 0);
            }
            if (work->updateDisplay == TRUE) {
                work->updateDisplay = FALSE;
                func_ov257_021aad48(sys);
            }
            MIi_CpuClear32(0, G2_GetBG3ScrPtr(), CTVT_SCREEN_SIZE);
            work->redrawAll = FALSE;
            if (work->zoomed != CommTvt_IsZoomed(sys)) {
                work->zoomed = CommTvt_IsZoomed(sys);
                if (work->zoomed == TRUE) {
                    mtx._00 = FX_Inv(FX32_ONE * 2);
                    mtx._11 = FX_Inv(FX32_ONE * 2);
                } else {
                    mtx._00 = FX_Inv(FX32_ONE);
                    mtx._11 = FX_Inv(FX32_ONE);
                }
                mtx._01 = 0;
                mtx._10 = 0;
                G2_SetBG3Affine(&mtx, 0, 0, 0, 0);
                for (i = 0; i < CTVT_CAMERA_MEMBERS; i++) {
                    if (work->zoomed == TRUE) {
                        work->windows[i].target.x /= 2;
                        work->windows[i].target.y /= 2;
                        work->windows[i].target.width /= 2;
                        work->windows[i].target.height /= 2;
                        work->windows[i].cur.x /= 2;
                        work->windows[i].cur.y /= 2;
                        work->windows[i].cur.width /= 2;
                        work->windows[i].cur.height /= 2;
                    } else {
                        work->windows[i].target.x *= 2;
                        work->windows[i].target.y *= 2;
                        work->windows[i].target.width *= 2;
                        work->windows[i].target.height *= 2;
                        work->windows[i].cur.x *= 2;
                        work->windows[i].cur.y *= 2;
                        work->windows[i].cur.width *= 2;
                        work->windows[i].cur.height *= 2;
                    }
                }
            }
            work->opening = FALSE;
        }

        for (i = 0; i < CTVT_CAMERA_MEMBERS; i++) {
            if (!(work->dirtyMask & (1 << i))) {
                continue;
            }
            if (CtvtComm_IsMemberInfoReceived(sys, comm, i) == TRUE) {
                pos = i;
                if (displayMode == CTDM_DOUBLE && i != 0) {
                    pos = 1;
                }
                if (zoomed == FALSE) {
                    width = 128;
                    height = displayMode == CTDM_DOUBLE ? 192 : 96;
                } else {
                    width = 64;
                    height = displayMode == CTDM_DOUBLE ? 96 : 48;
                }
                if (CtvtComm_HasMemberCamera(sys, comm, i) == TRUE && !canPlayerExchangePhotos()) {
                    size = width * height * 2;
                    src = (uptr)work->screen + pos * size;
                    DC_FlushRange((void *)src, size);
                } else if (zoomed == FALSE) {
                    if (displayMode == CTDM_DOUBLE) {
                        src = (uptr)work->noCameraPicture;
                        DC_FlushRange((void *)src, 0xc000);
                    } else {
                        src = (uptr)work->noCameraPicture + 0x3000;
                        DC_FlushRange((void *)src, 0x6000);
                    }
                } else {
                    if (displayMode == CTDM_DOUBLE) {
                        src = (uptr)work->noCameraPictureZoomed;
                        DC_FlushRange((void *)src, 0x3000);
                    } else {
                        src = (uptr)work->noCameraPictureZoomed + 0xc00;
                        DC_FlushRange((void *)src, 0x1800);
                    }
                }
                for (row = 0; row < work->windows[i].cur.height; row++) {
                    int y = height - work->windows[i].cur.height + row;

                    GX_LoadBG3Scr((void *)(src + y * width * 2 + (width - work->windows[i].cur.width) * 2),
                                  work->windows[i].cur.x * 2 + (work->windows[i].cur.y + row) * 512,
                                  work->windows[i].cur.width * 2);
                }
            }
            work->dirtyMask -= 1 << i;
        }
    }
}

void CtvtCamera_StopCamera(CommTvtWork *sys, CtvtCamera *work) {
    if (CommTvt_IsCameraEnabled() == TRUE) {
        CameraSystem_Stop(work->camera);
    }
}

BOOL CtvtCamera_IsSoundDone(CommTvtWork *sys, CtvtCamera *work) {
    if (CommTvt_IsCameraEnabled() == TRUE) {
        return CameraSystem_IsSoundDone(work->camera);
    }
    return TRUE;
}

static void CtvtCamera_OnFrame(void *frame, void *data) {
    CommTvtWork *sys = data;
    CtvtCamera *work = CommTvt_GetCamera(sys);
    int displayMode = CommTvt_GetDisplayMode(sys);
    BOOL zoomed = CommTvt_IsZoomed(sys);
    int left;
    u8 self = CommTvt_GetSelfIndex(sys);
    int y;
    u8 pos;
    u16 top;
    int width;
    u16 height;

    if (func_ov257_021aab10(sys) == TRUE || CommTvt_GetMemberCount(sys) < 2) {
        return;
    }
    if (work->dirtyMask & (1 << self)) {
        return;
    }

    if (func_ov257_021aab18(sys) == FALSE) {
        for (y = 0; y < 192; y++) {
            MIi_CpuCopy32((u16 *)frame + y * 256 + 64, work->frameCopy + y * 128, 256);
        }
    }
    if (displayMode == CTDM_SINGLE) {
        GFL_ASSERT_MSG(FALSE, "mode CTDM_SINGLE is no support!\n");
        return;
    }
    pos = self;
    if (displayMode == CTDM_DOUBLE && self > 1) {
        pos = 1;
    }
    if (zoomed == FALSE) {
        left = 0;
        top = displayMode == CTDM_DOUBLE ? 0 : 48;
        width = 128;
        height = displayMode == CTDM_DOUBLE ? 192 : 96;
    } else {
        left = 32;
        top = displayMode == CTDM_DOUBLE ? 48 : 80;
        width = 64;
        height = displayMode == CTDM_DOUBLE ? 96 : 48;
    }

    if (func_ov257_021aab5c(sys) == FALSE) {
        func_ov257_021aab60(sys, TRUE);
        for (y = 0; y < height; y++) {
            MIi_CpuCopy32((void *)((uptr)work->frameCopy + (top + y) * 256 + left * 2),
                          (void *)((uptr)work->screen + pos * (width * height * 2) + width * y * 2), width * 2);
        }
        func_ov257_021aab60(sys, FALSE);
        work->dirtyMask |= 1 << self;
    }
}

void CtvtCamera_SetDirty(CommTvtWork *sys, CtvtCamera *work, u8 member) {
    work->dirtyMask |= 1 << member;
}

void CtvtCamera_ClearDirty(CommTvtWork *sys, CtvtCamera *work, u8 member) {
    if (work->redrawAll == FALSE) {
        u32 bit = 1 << member;

        if (work->dirtyMask & bit) {
            work->dirtyMask -= bit;
        }
    }
}

void CtvtCamera_Redraw(CommTvtWork *sys, CtvtCamera *work, BOOL startOpening, BOOL updateDisplay) {
    CtvtComm *comm = CommTvt_GetComm(sys);
    u8 i;

    work->redrawAll = TRUE;
    if (work->startOpening == FALSE) {
        work->startOpening = startOpening;
    }
    if (work->updateDisplay == FALSE) {
        work->updateDisplay = updateDisplay;
    }
    work->dirtyMask = 0;
    work->redrawFrames = 0;
    if (CommTvt_IsCameraEnabled() == FALSE) {
        work->dirtyMask |= 1 << CommTvt_GetSelfIndex(sys);
    }
    for (i = 0; i < CTVT_CAMERA_MEMBERS; i++) {
        if (CtvtComm_IsMemberActive(sys, comm, i) == TRUE) {
            CtvtCamera_SetWindow(sys, work, i);
        }
    }
}

BOOL CtvtCamera_IsRedrawing(CommTvtWork *sys, CtvtCamera *work) {
    return work->redrawAll;
}

u16 CtvtCamera_GetWidth(CommTvtWork *sys, CtvtCamera *work) {
    int displayMode = CommTvt_GetDisplayMode(sys);

    if (CommTvt_IsZoomed(sys) == FALSE) {
        return sPictureWidths[displayMode];
    }
    return sPictureWidths[displayMode] / 2;
}

u16 CtvtCamera_GetHeight(CommTvtWork *sys, CtvtCamera *work) {
    int displayMode = CommTvt_GetDisplayMode(sys);

    if (CommTvt_IsZoomed(sys) == FALSE) {
        return sPictureHeights[displayMode];
    }
    return sPictureHeights[displayMode] / 2;
}

void *CtvtCamera_GetOwnPicture(CommTvtWork *sys, CtvtCamera *work) {
    return CtvtCamera_GetPicture(sys, work, CommTvt_GetSelfIndex(sys));
}

void *CtvtCamera_GetPicture(CommTvtWork *sys, CtvtCamera *work, u8 member) {
    u32 size = CtvtCamera_GetPictureSize(sys, work);

    if (CommTvt_GetDisplayMode(sys) == CTDM_DOUBLE && member != 0) {
        return work->screen + size;
    }
    return work->screen + size * member;
}

u32 CtvtCamera_GetPictureSize(CommTvtWork *sys, CtvtCamera *work) {
    int displayMode = CommTvt_GetDisplayMode(sys);

    if (CommTvt_IsZoomed(sys) == FALSE) {
        return sPictureSizes[displayMode];
    }
    return sPictureSizes[displayMode] / 4;
}

static void CtvtCamera_SetWindowRect(CtvtCameraWindow *window, u16 x, u16 y, u16 width, u16 height) {
    window->moving = TRUE;
    window->cur.x = x;
    window->cur.y = y;
    window->cur.width = width;
    window->cur.height = height;
}

static void CtvtCamera_SetWindowTarget(CtvtCameraWindow *window, u16 x, u16 y, u16 width, u16 height) {
    window->moving = TRUE;
    window->target.x = x;
    window->target.y = y;
    window->target.width = width;
    window->target.height = height;
}

static BOOL CtvtCamera_MoveWindow(CtvtCamera *work, CtvtCameraWindow *window) {
    u32 moved;

    if (window->moving == TRUE) {
        moved = 0;
        moved |= CtvtCamera_MoveCoord(work, &window->cur.x, &window->target.x);
        moved |= CtvtCamera_MoveCoord(work, &window->cur.y, &window->target.y);
        moved |= CtvtCamera_MoveCoord(work, &window->cur.width, &window->target.width);
        moved |= CtvtCamera_MoveCoord(work, &window->cur.height, &window->target.height);
        if (!moved) {
            window->moving = FALSE;
        }
        if (window->playSE == TRUE) {
            window->playSE = FALSE;
            GFL_SndSEPlay(SEQ_SE_SYS_47);
        }
        window->slide = FALSE;
        return TRUE;
    }
    return FALSE;
}

static BOOL CtvtCamera_MoveCoord(CtvtCamera *work, u16 *cur, u16 *target) {
    u8 speed = work->zoomed == TRUE ? 4 : 8;

    if (*cur < *target) {
        if (*cur + speed < *target) {
            *cur += speed;
        } else {
            *cur = *target;
        }
        return TRUE;
    }
    if (*cur > *target) {
        if (*cur - speed > *target) {
            *cur -= speed;
        } else {
            *cur = *target;
        }
        return TRUE;
    }
    return FALSE;
}

void CtvtCamera_SetWindow(CommTvtWork *sys, CtvtCamera *work, u8 member) {
    int displayMode = CommTvt_GetDisplayMode(sys);
    int pos = member;

    if (displayMode == CTDM_DOUBLE && member != 0) {
        pos = 1;
    }
    if (work->zoomed == FALSE) {
        work->windows[member].target.x = (pos == 0 || pos == 2) ? 0 : 128;
        work->windows[member].target.y = (pos == 0 || pos == 1) ? 0 : 96;
        work->windows[member].target.width = 128;
        work->windows[member].target.height = displayMode == CTDM_DOUBLE ? 192 : 96;
    } else {
        work->windows[member].target.x = (pos == 0 || pos == 2) ? 0 : 64;
        work->windows[member].target.y = (pos == 0 || pos == 1) ? 0 : 48;
        work->windows[member].target.width = 64;
        work->windows[member].target.height = displayMode == CTDM_DOUBLE ? 96 : 48;
    }

    if (work->windows[member].slide == FALSE) {
        work->windows[member].cur.x = work->windows[member].target.x;
        work->windows[member].cur.y = work->windows[member].target.y;
        work->windows[member].cur.width = work->windows[member].target.width;
        work->windows[member].cur.height = work->windows[member].target.height;
    } else {
        // The window grows from the screen's corner or edge nearest to it
        work->windows[member].playSE = TRUE;
        switch (pos) {
        case 0:
            if (work->zoomed == FALSE) {
                CtvtCamera_SetWindowRect(&work->windows[member], 0, 0, 128, 0);
            } else {
                CtvtCamera_SetWindowRect(&work->windows[member], 0, 0, 64, 0);
            }
            break;
        case 1:
            if (work->zoomed == FALSE) {
                CtvtCamera_SetWindowRect(&work->windows[member], 128, 0, 0, displayMode == CTDM_DOUBLE ? 192 : 96);
            } else {
                CtvtCamera_SetWindowRect(&work->windows[member], 64, 0, 0, displayMode == CTDM_DOUBLE ? 96 : 48);
            }
            break;
        case 2:
            if (work->zoomed == FALSE) {
                CtvtCamera_SetWindowRect(&work->windows[member], 0, 96, 128, 0);
            } else {
                CtvtCamera_SetWindowRect(&work->windows[member], 0, 48, 64, 0);
            }
            break;
        case 3:
            if (work->zoomed == FALSE) {
                CtvtCamera_SetWindowRect(&work->windows[member], 128, 96, 0, 96);
            } else {
                CtvtCamera_SetWindowRect(&work->windows[member], 64, 48, 0, 48);
            }
            break;
        }
    }
    work->windows[member].moving = TRUE;
}

void CtvtCamera_MarkWindow(CommTvtWork *sys, CtvtCamera *work, u8 member) {
    work->windows[member].slide = TRUE;
    work->windows[member].playSE = TRUE;
}

BOOL CtvtCamera_IsWindowMoving(CommTvtWork *sys, CtvtCamera *work, u8 member) {
    return work->windows[member].moving;
}

void CtvtCamera_SetEffect(CommTvtWork *sys, CtvtCamera *work, CAMERAEffect effect) {
    if (CommTvt_IsCameraEnabled() == TRUE) {
        CAMERA_I2CEffectEx(CAMERA_SELECT_BOTH, CAMERA_CONTEXT_BOTH, effect);
    }
}

void CtvtCamera_StartRecording(CommTvtWork *sys, CtvtCamera *work) {
    if (CommTvt_IsCameraEnabled() == TRUE) {
        CameraSystem_PlayVideoStartSound(work->camera);
        work->recording = TRUE;
    }
}

void CtvtCamera_EndRecording(CommTvtWork *sys, CtvtCamera *work) {
    if (CommTvt_IsCameraEnabled() == TRUE && work->recording == TRUE && work->recordingEnded == FALSE) {
        CameraSystem_PlayVideoEndSound(work->camera);
        work->recordingEnded = TRUE;
    }
}

CameraSystem *CtvtCamera_GetCameraSystem(CommTvtWork *sys, CtvtCamera *work) {
    return work->camera;
}

void CtvtCamera_Restart(CommTvtWork *sys, CtvtCamera *work) {
    u8 i;

    if (CommTvt_IsCameraEnabled() == TRUE) {
        CameraSystem_SetFrameCallback(work->camera, CtvtCamera_OnFrame, sys);
        CameraSystem_Start(work->camera);
    }
    work->redrawFrames = 0;
    work->openLines = CTVT_OPEN_END;
    work->dirtyMask = 0;
    work->redrawAll = FALSE;
    work->startOpening = TRUE;
    work->updateDisplay = TRUE;
    work->zoomed = FALSE;
    work->opening = FALSE;
    for (i = 0; i < CTVT_CAMERA_MEMBERS; i++) {
        CtvtCamera_SetWindowRect(&work->windows[i], 0, 0, 0, 0);
        CtvtCamera_SetWindowTarget(&work->windows[i], 0, 0, 0, 0);
        work->windows[i].moving = FALSE;
        work->windows[i].slide = TRUE;
        work->windows[i].playSE = TRUE;
    }
}
