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

DeliveryBeaconWork *DeliveryBeacon_Init(const DeliveryInit *init);
// Starts receiving, unless the network is busy
BOOL DeliveryBeacon_RecvStart(DeliveryBeaconWork *work);
// Whether any beacon came
BOOL DeliveryBeacon_IsAnyReceived(DeliveryBeaconWork *work);
// Whether every beacon came, after which the data is decrypted into the buffer
BOOL DeliveryBeacon_IsReceiveDone(DeliveryBeaconWork *work);
void DeliveryBeacon_Main(DeliveryBeaconWork *work);
void DeliveryBeacon_End(DeliveryBeaconWork *work);

#endif // POKEBW2_FIELD_DELIVERY_BEACON_H
