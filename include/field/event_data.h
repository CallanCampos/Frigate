#ifndef POKEBW2_FIELD_EVENT_DATA_H
#define POKEBW2_FIELD_EVENT_DATA_H

// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/encounter.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

struct EventData {
    u8 unk0[4];
    ArcTool *entityArc;
    ArcTool *encArc;
    ArcTool *otherArc;
    u16 zoneId;
    u16 entityCount;
    u16 npcCount;
    u16 warpCount;
    u16 triggerCount;
    u8 pad1A[2];
    ZoneBGEntity *entities;
    ZoneNPC *npcs;
    ZoneWarp *warps;
    ZoneTrigger *triggers;
    u16 prevEntityCount;
    u16 prevNpcCount;
    u16 prevWarpCount;
    u16 prevTriggerCount;
    u32 encLoaded;
    EncData encData;
    void *initScript;
    // The zone's entity file, read from here: the offset of its init script from the counts, then the counts of the
    // entities, NPCs, warps and triggers and the entities themselves
    u32 initScriptOffset;
    u8 cache[0x880];
    u8 rest[0x100];
};

extern const char data_ov012_0216e298[];

EventData *EventData_Create(HeapID heapId);
void EventData_Free(EventData *data);
void EventData_Clear(EventData *data);
void EventData_Reset(EventData *data);
// The quicksand trigger at the position, facing direction
ZoneTrigger *FindQuicksandTrigger(EventData *data, EventWork *eventWork, const VecFx32 *position, u32 direction);
void EventData_LoadZone(EventData *data, u16 zoneId, u8 season);
void EventData_LoadEntities(EventData *data, u16 zoneId, u8 season);
void LoadZoneEntities(EventData *data, u16 zoneId, u8 season);
void EventData_LoadEncData(EventData *data, u16 zoneId, u8 season);
void *GetZoneInitScrPointer(EventData *data);
u32 IsEncountDataLoaded(EventData *data);
EncData *GetEncountData(EventData *data);
void *GetZoneProxiesAndCount(EventData *data, u32 *count);
u16 CheckProxyEntityEvent(EventData *data, EventWork *eventWork, const void *position, u16 direction);
u16 CheckProxyEntityEventGrid(EventData *data, EventWork *eventWork, const VecFx32 *position, u16 direction);
u16 CheckProxyEntityEventRail(EventData *data, EventWork *eventWork, const RailPosition *position, u16 direction);
// What the warp lookups return where there is no warp (our name)
#define WARP_ID_NONE 0xffff
s32 GetWarpAtPosition(EventData *data, const VecFx32 *position);
s32 GetWarpIDByPlayerPos(EventData *data, const VecFx32 *position, u8 direction);
s32 GetWarpIDByPlayerPosRail(EventData *data, const RailPosition *position);

#endif // POKEBW2_FIELD_EVENT_DATA_H
