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

BtlServer *func_ov167_0219e3cc(BtlMainModule *mainModule, const MATHRandContext32 *rand, BtlPokeCon *pokeCon, u32 a3,
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
    func_ov167_021bd090(rand);
    for (i = 0; i < 4; i++) {
        func_ov167_0219f22c(&server->clients[i]);
    }
    server->flow =
        func_ov167_0219f390(server, server->mainModule, server->pokeCon, &server->queue, server->unk4C, server->heapId);
    server->clientIdList = func_ov167_0219ffe4(server->flow);
    func_ov167_0219e5f0(server, func_ov167_0219e608);
    return server;
}

void func_ov167_0219e498(BtlServer *server, BtlAdapter *adapter, u8 clientId, u8 numCoverPos) {
    BattleParty *party = GetPartyData(server->pokeCon, clientId);

    func_ov167_0219f244(&server->clients[clientId], clientId, adapter, TRUE);
    func_ov167_0219f24c(&server->clients[clientId], party, numCoverPos);
}

void func_ov167_0219e4d0(BtlServer *server, u32 mode, void *netHandle, u8 clientId, u8 numCoverPos) {
    BattleParty *party = GetPartyData(server->pokeCon, clientId);
    BtlAdapter *adapter = func_ov167_021d4a44(netHandle, clientId, TRUE, server->heapId);

    func_ov167_0219f244(&server->clients[clientId], clientId, adapter, FALSE);
    func_ov167_0219f24c(&server->clients[clientId], party, numCoverPos);
}

void func_ov167_0219e514(BtlServer *server, u8 clientId, u8 numCoverPos) {
    BattleParty *party = GetPartyData(server->pokeCon, clientId);

    func_ov167_0219f244(&server->clients[clientId], clientId, NULL, FALSE);
    func_ov167_0219f24c(&server->clients[clientId], party, numCoverPos);
}

void func_ov167_0219e544(BtlServer *server) {
    func_ov167_0219e5f0(server, func_ov167_0219e608);
    func_ov167_0219f200(server);
    func_ov167_0219f3f8(server->flow);
}

void func_ov167_0219e560(BtlServer *server) {
    int i;

    GFL_StrBufFree(server->strBuf);
    func_ov167_0219f570(server->flow);
    for (i = 0; i < 4; i++) {
        if (server->clients[i].clientId != 0xff && server->clients[i].adapter != NULL &&
            !server->clients[i].isAttached) {
            func_ov167_021d4acc(server->clients[i].adapter);
        }
    }
    GFL_HeapFree(server);
}

BOOL func_ov167_0219e5a0(BtlServer *server) {
    if (server->state != 2) {
        if (server->proc(server, &server->seq)) {
            func_ov167_0219f128(server, 9, server->clientIdList, sizeof(BtlClientIDList));
            server->state = 2;
        }
    } else if (func_ov167_0219f19c(server)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_0219e5e0(BtlServer *server, BtlClientIDList *out) {
    *out = *server->clientIdList;
}

void func_ov167_0219e5f0(BtlServer *server, BtlServerProc proc) {
    server->proc = proc;
    server->seq = 0;
}

void func_ov167_0219e5f8(BtlServer *server) {
    func_ov167_0219e5f0(server, func_ov167_0219e6b0);
}

BOOL func_ov167_0219e608(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        func_ov167_0219f11c(server, 1);
        (*seq)++;
    case 1:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            if (BtlSetup_GetBattleType(server->mainModule) != 4) {
                func_ov167_0219f000(server, func_ov167_0219f588(server->flow));
                (*seq)++;
            } else {
                func_ov167_0219e5f0(server, func_ov167_0219ec84);
            }
        }
        break;
    case 2:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            (*seq)++;
        }
        break;
    case 3:
        func_ov167_0219f128(server, 8, server->queuePtr->buffer, server->queuePtr->writePtr);
        (*seq)++;
        break;
    case 4:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            func_ov167_0219e5f8(server);
        }
        break;
    }
    return FALSE;
}

