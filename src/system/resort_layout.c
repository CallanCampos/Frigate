// resort_layout.c: the Join Avenue's fixed layout, its zones and where its people and shops stand. The name is a
// descriptive guess: no string names the file. It is a file of its own, between resonance_resort_data.c and
// resort_binary.c, because its .rodata starts the sizes over: MWCC lays out a file's data by size, and this 6-byte
// table follows resonance_resort_data.c's tables of 16 to 49 bytes. Our names
#include "types.h"
#include "system/resort_layout.h"

// A place on the grid and the direction faced there
typedef struct {
    u16 x;
    u16 z;
    u32 dir;
} ResortPlace;

typedef struct {
    u16 x;
    u16 y;
} ResortOffset;

// Zone 0 and the avenue's two zones
static const u16 sZones[RESORT_ZONE_COUNT] = { 0, 490, 491 };

static const u32 sUnk[4] = { 1, 1, 1, 2 };

static const ResortPlace sPlaces2[4] = {
    { 13, 5, 1 },
    { 13, 7, 0 },
    { 11, 11, 2 },
    { 10, 10, 1 },
};

static const ResortOffset sShopOffsets[48] = {
    { 0, 2 }, { 5, 2 }, { 0, 3 }, { 5, 1 }, { 0, 2 }, { 5, 2 }, { 0, 2 }, { 0, 2 }, { 0, 2 }, { 5, 2 },
    { 5, 2 }, { 5, 2 }, { 0, 2 }, { 0, 2 }, { 0, 2 }, { 5, 2 }, { 5, 2 }, { 5, 2 }, { 0, 2 }, { 0, 2 },
    { 0, 2 }, { 5, 2 }, { 5, 2 }, { 5, 2 }, { 0, 2 }, { 0, 2 }, { 0, 2 }, { 5, 2 }, { 5, 2 }, { 5, 2 },
    { 0, 2 }, { 0, 2 }, { 0, 2 }, { 5, 2 }, { 5, 2 }, { 5, 2 }, { 0, 2 }, { 0, 2 }, { 0, 2 }, { 5, 2 },
    { 5, 2 }, { 5, 2 }, { 0, 2 }, { 0, 2 }, { 0, 3 }, { 5, 2 }, { 5, 2 }, { 5, 1 },
};

static const ResortPlace sPlaces[48] = {
    { 2, 3, 2 }, { 3, 3, 3 }, { 2, 4, 2 }, { 3, 2, 3 }, { 1, 3, 2 }, { 4, 3, 3 }, { 2, 3, 2 }, { 2, 3, 2 },
    { 1, 3, 2 }, { 3, 3, 3 }, { 3, 3, 3 }, { 4, 3, 3 }, { 2, 3, 2 }, { 2, 3, 2 }, { 1, 3, 2 }, { 3, 3, 3 },
    { 3, 3, 3 }, { 4, 3, 3 }, { 2, 3, 2 }, { 2, 3, 2 }, { 1, 3, 2 }, { 3, 3, 3 }, { 3, 3, 3 }, { 4, 3, 3 },
    { 2, 3, 2 }, { 2, 3, 2 }, { 1, 3, 2 }, { 3, 3, 3 }, { 3, 3, 3 }, { 4, 3, 3 }, { 2, 3, 2 }, { 2, 3, 2 },
    { 1, 3, 2 }, { 3, 3, 3 }, { 3, 3, 3 }, { 4, 3, 3 }, { 2, 3, 2 }, { 1, 3, 2 }, { 1, 3, 2 }, { 3, 3, 3 },
    { 4, 3, 3 }, { 4, 3, 3 }, { 2, 3, 2 }, { 2, 3, 2 }, { 1, 4, 2 }, { 3, 3, 3 }, { 3, 3, 3 }, { 4, 2, 3 },
};

u16 ResortLayout_GetZoneId(u32 index) {
    return sZones[index];
}

u32 ResortLayout_GetZoneIndex(u16 zoneId) {
    int i;

    for (i = 0; i < RESORT_ZONE_COUNT; i++) {
        if (zoneId == sZones[i]) {
            return i;
        }
    }
    return 0;
}

void func_02039538(u16 index, u16 *x, u16 *z, u16 *dir) {
    *x = sPlaces[index].x;
    *z = sPlaces[index].z;
    *dir = sPlaces[index].dir;
}

void func_02039560(u16 index, u16 *x, u16 *y) {
    *x = sShopOffsets[index].x;
    *y = sShopOffsets[index].y;
}

void func_02039578(u16 index, u16 *x, u16 *z, u16 *dir) {
    *x = sPlaces2[index].x;
    *z = sPlaces2[index].z;
    *dir = sPlaces2[index].dir;
}

u32 func_020395a0(u32 index) {
    return sUnk[index];
}
