#include "types.h"
#include "battle/btl_action.h"
#include "battle/btl_adapter.h"
#include "battle/btl_calc.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_rec.h"
#include "battle/btl_server.h"
#include "battle/btl_server_flow.h"
#include "constants/pokemon.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "pml/poke_party.h"
#include "save/join_avenue.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"

BtlServer *BattleServer_Create(BtlMainModule *mainModule, const MATHRandContext32 *rand, BtlPokeCon *pokeCon, u32 a3,
                               HeapID heapId) {
    BtlServer *server = GFL_HeapAllocate(heapId, sizeof(BtlServer), TRUE, "btl_server.c", 0xae);
    int i;

    server->mainModule = mainModule;
    server->pokeCon = pokeCon;
    server->heapId = heapId;
    server->queuePtr = &server->queue;
    server->state = 0;
    server->flow = NULL;
    server->count = 0;
    server->unkCBF = 0;
    server->unk4C = a3;
    server->rand = *rand;
    server->strBuf = GFL_StrBufCreate(0x20, heapId);
    BtlCalc_ResetRand(rand);
    for (i = 0; i < 4; i++) {
        ServerClient_Clear(&server->clients[i]);
    }
    server->flow = BtlServerFlow_Create(server, server->mainModule, server->pokeCon, &server->queue, server->unk4C,
                                        server->heapId);
    server->clientIdList = BtlServerFlow_GetClientIDList(server->flow);
    BattleServer_SetProc(server, BattleServer_ProcStart);
    return server;
}

void BattleServer_AttachClient(BtlServer *server, BtlAdapter *adapter, u8 clientId, u8 numCoverPos) {
    BattleParty *party = GetPartyData(server->pokeCon, clientId);

    ServerClient_Attach(&server->clients[clientId], clientId, adapter, TRUE);
    ServerClient_SetParty(&server->clients[clientId], party, numCoverPos);
}

void BattleServer_AttachNetClient(BtlServer *server, u32 mode, void *netHandle, u8 clientId, u8 numCoverPos) {
    BattleParty *party = GetPartyData(server->pokeCon, clientId);
    BtlAdapter *adapter = BtlAdapter_Create(netHandle, clientId, TRUE, server->heapId);

    ServerClient_Attach(&server->clients[clientId], clientId, adapter, FALSE);
    ServerClient_SetParty(&server->clients[clientId], party, numCoverPos);
}

void BattleServer_AttachCmdCheckClient(BtlServer *server, u8 clientId, u8 numCoverPos) {
    BattleParty *party = GetPartyData(server->pokeCon, clientId);

    ServerClient_Attach(&server->clients[clientId], clientId, NULL, FALSE);
    ServerClient_SetParty(&server->clients[clientId], party, numCoverPos);
}

void BattleServer_Start(BtlServer *server) {
    BattleServer_SetProc(server, BattleServer_ProcStart);
    BattleServer_ResetAdapters(server);
    BtlServerFlow_ResetSystem(server->flow);
}

void BattleServer_Delete(BtlServer *server) {
    int i;

    GFL_StrBufFree(server->strBuf);
    BtlServerFlow_Delete(server->flow);
    for (i = 0; i < 4; i++) {
        if (server->clients[i].clientId != 0xff && server->clients[i].adapter != NULL &&
            !server->clients[i].isAttached) {
            BtlAdapter_Delete(server->clients[i].adapter);
        }
    }
    GFL_HeapFree(server);
}

BOOL BattleServer_Main(BtlServer *server) {
    if (server->state != 2) {
        if (server->proc(server, &server->seq)) {
            BattleServer_BroadcastCmdData(server, BTL_ACMD_QUIT, server->clientIdList, sizeof(BtlClientIDList));
            server->state = 2;
        }
    } else if (BattleServer_AllClientsReplied(server)) {
        return TRUE;
    }
    return FALSE;
}

void BattleServer_GetClientIDList(BtlServer *server, BtlClientIDList *out) {
    *out = *server->clientIdList;
}

