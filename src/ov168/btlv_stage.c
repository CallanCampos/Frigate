// The battle view's 3D stage: the two grounds the Pokémon stand on, the player's in front and the opponent's behind,
// their animations and the fade of their palettes. The name is the ROM's string, from GFL_HeapAllocate's asserts.
// BtlvStage_Create is swan's name; the rest are ours

#include "battle/btlv_stage.h"
#include "types.h"
#include "battle/btlv_effect.h"
#include "constants/arc.h"
#include "constants/battle.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/g3d.h"

// How a ground's animation runs
enum {
    STAGE_ANM_LOOP,     // every frame, at normal speed
    STAGE_ANM_ON_START, // when BtlvStage_StartAnimation starts it, for its frames
};

// A ground's animation, 16 bytes
typedef struct {
    s32 mode;    // 0x0  STAGE_ANM_*
    BOOL active; // 0x4
    fx32 speed;  // 0x8
    s32 frames;  // 0xc  left to run
} BtlvStageAnm;

// A stage of file 2 of archive 151, 0x42 bytes
typedef struct {
    GXRgb edgeColor; // G3X_SetEdgeColorTable reads the table from here
    // The resources of archive 11 by row (0 the models, 1-3 the animations), then by variant and ground, two to a
    // variant; 0xffff for none
    u16 resources[4][8];
} BtlvStageData;

// The stage's work, 0xc4 bytes
struct BtlvStage {
    G3DResource *models[2];        // 0x00  the grounds' model resources, with their textures
    u16 anmCount;                  // 0x08
    u16 anmResCount;               // 0x0a  how many of anmResources were allocated
    BtlvStageAnm *anms[2];         // 0x0c
    G3DResource **anmResources[2]; // 0x14  3 each
    G3DAnimation **anmObjs[2];     // 0x1c  3 each
    G3DModel *mdls[2];             // 0x24
    G3DActor *actors[2];           // 0x2c
    SRTMatrix srt[2];              // 0x34
    BtlvTexPaletteFade fade;       // 0xac
    u32 hidden : 2;                // 0xbc  a bit by ground
    HeapID heapId;                 // 0xc0
};

static u32 BtlvStage_CountAnimations(const BtlvStageData *data, u32 stageId, u32 variant);
static void BtlvStage_InitSRT(BtlvStage *stage, u32 side, fx32 scale);

// Where each ground stands
static const VecFx32 GROUND_POSITIONS[2] = {
    { 0, 0, 0x572f },
    { 0, 0, -0xcb7d },
};

static u32 BtlvStage_CountAnimations(const BtlvStageData *data, u32 stageId, u32 variant) {
    u32 count = 0;
    int i;

    for (i = 1; i < 4; i++) {
        if (data[stageId].resources[i][variant * 2] != 0xffff) {
            count++;
        }
    }
    return count;
}

static void BtlvStage_InitSRT(BtlvStage *stage, u32 side, fx32 scale) {
    stage->srt[side].translation.x = GROUND_POSITIONS[side].x;
    stage->srt[side].translation.y = GROUND_POSITIONS[side].y;
    stage->srt[side].translation.z = GROUND_POSITIONS[side].z;
    stage->srt[side].scale.x = scale;
    stage->srt[side].scale.y = scale;
    stage->srt[side].scale.z = scale;
    MTX_Identity33_(&stage->srt[side].rotation);
}

