#include "types.h"
#include "app/musical/sta_act_bg.h"
#include "constants/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

// Overlay 209's sta_act_bg.c: the stage's background, a 3D model with three animations, from ARCID_MUSICAL's
// files 30 (the models), 36, 41 and 46 (their animations) on

#define STA_ACT_BG_FILE_MODEL 30
#define STA_ACT_BG_FILE_ANIME1 36
#define STA_ACT_BG_FILE_ANIME2 46
#define STA_ACT_BG_FILE_ANIME3 41

static void StaActBg_UnloadBg(StaActBg *bg);

static SRTMatrix sStaActBgMatrix = {
    { FX32_CONST(16), FX32_CONST(3), FX32_CONST(-6) },
    { FX32_CONST(0.2), FX32_CONST(0.2), FX32_CONST(0.2) },
};

StaActBg *StaActBg_InitSystem(HeapID heapId, StaActing *stage) {
    StaActBg *bg = GFL_HeapAllocate(heapId, sizeof(StaActBg), FALSE, "sta_act_bg.c", 81);

    bg->heapId = heapId;
    bg->stage = stage;
    bg->scroll = 0;
    bg->mdlRes = NULL;
    return bg;
}

void StaActBg_TermSystem(StaActBg *bg) {
    if (bg->mdlRes != NULL) {
        StaActBg_UnloadBg(bg);
    }
    GFL_HeapFree(bg);
}

void StaActBg_UpdateSystem(StaActBg *bg) {
    u8 i;

    if (bg->mdlRes != NULL) {
        for (i = 0; i < 3; i++) {
            GFL_G3DActorStepAnmFrameLoop(bg->actor, i, FX32_ONE);
        }
    }
}

void StaActBg_DrawSystem(StaActBg *bg) {
    if (bg->mdlRes != NULL) {
        MTX_Identity33_(&sStaActBgMatrix.rotation);
        GFL_G3DSysDrawObj(bg->actor, &sStaActBgMatrix);
    }
}

void StaActBg_LoadBg(StaActBg *bg, u8 bgNo) {
    u8 i;

    if (bg->mdlRes != NULL) {
        StaActBg_UnloadBg(bg);
    }
    bg->mdlRes = GFL_G3DSysReadArcSysResource(ARCID_MUSICAL, bgNo + STA_ACT_BG_FILE_MODEL);
    bg->anmRes[0] = GFL_G3DSysReadArcSysResource(ARCID_MUSICAL, bgNo + STA_ACT_BG_FILE_ANIME1);
    bg->anmRes[1] = GFL_G3DSysReadArcSysResource(ARCID_MUSICAL, bgNo + STA_ACT_BG_FILE_ANIME2);
    bg->anmRes[2] = GFL_G3DSysReadArcSysResource(ARCID_MUSICAL, bgNo + STA_ACT_BG_FILE_ANIME3);
    if (GFL_G3DResUploadTexData(bg->mdlRes)) {
        bg->mdl = GFL_G3DMdlCreate(bg->mdlRes, 0, bg->mdlRes);
        for (i = 0; i < 3; i++) {
            bg->anms[i] = GFL_G3DAnmCreate(bg->mdl, bg->anmRes[i], 0);
        }
        bg->actor = GFL_G3DActorCreate(bg->mdl, bg->anms, 3);
        for (i = 0; i < 3; i++) {
            GFL_G3DActorBindAnm(bg->actor, i);
        }
    }
}

static void StaActBg_UnloadBg(StaActBg *bg) {
    u8 i;

    GFL_G3DActorFree(bg->actor);
    for (i = 0; i < 3; i++) {
        GFL_G3DAnmFree(bg->anms[i]);
    }
    GFL_G3DMdlFree(bg->mdl);
    for (i = 0; i < 3; i++) {
        GFL_G3DResFree(bg->anmRes[i]);
    }
    GFL_G3DResFreeTexData(bg->mdlRes);
    GFL_G3DResFree(bg->mdlRes);
    bg->mdlRes = NULL;
}

void StaActBg_SetScrollOffset(StaActBg *bg, u16 scroll) {
    bg->scroll = scroll;
}