void BattleServer_SetProc(BtlServer *server, BtlServerProc proc) {
    server->proc = proc;
    server->seq = 0;
}

void BattleServer_SetSelectActionProc(BtlServer *server) {
    BattleServer_SetProc(server, BattleServer_ProcSelectAction);
}

BOOL BattleServer_ProcStart(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        BattleServer_BroadcastCmd(server, BTL_ACMD_SETUP_VIEW);
        (*seq)++;
    case 1:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            if (BtlSetup_GetBattleType(server->mainModule) != 4) {
                BattleServer_SendRecStartChunk(server, BtlServerFlow_StartBattle(server->flow));
                (*seq)++;
            } else {
                BattleServer_SetProc(server, BattleServer_ProcBattleEnd);
            }
        }
        break;
    case 2:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            (*seq)++;
        }
        break;
    case 3:
        BattleServer_BroadcastCmdData(server, BTL_ACMD_SERVER_CMD, server->queuePtr->buffer,
                                      server->queuePtr->writePtr);
        (*seq)++;
        break;
    case 4:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            BattleServer_SetSelectActionProc(server);
        }
        break;
    }
    return FALSE;
}

BOOL BattleServer_ProcSelectAction(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        if (func_ov167_0219fe24(server->flow)) {
            BattleServer_BroadcastCmdData(server, BTL_ACMD_SERVER_CMD, server->queuePtr->buffer,
                                          server->queuePtr->writePtr);
            (*seq)++;
        } else {
            *seq += 2;
        }
        break;
    case 1:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            (*seq)++;
        }
        break;
    case 2:
        BattleServer_BroadcastCmd(server, BTL_ACMD_SELECT_ACTION);
        (*seq)++;
        break;
    case 3:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            if (IsGameTimeUp(server->mainModule)) {
                BattleServer_SetProc(server, BattleServer_ProcTimeUp);
            } else if (func_ov167_0219df50(server->mainModule)) {
                BattleServer_SetProc(server, BattleServer_ProcBattleEnd);
            } else {
                BattleServer_StoreClientActions(server);
                if (BattleServer_SendRecActionsChunk(server, 1, BattleServer_IsActionChapter(server))) {
                    (*seq)++;
                } else {
                    *seq += 2;
                }
            }
        }
        break;
    case 4:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            (*seq)++;
        }
        break;
    case 5:
        BtlServerFlow_PrepareTurn(server->flow);
        BattleServer_FinishCmd(server);
        (*seq)++;
    case 6:
        server->flowResult = BtlServerFlow_RunTurn(server->flow, &server->clientActions);
        if (GetCommMode(server->mainModule)) {
            func_ov167_0219ef70(&server->clientActions);
        }
        BattleServer_BroadcastCmdData(server, BTL_ACMD_SERVER_CMD, server->queuePtr->buffer,
                                      server->queuePtr->writePtr);
        (*seq)++;
        break;
    case 7:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            switch (server->flowResult) {
            case 0:
                BattleServer_SetSelectActionProc(server);
                break;
            case 3:
                *seq = 6;
                break;
            case 2: {
                u32 battleType = BtlSetup_GetBattleType(server->mainModule);
                u32 battleStyle = BtlSetup_GetBattleStyle(server->mainModule);

                if (battleType == 0 && battleStyle == 0) {
                    BattleServer_SetProc(server, BattleServer_ProcAskEscape);
                } else {
                    BattleServer_SetProc(server, BattleServer_ProcChangePokemonEndTurn);
                }
                break;
            }
            case 1:
                BattleServer_SetProc(server, BattleServer_ProcChangePokemonMidTurn);
                break;
            case 6:
                func_ov167_0219c9fc(server->mainModule, func_ov167_0219fff0(server->flow));
                BattleServer_SetProc(server, BattleServer_ProcBattleEnd);
                return FALSE;
            case 4:
                SetBattleResult(server->mainModule, BtlServerFlow_GetBattleResult(server->flow));
                BattleServer_SetProc(server, BattleServer_ProcBattleEnd);
                return FALSE;
            case 5:
            default:
                return TRUE;
            }
        }
        break;
    }
    return FALSE;
}

