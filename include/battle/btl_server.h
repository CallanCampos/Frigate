#ifndef POKEBW2_BATTLE_BTL_SERVER_H
#define POKEBW2_BATTLE_BTL_SERVER_H

// Overlay 167's btl_server.c: the battle server, which runs the turns with btl_server_flow.c and sends the commands to
// the clients through their adapters. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// where it has them

#include "types.h"
#include "battle/btl_action.h"
#include "battle/btl_calc.h"
#include "battle/btl_rec.h"
#include "battle/btl_server_cmd.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "struct_decls.h"

// A client the server sends commands to
typedef struct {
    BtlAdapter *adapter;
    BattleParty *party;
    u8 numMons;
    u8 numCoverPos;
    // Whether the adapter was given to the server, rather than made by it
    u8 isAttached;
    // 0xff for no client
    u8 clientId;
} BtlServerClient;

// The actions each client chose
typedef struct {
    BattleAction actions[4][3];
    u8 count[4];
} BtlClientActions;

// A step of the server, which returns TRUE when the battle ends
typedef BOOL (*BtlServerProc)(BtlServer *server, int *seq);

struct BtlServer {
    BtlServerProc proc;
    int seq;
    u32 unk08;
    BtlMainModule *mainModule;
    BtlPokeCon *pokeCon;
    BtlServerClient clients[4];
    BtlServerFlow *flow;
    u32 flowResult;
    u32 unk4C;
    MATHRandContext32 rand;
    BtlRecTool recTool;
    // Sent to the clients when the battle ends
    struct {
        u16 playerClientId;
        u16 result;
    } exitData;
    StrBuf *strBuf;
    BtlClientIDList *clientIdList;
    BtlClientActions clientActions;
    u8 unkE8[4];
    u8 switchModeMonId;
    u8 state;
    u8 unkEE[2];
    BtlServerCmdQueue queue;
    BtlServerCmdQueue *queuePtr;
    u8 posList[6];
    u8 count;
    u8 unkCBB[4];
    u8 unkCBF;
    u16 heapId;
    u8 unkCC2[2];
};

BtlServer *BattleServer_Create(BtlMainModule *mainModule, const MATHRandContext32 *rand, BtlPokeCon *pokeCon, u32 a3,
                               HeapID heapId);
void BattleServer_AttachClient(BtlServer *server, BtlAdapter *adapter, u8 clientId, u8 numCoverPos);
void BattleServer_AttachNetClient(BtlServer *server, u32 mode, NetHandle *netHandle, u8 clientId, u8 numCoverPos);
void BattleServer_AttachCmdCheckClient(BtlServer *server, u8 clientId, u8 numCoverPos);
void BattleServer_Start(BtlServer *server);
void BattleServer_Delete(BtlServer *server);
BOOL BattleServer_Main(BtlServer *server);
void BattleServer_GetClientIDList(BtlServer *server, BtlClientIDList *out);
void BattleServer_SetProc(BtlServer *server, BtlServerProc proc);
void BattleServer_SetSelectActionProc(BtlServer *server);
BOOL BattleServer_ProcStart(BtlServer *server, int *seq);
BOOL BattleServer_ProcSelectAction(BtlServer *server, int *seq);
BOOL BattleServer_IsActionChapter(BtlServer *server);
BOOL BattleServer_ProcAskEscape(BtlServer *server, int *seq);
BOOL BattleServer_ProcChangePokemonEndTurn(BtlServer *server, int *seq);
BOOL DoesSwitchModeNeedConfirming(BtlServer *server);
u8 GetNextEnemyForSwitchMode(BtlServer *server);
BOOL BattleServer_ProcChangePokemonMidTurn(BtlServer *server, int *seq);
BOOL BattleServer_ProcTimeUp(BtlServer *server, int *seq);
BOOL BattleServer_ProcBattleEnd(BtlServer *server, int *seq);
BOOL BattleServer_ProcWaitExit(BtlServer *server, int *seq);
BOOL BattleServer_ProcWildWin(BtlServer *server, int *seq);
BOOL BattleServer_ProcWildLose(BtlServer *server, int *seq);
BOOL BattleServer_ProcCommEnd(BtlServer *server, int *seq);
BOOL BattleServer_ProcTrainerEnd(BtlServer *server, int *seq);
BOOL BattleServer_ProcFacilityEnd(BtlServer *server, int *seq);
void BattleServer_ReadRecActions(BtlServer *server, const void *data, u32 size);
void BattleServer_NoOp(BtlClientActions *clientActions);
BOOL BattleServer_CheckCmdMismatch(BtlServer *server, u32 mode, const void *data, u32 size);
BOOL BattleServer_SendRecStartChunk(BtlServer *server, BOOL value);
BOOL BattleServer_SendRecActionsChunk(BtlServer *server, u8 value, BOOL flag);
void *BattleServer_MakeRecActionsChunk(BtlServer *server, u8 value, BOOL flag, u32 *size);
void BattleServer_StoreClientActions(BtlServer *server);
void BattleServer_BroadcastCmd(BtlServer *server, u32 cmd);
void BattleServer_BroadcastCmdData(BtlServer *server, u32 cmd, const void *data, u32 size);
void BattleServer_SendCmdToClient(BtlServer *server, u32 cmd, u8 clientId, const void *data, u32 size);
BOOL BattleServer_AllClientsReplied(BtlServer *server);
void BattleServer_FinishCmd(BtlServer *server);
void BattleServer_ResetAdapters(BtlServer *server);
void ServerClient_Clear(BtlServerClient *client);
BOOL IsSwitchModeEnabled(BtlServerClient *client);
void ServerClient_Attach(BtlServerClient *client, u8 clientId, BtlAdapter *adapter, u8 isAttached);
void ServerClient_SetParty(BtlServerClient *client, BattleParty *party, u8 numCoverPos);
BtlServerClient *BattleServer_GetClient(BtlServer *server, u8 clientId);
BtlServerClient *BattleServer_GetClientNoCheck(BtlServer *server, u8 clientId);
BOOL BattleServer_IsClientActive(BtlServer *server, u8 clientId);
u8 ClientActions_GetCount(const BtlClientActions *clientActions, u8 clientId);
BattleAction ClientActions_GetAction(const BtlClientActions *clientActions, u8 clientId, u8 index);
void BattleServer_SendNicknameBeacon(BtlServer *server, BattleMon *mon);
void BattleServer_NotifyCapture(BtlServer *server, BattleMon *mon);
void BattleServer_AddPayDayMoney(BtlServer *server, u32 money);
void BattleServer_SetDoublePrizeMoney(BtlServer *server);
void BattleServer_ClearChangeRequests(BtlServer *server);
void RequestChangePokemon(BtlServer *server, u8 pos);
BtlServerFlow *BattleServer_GetFlow(BtlServer *server);

#endif // POKEBW2_BATTLE_BTL_SERVER_H
