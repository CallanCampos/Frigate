#include "system/resort_binary.h"
#include "types.h"
#include "constants/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "save/join_avenue.h"

// The Join Avenue's tables of u16 rows. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0):
// join_ave_raffle_shop. The rest are ours

static const u16 *ResortBinary_GetRow(ResortBinary *table, u32 row);
static u32 ResortShopData_GetShopCount(ResortShopData *data);

ResortBinary *ResortBinary_Load(u32 fileId, u32 columns, HeapID heapId) {
    ResortBinary *table = GFL_HeapAllocate(heapId, sizeof(ResortBinary), TRUE, "resort_binary.c", 58);

    table->data = GFL_ArcSysReadHeapNewLZGetLen(ARCID_RESORT_BINARY, fileId, FALSE, heapId, &table->size);
    table->columns = columns;
    return table;
}

void ResortBinary_Free(ResortBinary *table) {
    GFL_HeapFree(table->data);
    GFL_HeapFree(table);
}

u16 ResortBinary_Get(ResortBinary *table, u32 row, u32 column) {
    return table->data[table->columns * row + column];
}

u32 ResortBinary_GetRowCount(ResortBinary *table) {
    return table->size / (table->columns * sizeof(u16));
}

static const u16 *ResortBinary_GetRow(ResortBinary *table, u32 row) {
    return &table->data[table->columns * row];
}

u16 ResortBinary_GetFoundValue(const u16 *row) {
    return row[3];
}

const u16 *ResortBinary_FindRow(ResortBinary *table, u32 key0, u32 key1, u16 key2) {
    u32 i;

    for (i = 0; i < ResortBinary_GetRowCount(table); i++) {
        const u16 *row = ResortBinary_GetRow(table, i);

        if (key0 == ResortBinary_Get(table, i, 0) && key1 == ResortBinary_Get(table, i, 1) &&
            key2 == ResortBinary_Get(table, i, 2)) {
            return row;
        }
    }
    return NULL;
}

u32 ResortBinary_FindRange(ResortBinary *table, u32 value) {
    u32 i;

    for (i = 0; i < ResortBinary_GetRowCount(table); i++) {
        u16 start = ResortBinary_Get(table, i, 0);
        u16 end = ResortBinary_Get(table, i, 1);

        if (start <= value && value <= end) {
            return i;
        }
    }
    return 0;
}

u16 ResortBinary_GetLastRangeEnd(ResortBinary *table) {
    return ResortBinary_Get(table, ResortBinary_GetRowCount(table) - 1, 1);
}

ResortShopData *ResortShopData_Load(HeapID heapId) {
    ResortShopData *data = GFL_HeapAllocate(heapId, sizeof(ResortShopData), TRUE, "resort_binary.c", 261);

    data->shops = ResortBinary_Load(1, 25, heapId);
    data->goods = ResortBinary_Load(2, 10, heapId);
    return data;
}

void ResortShopData_Free(ResortShopData *data) {
    ResortBinary_Free(data->goods);
    ResortBinary_Free(data->shops);
    GFL_HeapFree(data);
}

u32 ResortShopData_FindShop(ResortShopData *data, u16 key0, u16 key2, u16 key1) {
    u32 i;

    for (i = 1; i < ResortShopData_GetShopCount(data); i++) {
        const u16 *shop = ResortBinary_GetRow(data->shops, i);

        if (key0 == ResortShopData_GetShopParam(shop, 0) && key1 == ResortShopData_GetShopParam(shop, 1) &&
            key2 == ResortShopData_GetShopParam(shop, 2)) {
            return i;
        }
    }
    return 0;
}

const u16 *ResortShopData_GetPersonShop(ResortShopData *data, JoinAvenuePerson *person) {
    u16 id = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(person), 0);

    return ResortBinary_GetRow(data->shops, id);
}

const u16 *ResortShopData_GetShop(ResortShopData *data, u32 id) {
    return ResortBinary_GetRow(data->shops, id);
}

static u32 ResortShopData_GetShopCount(ResortShopData *data) {
    return ResortBinary_GetRowCount(data->shops);
}

u16 ResortShopData_GetShopParam(const u16 *shop, u32 column) {
    return shop[column];
}

const u16 *ResortShopData_GetGoods(ResortShopData *data, const u16 *shop, u16 index) {
    u16 row = shop[9 + index];

    if (row < ResortBinary_GetRowCount(data->goods)) {
        return ResortBinary_GetRow(data->goods, row);
    }
    return NULL;
}

u32 ResortShopData_GetGoodsCount(ResortShopData *data, const u16 *shop) {
    int i;

    for (i = 0; i < RESORT_SHOP_GOODS_MAX; i++) {
        if (ResortShopData_GetGoods(data, shop, i) == NULL) {
            break;
        }
    }
    return i;
}

u16 ResortShopData_GetGoodsParam(const u16 *goods, u32 column) {
    return goods[column];
}

u16 join_ave_raffle_shop(ResortBinary *table, u32 row, u32 value) {
    u32 sum = 0;
    int i;

    for (i = 0; i < RESORT_RAFFLE_PRIZE_COUNT; i++) {
        // The prize is read but not used: the callers read it again from the index this returns
        ResortBinary_Get(table, row, i * 2 + 3);
        sum += ResortBinary_Get(table, row, i * 2 + 2);
        if (value < sum) {
            return i;
        }
    }
    return RESORT_RAFFLE_PRIZE_COUNT;
}