BOOL BattleServer_IsActionChapter(BtlServer *server) {
    return TRUE;
}

BOOL BattleServer_ProcAskEscape(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        BattleServer_BroadcastCmd(server, BTL_ACMD_CONFIRM_NEXT_POKE);
        (*seq)++;
        break;
    case 1:
        if (BattleServer_AllClientsReplied(server)) {
            u8 clientId = GetPlayerClientID(server->mainModule);
            u8 *reply;

            BattleServer_FinishCmd(server);
            reply = BtlAdapter_GetReturnData(server->clients[clientId].adapter, NULL);
            if (*reply == 0) {
                BattleServer_SetProc(server, BattleServer_ProcChangePokemonEndTurn);
                break;
            }
            server->flowResult = BtlServerFlow_TryEscape(server->flow);
            BattleServer_BroadcastCmdData(server, BTL_ACMD_SERVER_CMD, server->queuePtr->buffer,
                                          server->queuePtr->writePtr);
            (*seq)++;
        }
        break;
    case 2:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            if (server->flowResult) {
                return TRUE;
            }
            BattleServer_SetProc(server, BattleServer_ProcChangePokemonEndTurn);
        }
        break;
    }
    return FALSE;
}

BOOL BattleServer_ProcChangePokemonEndTurn(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        BattleServer_BroadcastCmdData(server, BTL_ACMD_SELECT_CHANGE_FAINTED, server->posList, server->count);
        (*seq)++;
        break;
    case 1:
        if (BattleServer_AllClientsReplied(server)) {
            if (DoesSwitchModeNeedConfirming(server)) {
                server->switchModeMonId = GetNextEnemyForSwitchMode(server);
                BattleServer_SendCmdToClient(server, BTL_ACMD_CONFIRM_SHIFT_SWITCH, 0, &server->switchModeMonId, 1);
            }
            (*seq)++;
        }
        break;
    case 2:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            if (func_ov167_0219df50(server->mainModule)) {
                BattleServer_SetProc(server, BattleServer_ProcBattleEnd);
            } else {
                BattleServer_StoreClientActions(server);
                if (BattleServer_SendRecActionsChunk(server, 2, FALSE)) {
                    (*seq)++;
                } else {
                    *seq += 2;
                }
            }
        }
        break;
    case 3:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            (*seq)++;
        }
        break;
    case 4:
        BattleServer_FinishCmd(server);
        BtlServerFlow_PrepareFaintSwitch(server->flow);
        (*seq)++;
    case 5:
        server->flowResult = BtlServerFlow_RunFaintSwitch(server->flow, &server->clientActions);
        BattleServer_BroadcastCmdData(server, BTL_ACMD_SERVER_CMD, server->queuePtr->buffer,
                                      server->queuePtr->writePtr);
        (*seq)++;
        break;
    case 6:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            switch (server->flowResult) {
            case 2:
                *seq = 0;
                break;
            case 3:
                *seq = 5;
                break;
            case 4:
                SetBattleResult(server->mainModule, BtlServerFlow_GetBattleResult(server->flow));
                BattleServer_SetProc(server, BattleServer_ProcBattleEnd);
                break;
            case 5:
                return TRUE;
            default:
                BattleServer_SetSelectActionProc(server);
                break;
            }
        }
        break;
    }
    return FALSE;
}

