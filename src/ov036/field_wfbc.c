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

static void func_ov036_0218adfc(FieldWFBC *wfbc, HeapID heapId);
static void func_ov036_0218ae00(FieldWFBC *wfbc);
static void func_ov036_0218ae04(FieldWFBC *wfbc);
static void func_ov036_0218ae10(FieldWFBC *work, CityState *city, BOOL isOther);

FieldWFBC *func_ov036_0218adac(HeapID heapId) {
    FieldWFBC *wfbc = GFL_HeapAllocate(heapId, sizeof(FieldWFBC), TRUE, "field_wfbc.c", 87);

    func_ov036_0218ae04(wfbc);
    return wfbc;
}

void func_ov036_0218add0(FieldWFBC *wfbc) {
    func_ov036_0218ae00(wfbc);
    GFL_HeapFree(wfbc);
}

void func_ov036_0218ade0(FieldWFBC *wfbc, CityState *city, BOOL isOther, HeapID heapId) {
    func_ov036_0218adfc(wfbc, heapId);
    func_ov036_0218ae10(wfbc, city, isOther);
}

static void func_ov036_0218adfc(FieldWFBC *wfbc, HeapID heapId) {
}

static void func_ov036_0218ae00(FieldWFBC *wfbc) {
}

static void func_ov036_0218ae04(FieldWFBC *wfbc) {
    wfbc->city = 0;
    wfbc->isOther = FALSE;
    wfbc->state = NULL;
}

static void func_ov036_0218ae10(FieldWFBC *wfbc, CityState *city, BOOL isOther) {
    CityState_IsSet(city);
    func_ov036_0218ae04(wfbc);
    wfbc->city = city->city;
    wfbc->isOther = isOther;
    if (isOther == FALSE) {
        wfbc->state = city;
    } else {
        wfbc->state = NULL;
    }
}
