#ifndef POKEBW2_FIELD_FIELD_LENS_FLARE_H
#define POKEBW2_FIELD_FIELD_LENS_FLARE_H

#include "types.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// No lens flare: the end of an effect set's lens flares, after the 8 lens flares
#define LENS_FLARE_NONE 8

typedef struct FieldLensFlareEntry {
    u16 zoneId;
    u16 transitId;
    u16 effectSetIds[4][3];
} FieldLensFlareEntry;

typedef struct FieldLensFlareData {
    u32 byteCount;
    FieldLensFlareEntry *entries;
} FieldLensFlareData;

struct FieldLensFlare {
    FieldExpObjSystem *expObjSys;
    u32 requested;
    u32 active;
    u32 available;
    u32 state;
    FieldLensFlareData *ownedData;
    GameSystem *gameSystem;
    // A lens flare, or LENS_FLARE_NONE
    u16 effectId;
    u16 unk1e;
    GameData *gameData;
};

FieldLensFlareData *FieldLensFlareData_Create(HeapID heapId);
FieldLensFlare *FieldLensFlare_Create(GameSystem *gameSystem, GameData *gameData, FieldExpObjSystem *expObjSys,
                                      u8 effectIndex, u32 dayPeriod, HeapID heapId);
void FieldLensFlareData_Free(FieldLensFlareData *data);
void FieldLensFlare_Free(FieldLensFlare *lensFlare);
void FieldLensFlare_Update(FieldLensFlare *lensFlare, FieldCamera *camera);
void FieldLensFlare_Load(FieldExpObjSystem *expObjSys, FieldLensFlareData *data, u16 effectId);
void FieldLensFlare_CalcPos(FieldLensFlare *lensFlare, G3DCamera *camera);
void FieldLensFlare_RequestStart(FieldLensFlare *lensFlare);
void FieldLensFlare_GreenlightStart(FieldLensFlare *lensFlare);
void FieldLensFlare_Cancel(FieldLensFlare *lensFlare);
void FieldLensFlare_DecideForZoneTransit(FieldLensFlare *lensFlare, u16 zoneId, u16 prevZoneId, u32 fog);
BOOL FieldLensFlare_IsApplicable(GameData *gameData, u16 zoneId);
u8 FieldLensFlare_GetSubIndexForDayPeriod(u32 period);
u16 FieldLensFlare_GetEffectSetID(FieldLensFlareData *data, u32 entryIndex, u32 effectIndex, u32 subIndex);
u32 FieldLensFlareData_BytesToEntryCount(FieldLensFlareData *data);
u16 FieldLensFlareData_GetLensFlareID(FieldLensFlareData *data, u32 effectSet, u32 index);
u8 FieldLensFlareData_GetEffectSetSize(FieldLensFlareData *data, u32 effectSet);
u16 FieldLensFlareData_GetResDatID(FieldLensFlareData *data, u32 effectSet, u32 index);
FieldLensFlareEntry *FieldLensFlareData_GetEntry(FieldLensFlareData *data, u32 index);
u32 FieldLensFlareData_GetIdxForZoneTransit(FieldLensFlareData *data, u16 zoneId, u16 transitId);

#endif // POKEBW2_FIELD_FIELD_LENS_FLARE_H