// Function names from swan.
BOOL DoesSwitchModeNeedConfirming(BtlServer *server) {
    u32 i;
    BattleParty *party;

    if (IsSwitchMode(server->mainModule) && server->count != 0) {
        for (i = 0; i < server->count; i++) {
            if (GetClientIDFromPos(server->mainModule, server->posList[i]) == 0) {
                return FALSE;
            }
        }
        party = GetPartyData(server->pokeCon, GetPlayerClientID(server->mainModule));
        if (GetAlivePartyCount(party) < 2) {
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

u8 GetNextEnemyForSwitchMode(BtlServer *server) {
    BtlServerClient *client;
    BattleAction *action;
    u8 slot;

    client = &server->clients[1];
    if (IsSwitchModeEnabled(client)) {
        action = BtlAdapter_GetReturnData(client->adapter, NULL);
        if (BattleAction_GetAction(action) == 3) {
            slot = action->change.slot;
            return GetMonID(GetClientMonData(server->pokeCon, 1, slot));
        }
    }
    return 31;
}

BOOL BattleServer_ProcChangePokemonMidTurn(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        BattleServer_BroadcastCmdData(server, BTL_ACMD_SELECT_CHANGE_MID_TURN, server->posList, server->count);
        (*seq)++;
        break;
    case 1:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            if (func_ov167_0219df50(server->mainModule)) {
                BattleServer_SetProc(server, BattleServer_ProcBattleEnd);
            } else {
                BattleServer_StoreClientActions(server);
                if (BattleServer_SendRecActionsChunk(server, 3, FALSE)) {
                    (*seq)++;
                } else {
                    *seq += 2;
                }
            }
        }
        break;
    case 2:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            (*seq)++;
        }
        break;
    case 3:
        BattleServer_FinishCmd(server);
        BtlServerFlow_PrepareMidTurnSwitch(server->flow);
        (*seq)++;
    case 4:
        BtlServerCmdQueue_Init(server->queuePtr);
        server->flowResult = BtlServerFlow_RunMidTurnSwitch(server->flow, &server->clientActions);
        BattleServer_BroadcastCmdData(server, BTL_ACMD_SERVER_CMD, server->queuePtr->buffer,
                                      server->queuePtr->writePtr);
        (*seq)++;
        break;
    case 5:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            switch (server->flowResult) {
            case 1:
                *seq = 0;
                break;
            case 3:
                *seq = 4;
                break;
            case 2:
                BattleServer_SetProc(server, BattleServer_ProcChangePokemonEndTurn);
                break;
            case 4:
                SetBattleResult(server->mainModule, BtlServerFlow_GetBattleResult(server->flow));
                BattleServer_SetProc(server, BattleServer_ProcBattleEnd);
                break;
            case 5:
                return TRUE;
            default:
                BattleServer_SetSelectActionProc(server);
                break;
            }
        }
        break;
    }
    return FALSE;
}

BOOL BattleServer_ProcTimeUp(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0: {
        u32 size;
        void *data = BtlRecTool_MakeTimeoutChunk(&server->recTool, &size);

        BattleServer_BroadcastCmdData(server, BTL_ACMD_STORE_RECORD, data, size);
        (*seq)++;
        break;
    }
    case 1:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            BattleServer_BroadcastCmd(server, BTL_ACMD_TIME_UP);
            (*seq)++;
        }
        break;
    case 2:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            SetBattleResult(server->mainModule, BtlServerFlow_GetBattleResult(server->flow));
            BattleServer_SetProc(server, BattleServer_ProcBattleEnd);
        }
        break;
    }
    return FALSE;
}

BOOL BattleServer_ProcBattleEnd(BtlServer *server, int *seq) {
    u32 result = GetBattleResult(server->mainModule);

    server->exitData.result = result;
    server->exitData.playerClientId = GetPlayerClientID(server->mainModule);
    switch (BtlSetup_GetBattleType(server->mainModule)) {
    case 1:
        BattleServer_SetProc(server, BattleServer_ProcTrainerEnd);
        break;
    case 2:
        BattleServer_SetProc(server, BattleServer_ProcFacilityEnd);
        break;
    case 3:
        BattleServer_SetProc(server, BattleServer_ProcCommEnd);
        break;
    case 0:
        if (result == 1) {
            BattleServer_SetProc(server, BattleServer_ProcWildWin);
        } else if (result == 0 || result == 2) {
            BattleServer_SetProc(server, BattleServer_ProcWildLose);
        } else {
            BattleServer_SetProc(server, BattleServer_ProcWaitExit);
        }
        break;
    default:
        if (result == 1) {
            GFL_SndBGMPlay(GetWinBGM(server->mainModule), 0xffff);
        }
        BattleServer_SetProc(server, BattleServer_ProcWaitExit);
        break;
    }
    return FALSE;
}

