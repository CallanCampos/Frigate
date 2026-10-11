#ifndef POKEBW2_GFL_NET_WHPIPE_H
#define POKEBW2_GFL_NET_WHPIPE_H

#include "types.h"
#include "gfl/wih.h"

// net_whpipe.c, in overlay 30: the GFL net's wireless pipe, over wih.c: beacons, scanning and the connection

// The wireless pipe's state machine: its work begins with the state function that runs next, which takes the work
typedef void (*NetWhpipeStateFunc)(void *work);
typedef struct {
    NetWhpipeStateFunc state;
} NetWhpipeState;

// Sets the state that the pipe's work runs next, as the pipe's files do to move on
void NetWhpipe_SetState(NetWhpipeState *work, NetWhpipeStateFunc state);

// The game service ID of a beacon the scan found
u8 func_ov030_02173b78(int index);

// The Funfest's beacon
void func_ov030_02174108(u32 enabled);
// Whether the Funfest's beacon updates are sent (WH_GetSystemState returns 2)
BOOL func_ov030_02173c08(void);
// Sends the game's beacon
void func_ov030_02173780(void);

// The beacons that deliveries come by
void *func_ov030_02173b24(int index);
void func_ov030_02173ba4(int index);
void func_ov030_02173bc4(void);
void func_ov030_021740d0(int index);
u8 func_ov030_021740a4(u8 index);
void func_ov030_02173bec(int index);

// What the device table's functions in net_devwireless.c are built on
BOOL func_ov030_02173340(HeapID heapId, WHCallback callback, void *work, BOOL flag);
BOOL func_ov030_02173aa0(void);
void func_ov030_02173964(u16 connectBits);
BOOL func_ov030_021736b8(void);
void func_ov030_02173554(void);
u8 *func_ov030_02173b50(int index);
BOOL func_ov030_02173bf4(void);
BOOL func_ov030_02173cf8(BOOL a0);
BOOL func_ov030_021735b0(BOOL a0);
BOOL func_ov030_02173e0c(BOOL a0, const u8 *mac, int index, void (*callback)(void));
BOOL func_ov030_02173c1c(u8 *data, int size, BOOL (*callback)(BOOL ok));
void func_ov030_021733d0(BOOL (*callback)(u16 netId, u8 *data, u16 size));
BOOL func_ov030_021739ec(void);
BOOL func_ov030_02173a8c(void);
int func_ov030_02173c28(void);
int func_ov030_02173c30(void);
BOOL func_ov030_02173ae4(void);
void func_ov030_02173b04(int value);
void func_ov030_02173ec8(BOOL a0);
BOOL func_ov030_02174040(void);
void func_ov030_021740f0(int unused);
BOOL func_ov030_02173e9c(WHCallback callback);


// Stops the beacon when it was told to lock
void func_ov030_02174088(void);
// Sets what the scan calls with each beacon it finds and the game's argument
void func_ov030_02174120(void (*callback)(void *arg, const void *beacon, u8 gameCommandBase, u8 childCount), void *arg);

#endif // POKEBW2_GFL_NET_WHPIPE_H
