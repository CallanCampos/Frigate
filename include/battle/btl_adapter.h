#ifndef POKEBW2_BATTLE_BTL_ADAPTER_H
#define POKEBW2_BATTLE_BTL_ADAPTER_H

// Overlay 167's btl_adapter.c, which passes the server's commands to a client and the client's replies back

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Sets up the adapters, sharing one per client unless commMode is set
void BtlAdapter_InitSystem(u8 commMode);
void BtlAdapter_QuitSystem(void);
BtlAdapter *BtlAdapter_Create(void *netHandle, u8 clientId, BOOL flag, HeapID heapId);
void BtlAdapter_ResetState(BtlAdapter *adapter);
void BtlAdapter_Delete(BtlAdapter *adapter);
void BtlAdapter_ClearNetReplies(void);
void func_ov167_021d4ae8(void);
void BtlAdapter_SetCmd(BtlAdapter *adapter, u32 cmd, const void *data, u32 size);
BOOL BtlAdapter_WaitCmd(BtlAdapter *adapter);
// The client's reply to the last command, and its size if size isn't NULL
void *BtlAdapter_GetReturnData(BtlAdapter *adapter, u32 *size);
void BtlAdapter_SetIdle(BtlAdapter *adapter);

void BtlAdapter_ClearSendCmd(BtlAdapter *adapter);
u8 BtlAdapter_GetRecvCmd(BtlAdapter *adapter);
// The data the adapter received, and its size
u16 BtlAdapter_GetRecvData(BtlAdapter *adapter, const void **data);
BOOL BtlAdapter_SetReturnData(BtlAdapter *adapter, const void *data, u32 size);

#endif // POKEBW2_BATTLE_BTL_ADAPTER_H