BOOL BattleServer_ProcWaitExit(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0: {
        BOOL done = FALSE;

        if (!IsRecPlayMode(server->mainModule)) {
            if ((GCTX_HIDGetPressedKeys() & 3) || GFL_HIDIsTouchPressed()) {
                done = TRUE;
            }
        } else {
            done = TRUE;
        }
        if (done) {
            (*seq)++;
            return TRUE;
        }
        break;
    }
    case 1:
        return TRUE;
    }
    return FALSE;
}

BOOL BattleServer_ProcWildWin(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        BattleServer_BroadcastCmdData(server, BTL_ACMD_EXIT_WILD_WIN, &server->exitData, sizeof(server->exitData));
        (*seq)++;
        break;
    case 1:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            (*seq)++;
        }
        break;
    case 2:
        BattleServer_SetProc(server, BattleServer_ProcWaitExit);
        break;
    }
    return FALSE;
}

BOOL BattleServer_ProcWildLose(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        BattleServer_BroadcastCmdData(server, BTL_ACMD_EXIT_WILD_LOSE, &server->exitData, sizeof(server->exitData));
        (*seq)++;
        break;
    case 1:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            (*seq)++;
        }
        break;
    case 2:
        BattleServer_SetProc(server, BattleServer_ProcWaitExit);
        break;
    }
    return FALSE;
}

BOOL BattleServer_ProcCommEnd(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        BattleServer_BroadcastCmdData(server, BTL_ACMD_EXIT_COMM, &server->exitData, sizeof(server->exitData));
        (*seq)++;
        break;
    case 1:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            (*seq)++;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

BOOL BattleServer_ProcTrainerEnd(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        BattleServer_BroadcastCmdData(server, BTL_ACMD_EXIT_TRAINER, &server->exitData, sizeof(server->exitData));
        (*seq)++;
        break;
    case 1:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            (*seq)++;
        }
        break;
    case 2:
        BattleServer_SetProc(server, BattleServer_ProcWaitExit);
        break;
    }
    return FALSE;
}

BOOL BattleServer_ProcFacilityEnd(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        BattleServer_BroadcastCmdData(server, BTL_ACMD_EXIT_FACILITY, &server->exitData, sizeof(server->exitData));
        (*seq)++;
        break;
    case 1:
        if (BattleServer_AllClientsReplied(server)) {
            BattleServer_FinishCmd(server);
            (*seq)++;
        }
        break;
    case 2:
        if (!GetCommMode(server->mainModule)) {
            BattleServer_SetProc(server, BattleServer_ProcWaitExit);
            break;
        }
        return TRUE;
    }
    return FALSE;
}

// Reads the actions of a recorded turn
void BattleServer_ReadRecActions(BtlServer *server, const void *data, u32 size) {
    BattleAction actions[3];
    u32 pos;
    u8 clientId;
    u8 count;
    u8 i;

    pos = 0;
    for (i = 0; i < 4; i++) {
        server->clientActions.count[i] = 0;
    }
    BtlRecTool_SetData(&server->recTool, data, size);
    while (BtlRecTool_ReadClientActions(&server->recTool, &pos, &clientId, &count, actions)) {
        for (i = 0; i < count; i++) {
            server->clientActions.actions[clientId][i] = actions[i];
        }
        server->clientActions.count[clientId] = count;
    }
}

void func_ov167_0219ef70(BtlClientActions *clientActions) {
}