BtlvStage *BtlvStage_Create(u32 battleStyle, u32 stageId, u8 variant, HeapID heapId, u32 studioMode) {
    BtlvStage *stage;
    BtlvStageData *data;
    int anm;
    int i, j;
    u32 size;
    fx32 scale;

    stage = GFL_HeapAllocate(heapId, sizeof(BtlvStage), TRUE, "btlv_stage.c", 0x9c);
    data = GFL_ArcSysReadHeapNew(ARCID_BATTLE_BG_DATA, 2, HEAPID_TAIL(heapId));
    stage->heapId = heapId;
    // Pokéstar Studios' movies have their own stage
    if (studioMode == BTL_STUDIO_MODE_FILM || studioMode == BTL_STUDIO_MODE_PLAYBACK) {
        stageId = 27;
    }

    // A rotation battle has its own ground, the same for both sides
    if (battleStyle == BTL_STYLE_ROTATION) {
        stage->models[0] = GFL_G3DSysReadArcSysResource(ARCID_BTLV, 0x161);
        stage->models[1] = GFL_G3DSysReadArcSysResource(ARCID_BTLV, 0x161);
    } else if (data[stageId].resources[0][variant * 2] != 0xffff) {
        stage->models[0] = GFL_G3DSysReadArcSysResource(ARCID_BTLV, data[stageId].resources[0][variant * 2]);
        stage->models[1] = GFL_G3DSysReadArcSysResource(ARCID_BTLV, data[stageId].resources[0][variant * 2 + 1]);
    } else {
        stage->models[0] = GFL_G3DSysReadArcSysResource(ARCID_BTLV, data[stageId].resources[0][0]);
        stage->models[1] = GFL_G3DSysReadArcSysResource(ARCID_BTLV, data[stageId].resources[0][1]);
        variant = 0;
    }
    GFL_G3DResUploadTexData(stage->models[0]);
    stage->anmCount = BtlvStage_CountAnimations(data, stageId, variant);
    stage->mdls[0] = GFL_G3DMdlCreate(stage->models[0], 0, stage->models[0]);
    stage->mdls[1] = GFL_G3DMdlCreate(stage->models[1], 0, stage->models[1]);
    anm = 0;

    if (battleStyle == BTL_STYLE_ROTATION) {
        stage->anmCount = 1;
        stage->anmResCount = 1;
        stage->anmResources[0] = GFL_HeapAllocate(stage->heapId, 3 * sizeof(G3DResource *), TRUE, "btlv_stage.c", 0xd6);
        stage->anmResources[0][0] = GFL_G3DSysReadArcSysResource(ARCID_BTLV, 0x162);
        for (i = 0; i < 2; i++) {
            stage->anmObjs[i] = GFL_HeapAllocate(stage->heapId, 3 * sizeof(G3DAnimation *), TRUE, "btlv_stage.c", 0xdb);
            stage->anms[i] = GFL_HeapAllocate(stage->heapId, sizeof(BtlvStageAnm), TRUE, "btlv_stage.c", 0xdc);
            stage->anmObjs[i][0] = GFL_G3DAnmCreate(stage->mdls[i], stage->anmResources[0][0], 0);
            stage->anms[i][0].mode = STAGE_ANM_ON_START;
            stage->actors[i] = GFL_G3DActorCreate(stage->mdls[i], stage->anmObjs[i], stage->anmCount);
            GFL_G3DActorBindAnm(stage->actors[i], 0);
        }
    } else if (stage->anmCount != 0) {
        stage->anmResCount = 2;
        for (i = 0; i < 2; i++) {
            stage->anmResources[i] =
                GFL_HeapAllocate(stage->heapId, 3 * sizeof(G3DResource *), TRUE, "btlv_stage.c", 0xed);
        }
        for (i = 0; i < 2; i++) {
            stage->anmObjs[i] = GFL_HeapAllocate(stage->heapId, 3 * sizeof(G3DAnimation *), TRUE, "btlv_stage.c", 0xf3);
            stage->anms[i] = GFL_HeapAllocate(stage->heapId, 3 * sizeof(BtlvStageAnm), TRUE, "btlv_stage.c", 0xf4);
        }
        for (i = 1; i <= 3; i++) {
            if (data[stageId].resources[i][variant * 2] != 0xffff) {
                for (j = 0; j < 2; j++) {
                    stage->anmResources[j][anm] =
                        GFL_G3DSysReadArcSysResource(ARCID_BTLV, data[stageId].resources[i][variant * 2 + j]);
                    stage->anmObjs[j][anm] = GFL_G3DAnmCreate(stage->mdls[j], stage->anmResources[j][anm], 0);
                    stage->anms[j][anm].mode = STAGE_ANM_LOOP;
                }
                anm++;
            }
        }
        for (i = 0; i < 2; i++) {
            stage->actors[i] = GFL_G3DActorCreate(stage->mdls[i], stage->anmObjs[i], stage->anmCount);
            for (j = 0; j < stage->anmCount; j++) {
                GFL_G3DActorBindAnm(stage->actors[i], j);
            }
        }
    } else {
        stage->actors[0] = GFL_G3DActorCreate(stage->mdls[0], NULL, 0);
        stage->actors[1] = GFL_G3DActorCreate(stage->mdls[1], NULL, 0);
    }

    size = NNS_G3dGetTex(GFL_G3DResGetResData(stage->models[0]))->plttInfo.sizePltt << 3;
    stage->fade.resources = GFL_HeapAllocate(stage->heapId, sizeof(G3DResource *), TRUE, "btlv_stage.c", 0x120);
    stage->fade.palettes = GFL_HeapAllocate(stage->heapId, sizeof(void *), TRUE, "btlv_stage.c", 0x121);
    stage->fade.resources[0] = stage->models[0];
    stage->fade.palettes[0] = GFL_HeapAllocate(stage->heapId, size, TRUE, "btlv_stage.c", 0x124);
    stage->fade.active = FALSE;
    stage->fade.count = 1;

    // Larger grounds for double and triple battles
    scale = FX32_ONE;
    if (battleStyle == BTL_STYLE_TRIPLE) {
        scale = FX32_CONST(1.5);
    } else if (battleStyle == BTL_STYLE_DOUBLE) {
        scale = FX32_CONST(1.2);
    }
    BtlvStage_InitSRT(stage, 0, scale);
    BtlvStage_InitSRT(stage, 1, scale);
    if (battleStyle == BTL_STYLE_ROTATION) {
        stage->srt[0].translation.z = FX32_CONST(10);
        stage->srt[1].translation.z = FX32_CONST(-15);
    }

    G3X_SetEdgeColorTable(&data[stageId].edgeColor);
    GFL_HeapFree(data);
    return stage;
}

