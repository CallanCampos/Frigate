#ifndef POKEBW2_FIELD_DELIVERY_BEACON_H
#define POKEBW2_FIELD_DELIVERY_BEACON_H

// Overlay 12's delivery_beacon.c: receives data that is handed out over wireless beacons, in up to 18 beacons of 84
// bytes, as the Battle Test of the Trial House does. pWork, aInit, datasize and the DELIVERY_BEACON_* names are from
// the file's assert

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

#define DELIVERY_BEACON_MAX_NUM 18
#define DELIVERY_BEACON_ONCE_NUM 84

typedef struct {
    // The size of the data and where it goes
    int datasize;
    u8 *pData;
    // Only beacons of this region and of one of these bits are taken
    u32 region;
    u32 mask;
} DeliveryData;

typedef struct {
    // The high byte of the network commands
    u32 code;
    u8 flag4;
    u8 pad5;
    HeapID heapId;
    DeliveryData data[7];
    u32 dataNum;
} DeliveryInit;

// The work is passed as void * by the events that use it
void *func_ov012_02152990(const DeliveryInit *init);
// Starts receiving, unless the network is busy
BOOL func_ov012_02152b64(void *work);
// Whether any beacon came
BOOL func_ov012_02152bb4(void *work);
// Whether every beacon came, after which the data is decrypted into the buffer
BOOL func_ov012_02152bd4(void *work);
void func_ov012_02152bec(void *work);
void func_ov012_02152bfc(void *work);


#endif // POKEBW2_FIELD_DELIVERY_BEACON_H