// Whether running the step mode writes commands different from data
BOOL BattleServer_CheckCmdMismatch(BtlServer *server, u32 mode, const void *data, u32 size) {
    BtlServerCmdQueue_Init(server->queuePtr);
    switch (mode) {
    case 4:
        BtlServerFlow_StartBattle(server->flow);
        break;
    case 1:
        BtlServerFlow_PrepareTurn(server->flow);
        BtlServerFlow_RunTurn(server->flow, &server->clientActions);
        break;
    case 2:
        BtlServerFlow_PrepareFaintSwitch(server->flow);
        BtlServerFlow_RunFaintSwitch(server->flow, &server->clientActions);
        break;
    case 3:
        BtlServerFlow_PrepareMidTurnSwitch(server->flow);
        BtlServerFlow_RunMidTurnSwitch(server->flow, &server->clientActions);
        break;
    }
    if (server->queuePtr->writePtr == size && GFL_STD_MemCmp(server->queuePtr->buffer, data, size) == 0) {
        return FALSE;
    }
    return TRUE;
}

BOOL BattleServer_SendRecStartChunk(BtlServer *server, BOOL value) {
    u32 size;
    void *data;

    BtlRecTool_Init(&server->recTool, TRUE);
    data = BtlRecTool_MakeTurnChunk(&server->recTool, &size, value);
    BattleServer_BroadcastCmdData(server, BTL_ACMD_STORE_RECORD, data, size);
    return TRUE;
}

BOOL BattleServer_SendRecActionsChunk(BtlServer *server, u8 value, BOOL flag) {
    u32 size;
    void *data = BattleServer_MakeRecActionsChunk(server, value, flag, &size);

    if (data != NULL) {
        BattleServer_BroadcastCmdData(server, BTL_ACMD_STORE_RECORD, data, size);
        return TRUE;
    }
    return FALSE;
}

void *BattleServer_MakeRecActionsChunk(BtlServer *server, u8 value, BOOL flag, u32 *size) {
    u32 i;

    BtlRecTool_Init(&server->recTool, flag);
    for (i = 0; i < 4; i++) {
        if (server->clients[i].clientId != 0xff) {
            u32 actionSize;
            void *actions = BtlAdapter_GetReturnData(server->clients[i].adapter, &actionSize);

            BtlRecTool_AddClientActions(&server->recTool, i, actions, actionSize / sizeof(BattleAction));
        }
    }
    return BtlRecTool_FinishChunk(&server->recTool, value, size);
}

void BattleServer_StoreClientActions(BtlServer *server) {
    u32 i;

    for (i = 0; i < 4; i++) {
        BtlServerClient *client = &server->clients[i];

        server->clientActions.count[i] = 0;
        if (IsSwitchModeEnabled(client)) {
            u32 size;
            BattleAction *actions = BtlAdapter_GetReturnData(client->adapter, &size);
            u32 j;
            u32 count = size / sizeof(BattleAction);

            for (j = 0; j < count; j++) {
                server->clientActions.actions[i][j] = *actions++;
                server->clientActions.count[i]++;
            }
        }
    }
}

void BattleServer_BroadcastCmd(BtlServer *server, u32 cmd) {
    BattleServer_BroadcastCmdData(server, cmd, NULL, 0);
}

void BattleServer_BroadcastCmdData(BtlServer *server, u32 cmd, const void *data, u32 size) {
    int i;

    BtlAdapter_ClearNetReplies();
    for (i = 0; i < 4; i++) {
        if (IsSwitchModeEnabled(&server->clients[i])) {
            BtlAdapter_SetCmd(server->clients[i].adapter, cmd, data, size);
        }
    }
    func_ov167_021d4ae8();
}

void BattleServer_SendCmdToClient(BtlServer *server, u32 cmd, u8 clientId, const void *data, u32 size) {
    BtlServerClient *client = &server->clients[clientId];

    if (IsSwitchModeEnabled(client)) {
        BtlAdapter_SetIdle(client->adapter);
        BtlAdapter_SetCmd(client->adapter, cmd, data, size);
    }
}

BOOL BattleServer_AllClientsReplied(BtlServer *server) {
    BOOL done = TRUE;
    int i;

    for (i = 0; i < 4; i++) {
        if (IsSwitchModeEnabled(&server->clients[i]) && !BtlAdapter_WaitCmd(server->clients[i].adapter)) {
            done = FALSE;
        }
    }
    return done;
}

