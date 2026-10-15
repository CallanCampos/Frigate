#ifndef POKEBW2_BATTLE_BTL_CLIENT_H
#define POKEBW2_BATTLE_BTL_CLIENT_H

// Overlay 167's battle clients, each the player or an opponent

#include "types.h"
#include "battle/btl_calc.h"
#include "gfl/heap.h"
#include "nitro/math.h"
#include "struct_decls.h"

void func_ov167_021b18c4(BtlClient *client);
void *func_ov167_021b18d4(BtlClient *client, u32 *size);
void func_ov167_021b190c(BtlClient *client, BtlvCore *viewCore);
void func_ov167_021b1934(BtlClient *client, u32 arg1);
void func_ov167_021b1960(BtlClient *client);
u32 func_ov167_021b19a4(BtlClient *client);
void func_ov167_021b19b0(BtlClient *client, u8 value);
BOOL func_ov167_021b1d64(BtlClient *client);
BOOL func_ov167_021b1d90(BtlClient *client);

BtlClient *BattleClient_Create(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 commMode, void *netHandle,
                               u16 clientId, u16 numCoverPos, u8 isAI, u32 arg7, BOOL recPlay, MATHRandContext32 *rand,
                               HeapID heapId);
void BattleClient_Delete(BtlClient *client);
void func_ov167_021b18e8(BtlClient *client, void *data);
void func_ov167_021b1910(BtlClient *client, BtlServer *server);
BtlAdapter *func_ov167_021b1928(BtlClient *client);
BOOL func_ov167_021b192c(BtlClient *client);
void func_ov167_021b1d58(BtlClient *client, BtlClientIDList *list);

BOOL func_ov167_021b1978(BtlClient *client);
u16 func_ov167_021b198c(BtlClient *client);
BOOL func_ov167_021b1990(BtlClient *client);
u8 BattleClient_GetClientId(BtlClient *client);
BattleParty *BattleClient_GetParty(BtlClient *client);
u8 BattleClient_GetShooterEnergy(BtlClient *client);

u8 func_ov167_021b9188(BtlClient *client);
u16 func_ov167_021b9194(BtlClient *client);
u32 func_ov167_021b919c(BtlClient *client);

#endif // POKEBW2_BATTLE_BTL_CLIENT_H
