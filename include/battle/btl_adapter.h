#ifndef POKEBW2_BATTLE_BTL_ADAPTER_H
#define POKEBW2_BATTLE_BTL_ADAPTER_H

// Overlay 167's btl_adapter.c, which passes the server's commands to a client and the client's replies back

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The commands that the server sends its clients through their adapters (BtlAdapter_SetCmd), named after what the
// client does: set up the view, the players' choices, play the server's commands (SC_*), quit with the client IDs,
// store the record, end the battle by its kind, and show that the time is up
enum {
    BTL_ACMD_NONE,
    BTL_ACMD_SETUP_VIEW,
    BTL_ACMD_SELECT_ROTATION,
    BTL_ACMD_SELECT_ACTION,
    BTL_ACMD_CONFIRM_SHIFT_SWITCH,
    BTL_ACMD_SELECT_CHANGE_FAINTED,
    BTL_ACMD_SELECT_CHANGE_MID_TURN,
    BTL_ACMD_CONFIRM_NEXT_POKE,
    BTL_ACMD_SERVER_CMD,
    BTL_ACMD_QUIT,
    BTL_ACMD_STORE_RECORD,
    BTL_ACMD_EXIT_TRAINER,
    BTL_ACMD_EXIT_FACILITY,
    BTL_ACMD_EXIT_COMM,
    BTL_ACMD_EXIT_WILD_WIN,
    BTL_ACMD_EXIT_WILD_LOSE,
    BTL_ACMD_TIME_UP,
};

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