void BattleServer_FinishCmd(BtlServer *server) {
    int i;

    for (i = 0; i < 4; i++) {
        if (IsSwitchModeEnabled(&server->clients[i])) {
            BtlAdapter_SetIdle(server->clients[i].adapter);
        }
    }
}

void BattleServer_ResetAdapters(BtlServer *server) {
    int i;

    for (i = 0; i < 4; i++) {
        if (IsSwitchModeEnabled(&server->clients[i])) {
            BtlAdapter_ResetState(server->clients[i].adapter);
        }
    }
}

void ServerClient_Clear(BtlServerClient *client) {
    client->clientId = 0xff;
}

// Function name from swan.
BOOL IsSwitchModeEnabled(BtlServerClient *client) {
    if (client->clientId != 0xff) {
        return TRUE;
    }
    return FALSE;
}

void ServerClient_Attach(BtlServerClient *client, u8 clientId, BtlAdapter *adapter, u8 isAttached) {
    client->clientId = clientId;
    client->adapter = adapter;
    client->isAttached = isAttached;
}

void ServerClient_SetParty(BtlServerClient *client, BattleParty *party, u8 numCoverPos) {
    client->party = party;
    client->numMons = GetNumMonsInParty(party);
    client->numCoverPos = numCoverPos;
}

BtlServerClient *BattleServer_GetClient(BtlServer *server, u8 clientId) {
    if (clientId < 4 && server->clients[clientId].clientId != 0xff) {
        return &server->clients[clientId];
    }
    return NULL;
}

BtlServerClient *BattleServer_GetClientNoCheck(BtlServer *server, u8 clientId) {
    if (server->clients[clientId].clientId != 0xff) {
        return &server->clients[clientId];
    }
    return NULL;
}

BOOL BattleServer_IsClientActive(BtlServer *server, u8 clientId) {
    if (clientId < 4) {
        return IsSwitchModeEnabled(&server->clients[clientId]);
    }
    return FALSE;
}

u8 ClientActions_GetCount(const BtlClientActions *clientActions, u8 clientId) {
    return clientActions->count[clientId];
}

BattleAction ClientActions_GetAction(const BtlClientActions *clientActions, u8 clientId, u8 index) {
    return clientActions->actions[clientId][index];
}

void BattleServer_SendNicknameBeacon(BtlServer *server, BattleMon *mon) {
    PokeParty_GetParam(GetSrcData(mon), PKM_PARAM_NICKNAME, server->strBuf);
    GameBeaconSys_SendNickname(server->strBuf);
}

void BattleServer_NotifyCapture(BtlServer *server, BattleMon *mon) {
    u32 species = PokeParty_GetParam(GetSrcData(mon), PKM_PARAM_SPECIES, NULL);
    BOOL a = BtlSetup_IsBattleType(server->mainModule, 0x4000);
    BOOL b = BtlSetup_IsBattleType(server->mainModule, 0x8000);

    GameBeaconSys_SendCapture(species, a, b);
    IncRecord(server->mainModule, 7);
    IncRecord(server->mainModule, 0x54);
    JoinAvenue_RecordActivity(0x1f);
}

void BattleServer_AddPayDayMoney(BtlServer *server, u32 money) {
    AddBonusMoney(server->mainModule, money);
}

void BattleServer_SetDoublePrizeMoney(BtlServer *server) {
    SetMoneyDoubled(server->mainModule);
}

void BattleServer_ClearChangeRequests(BtlServer *server) {
    server->count = 0;
    server->unkCBF = 0;
}

// Function name from swan.
void RequestChangePokemon(BtlServer *server, u8 pos) {
    u32 i;

    for (i = 0; i < server->count; i++) {
        if (pos == server->posList[i]) {
            return;
        }
    }
    server->posList[server->count++] = pos;
}

BtlServerFlow *BattleServer_GetFlow(BtlServer *server) {
    return server->flow;
}
