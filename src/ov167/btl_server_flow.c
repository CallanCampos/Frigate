#include "types.h"
#include "battle/btl_ability.h"
#include "battle/btl_action.h"
#include "battle/btl_action_order.h"
#include "battle/btl_display.h"
#include "battle/btl_event.h"
#include "battle/btl_faintrec.h"
#include "battle/btl_field.h"
#include "battle/btl_handler.h"
#include "battle/btl_handler_work.h"
#include "battle/btl_item.h"
#include "battle/btl_main.h"
#include "battle/btl_monset.h"
#include "battle/btl_move.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_pospoke.h"
#include "battle/btl_server.h"
#include "battle/btl_server_cmd.h"
#include "battle/btl_server_flow.h"
#include "battle/btl_server_flow_sub.h"
#include "battle/btl_tables.h"
#include "battle/btl_wazarec.h"
#include "battle/condition_effects.h"
#include "battle/pos_handlers.h"
#include "battle/side_handlers.h"
#include "constants/abilities.h"
#include "constants/battle.h"
#include "constants/moves.h"
#include "constants/tr_ai.h"
#include "constants/types.h"
#include "gfl/arc.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "pml/waza.h"
#include "system/comm_player_support.h"
#include "system/game_beacon.h"

s32 ConvertConditionCode(BattleMon *mon, s32 *condition);

// The targets hit by one strike of a damaging move, as ServerControl_DamageListCore works through them
static u32 sHitEffectiveness[3];
static BattleMon *sHitMons[3];
static u16 sHitDamages[3];
static u8 sHitUnk9[3];
static u8 sHitCritical[3];

BtlServerFlow *BtlServerFlow_Create(BtlServer *server, BtlMainModule *mainModule, BtlPokeCon *pokeCon,
                                    BtlServerCmdQueue *queue, u32 a4, HeapID heapId) {
    BtlServerFlow *flow;

    flow = GFL_HeapAllocate(heapId, sizeof(BtlServerFlow), TRUE, "btl_server_flow.c", 583);
    flow->server = server;
    flow->pokeCon = pokeCon;
    flow->mainModule = mainModule;
    flow->actionOrderCount = 0;
    flow->unk10 = 0;
    flow->unk14 = 0;
    flow->queue = queue;
    flow->heapId = heapId;
    flow->unk18 = a4;
    flow->unk4A4 = loadEvolutionFile(heapId);
    BtlServerFlow_InitWork(flow);
    return flow;
}

void BtlServerFlow_ResetSystem(BtlServerFlow *flow) {
    BtlServerFlow_InitWork(flow);
}

void BtlServerFlow_InitWork(BtlServerFlow *flow) {
    BattleEvent_Init();
    FieldStatusInit(0);
    BtlWazaRec_Init(&flow->wazaRec);
    BtlFaintRec_Init(&flow->faintRec);
    BtlPosPoke_Init(&flow->posPoke, flow->mainModule, flow->pokeCon, BtlSetup_GetBattleStyle(flow->mainModule));
    MI_CpuFill8(flow->unk7A9, 0, sizeof(flow->unk7A9));
    MI_CpuFill8(flow->unk7C1, 0, sizeof(flow->unk7C1));
    MI_CpuFill8(flow->unk7D9, 0, sizeof(flow->unk7D9));
    MI_CpuFill8(flow->unk4D4, 0, sizeof(flow->unk4D4));
    BtlServerFlow_ClearEffectivenessCounts(flow->unk1F80);
    BtlServerFlow_ResetFrames(flow);
    BtlClientIDList_Init(&flow->clientIdList);
    InitState(&flow->actionState);
    func_ov167_021a8f8c(&flow->unk1B54);
    SideEffectsInit();
    flow->unk786 = 0;
    flow->unk787 = 0;
    flow->unk78A_0 = 0;
    flow->unk78A_1 = 0;
    flow->unk78A_2 = 0;
    flow->unk77C = 0;
    flow->unk789 = 0x1f;
    flow->unk1F78 = 0;
    flow->unk784 = 6;
    flow->unk774 = 0;
    flow->unk77E = 0;
    flow->unk778 = 0;
    flow->unk78A_6 = 0;
}

void BtlServerFlow_Delete(BtlServerFlow *flow) {
    GFL_ArcToolFree(flow->unk4A4);
    GFL_HeapFree(flow);
}

BOOL BtlServerFlow_StartBattle(BtlServerFlow *flow) {
    BtlServerCmdQueue *queue;
    u8 weather;
    u32 clientId;
    BOOL result;
    BtlServerClient *client;
    u32 i;
    BattleMon *mon;

    queue = flow->queue;
    queue->writePtr = 0;
    result = FALSE;
    queue->readPtr = 0;
    weather = GetFieldEffectData(flow->mainModule)->env.weather;
    if (weather != 0 && ServerControl_ChangeWeather(flow, weather, 0xff)) {
        result = TRUE;
    }
    for (clientId = 0; clientId < 4; clientId++) {
        client = BattleServer_GetClientNoCheck(flow->server, clientId);
        if (client != NULL) {
            for (i = 0; i < client->numCoverPos; i++) {
                mon = BattleParty_GetMon(client->party, i);
                if (mon != NULL && !IsFainted(mon)) {
                    ServerControl_SwitchInCore(flow, clientId, i, i);
                }
            }
            if (BtlSetup_GetBattleStyle(flow->mainModule) == 3) {
                for (i = 0; i < 3; i++) {
                    mon = BattleParty_GetMon(client->party, i);
                    if (mon != NULL && !IsFainted(mon)) {
                        RegisterDexMon(flow->mainModule, clientId, mon);
                    }
                }
            }
        }
    }
    if (ServerControl_AfterSwitchIn(flow)) {
        result = TRUE;
    }
    return result;
}

void BtlServerFlow_PrepareTurn(BtlServerFlow *flow) {
    flow->unk77C = 0;
    flow->unk783 = 0;
}

u32 BtlServerFlow_RunTurn(BtlServerFlow *flow, BtlClientActions *clientActions) {
    u32 i;

    flow->unk14 = 0;
    BtlServerCmdQueue_Init(flow->queue);
    if (flow->unk77C == 0) {
        BtlServerFlow_ClearRevivedMons(flow);
        BtlFaintRec_NextTurn(&flow->faintRec);
        for (i = 0; i < 4; i++) {
            flow->unk1FEC[i] = 0;
        }
        BattleEvent_ClearTurnPause();
        BattleServer_ClearChangeRequests(flow->server);
        BattleEventVar_CheckReset();
        ServerControl_ClearRechargeFlags(flow);
        flow->actionOrderCount = ActionOrder_Build(flow, clientActions, flow->actionOrder, 6);
        flow->unk77C = 1;
    }
    flow->unk783 = ActionOrder_Run(flow, flow->unk783);
    return flow->unk14;
}

void ServerControl_ClearRechargeFlags(BtlServerFlow *flow) {
    BtlFlowMonIter iter;
    BattleMon *mon;

    BtlFlowMonIter_Init(&iter, flow);
    while (BtlFlowMonIter_Next(&iter, flow, &mon)) {
        if (GetAdditionalConditionFlag(mon, 0xc)) {
            scPut_ResetContFlag(flow, mon, 0xc);
        }
    }
}

void BtlServerFlow_PrepareMidTurnSwitch(BtlServerFlow *flow) {
    flow->unk77C = 0;
}

u32 BtlServerFlow_RunMidTurnSwitch(BtlServerFlow *flow, BtlClientActions *clientActions) {
    flow->unk14 = 0;
    BtlServerCmdQueue_Init(flow->queue);
    if (flow->unk77C == 0) {
        BattleServer_ClearChangeRequests(flow->server);
        BattleEventVar_CheckReset();
        flow->unk77D = 0;
        flow->unk77C = 1;
        if (ServerControl_MidTurnSwitchIn(flow, clientActions)) {
            flow->unk14 = 3;
            return flow->unk14;
        }
    }
    flow->unk783 = ActionOrder_Run(flow, flow->unk783);
    return flow->unk14;
}

void BtlServerFlow_PrepareFaintSwitch(BtlServerFlow *flow) {
    flow->unk77C = 0;
}

u32 BtlServerFlow_RunFaintSwitch(BtlServerFlow *flow, BtlClientActions *clientActions) {
    u32 i;
    u32 sideEffects;

    flow->unk14 = 0;
    BtlServerCmdQueue_Init(flow->queue);
    if (flow->unk77C == 0) {
        BattleServer_ClearChangeRequests(flow->server);
        BattleEventVar_CheckReset();
        sideEffects = BtlFaintRec_GetCount(&flow->faintRec, 0);
        flow->actionOrderCount = ActionOrder_Build(flow, clientActions, flow->actionOrder, 6);
        flow->unk783 = 0;
        for (i = 0; i < flow->actionOrderCount; i++) {
            if (flow->actionOrder[i].action.change.action == BATTLE_ACTION_CHANGE &&
                !flow->actionOrder[i].action.change.unk10 && !IsFainted(flow->actionOrder[i].mon)) {
                ServerControl_ChangePokemon(flow, flow->actionOrder[i].mon, flow->actionOrder[i].action.change.slot);
                flow->actionOrder[i].done = TRUE;
            }
        }
        for (i = 0; i < flow->actionOrderCount; i++) {
            if (flow->actionOrder[i].action.change.action == BATTLE_ACTION_CHANGE &&
                !flow->actionOrder[i].action.change.unk10 && IsFainted(flow->actionOrder[i].mon)) {
                ServerControl_SwitchInFillSlot(flow, flow->actionOrder[i].clientId,
                                               flow->actionOrder[i].action.change.unk4,
                                               flow->actionOrder[i].action.change.slot, TRUE);
                flow->actionOrder[i].done = TRUE;
            }
        }
        if (BtlSetup_GetBattleStyle(flow->mainModule) == 3) {
            for (i = 0; i < flow->actionOrderCount; i++) {
                if (flow->actionOrder[i].action.change.action == BATTLE_ACTION_ROTATION &&
                    !flow->actionOrder[i].action.change.unk10) {
                    ActionOrder_Proc(flow, &flow->actionOrder[i]);
                }
            }
        }
        flow->unk77C = 1;
    }
    ServerControl_AfterSwitchIn(flow);
    ServerControl_CheckExpGain(flow);
    ServerControl_RequestChangeForEmptyPos(flow, &flow->unk4CE);
    if (sideEffects == BtlFaintRec_GetCount(&flow->faintRec, 0)) {
        ServerControl_TripleBattleShiftToCenter(flow);
        return 0;
    }
    if (!ServerControl_CheckMatchup(flow)) {
        ServerControl_UpdateIllusion(flow, &flow->unk4CE);
        return 2;
    }
    return 4;
}

u32 ActionOrder_Run(BtlServerFlow *flow, u32 i) {
    u8 fainted;
    u32 prevAction;
    u32 action;
    u8 matchup;
    u8 switched;
    u32 sideEffects;

    prevAction = 0;
    for (; i < flow->actionOrderCount; i++) {
        action = BattleAction_GetAction(&flow->actionOrder[i].action);
        if (prevAction == 6 && action != 6) {
            ServerControl_RotationIn(flow);
            ActionOrder_RecalcAndSort(flow, &flow->actionOrder[i], flow->actionOrderCount - i);
        }
        prevAction = ActionOrder_Proc(flow, &flow->actionOrder[i]);
        fainted = ServerControl_CheckExpGain(flow);
        func_ov167_021bc6ac(GetPokeParam(flow->pokeCon, 0));
        matchup = ServerControl_CheckMatchup(flow);
        func_ov167_021bc6ac(GetPokeParam(flow->pokeCon, 0));
        if (matchup) {
            flow->unk14 = 4;
            return i + 1;
        }
        if (flow->unk14 == 6) {
            return i + 1;
        }
        if (flow->unk14 == 1) {
            return i + 1;
        }
        if (fainted) {
            func_ov167_021bc6ac(GetPokeParam(flow->pokeCon, 0));
            flow->unk14 = 3;
            return i + 1;
        }
    }
    if (flow->unk14 == 0) {
        switched = ServerControl_TurnCheck(flow);
        if ((u8)ServerControl_CheckMatchup(flow)) {
            flow->unk14 = 4;
            return flow->actionOrderCount;
        }
        if (switched) {
            flow->unk14 = 3;
            return flow->actionOrderCount;
        }
        sideEffects = BtlFaintRec_GetCount(&flow->faintRec, 0);
        if (BtlServerFlow_CheckRevivedSwitchIn(flow) || sideEffects) {
            ServerControl_RequestChangeForEmptyPos(flow, &flow->unk4CE);
            ServerControl_UpdateIllusion(flow, &flow->unk4CE);
            flow->unk14 = 2;
            return flow->actionOrderCount;
        }
        flow->unk14 = 0;
    }
    return flow->actionOrderCount;
}

static inline u32 ActionOrder_MakeKey(u16 speed, u8 unk13, u8 unk16, u8 unk22) {
    return ((unk22 & 7) << 22) | ((unk16 & 0x3f) << 16) | ((unk13 & 7) << 13) | (speed & 0x1fff);
}

// The move of an entry's fight action, 0 for any other action
static inline u16 ActionOrder_GetMove(const ActionOrderEntry *entry) {
    return BattleAction_GetMove(&entry->action);
}

void ActionOrder_RecalcAndSort(BtlServerFlow *flow, ActionOrderEntry *order, u32 count) {
    u32 i;
    u32 action;
    u16 move;
    u32 state;
    u32 unk16;
    u16 speed;

    for (i = 0; i < count; i++) {
        speed = ServerEvent_CalculateSpeed(flow, order[i].mon, TRUE);
        order[i].key = (order[i].key & 0xffffe000) | (speed & 0x1fff);
        action = BattleAction_GetAction(&order[i].action);
        if (action == 1) {
            move = ActionOrder_GetMove(&order[i]);
            state = PushState(&flow->actionState, 0x436);
            unk16 = ServerEvent_GetMovePriority(flow, move, order[i].mon);
            order[i].key =
                ActionOrder_MakeKey(order[i].key & 0x1fff, (order[i].key >> 13) & 7, unk16, (order[i].key >> 22) & 7);
            PopState(&flow->actionState, state, 0x439);
        }
        if (action == 1 || action == 5) {
            state = PushState(&flow->actionState, 0x441);
            order[i].key =
                (order[i].key & 0xffff1fff) | ((ServerEvent_CheckSpecialPriority(flow, order[i].mon) & 7) << 13);
            PopState(&flow->actionState, state, 0x444);
        }
    }
    ActionOrder_Sort(order, count);
}

BOOL ServerControl_MidTurnSwitchIn(BtlServerFlow *flow, BtlClientActions *clientActions) {
    u32 clientId;
    u32 count;
    u32 i;
    BattleAction action;

    for (clientId = 0; clientId < 4; clientId++) {
        if (BattleServer_GetClient(flow->server, clientId)) {
            count = ClientActions_GetCount(clientActions, clientId);
            for (i = 0; i < count; i++) {
                action = ClientActions_GetAction(clientActions, clientId, i);
                if (action.change.action == BATTLE_ACTION_CHANGE && !action.change.unk10) {
                    ServerControl_SwitchInFillSlot(flow, clientId, action.change.unk4, action.change.slot, TRUE);
                }
            }
        }
    }
    ServerControl_AfterSwitchIn(flow);
    return ServerControl_CheckExpGain(flow);
}

BOOL ServerControl_CheckMatchup(BtlServerFlow *flow) {
    u8 hasMons[2];
    u32 i;
    BattleParty *party;
    u8 side;
    u32 alive;

    hasMons[0] = hasMons[1] = 0;
    for (i = 0; i < 4; i++) {
        if (DoesClientExist(flow->mainModule, i)) {
            party = GetPartyData(flow->pokeCon, i);
            side = GetClientSide(flow->mainModule, i);
            alive = GetAlivePartyCount(party);
            if (i == 0) {
                func_ov167_021bc6ac(GetPokeParam(flow->pokeCon, 0));
            }
            if (alive != 0) {
                hasMons[side] = 1;
            }
        }
    }
    func_ov167_021bc6ac(GetPokeParam(flow->pokeCon, 0));
    if ((GetStudioMode(flow->mainModule) == 1 || GetStudioMode(flow->mainModule) == 2) &&
        CheckScriptedBattleRules(flow->mainModule, flow->pokeCon)) {
        return TRUE;
    }
    if (!hasMons[0] || !hasMons[1]) {
        return TRUE;
    }
    return FALSE;
}

BOOL ServerControl_PlayerSideHasMons(BtlServerFlow *flow) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (DoesClientExist(flow->mainModule, i) &&
            GetClientSide(flow->mainModule, i) ==
                GetClientSide(flow->mainModule, GetPlayerClientID(flow->mainModule)) &&
            GetAlivePartyCount(GetPartyData(flow->pokeCon, i)) != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

u32 BtlServerFlow_TryEscape(BtlServerFlow *flow) {
    BattleMon *mon;
    u32 result;

    mon = PokeCon_GetClientPartyMon(flow->pokeCon, GetPlayerClientID(flow->mainModule), 0);
    result = 0;
    BtlServerCmdQueue_Init(flow->queue);
    if (ServerControl_Escape(flow, mon)) {
        result = 1;
    }
    return result;
}

BOOL func_ov167_0219fe24(BtlServerFlow *flow) {
    BOOL result;

    result = FALSE;
    BtlServerCmdQueue_Init(flow->queue);
    func_ov167_0219fe44(flow);
    if (flow->queue->writePtr != 0) {
        result = TRUE;
    }
    return result;
}

void func_ov167_0219fe44(BtlServerFlow *flow) {
    u32 i;
    u32 count;
    u8 clientIds[4];

    if (flow->unk18 == 1) {
        for (i = 0; i < 4; i++) {
            if (BattleServer_IsClientActive(flow->server, i)) {
                if (BtlSetup_GetBattleStyle(flow->mainModule) != 2) {
                    count = 1;
                } else {
                    count = BtlPosPoke_GetEmptyPositions(&flow->posPoke, i, clientIds) + 1;
                }
                BtlServerCmdQueue_Put(flow->queue, SC_OP_ADD_SHOOTER_ENERGY, (u8)i, (u8)count);
            }
        }
    }
}

void ServerControl_Rotation(BtlServerFlow *flow, u8 clientId, u8 slot) {
    BattleParty *party;
    BattleMon *outMon;
    BattleMon *inMon;

    if (slot != 1 && slot != 0) {
        party = GetPartyData(flow->pokeCon, clientId);
        BtlServerCmdQueue_Put(flow->queue, SC_ROTATION, clientId, slot);
        BattleParty_Rotate(party, slot, &outMon, &inMon);
        if (!IsFainted(outMon)) {
            ServerControl_ClearMonDependentEffects(flow, outMon, TRUE);
        }
        if (!IsFainted(inMon)) {
            if (IsFieldEffectActive(2)) {
                if (CheckCondition(inMon, CONDITION_TELEKINESIS)) {
                    ServerControl_CureCondition(flow, inMon, CONDITION_TELEKINESIS, 0);
                }
                if (CheckCondition(inMon, CONDITION_MAGNET_RISE)) {
                    ServerControl_CureCondition(flow, inMon, CONDITION_MAGNET_RISE, 0);
                }
            }
            AbilityEvent_ItemRotationWake(inMon);
            ItemEvent_ItemRotationWake(inMon);
        }
        BtlPosPoke_Rotate(&flow->posPoke, slot, clientId, inMon, flow->pokeCon);
    }
}

BOOL ServerControl_RequestChangeForEmptyPos(BtlServerFlow *flow, BtlFlowClientList *list) {
    BOOL result;
    u8 clientId;
    u8 count;
    u8 i;
    u8 positions[4];

    result = FALSE;
    list->count = 0;
    for (clientId = 0; clientId < 4; clientId++) {
        count = BtlPosPoke_GetEmptyPositions(&flow->posPoke, clientId, positions);
        if (count != 0) {
            for (i = 0; i < count; i++) {
                RequestChangePokemon(flow->server, positions[i]);
            }
            result = TRUE;
            list->clientIds[list->count++] = clientId;
        }
    }
    return result;
}

BtlClientIDList *BtlServerFlow_GetClientIDList(BtlServerFlow *flow) {
    return &flow->clientIdList;
}

u8 func_ov167_0219fff0(BtlServerFlow *flow) {
    return flow->unk784;
}

void ServerControl_BeforeFirstFight(BtlServerFlow *flow) {
    u32 state;
    u32 i;
    u16 move;
    BattleMon *mon;

    state = PushState(&flow->actionState, 0x59b);
    for (i = 0; i < flow->actionOrderCount; i++) {
        move = ActionOrder_GetMove(&flow->actionOrder[i]);
        if (move != 0) {
            mon = flow->actionOrder[i].mon;
            if (MoveEvent_AddItem(mon, move, GetBattleMonStat(mon, 0xc))) {
                ServerEvent_BeforeFight(flow, mon, move);
                MoveEvent_RemoveItem(mon, move);
            }
        }
    }
    PopState(&flow->actionState, state, 0x5aa);
}

u8 ActionOrder_Build(BtlServerFlow *flow, BtlClientActions *clientActions, ActionOrderEntry *order, u8 max) {
    u16 speed;
    BattleMon *mon;
    u32 state;
    u8 clientId;
    u8 j;
    u8 numActions;
    BOOL rotated;
    u8 i;
    u8 count;
    BtlServerClient *client;
    u8 slot;
    ActionOrderEntry *entry;
    u8 kind;
    u8 priority;

    rotated = FALSE;
    for (clientId = 0, count = 0; clientId < 4; clientId++) {
        client = BattleServer_GetClient(flow->server, clientId);
        if (client == NULL) {
            continue;
        }
        numActions = ClientActions_GetCount(clientActions, clientId);
        slot = 0;
        for (j = 0; j < numActions; j++) {
            entry = &order[count];
            entry->action = ClientActions_GetAction(clientActions, clientId, j);
            if (BattleAction_IsChangeDeplete(&entry->action)) {
                break;
            }
            switch ((u8)BattleAction_GetAction(&entry->action)) {
            case 6:
                slot = GetRotationInSlot((u8)entry->action.change.unk4);
                entry->mon = BattleParty_GetMon(client->party, slot);
                rotated = TRUE;
                break;
            case 3:
                entry->mon = BattleParty_GetMon(client->party, entry->action.change.unk4);
                slot++;
                break;
            default:
                entry->mon = BattleParty_GetMon(client->party, slot);
                slot++;
                break;
            }
            entry->clientId = clientId;
            entry->done = FALSE;
            entry->interrupting = FALSE;
            count++;
        }
    }
    for (i = 0; i < count; i++) {
        state = PushState(&flow->actionState, 0x64a);
        entry = &order[i];
        mon = order[i].mon;
        switch (entry->action.bits.action) {
        case BATTLE_ACTION_ESCAPE:
            if (BtlSetup_GetBattleType(flow->mainModule) == 0 && entry->clientId == 1) {
                kind = 0;
            } else {
                kind = 4;
            }
            break;
        case BATTLE_ACTION_CHANGE:
            kind = 3;
            break;
        case BATTLE_ACTION_ITEM:
            kind = 2;
            break;
        case BATTLE_ACTION_ROTATION:
            kind = 1;
            break;
        case BATTLE_ACTION_SKIP:
            kind = 0;
            break;
        case BATTLE_ACTION_SHIFT:
            kind = 0;
            break;
        case BATTLE_ACTION_FIGHT:
            kind = 0;
            break;
        case BATTLE_ACTION_NULL:
            entry->key = ActionOrder_MakeKey(0, 1, 0, 4);
            continue;
        default:
            kind = 0;
            break;
        case BATTLE_ACTION_REC_TIMEOUT:
            continue;
        }
        if (entry->action.bits.action == BATTLE_ACTION_FIGHT) {
            priority = ServerEvent_GetMovePriority(flow, ActionOrder_GetMove(&order[i]), mon);
        } else if (entry->action.bits.action == BATTLE_ACTION_SHIFT ||
                   entry->action.bits.action == BATTLE_ACTION_SKIP) {
            priority = 7;
        } else {
            priority = 0;
        }
        speed = ServerEvent_CalculateSpeed(flow, mon, TRUE);
        PopState(&flow->actionState, state, 0x682);
        entry->key = ActionOrder_MakeKey(speed, 1, priority, kind);
    }
    ActionOrder_Sort(order, count);
    if (!rotated) {
        for (i = 0; i < count; i++) {
            entry = &order[i];
            if (entry->action.bits.action == BATTLE_ACTION_FIGHT || entry->action.bits.action == BATTLE_ACTION_SHIFT) {
                state = PushState(&flow->actionState, 0x694);
                entry->key =
                    (entry->key & 0xffff1fff) | ((ServerEvent_CheckSpecialPriority(flow, entry->mon) & 7) << 13);
                PopState(&flow->actionState, state, 0x697);
            }
        }
        ActionOrder_Sort(order, count);
    }
    return count;
}

void ActionOrder_Sort(ActionOrderEntry *order, u32 count) {
    u32 i;
    u32 j;
    ActionOrderEntry tmp;

    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++) {
            if (order[i].key <= order[j].key && (order[i].key != order[j].key || BattleRandom(2) != 0)) {
                tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }
}

u8 ServerEvent_GetMovePriority(BtlServerFlow *flow, u16 move, BattleMon *mon) {
    u8 priority;

    priority = PML_MoveGetParam(move, MOVE_PARAM_PRIORITY) + 7;
    BattleEventVar_Push(0x6d1);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetValue(0x18, priority);
    BattleEvent_CallHandlers(flow, 0x11);
    priority = BattleEventVar_GetValue(0x18);
    BattleEventVar_Pop(0x6d7);
    return priority;
}

u16 ServerEvent_CalculateSpeed(BtlServerFlow *flow, BattleMon *mon, BOOL flag) {
    u32 speed;

    speed = GetBattleMonStat(mon, 0xc);
    BattleEventVar_Push(0x6ea);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x2f, speed);
    BattleEventVar_SetValue(0x51, 1);
    BattleEventVar_SetValue(0x4a, 0);
    BattleEventVar_SetMulValue(0x35, 0x1000, 0x19a, 0x20000);
    BattleEvent_CallHandlers(flow, 0x13);
    speed = fixed_round(BattleEventVar_GetValue(0x2f), BattleEventVar_GetValue(0x35));
    if (GetBattleMonStatus(mon) == 1 && BattleEventVar_GetValue(0x51)) {
        speed = speed * 25 / 100;
    }
    if (speed > 10000) {
        speed = 10000;
    }
    if (flag && BattleEventVar_GetValue(0x4a)) {
        speed = 10000 - speed;
    }
    BattleEventVar_Pop(0x70a);
    return speed;
}

// Function name from swan.
ActionOrderEntry *ActionOrder_SearchByMonID(BtlServerFlow *flow, u8 monId) {
    u32 i;

    for (i = 0; i < flow->actionOrderCount; i++) {
        if (GetMonID(flow->actionOrder[i].mon) == monId) {
            return &flow->actionOrder[i];
        }
    }
    return NULL;
}

ActionOrderEntry *ActionOrder_SearchNextByMonID(BtlServerFlow *flow, ActionOrderEntry *after, u8 monId) {
    u32 i;

    for (i = 0; i < flow->actionOrderCount; i++) {
        if (&flow->actionOrder[i] == after) {
            i++;
            break;
        }
    }
    for (; i < flow->actionOrderCount; i++) {
        if (monId == GetMonID(flow->actionOrder[i].mon)) {
            return &flow->actionOrder[i];
        }
    }
    return NULL;
}

ActionOrderEntry *ActionOrder_SearchByMoveID(BtlServerFlow *flow, u16 moveId, u8 start) {
    u32 i;

    for (i = start; i < flow->actionOrderCount; i++) {
        if (!flow->actionOrder[i].done && BattleAction_GetAction(&flow->actionOrder[i].action) == BATTLE_ACTION_FIGHT &&
            moveId == BattleAction_GetFightMove(&flow->actionOrder[i].action)) {
            return &flow->actionOrder[i];
        }
    }
    return NULL;
}

ActionOrderEntry *ActionOrder_SearchAllyByMove(BtlServerFlow *flow, u16 move, u8 monId, u8 target) {
    ActionOrderEntry *entry;

    entry = ActionOrder_SearchByMoveID(flow, move, 0);
    while (entry != NULL) {
        if (IsAllyMonID(monId, GetMonID(entry->mon)) && !IsFainted(entry->mon)) {
            return entry;
        }
        entry = ActionOrder_SearchByMoveID(flow, move, ActionOrder_GetIndex(flow, entry) + 1);
    }
    return NULL;
}

u8 ActionOrder_GetIndex(BtlServerFlow *flow, ActionOrderEntry *entry) {
    s32 i;

    for (i = 0; i < flow->actionOrderCount; i++) {
        if (&flow->actionOrder[i] == entry) {
            return i;
        }
    }
    return flow->actionOrderCount;
}

s32 ActionOrderTool_Interrupt(BtlServerFlow *flow, ActionOrderEntry *entry, s32 start) {
    s32 from;
    s32 to;
    s32 i;
    u8 count;

    from = -1;
    to = -1;
    count = flow->actionOrderCount;
    for (i = start; i < count; i++) {
        if (&flow->actionOrder[i] == entry) {
            from = i;
            break;
        }
    }
    for (; start < count; start++) {
        if (!flow->actionOrder[start].done) {
            to = start;
            break;
        }
    }
    if (from >= 0 && to >= 0 && from > to) {
        flow->tempEntry = *entry;
        for (; from > to; from--) {
            flow->actionOrder[from] = flow->actionOrder[from - 1];
        }
        flow->actionOrder[to] = flow->tempEntry;
        return to;
    }
    return -1;
}

void ActionOrderTool_SendToLast(BtlServerFlow *flow, ActionOrderEntry *entry) {
    s32 from;
    s32 i;

    from = -1;
    for (i = 0; i < flow->actionOrderCount; i++) {
        if (&flow->actionOrder[i] == entry) {
            from = i;
            break;
        }
    }
    if (from >= 0) {
        flow->tempEntry = *entry;
        for (; from < flow->actionOrderCount - 1; from++) {
            flow->actionOrder[from] = flow->actionOrder[from + 1];
        }
        flow->actionOrder[from] = flow->tempEntry;
    }
}

u32 ActionOrder_Proc(BtlServerFlow *flow, ActionOrderEntry *entry) {
    BattleMon *mon;
    BattleAction action;
    u32 state;

    if (!entry->done) {
        mon = entry->mon;
        action = entry->action;
        BtlServerFlow_PushFrame(flow);
        entry->done = TRUE;
        if ((flow->unk14 != 5 || action.bits.action == BATTLE_ACTION_ESCAPE) && !IsFainted(mon) &&
            (action.bits.action == BATTLE_ACTION_ROTATION || DoesBattleMonExist(&flow->posPoke, GetMonID(mon)))) {
            if (CheckCondition(mon, CONDITION_SKY_DROP) &&
                (action.bits.action != BATTLE_ACTION_ESCAPE || GetRunMode(flow->mainModule) != 2)) {
                if (GetConditionCount(mon, 3)) {
                    ServerControl_SetMonCounter(flow, mon, 3, 0);
                }
            } else {
                ServerControl_ActionStart(flow, mon, entry->action.bits.action);
                BattleMon_SetTurnFlag(mon, 0);
                switch (action.bits.action) {
                case BATTLE_ACTION_ROTATION:
                    if (BtlSetup_GetBattleStyle(flow->mainModule) == 3) {
                        ServerControl_Rotation(flow, entry->clientId, action.change.unk4);
                    }
                    break;
                case BATTLE_ACTION_FIGHT:
                    if (!flow->unk1FEC[0]) {
                        ServerControl_BeforeFirstFight(flow);
                        flow->unk1FEC[0] = TRUE;
                    }
                    flow->unk1F7C = ServerControl_CheckObedience(flow, mon);
                    ServerControl_Fight(flow, mon, &action, entry->key & 0x3fffff);
                    break;
                case BATTLE_ACTION_ITEM:
                    if (UseBagItem(flow, mon, action.item.item, action.item.param, action.item.target) == 1 &&
                        ServerControl_EscapeCore(flow, mon, TRUE, TRUE)) {
                        flow->unk14 = 5;
                    }
                    break;
                case BATTLE_ACTION_CHANGE:
                    ServerControl_ChangePokemon(flow, mon, action.change.slot);
                    ServerControl_AfterSwitchIn(flow);
                    break;
                case BATTLE_ACTION_ESCAPE:
                    if (ServerControl_Escape(flow, mon)) {
                        flow->unk14 = 5;
                    }
                    break;
                case BATTLE_ACTION_REC_TIMEOUT:
                    flow->unk14 = 4;
                    break;
                case BATTLE_ACTION_SHIFT:
                    ServerControl_TripleShift(flow, mon);
                    break;
                case BATTLE_ACTION_SKIP:
                    ServerDisplay_MustRecharge(flow, mon);
                    break;
                }
                if (action.bits.action == BATTLE_ACTION_FIGHT || action.bits.action == BATTLE_ACTION_ITEM) {
                    BattleMon_SetTurnFlag(mon, 1);
                    scPut_SetContFlag(flow, mon, 0);
                }
                state = PushState(&flow->actionState, 0x845);
                ServerEvent_ActionEnd(flow, mon, action.bits.action);
                PopState(&flow->actionState, state, 0x847);
            }
        }
        BtlServerFlow_PopFrame(flow);
        return action.bits.action;
    }
    return 0;
}

void ServerControl_ActionStart(BtlServerFlow *flow, BattleMon *mon, u32 action) {
    u32 state;

    state = PushState(&flow->actionState, 0x85e);
    ServerEvent_ActionStart(flow, mon, action);
    PopState(&flow->actionState, state, 0x861);
}

void ServerEvent_ActionStart(BtlServerFlow *flow, BattleMon *mon, u32 action) {
    BattleEventVar_Push(0x86e);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0xc, action);
    BattleEvent_CallHandlers(flow, 1);
    BattleEventVar_Pop(0x872);
}

void ServerEvent_ActionEnd(BtlServerFlow *flow, BattleMon *mon, u32 action) {
    BattleEventVar_Push(0x87e);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0xc, action);
    BattleEvent_CallHandlers(flow, 2);
    BattleEventVar_Pop(0x882);
}

u32 ServerControl_CheckObedience(BtlServerFlow *flow, BattleMon *mon) {
    void *src;
    PlayerInfo *player;
    u32 badges;
    u16 level;
    u16 maxLevel;
    u16 rand;

    if (BtlSetup_GetBattleType(flow->mainModule) <= 1 &&
        GetClientIDFromMonID(GetMonID(mon)) == GetPlayerClientID(flow->mainModule)) {
        src = GetSrcData(mon);
        player = GetPlayerInfo(flow->mainModule);
        if (GetStudioMode(flow->mainModule) != 0) {
            return 0;
        }
        if (!IsTrainerOT(src, player)) {
            badges = GetBadgeCount(flow->mainModule);
            if (badges < 8) {
                level = GetBattleMonStat(mon, 0xf);
                maxLevel = (badges + 1) * 10;
                if (level <= maxLevel) {
                    return 0;
                }
                if ((u16)BattleRandom(level + maxLevel + 1) < maxLevel) {
                    return 0;
                }
                if (CheckCondition(mon, CONDITION_SLEEP)) {
                    return 4;
                }
                rand = BattleRandom(256);
                if (rand < level - maxLevel && !ServerEvent_CheckSleepPrevented(flow, mon)) {
                    return 2;
                }
                rand -= level - maxLevel;
                if (rand < level - maxLevel) {
                    return 3;
                }
                return 4;
            }
        }
    }
    return 0;
}

BOOL ServerEvent_CheckSleepPrevented(BtlServerFlow *flow, BattleMon *mon) {
    BOOL result;

    BattleEventVar_Push(0x8e0);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetRewriteOnceValue(0x41, 0);
    BattleEvent_CallHandlers(flow, 0xe);
    result = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x8e5);
    return result;
}

BOOL ActionOrder_InterruptProc(BtlServerFlow *flow, u8 monId, u8 targetId) {
    ActionOrderEntry *entry;

    entry = ActionOrder_SearchByMonID(flow, monId);
    if (entry != NULL) {
        if (entry->done && BattleAction_GetAction(&entry->action) == BATTLE_ACTION_ROTATION) {
            entry = ActionOrder_SearchNextByMonID(flow, entry, monId);
        }
        if (!entry->done) {
            if (GetEncoreReplacementMove(entry->mon, &entry->action)) {
                return FALSE;
            }
            if (BattleAction_GetAction(&entry->action) == BATTLE_ACTION_ROTATION) {
                return FALSE;
            }
            BattleAction_ChangeFightTargetPos(&entry->action, GetBattlePos(&flow->posPoke, targetId));
            ActionOrder_Proc(flow, entry);
            return TRUE;
        }
    }
    return FALSE;
}

// Function names from swan.
BOOL ActionOrder_InterruptReserve(BtlServerFlow *flow, u8 monId) {
    ActionOrderEntry *entry;

    entry = ActionOrder_SearchByMonID(flow, monId);
    if (entry && !entry->done && ActionOrderTool_Interrupt(flow, entry, 0) >= 0) {
        return TRUE;
    }
    return FALSE;
}

BOOL ActionOrder_InterruptReserveByMove(BtlServerFlow *flow, u16 moveId) {
    u32 start;
    BOOL didInterrupt;
    ActionOrderEntry *entry;
    s32 index;

    start = 0;
    entry = ActionOrder_SearchByMoveID(flow, moveId, 0);
    didInterrupt = FALSE;
    while (entry) {
        index = ActionOrderTool_Interrupt(flow, entry, start);
        if (index < 0) {
            break;
        }
        start = index + 1;
        entry = ActionOrder_SearchByMoveID(flow, moveId, (u8)start);
        didInterrupt = TRUE;
    }
    return didInterrupt;
}

BOOL ActionOrder_SendToLast(BtlServerFlow *flow, u8 monId) {
    ActionOrderEntry *entry;

    entry = ActionOrder_SearchByMonID(flow, monId);
    if (entry && !entry->done) {
        ActionOrderTool_SendToLast(flow, entry);
        return TRUE;
    }
    return FALSE;
}

void ActionOrder_ForceDone(BtlServerFlow *flow, u8 monId) {
    ActionOrderEntry *entry;

    entry = ActionOrder_SearchByMonID(flow, monId);
    if (entry) {
        entry->done = 1;
    }
}

void BattleMoveEffectState_Init(BattleMoveEffectState *targets) {
    targets->unk00 = 0;
    targets->pos1 = 6;
    targets->pos2 = 6;
    targets->index = 0;
    targets->enabled = FALSE;
    targets->unk05_1 = 0;
}

void BattleMoveEffectState_SetTargets(BattleMoveEffectState *targets, BtlServerFlow *flow, BattleMon *mon,
                                      void *monSet) {
    u32 count;

    count = BtlMonSet_GetCountMax(monSet);
    targets->pos1 = GetBattlePos(&flow->posPoke, GetMonID(mon));
    targets->pos2 = 6;
    targets->enabled = FALSE;
    targets->unk05_1 = 0;
    if (count == 1 && BtlMonSet_GetCount(monSet)) {
        targets->pos2 = GetBattlePos(&flow->posPoke, GetMonID(BtlMonSet_Get(monSet, 0)));
    }
}

void BtlFlowMonIter_Init(BtlFlowMonIter *iter, BtlServerFlow *flow) {
    u8 clientId;
    BtlServerClient *client;
    u8 i;

    clientId = 0;
    iter->clientId = 0;
    iter->index = clientId;
    iter->done = TRUE;
    iter->rotation = clientId;
    for (; clientId < 4; clientId++) {
        client = BattleServer_GetClientNoCheck(flow->server, clientId);
        if (client != NULL) {
            for (i = 0; i < client->numCoverPos; i++) {
                if (BtlFlow_IsMonAlive(GetBattleMonFromParty(client->party, i))) {
                    iter->clientId = clientId;
                    iter->done = FALSE;
                    iter->index = i;
                    return;
                }
            }
        }
    }
}

void BtlFlowMonIter_SetRotation(BtlFlowMonIter *iter, BtlServerFlow *flow) {
    if (BtlSetup_GetBattleStyle(flow->mainModule) == 3) {
        iter->rotation = TRUE;
    }
}

BOOL BtlFlowMonIter_Next(BtlFlowMonIter *iter, BtlServerFlow *flow, BattleMon **mon) {
    BtlServerClient *client;
    u8 count;

    if (iter->done) {
        return FALSE;
    }
    *mon = BattleParty_GetMon(BattleServer_GetClient(flow->server, iter->clientId)->party, iter->index);
    iter->index++;
    while (iter->clientId < 4) {
        client = BattleServer_GetClientNoCheck(flow->server, iter->clientId);
        if (client != NULL) {
            count = !iter->rotation ? client->numCoverPos : 3;
            for (; iter->index < count; iter->index++) {
                if (BtlFlow_IsMonAlive(BattleParty_GetMon(client->party, iter->index))) {
                    return TRUE;
                }
            }
        }
        iter->clientId++;
        iter->index = 0;
    }
    iter->done = TRUE;
    return TRUE;
}

void ServerControl_TripleShift(BtlServerFlow *flow, BattleMon *mon) {
    u8 clientId;
    s32 slot;

    clientId = GetClientIDFromMonID(GetMonID(mon));
    slot = FindPartyMon(GetPartyData(flow->pokeCon, clientId), mon);
    if (slot == 0 || slot == 2) {
        ServerControl_MoveCore(flow, clientId, slot, 1, 0);
        ServerDisplay_SkyDropTargetAppear(flow, mon, 0xe7);
        ServerControl_AfterMove(flow, clientId, slot, 1);
    }
}

void ServerControl_MoveCore(BtlServerFlow *handler, u8 clientId, u8 firstSlot, u8 secondSlot, u32 flag) {
    BattleParty *party;
    u8 firstPos;
    u8 secondPos;
    BattleMon *first;
    BattleMon *second;

    party = GetPartyData(handler->pokeCon, clientId);
    firstPos = GetClientBattlePos(handler->mainModule, clientId, firstSlot);
    secondPos = GetClientBattlePos(handler->mainModule, clientId, secondSlot);
    BtlPosPoke_Swap(&handler->posPoke, firstPos, secondPos);
    BattleParty_Swap(party, firstSlot, secondSlot);
    if (!flag) {
        BtlServerCmdQueue_Put(handler->queue, SC_SWAP_POS, clientId, firstPos, secondPos);
        first = BattleParty_GetMon(party, firstSlot);
        second = BattleParty_GetMon(party, secondSlot);
        if (!IsFainted(first)) {
            BattleMon_SetTurnFlag(first, 0xc);
        }
        if (!IsFainted(second)) {
            BattleMon_SetTurnFlag(second, 0xc);
        }
    }
}

void ServerControl_AfterMove(BtlServerFlow *handler, u8 clientId, u8 firstSlot, u8 secondSlot) {
    BattleParty *party;
    BattleMon *first;
    BattleMon *second;

    party = GetPartyData(handler->pokeCon, clientId);
    first = GetBattleMonFromParty(party, firstSlot);
    second = GetBattleMonFromParty(party, secondSlot);
    ServerControl_AfterMoveCore(handler, first);
    ServerControl_AfterMoveCore(handler, second);
}

void ServerControl_AfterMoveCore(BtlServerFlow *flow, BattleMon *mon) {
    u32 state;

    if (!IsFainted(mon)) {
        state = PushState(&flow->actionState, 0xa99);
        ServerEvent_AfterMove(flow, mon);
        PopState(&flow->actionState, state, 0xa9b);
    }
}

void ServerEvent_AfterMove(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0xaa8);
    BattleEventVar_SetValue(2, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0xa4);
    BattleEventVar_Pop(0xaab);
}

BOOL ServerControl_Escape(BtlServerFlow *flow, BattleMon *mon) {
    u8 clientId;
    u8 playerClientId;
    BOOL result;

    clientId = GetClientIDFromMonID(GetMonID(mon));
    playerClientId = GetPlayerClientID(flow->mainModule);
    result = ServerControl_EscapeSub(flow, mon, FALSE);
    if (result) {
        return TRUE;
    }
    if (clientId == playerClientId) {
        IncRecord(flow->mainModule, 0x4e);
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x49, SC_MSG_ARG_END);
    } else {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x36b, GetMonID(mon), SC_MSG_ARG_END);
    }
    if (clientId == playerClientId) {
        flow->unk786++;
        if (flow->unk786 > 30) {
            flow->unk786 = 30;
        }
    }
    return result;
}

BOOL ServerControl_EscapeSub(BtlServerFlow *handler, BattleMon *mon, u32 flag) {
    BOOL result;
    BattleMon *enemy;
    u16 speed;
    u16 enemySpeed;
    u32 chance;

    result = FALSE;
    if (GetRunMode(handler->mainModule) == 0) {
        if (GetClientIDFromMonID(GetMonID(mon)) == GetPlayerClientID(handler->mainModule)) {
            result = ServerEvent_CheckSureEscape(handler, mon);
            if (result) {
                flag = TRUE;
            }
        } else {
            flag = TRUE;
        }
    } else {
        flag = TRUE;
        result = TRUE;
    }
    if (!flag) {
        enemy = GetClientMonData(handler->pokeCon, 1, 0);
        if (enemy != NULL) {
            speed = RawBattleMonStat(mon, 0xc);
            enemySpeed = RawBattleMonStat(enemy, 0xc);
            if (speed <= enemySpeed) {
                chance = (speed << 12) / enemySpeed * 128 >> 12;
                chance += handler->unk786 * 30;
                if (BattleRandom(256) >= chance) {
                    return FALSE;
                }
            }
        }
    }
    return ServerControl_EscapeCore(handler, mon, result, FALSE);
}

BOOL ServerEvent_CheckSureEscape(BtlServerFlow *flow, BattleMon *mon) {
    BOOL result;

    if (!IsFainted(mon)) {
        BattleEventVar_Push(0xb31);
        BattleEventVar_SetConstValue(2, GetMonID(mon));
        BattleEventVar_SetRewriteOnceValue(0x51, 0);
        BattleEvent_CallHandlers(flow, 0xb);
        result = BattleEventVar_GetValue(0x51);
        BattleEventVar_Pop(0xb36);
        return result;
    }
    return FALSE;
}

BOOL ServerControl_EscapeCore(BtlServerFlow *flow, BattleMon *mon, BOOL arg2, BOOL arg3) {
    u8 clientId;
    u8 isEnemy;
    BOOL result;
    u32 state;
    u32 state2;

    clientId = GetClientIDFromMonID(GetMonID(mon));
    if (GetRunMode(flow->mainModule) == 0) {
        isEnemy = !IsAllyClientID(clientId, GetPlayerClientID(flow->mainModule)) ? TRUE : FALSE;
        if (!arg2 && !IsFainted(mon)) {
            state = PushState(&flow->actionState, 0xb58);
            result = ServerEvent_CheckEscapeBlocked(flow, mon);
            PopState(&flow->actionState, state, 0xb5a);
            if (result) {
                return FALSE;
            }
        }
        if (!flow->unk78A_2) {
            if (!arg3) {
                state2 = PushState(&flow->actionState, 0xb68);
                result = ServerEvent_EscapeMessage(flow, mon);
                PopState(&flow->actionState, state2, 0xb6a);
            } else {
                result = FALSE;
            }
            if (!result) {
                if (isEnemy) {
                    if (BtlSetup_GetBattleType(flow->mainModule) == 0) {
                        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD_SE, 0x4b, 0x56a, GetMonID(mon),
                                                 SC_MSG_ARG_END);
                    }
                } else {
                    BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD_SE, 0x48, 0x56a, SC_MSG_ARG_END);
                }
            }
            flow->unk78A_2 = TRUE;
        }
    } else if (GetCommMode(flow->mainModule) && !flow->unk78A_2) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0xc6, SC_MSG_ARG_END);
        flow->unk78A_2 = TRUE;
    }
    BtlClientIDList_Add(&flow->clientIdList, clientId);
    return TRUE;
}

BOOL ServerEvent_CheckEscapeBlocked(BtlServerFlow *flow, BattleMon *mon) {
    BOOL result;

    BattleEventVar_Push(0xb9d);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetRewriteOnceValue(0x41, 0);
    Condition_CheckTrapped(flow, mon);
    BattleEvent_CallHandlers(flow, 0xc);
    result = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0xba3);
    return result;
}

BOOL ServerEvent_EscapeMessage(BtlServerFlow *flow, BattleMon *mon) {
    BOOL result;

    BattleEventVar_Push(0xbb3);
    BattleEventVar_SetValue(2, GetMonID(mon));
    BattleEventVar_SetRewriteOnceValue(0x51, 0);
    BattleEvent_CallHandlers(flow, 0xd);
    result = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0xbb8);
    return result;
}

void ServerControl_SwitchInCore(BtlServerFlow *flow, u8 clientId, u8 pos, u8 slot) {
    BattleParty *party;
    BattleMon *mon;
    u8 monId;

    party = GetPartyData(flow->pokeCon, clientId);
    BtlServerCmdQueue_Put(flow->queue, SC_OP_SETUP_ILLUSION, clientId);
    UpdatePartyIllusionDisguises(flow->mainModule, party, clientId);
    if (pos != slot) {
        BattleParty_Swap(party, pos, slot);
    }
    mon = BattleParty_GetMon(party, pos);
    monId = GetMonID(mon);
    RegisterDexMon(flow->mainModule, clientId, mon);
    AbilityEvent_AddItem(mon);
    ItemEvent_AddItem(mon);
    BattleMon_SetAppearTurn(mon, flow->unk10);
    BattleMon_ClearForSwitchIn(mon);
    flow->unk7C1[monId] = TRUE;
    if (BtlSetup_GetBattleStyle(flow->mainModule) == 3 && GetClientBattlerCount(flow->mainModule, clientId) <= pos) {
        flow->unk7C1[monId] = FALSE;
    }
    BtlServerCmdQueue_Put(flow->queue, SC_OP_SEND_IN, clientId, pos, slot, (u16)flow->unk10);
    BtlPosPoke_SetMon(&flow->posPoke, GetClientBattlePos(flow->mainModule, clientId, pos), monId, flow->pokeCon);
}

void ServerControl_SwitchInFillSlot(BtlServerFlow *handler, u8 target, u8 slot, u8 slotAgain, BOOL flag) {
    ServerControl_SwitchInCore(handler, target, slot, slotAgain);
    BtlServerCmdQueue_Put(handler->queue, SC_SEND_OUT, target, slot, slotAgain, (u8)flag);
}

BOOL ServerControl_AfterSwitchIn(BtlServerFlow *handler) {
    BOOL result;
    BtlFlowMonIter iter;
    BattleMon *mon;
    u8 monId;
    void *monSet;
    u32 state;

    result = FALSE;
    monSet = handler->unk1A68;
    BtlMonSet_Clear(monSet);
    BtlFlowMonIter_Init(&iter, handler);
    while (BtlFlowMonIter_Next(&iter, handler, &mon)) {
        monId = GetMonID(mon);
        if (handler->unk7C1[monId]) {
            BtlMonSet_Add(monSet, mon);
            handler->unk7C1[monId] = FALSE;
        }
    }
    SortBySpeed(monSet, handler);
    state = PushState(&handler->actionState, 0xc28);
    ServerEvent_MemberInPrev(handler);
    if (BattleHandler_Result(handler)) {
        result = TRUE;
    }
    PopState(&handler->actionState, state, 0xc2e);
    BtlMonSet_ResetIndex(monSet);
    while ((mon = BtlMonSet_GetNext(monSet)) != NULL) {
        state = PushState(&handler->actionState, 0xc34);
        ServerEvent_MemberIn(handler, mon);
        if (BattleHandler_Result(handler)) {
            result = TRUE;
        }
        PopState(&handler->actionState, state, 0xc3a);
    }
    state = PushState(&handler->actionState, 0xc3d);
    ServerEvent_MemberInComplete(handler);
    if (BattleHandler_Result(handler)) {
        result = TRUE;
    }
    PopState(&handler->actionState, state, 0xc43);
    return result;
}

void ServerEvent_MemberIn(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0xc53);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0x55);
    BattleEventVar_Pop(0xc56);
}

void ServerEvent_AbilityMemberIn(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0xc65);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEvent_CallFactorHandlers(flow, 0x55, 4);
    BattleEventVar_Pop(0xc68);
}

void ServerEvent_MemberInPrev(BtlServerFlow *flow) {
    BattleEventVar_Push(0xc75);
    BattleEvent_CallHandlers(flow, 0x56);
    BattleEventVar_Pop(0xc77);
}

void ServerEvent_MemberInComplete(BtlServerFlow *flow) {
    BattleEventVar_Push(0xc83);
    BattleEvent_CallHandlers(flow, 0x57);
    BattleEventVar_Pop(0xc85);
}

void ServerControl_RotationIn(BtlServerFlow *flow) {
    u32 state;

    state = PushState(&flow->actionState, 0xc92);
    ServerEvent_RotationIn(flow);
    PopState(&flow->actionState, state, 0xc94);
}

void ServerEvent_RotationIn(BtlServerFlow *flow) {
    BattleEventVar_Push(0xca1);
    BattleEvent_CallHandlers(flow, 0x58);
    BattleEventVar_Pop(0xca3);
}

void ServerDisplay_RecallMessage(BtlServerFlow *flow, BattleMon *mon) {
    u8 monId;

    monId = GetMonID(mon);
    BtlServerCmdQueue_Put(flow->queue, SC_RECALL_MSG, GetClientIDFromMonID(monId), monId);
}

void ServerControl_ChangePokemon(BtlServerFlow *flow, BattleMon *mon, u8 slot) {
    u8 clientId;
    u8 index;

    ServerDisplay_RecallMessage(flow, mon);
    if (ServerControl_SwitchOut(flow, mon, FALSE)) {
        GetPosClientAndIndex(flow->mainModule, MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(mon)),
                             &clientId, &index);
        if (flow->unk7D9[clientId] < 0xff) {
            flow->unk7D9[clientId]++;
        }
        ServerControl_SwitchInFillSlot(flow, clientId, index, slot, TRUE);
    }
}

BOOL ServerControl_SwitchOut(BtlServerFlow *handler, BattleMon *mon, u8 flag) {
    u32 count;
    u32 i;

    if (!flag) {
        count = ServerEvent_InterruptSwitch(handler, mon);
        if (count != 0) {
            handler->unk78A_0 = TRUE;
            for (i = 0; i < count; i++) {
                ActionOrder_InterruptProc(handler, handler->interruptMonIds[i], GetMonID(mon));
                if (IsFainted(mon)) {
                    break;
                }
            }
            handler->unk78A_0 = FALSE;
        }
    }
    if (!IsFainted(mon)) {
        ServerControl_SwitchOutCore(handler, mon, 0x26c);
        return TRUE;
    }
    return FALSE;
}

void ServerControl_SwitchOutCore(BtlServerFlow *flow, BattleMon *mon, u32 effect) {
    u8 monId;
    u8 pos;
    u32 state;

    monId = GetMonID(mon);
    pos = MonIDToBattlePos(flow->mainModule, flow->pokeCon, monId);
    if (pos != 6) {
        BtlServerCmdQueue_Put(flow->queue, SC_RECALL, pos, effect);
    }
    ActionOrder_ForceDone(flow, monId);
    state = PushState(&flow->actionState, 0xd0f);
    ServerControl_SwitchOutConfirm(flow, mon);
    PopState(&flow->actionState, state, 0xd11);
    ServerControl_ClearMonDependentEffects(flow, mon, FALSE);
    Clear_ForSwitch(mon);
    BtlServerCmdQueue_Put(flow->queue, SC_OP_CLEAR_FOR_SWITCH, GetMonID(mon));
    BtlPosPoke_RemoveMon(&flow->posPoke, monId);
}

void ServerControl_SwitchOutConfirm(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0xd24);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0x54);
    BattleEventVar_Pop(0xd27);
}

u16 GetEncoreReplacementMove(BattleMon *mon, BattleAction *action) {
    u16 move;

    if (CheckCondition(mon, CONDITION_ENCORE) && action->bits.move != 0xa5) {
        move = Condition_GetParam(GetConditionContinuationParam(mon, CONDITION_ENCORE));
        if (move != action->bits.move && BattleMon_GetPPOfMove(mon, move)) {
            return move;
        }
    }
    return 0;
}

void ServerControl_Fight(BtlServerFlow *flow, BattleMon *mon, BattleAction *action, u32 key) {
    BtlFlowFightWork work;
    u8 target;
    u32 prevStatus;
    u8 cond19;
    u8 cond1a;
    u8 hasDelegate;
    u8 usedMove;
    u8 moveSlot;
    u8 targetCount;
    u16 move;
    u16 actualMove;
    u16 encoreMove;

    prevStatus = BattleMon_GetSemiInvulnFlag(mon);
    BattleMoveEffectState_Init(flow->moveEffect);
    BtlFlowReactionList_Init(flow->unk4B0);
    work.called.move = 0;
    work.called.target = 6;
    encoreMove = GetEncoreReplacementMove(mon, action);
    if (encoreMove) {
        BattleAction_SetFightParam(action, encoreMove,
                                   DecideMoveTargetAuto(flow->mainModule, flow->pokeCon, mon, encoreMove));
    }
    move = action->bits.move;
    moveSlot = BattleMon_FindMoveSlot(mon, move);
    actualMove = move;
    target = action->bits.target;
    usedMove = FALSE;
    work.result = 0;
    cond19 = CheckCondition(mon, CONDITION_MOVE_LOCK);
    cond1a = CheckCondition(mon, CONDITION_CHARGE);
    MoveEvent_AddItem(mon, move, key);
    ServerControl_MoveSeqStart(flow, mon, move);
    do {
        if (ServerControl_CheckMoveFail(flow, mon, move, cond19 || cond1a)) {
            break;
        }
        if (!ServerEvent_GetCalledMove(flow, mon, move, target, &work.called)) {
            ServerDisplay_MoveMessage(flow, mon, move);
            ServerEvent_GetMoveParam(flow, move, mon, flow->unk1AB4);
            CollectMoveTargets(flow, mon, target, flow->unk1AB4, flow->unk850);
            ServerControl_ConsumePP(flow, mon, move, moveSlot, flow->unk850);
            usedMove = TRUE;
            ServerControl_MoveFail(flow, mon, move, 0x1a);
            break;
        }
        hasDelegate = work.called.move != 0 ? TRUE : FALSE;
        if (hasDelegate) {
            ServerDisplay_MoveMessage(flow, mon, move);
            ServerDisplay_MoveAnimation(flow, mon, move, target);
            MoveEvent_AddItem(mon, work.called.move, key);
            actualMove = work.called.move;
            target = work.called.target;
        } else {
            actualMove = move;
        }
        ServerEvent_GetMoveParam(flow, move, mon, flow->unk1AB4);
        if (hasDelegate) {
            ServerEvent_GetMoveParam(flow, actualMove, mon, flow->unk1AB0);
            flow->unk1AB0->flags.raw |= 4;
            flow->unk1AB0->originalMove = move;
            if (ServerControl_CheckDelegateMoveFail(flow, mon, flow->unk1AB0)) {
                ServerControl_ConsumePP(flow, mon, move, moveSlot, flow->unk850);
                usedMove = TRUE;
                break;
            }
        } else {
            *flow->unk1AB0 = *flow->unk1AB4;
        }
        BtlMonSet_Clear(flow->unk850);
        BtlMonSet_Clear(flow->unk854);
        CollectMoveTargets(flow, mon, target, flow->unk1AB0, flow->unk850);
        targetCount = BtlMonSet_GetCountMax(flow->unk850);
        BtlMonSet_RemoveFainted(flow->unk850);
        BtlMonSet_CopyNotFainted(flow->unk850, flow->unk854);
        BtlMonSet_SetNumTargets(flow->unk854, targetCount);
        if (!cond19 && !cond1a && moveSlot != 4) {
            ServerControl_ConsumePP(flow, mon, move, moveSlot, flow->unk850);
            usedMove = TRUE;
        }
        ServerControl_CallMoveEvent(flow, mon, flow->unk1AB4, 0x22);
        ServerEvent_RewriteMoveType(flow, mon, flow->unk1AB0);
        if (ServerEvent_SetMoveMessage(flow, mon, move, actualMove, &flow->message)) {
            BattleHandler_SetString(flow, &flow->message);
            BattleHandler_StrClear(&flow->message);
        } else {
            ServerDisplay_MoveMessage(flow, mon, actualMove);
        }
        flow->unk1F78 = actualMove;
        if (CheckCondition(mon, CONDITION_MICLE_BERRY)) {
            ServerControl_CureCondition(flow, mon, CONDITION_MICLE_BERRY, 0);
            BattleMon_SetTurnFlag(mon, 0xe);
        }
        if (ServerControl_CheckMoveFailLate(flow, mon, actualMove)) {
            break;
        }
        if (ServerControl_CheckDelayMove(flow, mon, actualMove, target, &work)) {
            break;
        }
        if (ServerControl_CheckPledgeCombo(flow, mon, actualMove, target)) {
            break;
        }
        BtlWazaRec_Add(&flow->wazaRec, actualMove, flow->unk10, GetMonID(mon));
        ServerControl_CallMoveEvent(flow, mon, flow->unk1AB0, 0x23);
        if (ServerControl_CheckMoveSteal(flow, mon, actualMove, flow->unk854, flow->unk4AC)) {
            ServerControl_StealMove(flow, mon, actualMove, flow->unk854);
        } else {
            work.result = ServerControl_ExecuteMove(flow, mon, actualMove, flow->unk854);
        }
    } while (0);
    BattleMon_SetTurnFlag(mon, 3);
    if (usedMove || cond19 || actualMove == 0xa5) {
        BattleMon_SetPrevMove(mon, target, work.result, flow->unk1AB0->type, actualMove, move);
        BtlServerCmd_Put18(flow->queue, GetMonID(mon), target, work.result, flow->unk1AB0->type, actualMove, move);
    } else if (GetConditionCount(mon, 3)) {
        ServerControl_SetMonCounter(flow, mon, 3, 0);
    }
    if (prevStatus != 0x10 && BattleMon_GetSemiInvulnFlag(mon) != 0x10) {
        ServerControl_ClearChargeState(flow, mon);
    }
    if (GetTurnFlag(mon, 0xb)) {
        BtlServerCmdQueue_Put(flow->queue, SC_SET_MON_VISIBLE, GetMonID(mon), 0);
        ServerControl_SkyDropCheckRelease(flow, mon, FALSE);
    }
    ServerControl_MoveSeqEnd(flow, mon, actualMove, work.result);
    if (work.called.move) {
        MoveEvent_RemoveItem(mon, work.called.move);
    }
    MoveEvent_RemoveItem(mon, move);
    ServerControl_ReflectMoves(flow, actualMove);
}

BOOL ServerControl_CheckDelegateMoveFail(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param) {
    u16 move;
    u32 condition;

    move = param->move;
    condition = 0;
    if (CheckCondition(mon, CONDITION_HEAL_BLOCK) && getMoveFlag(move, 0xc)) {
        condition = 0xd;
    } else if (IsFieldEffectActive(2) && getMoveFlag(move, 9)) {
        condition = 0x14;
    }
    if (condition != 0) {
        ServerControl_MoveFail(flow, mon, move, condition);
        return TRUE;
    }
    return FALSE;
}

void ServerControl_ReflectMoves(BtlServerFlow *flow, u16 move) {
    u32 i;
    BattleMon *mon;
    BattleMon *target;
    u32 state;

    for (i = 0; i < flow->unk4B0->count; i++) {
        mon = GetPokeParam(flow->pokeCon, flow->unk4B0->monIds[i]);
        target = GetPokeParam(flow->pokeCon, flow->unk4B0->unk0D[i]);
        if (CanPokemonBattle(mon)) {
            state = PushState(&flow->actionState, 0xe59);
            ServerEvent_ReflectMove(flow, mon, target, move);
            PopState(&flow->actionState, state, 0xe5b);
            ServerEvent_GetMoveParam(flow, move, mon, flow->unk1AB0);
            flow->unk1AB0->flags.unk0 = TRUE;
            CollectMoveTargets(flow, mon, flow->unk4B0->targets[i], flow->unk1AB0, flow->unk864);
            BattleEvent_PausePersistentMoveItems(GetMonID(mon));
            MoveEvent_AddItem(mon, move, GetBattleMonStat(mon, 0xc));
            ServerControl_ExecuteMove(flow, mon, move, flow->unk864);
            RemoveForce(mon, move);
            BattleEvent_ResumeMoveItems(GetMonID(mon));
        }
    }
}

void BtlFlowReactionList_Init(BtlFlowReactionList *list) {
    u32 i;

    list->count = 0;
    for (i = 0; i < 6; i++) {
        list->monIds[i] = 0x1f;
    }
}

void BtlFlowReactionList_Add(BtlFlowReactionList *list, u8 monId, u8 arg2, u8 target) {
    if (list->count < 6) {
        list->monIds[list->count] = monId;
        list->unk0D[list->count] = arg2;
        list->targets[list->count] = target;
        list->count++;
    }
}

void ServerControl_StealMove(BtlServerFlow *flow, BattleMon *attacker, u16 move, void *targets) {
    BattleMon *mon;
    u32 state;

    if (flow->unk4AC->count != 0) {
        mon = GetPokeParam(flow->pokeCon, flow->unk4AC->monIds[0]);
        state = PushState(&flow->actionState, 0xe98);
        ServerEvent_StealMove(flow, mon, attacker, move);
        PopState(&flow->actionState, state, 0xe9a);
        ServerEvent_GetMoveParam(flow, move, mon, flow->unk1AB0);
        CollectMoveTargets(flow, mon, flow->unk4AC->targets[0], flow->unk1AB0, flow->unk864);
        if (CheckCondition(mon, CONDITION_HEAL_BLOCK) && getMoveFlag(move, 0xc)) {
            ServerDisplay_MoveFailMessage(flow, mon, move, 0xd);
            return;
        }
        MoveEvent_AddItem(mon, move, GetBattleMonStat(mon, 0xc));
        ServerControl_ExecuteMove(flow, mon, move, flow->unk864);
        RemoveForce(mon, move);
    }
}

void ServerEvent_RewriteMoveType(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param) {
    BattleEventVar_Push(0xebb);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetRewriteOnceValue(0x16, param->type);
    BattleEvent_CallHandlers(flow, 0xa2);
    param->type = BattleEventVar_GetValue(0x16);
    BattleEventVar_Pop(0xec1);
}

void ServerControl_CallMoveEvent(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 event) {
    u32 state;

    state = PushState(&flow->actionState, 0xece);
    ServerEvent_CallMoveEvent(flow, mon, param, event);
    PopState(&flow->actionState, state, 0xed2);
}

void ServerEvent_CallMoveEvent(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 event) {
    BattleEventVar_Push(0xedf);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEvent_CallHandlers(flow, event);
    BattleEventVar_Pop(0xee4);
}

// Moves that combine with each other when allies use them in the same turn
static const u16 data_ov167_021d6cec[3] = { MOVE_FIRE_PLEDGE, MOVE_WATER_PLEDGE, MOVE_GRASS_PLEDGE };

BOOL ServerControl_CheckPledgeCombo(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target) {
    u32 i;
    u32 count;
    u8 monId;
    u8 partnerId;
    BattleMon *partner;
    u32 index;
    u32 minIndex;
    ActionOrderEntry *entry;
    ActionOrderEntry *found[2];

    for (i = 0; i < 3; i++) {
        if (move == data_ov167_021d6cec[i]) {
            break;
        }
    }
    if (i != 3 && !GetTurnFlag(mon, 0xa) && !BattleMon_HasComboMove(mon)) {
        count = 0;
        monId = GetMonID(mon);
        for (i = 0; i < 3; i++) {
            if (move != data_ov167_021d6cec[i]) {
                found[count] = ActionOrder_SearchAllyByMove(flow, data_ov167_021d6cec[i], monId, target);
                if (found[count] != NULL) {
                    count++;
                    if (count >= 2) {
                        break;
                    }
                }
            }
        }
        if (count != 0) {
            minIndex = 6;
            partner = found[0]->mon;
            for (i = 0; i < count; i++) {
                entry = found[i];
                index = ActionOrder_GetIndex(flow, entry);
                if (index < minIndex) {
                    minIndex = index;
                    partner = entry->mon;
                }
            }
            partnerId = GetMonID(partner);
            BattleMon_SetTurnFlag(mon, 0xa);
            BattleMon_SetComboMove(partner, monId, move);
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x47a, monId, partnerId, SC_MSG_ARG_END);
            ActionOrder_InterruptReserve(flow, partnerId);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ServerControl_CheckDelayMove(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target, BtlFlowFightWork *work) {
    u32 state;
    BOOL result;
    u8 pos;

    state = PushState(&flow->actionState, 0xf46);
    result = ServerEvent_CheckDelayMove(flow, mon, target);
    work->result = 0;
    if (result) {
        pos = MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(mon));
        if (BattleHandler_Result(flow) == 2) {
            BtlServerCmdQueue_Put(flow->queue, SC_MOVE_ANIM, pos, target, move, 0);
            ServerControl_DelayMoveDecide(flow, mon);
            work->result = 1;
        } else {
            ServerDisplay_MoveFailed(flow, mon, move);
        }
    }
    PopState(&flow->actionState, state, 0xf59);
    return result;
}

BOOL ServerEvent_CheckDelayMove(BtlServerFlow *flow, BattleMon *mon, u8 target) {
    BOOL result;

    BattleEventVar_Push(0xf6b);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0xd, target);
    BattleEventVar_SetRewriteOnceValue(0x51, 0);
    BattleEvent_CallHandlers(flow, 6);
    result = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0xf71);
    return result;
}

void ServerControl_DelayMoveDecide(BtlServerFlow *flow, BattleMon *mon) {
    u32 state;

    state = PushState(&flow->actionState, 0xf7f);
    ServerEvent_DelayMoveDecide(flow, mon);
    PopState(&flow->actionState, state, 0xf84);
}

void ServerEvent_DelayMoveDecide(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0xf90);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 7);
    BattleEventVar_Pop(0xf93);
}

void ServerControl_MoveSeqStart(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    u32 state;

    state = PushState(&flow->actionState, 0xfa1);
    ServerEvent_MoveSeqStart(flow, mon, move);
    PopState(&flow->actionState, state, 0xfa3);
}

void ServerEvent_MoveSeqStart(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    BattleEventVar_Push(0xfb0);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, move);
    BattleEvent_CallHandlers(flow, 3);
    BattleEventVar_Pop(0xfb4);
}

void ServerControl_MoveSeqEnd(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 result) {
    u32 state;

    state = PushState(&flow->actionState, 0xfc1);
    ServerEvent_MoveSeqEnd(flow, mon, move, result);
    PopState(&flow->actionState, state, 0xfc3);
}

void ServerEvent_MoveSeqEnd(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 result) {
    BattleEventVar_Push(0xfd0);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetConstValue(0x51, result);
    BattleEvent_CallHandlers(flow, 4);
    BattleEventVar_Pop(0xfd5);
}

void ServerEvent_StealMove(BtlServerFlow *flow, BattleMon *mon, BattleMon *attacker, u16 move) {
    BattleEventVar_Push(0xfe3);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(0x12, move);
    BattleEvent_CallHandlers(flow, 8);
    BattleEventVar_Pop(0xfe8);
}

void ServerEvent_ReflectMove(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move) {
    BattleEventVar_Push(0xff5);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(3, GetMonID(target));
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetRewriteOnceValue(0x51, 0);
    BattleEvent_CallHandlers(flow, 9);
    BattleEventVar_Pop(0xffb);
}

BOOL ServerControl_CheckMoveSteal(BtlServerFlow *flow, BattleMon *mon, u16 move, void *targets,
                                  BtlFlowReactionList *list) {
    u8 monId;
    u8 target;

    monId = 0x1f;
    target = 0x1f;
    ServerEvent_CheckMoveSteal(flow, mon, move, targets, &monId, &target);
    if (monId != 0x1f) {
        list->targets[0] = target != 0x1f ? GetBattlePos(&flow->posPoke, target) : 6;
        list->monIds[0] = monId;
        list->count = 1;
        return TRUE;
    }
    return FALSE;
}

BOOL ServerEvent_CheckMoveSteal(BtlServerFlow *flow, BattleMon *mon, u16 move, void *targets, u8 *monId, u8 *target) {
    u32 count;
    u32 i;

    count = BtlMonSet_GetCount(targets);
    BattleEventVar_Push(0x1038);
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(5, count);
    for (i = 0; i < count; i++) {
        BattleEventVar_SetConstValue(i + 6, GetMonID(BtlMonSet_Get(targets, i)));
    }
    BattleEventVar_SetRewriteOnceValue(2, 0x1f);
    BattleEventVar_SetValue(4, 0x1f);
    BattleEvent_CallHandlers(flow, 0x1a);
    *monId = BattleEventVar_GetValue(2);
    *target = BattleEventVar_GetValue(4);
    BattleEventVar_Pop(0x104b);
    if (*monId != 0x1f) {
        return TRUE;
    }
    return FALSE;
}

void ServerDisplay_MoveAnimation(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target) {
    BtlServerCmdQueue_Put(flow->queue, SC_MOVE_ANIM, MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(mon)),
                          target, move, 0);
}

BOOL IsPlayerMon(BtlServerFlow *flow, BattleMon *mon) {
    if (GetClientIDFromMonID(GetMonID(mon)) == GetPlayerClientID(flow->mainModule)) {
        return TRUE;
    }
    return FALSE;
}

void ServerControl_RecordMoveUse(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    if (IsPlayerMon(flow, mon)) {
        if (move == 0x96) {
            IncRecord(flow->mainModule, 0x49);
        } else if (move == 0xa5) {
            IncRecord(flow->mainModule, 0x4a);
        }
    }
}

BOOL ServerControl_ExecuteMove(BtlServerFlow *flow, BattleMon *mon, u16 move, void *targets) {
    s32 quality;
    u8 monId;
    BOOL success;
    BOOL isDamage;
    BOOL isDamage2;
    u32 reserved;
    u32 state;
    u32 result;

    quality = PML_MoveGetQuality(move);
    monId = GetMonID(mon);
    success = TRUE;
    isDamage = FALSE;
    isDamage2 = FALSE;
    ServerControl_RecordMoveUse(flow, mon, move);
    if (getMoveFlag(move, 1)) {
        switch (ServerControl_CheckChargeTurn(flow, mon, targets, move)) {
        case 0:
            break;
        case 1:
            return isDamage;
        case 2:
            return success;
        case 3:
            return isDamage;
        case 4:
            break;
        }
    }
    BtlMonSet_Clear(flow->unk860);
    reserved = SCQUE_RESERVE_Pos(flow->queue, SC_MOVE_ANIM);
    BattleMoveEffectState_SetTargets(flow->moveEffect, flow, mon, targets);
    state = PushState(&flow->actionState, 0x10b8);
    result = ServerEvent_MoveExecuteStart(flow, mon, move, targets);
    PopState(&flow->actionState, state, 0x10ba);
    if (result) {
        if (!flow->moveEffect->enabled) {
            flow->moveEffect->enabled = TRUE;
        }
        if (result == 2) {
            ServerControl_PutMoveAnimation(flow, move, flow->moveEffect, reserved);
            return TRUE;
        }
    }
    switch (quality) {
    case 0:
    case 4:
    case 6:
    case 7:
    case 8:
        isDamage = TRUE;
        isDamage2 = TRUE;
        break;
    case 9:
        isDamage2 = TRUE;
        break;
    }
    BtlMonSet_RemoveFainted(targets);
    ServerControl_RemoveOutOfRangeTargets(flow, flow->unk1AB0, mon, targets);
    ServerControl_RemoveSelfTarget(flow, mon, targets, flow->unk1AB0, isDamage);
    if (BtlMonSet_IsEmptyAfterTargeting(targets)) {
        if (!flow->unk78A_4) {
            ServerDisplay_MoveFailed(flow, mon, move);
        }
        success = FALSE;
    }
    if (success) {
        if (quality != 9) {
            ServerControl_RemoveHiddenTargets(flow, flow->unk1AB0, mon, targets);
        }
        EffectivenessTable_Init(flow->unk1F8C);
        if (isDamage2 || ServerEvent_CheckTypeImmunityEnabled(flow, flow->unk1AB0, mon)) {
            ServerControl_RemoveTypeImmuneTargets(flow, flow->unk1AB0, mon, targets, flow->unk1F8C);
        }
        ServerControl_RemoveNoEffectTargets(flow, flow->unk1AB0, mon, targets, flow->unk1F8C);
        if (quality != 9) {
            ServerControl_RemoveMissedTargets(flow, flow->unk1AB0, mon, targets);
        }
        if (BtlMonSet_IsEmptyAfterTargeting(targets)) {
            ServerControl_MoveExecuteNoEffect(flow, monId, move);
            success = FALSE;
        } else {
            ServerControl_RemoveSubstituteTargets(flow, flow->unk1AB0, mon, targets, isDamage2);
            if (BtlMonSet_IsEmptyAfterTargeting(targets)) {
                ServerDisplay_MoveFailed(flow, mon, move);
                ServerControl_MoveExecuteNoEffect(flow, monId, move);
                success = FALSE;
            }
        }
    }
    if (success) {
        if (isDamage) {
            ServerControl_DamageMove(flow, flow->unk1AB0, mon, targets, flow->unk1F8C, 0);
        } else {
            switch (quality) {
            case 2:
                ServerControl_SimpleEffect(flow, flow->unk1AB0, mon, targets);
                break;
            case 1:
                ServerControl_SimpleCondition(flow, move, mon, targets);
                break;
            case 5:
                ServerControl_StatChangeAndCondition(flow, flow->unk1AB0, mon, targets);
                break;
            case 9:
                ServerControl_OHKO(flow, flow->unk1AB0, mon, targets);
                break;
            case 0xc:
                ServerControl_ForceSwitch(flow, move, mon, targets);
                break;
            case 3:
                ServerControl_SimpleRecover(flow, move, mon, targets);
                break;
            case 0xa:
                ServerControl_FieldEffect(flow, flow->unk1AB0, mon);
                break;
            case 0xb:
            case 0xd:
                ServerControl_UncategorizedMove(flow, flow->unk1AB0, mon, targets);
                break;
            }
        }
        if (flow->moveEffect->enabled) {
            if (!flow->moveEffect->unk05_1) {
                ServerControl_PutMoveAnimation(flow, move, flow->moveEffect, reserved);
            }
            BtlWazaRec_MarkLastSucceeded(&flow->wazaRec);
            if (!IsFainted(mon) && getMoveFlag(move, 2)) {
                scPut_SetContFlag(flow, mon, 0xc);
            }
            ServerControl_MoveExecuteEffective(flow, monId, move);
        } else {
            ServerControl_MoveExecuteNoEffect(flow, monId, move);
            success = FALSE;
        }
    } else if (flow->moveEffect->enabled) {
        ServerControl_PutMoveAnimation(flow, move, flow->moveEffect, reserved);
    }
    ServerControl_MoveExecuteEnd(flow, monId, move);
    ServerControl_CheckFainted(flow, mon);
    return success;
}

void ServerControl_RemoveOutOfRangeTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon,
                                           void *targets) {
    BattleMon *target;

    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        if (IsTargetOutOfRange(flow, mon, target, param->move)) {
            BtlMonSet_Remove(targets, target);
            ServerDisplay_MoveAvoided(flow, target, param->move);
            flow->unk78A_4 = TRUE;
        }
    }
}

void ServerControl_RemoveSelfTarget(BtlServerFlow *flow, BattleMon *mon, void *targets, BtlFlowMoveParam *param,
                                    BOOL flag) {
    BattleMon *target;

    if (BtlMonSet_GetCountMax(targets) == 1) {
        target = BtlMonSet_Get(targets, 0);
        if (target != NULL && GetMonID(target) == GetMonID(mon) && param->targetType != 7 &&
            (GetTurnFlag(mon, 0xc) || flag)) {
            BtlMonSet_Remove(targets, target);
        }
    }
}

BOOL ServerEvent_CheckTypeImmunityEnabled(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon) {
    BOOL result;

    BattleEventVar_Push(0x11a4);
    BattleEventVar_SetValue(3, GetMonID(mon));
    BattleEventVar_SetValue(0x12, param->move);
    BattleEventVar_SetRewriteOnceValue(0x51, 0);
    BattleEvent_CallHandlers(flow, 0x3d);
    result = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0x11aa);
    return result;
}

void ServerControl_RemoveSubstituteTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets,
                                           BOOL flag) {
    BattleMon *target;

    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        if (mon != target && IsSubstituteActive(target) && !flag && !getMoveFlag(param->move, 0xd)) {
            BtlMonSet_Remove(targets, target);
        }
    }
}

void ServerControl_MoveExecuteEffective(BtlServerFlow *flow, u8 monId, u16 move) {
    u32 state;

    state = PushState(&flow->actionState, 0x11f7);
    ServerEvent_MoveExecuteEvent(flow, monId, move, 0x25);
    PopState(&flow->actionState, state, 0x11f9);
}

void ServerControl_MoveExecuteNoEffect(BtlServerFlow *flow, u8 monId, u16 move) {
    u32 state;

    state = PushState(&flow->actionState, 0x1206);
    ServerEvent_MoveExecuteEvent(flow, monId, move, 0x26);
    PopState(&flow->actionState, state, 0x1208);
}

void ServerControl_MoveExecuteEnd(BtlServerFlow *flow, u8 monId, u16 move) {
    u32 state;

    state = PushState(&flow->actionState, 0x1215);
    ServerEvent_MoveExecuteEvent(flow, monId, move, 0x27);
    PopState(&flow->actionState, state, 0x1217);
}

void ServerEvent_MoveExecuteEvent(BtlServerFlow *flow, u8 monId, u16 move, u32 event) {
    BattleEventVar_Push(0x1224);
    BattleEventVar_SetConstValue(2, monId);
    BattleEventVar_SetConstValue(0x12, move);
    BattleEvent_CallHandlers(flow, event);
    BattleEventVar_Pop(0x1228);
}

BOOL IsGuaranteedHit(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender) {
    u8 lockedId;

    if (BtlPosPoke_IsMonAtActivePos(&flow->posPoke, flow->mainModule, GetMonID(attacker)) &&
        GetBattleMonStat(attacker, 0x11) == 0x63) {
        return TRUE;
    }
    if (BtlPosPoke_IsMonAtActivePos(&flow->posPoke, flow->mainModule, GetMonID(defender)) &&
        GetBattleMonStat(defender, 0x11) == 0x63) {
        return TRUE;
    }
    if (CheckCondition(attacker, CONDITION_ALWAYS_HIT)) {
        return TRUE;
    }
    if (CheckCondition(attacker, CONDITION_LOCK_ON)) {
        lockedId = GetDisabledMove(attacker, CONDITION_LOCK_ON);
        if (lockedId == GetMonID(defender)) {
            return TRUE;
        }
    }
    return FALSE;
}

void ServerControl_RemoveTypeImmuneTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets,
                                           void *data) {
    BattleMon *target;
    BOOL hit;

    EffectivenessTable_Init(data);
    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        hit = ServerControl_CheckMoveEffectiveness(flow, mon, target, param, TRUE);
        EffectivenessTable_Set(data, GetMonID(target), hit);
        if (!hit) {
            BtlMonSet_Remove(targets, target);
            BtlServerFlow_CountEffectiveness(flow->unk1F80, flow, mon, target, 0);
            if (GetClientIDFromMonID(GetMonID(mon)) == GetPlayerClientID(flow->mainModule)) {
                IncRecord(flow->mainModule, 0x4b);
            }
        }
    }
}

void ServerControl_RemoveNoEffectTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets,
                                         void *data) {
    BattleMon *target;

    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        if (!IsGuaranteedHit(flow, mon, target) && ServerControl_CheckNoEffect(flow, param, mon, target, data, 0x2b)) {
            BtlMonSet_Remove(targets, target);
        }
    }
    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        if (ServerControl_CheckNoEffect(flow, param, mon, target, data, 0x2c)) {
            BtlMonSet_Remove(targets, target);
        }
    }
    if (getMoveFlag(param->move, 3)) {
        BtlMonSet_ResetIndex(targets);
        while ((target = BtlMonSet_GetNext(targets)) != NULL) {
            if (GetTurnFlag(target, 7) && !ServerEvent_CheckProtectBreak(flow, mon, target, param->move)) {
                BtlMonSet_Remove(targets, target);
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x20b, GetMonID(target), SC_MSG_ARG_END);
            }
        }
    }
    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        if (ServerControl_CheckNoEffect(flow, param, mon, target, data, 0x2d)) {
            BtlMonSet_Remove(targets, target);
        }
    }
}

BOOL ServerControl_CheckNoEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BattleMon *target,
                                 void *data, u32 event) {
    u32 state;
    BOOL result;
    BOOL silent;

    state = PushState(&flow->actionState, 0x12c0);
    silent = FALSE;
    result = ServerEvent_CheckNoEffect(flow, param, event, mon, target, data, &flow->message, &silent);
    if (result) {
        if (BattleHandler_StrIsEnabled(&flow->message)) {
            BattleHandler_SetString(flow, &flow->message);
            BattleHandler_StrClear(&flow->message);
        } else if (BattleHandler_Result(flow) == 0 && !silent) {
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0xd2, GetMonID(target), SC_MSG_ARG_END);
        }
    }
    PopState(&flow->actionState, state, 0x12da);
    return result;
}

BOOL ServerEvent_CheckNoEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, u32 event, BattleMon *mon,
                               BattleMon *target, void *data, BattleHandlerString *string, BOOL *silent) {
    u32 value;
    BOOL result;

    value = EffectivenessTable_GetValue(data, GetMonID(target));
    BattleHandler_StrClear(string);
    BattleEventVar_Push(0x12f2);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEventVar_SetConstValue(0x4e, param->flags.unk0);
    BattleEventVar_SetConstValue(0x3f, (s32)string);
    BattleEventVar_SetConstValue(0x38, value);
    BattleEventVar_SetRewriteOnceValue(0x40, 0);
    BattleEventVar_SetRewriteOnceValue(0x51, 0);
    BattleEvent_CallHandlers(flow, event);
    result = BattleEventVar_GetValue(0x40);
    *silent = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0x12ff);
    return result;
}

void ServerControl_RemoveHiddenTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets) {
    BattleMon *target;

    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        if (!IsGuaranteedHit(flow, mon, target) && ServerEvent_CheckSemiInvulnAvoid(flow, mon, target, param->move)) {
            BtlMonSet_Remove(targets, target);
            ServerDisplay_MoveAvoided(flow, target, param->move);
        }
    }
}

void ServerControl_RemoveMissedTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets) {
    BattleMon *target;

    if (BtlMonSet_GetCountMax(targets) != 1 || BtlMonSet_Get(targets, 0) != mon) {
        BtlMonSet_ResetIndex(targets);
        while ((target = BtlMonSet_GetNext(targets)) != NULL) {
            if (!ServerEvent_SkipAccuracyCheck(flow, mon, target, param) &&
                !ServerEvent_CheckHit(flow, mon, target, param)) {
                BtlMonSet_Remove(targets, target);
                ServerDisplay_MoveAvoided(flow, target, param->move);
            }
        }
    }
}

BOOL ServerEvent_SkipAccuracyCheck(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, BtlFlowMoveParam *param) {
    BOOL result;

    BattleEventVar_Push(0x1351);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetRewriteOnceValue(0x51, 0);
    BattleEvent_CallHandlers(flow, 0x1c);
    result = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0x1358);
    return result;
}

BOOL IsTargetOutOfRange(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move) {
    if (BtlSetup_GetBattleStyle(flow->mainModule) == 2) {
        u8 pos1, pos2;
        if (move != 0 && getMoveFlag(move, 0xb)) {
            return FALSE;
        }
        pos1 = MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(mon));
        pos2 = MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(target));
        if (!IsAdjacentOpponent(pos1, pos2)) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ServerEvent_CheckHit(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, BtlFlowMoveParam *param) {
    u8 ignore;
    s8 accuracy;
    u32 base;
    s8 evasion;
    u32 ratio;
    s32 stage;
    u32 chance;

    if (IsGuaranteedHit(flow, mon, target)) {
        return TRUE;
    }
    if (ServerEvent_CheckAlwaysHit(flow, mon, target, param->move)) {
        return TRUE;
    }
    if (CheckCondition(target, CONDITION_TELEKINESIS)) {
        return TRUE;
    }
    base = ServerEvent_GetMoveAccuracy(flow, mon, target, param);

    BattleEventVar_Push(0x13a6);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetRewriteOnceValue(0x4b, 0);
    BattleEventVar_SetValue(0x27, GetBattleMonStat(mon, 6));
    BattleEventVar_SetValue(0x28, GetBattleMonStat(target, 7));
    BattleEventVar_SetMulValue(0x35, 0x1000, 0x29, 0x20000);
    BattleEvent_CallHandlers(flow, 0x33);
    accuracy = BattleEventVar_GetValue(0x27);
    evasion = BattleEventVar_GetValue(0x28);
    ignore = BattleEventVar_GetValue(0x4b);
    if ((CheckCondition(target, CONDITION_FORESIGHT) && evasion > 6) || ignore) {
        evasion = 6;
    }
    if (GetTurnFlag(mon, 0xe)) {
        BattleEventVar_MulValue(0x35, 0x1333);
    }
    ratio = BattleEventVar_GetValue(0x35);
    BattleEventVar_Pop(0x13bf);

    stage = accuracy + 6 - evasion;
    if (stage < 0) {
        stage = 0;
    }
    if (stage > 12) {
        stage = 12;
    }
    chance = fixed_round(BtlCalc_ApplyAccuracyStage((u8)base, (u8)(s8)stage), ratio);
    if (chance > 100) {
        chance = 100;
    }
    if (ReturnZero(flow->mainModule, 6)) {
        chance = 100;
    }
    return BattleRandom(100) < (u8)chance;
}

void ServerControl_PutMoveAnimation(BtlServerFlow *flow, u16 move, BattleMoveEffectState *effect, u32 reserved) {
    u32 shown = effect->unk00 ? effect->unk00 : move;
    BtlServerCmdQueue_PutReserved(flow->queue, (u16)reserved, SC_MOVE_ANIM, effect->pos1, effect->pos2, shown,
                                  effect->index);
    effect->index = 0;
    effect->unk05_1 = 1;
}

u32 ServerControl_CheckChargeTurn(BtlServerFlow *flow, BattleMon *mon, void *targets, u16 move) {
    u32 state;
    u32 result;

    if (!CheckCondition(mon, CONDITION_CHARGE)) {
        u8 pos = MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(mon));
        if (ServerEvent_CheckChargeFail(flow, mon, targets)) {
            ServerDisplay_MoveFailed(flow, mon, move);
            return 1;
        }
        if (!ServerEvent_CheckChargeSkip(flow, mon, move)) {
            if (ServerControl_StartChargeTurn(flow, mon, pos, targets, move)) {
                BattleCondition cond = AddTurnCondition(2, move);
                ServerDisplay_AddCondition(flow, mon, 0x1a, cond);
            }
            return 2;
        }
        if (!ServerControl_StartChargeTurn(flow, mon, pos, targets, move)) {
            return 1;
        }
        state = PushState(&flow->actionState, 0x1414);
        ServerEvent_ChargeSkipFixed(flow, mon, move);
        PopState(&flow->actionState, state, 0x1416);
    }
    flow->moveEffect->index = 1;
    state = PushState(&flow->actionState, 0x141e);
    result = ServerEvent_ChargeRelease(flow, mon, targets, move);
    PopState(&flow->actionState, state, 0x1420);
    BattleMon_SetTurnFlag(mon, 0xb);
    ServerControl_ClearChargeState(flow, mon);
    return result ? 4 : 3;
}

BOOL ServerControl_StartChargeTurn(BtlServerFlow *flow, BattleMon *mon, u8 pos, void *targets, u16 move) {
    u32 state;
    BOOL result;
    BOOL failed = FALSE;
    u8 id = 0x1f;

    state = PushState(&flow->actionState, 0x1438);
    result = ServerEvent_ChargeStart(flow, mon, targets, move, &id, &failed);
    if (result) {
        u8 targetPos = flow->unk77F;
        if (BtlMonSet_GetCount(targets)) {
            targetPos = GetBattlePos(&flow->posPoke, GetMonID(BtlMonSet_Get(targets, 0)));
        }
        BtlServerCmdQueue_Put(flow->queue, SC_MOVE_ANIM, pos, targetPos, move, 0);
    }
    PopState(&flow->actionState, state, 0x1445);
    if (result) {
        state = PushState(&flow->actionState, 0x1449);
        ServerEvent_ChargeStartFixed(flow, mon);
        PopState(&flow->actionState, state, 0x144b);
        if (IsSemiInvulnMove(mon)) {
            BtlServerCmdQueue_Put(flow->queue, SC_SET_MON_VISIBLE, GetMonID(mon), 1);
        }
        if (id != 0x1f) {
            BtlServerCmdQueue_Put(flow->queue, SC_SET_MON_VISIBLE, id, 1);
        }
    } else if (!failed) {
        ServerDisplay_MoveFailed(flow, mon, move);
    }
    return result;
}

void ServerControl_ClearChargeState(BtlServerFlow *flow, BattleMon *mon) {
    u32 flag;
    if (CheckCondition(mon, CONDITION_CHARGE)) {
        ServerControl_CureCondition(flow, mon, CONDITION_CHARGE, 0);
        BattleMon_SetTurnFlag(mon, 0xb);
    }
    while ((flag = BattleMon_GetSemiInvulnFlag(mon)) != 0x10) {
        scPut_ResetContFlag(flow, mon, flag);
    }
}

BOOL ServerControl_SkyDropCatch(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BOOL *failed) {
    *failed = FALSE;
    if (!IsFainted(target) && !IsSubstituteActive(target) && !IsSemiInvulnMove(target)) {
        if (!GetTurnFlag(target, 7)) {
            u8 targetId = GetMonID(target);
            BattleCondition cond;

            scPut_SetContFlag(flow, attacker, 3);
            scPut_SetContFlag(flow, target, 3);
            ServerControl_SetMonCounter(flow, attacker, 4, targetId + 1);
            cond = MakeConditionDependPoke(GetMonID(attacker));
            ServerControl_AddCondition(flow, target, attacker, 0x21, cond, FALSE, FALSE, NULL);
            return TRUE;
        }
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x20b, GetMonID(target), SC_MSG_ARG_END);
        *failed = TRUE;
    }
    return FALSE;
}

void ServerControl_SkyDropCheckRelease(BtlServerFlow *flow, BattleMon *mon, BOOL flag) {
    u8 targetId = BtlFlow_GetSkyDropTarget(mon);

    if (targetId != 0x1f) {
        BattleMon *target = GetPokeParam(flow->pokeCon, targetId);
        if (CheckCondition(target, CONDITION_SKY_DROP)) {
            ServerControl_CureCondition(flow, target, CONDITION_SKY_DROP, 0);
            scPut_ResetContFlag(flow, target, 3);
            if (DoesBattleMonExist(&flow->posPoke, targetId)) {
                BtlServerCmdQueue_Put(flow->queue, SC_SET_MON_VISIBLE, targetId, 0);
                ServerDisplay_SkyDropTargetAppear(flow, target, 0x465);
            }
        }
        ServerControl_SetMonCounter(flow, mon, 4, 0);
        if (!flag && !IsFainted(mon)) {
            BtlServerCmdQueue_Put(flow->queue, SC_SET_MON_VISIBLE, GetMonID(mon), 0);
        }
    }
}

BOOL ServerControl_CheckMoveFail(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 flag) {
    u32 status;
    BOOL thawedByMove = FALSE;
    u32 cause = 0;

    ServerControl_CheckWakeUp(flow, mon, move);
    status = GetBattleMonStatus(mon);
    do {
        if (status == 2 && !ServerEvent_CanUseMoveAsleep(flow, mon, move, 2)) {
            cause = 2;
            break;
        }
        thawedByMove = ServerControl_CheckThaw(flow, mon, move);
        status = GetBattleMonStatus(mon);
        if (status == 3 && !thawedByMove) {
            cause = 4;
            break;
        }
        if (flow->unk1F7C == 4) {
            cause = 0x16;
            break;
        }
        if (flow->unk1F7C == 2) {
            cause = 0x17;
            break;
        }
        if (!flag) {
            u8 slot = BattleMon_FindMoveSlot(mon, move);
            if (slot != 4 && GetMovePP(mon, slot) == 0) {
                cause = 1;
                break;
            }
        }
        cause = ServerEvent_CheckMoveFail(flow, mon, move, 0x1e);
        if (cause != 0) {
            break;
        }
        if (GetTurnFlag(mon, 6)) {
            cause = 7;
            break;
        }
        if (GetTurnFlag(mon, 4)) {
            cause = 6;
            break;
        }
        if (CheckCondition(mon, CONDITION_DISABLE) && move != 0xa5) {
            BattleConditionCont cont = GetConditionContinuationParam(mon, CONDITION_DISABLE);
            if (move == Condition_GetParam(cont)) {
                cause = 9;
                break;
            }
        }
        if (CheckCondition(mon, CONDITION_HEAL_BLOCK) && getMoveFlag(move, 0xc)) {
            cause = 0xd;
            break;
        }
        if (IsFieldEffectActive(2) && getMoveFlag(move, 9)) {
            cause = 0x14;
            break;
        }
        if (move != 0xa5) {
            if (CheckCondition(mon, CONDITION_ENCORE)) {
                u16 prev = GetPreviousMoveUsed(mon);
                if (prev != 0xa5 && prev != move) {
                    cause = 0x11;
                    break;
                }
            }
            if (CheckCondition(mon, CONDITION_MOVE_LOCK) && move != GetPreviousMoveID(mon)) {
                cause = 0x11;
                break;
            }
            if (CheckCondition(mon, CONDITION_TAUNT) && !PML_MoveIsDamaging(move)) {
                cause = 0xa;
                break;
            }
            if (IsFieldEffectActive(3) && FieldStatusCheckImprison(flow->pokeCon, mon, move)) {
                cause = 0xc;
                break;
            }
        }
        if (ServerControl_CheckConfusion(flow, mon)) {
            cause = 5;
            break;
        }
        if (status == 1 && RollEffectChance(0x19)) {
            cause = 3;
            break;
        }
        if (ServerControl_CheckAttract(flow, mon)) {
            cause = 8;
            break;
        }
    } while (0);

    if (cause != 0) {
        ServerControl_MoveFail(flow, mon, move, cause);
        return TRUE;
    }
    if (status == 2) {
        ServerDisplay_MoveFailMessage(flow, mon, move, 2);
        ServerDisplay_AddEffectAtPosition(flow, mon, 0x256);
    } else if (thawedByMove && status == 3) {
        ServerControl_CureStatus(flow, mon, status, FALSE);
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x12f, GetMonID(mon), move, SC_MSG_ARG_END);
    }
    return FALSE;
}

BOOL ServerControl_CheckMoveFailLate(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    u32 cause;
    if (move != 0xa5) {
        cause = ServerEvent_CheckMoveFail(flow, mon, move, 0x1f);
        if (cause != 0) {
            ServerControl_MoveFail(flow, mon, move, cause);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ServerEvent_CanUseMoveAsleep(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 status) {
    BOOL result;
    BattleEventVar_Push(0x15b1);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x22, status);
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetRewriteOnceValue(0x41, 0);
    BattleEvent_CallHandlers(flow, 0x1d);
    result = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x15b8);
    return result;
}

u32 ServerEvent_CheckMoveFail(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 event) {
    u32 cause;
    BattleEventVar_Push(0x15cb);
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetRewriteOnceValue(0x22, 0);
    BattleEvent_CallHandlers(flow, event);
    cause = BattleEventVar_GetValue(0x22);
    BattleEventVar_Pop(0x15d1);
    return cause;
}

BOOL ServerControl_CheckConfusion(BtlServerFlow *flow, BattleMon *mon) {
    if (flow->unk1F7C == 3) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0xbc, GetMonID(mon), SC_MSG_ARG_END);
        return TRUE;
    }
    if (CheckCondition(mon, CONDITION_CONFUSION)) {
        if (BattleMon_TickConfusion(mon)) {
            ServerControl_CureCondition(flow, mon, CONDITION_CONFUSION, 0);
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x15f, GetMonID(mon), SC_MSG_ARG_END);
            return FALSE;
        }
        ServerDisplay_Confused(flow, mon);
        return RollEffectChance(0x32);
    }
    return FALSE;
}

BOOL ServerControl_CheckAttract(BtlServerFlow *flow, BattleMon *mon) {
    if (CheckCondition(mon, CONDITION_ATTRACT)) {
        ServerDisplay_InLove(flow, mon);
        if (RollEffectChance(0x32)) {
            return TRUE;
        }
    }
    return FALSE;
}

void ServerControl_CheckWakeUp(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    u32 status = GetBattleMonStatus(mon);
    if (status == 2 && BattleMon_TickSleep(mon)) {
        ServerControl_CureStatus(flow, mon, status, TRUE);
    }
}

BOOL ServerControl_CheckThaw(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    u32 status = GetBattleMonStatus(mon);
    BOOL thawed = FALSE;
    BOOL thawedByMove = FALSE;
    if (status == 3) {
        if (getMoveFlag(move, 0xa)) {
            thawedByMove = TRUE;
        } else if (RollEffectChance(0x14)) {
            thawed = TRUE;
        }
    }
    if (thawed) {
        ServerControl_CureStatus(flow, mon, status, TRUE);
    }
    return thawedByMove;
}

void ServerControl_MoveFail(BtlServerFlow *flow, BattleMon *mon, u16 move, s32 cause) {
    u32 state;

    if (cause == 5) {
        ServerControl_ConfusionDamage(flow, mon);
        if (ServerControl_CheckFainted(flow, mon) && ServerControl_CheckMatchup(flow)) {
            return;
        }
    } else {
        ServerDisplay_MoveFailMessage(flow, mon, move, cause);
        switch (cause) {
        case 2:
            ServerDisplay_AddEffectAtPosition(flow, mon, 0x256);
            break;
        case 3:
            ServerDisplay_AddEffectAtPosition(flow, mon, 0x25a);
            break;
        case 4:
            ServerDisplay_AddEffectAtPosition(flow, mon, 0x259);
            break;
        case 0x17: {
            BattleCondition cond = BtlCalc_MakeStatusCondition(2);
            BtlServerCmdQueue_Put(flow->queue, SC_SET_MON_VISIBLE, GetMonID(mon), 0);
            ServerDisplay_AddCondition(flow, mon, 2, cond);
            ServerDisplay_AddEffectAtPosition(flow, mon, 0x256);
            break;
        }
        }
    }
    state = PushState(&flow->actionState, 0x1670);
    ServerEvent_MoveExecuteFail(flow, mon, move, cause);
    PopState(&flow->actionState, state, 0x1672);
}

void ServerDisplay_MoveFailMessage(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 cause) {
    u8 monId = GetMonID(mon);

    switch (cause) {
    case 2:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x135, monId, SC_MSG_ARG_END);
        break;
    case 3:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x114, monId, SC_MSG_ARG_END);
        break;
    case 4:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x123, monId, SC_MSG_ARG_END);
        break;
    case 6:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x16b, monId, SC_MSG_ARG_END);
        break;
    case 7:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x16e, monId, SC_MSG_ARG_END);
        break;
    case 8:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x150, monId, SC_MSG_ARG_END);
        break;
    case 9:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x253, monId, move, SC_MSG_ARG_END);
        break;
    case 0x12:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x1bd, monId, SC_MSG_ARG_END);
        break;
    case 0xa:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x23b, monId, move, SC_MSG_ARG_END);
        break;
    case 0xb:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x244, monId, move, SC_MSG_ARG_END);
        break;
    case 0xc:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x24d, monId, move, SC_MSG_ARG_END);
        break;
    case 0xd:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x37a, monId, move, SC_MSG_ARG_END);
        break;
    case 0xe:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x37d, monId, SC_MSG_ARG_END);
        break;
    case 0xf:
        BtlServerCmdQueue_Put(flow->queue, SC_ABILITY_POPUP_IN, monId);
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x1c3, monId, SC_MSG_ARG_END);
        BtlServerCmdQueue_Put(flow->queue, SC_ABILITY_POPUP_OUT, monId);
        break;
    case 0x14:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x43e, monId, move, SC_MSG_ARG_END);
        break;
    case 0x16:
        if (CheckCondition(mon, CONDITION_SLEEP)) {
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0xc2, monId, SC_MSG_ARG_END);
        } else {
            u8 index = BattleRandom(4);
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0xbc + index, monId, SC_MSG_ARG_END);
        }
        break;
    case 0x17:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0xc1, monId, SC_MSG_ARG_END);
        break;
    case 1:
        ServerDisplay_MoveMessage(flow, mon, move);
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x52, monId, SC_MSG_ARG_END);
        break;
    case 0x13:
    case 0x19:
        break;
    default:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x47, monId, SC_MSG_ARG_END);
        break;
    }
}

void ServerControl_SetMonCounter(BtlServerFlow *flow, BattleMon *mon, u32 counter, u8 value) {
    BtlServerCmdQueue_Put(flow->queue, SC_OP_SET_COUNTER, GetMonID(mon), (u8)counter, value);
    COUNTER_Set(mon, counter, value);
}

void ServerControl_ConsumePP(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 moveSlot, void *targets) {
    u8 amount = ServerEvent_CalcPPDecrement(flow, mon, moveSlot, move, targets);
    ServerControl_DecrementPP(flow, mon, moveSlot, amount);
}

u32 ServerEvent_CalcPPDecrement(BtlServerFlow *flow, BattleMon *mon, u8 moveSlot, u16 move, void *targets) {
    u32 amount;
    BattleMon *target;
    u32 i;

    BattleEventVar_Push(0x16ff);
    i = 0;
    BattleEventVar_SetConstValue(5, BtlMonSet_GetCount(targets));
    while ((target = BtlMonSet_Get(targets, i)) != NULL) {
        BattleEventVar_SetConstValue(6 + i, GetMonID(target));
        i++;
    }
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetConstValue(0x17, moveSlot);
    BattleEventVar_SetValue(0x20, 1);
    BattleEvent_CallHandlers(flow, 0x4e);
    amount = BattleEventVar_GetValue(0x20);
    BattleEventVar_Pop(0x1714);
    return amount;
}

BOOL ServerControl_DecrementPP(BtlServerFlow *flow, BattleMon *mon, u8 moveIndex, u8 amount) {
    u8 pp = GetMovePP(mon, moveIndex);
    if (amount >= pp) {
        amount = pp;
    }
    if (amount != 0) {
        ServerControl_DecrementPPCore(flow, mon, moveIndex, amount);
        return TRUE;
    }
    return FALSE;
}

void ServerControl_DecrementPPCore(BtlServerFlow *flow, BattleMon *mon, u8 moveIndex, u8 amount) {
    if (!ReturnZero(flow->mainModule, 5)) {
        u8 monId = GetMonID(mon);
        Move_DecrementPP(mon, moveIndex, amount);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_SUBTRACT_PP, monId, moveIndex, amount);
        Move_SetUsed(mon, moveIndex);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_SET_MOVE_USED, monId, moveIndex);
    }
}

void ServerControl_DamageMove(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets, void *data,
                              u32 arg5) {
    u32 reserved;
    u32 damage = 0;
    BOOL showEffect = FALSE;

    flow->unk1FEC[1] = damage;
    ServerControl_DamageProcessStart(flow, mon, param);
    BtlMonSet_Clear(flow->unk860);
    ServerEvent_CheckMultihitHits(flow, mon, param->move, flow->unk4B4);
    ServerControl_CheckDamageToRecover(flow, mon, param, targets);
    if (BtlMonSet_GetCount(targets)) {
        if (BtlFlowHitWork_IsMultiHit(flow->unk4B4) && BtlMonSet_GetCountMax(targets) == 1) {
            damage = ServerControl_DamageMultiHit(flow, param, mon, targets, data);
        } else {
            damage = ServerControl_DamageAllTargets(flow, param, mon, targets, data, &reserved, arg5);
            showEffect = TRUE;
        }
        ServerControl_CalcRecoil(flow, mon, param->move, damage);
    }
    ServerControl_DamageProcessEnd(flow, param, mon, flow->unk860, damage, arg5);
    if (showEffect) {
        BattleMoveEffectState *effect = flow->moveEffect;
        if (!effect->enabled) {
            effect->enabled = 1;
            effect->unk05_1 = 1;
        }
        ServerControl_PutMoveAnimation(flow, param->move, flow->moveEffect, reserved);
    }
}

void ServerControl_CalcDamageList(BtlServerFlow *flow, BattleMon *attacker, void *targets,
                                  const BtlFlowMoveParam *param, void *effectiveness, u32 arg5,
                                  BtlFlowDamageList *list) {
    u16 damage;
    u16 adjusted;
    u32 i;
    BattleMon *target;

    list->count = 0;
    i = 0;
    list->substituteCount = 0;
    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        list->entries[i].monId = GetMonID(target);
        list->entries[i].critical = ServerEvent_CheckCritical(flow, attacker, target, param->move);
        list->entries[i].effectiveness = EffectivenessTable_Get(effectiveness, list->entries[i].monId);
        list->entries[i].fixedDamage =
            ServerEvent_CalcDamage(flow, attacker, target, param, list->entries[i].effectiveness, arg5,
                                   list->entries[i].critical, FALSE, &damage);
        list->entries[i].damage = damage;
        if (list->entries[i].fixedDamage) {
            list->entries[i].critical = FALSE;
            list->entries[i].effectiveness = 3;
        }
        list->entries[i].substitute = IsSubstituteActive(target);
        if (!list->entries[i].substitute) {
            adjusted = ClampDamageToHP(target, list->entries[i].damage);
            list->entries[i].unk9 = ServerEvent_CheckEndure(flow, attacker, target, 1, &adjusted);
            list->entries[i].damage = adjusted;
            list->count++;
        } else {
            list->entries[i].damage = damage;
            list->entries[i].unk9 = 0;
            list->substituteCount++;
        }
        BtlServerFlow_CountEffectiveness(flow->unk1F80, flow, attacker, target, list->entries[i].effectiveness);
        i++;
    }
    list->total = i;
}

u32 BtlFlowDamageList_GetCount(const BtlFlowDamageList *list) {
    return list->total;
}

u32 BtlFlowDamageList_GetSubstituteHits(BtlServerFlow *flow, const BtlFlowDamageList *list, BattleMon **mons,
                                        u16 *damages, u32 *effectiveness, u8 *critical) {
    u32 i;
    u32 count;

    for (i = 0, count = 0; i < list->total; i++) {
        if (list->entries[i].substitute) {
            mons[count] = GetPokeParam(flow->pokeCon, list->entries[i].monId);
            damages[count] = list->entries[i].damage;
            effectiveness[count] = list->entries[i].effectiveness;
            critical[count] = list->entries[i].critical;
            count++;
        }
    }
    return count;
}

u32 BtlFlowDamageList_GetMons(BtlServerFlow *flow, const BtlFlowDamageList *list, BattleMon **mons) {
    u32 i;
    for (i = 0; i < list->total; i++) {
        mons[i] = GetPokeParam(flow->pokeCon, list->entries[i].monId);
    }
    return list->total;
}

u32 BtlFlowDamageList_GetDirectHits(BtlServerFlow *flow, const BtlFlowDamageList *list, BattleMon **mons, u16 *damages,
                                    u32 *effectiveness, u8 *critical, u8 *unk9) {
    u32 i;
    u32 count;

    for (i = 0, count = 0; i < list->total; i++) {
        if (!list->entries[i].substitute) {
            mons[count] = GetPokeParam(flow->pokeCon, list->entries[i].monId);
            damages[count] = list->entries[i].damage;
            effectiveness[count] = list->entries[i].effectiveness;
            critical[count] = list->entries[i].critical;
            unk9[count] = list->entries[i].unk9;
            count++;
        }
    }
    return count;
}

u32 BtlFlowDamageList_GetTotalDamage(const BtlFlowDamageList *list) {
    u32 i;
    u32 total = 0;
    for (i = 0; i < list->total; i++) {
        total += list->entries[i].damage;
    }
    return total;
}

u32 ServerControl_DamageAllTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, void *targets,
                                   void *data, u32 *reserved, u32 arg6) {
    BtlFlowDamageFlags flags;
    u32 ratio;
    u32 damage;

    if (BtlMonSet_GetCountMax(targets) == 1) {
        ratio = 0x1000;
    } else {
        ratio = 0xc00;
    }
    damage = 0;
    flags.multipleTargets = BtlMonSet_GetCount(targets) > 1;
    flags.unk1 = arg6;
    BtlMonSet_CopyAllies(targets, attacker, flow->unk858);
    BtlMonSet_CopyOpponents(targets, attacker, flow->unk85C);
    ServerControl_CalcDamageList(flow, attacker, flow->unk858, param, data, ratio, flow->unk86C);
    ServerControl_CalcDamageList(flow, attacker, flow->unk85C, param, data, ratio, flow->unk870);
    *reserved = SCQUE_RESERVE_Pos(flow->queue, SC_MOVE_ANIM);
    if (BtlMonSet_GetCount(flow->unk858)) {
        damage +=
            ServerControl_DamageList(flow, param, attacker, flow->unk858, flow->unk86C, flow->unk4B4, ratio, flags);
        if (damage != 0 && IsPlayerMon(flow, attacker)) {
            IncRecord(flow->mainModule, 0x4d);
        }
    }
    if (BtlMonSet_GetCount(flow->unk85C)) {
        damage +=
            ServerControl_DamageList(flow, param, attacker, flow->unk85C, flow->unk870, flow->unk4B4, ratio, flags);
    }
    return damage;
}

u32 ServerControl_DamageMultiHit(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, void *targets,
                                 void *data) {
    u32 hit;
    u32 hits;
    u32 damage = 0;
    u32 status;
    u8 targetPos = 6;
    BattleMon *target = BtlMonSet_Get(targets, 0);

    flow->unk789 = GetMonID(target);
    if (IsBattleSceneOff(flow->mainModule)) {
        targetPos = GetBattlePos(&flow->posPoke, GetMonID(target));
        if (targetPos != 6) {
            BtlServerCmdQueue_Put(flow->queue, SC_EFFECT_BETWEEN_POS, GetBattlePos(&flow->posPoke, GetMonID(attacker)),
                                  targetPos, 0x27e);
        }
    }
    {
        BtlFlowDamageFlags flags = { 0 };
        for (hit = 0, hits = 0; hit < flow->unk4B4->count; hit++) {
            BattleMoveEffectState *effect;

            status = GetBattleMonStatus(attacker);
            effect = flow->moveEffect;
            if (!effect->enabled) {
                effect->enabled = 1;
                effect->unk05_1 = 1;
            }
            BtlServerCmdQueue_Put(flow->queue, SC_MOVE_ANIM, flow->moveEffect->pos1, flow->moveEffect->pos2,
                                  param->move, 0);
            ServerControl_CalcDamageList(flow, attacker, targets, param, data, 0x1000, flow->unk870);
            damage +=
                ServerControl_DamageList(flow, param, attacker, targets, flow->unk870, flow->unk4B4, 0x1000, flags);
            hits++;
            if (IsFainted(target) || IsFainted(attacker)) {
                break;
            }
            ServerControl_CheckItemReaction(flow, target, 1);
            if (GetBattleMonStatus(attacker) == 2 && status != 2) {
                break;
            }
            if (flow->unk4B4->unk02 && !ServerEvent_CheckHit(flow, attacker, target, param)) {
                break;
            }
        }
    }
    if (hits != 0) {
        ServerDisplay_EffectivenessMessage(flow, flow->unk4B4->unk05);
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x20, hits, SC_MSG_ARG_END);
    }
    if (IsFainted(target)) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0, GetMonID(target), SC_MSG_ARG_END);
    }
    if (targetPos != 6) {
        BtlServerCmdQueue_Put(flow->queue, SC_EFFECT_BY_POS, GetBattlePos(&flow->posPoke, GetMonID(attacker)), 0x291);
    }
    flow->unk789 = 0x1f;
    return damage;
}

BOOL BtlFlowHitWork_IsFirstHit(BtlFlowHitWork *hitWork) {
    if (hitWork->unk01 == 0) {
        return TRUE;
    }
    return FALSE;
}

u8 BtlFlowHitWork_IsMultiHit(BtlFlowHitWork *hitWork) {
    return hitWork->unk03;
}

void BtlFlowHitWork_SetEffectiveness(BtlFlowHitWork *hitWork, u32 value) {
    if (hitWork->unk05 == 3) {
        hitWork->unk05 = value;
    }
}

u32 ServerControl_DamageList(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, void *monSet,
                             BtlFlowDamageList *list, BtlFlowHitWork *hitWork, u32 ratio, BtlFlowDamageFlags flags) {
    u8 total = BtlFlowDamageList_GetCount(list);
    if (total) {
        u32 damage = ServerControl_DamageListCore(flow, attacker, monSet, list, param, hitWork, ratio, flags);
        ServerControl_AfterDamage(flow, attacker, param, damage);
        return damage;
    }
    return 0;
}

u32 ServerControl_DamageListCore(BtlServerFlow *flow, BattleMon *attacker, void *monSet, BtlFlowDamageList *list,
                                 BtlFlowMoveParam *param, BtlFlowHitWork *hitWork, u32 ratio,
                                 BtlFlowDamageFlags flags) {
    int i;
    u8 hitCount;
    u32 total;
    u8 attackerPos;

    attackerPos = GetBattlePos(&flow->posPoke, GetMonID(attacker));
    total = BtlFlowDamageList_GetTotalDamage(list);
    hitCount = BtlFlowDamageList_GetSubstituteHits(flow, list, sHitMons, sHitDamages, sHitEffectiveness, sHitCritical);
    for (i = 0; i < hitCount; i++) {
        if (IsSubstituteActive(sHitMons[i])) {
            u16 dealt = ServerControl_DamageSubstitute(flow, attacker, sHitMons[i], sHitDamages[i],
                                                       sHitEffectiveness[i], sHitCritical[i], param);
            BtlMonSet_AddDamage(flow->unk860, sHitMons[i], dealt, 1);
            total -= sHitDamages[i] - dealt;
        }
    }
    if (BtlFlowHitWork_IsFirstHit(hitWork)) {
        u8 count = BtlFlowDamageList_GetMons(flow, list, sHitMons);
        for (i = 0; i < count; i++) {
            ServerControl_DamageDetermine(flow, attacker, sHitMons[i], param);
        }
    }
    hitWork->unk01++;
    hitCount =
        BtlFlowDamageList_GetDirectHits(flow, list, sHitMons, sHitDamages, sHitEffectiveness, sHitCritical, sHitUnk9);
    if (hitCount == 0) {
        return total;
    }
    ServerControl_BeforeDamageDisplay(flow, param, attacker, hitCount, sHitMons);
    ServerDisplay_Damage(flow, param, hitCount, sHitEffectiveness, sHitMons, sHitDamages, sHitCritical,
                         flags.multipleTargets);
    if (!BtlFlowHitWork_IsMultiHit(hitWork)) {
        ServerDisplay_EffectivenessMessageMulti(flow, hitCount, sHitEffectiveness, sHitMons, flags.multipleTargets);
    } else {
        BtlFlowHitWork_SetEffectiveness(hitWork, sHitEffectiveness[0]);
    }
    ServerDisplay_CriticalHitMessage(flow, hitCount, sHitMons, sHitCritical, flags.multipleTargets);
    for (i = 0; i < hitCount; i++) {
        BtlMonSet_AddDamage(flow->unk860, sHitMons[i], sHitDamages[i], 0);
        ServerControl_RecordDamage(flow, attackerPos, attacker, sHitMons[i], param, sHitDamages[i]);
        BattleMon_SetTurnFlag(sHitMons[i], 2);
    }
    for (i = 0; i < hitCount; i++) {
        if (sHitUnk9[i]) {
            ServerControl_Endure(flow, sHitMons[i], sHitUnk9[i]);
        }
    }
    for (i = 0; i < hitCount; i++) {
        ServerControl_DamageFlinch(flow, param, attacker, sHitMons[i]);
        ServerControl_DamageDrain(flow, param, attacker, sHitMons[i], sHitDamages[i]);
        ServerControl_DamageAdditionalEffect(flow, param, attacker, sHitMons[i], sHitDamages[i], 0);
        ServerControl_DamageReaction(flow, attacker, sHitMons[i], param, sHitEffectiveness[i], sHitDamages[i],
                                     sHitCritical[i], 0);
    }
    for (i = 0; i < hitCount; i++) {
        ServerControl_CheckFainted(flow, sHitMons[i]);
    }
    ServerControl_CheckFainted(flow, attacker);
    return total;
}

void ServerControl_CheckDamageToRecover(BtlServerFlow *flow, BattleMon *attacker, BtlFlowMoveParam *param,
                                        void *targets) {
    BattleMon *target;

    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        if (ServerEvent_CheckDamageToRecover(flow, attacker, target, param)) {
            BattleMoveEffectState *effect;
            u32 state;

            effect = flow->moveEffect;
            if (!effect->enabled) {
                effect->enabled = 1;
            }
            state = PushState(&flow->actionState, 0x198b);
            ServerEvent_ApplyDamageToRecover(flow, attacker, target);
            PopState(&flow->actionState, state, 0x198f);
            BtlMonSet_Remove(targets, target);
        }
    }
}

void ServerDisplay_EffectivenessMessage(BtlServerFlow *flow, s32 effectiveness) {
    if (effectiveness < 3) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x4f, SC_MSG_ARG_END);
    } else if (effectiveness > 3) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x4e, SC_MSG_ARG_END);
    }
}

u32 ClampDamageToHP(BattleMon *mon, u32 damage) {
    u32 hp = GetBattleMonStat(mon, 0xd);
    if (damage > hp) {
        damage = hp;
    }
    return damage;
}

void ServerControl_DamageDetermine(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target,
                                   BtlFlowMoveParam *param) {
    u32 state = PushState(&flow->actionState, 0x19ba);
    ServerEvent_DamageDetermine(flow, attacker, target, param);
    PopState(&flow->actionState, state, 0x19be);
}

void ServerEvent_DamageDetermine(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BtlFlowMoveParam *param) {
    BattleEventVar_Push(0x19cd);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEvent_CallHandlers(flow, 0x45);
    BattleEventVar_Pop(0x19d4);
}

u32 ServerEvent_CheckEndure(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u32 arg3, u16 *damage) {
    u32 cause;
    int hp = GetBattleMonStat(target, 0xd);

    if (hp > *damage) {
        return 0;
    }
    BattleEventVar_Push(0x19ef);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x51, arg3);
    BattleEventVar_SetRewriteOnceValue(0x3a, 0);
    BattleEvent_CallHandlers(flow, 0x74);
    cause = BattleEventVar_GetValue(0x3a);
    BattleEventVar_Pop(0x19f6);
    if (cause != 0) {
        *damage = GetBattleMonStat(target, 0xd) - 1;
    }
    return cause;
}

void ServerControl_Endure(BtlServerFlow *flow, BattleMon *mon, u32 cause) {
    u32 state;
    u8 monId = GetMonID(mon);

    switch (cause) {
    case 1:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x202, monId, SC_MSG_ARG_END);
        break;
    case 3:
    default:
        state = PushState(&flow->actionState, 0x1a14);
        ServerEvent_EndureUsed(flow, mon, cause);
        PopState(&flow->actionState, state, 0x1a17);
        break;
    }
}

void ServerEvent_EndureUsed(BtlServerFlow *flow, BattleMon *mon, u32 cause) {
    BattleEventVar_Push(0x1a27);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0x75);
    BattleEventVar_Pop(0x1a2a);
}

void ServerControl_BeforeDamageDisplay(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, u8 count,
                                       BattleMon **mons) {
    u32 state = PushState(&flow->actionState, 0x1a39);
    ServerEvent_BeforeDamageDisplay(flow, param, attacker, count, mons);
    PopState(&flow->actionState, state, 0x1a3e);
}

void ServerEvent_BeforeDamageDisplay(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, u8 count,
                                     BattleMon **mons) {
    u32 i;

    BattleEventVar_Push(0x1a4f);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(5, count);
    for (i = 0; i < count; i++) {
        BattleEventVar_SetConstValue(6 + i, GetMonID(mons[i]));
    }
    BattleEvent_CallHandlers(flow, 0x44);
    BattleEventVar_Pop(0x1a57);
}

void ServerControl_RecordDamage(BtlServerFlow *flow, u8 attackerPos, BattleMon *attacker, BattleMon *target,
                                BtlFlowMoveParam *param, u16 damage) {
    u8 attackerId = GetMonID(attacker);
    u8 targetId = GetMonID(target);
    BattleMonDamageRecord record;

    BattleMonDamageRecord_Init(&record, attackerId, attackerPos, param->move, param->type, damage);
    BattleMon_AddDamageRecord(target, &record);
    BtlServerCmdQueue_Put(flow->queue, SC_OP_ADD_DAMAGE_RECORD, targetId, attackerId, attackerPos, param->type,
                          param->move, damage);
}

void ServerControl_DamageAdditionalEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker,
                                          BattleMon *target, u16 damage, BOOL flag) {
    switch (PML_MoveGetQuality(param->move)) {
    case 7:
        ServerEvent_DamageAddEffect(flow, param, attacker, attacker);
        break;
    case 6:
        if (!flag) {
            ServerEvent_DamageAddEffect(flow, param, attacker, target);
        }
        break;
    case 4:
        if (!flag) {
            ServerControl_DamageAddCondition(flow, param, attacker, target);
        }
        break;
    }
}

void ServerControl_AfterDamage(BtlServerFlow *flow, BattleMon *attacker, BtlFlowMoveParam *param, u32 damage) {
    u32 state = PushState(&flow->actionState, 0x1aa2);
    ServerEvent_AfterDamage(flow, attacker, param, damage);
    PopState(&flow->actionState, state, 0x1aa5);
}

void ServerEvent_AfterDamage(BtlServerFlow *flow, BattleMon *attacker, BtlFlowMoveParam *param, u32 damage) {
    BattleEventVar_Push(0x1ab7);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEventVar_SetConstValue(0x32, damage);
    BattleEvent_CallHandlers(flow, 0x4d);
    BattleEventVar_Pop(0x1abd);
}

void ServerControl_CheckItemReaction(BtlServerFlow *flow, BattleMon *mon, u32 reaction) {
    if (GetBattleMonHeldItem(mon) != 0) {
        u32 state = PushState(&flow->actionState, 0x1acc);
        ServerEvent_CheckItemReaction(flow, mon, reaction);
        PopState(&flow->actionState, state, 0x1ad2);
    }
}

void ServerEvent_CheckItemReaction(BtlServerFlow *flow, BattleMon *mon, u32 reaction) {
    BattleEventVar_Push(0x1adf);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x2e, reaction);
    BattleEvent_CallHandlers(flow, 0x91);
    BattleEventVar_Pop(0x1ae3);
}

void ServerControl_DamageProcessStart(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param) {
    u32 state = PushState(&flow->actionState, 0x1aea);
    ServerEvent_DamageProcessStart(flow, mon, param);
    PopState(&flow->actionState, state, 0x1aec);
}

void ServerEvent_DamageProcessStart(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param) {
    BattleEventVar_Push(0x1afa);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEvent_CallHandlers(flow, 0x81);
    BattleEventVar_Pop(0x1b01);
}

void ServerControl_DamageProcessEnd(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *monSet,
                                    u32 damage, u32 arg5) {
    u32 state;
    BattleMon *target;

    ServerControl_CheckFireThaw(flow, param, mon, monSet);
    state = PushState(&flow->actionState, 0x1b17);
    ServerEvent_DamageProcessEndSub(flow, mon, monSet, param, arg5, FALSE, 0x84);
    ServerEvent_DamageProcessEndSub(flow, mon, monSet, param, arg5, TRUE, 0x83);
    ServerEvent_DamageProcessEndSub(flow, mon, monSet, param, arg5, FALSE, 0x85);
    BtlMonSet_ResetIndex(monSet);
    while ((target = BtlMonSet_GetNext(monSet)) != NULL) {
        ServerControl_CheckItemReaction(flow, target, 1);
    }
    ServerEvent_DamageProcessEndSub(flow, mon, monSet, param, arg5, FALSE, 0x86);
    ServerEvent_DamageProcessEndSub(flow, mon, monSet, param, arg5, TRUE, 0x87);
    ServerEvent_DamageProcessEnd(flow, mon, monSet, param, arg5);
    PopState(&flow->actionState, state, 0x1b2b);
}

void ServerEvent_DamageProcessEndSub(BtlServerFlow *flow, BattleMon *mon, void *monSet, BtlFlowMoveParam *param,
                                     u32 arg4, BOOL flag, u32 event) {
    u32 value;
    u32 total;
    u32 count;
    u32 i;
    u32 hits;
    BattleMon *target;

    count = BtlMonSet_GetCount(monSet);
    total = 0;
    hits = 0;
    BattleEventVar_Push(0x1b3b);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x4d, arg4);
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x16, param->type);
    for (i = 0; i < count; i++) {
        BOOL found;
        target = BtlMonSet_Get(monSet, i);
        if (flag) {
            found = BtlMonSet_GetHpDamage(monSet, target, &value);
        } else {
            found = BtlMonSet_GetTotalDamage(monSet, target, &value);
        }
        if (found) {
            total += value;
            BattleEventVar_SetConstValue(6 + hits, GetMonID(target));
            hits++;
        }
    }
    if (hits != 0) {
        BattleEventVar_SetConstValue(5, hits);
        BattleEventVar_SetRewriteOnceValue(0x47, 0);
        BattleEventVar_SetRewriteOnceValue(0x48, 0);
        BattleEventVar_SetConstValue(0x32, total);
        BattleEvent_CallHandlers(flow, 0x82);
        if (!BattleEventVar_GetValue(0x48)) {
            BattleEvent_CallHandlers(flow, event);
        }
    }
    BattleEventVar_Pop(0x1b62);
}

void ServerEvent_DamageProcessEnd(BtlServerFlow *flow, BattleMon *mon, void *monSet, BtlFlowMoveParam *param,
                                  u32 arg4) {
    BattleEventVar_Push(0x1b71);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x4d, arg4);
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEvent_CallHandlers(flow, 0x88);
    BattleEventVar_Pop(0x1b76);
}

void ServerControl_DamageFlinch(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, BattleMon *target) {
    ServerControl_FlinchCore(flow, target, ServerEvent_GetFlinchChance(flow, param->move, attacker));
}

void ServerControl_CheckFireThaw(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *monSet) {
    BattleMon *target;

    if (param->type == 9) {
        BtlMonSet_ResetIndex(monSet);
        while ((target = BtlMonSet_GetNext(monSet)) != NULL) {
            if (GetBattleMonStatus(target) == 3 && !IsSubstituteActive(target)) {
                ServerControl_CureStatus(flow, target, 3, TRUE);
            }
        }
    }
}

BOOL ServerControl_FlinchCore(BtlServerFlow *flow, BattleMon *mon, u32 chance) {
    if (GetTurnFlag(mon, 5)) {
        BattleMon_SetTurnFlag(mon, 6);
        return TRUE;
    }
    if (ServerEvent_CheckFlinch(flow, mon, chance)) {
        BattleMon_SetTurnFlag(mon, 4);
        return TRUE;
    }
    if (chance >= 100) {
        u32 state = PushState(&flow->actionState, 0x1bbd);
        ServerEvent_FlinchFail(flow, mon);
        PopState(&flow->actionState, state, 0x1bbf);
    }
    return FALSE;
}

void ServerControl_DamageDrain(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, BattleMon *target,
                               u32 damage) {
    if (PML_MoveGetQuality(param->move) == 8) {
        if (ServerControl_DrainCore(flow, attacker, target,
                                    MultiplyValueByRatio(damage, PML_MoveGetParam(param->move, MOVE_PARAM_DRAIN)))) {
            ServerDisplay_SkyDropTargetAppear(flow, target, 0x383);
        }
    }
}

BOOL ServerControl_DrainCore(BtlServerFlow *flow, BattleMon *mon, BattleMon *source, u16 amount) {
    u32 state;
    BOOL result;

    state = PushState(&flow->actionState, 0x1c02);
    result = FALSE;
    amount = ServerEvent_CalcDrainAmount(flow, mon, source, amount);
    if (amount != 0 && !ServerControl_RecoverHPCheckFail(flow, mon)) {
        result = ServerControl_RecoverHP(flow, mon, amount, TRUE);
    }
    PopState(&flow->actionState, state, 0x1c10);
    return result;
}

BOOL ServerEvent_CalcDamage(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender,
                            const BtlFlowMoveParam *param, u32 effectiveness, u32 ratio, BOOL critical, BOOL fixedRoll,
                            u16 *damage) {
    u32 attack;
    u32 power;
    u32 category;
    BOOL fixed;
    u32 result;
    u32 value;
    u32 defense;

    category = PML_MoveGetCategory(param->move);
    fixed = FALSE;
    BattleEventVar_Push(0x1c31);
    BattleEventVar_SetConstValue(0x38, effectiveness);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(defender));
    BattleEventVar_SetConstValue(0x45, critical);
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x1a, category);
    BattleEventVar_SetValue(0x37, 0);
    BattleEvent_CallHandlers(flow, 0x46);
    result = BattleEventVar_GetValue(0x37);
    if (result != 0) {
        fixed = TRUE;
    } else {
        fx32 mod;
        u32 level;

        power = ServerEvent_GetMovePower(flow, attacker, defender, param);
        attack = ServerEvent_GetAttackPower(flow, attacker, defender, param, critical);
        defense = ServerEvent_GetTargetDefenses(flow, attacker, defender, param, critical);
        level = (u8)GetBattleMonStat(attacker, 0xf);
        value = CalcBaseDamage(power, attack, level, defense);
        if (ratio != 0x1000) {
            value = fixed_round(value, ratio);
        }
        mod = WeatherPowerMod(ServerEvent_GetWeather(flow), param->type);
        if (mod != 0x1000) {
            value = fixed_round(value, mod);
        }
        if (critical) {
            value *= 2;
        }
        if (!ReturnZero(flow->mainModule, 7) && func_ov167_021ae30c(flow)) {
            u16 roll;
            if (fixedRoll) {
                roll = 0x55;
            } else {
                roll = 100 - BattleRandom(16);
            }
            value = value * roll / 100;
        }
        if (param->type != 0x11) {
            value = fixed_round(value, ServerEvent_SameTypeAttackBonus(flow, attacker, param->type));
        }
        value = TypeEffectivenessPowerMod(value, effectiveness);
        if (category == 1 && GetBattleMonStatus(attacker) == 4 && GetBattleMonStat(attacker, 0x11) != 0x3e) {
            value = value * 50 / 100;
        }
        if (value == 0) {
            value = 1;
        }
        BattleEventVar_SetMulValue(0x35, 0x1000, 0x29, 0x20000);
        BattleEventVar_SetValue(0x32, value);
        BattleEvent_CallHandlers(flow, 0x47);
        mod = BattleEventVar_GetValue(0x35);
        result = fixed_round(BattleEventVar_GetValue(0x32), mod);
    }
    BattleEvent_CallHandlers(flow, 0x48);
    BattleEventVar_Pop(0x1c9d);
    *damage = result;
    return fixed;
}

void ServerControl_CalcRecoil(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 damage) {
    BOOL forced;

    if (!IsFainted(mon)) {
        s32 recoil = ServerEvent_CalcRecoil(flow, mon, move, damage, &forced);
        if (recoil != 0) {
            BattleHandler_StrSetup(&flow->message, 2, 0x17a);
            BattleHandler_AddArg(&flow->message, GetMonID(mon));
            if (forced || ServerControl_CheckSimpleDamageEnabled(flow, mon, recoil)) {
                ServerControl_SimpleDamageCore(flow, mon, recoil, &flow->message);
            }
        }
    }
}

BOOL ServerControl_CheckSimpleDamageEnabled(BtlServerFlow *flow, BattleMon *mon, u32 damage) {
    if (ServerEvent_CheckSimpleDamageEnabled(flow, mon, damage)) {
        return TRUE;
    }
    return FALSE;
}

BOOL ServerControl_SimpleDamageCore(BtlServerFlow *flow, BattleMon *mon, u32 damage, BattleHandlerString *string) {
    s32 change = -damage;
    if (change != 0) {
        ServerDisplay_SimpleHP(flow, mon, change, TRUE);
        BattleMon_SetTurnFlag(mon, 2);
        if (string != NULL) {
            BattleHandler_SetString(flow, string);
            BattleHandler_StrClear(string);
        }
        ServerControl_CheckItemReaction(flow, mon, 1);
        if (ServerControl_CheckFainted(flow, mon)) {
            ServerControl_CheckMatchup(flow);
            return TRUE;
        }
        return TRUE;
    }
    return FALSE;
}

BOOL ServerControl_UseHeldItem(BtlServerFlow *flow, BattleMon *mon) {
    u32 item = GetBattleMonHeldItem(mon);
    BOOL result = FALSE;

    if (DoesBattleMonExist(&flow->posPoke, GetMonID(mon))) {
        u32 state = PushState(&flow->actionState, 0x1cfa);
        if (!ServerEvent_CheckHeldItemFail(flow, mon, item)) {
            result = TRUE;
        }
        if (result) {
            u32 itemState;
            u32 reserved;

            reserved = SCQUE_RESERVE_Pos(flow->queue, SC_USE_HELD_ITEM);
            itemState = PushStateUseItem(&flow->actionState, item, 0x1d04);
            ServerEvent_EquipItem(flow, mon);
            if (BattleHandler_Result(flow) != 2) {
                result = FALSE;
            }
            PopState(&flow->actionState, itemState, 0x1d09);
            if (result) {
                BtlServerCmdQueue_PutReserved(flow->queue, reserved, SC_USE_HELD_ITEM, GetMonID(mon));
                if (ItemGetParam(item, 0x10)) {
                    ServerControl_ChangeHeldItem(flow, mon, 0, TRUE);
                }
            }
        }
        PopState(&flow->actionState, state, 0x1d16);
    }
    return result;
}

BOOL ServerEvent_CheckHeldItemFail(BtlServerFlow *flow, BattleMon *mon, u16 item) {
    BOOL failed;
    BattleEventVar_Push(0x1d29);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x2d, item);
    BattleEventVar_SetRewriteOnceValue(0x41, 0);
    BattleEvent_CallHandlers(flow, 0x90);
    failed = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x1d2f);
    return failed;
}

void ServerControl_ChangeHeldItem(BtlServerFlow *flow, BattleMon *mon, u16 item, BOOL consume) {
    u32 state;
    u16 oldItem;
    u8 monId;

    monId = GetMonID(mon);
    oldItem = GetBattleMonHeldItem(mon);

    state = PushState(&flow->actionState, 0x1d3d);
    ServerEvent_ItemSetDecide(flow, mon, item);
    PopState(&flow->actionState, state, 0x1d3f);
    if (item == 0) {
        scPut_SetContFlag(flow, mon, 0xf);
    }
    ItemEvent_RemoveItem(mon);
    BtlServerCmdQueue_Put(flow->queue, SC_OP_SET_ITEM, monId, item);
    SetItem(mon, item);
    if (item != 0) {
        ItemEvent_AddItem(mon);
    }
    state = PushState(&flow->actionState, 0x1d4d);
    ServerEvent_ItemSetFixed(flow, mon);
    PopState(&flow->actionState, state, 0x1d4f);
    if (consume) {
        ConsumeItem(mon, oldItem);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_CONSUME_ITEM, monId, oldItem);
        ServerDisplay_SetTurnFlag(flow, mon, 8);
    }
}

void ServerControl_FaintPokemon(BtlServerFlow *flow, BattleMon *mon) {
    ServerDisplay_FaintPokemon(flow, mon, 0);
    ServerControl_CheckFainted(flow, mon);
}

void ServerControl_DamageAddCondition(BtlServerFlow *flow, const BtlFlowMoveParam *param, BattleMon *attacker,
                                      BattleMon *target) {
    BattleCondition value;
    u32 condition = ServerEvent_CheckMoveAddCondition(flow, param->move, attacker, target, &value);
    if (condition != 0 && !IsFainted(target)) {
        ServerControl_MoveConditionCore(flow, attacker, target, param->move, condition, value, FALSE);
    }
}

u32 ServerEvent_CheckMoveAddCondition(BtlServerFlow *flow, u16 move, BattleMon *attacker, BattleMon *target,
                                      BattleCondition *value) {
    u32 condition;
    MoveConditionParam param;
    BattleCondition result;
    u8 chance;
    u8 failed;

    condition = PML_MoveGetParam(move, MOVE_PARAM_CONDITION);
    param = PML_MoveGetConditionParam(move);
    chance = PML_MoveGetParam(move, MOVE_PARAM_CONDITION_CHANCE);
    BtlCalc_MakeMoveCondition(param, attacker, &result);
    BattleEventVar_Push(0x1d8c);
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetValue(0x1d, condition);
    BattleEventVar_SetRewriteOnceValue(0x41, 0);
    BattleEventVar_SetRewriteOnceValue(0x1e, result.raw);
    BattleEventVar_SetValue(0x26, chance);
    BattleEvent_CallHandlers(flow, 0x64);
    condition = BattleEventVar_GetValue(0x1d);
    chance = BattleEventVar_GetValue(0x26);
    failed = BattleEventVar_GetValue(0x41);
    if (condition == 0xffff) {
        condition = 0;
    }
    result.raw = BattleEventVar_GetValue(0x1e);
    BattleEventVar_Pop(0x1d9f);
    if (!failed && condition != 0) {
        BOOL hit = BattleRandom(100) < chance ? TRUE : FALSE;
        if (hit) {
            *value = result;
            return condition;
        }
        if (ReturnZero(flow->mainModule, 0)) {
            *value = result;
            return condition;
        }
    }
    return 0;
}

void ServerControl_SimpleCondition(BtlServerFlow *flow, u16 move, BattleMon *mon, void *targets) {
    u32 condition;
    MoveConditionParam param;
    BOOL success;
    BattleMon *target;

    condition = PML_MoveGetParam(move, MOVE_PARAM_CONDITION);
    param = PML_MoveGetConditionParam(move);
    success = FALSE;
    if (BtlMonSet_GetCount(targets)) {
        BtlMonSet_ResetIndex(targets);
        while ((target = BtlMonSet_GetNext(targets)) != NULL) {
            BattleCondition value;
            BtlCalc_MakeMoveCondition(param, mon, &value);
            if (ServerControl_MoveConditionCore(flow, mon, target, move, condition, value, TRUE)) {
                BattleMoveEffectState *effect = flow->moveEffect;
                if (!effect->enabled) {
                    effect->enabled = 1;
                }
                success = TRUE;
            }
        }
        if (!success && !flow->unk78A_4) {
            ServerDisplay_MoveFailed(flow, mon, move);
        }
    } else {
        ServerDisplay_MoveFailed(flow, mon, move);
    }
}

BOOL ServerControl_MoveConditionCore(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u16 move,
                                     u32 condition, BattleCondition value, BOOL flag) {
    BOOL defaultMessage;

    BattleHandler_StrClear(&flow->message);
    if (condition == 0xffff) {
        condition = ServerEvent_DecideSpecialMoveCondition(flow, attacker, target, &flow->message);
        if (condition == 0 || condition == 0xffff) {
            return FALSE;
        }
    } else {
        ServerEvent_AddMoveConditionString(flow, condition, attacker, target, &flow->message);
    }
    ServerEvent_MoveConditionContinue(flow, attacker, target, condition, &value);
    defaultMessage = BattleHandler_StrIsEnabled(&flow->message) ? FALSE : TRUE;
    if (ServerEvent_AddCondition(flow, target, attacker, condition, value, flag, defaultMessage)) {
        if (!defaultMessage) {
            BattleHandler_SetString(flow, &flow->message);
            BattleHandler_StrClear(&flow->message);
        }
        return TRUE;
    }
    return FALSE;
}

u32 ServerEvent_DecideSpecialMoveCondition(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target,
                                           BattleHandlerString *string) {
    u32 condition;
    BattleEventVar_Push(0x1e11);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x3f, (s32)string);
    BattleEventVar_SetValue(0x1d, 0);
    BattleEvent_CallHandlers(flow, 0x60);
    condition = BattleEventVar_GetValue(0x1d);
    BattleEventVar_Pop(0x1e18);
    return condition;
}

void ServerEvent_AddMoveConditionString(BtlServerFlow *flow, u32 condition, BattleMon *attacker, BattleMon *target,
                                        BattleHandlerString *string) {
    BattleEventVar_Push(0x1e28);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x3f, (s32)string);
    BattleEventVar_SetConstValue(0x1d, condition);
    BattleEvent_CallHandlers(flow, 0x61);
    BattleEventVar_Pop(0x1e2e);
}

void ServerEvent_MoveConditionContinue(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u32 condition,
                                       BattleCondition *value) {
    BattleEventVar_Push(0x1e3e);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x1d, condition);
    BattleEventVar_SetValue(0x1e, value->raw);
    BattleEvent_CallHandlers(flow, 0x62);
    value->raw = BattleEventVar_GetValue(0x1e);
    BattleEventVar_Pop(0x1e45);
}

BOOL ServerEvent_AddCondition(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition,
                              BattleCondition value, BOOL flag, BOOL defaultMessage) {
    if (!ServerControl_AddConditionCheckFail(flow, target, attacker, condition, value, 0, flag)) {
        ServerControl_AddCondition(flow, target, attacker, condition, value, defaultMessage, FALSE, NULL);
        return TRUE;
    }
    return FALSE;
}

BOOL ServerControl_AddConditionCheckFail(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition,
                                         BattleCondition value, u8 overwrite, BOOL showFail) {
    u32 state;
    BOOL failed;
    u32 cause = AddConditionCheckFailOverwrite(flow, target, condition, value, overwrite);

    if (cause != 0) {
        if (showFail) {
            AddConditionCheckFailStandard(flow, target, cause, condition);
        }
        return TRUE;
    }
    state = PushState(&flow->actionState, 0x1e73);
    failed = ServerEvent_MoveConditionCheckFail(flow, attacker, target, condition);
    if (failed && showFail) {
        ServerEvent_AddConditionFailed(flow, target, attacker, condition);
        flow->unk78A_4 = 1;
    }
    PopState(&flow->actionState, state, 0x1e80);
    return failed;
}

u32 AddConditionCheckFailOverwrite(BtlServerFlow *flow, BattleMon *mon, s32 condition, BattleCondition value,
                                   u8 overwrite) {
    if (CheckCondition(mon, condition) && overwrite != 2) {
        return 1;
    }
    if (condition < 6 && GetBattleMonStatus(mon) != 0 && overwrite == 0) {
        return 3;
    }
    if (ServerEvent_GetWeather(flow) == 1 && condition == 3) {
        return 3;
    }
    if (condition == 5) {
        PokeTypePair type = GetPokeType(mon);
        if (PokeTypePair_IsMatch(type, 8) || PokeTypePair_IsMatch(type, 3)) {
            return 2;
        }
    }
    if (condition == 4 && PokeTypePair_IsMatch(GetPokeType(mon), 9)) {
        return 2;
    }
    if (condition == 3 && PokeTypePair_IsMatch(GetPokeType(mon), 0xe)) {
        return 2;
    }
    if (condition == 0x12 && PokeTypePair_IsMatch(GetPokeType(mon), 0xb)) {
        return 2;
    }
    if (condition == 0xe && GetBattleMonStatus(mon) != 0) {
        return 3;
    }
    if (condition == 0x10 && GetBattleMonStat(mon, 0x10) == 0x79) {
        return 3;
    }
    return 0;
}

void ServerControl_AddCondition(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition,
                                BattleCondition value, BOOL showMessage, BOOL skipItemReaction,
                                const BattleHandlerString *string) {
    u32 state;

    ServerDisplay_AddCondition(flow, target, condition, value);
    switch (condition) {
    case 5:
        ServerDisplay_AddEffectAtPosition(flow, target, 0x257);
        break;
    case 4:
        ServerDisplay_AddEffectAtPosition(flow, target, 0x258);
        break;
    case 1:
        ServerDisplay_AddEffectAtPosition(flow, target, 0x25a);
        break;
    case 3:
        ServerDisplay_AddEffectAtPosition(flow, target, 0x259);
        break;
    case 2:
        ServerDisplay_AddEffectAtPosition(flow, target, 0x256);
        break;
    case 6:
        ServerDisplay_AddEffectAtPosition(flow, target, 0x25b);
        break;
    case 7:
        ServerDisplay_AddEffectAtPosition(flow, target, 0x25c);
        break;
    }
    if (showMessage) {
        Condition_SetAddMessage(condition, value, target, &flow->message);
        BattleHandler_SetString(flow, &flow->message);
        BattleHandler_StrClear(&flow->message);
    } else if (string != NULL) {
        BattleHandler_SetString(flow, string);
    }
    if (condition == 3 && GetBattleMonSpecies(target) == 0x1ec && GetBattleMonStat(target, 0x13) == 1) {
        AbilityEvent_RemoveItem(target);
        ChangeForm(target, 0);
        BtlServerCmdQueue_Put(flow->queue, SC_CHANGE_FORM, GetMonID(target), 0);
        AbilityEvent_AddItem(target);
        ServerDisplay_SkyDropTargetAppear(flow, target, 0xde);
    }
    state = PushState(&flow->actionState, 0x1f10);
    if (IsBasicStatus(condition)) {
        ServerEvent_ConditionConfirmed(flow, target, attacker, condition, value);
    } else if (condition == 0x10) {
        ServerEvent_GastroAcidConfirmed(flow, target);
        if (GetBattleMonStat(target, 0x11) == 0x7f) {
            ServerControl_UnnerveAction(flow, target);
        }
    } else {
        ServerEvent_MoveStatusConfirmed(flow, target, attacker, condition);
    }
    PopState(&flow->actionState, state, 0x1f1f);
    if (!skipItemReaction) {
        ServerControl_CheckItemReaction(flow, target, 3);
    }
}

u8 ServerEvent_GetWeather(BtlServerFlow *flow) {
    u8 weather;
    BOOL suppressed;

    BattleEventVar_Push(0x1f33);
    weather = 0;
    BattleEventVar_SetRewriteOnceValue(0x41, 0);
    BattleEvent_CallHandlers(flow, 0x7a);
    suppressed = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x1f37);
    if (!suppressed) {
        weather = GetFieldWeather();
    }
    return weather;
}

fx32 ServerEvent_GetWeightRatio(BtlServerFlow *flow, BattleMon *mon) {
    fx32 ratio;
    BattleEventVar_Push(0x1f4d);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetMulValue(0x35, 0x1000, 0x19a, 0x20000);
    BattleEvent_CallHandlers(flow, 0x7b);
    ratio = BattleEventVar_GetValue(0x35);
    BattleEventVar_Pop(0x1f52);
    return ratio;
}

BOOL ServerEvent_MoveConditionCheckFail(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u32 condition) {
    BOOL failed;
    BattleEventVar_Push(0x1f65);
    BattleEventVar_SetConstValue(3, attacker != NULL ? GetMonID(attacker) : 0x1f);
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x1d, condition);
    BattleEventVar_SetRewriteOnceValue(0x41, 0);
    BattleEvent_CallHandlers(flow, 0x65);
    failed = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x1f6c);
    return failed;
}

void ServerEvent_AddConditionFailed(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition) {
    BattleEventVar_Push(0x1f7a);
    BattleEventVar_SetValue(4, GetMonID(target));
    BattleEventVar_SetValue(3, GetMonID(attacker));
    BattleEventVar_SetValue(0x1d, condition);
    BattleEvent_CallHandlers(flow, 0x67);
    BattleEventVar_Pop(0x1f7f);
}

void ServerEvent_ConditionConfirmed(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition,
                                    BattleCondition value) {
    BattleEventVar_Push(0x1f8e);
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(3, (u8)(attacker != NULL ? GetMonID(attacker) : 0x1f));
    BattleEventVar_SetConstValue(0x1d, condition);
    BattleEventVar_SetConstValue(0x1e, value.raw);
    BattleEvent_CallHandlers(flow, 0x68);
    BattleEventVar_Pop(0x1f97);
}

void ServerEvent_MoveStatusConfirmed(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition) {
    BattleEventVar_Push(0x1fa5);
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(3, (u8)(attacker != NULL ? GetMonID(attacker) : 0x1f));
    BattleEventVar_SetConstValue(0x1d, condition);
    BattleEvent_CallHandlers(flow, 0x69);
    BattleEventVar_Pop(0x1fad);
}

void ServerEvent_GastroAcidConfirmed(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0x1fb9);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEvent_ForceCallHandlers(flow, 0x6a);
    BattleEventVar_Pop(0x1fbc);
}

void ServerEvent_DamageAddEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, BattleMon *target) {
    if (!IsFainted(target) && ServerEvent_RollStatDropEffectChance(flow, param, attacker, target)) {
        ServerControl_ApplyMoveStatChanges(flow, param, attacker, target, 0);
    }
}

BOOL ServerEvent_RollStatDropEffectChance(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker,
                                          BattleMon *target) {
    BOOL hit;
    u8 failed;
    u8 chance = PML_MoveGetParam(param->move, MOVE_PARAM_STAT_CHANGE_CHANCE);

    BattleEventVar_Push(0x1fde);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetRewriteOnceValue(0x41, 0);
    hit = FALSE;
    BattleEventVar_SetValue(0x26, chance);
    BattleEvent_CallHandlers(flow, 0x51);
    failed = BattleEventVar_GetValue(0x41);
    chance = BattleEventVar_GetValue(0x26);
    BattleEventVar_Pop(0x1fe8);
    if (!failed) {
        if (BattleRandom(100) < chance) {
            hit = TRUE;
        }
        if (hit) {
            return TRUE;
        }
        if (ReturnZero(flow->mainModule, 0)) {
            return TRUE;
        }
    }
    return FALSE;
}

void ServerControl_SimpleEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets) {
    BattleMon *target;

    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        if (ServerControl_ApplyMoveStatChanges(flow, param, mon, target, TRUE)) {
            BattleMoveEffectState *effect = flow->moveEffect;
            if (!effect->enabled) {
                effect->enabled = 1;
            }
        }
    }
}

u32 ServerControl_NextSerial(BtlServerFlow *flow) {
    flow->unk778++;
    if (flow->unk778 == 0) {
        flow->unk778 = 1;
    }
    return flow->unk778;
}

BOOL ServerControl_ApplyMoveStatChanges(BtlServerFlow *flow, const BtlFlowMoveParam *param, BattleMon *attacker,
                                        BattleMon *target, BOOL showFail) {
    MoveStatChangeStat stat;
    s32 change;
    BOOL result;
    u32 count;
    u32 i;
    u32 serial;
    BOOL changed;
    u8 attackerId;

    result = FALSE;
    attackerId = GetMonID(attacker);
    serial = ServerControl_NextSerial(flow);
    count = PML_MoveGetStatChangeStat(param->move);
    for (i = 0; i < count; i++) {
        changed = FALSE;
        ServerEvent_GetMoveStatChangeValue(flow, param->move, i, attacker, target, &stat, &change);
        if (stat != MOVE_STAT_CHANGE_NONE) {
            if (stat != MOVE_STAT_CHANGE_ALL) {
                changed = ServerControl_StatChangeCore(flow, attackerId, target, stat, change, attackerId, changed,
                                                       serial, showFail, TRUE);
            } else {
                u8 s;
                for (s = 1; s < 6; s++) {
                    if (ServerControl_StatChangeCore(flow, attackerId, target, s, change, attackerId, 0, serial,
                                                     showFail, TRUE)) {
                        changed = TRUE;
                    }
                }
            }
            if (changed) {
                u32 state = PushState(&flow->actionState, 0x2047);
                ServerEvent_MoveStatChangeApplied(flow, target, param->move, stat, change);
                PopState(&flow->actionState, state, 0x2049);
                result = TRUE;
            }
        }
    }
    return result;
}

void ServerEvent_GetMoveStatChangeValue(BtlServerFlow *flow, u16 move, u32 index, BattleMon *attacker,
                                        BattleMon *target, MoveStatChangeStat *stat, s32 *change) {
    u8 multiplier;

    *stat = PML_MoveGetStatChangeStage(move, index, change);
    BattleEventVar_Push(0x2063);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetValue(0x1f, *stat);
    BattleEventVar_SetValue(0x20, *change);
    BattleEventVar_SetValue(0x35, 1);
    BattleEvent_CallHandlers(flow, 0x59);
    *stat = BattleEventVar_GetValue(0x1f);
    *change = BattleEventVar_GetValue(0x20);
    multiplier = BattleEventVar_GetValue(0x35);
    if (multiplier > 1) {
        *change *= multiplier;
    }
    BattleEventVar_Pop(0x2072);
    if (*stat == 8) {
        *stat = 0;
    }
}

BOOL ServerControl_StatChangeCore(BtlServerFlow *flow, u8 monId, BattleMon *mon, MoveStatChangeStat stat, s32 change,
                                  u8 attackerId, u16 context, u32 value, BOOL showFail, BOOL flag) {
    BOOL result;

    change = ServerEvent_CheckSubstituteInteraction(flow, mon, stat, attackerId, context, change);
    if (!IsStatChangeValid(mon, stat, change)) {
        if (showFail) {
            ServerDisplay_StatChangeLimit(flow, mon, stat, change);
            flow->unk78A_4 = 1;
        }
        return FALSE;
    }
    if (IsSubstituteActive(mon)) {
        u8 targetId = GetMonID(mon);
        if (monId != targetId) {
            if (showFail) {
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0xd2, targetId, SC_MSG_ARG_END);
            }
            return FALSE;
        }
    }
    result = TRUE;
    if (ServerEvent_CheckStatChange(flow, mon, stat, monId, change, value)) {
        u32 state;
        ServerDisplay_StatChange(flow, mon, stat, change, context, flag);
        state = PushState(&flow->actionState, 0x20ad);
        ServerEvent_StatChangeApplied(flow, monId, mon, stat, change);
        PopState(&flow->actionState, state, 0x20af);
    } else {
        if (showFail) {
            u32 state = PushState(&flow->actionState, 0x20b6);
            ServerEvent_StatChangeFailed(flow, mon, value);
            PopState(&flow->actionState, state, 0x20b8);
        }
        result = FALSE;
    }
    return result;
}

s32 ServerEvent_CheckSubstituteInteraction(BtlServerFlow *flow, BattleMon *mon, u32 stat, u8 attackerId, u16 context,
                                           s32 change) {
    BattleEventVar_Push(0x20d1);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(3, attackerId);
    BattleEventVar_SetConstValue(0x1f, stat);
    BattleEventVar_SetRewriteOnceValue(0x20, change);
    BattleEvent_CallHandlers(flow, 0x5a);
    change = BattleEventVar_GetValue(0x20);
    BattleEventVar_Pop(0x20d8);
    return change;
}

void ServerControl_StatChangeAndCondition(BtlServerFlow *flow, const BtlFlowMoveParam *param, BattleMon *mon,
                                          void *targets) {
    u32 condition;
    MoveConditionParam conditionParam;
    BattleCondition value;
    BattleMon *target;

    condition = PML_MoveGetParam(param->move, MOVE_PARAM_CONDITION);
    conditionParam = PML_MoveGetConditionParam(param->move);
    BtlCalc_MakeMoveCondition(conditionParam, mon, &value);
    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        BattleMoveEffectState *effect;

        if (ServerControl_ApplyMoveStatChanges(flow, param, mon, target, TRUE)) {
            effect = flow->moveEffect;
            if (!effect->enabled) {
                effect->enabled = 1;
            }
        }
        if (ServerControl_MoveConditionCore(flow, mon, target, param->move, condition, value, TRUE)) {
            effect = flow->moveEffect;
            if (!effect->enabled) {
                effect->enabled = 1;
            }
        }
    }
}

void ServerControl_SimpleRecover(BtlServerFlow *flow, u16 move, BattleMon *mon, void *targets) {
    BattleMon *target;

    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        u8 targetId = GetMonID(target);
        if (!ServerControl_RecoverHPCheckFail(flow, target)) {
            if (ServerControl_RecoverHP(flow, target, ServerEvent_CalcMoveHealAmount(flow, move, target), TRUE)) {
                BattleMoveEffectState *effect = flow->moveEffect;
                if (!effect->enabled) {
                    effect->enabled = 1;
                }
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x183, targetId, SC_MSG_ARG_END);
            }
        } else if (IsMonFullHP(target)) {
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x37d, targetId, SC_MSG_ARG_END);
        }
    }
}

BOOL ServerControl_RecoverHP(BtlServerFlow *flow, BattleMon *mon, u16 amount, BOOL flag) {
    if (!ServerControl_RecoverHPCheckFailSpecial(flow, mon, flag)) {
        ServerControl_RecoverHPCore(flow, mon, amount);
        return TRUE;
    }
    return FALSE;
}

BOOL ServerControl_RecoverHPCheckFail(BtlServerFlow *flow, BattleMon *mon) {
    if (!CanPokemonBattle(mon)) {
        return TRUE;
    }
    if (IsMonFullHP(mon)) {
        return TRUE;
    }
    return FALSE;
}

BOOL ServerControl_RecoverHPCheckFailSpecial(BtlServerFlow *flow, BattleMon *mon, BOOL showMessage) {
    if (CheckCondition(mon, CONDITION_HEAL_BLOCK)) {
        if (showMessage) {
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x377, GetMonID(mon), SC_MSG_ARG_END);
            flow->unk78A_5 = 1;
        }
        return TRUE;
    }
    return FALSE;
}

void ServerControl_RecoverHPCore(BtlServerFlow *flow, BattleMon *mon, u16 amount) {
    u8 pos = GetBattlePos(&flow->posPoke, GetMonID(mon));
    if (pos != 6) {
        BtlServerCmdQueue_Put(flow->queue, SC_EFFECT_BY_POS, pos, 0x263);
    }
    ServerDisplay_SimpleHP(flow, mon, amount, TRUE);
}

void ServerControl_OHKO(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets) {
    u32 i = 0;
    u8 attackerLevel = GetBattleMonStat(mon, 0xf);
    BattleMon *target;

    while (TRUE) {
        u8 targetId;
        u32 effectiveness;
        u32 state;

        target = BtlMonSet_Get(targets, i++);
        if (target == NULL) {
            break;
        }
        if (IsFainted(target)) {
            continue;
        }
        targetId = GetMonID(target);
        if (!IsGuaranteedHit(flow, mon, target) && ServerEvent_CheckSemiInvulnAvoid(flow, mon, target, param->move)) {
            ServerDisplay_MoveAvoided(flow, target, param->move);
            continue;
        }
        if (IsTargetOutOfRange(flow, mon, target, param->move)) {
            ServerDisplay_MoveAvoided(flow, target, param->move);
            continue;
        }
        if (attackerLevel < (u8)GetBattleMonStat(target, 0xf)) {
            ServerDisplay_Unaffected(flow, target);
            continue;
        }
        effectiveness = EffectivenessTable_Get(flow->unk1F8C, targetId);
        if (effectiveness == 0) {
            ServerDisplay_Unaffected(flow, target);
            continue;
        }
        state = PushState(&flow->actionState, 0x217b);
        if (ServerEvent_CheckOHKOHit(flow, mon, target, param->move)) {
            u8 attackerPos = GetBattlePos(&flow->posPoke, GetMonID(mon));
            u16 damage = GetBattleMonStat(target, 0xd);
            u32 critical = BtlCalc_GetEffectivenessCategory(effectiveness);
            BOOL substitute = FALSE;
            BOOL endured = FALSE;
            BattleMoveEffectState *effect = flow->moveEffect;

            if (!effect->enabled) {
                effect->enabled = 1;
            }
            if (IsSubstituteActive(target)) {
                damage = ServerControl_OHKOSubstitute(flow, target, param, critical);
                BtlMonSet_AddDamage(targets, target, damage, TRUE);
                substitute = TRUE;
            } else {
                u32 cause = ServerEvent_CheckEndure(flow, mon, target, 1, &damage);
                if (cause == 0) {
                    ServerControl_OHKOSuccess(flow, target, param, critical);
                } else {
                    ServerControl_OHKOSurvived(flow, target, param, critical, cause, damage);
                    BtlMonSet_AddDamage(targets, target, damage, FALSE);
                    endured = TRUE;
                }
            }
            if (!substitute) {
                ServerControl_RecordDamage(flow, attackerPos, mon, target, param, damage);
            }
            ServerControl_DamageReaction(flow, mon, target, param, effectiveness, damage, 0, substitute);
            if (endured || substitute) {
                ServerEvent_DamageProcessEnd(flow, mon, targets, param, 0);
            }
            ServerControl_CheckFainted(flow, target);
        } else if (!BattleHandler_Result(flow)) {
            ServerDisplay_MoveAvoided(flow, target, param->move);
        }
        PopState(&flow->actionState, state, 0x21ac);
    }
}

void ServerControl_OHKOSuccess(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 critical) {
    u8 monId = GetMonID(mon);
    HPZero(mon);
    BtlServerCmdQueue_Put(flow->queue, SC_OP_ZERO_HP, monId);
    BtlServerCmdQueue_Put(flow->queue, SC_DAMAGE, monId, (u8)critical, param->move);
    BtlServerCmdQueue_Put(flow->queue, SC_ONE_HIT_KO, monId);
}

void ServerControl_OHKOSurvived(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 critical, u32 cause,
                                u16 damage) {
    u16 move;
    u8 monId;

    ServerDisplay_DecreaseHP(flow, mon, damage);
    move = param->move;
    monId = GetMonID(mon);
    BtlServerCmdQueue_Put(flow->queue, SC_DAMAGE, monId, (u8)critical, move);
    ServerControl_Endure(flow, mon, cause);
}

u16 ServerControl_OHKOSubstitute(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 critical) {
    u16 damage;
    u8 monId;

    monId = GetMonID(mon);
    damage = BattleMon_GetSubstituteHP(mon);
    BtlServerCmdQueue_Put(flow->queue, SC_ONE_HIT_KO, monId);
    BtlServerCmdQueue_Put(flow->queue, SC_DAMAGE, monId, (u8)critical, param->move);
    ServerControl_BreakSubstitute(flow, mon);
    return damage;
}

void ServerControl_ForceSwitch(BtlServerFlow *flow, u16 move, BattleMon *mon, void *targets) {
    BOOL failed = FALSE;
    BattleMon *target;

    BtlMonSet_ResetIndex(targets);
    while ((target = BtlMonSet_GetNext(targets)) != NULL) {
        if (ServerControl_ForceSwitchCore(flow, mon, target, 0, &failed, 0, 0, 0)) {
            BattleMoveEffectState *effect = flow->moveEffect;
            if (!effect->enabled) {
                effect->enabled = 1;
            }
        } else if (!failed) {
            ServerDisplay_MoveFailed(flow, mon, move);
        }
    }
}

BOOL ServerControl_ForceSwitchCore(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BOOL forced,
                                   BOOL *failed, u16 effect, BOOL ignoreLevel, const BattleHandlerString *string) {
    u8 clientPos[2];
    u8 pos;
    u32 state;
    u8 blocked;
    u32 mode;

    *failed = FALSE;
    if (forced) {
        mode = 1;
    } else {
        mode = ServerControl_GetForceSwitchMode(flow);
    }
    if (mode == 2) {
        return FALSE;
    }
    pos = MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(target));
    if (pos == 6) {
        return FALSE;
    }
    if (IsFainted(target)) {
        return FALSE;
    }
    if (CheckCondition(target, CONDITION_SKY_DROP)) {
        return FALSE;
    }
    if (BtlFlow_GetSkyDropTarget(target) != 0x1f) {
        return FALSE;
    }
    state = PushState(&flow->actionState, 0x222c);
    blocked = ServerEvent_CheckForceSwitchBlocked(flow, attacker, target);
    if (blocked && BattleHandler_Result(flow)) {
        *failed = TRUE;
    }
    PopState(&flow->actionState, state, 0x2234);
    if (blocked) {
        return FALSE;
    }
    GetPosClientAndIndex(flow->mainModule, pos, &clientPos[1], &clientPos[0]);
    if (mode == 1) {
        s32 slot = ServerControl_GetRandomSwitchSlot(flow, BattleServer_GetClient(flow->server, clientPos[1]));
        if (slot >= 0) {
            u8 newMonId = GetMonID(PokeCon_GetClientPartyMon(flow->pokeCon, clientPos[1], slot));
            ServerControl_SwitchOutCore(flow, target, effect);
            if (string != NULL) {
                BattleHandler_SetString(flow, string);
            }
            ServerControl_SwitchInFillSlot(flow, clientPos[1], clientPos[0], slot, FALSE);
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x34d, newMonId, SC_MSG_ARG_END);
            ServerControl_AfterSwitchIn(flow);
        } else {
            return FALSE;
        }
    } else {
        u8 attackerClient = GetClientIDFromMonID(GetMonID(attacker));
        if (!ignoreLevel) {
            u8 attackerLevel = GetBattleMonStat(attacker, 0xf);
            if ((u8)GetBattleMonStat(target, 0xf) > attackerLevel) {
                return FALSE;
            }
        }
        BtlClientIDList_Add(&flow->clientIdList, attackerClient);
        ServerControl_SwitchOutCore(flow, target, effect);
        flow->unk14 = 5;
    }
    return TRUE;
}

u32 ServerControl_GetForceSwitchMode(BtlServerFlow *flow) {
    u32 style = BtlSetup_GetBattleStyle(flow->mainModule);
    u32 type = BtlSetup_GetBattleType(flow->mainModule);
    if (style == 0) {
        switch (type) {
        case 0:
            return 0;
        default:
            return 1;
        }
    }
    return 1;
}

s32 ServerControl_GetRandomSwitchSlot(BtlServerFlow *flow, BtlServerClient *client) {
    u8 slots[6];
    u32 count = GetNumMonsInParty(client->party);
    u32 i = GetClientBenchStart(flow->mainModule, client->clientId);
    u32 numSlots = 0;

    for (; i < count; i++) {
        if (CanPokemonBattle(GetBattleMonFromParty(client->party, i))) {
            slots[numSlots++] = i;
        }
    }
    if (numSlots != 0) {
        return slots[BattleRandom(numSlots)];
    }
    return -1;
}

BOOL ServerEvent_CheckForceSwitchBlocked(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target) {
    BOOL blocked;
    BattleEventVar_Push(0x22bd);
    BattleEventVar_SetRewriteOnceValue(0x41, 0);
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEvent_CallHandlers(flow, 0x8b);
    Condition_CheckIngrainForceSwitch(flow, target);
    blocked = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x22c3);
    return blocked;
}

void ServerControl_FieldEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon) {
    u8 weather = GetMoveWeather(param->move);

    if (weather != 0) {
        if (ServerControl_ChangeWeather(flow, weather, ServerEvent_IncreaseMoveWeatherTurns(flow, weather, mon) + 5)) {
            BattleMoveEffectState *effect = flow->moveEffect;
            if (!effect->enabled) {
                effect->enabled = 1;
            }
        } else {
            ServerDisplay_MoveFailed(flow, mon, param->move);
        }
    } else {
        BOOL success;
        u32 state = PushState(&flow->actionState, 0x22d8);
        ServerEvent_FieldEffectMove(flow, mon, param->move);
        success = BattleHandler_Result(flow) == 2;
        PopState(&flow->actionState, state, 0x22dc);
        if (success) {
            BattleMoveEffectState *effect = flow->moveEffect;
            if (!effect->enabled) {
                effect->enabled = 1;
            }
        } else {
            ServerDisplay_MoveFailed(flow, mon, param->move);
        }
    }
}

BOOL ServerControl_ChangeWeather(BtlServerFlow *flow, u8 weather, u8 turns) {
    if (ServerControl_ChangeWeatherCheck(flow, weather, turns)) {
        ServerControl_ChangeWeatherCore(flow, weather, turns);
        return TRUE;
    }
    return FALSE;
}

BOOL ServerControl_ChangeWeatherCheck(BtlServerFlow *flow, u8 weather, u8 duration) {
    if (weather >= 5) {
        return FALSE;
    }
    if (weather == GetFieldWeather() && (duration != 0xff || FieldStatusGetWeatherTurns() == 0xff)) {
        return FALSE;
    }
    return TRUE;
}

void ServerControl_ChangeWeatherCore(BtlServerFlow *flow, u8 weather, u8 duration) {
    FieldStatusSetWeather(weather, duration);
    BtlServerCmdQueue_Put(flow->queue, SC_WEATHER_START, weather, duration);
    ServerControl_ChangeWeatherAfter(flow, weather);
}

void ServerControl_ChangeWeatherAfter(BtlServerFlow *flow, u8 weather) {
    u32 state = PushState(&flow->actionState, 0x2313);
    ServerEvent_AfterWeatherChange(flow, weather);
    PopState(&flow->actionState, state, 0x2315);
}

u8 ServerEvent_IncreaseMoveWeatherTurns(BtlServerFlow *flow, u8 weather, BattleMon *mon) {
    u8 turns;
    BattleEventVar_Push(0x2327);
    BattleEventVar_SetConstValue(0x39, weather);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetValue(0x24, 0);
    BattleEvent_CallHandlers(flow, 0x7c);
    turns = BattleEventVar_GetValue(0x24);
    BattleEventVar_Pop(0x232d);
    return turns;
}

BOOL ServerControl_FieldEffectCore(BtlServerFlow *flow, u32 effect, BattleCondition value, u8 dependPoke) {
    if (FieldStatusAddEffect(effect, value)) {
        BtlServerCmdQueue_Put(flow->queue, SC_OP_ADD_FIELD_EFFECT, (u8)effect, value.raw);
        return TRUE;
    }
    if (dependPoke) {
        u8 monId = Condition_GetMonID(value);
        if (monId != 0x1f) {
            FieldStatusAddDependPoke(effect, monId);
            BtlServerCmdQueue_Put(flow->queue, SC_OP_ADD_FIELD_EFFECT_DEPEND_MON, (u8)effect, monId);
            return TRUE;
        }
    }
    return FALSE;
}

void ServerEvent_FieldEffectMove(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    BattleEventVar_Push(0x2370);
    BattleEventVar_SetValue(3, GetMonID(mon));
    BattleEventVar_SetValue(0x12, move);
    BattleEvent_CallHandlers(flow, 0x9e);
    BattleEventVar_Pop(0x2374);
}

void ServerControl_UncategorizedMove(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets) {
    if (!BtlMonSet_IsEmptyAfterTargeting(targets)) {
        switch (param->move) {
        case 0x11d:
            ServerControl_SkillSwap(flow, mon, targets);
            break;
        case 0xa4:
            if (ServerControl_Substitute(flow, mon)) {
                BattleMoveEffectState *effect = flow->moveEffect;
                if (!effect->enabled) {
                    effect->enabled = 1;
                    effect->unk05_1 = 1;
                }
            }
            break;
        default: {
            BOOL showFail;
            u32 result;
            u32 state = PushState(&flow->actionState, 0x238f);

            result = BattleHandler_Result(flow);
            if (ServerEvent_UncategorizedMove(flow, param, mon, targets, &showFail)) {
                result = BattleHandler_Result(flow);
                if (result == 2) {
                    BattleMoveEffectState *effect = flow->moveEffect;
                    if (!effect->enabled) {
                        effect->enabled = 1;
                    }
                }
            }
            if (result <= 1 && showFail && !flow->unk78A_4) {
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x47, SC_MSG_ARG_END);
            }
            PopState(&flow->actionState, state, 0x23a9);
            break;
        }
        }
    }
}

void ServerControl_SkillSwap(BtlServerFlow *flow, BattleMon *mon, void *targets) {
    u32 state2;
    u32 state;
    BattleMon *target;
    u8 monId;
    u8 targetId;
    u32 ability;
    u32 targetAbility;

    target = BtlMonSet_Get(targets, 0);
    ability = GetBattleMonStat(mon, 0x10);
    targetAbility = GetBattleMonStat(target, 0x10);
    if (ability != targetAbility && !IsSkillSwapExcludedAbility(ability) &&
        !IsSkillSwapExcludedAbility(targetAbility)) {
        BattleMoveEffectState *effect;

        monId = GetMonID(mon);
        targetId = GetMonID(target);
        effect = flow->moveEffect;
        if (!effect->enabled) {
            effect->enabled = 1;
        }
        BtlServerCmdQueue_Put(flow->queue, SC_SWAP_ABILITY, monId, targetId, (u16)targetAbility, (u16)ability);
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x1fc, monId, SC_MSG_ARG_END);
        state = PushState(&flow->actionState, 0x23c4);
        ServerEvent_ChangeAbilityBefore(flow, monId, ability, targetAbility);
        ServerEvent_ChangeAbilityBefore(flow, targetId, targetAbility, ability);
        PopState(&flow->actionState, state, 0x23c7);
        ChangeAbility(mon, targetAbility);
        ChangeAbility(target, ability);
        AbilityEvent_Swap(mon, target);
        BtlServerCmdQueue_Put(flow->queue, SC_ABILITY_POPUP_OUT, monId);
        BtlServerCmdQueue_Put(flow->queue, SC_ABILITY_POPUP_OUT, targetId);
        if (ability != targetAbility) {
            state2 = PushState(&flow->actionState, 0x23d4);
            ServerEvent_ChangeAbilityAfter(flow, monId);
            ServerEvent_ChangeAbilityAfter(flow, targetId);
            PopState(&flow->actionState, state2, 0x23d7);
        }
        if (!CheckCondition(mon, CONDITION_GASTRO_ACID)) {
            if (ability == 0x67) {
                ServerControl_CheckItemReaction(flow, mon, 0);
            }
            if (ability == 0x7f) {
                ServerControl_UnnerveAction(flow, mon);
            }
        }
        if (!CheckCondition(target, CONDITION_GASTRO_ACID)) {
            if (targetAbility == 0x67) {
                ServerControl_CheckItemReaction(flow, target, 0);
            }
            if (targetAbility == 0x7f) {
                ServerControl_UnnerveAction(flow, target);
            }
        }
    } else {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x47, SC_MSG_ARG_END);
    }
}

void ServerControl_UnnerveAction(BtlServerFlow *flow, BattleMon *mon) {
    BtlFlowMonIter iter;
    BattleMon *target;
    u8 monId = GetMonID(mon);

    BtlFlowMonIter_Init(&iter, flow);
    while (BtlFlowMonIter_Next(&iter, flow, &target)) {
        if (!IsAllyMonID(monId, GetMonID(target))) {
            ServerControl_CheckItemReaction(flow, target, 0);
        }
    }
}

BOOL ServerControl_Substitute(BtlServerFlow *flow, BattleMon *mon) {
    if (!IsSubstituteActive(mon)) {
        s32 cost = DivideMaxHPZeroCheck(mon, 4);
        if ((s32)GetBattleMonStat(mon, 0xd) > cost) {
            u8 pos = GetBattlePos(&flow->posPoke, GetMonID(mon));
            if (pos != 6) {
                ServerDisplay_SimpleHP(flow, mon, -cost, TRUE);
                ServerControl_CheckItemReaction(flow, mon, 1);
                BattleMon_SetSubstituteHP(mon, cost);
                BtlServerCmdQueue_Put(flow->queue, SC_OP_CREATE_SUBSTITUTE, GetMonID(mon), (u16)cost);
                BtlServerCmdQueue_Put(flow->queue, SC_SUBSTITUTE_CREATE, pos);
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x311, GetMonID(mon), SC_MSG_ARG_END);
                return TRUE;
            }
        }
        ServerDisplay_StandardMessage(flow, 0x7b, 0, NULL);
    } else {
        ServerDisplay_SkyDropTargetAppear(flow, mon, 0x314);
    }
    return FALSE;
}

u16 ServerControl_DamageSubstitute(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u16 damage,
                                   u32 effectiveness, u8 critical, BtlFlowMoveParam *param) {
    BtlServerCmd_Put54(flow->queue, GetMonID(target), effectiveness, param->move);
    ServerDisplay_SkyDropTargetAppear(flow, target, 0x317);
    ServerDisplay_EffectivenessMessageMulti(flow, 1, &effectiveness, &target, FALSE);
    ServerDisplay_CriticalHitMessage(flow, 1, &target, &critical, FALSE);
    if (BattleMon_DamageSubstitute(target, &damage)) {
        ServerControl_BreakSubstitute(flow, target);
    }
    ServerControl_DamageDrain(flow, param, attacker, target, damage);
    ServerControl_DamageAdditionalEffect(flow, param, attacker, target, damage, TRUE);
    ServerControl_DamageReaction(flow, attacker, target, param, effectiveness, damage, critical, TRUE);
    return damage;
}

void ServerControl_BreakSubstitute(BtlServerFlow *flow, BattleMon *mon) {
    u8 monId = GetMonID(mon);
    u8 pos = GetBattlePos(&flow->posPoke, monId);
    ServerDisplay_SkyDropTargetAppear(flow, mon, 0x31a);
    ResetSpActPriority(mon);
    BtlServerCmdQueue_Put(flow->queue, SC_OP_DELETE_SUBSTITUTE, monId);
    BtlServerCmdQueue_Put(flow->queue, SC_SUBSTITUTE_DELETE, pos);
}

void ServerControl_DamageReaction(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BtlFlowMoveParam *param,
                                  u32 effectiveness, u32 damage, u32 critical, BOOL flag) {
    u32 state = PushState(&flow->actionState, 0x246e);
    ServerEvent_DamageReaction(flow, attacker, target, param, effectiveness, damage, critical, flag);
    PopState(&flow->actionState, state, 0x2471);
}

void ServerEvent_DamageReaction(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BtlFlowMoveParam *param,
                                u32 effectiveness, u32 damage, u32 critical, BOOL flag) {
    u8 targetId = GetMonID(target);

    BattleEventVar_Push(0x2487);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, targetId);
    BattleEventVar_SetConstValue(0x14, param->originalMove);
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x38, effectiveness);
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEventVar_SetConstValue(0x1a, param->category);
    BattleEventVar_SetConstValue(0x32, damage);
    BattleEventVar_SetConstValue(0x45, critical);
    BattleEventVar_SetConstValue(0x46, flag);
    BattleEventVar_SetRewriteOnceValue(0x47, 0);
    BattleEvent_CallHandlers(flow, 0x4a);
    BattleEvent_CallHandlers(flow, 0x4b);
    BattleEvent_CallHandlers(flow, 0x4c);
    BattleEventVar_Pop(0x2498);
}

BOOL ServerEvent_UncategorizedMove(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets,
                                   BOOL *showFail) {
    u8 realCount;
    u8 count;
    u8 i;

    BattleEventVar_Push(0x24aa);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    realCount = BtlMonSet_GetCountMax(targets);
    count = BtlMonSet_GetCount(targets);
    BattleEventVar_SetConstValue(5, count);
    BattleEventVar_SetRewriteOnceValue(0x51, 1);
    for (i = 0; i < count; i++) {
        BattleEventVar_SetConstValue(6 + i, GetMonID(BtlMonSet_Get(targets, i)));
    }
    BattleEventVar_SetConstValue(0x12, param->move);
    if (realCount) {
        if (count) {
            BattleEvent_CallHandlers(flow, 0xa0);
        }
    } else {
        BattleEvent_CallHandlers(flow, 0xa1);
    }
    *showFail = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0x24ca);
    if (BattleHandler_Result(flow)) {
        return TRUE;
    }
    return FALSE;
}

void StoreBattleMonsSpeedOrder(BtlServerFlow *flow, void *monSet) {
    BtlFlowMonIter iter;
    BattleMon *mon;

    BtlMonSet_Clear(monSet);
    BtlFlowMonIter_Init(&iter, flow);
    while (BtlFlowMonIter_Next(&iter, flow, &mon)) {
        BtlMonSet_Add(monSet, mon);
    }
    SortBySpeed(monSet, flow);
}

BOOL ServerControl_TurnCheck(BtlServerFlow *flow) {
    void *monSet = flow->unk854;
    BOOL result = FALSE;
    BattleMon *mon;

    if (flow->unk77E == 0) {
        StoreBattleMonsSpeedOrder(flow, monSet);
        flow->unk77E = 1;
        ServerControl_CommPlayerSupportHeal(flow);
    }
    switch (flow->unk77E) {
    case 1:
        flow->unk77E++;
        BtlServerCmdQueue_Put(flow->queue, SC_HIDE_MSG_WINDOW, 0);
        result = FALSE;
        if (ServerControl_TurnCheckWeather(flow, monSet)) {
            result = TRUE;
            break;
        }
        if (ServerControl_CheckMatchup(flow)) {
            return result;
        }
    case 2:
        flow->unk77E++;
        if (ServerControl_TurnCheckEvent(flow, monSet, 0x76)) {
            result = TRUE;
            break;
        }
        if (ServerControl_CheckMatchup(flow)) {
            return FALSE;
        }
    case 3:
        flow->unk77E++;
        if (ServerControl_TurnCheckConditions(flow, monSet)) {
            result = TRUE;
            break;
        }
        if (ServerControl_CheckMatchup(flow)) {
            return FALSE;
        }
    case 4:
        flow->unk77E++;
        ServerControl_TurnCheckSideEffects(flow);
        ServerControl_TurnCheckFieldEffects(flow);
    case 5:
        flow->unk77E++;
        if (ServerControl_TurnCheckEvent(flow, monSet, 0x77)) {
            result = TRUE;
            break;
        }
        if (ServerControl_CheckMatchup(flow)) {
            return FALSE;
        }
    case 6:
        ServerControl_TurnCheckEvent(flow, monSet, 0x78);
        flow->unk77E++;
        BtlMonSet_ResetIndex(monSet);
        while ((mon = BtlMonSet_GetNext(monSet)) != NULL) {
            BattleMon_EndTurn(mon);
            ComboMove_ClearParam(mon);
            BtlServerCmdQueue_Put(flow->queue, SC_OP_TURN_CHECK, GetMonID(mon));
        }
        ServerControl_TripleBattleShiftToCenter(flow);
        BattleEvent_RemoveIsolatedItems();
        if (flow->unk10 < 0x270f) {
            flow->unk10++;
        }
        flow->unk77E = 0;
        flow->unk774 = 0;
        return FALSE;
    }
    return result;
}

void ServerControl_TripleBattleShiftToCenter(BtlServerFlow *flow) {
    if (BtlSetup_GetBattleStyle(flow->mainModule) == 2) {
        u8 playerClient = GetPlayerClientID(flow->mainModule);
        u8 firstClient = playerClient;
        u8 secondClient = GetOpponentClientID(flow->mainModule, playerClient, 0);
        BattleParty *firstParty;
        BattleParty *secondParty;

        if (playerClient > secondClient) {
            firstClient = secondClient;
            secondClient = playerClient;
        }
        firstParty = GetPartyData(flow->pokeCon, firstClient);
        secondParty = GetPartyData(flow->pokeCon, secondClient);
        if (GetAlivePartyCount(firstParty) == 1 && GetAlivePartyCount(secondParty) == 1) {
            BattleMon *firstMon = BattleParty_GetFirstAlive(firstParty);
            BattleMon *secondMon = BattleParty_GetFirstAlive(secondParty);
            u8 firstId = GetMonID(firstMon);
            u8 secondId = GetMonID(secondMon);
            u8 firstPos = GetBattlePos(&flow->posPoke, firstId);
            u8 secondPos = GetBattlePos(&flow->posPoke, secondId);

            if (firstPos != 6 && secondPos != 6) {
                u8 firstSlot = GetPosIndexInClient(flow->mainModule, firstPos);
                u8 secondSlot = GetPosIndexInClient(flow->mainModule, secondPos);
                if (firstSlot == secondSlot && !IsTripleCenterPos(firstPos)) {
                    BtlServerCmdQueue_Put(flow->queue, SC_MOVE_TO_CENTER, firstClient, firstSlot, secondClient,
                                          secondSlot);
                    ServerControl_MoveCore(flow, firstClient, firstSlot, 1, TRUE);
                    ServerControl_MoveCore(flow, secondClient, secondSlot, 1, TRUE);
                    ServerControl_AfterMove(flow, firstClient, firstSlot, 1);
                    ServerControl_AfterMove(flow, secondClient, secondSlot, 1);
                }
            }
        }
    }
}

void ServerControl_CommPlayerSupportHeal(BtlServerFlow *flow) {
    void *passPower = GetCommPlayerSupport(flow->mainModule);

    if (passPower != NULL) {
        u32 type = CommPlayerSupport_GetType(passPower);
        if (type != 0 && type != 3) {
            u8 monIds[3];
            u8 count;
            u8 i;
            u8 clientId = GetPlayerClientID(flow->mainModule);
            BtlServerClient *client = BattleServer_GetClient(flow->server, clientId);

            for (i = 0, count = 0; i < client->numCoverPos; i++) {
                BattleMon *mon = PokeCon_GetClientPartyMon(flow->pokeCon, clientId, i);
                if (BtlFlow_IsMonAlive(mon) && !IsMonFullHP(mon)) {
                    monIds[count++] = GetMonID(mon);
                }
            }
            if (count != 0) {
                u8 monId = monIds[GFL_RandomMTRange(count)];
                BattleMon *mon = GetPokeParam(flow->pokeCon, monId);
                u32 amount = GetBattleMonStat(mon, 0xe);
                if (type == 1) {
                    amount >>= 1;
                }
                if (amount != 0 && ServerControl_RecoverHP(flow, mon, amount, FALSE)) {
                    BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x53, 4, monId, SC_MSG_ARG_END);
                    CommPlayerSupport_SetUsed(passPower);
                }
            }
        }
    }
}

BOOL ServerControl_TurnCheckEvent(BtlServerFlow *flow, void *monSet, u32 event) {
    u32 state;
    BattleMon *mon;

    BtlMonSet_ResetIndex(monSet);
    state = PushState(&flow->actionState, 0x25bc);
    ServerEvent_TurnCheck(flow, 0x1f, event);
    PopState(&flow->actionState, state, 0x25be);
    if (!ServerControl_CheckMatchup(flow)) {
        while ((mon = BtlMonSet_GetNext(monSet)) != NULL) {
            state = PushState(&flow->actionState, 0x25c4);
            ServerEvent_TurnCheck(flow, GetMonID(mon), event);
            PopState(&flow->actionState, state, 0x25c6);
            ServerControl_CheckFainted(flow, mon);
            if (ServerControl_CheckMatchup(flow)) {
                break;
            }
        }
    }
    return ServerControl_CheckExpGain(flow);
}

void ServerEvent_TurnCheck(BtlServerFlow *flow, u8 monId, u32 event) {
    BattleEventVar_Push(0x25d3);
    BattleEventVar_SetConstValue(2, monId);
    BattleEvent_CallHandlers(flow, event);
    BattleEventVar_Pop(0x25d6);
}

BOOL ServerControl_TurnCheckConditions(BtlServerFlow *flow, void *monSet) {
    u8 monIds[6] = { 0, 0, 0, 0, 3, 0 };
    BattleCondition prev;
    BOOL cured;
    u32 state;
    u32 count;
    u32 index;
    u32 i;

    count = BtlMonSet_GetCount(monSet);
    for (i = 0; i < count; i++) {
        monIds[i] = GetMonID(BtlMonSet_Get(monSet, i));
    }
    index = 0;
    while (TRUE) {
        u32 condition = GetConditionByIndex(index++);
        if (condition == 0) {
            break;
        }
        for (i = 0; i < count; i++) {
            BattleMon *mon = GetPokeParam(flow->pokeCon, monIds[i]);
            if (!IsFainted(mon) && BattleMon_TickCondition(mon, condition, &prev, &cured)) {
                state = PushState(&flow->actionState, 0x2600);
                Condition_EndTurnEffect(mon, condition, prev, cured, flow);
                PopState(&flow->actionState, state, 0x2602);
            }
        }
        if (ServerControl_CheckMatchup(flow)) {
            break;
        }
    }
    BtlServerCmdQueue_Put(flow->queue, SC_OP_CONDITION_TURN_CHECK, (u8)count, (u8)index, BtlCalc_PackMonIDs(monIds));
    return ServerControl_CheckExpGain(flow);
}

void ServerControl_ConditionDamage(BtlServerFlow *flow, BattleMon *mon, u32 condition, u32 damage) {
    u32 state = PushState(&flow->actionState, 0x2624);
    u32 amount = ServerEvent_ConditionDamage(flow, mon, condition, damage);

    if (amount != 0 && ServerControl_CheckSimpleDamageEnabled(flow, mon, amount)) {
        switch (condition) {
        case 5:
            ServerDisplay_AddEffectAtPosition(flow, mon, 0x257);
            break;
        case 4:
            ServerDisplay_AddEffectAtPosition(flow, mon, 0x258);
            break;
        case 10:
            ServerDisplay_AddEffectAtPosition(flow, mon, 0x273);
            break;
        case 9:
            ServerDisplay_AddEffectAtPosition(flow, mon, 0x274);
            break;
        }
        BattleHandler_StrClear(&flow->message);
        Condition_SetDamageMessage(&flow->message, mon, condition);
        ServerControl_SimpleDamageCore(flow, mon, amount, &flow->message);
    }
    PopState(&flow->actionState, state, 0x2644);
}

u32 ServerEvent_ConditionDamage(BtlServerFlow *flow, BattleMon *mon, u32 condition, u32 damage) {
    u32 result;
    BattleEventVar_Push(0x2652);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x1d, condition);
    BattleEventVar_SetValue(0x32, damage);
    BattleEvent_CallHandlers(flow, 0x6b);
    result = BattleEventVar_GetValue(0x32);
    BattleEventVar_Pop(0x2658);
    return result;
}

void ServerControl_TurnCheckSideEffects(BtlServerFlow *flow) {
    SideEffectsEndTurn(ServerControl_SideEffectEndMessage, flow);
}

void ServerControl_SideEffectEndMessage(u8 side, u32 effect, BtlServerFlow *flow) {
    ServerDisplay_SideEffectEnd(flow, effect, side);
}

void ServerDisplay_SideEffectEnd(BtlServerFlow *flow, u32 effect, u32 side) {
    s32 message = -1;

    switch (effect) {
    case 0:
        message = 0x7e;
        break;
    case 1:
        message = 0x82;
        break;
    case 2:
        message = 0x86;
        break;
    case 3:
        message = 0x8a;
        break;
    case 4:
        message = 0x8e;
        break;
    case 5:
        message = 0x92;
        break;
    case 6:
        message = 0x96;
        break;
    case 7:
        message = 0x9a;
        break;
    case 8:
        message = 0x9e;
        break;
    case 11:
        message = 0xa6;
        break;
    case 12:
        message = 0xaa;
        break;
    case 13:
        message = 0xae;
        break;
    }
    if (message >= 0) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, message, side, SC_MSG_ARG_END);
    }
}

void ServerControl_TurnCheckFieldEffects(BtlServerFlow *flow) {
    FieldStatusTurnCheck(ServerControl_FieldEffectEndCallback, flow);
    BtlServerCmdQueue_Put(flow->queue, SC_OP_FIELD_TURN_CHECK, 0);
}

void ServerControl_FieldEffectEndCallback(u32 effect, BtlServerFlow *flow) {
    ServerControl_FieldEffectEnd(flow, effect);
}

void ServerControl_FieldEffectEnd(BtlServerFlow *flow, u32 effect) {
    s32 message = -1;

    switch (effect) {
    case 1:
        message = 0x74;
        break;
    case 2:
        message = 0x76;
        break;
    case 6:
        message = 0xb3;
        break;
    case 7:
        message = 0xb5;
        break;
    }
    if (message >= 0) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, message, SC_MSG_ARG_END);
    }
    BtlServerCmdQueue_Put(flow->queue, SC_OP_REMOVE_FIELD_EFFECT, (u8)effect);
    if (effect == 7) {
        BattleMon *mon;

        StoreBattleMonsSpeedOrder(flow, flow->unk868);
        BtlMonSet_ResetIndex(flow->unk868);
        while ((mon = BtlMonSet_GetNext(flow->unk868)) != NULL) {
            if (CanPokemonBattle(mon)) {
                ServerControl_CheckItemReaction(flow, mon, 0);
            }
        }
    }
}

BOOL ServerControl_TurnCheckWeather(BtlServerFlow *flow, void *monSet) {
    u8 ended = FieldStatusDecWeatherTurns();
    u32 weather;
    BOOL damaged;
    BattleMon *mon;

    if (ended != 0) {
        BtlServerCmdQueue_Put(flow->queue, SC_WEATHER_END, ended);
        ServerControl_ChangeWeatherAfter(flow, 0);
        return FALSE;
    }
    damaged = FALSE;
    weather = ServerEvent_GetWeather(flow);
    BtlMonSet_ResetIndex(monSet);
    while ((mon = BtlMonSet_GetNext(monSet)) != NULL) {
        if (!IsFainted(mon) && !GetAdditionalConditionFlag(mon, 5) && !GetAdditionalConditionFlag(mon, 4)) {
            s32 damage;
            u32 state = PushState(&flow->actionState, 0x26cf);
            damage = ServerEvent_CheckWeatherDamage(flow, mon, weather, BtlCalc_GetWeatherDamage(mon, weather));
            if (damage != 0) {
                ServerControl_WeatherDamage(flow, mon, weather, damage);
                damaged = TRUE;
            }
            PopState(&flow->actionState, state, 0x26db);
            ServerControl_CheckFainted(flow, mon);
        }
    }
    if (damaged) {
        ServerControl_ViewEffect(flow, 0x255, 6, 6, 0, 0);
    }
    return ServerControl_CheckExpGain(flow);
}

s32 ServerEvent_CheckWeatherDamage(BtlServerFlow *flow, BattleMon *mon, u32 weather, s32 damage) {
    u8 cancel;
    s32 value;
    s32 result;

    BattleEventVar_Push(0x26f1);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x39, weather);
    result = 0;
    BattleEventVar_SetRewriteOnceValue(0x41, 0);
    BattleEventVar_SetValue(0x32, damage);
    BattleEvent_CallHandlers(flow, 0x7f);
    value = BattleEventVar_GetValue(0x32);
    cancel = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x26fa);
    if (!cancel) {
        result = value;
    }
    return result;
}

void ServerControl_WeatherDamage(BtlServerFlow *flow, BattleMon *mon, u32 weather, s32 damage) {
    u8 monId = GetMonID(mon);

    switch (weather) {
    case 4:
        BattleHandler_StrSetup(&flow->message, 2, 0x18c);
        BattleHandler_AddArg(&flow->message, monId);
        break;
    case 3:
        BattleHandler_StrSetup(&flow->message, 2, 0x18f);
        BattleHandler_AddArg(&flow->message, monId);
        break;
    default:
        BattleHandler_StrClear(&flow->message);
        break;
    }
    if (damage > 0 && ServerControl_CheckSimpleDamageEnabled(flow, mon, damage)) {
        BattleHandler_SetString(flow, &flow->message);
        BattleHandler_StrClear(&flow->message);
        ServerControl_ViewEffect(flow, 0x290, BtlServerFlow_MonIDToPos(flow, monId), 6, 0, 0);
        if ((s32)GetBattleMonStat(mon, 0xd) <= damage) {
            ServerControl_ViewEffect(flow, 0x255, 6, 6, 0, 0);
        }
        ServerControl_SimpleDamageCore(flow, mon, damage, NULL);
    }
}

BOOL ServerControl_CheckFainted(BtlServerFlow *flow, BattleMon *mon) {
    u8 monId = GetMonID(mon);

    if (!flow->unk7A9[monId] && IsFainted(mon)) {
        BOOL bigLevelGap;
        u8 clientId;
        u8 playerClient;

        flow->unk7A9[monId] = TRUE;
        BtlFaintRec_Add(&flow->faintRec, monId);
        if (!BtlSetup_IsBattleType(flow->mainModule, 0x200) || GetClientIDFromMonID(monId) == 0) {
            if (flow->unk789 != monId) {
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0, monId, SC_MSG_ARG_END);
            }
        }
        BtlServerCmdQueue_Put(flow->queue, SC_FAINT, monId);
        ServerControl_ClearMonDependentEffects(flow, mon, FALSE);
        Clear_ForFainted(mon);
        if (GetClientIDFromMonID(monId) == GetPlayerClientID(flow->mainModule)) {
            bigLevelGap = FALSE;
            s32 level = GetBattleMonStat(mon, 0xf);
            if (level + 30 <= (s32)GetEnemyMaxLevel(flow)) {
                bigLevelGap = TRUE;
            }
            ChangeFriendshipWhenFainted(flow->mainModule, mon, bigLevelGap);
        }
        BtlPosPoke_RemoveMon(&flow->posPoke, monId);
        clientId = GetClientIDFromMonID(monId);
        playerClient = GetPlayerClientID(flow->mainModule);
        if (clientId == playerClient) {
            IncRecord(flow->mainModule, 0x4c);
        } else if (!IsAllyClientID(clientId, playerClient)) {
            IncRecord(flow->mainModule, 0x19);
            IncRecord(flow->mainModule, 0x52);
        }
        return TRUE;
    }
    return FALSE;
}

u32 GetEnemyMaxLevel(BtlServerFlow *flow) {
    u32 maxLevel = 1;
    u32 posMax = GetValidPosMax(flow->mainModule);
    u8 playerClient = GetPlayerClientID(flow->mainModule);
    u32 pos;

    for (pos = 0; pos <= posMax; pos++) {
        if (!IsAllyClientID(playerClient, GetClientIDFromPos(flow->mainModule, pos))) {
            u8 monId = GetExistPokeID(&flow->posPoke, pos);
            if (monId != 0x1f) {
                u32 level = GetBattleMonStat(GetPokeParam(flow->pokeCon, monId), 0xf);
                if (level > maxLevel) {
                    maxLevel = level;
                }
            }
        }
    }
    return maxLevel;
}

void ServerControl_ClearMonDependentEffects(BtlServerFlow *flow, BattleMon *mon, BOOL rotation) {
    BtlFlowMonIter iter;
    BattleMon *other;
    u8 monId = GetMonID(mon);

    ServerEvent_BeforeFaint(flow, mon);
    ServerControl_SkyDropCheckRelease(flow, mon, FALSE);
    if (!rotation) {
        AbilityEvent_RemoveItem(mon);
        ItemEvent_RemoveItem(mon);
    } else {
        AbilityEvent_ItemRotationSleep(mon);
        ItemEvent_ItemRotationSleep(mon);
    }
    RemoveForceAll(mon);
    BtlFlowMonIter_Init(&iter, flow);
    BtlFlowMonIter_SetRotation(&iter, flow);
    while (BtlFlowMonIter_Next(&iter, flow, &other)) {
        BattleMon_CureConditionsFromMon(other, monId);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_CURE_MON_DEPENDENT_CONDITIONS, GetMonID(other), monId);
    }
    FieldStatusRemoveDependPoke(monId);
    BtlServerCmdQueue_Put(flow->queue, SC_OP_REMOVE_FIELD_EFFECT_DEPEND_MON, monId);
    if (!ServerControl_CheckMatchup(flow) && (u16)GetBattleMonStat(mon, 0x11) == 0x7f) {
        ServerControl_UnnerveAction(flow, mon);
    }
}

void ServerEvent_BeforeFaint(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0x27e8);
    BattleEventVar_SetValue(2, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0xa3);
    BattleEventVar_Pop(0x27eb);
}

BOOL ServerControl_CheckExpGain(BtlServerFlow *flow) {
    BOOL result = FALSE;

    func_ov167_021bc6ac(GetPokeParam(flow->pokeCon, 0));
    if (func_ov167_0219bdfc(flow->mainModule)) {
        u32 i;
        u32 count = BtlFaintRec_GetCount(&flow->faintRec, 0);

        func_ov167_021bc6ac(GetPokeParam(flow->pokeCon, 0));
        for (i = 0; i < count; i++) {
            if (!BtlFaintRec_IsHandled(&flow->faintRec, 0, i)) {
                BattleMon *mon = GetPokeParam(flow->pokeCon, BtlFaintRec_GetMonId(&flow->faintRec, 0, i));
                func_ov167_021bc6ac(GetPokeParam(flow->pokeCon, 0));
                if (BtlSetup_GetBattleType(flow->mainModule) == 0 && !flow->unk78A_1 &&
                    ServerControl_CheckMatchup(flow) && ServerControl_PlayerSideHasMons(flow)) {
                    BtlServerCmdQueue_Put(flow->queue, SC_PLAY_BGM, GetWinBGM(flow->mainModule));
                    flow->unk78A_1 = 1;
                }
                if (ServerControl_GainExpFromFainted(flow, mon)) {
                    result = TRUE;
                }
                BtlFaintRec_SetHandled(&flow->faintRec, 0, i);
            }
        }
    }
    return result;
}

BOOL ServerControl_GainExpFromFainted(BtlServerFlow *flow, BattleMon *mon) {
    if (GetSideFromMonID(GetMonID(mon)) == 1) {
        BattleParty *party = GetPartyData(flow->pokeCon, GetPlayerClientID(flow->mainModule));
        AddExpAndEVs(flow, party, mon, flow->expEntries);
        if (BtlSetup_GetBattleType(flow->mainModule) == 0) {
            u16 species = GetBattleMonSpecies(mon);
            GameBeaconSys_SendDefeatMission(species, BtlSetup_IsBattleType(flow->mainModule, 0x4000),
                                            BtlSetup_IsBattleType(flow->mainModule, 0x8000));
        }
        return ServerControl_GiveExp(flow, party, flow->expEntries);
    }
    return FALSE;
}

BOOL ServerControl_GiveExp(BtlServerFlow *flow, BattleParty *party, const BtlFlowExpEntry *entries) {
    u32 remaining;
    u32 i;
    u8 monId;
    u32 message;
    BOOL result = FALSE;

    for (i = 0; i < 6; i++) {
        const BtlFlowExpEntry *entry = &entries[i];
        if (entries[i].exp != 0) {
            BattleMon *mon = BattleParty_GetMon(party, i);
            if ((s32)GetBattleMonStat(mon, 0xf) < 100) {
                u32 exp = entry->exp;
                if (entry->boosted) {
                    message = 0x2b;
                } else {
                    message = 0x2a;
                }
                monId = GetMonID(mon);
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, (u16)message, monId, exp, SC_MSG_ARG_END);
                BtlServerCmdQueue_Put(flow->queue, SC_ADD_EVS, monId, entry->hp, entry->attack, entry->defense,
                                      entry->speed, entry->spAttack, entry->spDefense);
                remaining = exp;
                while (BattleMon_AddExp(mon, &remaining, &flow->levelUp)) {
                }
                BtlServerCmdQueue_Put(flow->queue, SC_EXP_GAIN, monId, exp);
                AddRecord(flow->mainModule, 0x1b, exp);
                result = TRUE;
            }
        }
    }
    return result;
}

BOOL ServerControl_HideTurnCancel(BtlServerFlow *flow, BattleMon *mon, u32 flag) {
    if (GetAdditionalConditionFlag(mon, flag)) {
        u8 monId = GetMonID(mon);
        scPut_ResetContFlag(flow, mon, flag);
        if (CheckCondition(mon, CONDITION_CHARGE)) {
            ServerControl_CureCondition(flow, mon, CONDITION_CHARGE, 0);
        }
        BtlServerCmdQueue_Put(flow->queue, SC_SET_MON_VISIBLE, monId, 0);
        ActionOrder_ForceDone(flow, monId);
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021a8f8c(BtlFlowUnk1B54 *work) {
    work->unk220 = 0;
}

void ServerDisplay_AddEffectAtPosition(BtlServerFlow *flow, BattleMon *mon, u32 effect) {
    u8 pos = GetBattlePos(&flow->posPoke, GetMonID(mon));
    if (pos != 6) {
        BtlServerCmdQueue_Put(flow->queue, SC_EFFECT_BY_POS, pos, effect);
    }
}

void ServerDisplay_MustRecharge(BtlServerFlow *flow, BattleMon *mon) {
    ServerDisplay_SkyDropTargetAppear(flow, mon, 0x350);
}

void ServerDisplay_Confused(BtlServerFlow *flow, BattleMon *mon) {
    ServerDisplay_AddEffectAtPosition(flow, mon, 0x25b);
    BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x15c, GetMonID(mon), SC_MSG_ARG_END);
}

void ServerDisplay_InLove(BtlServerFlow *flow, BattleMon *mon) {
    u8 move = GetDisabledMove(mon, CONDITION_ATTRACT);
    ServerDisplay_AddEffectAtPosition(flow, mon, 0x25c);
    BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x14d, GetMonID(mon), move, SC_MSG_ARG_END);
}

void ServerDisplay_ConfusionDamage(BtlServerFlow *flow, BattleMon *mon, u32 damage) {
    u8 monId = GetMonID(mon);
    BtlServerCmdQueue_Put(flow->queue, SC_CONFUSION_DAMAGE, monId);
    ServerDisplay_SimpleHP(flow, mon, -damage, TRUE);
    BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x50, monId, SC_MSG_ARG_END);
}

void ServerControl_CureStatus(BtlServerFlow *flow, BattleMon *mon, u32 status, BOOL flag) {
    u8 monId = GetMonID(mon);

    CureCondition(mon);
    BtlServerCmdQueue_Put(flow->queue, SC_OP_CURE_CONDITION, monId);
    BtlServerCmdQueue_Put(flow->queue, SC_SET_GAUGE_STATUS, monId, 0);
    if (flag) {
        switch (status) {
        case 2:
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x138, monId, SC_MSG_ARG_END);
            break;
        case 3:
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x126, monId, SC_MSG_ARG_END);
            break;
        default:
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x156, monId, SC_MSG_ARG_END);
            break;
        }
    }
}

void ServerDisplay_MoveMessage(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    BtlServerCmdQueue_Put(flow->queue, SC_MOVE_MSG, GetMonID(mon), move);
}

void ServerControl_CureCondition(BtlServerFlow *flow, BattleMon *mon, s32 condition, BattleCondition *prev) {
    if (condition != 0) {
        u8 monId = GetMonID(mon);
        if (prev != NULL) {
            *prev = GetConditionContinuationParam(mon, condition);
        }
        if (condition < 6) {
            CureCondition(mon);
            BtlServerCmdQueue_Put(flow->queue, SC_OP_CURE_CONDITION, monId);
            if (GetBattlePos(&flow->posPoke, monId) != 6) {
                BtlServerCmdQueue_Put(flow->queue, SC_SET_GAUGE_STATUS, monId, 0);
            }
        } else {
            CureMoveCondition(mon, condition);
            BtlServerCmdQueue_Put(flow->queue, SC_OP_CURE_MOVE_CONDITION, monId, (u16)condition);
        }
    }
}

s32 ConvertConditionCode(BattleMon *mon, s32 *condition) {
    s32 result = 0;
    u32 status = GetBattleMonStatus(mon);

    switch (*condition) {
    case 0:
        break;
    case 0x24:
        if (status != 0) {
            result = status;
        }
        *condition = 0;
        break;
    case 0x25:
        if (status != 0) {
            result = status;
            *condition = 6;
        } else {
            if (CheckCondition(mon, CONDITION_CONFUSION)) {
                result = 6;
            }
            *condition = 0;
        }
        break;
    case 0x26: {
        s32 found = BtlCalc_FindMentalHerbCondition(mon);
        if (found != 0) {
            return found;
        }
        *condition = result;
        return found;
    }
    default:
        if (CheckCondition(mon, *condition)) {
            result = *condition;
        }
        *condition = 0;
        break;
    }
    return result;
}

void ServerDisplay_MoveFailed(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x47, SC_MSG_ARG_END);
}

void ServerDisplay_MoveAvoided(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0xd5, GetMonID(mon), SC_MSG_ARG_END);
}

void ServerDisplay_DoesntAffect(BtlServerFlow *flow, BattleMon *mon) {
    BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0xd2, GetMonID(mon), SC_MSG_ARG_END);
}

void ServerDisplay_Unaffected(BtlServerFlow *flow, BattleMon *mon) {
    BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0xd8, GetMonID(mon), SC_MSG_ARG_END);
}

void ServerDisplay_Damage(BtlServerFlow *flow, BtlFlowMoveParam *param, u32 count, u32 *effectiveness, BattleMon **mons,
                          u16 *damages, u8 *critical, BOOL multipleTargets) {
    u8 superEffective;
    u8 notVeryEffective;
    u32 kind;
    u32 i;

    notVeryEffective = 0;
    superEffective = 0;
    for (i = 0; i < count; i++) {
        s32 value;
        ServerDisplay_DecreaseHP(flow, mons[i], damages[i]);
        value = effectiveness[i];
        if (value > 3) {
            superEffective++;
        }
        if (value < 3) {
            notVeryEffective++;
        }
    }
    if (superEffective) {
        kind = 2;
    } else if (notVeryEffective) {
        kind = 3;
    } else {
        kind = 1;
    }
    BtlServerCmdQueue_Put(flow->queue, SC_DAMAGE_PLURAL, (u8)count, (u8)kind, param->move);
    for (i = 0; i < count; i++) {
        BtlServerCmdQueue_PutByte(flow->queue, GetMonID(mons[i]));
    }
}

void ServerDisplay_EffectivenessMessageMulti(BtlServerFlow *flow, u32 count, u32 *effectiveness, BattleMon **mons,
                                             BOOL multipleTargets) {
    u8 monIds[3];
    u8 notVeryEffective;
    u8 superEffective;
    u32 i;

    notVeryEffective = 0;
    superEffective = 0;
    for (i = 0; i < count; i++) {
        s32 value = effectiveness[i];
        if (value > 3) {
            superEffective++;
        }
        if (value < 3) {
            notVeryEffective++;
        }
    }
    if (multipleTargets) {
        if (superEffective) {
            u8 n = 0;
            for (i = 0; i < count; i++) {
                if ((s32)effectiveness[i] > 3) {
                    monIds[n++] = GetMonID(mons[i]);
                }
            }
            switch (superEffective) {
            case 1:
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 6, monIds[0], SC_MSG_ARG_END);
                break;
            case 2:
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 9, monIds[0], monIds[1], SC_MSG_ARG_END);
                break;
            case 3:
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0xc, monIds[0], monIds[1], monIds[2], SC_MSG_ARG_END);
                break;
            }
        }
        if (notVeryEffective) {
            u8 n;
            for (i = 0, n = 0; i < count; i++) {
                if ((s32)effectiveness[i] < 3) {
                    monIds[n++] = GetMonID(mons[i]);
                }
            }
            switch (notVeryEffective) {
            case 1:
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0xf, monIds[0], SC_MSG_ARG_END);
                break;
            case 2:
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x12, monIds[0], monIds[1], SC_MSG_ARG_END);
                break;
            case 3:
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x15, monIds[0], monIds[1], monIds[2],
                                         SC_MSG_ARG_END);
                break;
            }
        }
    } else if (superEffective) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x4e, SC_MSG_ARG_END);
    } else if (notVeryEffective) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x4f, SC_MSG_ARG_END);
    }
}

void ServerDisplay_CriticalHitMessage(BtlServerFlow *flow, u32 count, BattleMon **mons, u8 *critical,
                                      BOOL multipleTargets) {
    u32 i;

    for (i = 0; i < count; i++) {
        if (critical[i]) {
            if (multipleTargets) {
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x180, GetMonID(mons[i]), SC_MSG_ARG_END);
            } else {
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x51, SC_MSG_ARG_END);
            }
        }
    }
}

// Function names from swan.
void ServerDisplay_AbilityPopupAdd(BtlServerFlow *handler, BattleMon *mon) {
    BtlServerCmdQueue_Put(handler->queue, SC_ABILITY_POPUP_IN, GetMonID(mon));
}

void ServerDisplay_AbilityPopupRemove(BtlServerFlow *handler, BattleMon *mon) {
    BtlServerCmdQueue_Put(handler->queue, SC_ABILITY_POPUP_OUT, GetMonID(mon));
}

void ServerDisplay_StatChangeLimit(BtlServerFlow *flow, BattleMon *mon, u32 stat, s32 change) {
    u8 monId = GetMonID(mon);
    if (change > 0) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x99, monId, stat, SC_MSG_ARG_END);
    } else {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0xae, monId, stat, SC_MSG_ARG_END);
    }
}

void ServerDisplay_StatChange(BtlServerFlow *flow, BattleMon *mon, u32 stat, s32 change, u16 context, BOOL flag) {
    u8 monId = GetMonID(mon);

    if (change > 0) {
        u32 actual = BattleMon_RaiseStatStage(mon, stat, change);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_RAISE_STAT_STAGE, monId, (u8)stat, (u8)actual);
        BtlServerCmdQueue_Put(flow->queue, SC_STAT_UP_EFFECT, monId, (u8)stat, (u8)actual);
        if (flag) {
            if (context == 0) {
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x1b, monId, stat, actual, SC_MSG_ARG_END);
            } else {
                BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x3aa, monId, context, stat, actual, SC_MSG_ARG_END);
            }
        }
    } else {
        u32 actual = BattleMon_LowerStatStage(mon, stat, -change);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_LOWER_STAT_STAGE, monId, (u8)stat, (u8)actual);
        BtlServerCmdQueue_Put(flow->queue, SC_STAT_DOWN_EFFECT, monId, (u8)stat, (u8)actual);
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x5a, monId, stat, actual, SC_MSG_ARG_END);
    }
}

void ServerDisplay_SimpleHP(BtlServerFlow *flow, BattleMon *mon, s32 amount, BOOL show) {
    u8 monId = GetMonID(mon);

    if (amount > 0) {
        HPAdd(mon, amount);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_ADD_HP, monId, (u16)amount);
    } else if (amount < 0) {
        ServerDisplay_DecreaseHP(flow, mon, -amount);
    }
    if (show && GetBattlePos(&flow->posPoke, monId) != 6) {
        BtlServerCmdQueue_Put(flow->queue, SC_UPDATE_HP_GAUGE, monId);
    }
}

void ServerDisplay_FaintPokemon(BtlServerFlow *flow, BattleMon *mon, u32 flag) {
    u32 hp = GetBattleMonStat(mon, 0xd);
    u8 monId = GetMonID(mon);

    HPZero(mon);
    BtlServerCmdQueue_Put(flow->queue, SC_OP_ZERO_HP, monId);
    BtlServerCmdQueue_Put(flow->queue, SC_FAINT_HP_GAUGE, monId, flag);
}

void ServerDisplay_AddCondition(BtlServerFlow *flow, BattleMon *mon, s32 condition, BattleCondition value) {
    u8 monId = GetMonID(mon);

    SetMoveCondition(mon, condition, value);
    BtlServerCmdQueue_Put(flow->queue, SC_OP_SET_CONDITION, monId, (u8)condition, value);
    // The major status conditions show an icon, and a bad poisoning has its own after the last of them
    if (condition < CONDITION_CONFUSION) {
        if (condition == CONDITION_POISON && Condition_IsBadlyPoisoned(value)) {
            condition = CONDITION_POISON + 1;
        }
        BtlServerCmdQueue_Put(flow->queue, SC_SET_GAUGE_STATUS, monId, condition);
    }
}

void AddConditionCheckFailStandard(BtlServerFlow *flow, BattleMon *mon, u32 cause, u32 condition) {
    u8 monId = GetMonID(mon);

    switch (cause) {
    case 1:
        switch (condition) {
        case CONDITION_POISON:
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0xf9, monId, SC_MSG_ARG_END);
            break;
        case CONDITION_BURN:
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x10b, monId, SC_MSG_ARG_END);
            break;
        case CONDITION_PARALYSIS:
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x11a, monId, SC_MSG_ARG_END);
            break;
        case CONDITION_SLEEP:
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x13b, monId, SC_MSG_ARG_END);
            break;
        case CONDITION_FREEZE:
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x129, monId, SC_MSG_ARG_END);
            break;
        case CONDITION_CONFUSION:
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x162, monId, SC_MSG_ARG_END);
            break;
        case CONDITION_HEAL_BLOCK:
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x18, monId, SC_MSG_ARG_END);
            break;
        case CONDITION_PERISH_SONG:
            return;
        case CONDITION_NONE:
        default:
            BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x47, SC_MSG_ARG_END);
            break;
        }
        flow->unk78A_4 = 1;
        break;
    case 2:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0xd2, monId, SC_MSG_ARG_END);
        flow->unk78A_4 = 1;
        break;
    default:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x47, SC_MSG_ARG_END);
        flow->unk78A_4 = 1;
        break;
    }
}

void ServerDisplay_SkyDropTargetAppear(BtlServerFlow *flow, BattleMon *mon, u16 message) {
    BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, message, GetMonID(mon), SC_MSG_ARG_END);
}

void ServerDisplay_StandardMessage(BtlServerFlow *flow, u16 message, u32 count, const u32 *args) {
    switch (count) {
    default:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, message, SC_MSG_ARG_END);
        break;
    case 1:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, message, args[0], SC_MSG_ARG_END);
        break;
    case 2:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, message, args[0], args[1], SC_MSG_ARG_END);
        break;
    case 3:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, message, args[0], args[1], args[2], SC_MSG_ARG_END);
        break;
    case 4:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, message, args[0], args[1], args[2], args[3], SC_MSG_ARG_END);
        break;
    }
}

void ServerDisplay_StandardMessageEx(BtlServerFlow *flow, u16 message, u16 soundEffect, u32 count, const u32 *args) {
    switch (count) {
    default:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD_SE, message, soundEffect, SC_MSG_ARG_END);
        break;
    case 1:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD_SE, message, soundEffect, args[0], SC_MSG_ARG_END);
        break;
    case 2:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD_SE, message, soundEffect, args[0], args[1], SC_MSG_ARG_END);
        break;
    case 3:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD_SE, message, soundEffect, args[0], args[1], args[2],
                                 SC_MSG_ARG_END);
        break;
    case 4:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD_SE, message, soundEffect, args[0], args[1], args[2], args[3],
                                 SC_MSG_ARG_END);
        break;
    }
}

void ServerDisplay_SetMessage(BtlServerFlow *flow, u16 message, u32 count, const u32 *args) {
    switch (count) {
    default:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, message, SC_MSG_ARG_END);
        break;
    case 1:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, message, args[0], SC_MSG_ARG_END);
        break;
    case 2:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, message, args[0], args[1], SC_MSG_ARG_END);
        break;
    case 3:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, message, args[0], args[1], args[2], SC_MSG_ARG_END);
        break;
    case 4:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, message, args[0], args[1], args[2], args[3], SC_MSG_ARG_END);
        break;
    }
}

void ServerDisplay_SetMessageEx(BtlServerFlow *flow, u16 message, u16 soundEffect, u32 count, const u32 *args) {
    switch (count) {
    default:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET_SE, message, soundEffect, SC_MSG_ARG_END);
        break;
    case 1:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET_SE, message, soundEffect, args[0], SC_MSG_ARG_END);
        break;
    case 2:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET_SE, message, soundEffect, args[0], args[1], SC_MSG_ARG_END);
        break;
    case 3:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET_SE, message, soundEffect, args[0], args[1], args[2],
                                 SC_MSG_ARG_END);
        break;
    case 4:
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET_SE, message, soundEffect, args[0], args[1], args[2], args[3],
                                 SC_MSG_ARG_END);
        break;
    }
}

void ServerDisplay_DecreaseHP(BtlServerFlow *flow, BattleMon *mon, u32 damage) {
    if (!ReturnZero(flow->mainModule, 4)) {
        HPSub(mon, damage);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_SUBTRACT_HP, GetMonID(mon), (u16)damage);
    }
}

void ServerDisplay_RecoverPP(BtlServerFlow *flow, BattleMon *mon, u8 slot, u8 amount, BOOL original) {
    u8 monId = GetMonID(mon);

    if (original) {
        Move_IncrementPP_Org(mon, slot, amount);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_ADD_ORIGINAL_PP, monId, slot, amount);
    } else {
        Move_IncrementPP(mon, slot, amount);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_ADD_PP, monId, slot, amount);
    }
}

void ServerDisplay_UseHeldItem(BtlServerFlow *flow, BattleMon *mon) {
    BtlServerCmdQueue_Put(flow->queue, SC_USE_HELD_ITEM, GetMonID(mon));
}

// Function names from swan.
void scPut_SetContFlag(BtlServerFlow *handler, BattleMon *mon, u32 flag) {
    BattleMon_SetConditionFlag(mon, flag);
    BtlServerCmdQueue_Put(handler->queue, SC_OP_SET_CONDITION_FLAG, GetMonID(mon), flag);
}

void scPut_ResetContFlag(BtlServerFlow *handler, BattleMon *mon, u32 flag) {
    BattleMon_ClearConditionFlag(mon, flag);
    BtlServerCmdQueue_Put(handler->queue, SC_OP_CLEAR_CONDITION_FLAG, GetMonID(mon), flag);
}

void ServerDisplay_SetTurnFlag(BtlServerFlow *handler, BattleMon *mon, u32 flag) {
    BattleMon_SetTurnFlag(mon, flag);
    BtlServerCmdQueue_Put(handler->queue, SC_OP_SET_TURN_FLAG, GetMonID(mon), flag);
}

void ServerControl_UpdateIllusion(BtlServerFlow *flow, BtlFlowClientList *list) {
    u32 i;
    BattleParty *party;

    for (i = 0; i < list->count; i++) {
        party = GetPartyData(flow->pokeCon, list->clientIds[i]);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_SETUP_ILLUSION, list->clientIds[i]);
        UpdatePartyIllusionDisguises(flow->mainModule, party, list->clientIds[i]);
    }
}

u32 ServerEvent_InterruptSwitch(BtlServerFlow *flow, BattleMon *mon) {
    u32 i;
    u16 move;

    // Pursuit strikes a mon that is switching out before it leaves
    for (i = 0; i < flow->actionOrderCount; i++) {
        if (flow->actionOrder[i].mon != mon && !flow->actionOrder[i].done) {
            move = ActionOrder_GetMove(&flow->actionOrder[i]);
            if (move == MOVE_PURSUIT &&
                MoveEvent_AddItem(flow->actionOrder[i].mon, move, GetBattleMonStat(flow->actionOrder[i].mon, 0xc))) {
                flow->actionOrder[i].interrupting = TRUE;
            }
        }
    }
    flow->interruptCount = 0;
    BattleEventVar_Push(0x2c37);
    BattleEventVar_SetConstValue(6, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0x53);
    BattleEventVar_Pop(0x2c3a);
    for (i = 0; i < flow->actionOrderCount; i++) {
        if (flow->actionOrder[i].interrupting) {
            move = ActionOrder_GetMove(&flow->actionOrder[i]);
            if (move == MOVE_PURSUIT) {
                RemoveForce(flow->actionOrder[i].mon, move);
            }
            flow->actionOrder[i].interrupting = FALSE;
        }
    }
    return flow->interruptCount;
}

BOOL ServerEvent_GetCalledMove(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target, BtlFlowCalledMove *called) {
    BOOL failed;

    BattleEventVar_Push(0x2c5e);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0xe, target);
    BattleEventVar_SetValue(0x12, 0);
    BattleEventVar_SetValue(0xd, 6);
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEvent_CallHandlers(flow, 0x18);
    called->move = BattleEventVar_GetValue(0x12);
    called->target = BattleEventVar_GetValue(0xd);
    failed = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x2c6b);
    return !failed;
}

u8 ServerEvent_CheckSpecialPriority(BtlServerFlow *flow, BattleMon *mon) {
    u8 result;

    BattleEventVar_Push(0x2c7c);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetRewriteOnceValue(0x11, 1);
    BattleEvent_CallHandlers(flow, 0xf);
    result = BattleEventVar_GetValue(0x11);
    BattleEventVar_Pop(0x2c81);
    return result;
}

void ServerEvent_BeforeFight(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    BattleEventVar_Push(0x2c91);
    BattleEventVar_SetValue(2, GetMonID(mon));
    BattleEventVar_SetValue(0x12, move);
    BattleEvent_CallHandlers(flow, 0x15);
    BattleEventVar_Pop(0x2c95);
}

// A confused mon hurts itself, with a typeless 40 power physical attack on itself
u16 ServerControl_ConfusionDamage(BtlServerFlow *flow, BattleMon *mon) {
    u32 attack;
    u32 defense;
    u32 cause;
    u16 damage;

    attack = GetBattleMonStat(mon, 8);
    defense = GetBattleMonStat(mon, 9);
    damage = CalcBaseDamage(40, attack, GetBattleMonStat(mon, 0xf), defense);
    damage = damage * (100 - BattleRandom(16)) / 100;
    if (damage == 0) {
        damage = 1;
    }
    cause = ServerEvent_CheckEndure(flow, mon, mon, 0, &damage);
    ServerDisplay_ConfusionDamage(flow, mon, damage);
    if (cause) {
        ServerControl_Endure(flow, mon, cause);
    }
    return damage;
}

BOOL ServerEvent_SetMoveMessage(BtlServerFlow *flow, BattleMon *mon, u16 move, u16 actualMove,
                                BattleHandlerString *string) {
    BOOL result;

    BattleHandler_StrClear(string);
    BattleEventVar_Push(0x2ccf);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, actualMove);
    BattleEventVar_SetConstValue(0x3f, (s32)string);
    BattleEventVar_SetRewriteOnceValue(0x51, FALSE);
    BattleEvent_CallHandlers(flow, 0x19);
    result = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0x2cd6);
    return result;
}

void ServerEvent_GetMoveParam(BtlServerFlow *flow, u16 move, BattleMon *mon, BtlFlowMoveParam *param) {
    BattleEventVar_Push(0x2ce7);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetRewriteOnceValue(0x16, PML_MoveGetType(move));
    BattleEventVar_SetValue(0x1c, GetPokeType(mon));
    BattleEventVar_SetValue(0x1a, PML_MoveGetCategory(move));
    BattleEventVar_SetValue(0x1b, PML_MoveGetParam(move, MOVE_PARAM_TARGET));
    BattleEventVar_SetRewriteOnceValue(0x4b, FALSE);
    BattleEvent_CallHandlers(flow, 0x28);
    param->move = move;
    param->originalMove = move;
    param->type = BattleEventVar_GetValue(0x16);
    param->userType = BattleEventVar_GetValue(0x1c);
    param->category = BattleEventVar_GetValue(0x1a);
    param->targetType = BattleEventVar_GetValue(0x1b);
    param->flags.raw = 0;
    if (BattleEventVar_GetValue(0x4b)) {
        param->type = TYPE_NULL;
    }
    BattleEventVar_Pop(0x2d04);
}

void ServerEvent_MoveExecuteFail(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 cause) {
    BattleEventVar_Push(0x2d13);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetConstValue(0x22, cause);
    BattleEvent_CallHandlers(flow, 0x21);
    BattleEventVar_Pop(0x2d18);
}

u32 ServerEvent_MoveExecuteStart(BtlServerFlow *flow, BattleMon *mon, u16 move, void *targets) {
    u32 count;
    u32 i;
    u32 result;
    u16 newMove;

    count = BtlMonSet_GetCount(targets);
    BattleEventVar_Push(0x2d2c);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetConstValue(5, count);
    for (i = 0; i < count; i++) {
        BattleEventVar_SetConstValue(6 + i, GetMonID(BtlMonSet_Get(targets, i)));
    }
    BattleEventVar_SetRewriteOnceValue(0x3e, 0);
    BattleEventVar_SetRewriteOnceValue(0x13, 0);
    BattleEvent_CallHandlers(flow, 0x24);
    result = BattleEventVar_GetValue(0x3e);
    newMove = BattleEventVar_GetValue(0x13);
    BattleEventVar_Pop(0x2d3d);
    if (newMove) {
        flow->moveEffect->unk00 = newMove;
    }
    return result;
}

BOOL ServerEvent_CheckProtectBreak(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move) {
    BOOL result;

    BattleEventVar_Push(0x2d55);
    BattleEventVar_SetValue(3, GetMonID(mon));
    BattleEventVar_SetValue(0x51, FALSE);
    BattleEvent_CallHandlers(flow, 0x2e);
    result = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0x2d5a);
    return result;
}

BOOL ServerEvent_CheckChargeFail(BtlServerFlow *flow, BattleMon *mon, void *targets) {
    BattleMon *target;
    BOOL result;

    target = BtlMonSet_Get(targets, 0);
    BattleEventVar_Push(0x2d6e);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(4, target != NULL ? GetMonID(target) : 0x1f);
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEvent_CallHandlers(flow, 0x93);
    result = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x2d74);
    return result;
}

BOOL ServerEvent_CheckChargeSkip(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    BOOL result;

    BattleEventVar_Push(0x2d85);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetRewriteOnceValue(0x51, FALSE);
    BattleEvent_CallHandlers(flow, 0x94);
    result = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0x2d8b);
    return result;
}

BOOL ServerEvent_ChargeStart(BtlServerFlow *flow, BattleMon *mon, void *targets, u16 move, u8 *monId, BOOL *failed) {
    u32 count;
    u32 i;
    BOOL stopped;

    count = BtlMonSet_GetCount(targets);
    BattleEventVar_Push(0x2d9f);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(5, count);
    if (count != 0) {
        for (i = 0; i < count; i++) {
            BattleEventVar_SetConstValue(6 + i, GetMonID(BtlMonSet_Get(targets, i)));
        }
    } else {
        BattleEventVar_SetConstValue(6, 0x1f);
    }
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEventVar_SetRewriteOnceValue(0x51, FALSE);
    BattleEventVar_SetRewriteOnceValue(0x4f, FALSE);
    BattleEvent_CallHandlers(flow, 0x95);
    if (BattleEventVar_GetValue(0x51)) {
        *monId = BattleEventVar_GetValue(6);
    } else {
        *monId = 0x1f;
    }
    *failed = BattleEventVar_GetValue(0x4f);
    stopped = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x2dbc);
    return stopped ? FALSE : TRUE;
}

void ServerEvent_ChargeStartFixed(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0x2dcb);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0x96);
    BattleEventVar_Pop(0x2dce);
}

void ServerEvent_ChargeSkipFixed(BtlServerFlow *flow, BattleMon *mon, u16 move) {
    BattleEventVar_Push(0x2ddc);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0x97);
    BattleEventVar_Pop(0x2ddf);
}

u32 ServerEvent_ChargeRelease(BtlServerFlow *flow, BattleMon *mon, void *targets, u16 move) {
    u32 count;
    u32 i;
    u8 failed;

    count = BtlMonSet_GetCount(targets);
    BattleEventVar_Push(0x2df2);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEventVar_SetConstValue(5, count);
    for (i = 0; i < count; i++) {
        BattleEventVar_SetConstValue(6 + i, GetMonID(BtlMonSet_Get(targets, i)));
    }
    BattleEvent_CallHandlers(flow, 0x98);
    failed = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x2dfd);
    return failed ? FALSE : TRUE;
}

BOOL ServerEvent_CheckSemiInvulnAvoid(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move) {
    BOOL result = FALSE;
    u32 status;

    status = BattleMon_GetSemiInvulnFlag(target);
    if (status != 0x10) {
        BattleEventVar_Push(0x2e14);
        BattleEventVar_SetConstValue(3, GetMonID(mon));
        BattleEventVar_SetConstValue(4, GetMonID(target));
        BattleEventVar_SetConstValue(0x21, status);
        BattleEventVar_SetRewriteOnceValue(0x42, TRUE);
        BattleEvent_CallHandlers(flow, 0x99);
        if (BattleEventVar_GetValue(0x42)) {
            result = TRUE;
        }
        BattleEventVar_Pop(0x2e1e);
    }
    return result;
}

BOOL ServerEvent_CheckAlwaysHit(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move) {
    BOOL result;

    result = PML_MoveIsAlwaysHit(move);
    if (!result) {
        BattleEventVar_Push(0x2e34);
        BattleEventVar_SetConstValue(3, GetMonID(mon));
        BattleEventVar_SetRewriteOnceValue(0x51, FALSE);
        BattleEvent_CallHandlers(flow, 0x32);
        result = BattleEventVar_GetValue(0x51);
        BattleEventVar_Pop(0x2e39);
    }
    return result;
}

u32 ServerEvent_GetMoveAccuracy(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, BtlFlowMoveParam *param) {
    u32 accuracy;
    u32 ratio;

    accuracy = PML_MoveGetParam(param->move, MOVE_PARAM_ACCURACY);
    BattleEventVar_Push(0x2e4e);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x1a, param->category);
    BattleEventVar_SetRewriteOnceValue(0x2b, accuracy);
    BattleEventVar_SetMulValue(0x35, 0x1000, 0x19a, 0x20000);
    BattleEvent_CallHandlers(flow, 0x34);
    accuracy = BattleEventVar_GetValue(0x2b);
    ratio = BattleEventVar_GetValue(0x35);
    BattleEventVar_Pop(0x2e58);
    return fixed_round(accuracy, ratio);
}

// Whether a one-hit KO move hits: never a mon of a higher level, and more likely for each level the attacker has
// over the target
BOOL ServerEvent_CheckOHKOHit(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u16 move) {
    u8 attackerLevel;
    u8 targetLevel;
    u8 accuracy;
    BOOL failed;
    BOOL result;

    attackerLevel = GetBattleMonStat(attacker, 0xf);
    targetLevel = GetBattleMonStat(target, 0xf);
    if (attackerLevel < targetLevel) {
        return FALSE;
    }
    BattleEventVar_Push(0x2e79);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    result = FALSE;
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEvent_CallHandlers(flow, 0x70);
    failed = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x2e7f);
    if (failed) {
        return FALSE;
    }
    if (IsGuaranteedHit(flow, attacker, target)) {
        result = TRUE;
    } else {
        accuracy = PML_MoveGetParam(move, MOVE_PARAM_ACCURACY);
        accuracy += attackerLevel - targetLevel;
        if (BattleRandom(100) < accuracy) {
            result = TRUE;
        }
    }
    return result;
}

BOOL ServerEvent_CheckDamageToRecover(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target,
                                      BtlFlowMoveParam *param) {
    BOOL result;

    BattleEventVar_Push(0x2ea2);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEventVar_SetRewriteOnceValue(0x51, FALSE);
    BattleEvent_CallHandlers(flow, 0x30);
    result = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0x2ea9);
    return result;
}

void ServerEvent_ApplyDamageToRecover(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target) {
    BattleEventVar_Push(0x2eb7);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEvent_CallHandlers(flow, 0x31);
    BattleEventVar_Pop(0x2ebb);
}

// Whether a move lands a critical hit
BOOL ServerEvent_CheckCritical(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u16 move) {
    u16 stage;
    BOOL critical;
    s32 value;

    stage = PML_MoveGetParam(move, MOVE_PARAM_CRIT_STAGE);
    stage += BattleMon_GetCritStage(attacker);
    BattleEventVar_Push(0x2ecf);
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetValue(0x2c, stage);
    critical = FALSE;
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEvent_CallHandlers(flow, 0x36);
    if (!BattleEventVar_GetValue(0x41)) {
        if (PML_MoveIsAlwaysCrit(move)) {
            critical = TRUE;
        } else {
            value = BattleEventVar_GetValue(0x2c);
            if (value > 4) {
                value = 4;
            }
            stage = value;
            critical = BtlCalc_RollCritical(stage);
        }
        if (ReturnZero(flow->mainModule, 3)) {
            critical = TRUE;
        }
    }
    BattleEventVar_Pop(0x2ee6);
    return critical;
}

s32 ServerEvent_CalcRecoil(BtlServerFlow *flow, BattleMon *mon, u16 move, s32 damage, BOOL *forced) {
    u8 monId;
    u8 ratio;
    u8 bonus;
    u8 failed;
    u8 force;
    u32 kind;

    monId = GetMonID(mon);
    // A share of the damage dealt, or of the user's max HP
    ratio = PML_MoveGetParam(move, MOVE_PARAM_RECOIL);
    kind = 0;
    *forced = FALSE;
    if (ratio != 0) {
        kind = 1;
    } else {
        ratio = PML_MoveGetParam(move, MOVE_PARAM_HP_COST);
        if (ratio != 0) {
            kind = 2;
        }
    }
    if (PML_MoveGetQuality(move) == 8) {
        return 0;
    }
    BattleEventVar_Push(0x2f12);
    BattleEventVar_SetConstValue(3, monId);
    BattleEventVar_SetValue(0x35, ratio);
    BattleEventVar_SetValue(0x36, 0);
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEventVar_SetRewriteOnceValue(0x51, FALSE);
    BattleEvent_CallHandlers(flow, 0x50);
    ratio = BattleEventVar_GetValue(0x35);
    failed = BattleEventVar_GetValue(0x41);
    force = BattleEventVar_GetValue(0x51);
    bonus = BattleEventVar_GetValue(0x36);
    BattleEventVar_Pop(0x2f1f);
    if (!force && failed == TRUE) {
        return 0;
    }
    ratio += bonus;
    if (ratio != 0) {
        if (kind == 2) {
            damage = MultiplyValueByRatio(GetBattleMonStat(mon, 0xe), ratio);
            if (damage < 1) {
                damage = 1;
            }
        } else if (damage != 0) {
            damage = MultiplyValueByRatio(damage, ratio);
            if (damage < 1) {
                damage = 1;
            }
        }
        *forced = force;
        return damage;
    }
    return 0;
}

void ServerEvent_EquipItem(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0x2f4c);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0x72);
    BattleEventVar_Pop(0x2f4f);
}

void ServerEvent_EquipTempItem(BtlServerFlow *flow, BattleMon *mon, u8 monIndex) {
    BattleEventVar_Push(0x2f5c);
    BattleEventVar_SetValue(2, GetMonID(mon));
    BattleEventVar_SetValue(3, monIndex);
    BattleEvent_ForceCallHandlers(flow, 0x73);
    BattleEventVar_Pop(0x2f60);
}

// The effectiveness of a move against a target's types, counting Ground moves against a floating target
u32 ServerControl_CheckMoveEffectiveness(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target,
                                         BtlFlowMoveParam *param, BOOL flag) {
    BOOL floating = FALSE;
    PokeTypePair types;
    u32 effectiveness;

    if (param->type == TYPE_NULL) {
        return TYPE_EFFECTIVENESS_NORMAL;
    }
    types = GetPokeType(target);
    effectiveness =
        ServerEvent_CheckTypeEffectiveness(flow, attacker, target, param->type, PokeTypePair_GetType1(types));
    if (!PokeTypePair_IsMonotype(types)) {
        effectiveness = GetTypeEffectivenessMultiplier(
            effectiveness,
            ServerEvent_CheckTypeEffectiveness(flow, attacker, target, param->type, PokeTypePair_GetType2(types)));
    }
    if (effectiveness != TYPE_EFFECTIVENESS_IMMUNE) {
        if (param->type != TYPE_GROUND) {
            return effectiveness;
        }
        floating = ServerControl_CheckFloating(flow, target, FALSE);
        if (!floating) {
            return effectiveness;
        }
        effectiveness = TYPE_EFFECTIVENESS_IMMUNE;
    } else if (param->type == TYPE_GROUND && !ServerControl_CheckFloating(flow, target, TRUE)) {
        return TYPE_EFFECTIVENESS_NORMAL;
    }
    if (effectiveness == TYPE_EFFECTIVENESS_IMMUNE && flag) {
        if (!floating) {
            ServerDisplay_DoesntAffect(flow, target);
        } else {
            ServerControl_FloatingImmune(flow, target);
        }
    }
    return effectiveness;
}

BOOL ServerControl_CheckFloating(BtlServerFlow *flow, BattleMon *mon, BOOL flag) {
    if (!IsFieldEffectActive(FIELD_CONDITION_GRAVITY) && ServerEvent_CheckFloating(flow, mon, flag)) {
        return TRUE;
    }
    return FALSE;
}

BOOL ServerEvent_CheckFloating(BtlServerFlow *flow, BattleMon *mon, BOOL flag) {
    u8 floating;
    u8 failed;

    floating = flag ? DoesMonHaveType(mon, TYPE_FLYING) : FALSE;
    BattleEventVar_Push(0x2fdb);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetRewriteOnceValue(0x51, floating);
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEvent_CallHandlers(flow, 0x12);
    Condition_CheckFloating(flow, mon);
    floating = BattleEventVar_GetValue(0x51);
    failed = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x2fe3);
    if (failed) {
        return FALSE;
    }
    return floating;
}

void ServerControl_FloatingImmune(BtlServerFlow *flow, BattleMon *mon) {
    u32 state;

    state = PushState(&flow->actionState, 0x2ff5);
    ServerEvent_FloatingImmune(flow, mon);
    if (BattleHandler_Result(flow) != 2) {
        ServerDisplay_DoesntAffect(flow, mon);
    }
    PopState(&flow->actionState, state, 0x2ffc);
}

void ServerEvent_FloatingImmune(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0x3008);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0x1b);
    BattleEventVar_Pop(0x300b);
}

u32 ServerEvent_CheckTypeEffectiveness(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u8 moveType,
                                       u8 defenseType) {
    BOOL ignoreImmunity;
    BOOL forceNormal;
    u32 effectiveness;

    BattleEventVar_Push(0x3021);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(target));
    BattleEventVar_SetConstValue(0x15, defenseType);
    BattleEventVar_SetConstValue(0x16, moveType);
    BattleEventVar_SetRewriteOnceValue(0x4b, FALSE);
    BattleEventVar_SetRewriteOnceValue(0x4c, FALSE);
    Condition_CheckTypeImmunity(flow, target);
    BattleEvent_CallHandlers(flow, 0x3e);
    ignoreImmunity = BattleEventVar_GetValue(0x4b);
    forceNormal = BattleEventVar_GetValue(0x4c);
    BattleEventVar_Pop(0x302e);
    if (forceNormal) {
        return TYPE_EFFECTIVENESS_NORMAL;
    }
    effectiveness = GetTypeEffectiveness(moveType, defenseType);
    if (effectiveness == TYPE_EFFECTIVENESS_IMMUNE && ignoreImmunity) {
        return TYPE_EFFECTIVENESS_NORMAL;
    }
    return effectiveness;
}

void ServerControl_ViewEffect(BtlServerFlow *flow, u16 effect, u8 pos1, u8 pos2, BOOL reserved, u32 reserve) {
    u32 posCount;

    if (pos2 != 6) {
        posCount = 2;
    } else {
        posCount = 1;
        if (pos1 == 6) {
            posCount = 0;
        }
    }
    if (reserved) {
        switch (posCount) {
        case 0:
            BtlServerCmdQueue_PutReserved(flow->queue, (u16)reserve, SC_EFFECT, effect);
            break;
        case 1:
            BtlServerCmdQueue_PutReserved(flow->queue, (u16)reserve, SC_EFFECT_BY_POS, pos1, effect);
            break;
        case 2:
            BtlServerCmdQueue_PutReserved(flow->queue, (u16)reserve, SC_EFFECT_BETWEEN_POS, pos1, pos2, effect);
            break;
        }
    } else {
        switch (posCount) {
        case 0:
            BtlServerCmdQueue_Put(flow->queue, SC_EFFECT, effect);
            break;
        case 1:
            BtlServerCmdQueue_Put(flow->queue, SC_EFFECT_BY_POS, pos1, effect);
            break;
        case 2:
            BtlServerCmdQueue_Put(flow->queue, SC_EFFECT_BETWEEN_POS, pos1, pos2, effect);
            break;
        }
    }
}

BOOL ServerEvent_DecrementPP(BtlServerFlow *flow, BattleMon *mon, u8 moveIndex) {
    BOOL result;

    BattleEventVar_Push(0x308e);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x17, moveIndex);
    BattleEventVar_SetRewriteOnceValue(0x51, FALSE);
    BattleEvent_CallHandlers(flow, 0x4f);
    result = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0x3094);
    return result;
}

BOOL ServerEvent_CheckMultihitHits(BtlServerFlow *flow, BattleMon *mon, u16 move, BtlFlowHitWork *hitWork) {
    u8 maxHits;
    u32 hits;

    maxHits = PML_MoveGetParam(move, MOVE_PARAM_HIT_MAX);
    hitWork->unk01 = 0;
    hitWork->unk05 = 3;
    if (maxHits > 1) {
        hits = BtlCalc_RollHitCount(maxHits);
        BattleEventVar_Push(0x30af);
        BattleEventVar_SetConstValue(3, GetMonID(mon));
        BattleEventVar_SetConstValue(0x29, maxHits);
        BattleEventVar_SetValue(0x2a, hits);
        BattleEventVar_SetRewriteOnceValue(0x51, FALSE);
        BattleEventVar_SetRewriteOnceValue(0x42, FALSE);
        BattleEvent_CallHandlers(flow, 0x35);
        if (maxHits <= 5 && BattleEventVar_GetValue(0x51)) {
            hitWork->count = maxHits;
            hitWork->unk02 = FALSE;
        } else {
            hitWork->count = BattleEventVar_GetValue(0x2a);
            hitWork->unk02 = BattleEventVar_GetValue(0x42);
        }
        hitWork->unk03 = TRUE;
        BattleEventVar_Pop(0x30c6);
        return TRUE;
    }
    hitWork->count = 1;
    hitWork->unk02 = FALSE;
    hitWork->unk03 = FALSE;
    return FALSE;
}

u16 ServerEvent_GetMovePower(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender,
                             const BtlFlowMoveParam *param) {
    u16 power;
    u32 ratio;

    power = PML_MoveGetBasePower(param->move);
    BattleEventVar_Push(0x30e6);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(defender));
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEventVar_SetConstValue(0x1a, param->category);
    BattleEventVar_SetRewriteOnceValue(0x30, power);
    BattleEvent_CallHandlers(flow, 0x37);
    BattleEventVar_SetMulValue(0x31, 0x1000, 0x29, 0x200000);
    BattleEvent_CallHandlers(flow, 0x38);
    power = BattleEventVar_GetValue(0x30);
    ratio = BattleEventVar_GetValue(0x31);
    BattleEventVar_Pop(0x30f3);
    return fixed_round(power, ratio);
}

BOOL ServerEvent_CheckSimpleDamageEnabled(BtlServerFlow *flow, BattleMon *mon, u32 damage) {
    BOOL result;

    BattleEventVar_Push(0x3108);
    BattleEventVar_SetConstValue(4, GetMonID(mon));
    BattleEventVar_SetRewriteOnceValue(0x51, TRUE);
    BattleEvent_CallHandlers(flow, 0x80);
    result = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0x310d);
    return result;
}

u16 ServerEvent_GetAttackPower(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender,
                               const BtlFlowMoveParam *param, BOOL critical) {
    u32 stat;
    u8 monId;
    u16 power;

    stat = PML_MoveGetCategory(param->move) == MOVE_CATEGORY_SPECIAL ? 10 : 8;
    BattleEventVar_Push(0x311d);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(defender));
    BattleEventVar_SetValue(0x3b, 0x1f);
    BattleEventVar_SetValue(0x51, FALSE);
    BattleEvent_CallHandlers(flow, 0x39);
    // Foul Play attacks with the target's stat
    monId = BattleEventVar_GetValue(0x3b);
    if (monId != 0x1f) {
        attacker = GetPokeParam(flow->pokeCon, monId);
    }
    if (BattleEventVar_GetValue(0x51)) {
        power = RawBattleMonStat(attacker, stat);
    } else if (critical) {
        power = CritAtkDefLevel(attacker, stat);
    } else {
        power = GetBattleMonStat(attacker, stat);
    }
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEventVar_SetConstValue(0x1a, param->category);
    BattleEventVar_SetValue(0x33, power);
    BattleEventVar_SetMulValue(0x35, 0x1000, 0x19a, 0x20000);
    BattleEvent_CallHandlers(flow, 0x3b);
    power = BattleEventVar_GetValue(0x33);
    power = fixed_round(power, BattleEventVar_GetValue(0x35));
    BattleEventVar_Pop(0x3144);
    return power;
}

u16 ServerEvent_GetTargetDefenses(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender,
                                  const BtlFlowMoveParam *param, BOOL critical) {
    u32 stat;
    u32 category;
    u8 raw;
    u16 defense;
    u32 ratio;

    stat = PML_MoveGetCategory(param->move) == MOVE_CATEGORY_SPECIAL ? 11 : 9;
    category = param->category;
    BattleEventVar_Push(0x3154);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(defender));
    BattleEventVar_SetConstValue(0x3c, stat);
    BattleEventVar_SetValue(0x3d, 0);
    BattleEventVar_SetRewriteOnceValue(0x51, FALSE);
    BattleEvent_CallHandlers(flow, 0x3a);
    // Wonder Room swaps the two defenses
    if (BattleEventVar_GetValue(0x3d) & 1) {
        if (stat == 9) {
            stat = 11;
        } else {
            stat = 9;
        }
        if (stat == 9) {
            category = MOVE_CATEGORY_PHYSICAL;
        } else {
            category = MOVE_CATEGORY_SPECIAL;
        }
    }
    raw = BattleEventVar_GetValue(0x51);
    BattleEventVar_Pop(0x3163);
    if (raw) {
        defense = RawBattleMonStat(defender, stat);
    } else if (critical) {
        defense = CritAtkDefLevel(defender, stat);
    } else {
        defense = GetBattleMonStat(defender, stat);
    }
    // A sandstorm raises the Special Defense of Rock types by half
    if (ServerEvent_GetWeather(flow) == BTL_WEATHER_SANDSTORM && DoesMonHaveType(defender, TYPE_ROCK) && stat == 11) {
        defense = fixed_round(defense, 0x1800);
    }
    BattleEventVar_Push(0x3178);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(4, GetMonID(defender));
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEventVar_SetConstValue(0x1a, category);
    BattleEventVar_SetValue(0x34, defense);
    BattleEventVar_SetMulValue(0x35, 0x1000, 0x19a, 0x20000);
    BattleEvent_CallHandlers(flow, 0x3c);
    defense = BattleEventVar_GetValue(0x34);
    ratio = BattleEventVar_GetValue(0x35);
    BattleEventVar_Pop(0x3183);
    return fixed_round(defense, ratio);
}

fx32 ServerEvent_SameTypeAttackBonus(BtlServerFlow *flow, BattleMon *attacker, u8 type) {
    BOOL stab;
    fx32 ratio;

    stab = DoesMonHaveType(attacker, type);
    ratio = 0x1000;
    BattleEventVar_Push(0x3199);
    BattleEventVar_SetConstValue(2, GetMonID(attacker));
    BattleEventVar_SetRewriteOnceValue(0x51, stab);
    BattleEvent_CallHandlers(flow, 0x40);
    stab = BattleEventVar_GetValue(0x51);
    if (stab) {
        ratio = 0x1800;
    }
    BattleEventVar_SetConstValue(0x44, stab);
    BattleEventVar_SetValue(0x35, ratio);
    BattleEvent_CallHandlers(flow, 0x41);
    ratio = BattleEventVar_GetValue(0x35);
    BattleEventVar_Pop(0x31aa);
    return ratio;
}

// The chance that a move makes its target flinch
u32 ServerEvent_GetFlinchChance(BtlServerFlow *flow, u16 move, BattleMon *attacker) {
    BOOL failed;
    BOOL doubled;
    u32 bonus;
    u32 chance;

    chance = PML_MoveGetParam(move, MOVE_PARAM_FLINCH_CHANCE);
    BattleEventVar_Push(0x31be);
    BattleEventVar_SetConstValue(3, GetMonID(attacker));
    BattleEventVar_SetConstValue(0x25, chance);
    BattleEventVar_SetRewriteOnceValue(0x45, FALSE);
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEventVar_SetValue(0x26, 0);
    BattleEvent_CallHandlers(flow, 0x6c);
    failed = BattleEventVar_GetValue(0x41);
    doubled = BattleEventVar_GetValue(0x45);
    bonus = BattleEventVar_GetValue(0x26);
    BattleEventVar_Pop(0x31c9);
    if (failed && chance != 0) {
        return 0;
    }
    if (chance == 0) {
        chance += bonus;
    }
    if (doubled) {
        chance *= 2;
    }
    return chance;
}

BOOL ServerEvent_CheckFlinch(BtlServerFlow *flow, BattleMon *mon, u32 chance) {
    BOOL failed;
    BOOL result;
    u32 rate;

    BattleEventVar_Push(0x31ea);
    BattleEventVar_SetConstValue(4, GetMonID(mon));
    BattleEventVar_SetValue(0x26, chance);
    result = FALSE;
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEvent_CallHandlers(flow, 0x6d);
    rate = BattleEventVar_GetValue(0x26);
    failed = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x31f2);
    if (failed) {
        return result;
    }
    if (rate != 0) {
        if (BtlServerFlow_CheckDebugFlag(flow, FALSE)) {
            return TRUE;
        }
        if (BattleRandom(100) < (u8)rate) {
            result = TRUE;
        }
        return result;
    }
    return result;
}

void ServerEvent_FlinchFail(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0x320b);
    BattleEventVar_SetValue(2, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0x6e);
    BattleEventVar_Pop(0x320e);
}

BOOL ServerEvent_CheckStatChange(BtlServerFlow *flow, BattleMon *mon, u32 stat, u8 monId, s32 change, u32 value) {
    BOOL failed;

    BattleEventVar_Push(0x3222);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(3, monId);
    BattleEventVar_SetConstValue(0x1f, stat);
    BattleEventVar_SetConstValue(0x20, change);
    BattleEventVar_SetConstValue(0x19, value);
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEvent_CallHandlers(flow, 0x5b);
    failed = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x322b);
    return !failed;
}

void ServerEvent_StatChangeFailed(BtlServerFlow *flow, BattleMon *mon, u32 value) {
    BattleEventVar_Push(0x323b);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x19, value);
    BattleEvent_CallHandlers(flow, 0x5c);
    BattleEventVar_Pop(0x323f);
}

void ServerEvent_StatChangeApplied(BtlServerFlow *flow, u8 monId, BattleMon *mon, u32 stat, s32 change) {
    BattleEventVar_Push(0x324f);
    BattleEventVar_SetConstValue(3, monId);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x1f, stat);
    BattleEventVar_SetConstValue(0x20, change);
    BattleEvent_CallHandlers(flow, 0x5d);
    BattleEventVar_Pop(0x3255);
}

void ServerEvent_MoveStatChangeApplied(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 stat, s32 change) {
    BattleEventVar_Push(0x3265);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x12, move);
    BattleEventVar_SetConstValue(0x1f, stat);
    BattleEventVar_SetConstValue(0x20, change);
    BattleEvent_CallHandlers(flow, 0x5e);
    BattleEventVar_Pop(0x326b);
}

u16 ServerEvent_CalcDrainAmount(BtlServerFlow *flow, BattleMon *mon, BattleMon *source, u16 amount) {
    u8 sourceId;
    u32 ratio;

    sourceId = source != NULL ? GetMonID(source) : 0x1f;
    BattleEventVar_Push(0x327f);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(4, sourceId);
    BattleEventVar_SetMulValue(0x35, 0x1000, 0x19a, 0x20000);
    BattleEventVar_SetValue(0x20, amount);
    BattleEvent_CallHandlers(flow, 0x8c);
    amount = BattleEventVar_GetValue(0x20);
    ratio = BattleEventVar_GetValue(0x35);
    if (amount != 0) {
        amount = GetRatioOverZero(amount, ratio);
    }
    BattleEventVar_RewriteValue(0x20, amount);
    BattleEvent_CallHandlers(flow, 0x8d);
    amount = BattleEventVar_GetValue(0x20);
    BattleEventVar_Pop(0x3290);
    return amount;
}

void ServerEvent_AfterWeatherChange(BtlServerFlow *flow, u8 weather) {
    BattleEventVar_Push(0x32a3);
    BattleEventVar_SetConstValue(0x39, weather);
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEvent_CallHandlers(flow, 0x7d);
    if (!BattleEventVar_GetValue(0x41)) {
        BattleEvent_CallHandlers(flow, 0x7e);
    }
    BattleEventVar_Pop(0x32ab);
}

u32 ServerEvent_CalcMoveHealAmount(BtlServerFlow *flow, u16 move, BattleMon *mon) {
    u32 ratio;
    fx32 multiplier;
    u32 maxHP;
    u32 amount;

    ratio = PML_MoveGetParam(move, MOVE_PARAM_HEAL);
    BattleEventVar_Push(0x32bd);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetValue(0x35, 0);
    BattleEvent_CallHandlers(flow, 0x8f);
    multiplier = BattleEventVar_GetValue(0x35);
    BattleEventVar_Pop(0x32c2);
    maxHP = GetBattleMonStat(mon, 0xe);
    if (multiplier != 0) {
        amount = fixed_round(maxHP, multiplier);
    } else {
        amount = MultiplyValueByRatio(maxHP, ratio);
    }
    if (amount == 0) {
        return 1;
    }
    if (amount > maxHP) {
        amount = maxHP;
    }
    return amount;
}

u32 ServerEvent_CheckItemSet(BtlServerFlow *flow, BattleMon *mon, u16 item) {
    u32 failed;

    BattleEventVar_Push(0x32e5);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x2d, item);
    BattleEventVar_SetRewriteOnceValue(0x41, FALSE);
    BattleEvent_CallHandlers(flow, 0x9a);
    failed = BattleEventVar_GetValue(0x41);
    BattleEventVar_Pop(0x32eb);
    return failed;
}

void ServerEvent_ItemSetFailed(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0x32fa);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0x9b);
    BattleEventVar_Pop(0x32fd);
}

void ServerEvent_ItemSetDecide(BtlServerFlow *flow, BattleMon *mon, u16 item) {
    BattleEventVar_Push(0x330a);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEventVar_SetConstValue(0x2d, item);
    BattleEvent_CallHandlers(flow, 0x9c);
    BattleEventVar_Pop(0x330e);
}

void ServerEvent_ItemSetFixed(BtlServerFlow *flow, BattleMon *mon) {
    BattleEventVar_Push(0x331b);
    BattleEventVar_SetConstValue(2, GetMonID(mon));
    BattleEvent_CallHandlers(flow, 0x9d);
    BattleEventVar_Pop(0x331e);
}

void ServerEvent_ChangeAbilityBefore(BtlServerFlow *flow, u8 monIndex, u16 oldAbility, u16 newAbility) {
    BattleEventVar_Push(0x332d);
    BattleEventVar_SetConstValue(2, monIndex);
    BattleEventVar_SetConstValue(0xf, oldAbility);
    BattleEventVar_SetConstValue(0x10, newAbility);
    BattleEvent_CallHandlers(flow, 0x89);
    BattleEventVar_Pop(0x3332);
}

void ServerEvent_ChangeAbilityAfter(BtlServerFlow *flow, u8 monIndex) {
    BattleEventVar_Push(0x333e);
    BattleEventVar_SetConstValue(2, monIndex);
    BattleEvent_CallHandlers(flow, 0x8a);
    BattleEventVar_Pop(0x3341);
}

void ServerEvent_CheckSideEffectParam(BtlServerFlow *flow, u8 monId, u32 effect, u8 side, BattleCondition *cont) {
    BattleEventVar_Push(0x334f);
    BattleEventVar_SetConstValue(2, monId);
    BattleEventVar_SetConstValue(0x52, side);
    BattleEventVar_SetConstValue(0x53, effect);
    BattleEventVar_SetValue(0x1e, cont->raw);
    BattleEvent_CallHandlers(flow, 0x9f);
    cont->raw = BattleEventVar_GetValue(0x1e);
    BattleEventVar_Pop(0x3356);
}

void ServerEvent_NotifyAirLock(BtlServerFlow *flow) {
    BattleEventVar_Push(0x3364);
    BattleEvent_CallHandlers(flow, 0x79);
    BattleEventVar_Pop(0x3366);
}

void BtlServerFlow_ClearEffectivenessCounts(u16 *counts) {
    MI_CpuFill8(counts, 0, 6 * sizeof(u16));
}

// Counts the effectiveness of the hits between the player's mons and their opponents', for the records
void BtlServerFlow_CountEffectiveness(u16 *counts, BtlServerFlow *flow, BattleMon *mon, BattleMon *target,
                                      s32 effectiveness) {
    u8 monId;
    u8 targetId;
    u8 clientId;
    u8 targetClientId;
    u8 opposing;
    u16 *count;

    monId = GetMonID(mon);
    targetId = GetMonID(target);
    clientId = GetClientIDFromMonID(monId);
    targetClientId = GetClientIDFromMonID(targetId);
    opposing = !IsAllyMonID(monId, targetId) ? TRUE : FALSE;
    count = NULL;
    if (IsPlayerMon(flow, mon) && opposing && effectiveness > TYPE_EFFECTIVENESS_NORMAL) {
        IncRecord(flow->mainModule, 0x2f);
    }
    if (clientId == 0) {
        if (opposing) {
            if (effectiveness == TYPE_EFFECTIVENESS_IMMUNE) {
                count = &counts[0];
            } else if (effectiveness > TYPE_EFFECTIVENESS_NORMAL) {
                count = &counts[1];
            } else if (effectiveness < TYPE_EFFECTIVENESS_NORMAL) {
                count = &counts[2];
            }
        }
    } else if (targetClientId == 0 && opposing) {
        if (effectiveness == TYPE_EFFECTIVENESS_IMMUNE) {
            count = &counts[3];
        } else if (effectiveness > TYPE_EFFECTIVENESS_NORMAL) {
            count = &counts[4];
        } else if (effectiveness < TYPE_EFFECTIVENESS_NORMAL) {
            count = &counts[5];
        }
    }
    if (count != NULL && *count < 9999) {
        (*count)++;
    }
}

u16 BtlServerFlow_GetPlayerImmuneHitCount(BtlServerFlow *flow) {
    return flow->unk1F80[0];
}

u16 BtlServerFlow_GetPlayerSuperEffectiveHitCount(BtlServerFlow *flow) {
    return flow->unk1F80[1];
}

u16 BtlServerFlow_GetPlayerNotVeryEffectiveHitCount(BtlServerFlow *flow) {
    return flow->unk1F80[2];
}

u16 BtlServerFlow_GetFoeImmuneHitCount(BtlServerFlow *flow) {
    return flow->unk1F80[3];
}

u16 BtlServerFlow_GetFoeNotVeryEffectiveHitCount(BtlServerFlow *flow) {
    return flow->unk1F80[5];
}

BattleMon *GetBattleMon(BtlServerFlow *flow, u8 monId) {
    return GetPokeParam(flow->pokeCon, monId);
}

u8 BtlServerFlow_GetMonBattlePos(BtlServerFlow *flow, u8 monId) {
    u8 pos;

    pos = GetBattlePos(&flow->posPoke, monId);
    if (!IsActivePos(flow->mainModule, pos)) {
        pos = 6;
    }
    return pos;
}

u8 BtlServerFlow_GetPosOfMon(BtlServerFlow *flow, u8 monId) {
    return BtlPosPoke_GetPosOfMon(&flow->posPoke, monId);
}

u8 BtlServerFlow_GetMonIDAtPos(BtlServerFlow *flow, u8 pos) {
    return GetExistPokeID(&flow->posPoke, pos);
}

// The IDs of the mons in battle that are on the other side from a mon
u8 BtlServerFlow_GetOpposingMonIDs(BtlServerFlow *flow, u8 monId, u8 *monIds) {
    BtlFlowMonIter iter;
    BattleMon *mon;
    u8 count = 0;

    BtlFlowMonIter_Init(&iter, flow);
    while (BtlFlowMonIter_Next(&iter, flow, &mon)) {
        if (!IsAllyMonID(monId, GetMonID(mon))) {
            monIds[count++] = GetMonID(mon);
        }
    }
    return count;
}

u32 CalcMoveEffectiveness(BtlServerFlow *flow, u8 attackerId, u8 defenderId, u16 move) {
    BattleMon *attacker;
    BattleMon *defender;
    BtlFlowMoveParam param;
    u32 effectiveness;

    attacker = GetPokeParam(flow->pokeCon, attackerId);
    defender = GetPokeParam(flow->pokeCon, defenderId);
    flow->unk774++;
    if (IsIllusionEnabled(defender)) {
        defender = GetIllusionDisguise(flow->mainModule, flow->pokeCon, defender);
    }
    ServerEvent_GetMoveParam(flow, move, attacker, &param);
    effectiveness = ServerControl_CheckMoveEffectiveness(flow, attacker, defender, &param, FALSE);
    flow->unk774--;
    return effectiveness;
}

u32 AICalcDamage(BtlServerFlow *flow, u8 attackerId, u8 defenderId, u16 move, BOOL withEffectiveness, u32 damageRoll) {
    BattleMon *attacker;
    BattleMon *defender;
    u32 effectiveness;
    BtlFlowMoveParam param;
    u16 damage;

    if (move != 0 && PML_MoveIsDamaging(move)) {
        attacker = GetPokeParam(flow->pokeCon, attackerId);
        defender = GetPokeParam(flow->pokeCon, defenderId);
        flow->unk774++;
        if (IsIllusionEnabled(defender)) {
            defender = GetIllusionDisguise(flow->mainModule, flow->pokeCon, defender);
        }
        if (withEffectiveness) {
            effectiveness = CalcMoveEffectiveness(flow, attackerId, defenderId, move);
        } else {
            effectiveness = TYPE_EFFECTIVENESS_NORMAL;
        }
        ServerEvent_GetMoveParam(flow, move, attacker, &param);
        ServerEvent_CalcDamage(flow, attacker, defender, &param, effectiveness, 0x1000, FALSE,
                               damageRoll == USE_MIN_DAMAGE ? TRUE : FALSE, &damage);
        flow->unk774--;
        return damage;
    }
    return 0;
}

// Whether the flow is only simulating a move, for the AI
BOOL BtlServerFlow_IsSimulating(BtlServerFlow *flow) {
    return flow->unk774 != 0 ? TRUE : FALSE;
}

u8 BtlServerFlow_GetChatterEffect(BtlServerFlow *flow, u8 monId) {
    return GetClientChatterEffect(flow->mainModule, GetClientIDFromMonID(monId));
}

u8 BtlServerFlow_GetClientCoverPosCount(BtlServerFlow *flow, u8 monId) {
    return BattleServer_GetClient(flow->server, GetClientIDFromMonID(monId))->numCoverPos;
}

u8 BtlServerFlow_GetClientFrontSlotCount(BtlServerFlow *flow, u8 monId) {
    if (BtlSetup_GetBattleStyle(flow->mainModule) != BTL_STYLE_ROTATION) {
        return BtlServerFlow_GetClientCoverPosCount(flow, monId);
    }
    return 3;
}

BOOL BtlServerFlow_HasReserveMon(BtlServerFlow *flow, u8 monId) {
    BattleParty *party;

    party = BtlServerFlow_GetMonParty(flow, monId);
    if (BattleParty_CountAliveFrom(party, BtlServerFlow_GetClientFrontSlotCount(flow, monId))) {
        return TRUE;
    }
    return FALSE;
}

// Sets a flag that can only be set once, and returns whether this call set it
BOOL func_ov167_021aba8c(BtlServerFlow *flow) {
    if (flow->unk78A_3) {
        return FALSE;
    }
    flow->unk78A_3 = 1;
    return TRUE;
}

u8 HandlerGetAlivePartyCount(BtlServerFlow *flow, u16 code, u8 *monIds) {
    u8 positions[6];
    u8 numPositions;
    u8 count;
    u8 i;
    BattleMon *mon;

    numPositions = ExpandPosFlags(flow->mainModule, code, positions);
    for (i = 0, count = 0; i < numPositions; i++) {
        mon = PokeCon_GetPosMon(flow->pokeCon, positions[i]);
        if (BtlFlow_IsMonAlive(mon)) {
            monIds[count++] = GetMonID(mon);
        }
    }
    return count;
}

BattleParty *BtlServerFlow_GetMonParty(BtlServerFlow *flow, u8 monId) {
    return GetClientParty(flow->pokeCon, GetClientIDFromMonID(monId));
}

BattleParty *BtlServerFlow_GetAllyParty(BtlServerFlow *flow, u8 monId) {
    u8 clientId;

    clientId = GetAllyClientID(flow->mainModule, GetClientIDFromMonID(monId));
    if (DoesClientExist(flow->mainModule, clientId)) {
        return GetClientParty(flow->pokeCon, clientId);
    }
    return NULL;
}

u8 BtlServerFlow_MonIDToPos(BtlServerFlow *flow, u8 monId) {
    return MonIDToBattlePos(flow->mainModule, flow->pokeCon, monId);
}

u8 BtlServerFlow_PosToMonID(BtlServerFlow *flow, u8 pos) {
    return GetMonID(PokeCon_GetPosMonInner(flow->pokeCon, pos));
}

u8 BtlServerFlow_GetMoveTargetPos(BtlServerFlow *flow, u8 monId, u16 move) {
    return DecideMoveTargetAuto(flow->mainModule, flow->pokeCon, GetPokeParam(flow->pokeCon, monId), move);
}

// The action a mon still has to take this turn
BOOL BtlServerFlow_GetPendingAction(BtlServerFlow *flow, u8 monId, BattleAction *action) {
    u32 i;

    for (i = 0; i < flow->actionOrderCount; i++) {
        if (monId == GetMonID(flow->actionOrder[i].mon) &&
            flow->actionOrder[i].action.bits.action != BATTLE_ACTION_ROTATION) {
            *action = flow->actionOrder[i].action;
            return TRUE;
        }
    }
    return FALSE;
}

// Whether every other mon has taken its action this turn
BOOL BtlServerFlow_IsLastToAct(BtlServerFlow *flow, u8 monId) {
    u32 i;
    u32 others;
    u32 done;

    done = 0;
    others = 0;
    for (i = 0; i < flow->actionOrderCount; i++) {
        if (monId != GetMonID(flow->actionOrder[i].mon)) {
            others++;
            if (flow->actionOrder[i].done) {
                done++;
            }
        }
    }
    return others == done ? TRUE : FALSE;
}

u16 BtlServerFlow_GetLastUsedMove(BtlServerFlow *flow) {
    return flow->unk1F78;
}

// A work buffer that the event handlers share, of at least size bytes
u8 *BtlServerFlow_GetHandlerWork(BtlServerFlow *flow, u32 size) {
    return flow->unk1FF0;
}

BtlWazaRec *BtlServerFlow_GetWazaRec(BtlServerFlow *flow) {
    return &flow->wazaRec;
}

BtlFaintRec *BtlServerFlow_GetFaintRec(BtlServerFlow *flow) {
    return &flow->faintRec;
}

u16 GetTurnCounter(BtlServerFlow *flow) {
    return flow->unk10;
}

u8 BtlServerFlow_GetSwitchCount(BtlServerFlow *flow, u32 index) {
    return flow->unk7D9[index];
}

BOOL BtlServerFlow_IsMonInBattle(BtlServerFlow *flow, u8 monId) {
    return DoesBattleMonExist(&flow->posPoke, monId);
}

u32 BtlServerFlow_GetBattleStyle(BtlServerFlow *flow) {
    return BtlSetup_GetBattleStyle(flow->mainModule);
}

u32 BtlServerFlow_GetBattleType(BtlServerFlow *flow) {
    return BtlSetup_GetBattleType(flow->mainModule);
}

u32 GetBattleTerrain(BtlServerFlow *flow) {
    return GetFieldEffectData(flow->mainModule)->env.terrain;
}

u32 BtlServerFlow_GetFrontPosCount(BtlServerFlow *flow) {
    return GetActivePosCount(flow->mainModule);
}

// Whether a mon's species can evolve
BOOL CheckEvolution(BtlServerFlow *flow, u8 monId) {
    u16 species;
    u16 i;

    species = GetBattleMonSpecies(GetPokeParam(flow->pokeCon, monId));
    for (i = 0; i < 7; i++) {
        if (getEvolutionInfoParam(flow->unk4A4, species, 0, i)) {
            return TRUE;
        }
    }
    return FALSE;
}

u16 BtlServerFlow_CalcSpeed(BtlServerFlow *flow, BattleMon *mon, BOOL flag) {
    return ServerEvent_CalculateSpeed(flow, mon, flag);
}

// How many mons in battle are faster than a mon
u32 BtlServerFlow_CountFasterMons(BtlServerFlow *flow, BattleMon *mon, BOOL flag) {
    u16 speed;
    u8 monId;
    BtlFlowMonIter iter;
    BattleMon *other;
    u16 count;

    speed = BtlServerFlow_CalcSpeed(flow, mon, flag);
    monId = GetMonID(mon);
    count = 0;
    BtlFlowMonIter_Init(&iter, flow);
    while (BtlFlowMonIter_Next(&iter, flow, &other)) {
        if (monId != GetMonID(other) && BtlServerFlow_CalcSpeed(flow, other, flag) > speed) {
            count++;
        }
    }
    return count;
}

BOOL BtlServerFlow_IsFloating(BtlServerFlow *flow, u8 monId) {
    return ServerControl_CheckFloating(flow, GetPokeParam(flow->pokeCon, monId), TRUE);
}

// Whether a mon can use its held item: not with Klutz, under Embargo or in Magic Room
BOOL BtlServerFlow_CanUseHeldItem(BtlServerFlow *flow, u8 monId) {
    BattleMon *mon;

    mon = GetPokeParam(flow->pokeCon, monId);
    if (GetBattleMonStat(mon, 0x11) == ABILITY_KLUTZ) {
        return FALSE;
    }
    if (CheckCondition(mon, CONDITION_EMBARGO)) {
        return FALSE;
    }
    if (IsFieldEffectActive(7)) {
        return FALSE;
    }
    return TRUE;
}

u8 GetWeather(BtlServerFlow *flow) {
    return ServerEvent_GetWeather(flow);
}

BOOL BtlServerFlow_IsTargetOutOfRange(BtlServerFlow *flow, u8 attackerId, u8 defenderId, u16 move) {
    BattleMon *attacker;

    attacker = GetPokeParam(flow->pokeCon, attackerId);
    return IsTargetOutOfRange(flow, attacker, GetPokeParam(flow->pokeCon, defenderId), move);
}

BOOL BtlServerFlow_CheckDebugFlag(BtlServerFlow *flow, u32 value) {
    return ReturnZero(flow->mainModule, value);
}

BOOL BtlServerFlow_IsSideEffectSet(BtlServerFlow *flow, u8 side, u32 sideEffect) {
    return IsSideEffectSet(side, sideEffect);
}

u32 BtlServerFlow_GetSideEffectLayers(BtlServerFlow *flow, u8 pos, u32 sideEffect) {
    return GetSideEffectLayers(GetPosSide(pos), sideEffect);
}

BOOL BtlServerFlow_IsPosEffectFree(BtlServerFlow *flow, u8 pos, u32 a2) {
    return IsPosEffectFree(a2, pos);
}

u8 BtlServerFlow_GetAllyClientID(BtlServerFlow *flow, u8 clientId) {
    return GetAllyClientID(flow->mainModule, clientId);
}

BOOL IsMonSwitchingOut(BtlServerFlow *flow) {
    return flow->unk78A_0;
}

void AddSwitchOutInterrupt(BtlServerFlow *flow, u8 monId) {
    if (flow->interruptCount < 6) {
        flow->interruptMonIds[flow->interruptCount++] = monId;
    }
}

BOOL BtlServerFlow_IsSemiInvuln(BtlServerFlow *flow, u8 monId) {
    return IsSemiInvulnMove(GetPokeParam(flow->pokeCon, monId));
}

// Whether a mon is carrying another off with Sky Drop
BOOL BtlServerFlow_IsSkyDropAttacker(BtlServerFlow *flow, u8 monId) {
    if (BtlFlow_GetSkyDropTarget(GetPokeParam(flow->pokeCon, monId)) != 0x1f) {
        return TRUE;
    }
    return FALSE;
}

// Whether a mon is either side of a Sky Drop
BOOL BtlServerFlow_IsInSkyDrop(BtlServerFlow *flow, u8 monId) {
    if (BtlServerFlow_IsSkyDropAttacker(flow, monId)) {
        return TRUE;
    }
    if (CheckCondition(GetPokeParam(flow->pokeCon, monId), 0x21)) {
        return TRUE;
    }
    return FALSE;
}

s32 BtlServerFlow_GetMonWeight(BtlServerFlow *flow, u8 monId) {
    BattleMon *mon;
    fx32 ratio;
    u32 weight;

    mon = GetPokeParam(flow->pokeCon, monId);
    ratio = ServerEvent_GetWeightRatio(flow, mon);
    weight = fixed_round(GetBattleMonWeight(mon), ratio);
    if (weight < 1) {
        weight = 1;
    }
    return weight;
}

u32 BtlServerFlow_GetNextSerial(BtlServerFlow *flow) {
    return ServerControl_NextSerial(flow);
}

BOOL BtlServerFlow_CheckMatchup(BtlServerFlow *flow) {
    return ServerControl_CheckMatchup(flow);
}

void SetMoveEffectIndex(BtlServerFlow *flow, u8 index) {
    flow->moveEffect->index = index;
}

// Pay Day's money, in the battles that give it
BOOL BtlServerFlow_AddPayDayMoney(BtlServerFlow *flow, u32 money, u8 monId) {
    if (BtlSetup_GetBattleType(flow->mainModule) <= 1) {
        BattleServer_AddPayDayMoney(flow->server, money);
        return TRUE;
    }
    return FALSE;
}

void BtlServerFlow_DoublePrizeMoney(BtlServerFlow *flow, u8 monId) {
    u32 type;
    u8 clientId;

    type = BtlSetup_GetBattleType(flow->mainModule);
    clientId = GetClientIDFromMonID(monId);
    if (type <= 1 && clientId == GetPlayerClientID(flow->mainModule)) {
        BattleServer_SetDoublePrizeMoney(flow->server);
    }
}

void BtlServerFlow_AddMagicCoatReflect(BtlServerFlow *flow, u8 monId, u8 targetId) {
    BtlFlowReactionList_Add(flow->unk4B0, monId, targetId, GetBattlePos(&flow->posPoke, targetId));
}

BOOL BtlServerFlow_StartSkyDrop(BtlServerFlow *flow, u8 attackerId, u8 targetId, BOOL *failed) {
    BattleMon *attacker;

    attacker = GetPokeParam(flow->pokeCon, attackerId);
    return ServerControl_SkyDropCatch(flow, attacker, GetPokeParam(flow->pokeCon, targetId), failed);
}

void BtlServerFlow_SkyDropRelease(BtlServerFlow *flow, u8 monId) {
    ServerControl_SkyDropCheckRelease(flow, GetPokeParam(flow->pokeCon, monId), TRUE);
}

BOOL BtlServerFlow_UseHeldItem(BtlServerFlow *flow, BattleMon *mon, u8 *flag) {
    BOOL result;

    flow->unk78A_5 = 0;
    result = ServerControl_UseHeldItem(flow, mon);
    *flag = flow->unk78A_5;
    return result;
}

void BtlServerFlow_TriggerAbility(BtlServerFlow *flow, BattleMon *mon) {
    ServerEvent_AbilityMemberIn(flow, mon);
}

u32 BtlServerFlow_GetBattleResult(BtlServerFlow *flow) {
    return CalcBattleResult(flow);
}

void BtlServerFlow_NotifyRevive(BtlServerFlow *flow, u8 monId) {
    BtlServerFlow_AddRevivedMon(flow, monId);
}

void BtlServerFlow_ClearRevivedMons(BtlServerFlow *flow) {
    flow->unk785 = 0;
}

void BtlServerFlow_AddRevivedMon(BtlServerFlow *flow, u8 monId) {
    u32 i;

    for (i = 0; i < flow->unk785; i++) {
        if (monId == flow->unk791[i]) {
            return;
        }
    }
    if (i < 24) {
        flow->unk791[i] = monId;
        flow->unk785++;
    }
}

BOOL BtlServerFlow_CheckRevivedSwitchIn(BtlServerFlow *flow) {
    u32 i;
    u8 positions[3];

    for (i = 0; i < flow->unk785; i++) {
        if (BtlPosPoke_GetEmptyPositions(&flow->posPoke, GetClientIDFromMonID(flow->unk791[i]), positions)) {
            return TRUE;
        }
    }
    return FALSE;
}

void BtlServerFlow_ResetFrames(BtlServerFlow *flow) {
    flow->frameDepth = 0;
    BtlServerFlow_SetFrame(flow, 0, TRUE);
}

void BtlServerFlow_PushFrame(BtlServerFlow *flow) {
    if (flow->frameDepth < 6) {
        flow->frameDepth++;
        BtlServerFlow_SetFrame(flow, flow->frameDepth, TRUE);
    }
}

void BtlServerFlow_PopFrame(BtlServerFlow *flow) {
    if (flow->frameDepth != 0) {
        flow->frameDepth--;
        BtlServerFlow_SetFrame(flow, flow->frameDepth, FALSE);
    }
}

// Points the flow at one level of its work, clearing it when the level is entered
void BtlServerFlow_SetFrame(BtlServerFlow *flow, u32 depth, BOOL clear) {
    BtlFlowWorkFrame *frame = &flow->frames[depth];

    flow->unk850 = frame->monSets[0];
    flow->unk854 = frame->monSets[1];
    flow->unk858 = frame->monSets[2];
    flow->unk85C = frame->monSets[3];
    flow->unk860 = frame->monSets[4];
    flow->unk864 = frame->monSets[5];
    flow->unk868 = frame->monSets[6];
    flow->moveEffect = &frame->moveEffect;
    flow->unk1AB0 = &frame->moveParams[0];
    flow->unk1AB4 = &frame->moveParams[1];
    flow->unk4B4 = &frame->hitWork;
    flow->unk4AC = &frame->reactionLists[0];
    flow->unk4B0 = &frame->reactionLists[1];
    flow->unk86C = &frame->damageLists[0];
    flow->unk870 = &frame->damageLists[1];
    flow->unk77F = frame->unk28C;
    flow->unk78A_3 = frame->unk28D;
    flow->unk78A_4 = frame->unk28E;
    if (clear) {
        BtlMonSet_Clear(flow->unk850);
        BtlMonSet_Clear(flow->unk854);
        BtlMonSet_Clear(flow->unk858);
        BtlMonSet_Clear(flow->unk85C);
        BtlMonSet_Clear(flow->unk860);
        BtlMonSet_Clear(flow->unk864);
        BtlMonSet_Clear(flow->unk868);
        BattleMoveEffectState_Init(flow->moveEffect);
        MI_CpuFill8(flow->unk1AB0, 0, sizeof(BtlFlowMoveParam));
        MI_CpuFill8(flow->unk1AB4, 0, sizeof(BtlFlowMoveParam));
        MI_CpuFill8(flow->unk4B4, 0, sizeof(BtlFlowHitWork));
        MI_CpuFill8(flow->unk4AC, 0, sizeof(BtlFlowReactionList));
        MI_CpuFill8(flow->unk4B0, 0, sizeof(BtlFlowReactionList));
        MI_CpuFill8(flow->unk86C, 0, sizeof(BtlFlowDamageList));
        MI_CpuFill8(flow->unk870, 0, sizeof(BtlFlowDamageList));
        flow->unk77F = 6;
        flow->unk78A_3 = 0;
        flow->unk78A_4 = 0;
    }
}

void BattleHandler_StrClear(BattleHandlerString *string) {
    MI_CpuFill8(string, 0, 0x28);
    string->enabled = 0;
}

BOOL BattleHandler_StrIsEnabled(const BattleHandlerString *string) {
    return string->enabled != 0;
}

void BattleHandler_StrSetup(BattleHandlerString *string, u32 enabled, u16 message) {
    string->enabled = enabled;
    string->message = message;
    string->count = 0;
}

void BattleHandler_AddArg(BattleHandlerString *string, u32 arg) {
    u16 count;

    count = string->count;
    if (count < 9) {
        string->count = count + 1;
        string->args[count] = arg;
    }
}

void BattleHandler_AddSoundEffect(BattleHandlerString *string, u32 soundEffect) {
    if (string->count < 9) {
        string->soundEffect = soundEffect;
        string->hasSound = 1;
    }
}

void *BattleHandler_PushWork(BtlServerFlow *flow, u32 command, u32 monId) {
    return PushWork(&flow->actionState, command, monId);
}

void BattleHandler_PushRun(BtlServerFlow *flow, u32 command, u32 monId) {
    void *work;

    work = BattleHandler_PushWork(flow, command, monId);
    BattleHandler_PopWork(flow, work);
}

void BattleHandler_PopWork(BtlServerFlow *flow, void *work) {
    BattleHandler_Execute(flow, work);
    PopWork(&flow->actionState, work);
}

u32 BattleHandler_Result(BtlServerFlow *handler) {
    BtlActionState *state;

    state = &handler->actionState;
    if (IsUsed(state)) {
        if (GetResult(state)) {
            return 2;
        }
        return 1;
    }
    return 0;
}

u32 BtlServerFlow_GetHandlerResult(BtlServerFlow *flow) {
    return BattleHandler_Result(flow);
}

// Function name from swan.
void BattleHandler_Execute(BtlServerFlow *flow, void *work) {
    BattleHandlerHeader *header = work;
    u16 itemId;
    u8 result;

    itemId = GetUseItemNo(&flow->actionState);
    if (IsUsed(&flow->actionState)) {
        result = GetPrevResult(&flow->actionState);
    } else {
        result = TRUE;
    }
    if ((header->checkPrevResult && !result) ||
        (header->checkFainted && IsFainted(GetPokeParamConst(flow->pokeCon, header->monId)))) {
        return;
    }
    switch (header->command) {
    case 1:
        result = BattleHandler_UseItemAnim(flow, work);
        break;
    case 2:
        result = BattleHandler_AbilityPopupAdd(flow, work);
        break;
    case 3:
        result = BattleHandler_AbilityPopupRemove(flow, work);
        break;
    case 0:
        result = BattleHandler_UseHeldItem(flow, work);
        break;
    case 5:
        result = BattleHandler_RecoverHP(flow, work, itemId);
        break;
    case 6:
        result = BattleHandler_Drain(flow, work, itemId);
        break;
    case 7:
        result = BattleHandler_Damage(flow, work);
        break;
    case 8:
        result = BattleHandler_ChangeHP(flow, work);
        break;
    case 9:
        result = BattleHandler_RecoverPP(flow, work, itemId);
        break;
    case 10:
        result = BattleHandler_DecrementPP(flow, work, itemId);
        break;
    case 11:
        result = BattleHandler_CureCondition(flow, work, itemId);
        break;
    case 12:
        result = BattleHandler_AddCondition(flow, work);
        break;
    case 14:
        result = BattleHandler_StatChange(flow, work, itemId);
        break;
    case 15:
        result = BattleHandler_SetStatStage(flow, work);
        break;
    case 18:
        result = BattleHandler_RecoverStatStage(flow, work);
        break;
    case 16:
        result = BattleHandler_ResetStatStage(flow, work);
        break;
    case 17:
        result = BattleHandler_SetStatus(flow, work);
        break;
    case 19:
        result = BattleHandler_Faint(flow, work);
        break;
    case 20:
        result = BattleHandler_ChangeType(flow, work);
        break;
    case 4:
        result = BattleHandler_Message(flow, work);
        break;
    case 21:
        result = BattleHandler_SetTurnFlag(flow, work);
        break;
    case 22:
        result = BattleHandler_ResetTurnFlag(flow, work);
        break;
    case 23:
        result = BattleHandler_SetContinueFlag(flow, work);
        break;
    case 24:
        result = BattleHandler_ResetContinueFlag(flow, work);
        break;
    case 25:
        result = BattleHandler_AddSideEffect(flow, work);
        break;
    case 26:
        result = BattleHandler_RemoveSideEffectCore(flow, work);
        break;
    case 27:
        result = BattleHandler_AddFieldEffect(flow, work);
        break;
    case 29:
        result = BattleHandler_ChangeWeather(flow, work);
        break;
    case 28:
        result = BattleHandler_RemoveFieldEffect(flow, work);
        break;
    case 30:
        result = BattleHandler_AddPosEffect(flow, work);
        break;
    case 31:
        result = BattleHandler_AbilityChange(flow, work);
        break;
    case 32:
        result = BattleHandler_SetItem(flow, work);
        break;
    case 33:
        result = BattleHandler_CheckHeldItem(flow, work);
        break;
    case 34:
        result = BattleHandler_ForceUseItem(flow, work);
        break;
    case 35:
        result = BattleHandler_ConsumeItem(flow, work);
        break;
    case 36:
        result = BattleHandler_SwapItem(flow, work);
        break;
    case 37:
        result = BattleHandler_UpdateMove(flow, work);
        break;
    case 38:
        result = BattleHandler_SetCounter(flow, work);
        break;
    case 39:
        result = BattleHandler_DelayMoveDamage(flow, work);
        break;
    case 40:
        result = BattleHandler_QuitBattle(flow, work);
        break;
    case 41:
        result = BattleHandler_Switch(flow, work);
        break;
    case 42:
        result = BattleHandler_BatonPass(flow, work);
        break;
    case 43:
        result = BattleHandler_Flinch(flow, work);
        break;
    case 44:
        result = BattleHandler_Revive(flow, work);
        break;
    case 45:
        result = BattleHandler_SetWeight(flow, work);
        break;
    case 46:
        result = BattleHandler_ForceSwitch(flow, work);
        break;
    case 47:
        result = BattleHandler_InterruptAction(flow, work);
        break;
    case 48:
        result = BattleHandler_InterruptMove(flow, work);
        break;
    case 49:
        result = BattleHandler_SendLast(flow, work);
        break;
    case 50:
        result = BattleHandler_SwapPoke(flow, work);
        break;
    case 51:
        result = BattleHandler_Transform(flow, work);
        break;
    case 52:
        result = BattleHandler_IllusionBreak(flow, work);
        break;
    case 53:
        result = BattleHandler_GravityCheck(flow, work);
        break;
    case 54:
        result = BattleHandler_HideTurnCancel(flow, work);
        break;
    case 55:
        result = BattleHandler_EffectAtPos(flow, work);
        break;
    case 56:
        result = BattleHandler_RemoveMessageWindow(flow, work);
        break;
    case 57:
        result = BattleHandler_ChangeForm(flow, work);
        break;
    case 58:
        result = BattleHandler_SetMoveEffectIndex(flow, work);
        break;
    case 59:
        result = BattleHandler_SetMoveEffectEnable(flow, work);
        break;
    }
    SetResult(&flow->actionState, result);
}

u8 BattleHandler_UseItemAnim(BtlServerFlow *handler, BattleHandlerHeader *param) {
    if (DoesBattleMonExist(&handler->posPoke, param->monId)) {
        ServerDisplay_UseHeldItem(handler, GetPokeParam(handler->pokeCon, param->monId));
        return TRUE;
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_AbilityPopupAdd(BtlServerFlow *handler, BattleHandlerHeader *param) {
    if (DoesBattleMonExist(&handler->posPoke, param->monId)) {
        ServerDisplay_AbilityPopupAdd(handler, GetPokeParam(handler->pokeCon, param->monId));
        return TRUE;
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_AbilityPopupRemove(BtlServerFlow *handler, BattleHandlerPopupParam *param) {
    BattleMon *mon = GetPokeParam(handler->pokeCon, param->monId);
    ServerDisplay_AbilityPopupRemove(handler, mon);
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_RecoverHP(BtlServerFlow *handler, const BattleHandlerRecoverHPParam *param, u16 itemId) {
    BattleMon *mon;
    BattleMon *target;
    u8 result = FALSE;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    target = GetPokeParam(handler->pokeCon, param->targetIndex);
    if (!ServerControl_RecoverHPCheckFail(handler, target)) {
        if (param->popup) {
            ServerDisplay_AbilityPopupAdd(handler, mon);
        }
        if (!param->skipCheck) {
            result = !ServerControl_RecoverHPCheckFailSpecial(handler, target, TRUE);
        } else {
            result = TRUE;
        }
        if (result) {
            ServerControl_RecoverHPCore(handler, target, param->amount);
            if (param->string.enabled) {
                BattleHandler_SetString(handler, &param->string);
            } else if (itemId != 0) {
                BtlServerCmdQueue_PutMsg(handler->queue, SC_MSG_SET, 0x38c, param->targetIndex, itemId, SC_MSG_ARG_END);
            }
        }
        if (param->popup) {
            ServerDisplay_AbilityPopupRemove(handler, mon);
        }
    }
    return result;
}

// Function name from swan.
u8 BattleHandler_Drain(BtlServerFlow *handler, BattleHandlerDrainParam *param, u16 itemId) {
    BattleMon *source;
    BattleMon *mon;

    source = NULL;
    if (param->sourceIndex != 0x1f) {
        source = GetPokeParam(handler->pokeCon, param->sourceIndex);
    }
    if (DoesBattleMonExist(&handler->posPoke, param->monIndex)) {
        mon = GetPokeParam(handler->pokeCon, param->monIndex);
        if (!IsFainted(mon)) {
            if (ServerControl_DrainCore(handler, mon, source, param->amount)) {
                if (param->string.enabled) {
                    BattleHandler_SetString(handler, &param->string);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_Damage(BtlServerFlow *handler, BattleHandlerDamageParam *param) {
    BattleMon *mon;
    BattleMon *source;

    if (DoesBattleMonExist(&handler->posPoke, param->targetIndex)) {
        mon = GetPokeParam(handler->pokeCon, param->targetIndex);
        source = NULL;
        if (param->sourceIndex != 0x1f) {
            source = GetPokeParam(handler->pokeCon, param->sourceIndex);
        }
        if (!IsFainted(mon)) {
            if (!param->checkSemi || !IsSemiInvulnMove(mon)) {
                if (ServerControl_CheckSimpleDamageEnabled(handler, mon, param->amount)) {
                    if (param->popup) {
                        ServerDisplay_AbilityPopupAdd(handler, source);
                    }
                    if (param->showViewEffect) {
                        ServerControl_ViewEffect(handler, param->effect, param->effectArg1, param->effectArg2, 0, 0);
                    }
                    ServerControl_SimpleDamageCore(handler, mon, param->amount, &param->string);
                    if (param->popup) {
                        ServerDisplay_AbilityPopupRemove(handler, source);
                    }
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_ChangeHP(BtlServerFlow *handler, BattleHandlerChangeHPParam *param) {
    u8 result;
    u32 i;
    BattleMon *mon;

    result = FALSE;
    for (i = 0; i < param->count; i++) {
        if (DoesBattleMonExist(&handler->posPoke, param->monIds[i])) {
            mon = GetPokeParam(handler->pokeCon, param->monIds[i]);
            if (!IsFainted(mon)) {
                ServerDisplay_SimpleHP(handler, mon, param->hpChanges[i], param->suppress == 0);
                if (param->skipReaction == 0) {
                    ServerControl_CheckItemReaction(handler, mon, 1);
                }
                result = TRUE;
            }
        }
    }
    return result;
}

// Function name from swan.
u8 BattleHandler_RecoverPP(BtlServerFlow *handler, const BattleHandlerPPParam *param, u16 itemId) {
    BattleMon *user;
    BattleMon *mon;
    BOOL original;

    user = GetPokeParam(handler->pokeCon, param->header.monId);
    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (CanPokemonBattle(mon) || param->allowFainted) {
        original = !param->currentMoves;
        if (!Move_IsPPFull(mon, param->moveIndex, original)) {
            ServerDisplay_RecoverPP(handler, mon, param->moveIndex, param->amount, original);
            BattleHandler_SetString(handler, &param->string);
            return TRUE;
        }
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_DecrementPP(BtlServerFlow *handler, BattleHandlerPPParam *param, u16 itemId) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (!IsFainted(mon) || param->allowFainted) {
        if (ServerControl_DecrementPP(handler, mon, param->moveIndex, param->amount)) {
            BattleHandler_SetString(handler, &param->string);
            if (ServerEvent_DecrementPP(handler, mon, param->moveIndex)) {
                ServerControl_UseHeldItem(handler, mon);
            }
        }
        return TRUE;
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_CureCondition(BtlServerFlow *handler, const struct BattleHandlerCureConditionParam *param,
                               u16 context) {
    BattleMon *target;
    u8 changed;
    u32 i;
    BattleMon *mon;
    s32 condition;
    s32 code;
    BattleCondition prev;

    target = GetPokeParam(handler->pokeCon, param->monIndex);
    changed = FALSE;
    if (param->popup) {
        ServerDisplay_AbilityPopupAdd(handler, target);
    }
    for (i = 0; i < param->count; i++) {
        mon = GetPokeParam(handler->pokeCon, param->monIds[i]);
        if (!CanPokemonBattle(mon)) {
            continue;
        }
        condition = param->condition;
        while ((code = ConvertConditionCode(mon, &condition)) != 0) {
            changed = TRUE;
            ServerControl_CureCondition(handler, mon, code, &prev);
            if (param->useString == 0) {
                if (Condition_SetCureMessage(code, prev, mon, context, &handler->message)) {
                    BattleHandler_SetString(handler, &handler->message);
                    BattleHandler_StrClear(&handler->message);
                }
            } else {
                BattleHandler_SetString(handler, &param->string);
            }
            if (code == 0x13) {
                ServerControl_CheckItemReaction(handler, mon, 0);
            }
        }
    }
    if (param->popup) {
        ServerDisplay_AbilityPopupRemove(handler, target);
    }
    return changed;
}

// Function name from swan.
u8 BattleHandler_AddCondition(BtlServerFlow *handler, const BattleHandlerAddConditionParam *param) {
    BattleMon *mon;
    u8 showMessage;
    BattleMon *target;

    mon = param->monIndex != 0x1f ? GetPokeParam(handler->pokeCon, param->monIndex) : NULL;
    showMessage = !BattleHandler_StrIsEnabled(&param->string) && !param->noMessage ? TRUE : FALSE;
    if (DoesBattleMonExist(&handler->posPoke, param->targetIndex)) {
        target = GetPokeParam(handler->pokeCon, param->targetIndex);
        if (!IsFainted(target) &&
            !ServerControl_AddConditionCheckFail(handler, target, mon, param->condition, param->value, param->overwrite,
                                                 param->showFail)) {
            if (param->popup) {
                ServerDisplay_AbilityPopupAdd(handler, mon);
            }
            ServerControl_AddCondition(handler, target, mon, param->condition, param->value, showMessage,
                                       param->skipItemReaction, &param->string);
            if (param->popup) {
                ServerDisplay_AbilityPopupRemove(handler, mon);
            }
            return TRUE;
        }
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_StatChange(BtlServerFlow *handler, const struct BattleHandlerStatChangeParam *param, u16 context) {
    BattleMon *popupMon;
    BattleMon *mon;
    BOOL valid;
    u8 result;
    u32 i;
    u32 stat;

    popupMon = GetPokeParam(handler->pokeCon, param->monIndex);
    valid = FALSE;
    result = FALSE;
    for (i = 0; i < param->count; i++) {
        if (DoesBattleMonExist(&handler->posPoke, param->monIds[i])) {
            mon = GetPokeParam(handler->pokeCon, param->monIds[i]);
            if (!IsFainted(mon) && IsStatChangeValid(mon, param->stat, param->change)) {
                valid = TRUE;
                break;
            }
        }
    }
    if (valid && param->popup) {
        ServerDisplay_AbilityPopupAdd(handler, popupMon);
    }
    for (i = 0; i < param->count; i++) {
        if (DoesBattleMonExist(&handler->posPoke, param->monIds[i])) {
            mon = GetPokeParam(handler->pokeCon, param->monIds[i]);
            if (!IsFainted(mon)) {
                stat = param->stat;
                if (ServerControl_StatChangeCore(handler, param->monIndex, mon, stat, param->change, 0x1f, context,
                                                 param->value, param->unk0e, param->flag == 0)) {
                    BattleHandler_SetString(handler, &param->string);
                    result = TRUE;
                }
            }
        }
    }
    if (valid && param->popup) {
        ServerDisplay_AbilityPopupRemove(handler, popupMon);
    }
    return result;
}

// Function name from swan.
u8 BattleHandler_SetStatStage(BtlServerFlow *handler, const BattleHandlerSetStatStageParam *param) {
    BattleMon *mon;

    if (DoesBattleMonExist(&handler->posPoke, param->monIndex)) {
        mon = GetPokeParam(handler->pokeCon, param->monIndex);
        if (!IsFainted(mon)) {
            BattleMon_SetStatStage(mon, 1, param->attack);
            BattleMon_SetStatStage(mon, 2, param->defense);
            BattleMon_SetStatStage(mon, 3, param->spAttack);
            BattleMon_SetStatStage(mon, 4, param->spDefense);
            BattleMon_SetStatStage(mon, 5, param->speed);
            BattleMon_SetStatStage(mon, 6, param->accuracy);
            BattleMon_SetStatStage(mon, 7, param->evasion);
            BtlServerCmdQueue_Put(handler->queue, SC_OP_SET_STAT_STAGES, param->monIndex, param->attack, param->defense,
                                  param->spAttack, param->spDefense, param->speed, param->accuracy, param->evasion);
            return TRUE;
        }
    }
    return FALSE;
}

// Function names from swan.
u8 BattleHandler_RecoverStatStage(BtlServerFlow *handler, BattleHandlerRecoverStatStageParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (!IsFainted(mon)) {
        BtlServerCmdQueue_Put(handler->queue, SC_OP_RECOVER_STAT_STAGES, param->monIndex);
        return StatStageRecover(mon);
    }
    return FALSE;
}

u8 BattleHandler_ResetStatStage(BtlServerFlow *handler, BattleHandlerResetStatStageParam *param) {
    u32 i;
    u8 result;
    BattleMon *mon;

    result = FALSE;
    for (i = 0; i < param->count; i++) {
        mon = GetPokeParam(handler->pokeCon, param->monIndices[i]);
        if (!IsFainted(mon)) {
            BtlServerCmdQueue_Put(handler->queue, SC_OP_RESET_STAT_STAGES, param->monIndices[i]);
            StatStageReset(mon);
            result = TRUE;
        }
    }
    return result;
}

// Function name from swan.
u8 BattleHandler_SetStatus(BtlServerFlow *handler, const BattleHandlerSetStatusParam *param) {
    BattleMon *mon;

    if (DoesBattleMonExist(&handler->posPoke, param->monIndex)) {
        mon = GetPokeParam(handler->pokeCon, param->monIndex);
        if (param->setAttack) {
            SetBaseStatus(mon, 8, param->attack);
            BtlServerCmdQueue_Put(handler->queue, SC_OP_SET_BASE_STAT, param->monIndex, 8, (u8)param->attack);
        }
        if (param->setDefense) {
            SetBaseStatus(mon, 9, param->defense);
            BtlServerCmdQueue_Put(handler->queue, SC_OP_SET_BASE_STAT, param->monIndex, 9, (u8)param->defense);
        }
        if (param->setSpAttack) {
            SetBaseStatus(mon, 10, param->spAttack);
            BtlServerCmdQueue_Put(handler->queue, SC_OP_SET_BASE_STAT, param->monIndex, 10, (u8)param->spAttack);
        }
        if (param->setSpDefense) {
            SetBaseStatus(mon, 11, param->spDefense);
            BtlServerCmdQueue_Put(handler->queue, SC_OP_SET_BASE_STAT, param->monIndex, 11, (u8)param->spDefense);
        }
        if (param->setSpeed) {
            SetBaseStatus(mon, 12, param->speed);
            BtlServerCmdQueue_Put(handler->queue, SC_OP_SET_BASE_STAT, param->monIndex, 12, (u8)param->speed);
        }
        BattleHandler_SetString(handler, &param->string);
        return TRUE;
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_Faint(BtlServerFlow *handler, BattleHandlerFaintParam *param) {
    BattleMon *mon;

    if (DoesBattleMonExist(&handler->posPoke, param->monIndex)) {
        mon = GetPokeParam(handler->pokeCon, param->monIndex);
        if (!IsFainted(mon) || param->force) {
            BattleHandler_SetString(handler, &param->string);
            ServerControl_FaintPokemon(handler, mon);
            return TRUE;
        }
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_ChangeType(BtlServerFlow *handler, BattleHandlerChangeTypeParam *param) {
    BattleMon *mon;

    if (DoesBattleMonExist(&handler->posPoke, param->monIndex)) {
        mon = GetPokeParam(handler->pokeCon, param->monIndex);
        if (!IsFainted(mon) && !IsArceus(GetBattleMonSpecies(mon))) {
            BtlServerCmdQueue_Put(handler->queue, SC_OP_CHANGE_TYPE, param->monIndex, param->type);
            ChangePokeType(mon, param->type);
            if (!param->suppressMessage && PokeTypePair_IsMonotype(param->type)) {
                BtlServerCmdQueue_PutMsg(handler->queue, SC_MSG_SET, 0x380, param->monIndex,
                                         PokeTypePair_GetType1(param->type), SC_MSG_ARG_END);
            }
            return TRUE;
        }
    }
    return FALSE;
}

u8 BattleHandler_Message(BtlServerFlow *handler, BattleHandlerMessageParam *param) {
    BattleMon *mon;

    mon = NULL;
    if (param->monIndex != 0x1f) {
        mon = GetPokeParam(handler->pokeCon, param->monIndex);
    }
    if (param->popup && mon != NULL) {
        ServerDisplay_AbilityPopupAdd(handler, mon);
    }
    BattleHandler_SetString(handler, &param->string);
    if (param->popup && mon != NULL) {
        ServerDisplay_AbilityPopupRemove(handler, mon);
    }
    return TRUE;
}

u8 BattleHandler_SetTurnFlag(BtlServerFlow *handler, BattleHandlerFlagParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (!IsFainted(mon)) {
        BattleMon_SetTurnFlag(mon, param->flag);
        return TRUE;
    }
    return FALSE;
}

u8 BattleHandler_ResetTurnFlag(BtlServerFlow *handler, BattleHandlerFlagParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (!IsFainted(mon)) {
        BattleMon_ClearTurnFlag(mon, param->flag);
        return TRUE;
    }
    return FALSE;
}

u8 BattleHandler_SetContinueFlag(BtlServerFlow *handler, BattleHandlerFlagParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (!IsFainted(mon)) {
        scPut_SetContFlag(handler, mon, param->flag);
        return TRUE;
    }
    return FALSE;
}

u8 BattleHandler_ResetContinueFlag(BtlServerFlow *handler, BattleHandlerFlagParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (!IsFainted(mon)) {
        scPut_ResetContFlag(handler, mon, param->flag);
        return TRUE;
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_AddSideEffect(BtlServerFlow *handler, const BattleHandlerAddSideEffectParam *param) {
    BattleCondition cont = param->cont;

    ServerEvent_CheckSideEffectParam(handler, param->header.monId, param->effect, param->side, &cont);
    if (SideEffectEventAdd(param->side, param->effect, cont)) {
        BattleHandler_SetString(handler, &param->string);
        return TRUE;
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_RemoveSideEffectCore(BtlServerFlow *handler, const BattleHandlerRemoveSideEffectParam *param) {
    u8 removed = FALSE;
    u32 i;

    for (i = 0; i < 14; i++) {
        if (BattleHandler_IsFlagSet(param->effects, i) && ServerDisplay_RemoveSideEffect(param->side, i)) {
            ServerDisplay_SideEffectEnd(handler, i, param->side);
            removed = TRUE;
        }
    }
    return removed;
}

u8 BattleHandler_AddFieldEffect(BtlServerFlow *handler, BattleHandlerAddFieldEffectParam *param) {
    if (ServerControl_FieldEffectCore(handler, param->effect, param->value, param->duration)) {
        BattleHandler_SetString(handler, &param->string);
        return TRUE;
    }
    return FALSE;
}

u8 BattleHandler_RemoveFieldEffect(BtlServerFlow *handler, BattleHandlerRemoveFieldEffectParam *param) {
    if (FieldStatusRemoveEffect(param->effect)) {
        ServerControl_FieldEffectEnd(handler, param->effect);
        return TRUE;
    }
    return FALSE;
}

u8 BattleHandler_ChangeWeather(BtlServerFlow *handler, BattleHandlerChangeWeatherParam *param) {
    BattleMon *mon;
    u32 result;
    u32 state;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    result = FALSE;
    if (param->weather != 0) {
        if (ServerControl_ChangeWeatherCheck(handler, param->weather, param->duration)) {
            if (param->popup) {
                ServerDisplay_AbilityPopupAdd(handler, mon);
            }
            ServerControl_ChangeWeatherCore(handler, param->weather, param->duration);
            BattleHandler_SetString(handler, &param->string);
            result = TRUE;
            if (param->popup) {
                ServerDisplay_AbilityPopupRemove(handler, mon);
            }
        }
    } else if (param->notifyAirLock != 0) {
        if (param->popup) {
            ServerDisplay_AbilityPopupAdd(handler, mon);
        }
        BattleHandler_SetString(handler, &param->string);
        state = PushState(&handler->actionState, 0x3c98);
        ServerEvent_NotifyAirLock(handler);
        PopState(&handler->actionState, state, 0x3c9a);
        result = TRUE;
        if (param->popup) {
            ServerDisplay_AbilityPopupRemove(handler, mon);
        }
    }
    return result;
}

// Function name from swan.
BOOL BattleHandler_SetString(BtlServerFlow *handler, const BattleHandlerString *string) {
    u16 soundEffect;
    u32 flags;

    flags = string->flags;

    if (!((flags << 16) >> 31)) {
        switch ((flags & 0xff)) {
        case 1:
            ServerDisplay_StandardMessage(handler, string->message, ((flags << 17) >> 25), string->args);
            return TRUE;
        case 2:
            ServerDisplay_SetMessage(handler, string->message, ((flags << 17) >> 25), string->args);
            return TRUE;
        }
    } else {
        soundEffect = string->soundEffect;
        switch ((flags & 0xff)) {
        case 1:
            ServerDisplay_StandardMessageEx(handler, string->message, soundEffect, ((flags << 17) >> 25), string->args);
            return TRUE;
        case 2:
            ServerDisplay_SetMessageEx(handler, string->message, soundEffect, ((flags << 17) >> 25), string->args);
            return TRUE;
        }
    }
    return FALSE;
}

// Adds an effect that waits at a position, as Wish and Future Sight do
u8 BattleHandler_AddPosEffect(BtlServerFlow *handler, const BattleHandlerPosEffectParam *param) {
    return PosEventAdd(param->effect, param->pos, param->header.monId, param->args, param->argCount) ? TRUE : FALSE;
}

// Function name from swan.
u8 BattleHandler_AbilityChange(BtlServerFlow *handler, BattleHandlerAbilityChangeParam *param) {
    BattleMon *mon;
    u16 oldAbility;
    u32 state;

    mon = GetPokeParam(handler->pokeCon, param->targetIndex);
    oldAbility = GetBattleMonStat(mon, 0x10);
    if (IsAbilityMultitype(oldAbility)) {
        return FALSE;
    }
    if (IsFainted(mon)) {
        return FALSE;
    }
    if (param->force || param->ability != oldAbility) {
        if (param->popup) {
            BtlServerCmdQueue_Put(handler->queue, SC_ABILITY_POPUP_IN, (u8)param->monIndex);
        }
        BtlServerCmdQueue_Put(handler->queue, SC_CHANGE_ABILITY, (u8)param->targetIndex, param->ability);
        BattleHandler_SetString(handler, &param->string);
        state = PushState(&handler->actionState, 0x3cf6);
        ServerEvent_ChangeAbilityBefore(handler, param->targetIndex, oldAbility, param->ability);
        PopState(&handler->actionState, state, 0x3cf8);
        AbilityEvent_RemoveItem(mon);
        ChangeAbility(mon, param->ability);
        BtlServerCmdQueue_Put(handler->queue, SC_OP_CHANGE_ABILITY, (u8)param->targetIndex, param->ability);
        AbilityEvent_AddItem(mon);
        BtlServerCmdQueue_Put(handler->queue, SC_ABILITY_POPUP_OUT, (u8)param->targetIndex);
        if (oldAbility != param->ability) {
            state = PushState(&handler->actionState, 0x3d06);
            ServerEvent_ChangeAbilityAfter(handler, param->targetIndex);
            PopState(&handler->actionState, state, 0x3d08);
        }
        if (param->popup) {
            BtlServerCmdQueue_Put(handler->queue, SC_ABILITY_POPUP_OUT, (u8)param->monIndex);
        }
        if (!CheckCondition(mon, CONDITION_GASTRO_ACID)) {
            if (oldAbility == 0x67) {
                ServerControl_CheckItemReaction(handler, mon, 0);
            }
            if (oldAbility == 0x7f) {
                ServerControl_UnnerveAction(handler, mon);
            }
        }
        return TRUE;
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_SetItem(BtlServerFlow *handler, BattleHandlerSetItemParam *param) {
    BattleMon *mon;
    u8 result;
    u32 state;

    mon = GetPokeParam(handler->pokeCon, param->targetIndex);
    if (param->monIndex != param->targetIndex) {
        state = PushState(&handler->actionState, 0x3d2c);
        result = ServerEvent_CheckItemSet(handler, mon, param->item);
        PopState(&handler->actionState, state, 0x3d2e);
        if (result) {
            state = PushState(&handler->actionState, 0x3d32);
            ServerEvent_ItemSetFailed(handler, mon);
            PopState(&handler->actionState, state, 0x3d34);
            return FALSE;
        }
    }
    if (param->popup) {
        BtlServerCmdQueue_Put(handler->queue, SC_ABILITY_POPUP_IN, (u8)param->monIndex);
    }
    BattleHandler_SetString(handler, &param->string);
    ServerControl_ChangeHeldItem(handler, mon, param->item, 0);
    if (param->popup) {
        BtlServerCmdQueue_Put(handler->queue, SC_ABILITY_POPUP_OUT, (u8)param->monIndex);
    }
    if (param->clearConsumed) {
        ClearConsumedItem(mon);
        BtlServerCmdQueue_Put(handler->queue, SC_OP_CLEAR_CONSUMED_ITEM, (u8)param->targetIndex);
    }
    if (param->clearOtherConsumed) {
        ClearConsumedItem(GetPokeParam(handler->pokeCon, param->otherIndex));
        BtlServerCmdQueue_Put(handler->queue, SC_OP_CLEAR_CONSUMED_ITEM, (u8)param->otherIndex);
    }
    ServerControl_CheckItemReaction(handler, mon, 0);
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_SwapItem(BtlServerFlow *handler, BattleHandlerSwapItemParam *param) {
    BattleMon *first;
    BattleMon *second;
    u16 firstItem;
    u16 secondItem;
    u8 result;
    u32 state;

    first = GetPokeParam(handler->pokeCon, param->otherIndex);
    second = GetPokeParam(handler->pokeCon, param->monIndex);
    firstItem = GetBattleMonHeldItem(second);
    secondItem = GetBattleMonHeldItem(first);
    state = PushState(&handler->actionState, 0x3d64);
    result = ServerEvent_CheckItemSet(handler, first, firstItem);
    PopState(&handler->actionState, state, 0x3d66);
    if (result) {
        state = PushState(&handler->actionState, 0x3d69);
        ServerEvent_ItemSetFailed(handler, first);
        PopState(&handler->actionState, state, 0x3d6b);
        return FALSE;
    }
    if (param->popup) {
        ServerDisplay_AbilityPopupAdd(handler, second);
    }
    BattleHandler_SetString(handler, &param->firstString);
    BattleHandler_SetString(handler, &param->secondString);
    BattleHandler_SetString(handler, &param->thirdString);
    if (param->popup) {
        ServerDisplay_AbilityPopupRemove(handler, second);
    }
    ServerControl_ChangeHeldItem(handler, second, secondItem, 0);
    ServerControl_ChangeHeldItem(handler, first, firstItem, 0);
    ServerControl_CheckItemReaction(handler, second, 0);
    ServerControl_CheckItemReaction(handler, first, 0);
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_CheckHeldItem(BtlServerFlow *handler, BattleHandlerCheckHeldItemParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    ServerControl_CheckItemReaction(handler, mon, param->reaction);
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_UseHeldItem(BtlServerFlow *handler, BattleHandlerUseHeldItemParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (!IsFainted(mon) || param->allowFainted) {
        if (param->checkFullHp && IsMonFullHP(mon)) {
            return FALSE;
        }
        if (ServerControl_UseHeldItem(handler, mon)) {
            return TRUE;
        }
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_ForceUseItem(BtlServerFlow *handler, BattleHandlerForceUseItemParam *param) {
    BattleMon *mon;
    void *temp;
    u32 reserve;
    u32 state;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (!IsFainted(mon)) {
        temp = ItemEvent_TempAdd(mon, param->item);
        if (temp != NULL) {
            reserve = SCQUE_RESERVE_Pos(handler->queue, SC_USE_HELD_ITEM);
            state = PushState(&handler->actionState, 0x3db8);
            ServerEvent_EquipTempItem(handler, mon, param->monIndex2);
            if (BattleHandler_Result(handler) == 2) {
                BtlServerCmdQueue_PutReserved(handler->queue, reserve, SC_USE_HELD_ITEM, param->monIndex);
            }
            PopState(&handler->actionState, state, 0x3dbf);
            ItemEvent_TempRemove(temp);
            return TRUE;
        }
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_ConsumeItem(BtlServerFlow *handler, BattleHandlerConsumeItemParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (param->skipDisplay == 0) {
        ServerDisplay_UseHeldItem(handler, mon);
        BattleHandler_SetString(handler, &param->string);
    }
    ServerControl_ChangeHeldItem(handler, mon, 0, 1);
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_UpdateMove(BtlServerFlow *handler, const BattleHandlerUpdateMoveParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    BtlServerCmdQueue_Put(handler->queue, SC_OP_UPDATE_MOVE, param->monIndex, param->slot, param->maxPP,
                          param->updateCurrent, param->move);
    Move_UpdateID(mon, param->slot, param->move, param->maxPP, param->updateCurrent);
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_SetCounter(BtlServerFlow *handler, BattleHandlerSetCounterParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    ServerControl_SetMonCounter(handler, mon, param->counter, param->value);
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_DelayMoveDamage(BtlServerFlow *handler, const BattleHandlerDelayMoveDamageParam *param) {
    BattleMon *attacker;
    BattleMon *target;
    BtlFlowMoveParam moveParam;
    BattleMoveEffectState savedEffect;
    BOOL result;

    attacker = GetPokeParam(handler->pokeCon, param->attackerIndex);
    target = GetPokeParam(handler->pokeCon, param->targetIndex);
    ServerEvent_GetMoveParam(handler, param->move, attacker, &moveParam);
    BtlMonSet_Clear(handler->unk860);
    BtlMonSet_Clear(handler->unk868);
    BtlMonSet_Add(handler->unk868, target);
    BtlMonSet_SetNumTargets(handler->unk868, 1);
    BtlMonSet_RemoveFainted(handler->unk868);
    if (BtlMonSet_IsEmptyAfterTargeting(handler->unk868)) {
        ServerDisplay_MoveFailed(handler, attacker, moveParam.move);
        return FALSE;
    }
    ServerControl_RemoveHiddenTargets(handler, &moveParam, attacker, handler->unk868);
    ServerControl_RemoveTypeImmuneTargets(handler, &moveParam, attacker, handler->unk868, handler->unk1F8C);
    ServerControl_RemoveNoEffectTargets(handler, &moveParam, attacker, handler->unk868, handler->unk1F8C);
    ServerControl_RemoveMissedTargets(handler, &moveParam, attacker, handler->unk868);
    if (BtlMonSet_IsEmptyAfterTargeting(handler->unk868)) {
        return FALSE;
    }
    savedEffect = *handler->moveEffect;
    BattleMoveEffectState_Init(handler->moveEffect);
    BattleMoveEffectState_SetTargets(handler->moveEffect, handler, attacker, handler->unk868);
    handler->moveEffect->index = 1;
    ServerControl_DamageMove(handler, &moveParam, attacker, handler->unk868, handler->unk1F8C, TRUE);
    result = handler->moveEffect->enabled;
    *handler->moveEffect = savedEffect;
    return result;
}

// Function name from swan.
u8 BattleHandler_QuitBattle(BtlServerFlow *handler, BattleHandlerQuitBattleParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (ServerControl_EscapeSub(handler, mon, TRUE)) {
        handler->unk14 = 5;
        return TRUE;
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_Switch(BtlServerFlow *handler, BattleHandlerSwitchParam *param) {
    BattleMon *mon;
    u8 pos;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (!ServerControl_CheckMatchup(handler) && !BtlServerFlow_IsInSkyDrop(handler, param->monIndex) &&
        handler->unk14 == 0) {
        BattleHandler_SetString(handler, &param->firstString);
        if (ServerControl_SwitchOut(handler, mon, param->flag)) {
            pos = MonIDToBattlePos(handler->mainModule, handler->pokeCon, param->monIndex);
            RequestChangePokemon(handler->server, pos);
            BattleHandler_SetString(handler, &param->secondString);
            handler->unk14 = 1;
            return TRUE;
        }
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_BatonPass(BtlServerFlow *handler, BattleHandlerBatonPassParam *param) {
    BattleMon *source;
    BattleMon *target;
    u8 pos;

    source = GetPokeParam(handler->pokeCon, param->sourceMonIndex);
    target = GetPokeParam(handler->pokeCon, param->targetMonIndex);
    if (CheckCondition(source, CONDITION_GASTRO_ACID)) {
        ServerEvent_GastroAcidConfirmed(handler, target);
    }
    CopyBatonPassParams(target, source);
    BtlServerCmdQueue_Put(handler->queue, SC_OP_BATON_PASS, param->sourceMonIndex, param->targetMonIndex);
    if (IsSubstituteActive(target)) {
        pos = GetBattlePos(&handler->posPoke, param->targetMonIndex);
        BtlServerCmdQueue_Put(handler->queue, SC_SUBSTITUTE_CREATE, pos);
    }
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_Flinch(BtlServerFlow *handler, BattleHandlerFlinchParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (ServerControl_FlinchCore(handler, mon, param->flag)) {
        return TRUE;
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_Revive(BtlServerFlow *handler, BattleHandlerReviveParam *param) {
    BattleMon *mon;
    u8 pos;
    u8 target;
    u8 slot;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    HPAdd(mon, param->amount);
    BtlServerCmdQueue_Put(handler->queue, SC_OP_ADD_HP, param->monIndex, param->amount);
    handler->unk7A9[param->monIndex] = 0;
    BattleHandler_SetString(handler, &param->string);
    pos = MonIDToBattlePos(handler->mainModule, handler->pokeCon, param->monIndex);
    if (pos != BTL_POS_MAX) {
        target = GetClientIDFromMonID(param->monIndex);
        slot = GetPosIndexInClient(handler->mainModule, pos);
        ServerControl_SwitchInFillSlot(handler, target, slot, slot, TRUE);
        ServerControl_AfterSwitchIn(handler);
    }
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_SetWeight(BtlServerFlow *handler, BattleHandlerSetWeightParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    SetWeight(mon, param->weight);
    BtlServerCmdQueue_Put(handler->queue, SC_OP_SET_WEIGHT, param->monIndex, param->weight);
    BattleHandler_SetString(handler, &param->string);
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_ForceSwitch(BtlServerFlow *handler, const BattleHandlerForceSwitchParam *param) {
    BattleMon *target;
    BOOL failed;

    target = GetPokeParam(handler->pokeCon, param->targetIndex);
    return ServerControl_ForceSwitchCore(handler, GetPokeParam(handler->pokeCon, param->header.monId), target,
                                         param->forced, &failed, param->effect, param->ignoreLevel, &param->string)
               ? TRUE
               : FALSE;
}

// Function names from swan.
u8 BattleHandler_InterruptAction(BtlServerFlow *handler, BattleHandlerInterruptParam *param) {
    if (ActionOrder_InterruptReserve(handler, param->monId)) {
        BattleHandler_SetString(handler, &param->string);
        return TRUE;
    }
    return FALSE;
}

u8 BattleHandler_InterruptMove(BtlServerFlow *handler, BattleHandlerInterruptParam *param) {
    return ActionOrder_InterruptReserveByMove(handler, param->moveId) != 0;
}

u8 BattleHandler_SendLast(BtlServerFlow *handler, BattleHandlerInterruptParam *param) {
    if (ActionOrder_SendToLast(handler, param->monId)) {
        BattleHandler_SetString(handler, &param->string);
        return TRUE;
    }
    return FALSE;
}

u8 BattleHandler_SwapPoke(BtlServerFlow *handler, BattleHandlerSwapPokeParam *param) {
    u8 clientId;
    BattleMon *first;
    BattleMon *second;
    BattleParty *party;
    s16 firstSlot;
    s16 secondSlot;

    if (param->firstMonIndex != param->secondMonIndex) {
        clientId = GetClientIDFromMonID(param->firstMonIndex);
        if (clientId == GetClientIDFromMonID(param->secondMonIndex)) {
            first = GetPokeParam(handler->pokeCon, param->firstMonIndex);
            second = GetPokeParam(handler->pokeCon, param->secondMonIndex);
            if (!IsFainted(first) && !IsFainted(second)) {
                party = GetPartyData(handler->pokeCon, clientId);
                firstSlot = FindPartyMon(party, first);
                secondSlot = FindPartyMon(party, second);
                if (firstSlot >= 0 && secondSlot >= 0) {
                    ServerControl_MoveCore(handler, clientId, firstSlot, secondSlot, 0);
                    BattleHandler_SetString(handler, &param->string);
                    ServerControl_AfterMove(handler, clientId, firstSlot, secondSlot);
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

u8 BattleHandler_Transform(BtlServerFlow *handler, BattleHandlerTransformParam *param) {
    BattleMon *mon;
    BattleMon *target;
    u16 oldAbility;
    u8 monId;
    u8 targetId;
    u32 state;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    target = GetPokeParam(handler->pokeCon, param->targetIndex);
    if (!IsIllusionEnabled(mon) && !IsIllusionEnabled(target) && !IsSemiInvulnMove(mon)) {
        oldAbility = GetBattleMonStat(mon, 0x10);
        if (TransformSet(mon, target)) {
            monId = GetMonID(mon);
            targetId = GetMonID(target);
            if (param->popup) {
                ServerDisplay_AbilityPopupAdd(handler, mon);
            }
            RemoveForceAll(mon);
            AbilityEvent_RemoveItem(mon);
            AbilityEvent_AddItem(mon);
            BtlServerCmdQueue_Put(handler->queue, SC_TRANSFORM, monId, targetId);
            BattleHandler_SetString(handler, &param->string);
            if (param->popup) {
                ServerDisplay_AbilityPopupRemove(handler, mon);
            }
            if (oldAbility != GetBattleMonStat(mon, 0x11)) {
                state = PushState(&handler->actionState, 0x3f37);
                ServerEvent_ChangeAbilityAfter(handler, monId);
                PopState(&handler->actionState, state, 0x3f39);
            }
            return TRUE;
        }
    }
    return FALSE;
}

u8 BattleHandler_IllusionBreak(BtlServerFlow *handler, BattleHandlerIllusionBreakParam *param) {
    BattleMon *mon;

    if (DoesBattleMonExist(&handler->posPoke, param->monIndex)) {
        mon = GetPokeParam(handler->pokeCon, param->monIndex);
        if (IsIllusionEnabled(mon)) {
            IllusionBreak(mon);
            BtlServerCmdQueue_Put(handler->queue, SC_ILLUSION_BREAK, param->monIndex);
            BattleHandler_SetString(handler, &param->string);
            return TRUE;
        }
    }
    return FALSE;
}

u8 BattleHandler_GravityCheck(BtlServerFlow *handler, BattleHandlerGravityCheckParam *param) {
    u8 monIds[6];
    u8 count;
    u8 i;
    BattleMon *mon;
    u32 changed;
    u16 code;
    u8 pos;

    pos = MonIDToBattlePos(handler->mainModule, handler->pokeCon, param->monIndex);
    code = (2 << 10) | pos;
    count = HandlerGetAlivePartyCount(handler, code, monIds);
    for (i = 0; i < count; i++) {
        mon = GetPokeParam(handler->pokeCon, monIds[i]);
        changed = FALSE;
        if (GetAdditionalConditionFlag(mon, 3)) {
            ServerControl_HideTurnCancel(handler, mon, 3);
            changed = TRUE;
        }
        if (ServerEvent_CheckFloating(handler, mon, 1)) {
            changed = TRUE;
        }
        if (CheckCondition(mon, CONDITION_MAGNET_RISE)) {
            ServerControl_CureCondition(handler, mon, CONDITION_MAGNET_RISE, 0);
            changed = TRUE;
        }
        if (CheckCondition(mon, CONDITION_TELEKINESIS)) {
            ServerControl_CureCondition(handler, mon, CONDITION_TELEKINESIS, 0);
            changed = TRUE;
        }
        if (changed) {
            BtlServerCmdQueue_PutMsg(handler->queue, SC_MSG_SET, 0x43b, monIds[i], SC_MSG_ARG_END);
        }
    }
    return TRUE;
}

u8 BattleHandler_HideTurnCancel(BtlServerFlow *handler, BattleHandlerHideTurnParam *param) {
    BattleMon *mon;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (!IsFainted(mon) && ServerControl_HideTurnCancel(handler, mon, param->flag)) {
        BattleHandler_SetString(handler, &param->string);
        return TRUE;
    }
    return FALSE;
}

// Function name from swan.
u8 BattleHandler_EffectAtPos(BtlServerFlow *handler, const BattleHandlerEffectAtPosParam *param) {
    if (param->hideMessageWindow) {
        BtlServerCmdQueue_Put(handler->queue, SC_HIDE_MSG_WINDOW, 0);
    }
    ServerControl_ViewEffect(handler, param->effect, param->pos1, param->pos2, param->reserved, param->reserve);
    BattleHandler_SetString(handler, &param->string);
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_RemoveMessageWindow(BtlServerFlow *handler, BattleHandlerHeader *header) {
    BtlServerCmdQueue_Put(handler->queue, SC_HIDE_MSG_WINDOW, 0);
    return TRUE;
}

// Function name from swan.
u8 BattleHandler_ChangeForm(BtlServerFlow *handler, BattleHandlerChangeFormParam *param) {
    BattleMon *mon;
    u8 currentForm;

    mon = GetPokeParam(handler->pokeCon, param->monIndex);
    if (!IsFainted(mon) && !TransformCheck(mon)) {
        currentForm = GetBattleMonStat(mon, 0x13);
        if (currentForm != param->form) {
            if (param->popup) {
                ServerDisplay_AbilityPopupAdd(handler, mon);
            }
            ChangeForm(mon, param->form);
            BtlServerCmdQueue_Put(handler->queue, SC_CHANGE_FORM, param->monIndex, param->form);
            BattleHandler_SetString(handler, &param->string);
            if (param->popup) {
                ServerDisplay_AbilityPopupRemove(handler, mon);
            }
            return TRUE;
        }
    }
    return FALSE;
}

u8 BattleHandler_SetMoveEffectIndex(BtlServerFlow *handler, BattleHandlerMoveEffectParam *param) {
    handler->moveEffect->index = param->index;
    return TRUE;
}

u8 BattleHandler_SetMoveEffectEnable(BtlServerFlow *handler, BattleHandlerHeader *header) {
    if (!handler->moveEffect->enabled) {
        handler->moveEffect->enabled = 1;
    }
    return TRUE;
}

BOOL func_ov167_021ae30c(BtlServerFlow *flow) {
    return !GetStudioMode(flow->mainModule);
}

u32 func_ov167_021ae320(BtlServerFlow *flow) {
    return flow->unk2130;
}
