#include "types.h"
#include "field/field_fog.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"

// The zone's fog data controller (field_zonefog.c, after the file name that its allocation passes): loads the
// zone's fog file a few frames after a request. Function names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0); the layout is ours

#define FOG_NONE 0xfffffff

struct FieldFogCtrl {
    u8 loadRequested;
    s8 loadDelay;
    u16 fileId;
    u32 *fogData;
    u32 staticLightIndex;
    ArcTool *arc;
};

FieldFogCtrl *FieldFogCtrl_Create(HeapID heapId) {
    FieldFogCtrl *ctrl = GFL_HeapAllocate(heapId, sizeof(FieldFogCtrl), TRUE, "field_zonefog.c", 81);

    ctrl->staticLightIndex = FOG_NONE;
    ctrl->arc = GFL_ArcSysCreateFileHandle(0x50, heapId);
    return ctrl;
}

void FieldFogCtrl_Free(FieldFogCtrl *ctrl) {
    GFL_ArcToolFree(ctrl->arc);
    FieldFogCtrl_Reset(ctrl);
    GFL_HeapFree(ctrl);
}

void FieldFogCtrl_Update(FieldFogCtrl *ctrl, HeapID heapId) {
    if (ctrl->loadRequested == TRUE) {
        ctrl->loadDelay--;
        if (ctrl->loadDelay <= 0) {
            ctrl->fogData = GFL_ArcToolReadHeapNewLZ(ctrl->arc, ctrl->fileId, FALSE, heapId);
            ctrl->loadRequested = FALSE;
        }
    }
}

void FieldFogCtrl_RequestLoad(FieldFogCtrl *ctrl, u32 fileId, u32 staticLightIndex, s8 delay) {
    if (fileId != FOG_NONE && fileId != ctrl->fileId) {
        ctrl->loadRequested = TRUE;
        ctrl->fileId = fileId;
        ctrl->loadDelay = delay;
    }
    ctrl->staticLightIndex = staticLightIndex;
}

u8 func_ov036_021977a4(FieldFogCtrl *ctrl) {
    return ctrl->loadRequested;
}

void FieldFogCtrl_Reset(FieldFogCtrl *ctrl) {
    if (ctrl->fogData != NULL) {
        GFL_HeapFree(ctrl->fogData);
        ctrl->fogData = NULL;
    }
    ctrl->staticLightIndex = FOG_NONE;
    if (ctrl->loadRequested == TRUE) {
        ctrl->loadRequested = FALSE;
    }
}

BOOL FieldFogCtrl_CheckArc(FieldFogCtrl *ctrl) {
    return ctrl->fogData != NULL;
}

BOOL FieldFogCtrl_IsStaticLightExists(FieldFogCtrl *ctrl) {
    return ctrl->staticLightIndex != FOG_NONE;
}

u32 FieldFogCtrl_GetFogParam0(FieldFogCtrl *ctrl) {
    if (ctrl->fogData != NULL) {
        return ctrl->fogData[0];
    }
    return FOG_NONE;
}

u32 FieldFogCtrl_GetFogParam1(FieldFogCtrl *ctrl) {
    if (ctrl->fogData != NULL) {
        return ctrl->fogData[1];
    }
    return FOG_NONE;
}

u32 FieldFogCtrl_GetStaticLightIndex(FieldFogCtrl *ctrl) {
    return ctrl->staticLightIndex;
}
