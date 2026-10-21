#include "types.h"
#include "field/event_data.h"
#include "field/field_map.h"
#include "field/resort_mapcreate.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "save/join_avenue.h"
#include "system/resort_binary.h"
#include "system/resort_layout.h"

ResortMapCreateWork *func_ov036_021c8954(HeapID heapId) {
    ResortMapCreateWork *work;

    work = GFL_HeapAllocate(heapId, sizeof(ResortMapCreateWork), TRUE, "resort_mapcreate.c", 73);
    work->heapId = heapId;
    work->zoneId = 0xffff;
    return work;
}

void func_ov036_021c897c(ResortMapCreateWork *work) {
    GFL_HeapFree(work);
}

void func_ov036_021c8984(ResortMapCreateWork *work, u16 zoneId, JoinAvenueInfo *info, JoinAvenueOccupants *occupants) {
    work->zoneId = zoneId;
    work->info = info;
    work->occupants = occupants;
}

u32 func_ov036_021c898c(ResortMapCreateWork *work, void *map, FieldPropSystem *propSystem, FieldChunk *fieldChunk,
                        u32 propCount, u32 chunkIndex, HeapID heapId) {
    if (IsZoneJoinAvenue(work->zoneId)) {
        return func_ov036_021c89cc(work, map, propSystem, fieldChunk, propCount, chunkIndex, heapId);
    }
    return propCount;
}

u32 func_ov036_021c89cc(ResortMapCreateWork *work, void *map, FieldPropSystem *propSystem, FieldChunk *fieldChunk,
                        u32 propCount, u32 chunkIndex, HeapID heapId) {
    ResortShopData *shops = ResortShopData_Load(HEAPID_TAIL(work->heapId));
    ResortBinary *table = ResortBinary_Load(3, 4, HEAPID_TAIL(work->heapId));
    s32 i;
    u16 column;
    u16 row;
    u16 rows;
    JoinAvenuePerson *person;
    const u16 *shop;
    u32 x;
    u32 y;
    u32 srcY;
    BOOL addBuildings;
    LandDataPatch *patch;

    for (i = 0; i < JOIN_AVE_OCCUPANT_COUNT; i++) {
        column = ResortBinary_Get(table, i, 1);
        row = ResortBinary_Get(table, i, 2);
        rows = 7;
        if (chunkIndex == row / 32 || chunkIndex == (row + 7) / 32) {
            person = JoinAvenueOccupants_GetPerson(work->occupants, i);
            if (!JoinAvenuePerson_IsEmpty(person)) {
                shop = ResortShopData_GetPersonShop(shops, person);
                x = column % 32;
                y = row % 32;
                srcY = 0;
                addBuildings = TRUE;
                if (i % 2 == 1) {
                    patch = ReadLandDataPatchA154Data(ResortShopData_GetShopParam(shop, 6), HEAPID_TAIL(heapId));
                } else {
                    patch = ReadLandDataPatchA154Data(ResortShopData_GetShopParam(shop, 7), HEAPID_TAIL(heapId));
                }
                if (chunkIndex == row / 32) {
                    if (y + 7 >= 32) {
                        rows = 32 - y;
                    }
                } else {
                    srcY = 32 - y;
                    rows = 7 - srcY;
                    y = 0;
                    addBuildings = FALSE;
                }
                func_ov036_021c2d04(patch, map, 0, srcY, x, y, 6, rows);
                if (addBuildings) {
                    propCount = LoadLandDataPatchBuildings(patch, propSystem, fieldChunk, propCount, x, y);
                }
                FreeLandDataPatch(patch);
            }
        }
    }
    ResortBinary_Free(table);
    ResortShopData_Free(shops);
    return propCount;
}

void func_ov036_021c8b40(ResortMapCreateWork *work, EventData *eventData) {
    ResortShopData *shops = ResortShopData_Load(HEAPID_TAIL(work->heapId));
    ResortBinary *table = ResortBinary_Load(3, 4, HEAPID_TAIL(work->heapId));
    s32 i;
    JoinAvenuePerson *person;
    u16 entity;
    u16 shopId;
    const u16 *shop;
    u16 position[2];

    for (i = 0; i < JOIN_AVE_OCCUPANT_COUNT; i++) {
        person = JoinAvenueOccupants_GetPerson(work->occupants, i);
        entity = ResortBinary_Get(table, i, 3);
        if (JoinAvenuePerson_IsEmpty(person)) {
            SetBGEntityLocation(eventData, entity, 0, 0, 0);
        } else {
            shopId = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(person), 0);
            shop = ResortShopData_GetShop(shops, shopId);
            ResortLayout_GetShopOffset(ResortShopData_GetShopParam(shop, (i % 2 == 0 ? 1 : 0) + 6), &position[1],
                                       &position[0]);
            position[1] += ResortBinary_Get(table, i, 1);
            position[0] += ResortBinary_Get(table, i, 2);
            SetBGEntityLocation(eventData, entity, position[1], 0, position[0]);
        }
    }
    ResortBinary_Free(table);
    ResortShopData_Free(shops);
}
