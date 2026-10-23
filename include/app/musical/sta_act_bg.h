#ifndef POKEBW2_APP_MUSICAL_STA_ACT_BG_H
#define POKEBW2_APP_MUSICAL_STA_ACT_BG_H

// Overlay 209's sta_act_bg.c: the stage's background, an animated 3D model of the musical's archive

#include "types.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "struct_decls.h"

struct StaActBg {
    HeapID heapId;
    // NULL in the photo
    StaActing *stage;
    // How far the stage has scrolled, in pixels
    u16 scroll;
    u8 unkA[0x12];
    // The model's resource, NULL until a background is loaded
    G3DResource *mdlRes;
    G3DResource *anmRes[3];
    G3DModel *mdl;
    void *anms[3];
    G3DActor *actor;
};

StaActBg *StaActBg_InitSystem(HeapID heapId, StaActing *stage);
void StaActBg_TermSystem(StaActBg *bg);
void StaActBg_UpdateSystem(StaActBg *bg);
void StaActBg_DrawSystem(StaActBg *bg);
void StaActBg_LoadBg(StaActBg *bg, u8 bgNo);
void StaActBg_SetScrollOffset(StaActBg *bg, u16 scroll);

#endif // POKEBW2_APP_MUSICAL_STA_ACT_BG_H