void BtlvStage_Delete(BtlvStage *stage) {
    int i, j;

    if (stage->anmCount != 0) {
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 2; j++) {
                if (stage->anmObjs[j][i] != NULL) {
                    GFL_G3DAnmFree(stage->anmObjs[j][i]);
                }
            }
            for (j = 0; j < stage->anmResCount; j++) {
                if (stage->anmResources[j][i] != NULL) {
                    GFL_G3DResFree(stage->anmResources[j][i]);
                }
            }
        }
        for (j = 0; j < 2; j++) {
            GFL_HeapFree(stage->anmObjs[j]);
            GFL_HeapFree(stage->anms[j]);
        }
        for (j = 0; j < stage->anmResCount; j++) {
            GFL_HeapFree(stage->anmResources[j]);
        }
    }
    for (i = 0; i < 2; i++) {
        GFL_G3DActorFree(stage->actors[i]);
        GFL_G3DMdlFree(stage->mdls[i]);
    }
    GFL_G3DResFree(stage->models[1]);
    GFL_G3DResFree(stage->models[0]);
    GFL_HeapFree(stage->fade.palettes[0]);
    GFL_HeapFree(stage->fade.palettes);
    GFL_HeapFree(stage->fade.resources);
    GFL_HeapFree(stage);
}

void BtlvStage_Main(BtlvStage *stage) {
    int i, j;
    fx32 speed;

    if (stage->anmCount != 0) {
        for (i = 0; i < stage->anmCount; i++) {
            for (j = 0; j < 2; j++) {
                speed = 0;
                switch (stage->anms[j][i].mode) {
                case STAGE_ANM_LOOP:
                    speed = FX32_ONE;
                    break;
                case STAGE_ANM_ON_START:
                    if (stage->anms[j][i].active == TRUE) {
                        speed = stage->anms[j][i].speed;
                        stage->anms[j][i].frames--;
                        if (stage->anms[j][i].frames == 0) {
                            stage->anms[j][i].active = FALSE;
                        }
                    }
                    break;
                }
                if (speed != 0) {
                    GFL_G3DActorStepAnmFrameLoop(stage->actors[j], i, speed);
                }
            }
        }
    }
    BtlvEffect_UpdateTexPaletteFade(&stage->fade);
}

void BtlvStage_Draw(BtlvStage *stage) {
    int i;

    for (i = 0; i < 2; i++) {
        if (!(stage->hidden & (1 << i))) {
            GFL_G3DSysDrawObj(stage->actors[i], &stage->srt[i]);
        }
    }
}

void BtlvStage_StartPaletteFade(BtlvStage *stage, u8 startEvy, u8 targetEvy, u8 wait, u16 color) {
    stage->fade.active = TRUE;
    stage->fade.evy = startEvy;
    stage->fade.targetEvy = targetEvy;
    stage->fade.wait = 0;
    stage->fade.waitFrames = wait;
    stage->fade.color = color;
}

BOOL BtlvStage_IsPaletteFading(BtlvStage *stage) {
    if (stage->fade.active) {
        return TRUE;
    }
    return FALSE;
}

void BtlvStage_SetVanish(BtlvStage *stage, u32 side, BOOL hide) {
    if (side == BTLV_STAGE_SIDE_BOTH) {
        if (hide) {
            stage->hidden = 3;
        } else {
            stage->hidden = 0;
        }
    } else if (hide) {
        stage->hidden |= 1 << side;
    } else {
        stage->hidden &= (1 << side) ^ 3;
    }
}

void BtlvStage_StartAnimation(BtlvStage *stage, u32 side, u32 anm, fx32 speed, s32 frames) {
    if (stage->anms[side][anm].mode == STAGE_ANM_ON_START) {
        stage->anms[side][anm].active = TRUE;
        stage->anms[side][anm].speed = speed;
        stage->anms[side][anm].frames = frames;
    }
}

BOOL BtlvStage_IsAnimating(BtlvStage *stage) {
    int i;

    for (i = 0; i < stage->anmCount; i++) {
        if (stage->anms[0][i].active == TRUE || stage->anms[1][i].active == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}
