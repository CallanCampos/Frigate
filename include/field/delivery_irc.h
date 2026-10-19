#ifndef POKEBW2_FIELD_DELIVERY_IRC_H
#define POKEBW2_FIELD_DELIVERY_IRC_H

// Overlay 12's delivery_irc.c: hands out or receives data over infrared, with nearly the same init as
// delivery_beacon.c. The sender picks the data whose region and mask match the receiver's, and sends it encrypted with
// its CRC

#include "types.h"
#include "field/delivery_beacon.h"

// The init of delivery_beacon.c, but with a halfword at offset 4
typedef struct {
    u32 code;
    u16 unk04;
    HeapID heapId;
    DeliveryData data[7];
    u32 dataNum;
} DeliveryIrcInit;

// The work is passed as void * by the screens that use it
void *DeliveryIrc_Init(const DeliveryIrcInit *init);
// Starts the network, unless it is busy
BOOL DeliveryIrc_RecvStart(void *work);
// 1 once the data came or went, 2 if it came damaged, 3 if there was none to give
u8 DeliveryIrc_GetResult(void *work);
void DeliveryIrc_Main(void *work);
void DeliveryIrc_End(void *work);

#endif // POKEBW2_FIELD_DELIVERY_IRC_H
