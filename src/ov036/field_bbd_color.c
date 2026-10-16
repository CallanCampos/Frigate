#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_map.h"
#include "gfl/arc.h"
#include "gfl/blact.h"
#include "gfl/heap.h"
#include "nitro/gx.h"

// The material colors of the billboard actors (field_bbd_color.c, after the file name that its allocation passes), and
// the dive spots of Undella Bay. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the
// layout is ours

typedef struct {
    ArcTool *arcTool;
    s32 normal[3];
    u16 diffuse[3];
    u16 ambient[3];
    u16 specular[3];
    u16 emissive[3];
} FldActMatColorPreset;

static u16 DecideAbyssalRuinsDiveZoneByUndellaBayPos(u32 zoneId, u32 x, u32 z);

// Dive spots of Undella Bay: zone, x, z, the Abyssal Ruins zone they lead to
static const u16 UNDELLA_BAY_DIVE_WARPS[4][4] = {
    { 0xf0, 0x320, 0x113, 0xf1 },
    { 0xf0, 0x311, 0x121, 0xf2 },
    { 0xf0, 0x32f, 0x121, 0xf3 },
    { 0xf0, 0x320, 0x133, 0xf4 },
};

void *FldActMatColorPreset_Create(HeapID heapId) {
    FldActMatColorPreset *preset =
        GFL_HeapAllocate(heapId, sizeof(FldActMatColorPreset), TRUE, "field_bbd_color.c", 91);

    preset->arcTool = GFL_ArcSysCreateFileHandle(0xad, heapId);
    return preset;
}

void FldActMatColorPreset_Free(void *work) {
    FldActMatColorPreset *preset = work;

    GFL_ArcToolFree(preset->arcTool);
    GFL_HeapFree(preset);
}

void FldActMatColorPreset_Load(void *work, u32 presetId) {
    FldActMatColorPreset *preset = work;

    GFL_ArcToolRead(preset->arcTool, presetId, preset->normal);
}

void FldActMatColorPreset_Apply(void *work, BlActScene *scene) {
    FldActMatColorPreset *preset = work;
    VecFx16 normal;
    GXRgb diffuse, ambient, specular, emissive;

    normal.x = preset->normal[0];
    normal.y = preset->normal[1];
    normal.z = preset->normal[2];
    VEC_Fx16Normalize(&normal, &normal);
    BlActScene_SetCustomNormal(scene, &normal);
    diffuse = preset->diffuse[0] | (preset->diffuse[1] << 5) | (preset->diffuse[2] << 10);
    BlActScene_SetColDiffuse(scene, &diffuse);
    ambient = preset->ambient[0] | (preset->ambient[1] << 5) | (preset->ambient[2] << 10);
    BlActScene_SetColAmbient(scene, &ambient);
    specular = preset->specular[0] | (preset->specular[1] << 5) | (preset->specular[2] << 10);
    BlActScene_SetColSpecular(scene, &specular);
    emissive = preset->emissive[0] | (preset->emissive[1] << 5) | (preset->emissive[2] << 10);
    BlActScene_SetColEmissive(scene, &emissive);
}

static u16 DecideAbyssalRuinsDiveZoneByUndellaBayPos(u32 zoneId, u32 x, u32 z) {
    u32 i;
    const u16 *warp = UNDELLA_BAY_DIVE_WARPS[0];

    for (i = 0; i < 4; i++, warp += 4) {
        if (warp[0] == zoneId && warp[1] <= x && (s32)x < warp[1] + 3 && warp[2] <= z && (s32)z < warp[2] + 3) {
            return warp[3];
        }
    }
    return 0x267;
}

u16 GetAbyssalRuinsDiveZoneID(Field *field, u16 *zoneId) {
    u16 playerZoneId = Field_GetPlayerStateZoneID(field);
    FieldActor *actor = FieldPlayer_GetActor(Field_GetPlayer(field));
    u16 x = GetGPosX(actor);
    u16 z = GetGPosZ(actor);

    *zoneId = DecideAbyssalRuinsDiveZoneByUndellaBayPos(playerZoneId, x, z);
    if (*zoneId != 0x267) {
        return TRUE;
    }
    *zoneId = playerZoneId;
    return FALSE;
}