BOOL func_ov167_0219e6b0(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        if (func_ov167_0219fe24(server->flow)) {
            func_ov167_0219f128(server, 8, server->queuePtr->buffer, server->queuePtr->writePtr);
            (*seq)++;
        } else {
            *seq += 2;
        }
        break;
    case 1:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            (*seq)++;
        }
        break;
    case 2:
        func_ov167_0219f11c(server, 3);
        (*seq)++;
        break;
    case 3:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            if (func_ov167_0219df10(server->mainModule)) {
                func_ov167_0219e5f0(server, func_ov167_0219ec0c);
            } else if (func_ov167_0219df50(server->mainModule)) {
                func_ov167_0219e5f0(server, func_ov167_0219ec84);
            } else {
                func_ov167_0219f0ac(server);
                if (func_ov167_0219f02c(server, 1, func_ov167_0219e860(server))) {
                    (*seq)++;
                } else {
                    *seq += 2;
                }
            }
        }
        break;
    case 4:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            (*seq)++;
        }
        break;
    case 5:
        func_ov167_0219f65c(server->flow);
        func_ov167_0219f1d4(server);
        (*seq)++;
    case 6:
        server->flowResult = func_ov167_0219f66c(server->flow, &server->clientActions);
        if (func_ov167_0219bee4(server->mainModule)) {
            func_ov167_0219ef70(&server->clientActions);
        }
        func_ov167_0219f128(server, 8, server->queuePtr->buffer, server->queuePtr->writePtr);
        (*seq)++;
        break;
    case 7:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            switch (server->flowResult) {
            case 0:
                func_ov167_0219e5f8(server);
                break;
            case 3:
                *seq = 6;
                break;
            case 2: {
                u32 battleType = BtlSetup_GetBattleType(server->mainModule);
                u32 battleStyle = BtlSetup_GetBattleStyle(server->mainModule);

                if (battleType == 0 && battleStyle == 0) {
                    func_ov167_0219e5f0(server, func_ov167_0219e864);
                } else {
                    func_ov167_0219e5f0(server, func_ov167_0219e8fc);
                }
                break;
            }
            case 1:
                func_ov167_0219e5f0(server, func_ov167_0219eae4);
                break;
            case 6:
                func_ov167_0219c9fc(server->mainModule, func_ov167_0219fff0(server->flow));
                func_ov167_0219e5f0(server, func_ov167_0219ec84);
                return FALSE;
            case 4:
                func_ov167_0219ca48(server->mainModule, func_ov167_021ac018(server->flow));
                func_ov167_0219e5f0(server, func_ov167_0219ec84);
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

BOOL func_ov167_0219e860(BtlServer *server) {
    return TRUE;
}

BOOL func_ov167_0219e864(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        func_ov167_0219f11c(server, 7);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_0219f19c(server)) {
            u8 clientId = GetPlayerClientID(server->mainModule);
            u8 *reply;

            func_ov167_0219f1d4(server);
            reply = func_ov167_021d4b50(server->clients[clientId].adapter, NULL);
            if (*reply == 0) {
                func_ov167_0219e5f0(server, func_ov167_0219e8fc);
                break;
            }
            server->flowResult = func_ov167_0219fdf4(server->flow);
            func_ov167_0219f128(server, 8, server->queuePtr->buffer, server->queuePtr->writePtr);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            if (server->flowResult) {
                return TRUE;
            }
            func_ov167_0219e5f0(server, func_ov167_0219e8fc);
        }
        break;
    }
    return FALSE;
}

BOOL func_ov167_0219e8fc(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        func_ov167_0219f128(server, 5, server->posList, server->count);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_0219f19c(server)) {
            if (DoesSwitchModeNeedConfirming(server)) {
                server->switchModeMonId = GetNextEnemyForSwitchMode(server);
                func_ov167_0219f16c(server, 4, 0, &server->switchModeMonId, 1);
            }
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            if (func_ov167_0219df50(server->mainModule)) {
                func_ov167_0219e5f0(server, func_ov167_0219ec84);
            } else {
                func_ov167_0219f0ac(server);
                if (func_ov167_0219f02c(server, 2, FALSE)) {
                    (*seq)++;
                } else {
                    *seq += 2;
                }
            }
        }
        break;
    case 3:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            (*seq)++;
        }
        break;
    case 4:
        func_ov167_0219f1d4(server);
        func_ov167_0219f7a8(server->flow);
        (*seq)++;
    case 5:
        server->flowResult = func_ov167_0219f7b4(server->flow, &server->clientActions);
        func_ov167_0219f128(server, 8, server->queuePtr->buffer, server->queuePtr->writePtr);
        (*seq)++;
        break;
    case 6:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            switch (server->flowResult) {
            case 2:
                *seq = 0;
                break;
            case 3:
                *seq = 5;
                break;
            case 4:
                func_ov167_0219ca48(server->mainModule, func_ov167_021ac018(server->flow));
                func_ov167_0219e5f0(server, func_ov167_0219ec84);
                break;
            case 5:
                return TRUE;
            default:
                func_ov167_0219e5f8(server);
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
            if (func_ov167_0219c650(server->mainModule, server->posList[i]) == 0) {
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
        action = func_ov167_021d4b50(client->adapter, NULL);
        if (BattleAction_GetAction(action) == 3) {
            slot = action->change.slot;
            return GetMonID(GetClientMonData(server->pokeCon, 1, slot));
        }
    }
    return 31;
}

