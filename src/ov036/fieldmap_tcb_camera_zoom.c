// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field.h"
#include "field/field_camera.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "math.h"
#include "nitro/fx.h"

typedef struct {
    Field *field;
    u16 frame;
    u16 frameCount;
    fx32 startZoom;
    fx32 endZoom;
} FieldCameraZoomTCB;

static void func_ov036_021c06bc(TCB *tcb, void *data);
static void FieldCameraZoomTCB_Callback(TCB *tcb, void *data);
static void func_ov036_021c070c(FieldCameraZoomTCB *zoom);
static void FieldCameraZoomTCB_Calc(FieldCameraZoomTCB *zoom);

void func_ov036_021c05d4(Field *field, u32 frameCount, fx32 zoomDelta) {
    HeapID heapId = Field_GetHeapID(field);
    FieldCamera *camera = Field_GetCameraSystem(field);
    TCBManager *tcbMgr = Field_GetTCBMgr(field);
    FieldCameraZoomTCB *zoom =
        GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(FieldCameraZoomTCB), FALSE, "fieldmap_tcb_camera_zoom.c", 68);

    zoom->frame = 0;
    zoom->frameCount = frameCount;
    zoom->startZoom = FieldCamera_CoordsGetZoom(camera);
    zoom->endZoom = zoomDelta + FieldCamera_CoordsGetZoom(camera);
    zoom->field = field;
    GFL_TCBMgrAddTask(tcbMgr, func_ov036_021c06bc, zoom, 0);
}

void FieldCameraZoomTCB_Create(Field *field, u32 frameCount, fx32 zoomDelta) {
    HeapID heapId = Field_GetHeapID(field);
    FieldCamera *camera = Field_GetCameraSystem(field);
    TCBManager *tcbMgr = Field_GetTCBMgr(field);
    FieldCameraZoomTCB *zoom =
        GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(FieldCameraZoomTCB), FALSE, "fieldmap_tcb_camera_zoom.c", 95);

    zoom->frame = 0;
    zoom->frameCount = frameCount;
    zoom->startZoom = FieldCamera_CoordsGetZoom(camera);
    zoom->endZoom = zoomDelta + FieldCamera_CoordsGetZoom(camera);
    zoom->field = field;
    GFL_TCBMgrAddTask(tcbMgr, FieldCameraZoomTCB_Callback, zoom, 0);
}

static void func_ov036_021c06bc(TCB *tcb, void *data) {
    FieldCameraZoomTCB *zoom = data;

    func_ov036_021c070c(zoom);
    zoom->frame++;
    if (zoom->frameCount < zoom->frame) {
        GFL_HeapFree(zoom);
        GFL_TCBRemove(tcb);
    }
}

static void FieldCameraZoomTCB_Callback(TCB *tcb, void *data) {
    FieldCameraZoomTCB *zoom = data;

    FieldCameraZoomTCB_Calc(zoom);
    zoom->frame++;
    if (zoom->frameCount < zoom->frame) {
        GFL_HeapFree(zoom);
        GFL_TCBRemove(tcb);
    }
}

static void func_ov036_021c070c(FieldCameraZoomTCB *zoom) {
    FieldCamera *camera = Field_GetCameraSystem(zoom->field);
    u16 frameCount = zoom->frameCount;

    if (zoom->frame <= frameCount) {
        f32 t = (f32)zoom->frame / (f32)frameCount;
        f32 start = zoom->startZoom / 4096.0f;
        f32 end = zoom->endZoom / 4096.0f;
        f32 value = (1.0f - t) * start + t * end;

        FieldCamera_CoordsSetZoom(camera, FX32_CONST(value));
    }
}

static void FieldCameraZoomTCB_Calc(FieldCameraZoomTCB *zoom) {
    FieldCamera *camera = Field_GetCameraSystem(zoom->field);
    u16 frameCount = zoom->frameCount;

    if (zoom->frame <= frameCount) {
        f32 t = sqrt((f32)zoom->frame / (f32)frameCount);
        f32 start = zoom->startZoom / 4096.0f;
        f32 end = zoom->endZoom / 4096.0f;
        f32 value = (1.0f - t) * start + t * end;

        FieldCamera_CoordsSetZoom(camera, FX32_CONST(value));
    }
}
