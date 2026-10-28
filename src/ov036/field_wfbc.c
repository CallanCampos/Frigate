#include "types.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "gfl/heap.h"
#include "system/game_data.h"

struct FieldWFBC {
    u8 city;
    u8 isOther;
    CityState *state;
};

static void FieldWFBC_LoadResources(FieldWFBC *wfbc, HeapID heapId);
static void FieldWFBC_FreeResources(FieldWFBC *wfbc);
static void FieldWFBC_Clear(FieldWFBC *wfbc);
static void FieldWFBC_SetCityState(FieldWFBC *work, CityState *city, BOOL isOther);

FieldWFBC *FieldWFBC_Create(HeapID heapId) {
    FieldWFBC *wfbc = GFL_HeapAllocate(heapId, sizeof(FieldWFBC), TRUE, "field_wfbc.c", 87);

    FieldWFBC_Clear(wfbc);
    return wfbc;
}

void FieldWFBC_Free(FieldWFBC *wfbc) {
    FieldWFBC_FreeResources(wfbc);
    GFL_HeapFree(wfbc);
}

void FieldWFBC_Setup(FieldWFBC *wfbc, CityState *city, BOOL isOther, HeapID heapId) {
    FieldWFBC_LoadResources(wfbc, heapId);
    FieldWFBC_SetCityState(wfbc, city, isOther);
}

// Empty in this game: Black and White's field_wfbc.c loads and frees the city's people data at these two calls
static void FieldWFBC_LoadResources(FieldWFBC *wfbc, HeapID heapId) {
}

static void FieldWFBC_FreeResources(FieldWFBC *wfbc) {
}

static void FieldWFBC_Clear(FieldWFBC *wfbc) {
    wfbc->city = 0;
    wfbc->isOther = FALSE;
    wfbc->state = NULL;
}

static void FieldWFBC_SetCityState(FieldWFBC *wfbc, CityState *city, BOOL isOther) {
    CityState_IsSet(city);
    FieldWFBC_Clear(wfbc);
    wfbc->city = city->city;
    wfbc->isOther = isOther;
    if (isOther == FALSE) {
        wfbc->state = city;
    } else {
        wfbc->state = NULL;
    }
}