BOOL func_ov167_0219eae4(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        func_ov167_0219f128(server, 6, server->posList, server->count);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            if (func_ov167_0219df50(server->mainModule)) {
                func_ov167_0219e5f0(server, func_ov167_0219ec84);
            } else {
                func_ov167_0219f0ac(server);
                if (func_ov167_0219f02c(server, 3, FALSE)) {
                    (*seq)++;
                } else {
                    *seq += 2;
                }
            }
        }
        break;
    case 2:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            (*seq)++;
        }
        break;
    case 3:
        func_ov167_0219f1d4(server);
        func_ov167_0219f748(server->flow);
        (*seq)++;
    case 4:
        BtlServerCmdQueue_Init(server->queuePtr);
        server->flowResult = func_ov167_0219f754(server->flow, &server->clientActions);
        func_ov167_0219f128(server, 8, server->queuePtr->buffer, server->queuePtr->writePtr);
        (*seq)++;
        break;
    case 5:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            switch (server->flowResult) {
            case 1:
                *seq = 0;
                break;
            case 3:
                *seq = 4;
                break;
            case 2:
                func_ov167_0219e5f0(server, func_ov167_0219e8fc);
                break;
            case 4:
                func_ov167_0219ca48(server->mainModule, func_ov167_021ac018(server->flow));
                func_ov167_0219e5f0(server, func_ov167_0219ec84);
                break;
            case 5:
                return TRUE;
            default:
                func_ov167_0219e5f8(server);
                break;
            }
        }
        break;
    }
    return FALSE;
}

BOOL func_ov167_0219ec0c(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0: {
        u32 size;
        void *data = func_ov167_021d4990(&server->recTool, &size);

        func_ov167_0219f128(server, 10, data, size);
        (*seq)++;
        break;
    }
    case 1:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            func_ov167_0219f11c(server, 0x10);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            func_ov167_0219ca48(server->mainModule, func_ov167_021ac018(server->flow));
            func_ov167_0219e5f0(server, func_ov167_0219ec84);
        }
        break;
    }
    return FALSE;
}

BOOL func_ov167_0219ec84(BtlServer *server, int *seq) {
    u32 result = func_ov167_0219ca58(server->mainModule);

    server->exitData.result = result;
    server->exitData.playerClientId = GetPlayerClientID(server->mainModule);
    switch (BtlSetup_GetBattleType(server->mainModule)) {
    case 1:
        func_ov167_0219e5f0(server, func_ov167_0219ee40);
        break;
    case 2:
        func_ov167_0219e5f0(server, func_ov167_0219ee88);
        break;
    case 3:
        func_ov167_0219e5f0(server, func_ov167_0219edfc);
        break;
    case 0:
        if (result == 1) {
            func_ov167_0219e5f0(server, func_ov167_0219ed6c);
        } else if (result == 0 || result == 2) {
            func_ov167_0219e5f0(server, func_ov167_0219edb4);
        } else {
            func_ov167_0219e5f0(server, func_ov167_0219ed24);
        }
        break;
    default:
        if (result == 1) {
            GFL_SndBGMPlay(func_ov167_0219bf00(server->mainModule), 0xffff);
        }
        func_ov167_0219e5f0(server, func_ov167_0219ed24);
        break;
    }
    return FALSE;
}

BOOL func_ov167_0219ed24(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0: {
        BOOL done = FALSE;

        if (!func_ov167_0219c980(server->mainModule)) {
            if ((GCTX_HIDGetPressedKeys() & 3) || func_0203da48()) {
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

BOOL func_ov167_0219ed6c(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        func_ov167_0219f128(server, 0xe, &server->exitData, sizeof(server->exitData));
        (*seq)++;
        break;
    case 1:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            (*seq)++;
        }
        break;
    case 2:
        func_ov167_0219e5f0(server, func_ov167_0219ed24);
        break;
    }
    return FALSE;
}

BOOL func_ov167_0219edb4(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        func_ov167_0219f128(server, 0xf, &server->exitData, sizeof(server->exitData));
        (*seq)++;
        break;
    case 1:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            (*seq)++;
        }
        break;
    case 2:
        func_ov167_0219e5f0(server, func_ov167_0219ed24);
        break;
    }
    return FALSE;
}

