#ifndef POKEBW2_FIELD_DELIVERY_IRC_H
#define POKEBW2_FIELD_DELIVERY_IRC_H

// Overlay 12's delivery_irc.c: hands out or receives data over infrared, with nearly the same init as delivery_beacon.c.
// The sender picks the data whose region and mask match the receiver's, and sends it encrypted with its CRC

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
void *func_ov012_0215309c(const DeliveryIrcInit *init);
// Starts the network, unless it is busy
BOOL func_ov012_021530f8(void *work);
// 1 once the data came or went, 2 if it came damaged, 3 if there was none to give
u8 func_ov012_02153130(void *work);
void func_ov012_0215313c(void *work);
void func_ov012_02153150(void *work);

// Overlay 31
// Sets the callback that the infrared link of overlay 31 calls with two values, or NULL for none
void func_ov031_02175a04(BOOL (*callback)(int a, int b));
void func_ov031_021759d0(u32 a0);
BOOL func_ov031_02175d8c(void);

#endif // POKEBW2_FIELD_DELIVERY_IRC_H