BOOL func_ov167_0219edfc(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        func_ov167_0219f128(server, 0xd, &server->exitData, sizeof(server->exitData));
        (*seq)++;
        break;
    case 1:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            (*seq)++;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_0219ee40(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        func_ov167_0219f128(server, 0xb, &server->exitData, sizeof(server->exitData));
        (*seq)++;
        break;
    case 1:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            (*seq)++;
        }
        break;
    case 2:
        func_ov167_0219e5f0(server, func_ov167_0219ed24);
        break;
    }
    return FALSE;
}

BOOL func_ov167_0219ee88(BtlServer *server, int *seq) {
    switch (*seq) {
    case 0:
        func_ov167_0219f128(server, 0xc, &server->exitData, sizeof(server->exitData));
        (*seq)++;
        break;
    case 1:
        if (func_ov167_0219f19c(server)) {
            func_ov167_0219f1d4(server);
            (*seq)++;
        }
        break;
    case 2:
        if (!func_ov167_0219bee4(server->mainModule)) {
            func_ov167_0219e5f0(server, func_ov167_0219ed24);
            break;
        }
        return TRUE;
    }
    return FALSE;
}

// Reads the actions of a recorded turn
void func_ov167_0219eee4(BtlServer *server, const void *data, u32 size) {
    BattleAction actions[3];
    u32 pos;
    u8 clientId;
    u8 count;
    u8 i;

    pos = 0;
    for (i = 0; i < 4; i++) {
        server->clientActions.count[i] = 0;
    }
    func_ov167_021d49a0(&server->recTool, data, size);
    while (func_ov167_021d49d0(&server->recTool, &pos, &clientId, &count, actions)) {
        for (i = 0; i < count; i++) {
            server->clientActions.actions[clientId][i] = actions[i];
        }
        server->clientActions.count[clientId] = count;
    }
}

void func_ov167_0219ef70(BtlClientActions *clientActions) {
}

// Whether running the step mode writes commands different from data
BOOL func_ov167_0219ef74(BtlServer *server, u32 mode, const void *data, u32 size) {
    BtlServerCmdQueue_Init(server->queuePtr);
    switch (mode) {
    case 4:
        func_ov167_0219f588(server->flow);
        break;
    case 1:
        func_ov167_0219f65c(server->flow);
        func_ov167_0219f66c(server->flow, &server->clientActions);
        break;
    case 2:
        func_ov167_0219f7a8(server->flow);
        func_ov167_0219f7b4(server->flow, &server->clientActions);
        break;
    case 3:
        func_ov167_0219f748(server->flow);
        func_ov167_0219f754(server->flow, &server->clientActions);
        break;
    }
    if (server->queuePtr->writePtr == size && GFL_STD_MemCmp(server->queuePtr->buffer, data, size) == 0) {
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov167_0219f000(BtlServer *server, u8 value) {
    u32 size;
    void *data;

    func_ov167_021d48a0(&server->recTool, TRUE);
    data = func_ov167_021d48c4(&server->recTool, &size, value);
    func_ov167_0219f128(server, 10, data, size);
    return TRUE;
}

BOOL func_ov167_0219f02c(BtlServer *server, u8 value, BOOL flag) {
    u32 size;
    void *data = func_ov167_0219f054(server, value, flag, &size);

    if (data != NULL) {
        func_ov167_0219f128(server, 10, data, size);
        return TRUE;
    }
    return FALSE;
}

void *func_ov167_0219f054(BtlServer *server, u8 value, BOOL flag, u32 *size) {
    u32 i;

    func_ov167_021d48a0(&server->recTool, flag);
    for (i = 0; i < 4; i++) {
        if (server->clients[i].clientId != 0xff) {
            u32 actionSize;
            void *actions = func_ov167_021d4b50(server->clients[i].adapter, &actionSize);

            func_ov167_021d48e0(&server->recTool, i, actions, actionSize / sizeof(BattleAction));
        }
    }
    return func_ov167_021d4958(&server->recTool, value, size);
}

void func_ov167_0219f0ac(BtlServer *server) {
    u32 i;

    for (i = 0; i < 4; i++) {
        BtlServerClient *client = &server->clients[i];

        server->clientActions.count[i] = 0;
        if (IsSwitchModeEnabled(client)) {
            u32 size;
            BattleAction *actions = func_ov167_021d4b50(client->adapter, &size);
            u32 j;
            u32 count = size / sizeof(BattleAction);

            for (j = 0; j < count; j++) {
                server->clientActions.actions[i][j] = *actions++;
                server->clientActions.count[i]++;
            }
        }
    }
}

void func_ov167_0219f11c(BtlServer *server, u32 cmd) {
    func_ov167_0219f128(server, cmd, NULL, 0);
}

void func_ov167_0219f128(BtlServer *server, u32 cmd, const void *data, u32 size) {
    int i;

    func_ov167_021d4ad4();
    for (i = 0; i < 4; i++) {
        if (IsSwitchModeEnabled(&server->clients[i])) {
            func_ov167_021d4aec(server->clients[i].adapter, cmd, data, size);
        }
    }
    func_ov167_021d4ae8();
}

void func_ov167_0219f16c(BtlServer *server, u32 cmd, u8 clientId, const void *data, u32 size) {
    BtlServerClient *client = &server->clients[clientId];

    if (IsSwitchModeEnabled(client)) {
        func_ov167_021d4b5c(client->adapter);
        func_ov167_021d4aec(client->adapter, cmd, data, size);
    }
}

BOOL func_ov167_0219f19c(BtlServer *server) {
    BOOL done = TRUE;
    int i;

    for (i = 0; i < 4; i++) {
        if (IsSwitchModeEnabled(&server->clients[i]) && !func_ov167_021d4b18(server->clients[i].adapter)) {
            done = FALSE;
        }
    }
    return done;
}

void func_ov167_0219f1d4(BtlServer *server) {
    int i;

    for (i = 0; i < 4; i++) {
        if (IsSwitchModeEnabled(&server->clients[i])) {
            func_ov167_021d4b5c(server->clients[i].adapter);
        }
    }
}

void func_ov167_0219f200(BtlServer *server) {
    int i;

    for (i = 0; i < 4; i++) {
        if (IsSwitchModeEnabled(&server->clients[i])) {
            func_ov167_021d4abc(server->clients[i].adapter);
        }
    }
}

void func_ov167_0219f22c(BtlServerClient *client) {
    client->clientId = 0xff;
}

// Function name from swan.
BOOL IsSwitchModeEnabled(BtlServerClient *client) {
    if (client->clientId != 0xff) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_0219f244(BtlServerClient *client, u8 clientId, BtlAdapter *adapter, u8 isAttached) {
    client->clientId = clientId;
    client->adapter = adapter;
    client->isAttached = isAttached;
}

void func_ov167_0219f24c(BtlServerClient *client, BattleParty *party, u8 numCoverPos) {
    client->party = party;
    client->numMons = GetNumMonsInParty(party);
    client->numCoverPos = numCoverPos;
}

BtlServerClient *func_ov167_0219f260(BtlServer *server, u8 clientId) {
    if (clientId < 4 && server->clients[clientId].clientId != 0xff) {
        return &server->clients[clientId];
    }
    return NULL;
}

BtlServerClient *func_ov167_0219f27c(BtlServer *server, u8 clientId) {
    if (server->clients[clientId].clientId != 0xff) {
        return &server->clients[clientId];
    }
    return NULL;
}

BOOL func_ov167_0219f294(BtlServer *server, u8 clientId) {
    if (clientId < 4) {
        return IsSwitchModeEnabled(&server->clients[clientId]);
    }
    return FALSE;
}

u8 func_ov167_0219f2ac(const BtlClientActions *clientActions, u8 clientId) {
    return clientActions->count[clientId];
}

BattleAction func_ov167_0219f2b4(const BtlClientActions *clientActions, u8 clientId, u8 index) {
    return clientActions->actions[clientId][index];
}

void func_ov167_0219f2c0(BtlServer *server, BattleMon *mon) {
    PokeParty_GetParam(GetSrcData(mon), PKM_PARAM_NICKNAME, server->strBuf);
    func_0202d2c8(server->strBuf);
}

void func_ov167_0219f2e0(BtlServer *server, BattleMon *mon) {
    u32 species = PokeParty_GetParam(GetSrcData(mon), PKM_PARAM_SPECIES, NULL);
    BOOL a = BtlSetup_IsBattleType(server->mainModule, 0x4000);
    BOOL b = BtlSetup_IsBattleType(server->mainModule, 0x8000);

    GameBeaconSys_SendCapture(species, a, b);
    func_ov167_0219dad0(server->mainModule, 7);
    func_ov167_0219dad0(server->mainModule, 0x54);
    func_02038bc8(0x1f);
}

void func_ov167_0219f330(BtlServer *server, u32 money) {
    func_ov167_0219cac0(server->mainModule, money);
}

void func_ov167_0219f33c(BtlServer *server) {
    func_ov167_0219cb10(server->mainModule);
}

void func_ov167_0219f348(BtlServer *server) {
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

BtlServerFlow *func_ov167_0219f38c(BtlServer *server) {
    return server->flow;
}
