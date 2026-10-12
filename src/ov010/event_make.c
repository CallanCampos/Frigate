#include "types.h"
#include "battle/battle_proc.h"
#include "battle/battle_rec_tool.h"
#include "battle/btl_net.h"
#include "battle/btl_setup.h"
#include "field/bsubway_scr.h"
#include "field/event_make.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "save/save_control.h"
#include "system/game_event.h"
#include "system/game_system.h"

#define HEAPID_BATTLE_PROC 0x76

const GameProcFunctions data_ov010_0215039c = { func_ov010_0214ff00, func_ov010_0214ff58, func_ov010_0214ff28 };

BOOL func_ov010_0214ff00(GameProc *proc, u32 *state, void *param, void *work) {
    BattleProcWork *procWork;

    GFL_HeapCreateChild(1, HEAPID_BATTLE_PROC, 0x4000);
    procWork = GFL_ProcInitSubsystem(proc, sizeof(BattleProcWork), HEAPID_BATTLE_PROC);
    procWork->manager = CreateGameProcManager(HEAPID_BATTLE_PROC);
    return TRUE;
}

BOOL func_ov010_0214ff28(GameProc *proc, u32 *state, void *param, void *work) {
    BattleParam *battle = param;
    BattleProcWork *procWork = work;

    if (battle->unk14 == 1 && GFL_NetErrCheck()) {
        GFL_NetErrMarkShown();
    }
    FreeGameProcManager(procWork->manager);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_BATTLE_PROC);
    return TRUE;
}

BOOL func_ov010_0214ff58(GameProc *proc, u32 *state, void *param, void *work) {
    BattleParam *battle = param;
    BattleProcWork *procWork = work;
    BOOL running = GFL_ProcMgrUpdate(procWork->manager);
    s32 last;
    u32 first;
    u32 second;
    s32 i;
    s32 j;
    u32 client;
    u32 source;
    BOOL saved;

    if (running != TRUE && GFL_NetErrCheck() && (s32)*state >= 0 && (s32)*state < 13) {
        if (procWork->commandsSet == TRUE) {
            func_02040c64(0x100);
        }
        if (procWork->ov167Loaded == TRUE) {
            GFL_OvlUnload(OVERLAY_ID(167));
        }
        if (procWork->vsPlayerLoaded == TRUE) {
            freeVSPlayerBlkClearPtr();
        }
        return TRUE;
    }
    switch (*state) {
    case 0:
        if (battle->setup->fieldSituation.unk1a == 0) {
            battle->players->unk44 = 1;
        } else {
            battle->players->unk44 = 3;
        }
        QueueGameProc(procWork->manager, OVERLAY_ID(305), &CommBtlDemoProc, battle->players);
        *state = 1;
        break;
    case 1:
        if (running == TRUE) {
            break;
        }
        if (battle->setup->fieldSituation.unk1a == 0) {
            *state = 6;
        } else {
            *state = 2;
        }
        break;
    case 2:
        func_02042d04(func_02040440(), 0xc9);
        *state = 3;
        break;
    case 3:
        if (func_02042d0c(func_02040440(), 0xc9)) {
            *state = 4;
        }
        break;
    case 4:
        if (battle->setup->fieldSituation.unk19 <= 1) {
            procWork->ov171Param.flipped = 0;
            procWork->ov171Param.party0 = battle->players->players[0].party;
            procWork->ov171Param.party1 = battle->players->players[1].party;
        } else {
            procWork->ov171Param.flipped = 1;
            procWork->ov171Param.party0 = battle->players->players[1].party;
            procWork->ov171Param.party1 = battle->players->players[0].party;
        }
        QueueGameProc(procWork->manager, OVERLAY_ID(171), &VS_MULTI_LIST_PROC_FUNCTIONS, &procWork->ov171Param);
        *state = 5;
        break;
    case 5:
        if (running == TRUE) {
            break;
        }
        *state = 6;
        break;
    case 6:
        procWork->ov167Loaded = TRUE;
        GFL_OvlLoad(OVERLAY_ID(167));
        procWork->commandsSet = TRUE;
        func_02040c20(0x100, data_ov167_021d7448, 9, NULL);
        func_02042d04(func_02040440(), 0xc8);
        *state = 7;
        break;
    case 7:
        if (func_02042d0c(func_02040440(), 0xc8)) {
            *state = 8;
        }
        break;
    case 8:
        procWork->vsPlayerLoaded = TRUE;
        func_0200bb24(HEAPID_BATTLE_PROC);
        QueueGameProc(procWork->manager, -1, &data_ov167_021d6ce0, battle->setup);
        *state = 9;
        break;
    case 9:
        if (running == TRUE) {
            break;
        }
        *state = 10;
        break;
    case 10:
        *state = 11;
        break;
    case 11:
        func_02042d04(func_02040440(), 0xca);
        *state = 12;
        break;
    case 12:
        if (!func_02042d0c(func_02040440(), 0xca)) {
            break;
        }
        func_02040c64(0x100);
        procWork->commandsSet = FALSE;
        GFL_OvlUnload(OVERLAY_ID(167));
        procWork->ov167Loaded = FALSE;
        func_0200c1f0();
        BattleRecTool_StoreSetup(battle->setup);
        func_0200c200();
        *state = 13;
        break;
    case 13:
        if (battle->setup->fieldSituation.unk1a == 0) {
            battle->players->unk44 = 2;
        } else {
            battle->players->unk44 = 4;
        }
        switch (battle->setup->unkA8) {
        case 1:
        case 4:
            battle->players->result = 0;
            break;
        case 0:
        case 3:
            battle->players->result = 1;
            break;
        case 2:
            battle->players->result = 2;
            break;
        case 6:
            break;
        default:
            battle->players->result = 2;
            break;
        }
        if (battle->setup->fieldSituation.unk1a == 0) {
            last = 1;
            first = 0;
            second = 1;
        } else {
            last = 3;
            first = 0;
            second = 2;
        }
        client = battle->setup->fieldSituation.unk19;
        for (i = 0; i <= last; i++) {
            if ((i & 1) == (client & 1)) {
                if (client & 1) {
                    source = (i & 2) + 1;
                } else {
                    source = i & 2;
                }
                for (j = 0; j < 6; j++) {
                    battle->players->players[first + (i >> 1)].order[j] = battle->setup->unkE7[source][j];
                }
            } else {
                if (client & 1) {
                    source = i & 2;
                } else {
                    source = (i & 2) + 1;
                }
                for (j = 0; j < 6; j++) {
                    battle->players->players[second + (i >> 1)].order[j] = battle->setup->unkE7[source][j];
                }
            }
        }
        if (battle->setup->unkA8 == 6) {
            *state = 16;
            break;
        }
        QueueGameProc(procWork->manager, OVERLAY_ID(305), &CommBtlDemoProc, battle->players);
        *state = 14;
        break;
    case 14:
        if (running == TRUE) {
            break;
        }
        *state = 15;
        break;
    case 15:
        if (battle->setup->unkB0 == NULL) {
            saved = FALSE;
        } else {
            saved = func_0200c1d0(battle->setup->unkAD) ? TRUE : FALSE;
        }
        battle->ov306Param.gameData = battle->gameData;
        battle->ov306Param.unk4 = saved;
        battle->ov306Param.unk8 = 1;
        battle->ov306Param.unkC = 1;
        battle->ov306Param.unk10 = battle->rule;
        battle->ov306Param.unk14 = battle->unk10;
        QueueGameProc(procWork->manager, OVERLAY_ID(306), &BtlRecSelProcFunctions, &battle->ov306Param);
        *state = 16;
        break;
    case 16:
        if (running == TRUE) {
            break;
        }
        freeVSPlayerBlkClearPtr();
        procWork->vsPlayerLoaded = FALSE;
        *state = 17;
        break;
    case 17:
        return TRUE;
    }
    return FALSE;
}

GameEvent *eventMakeFunc(GameSystem *gsys, void *data) {
    const EventMakeArgs *args = data;

    return func_ov010_02150310(gsys, args->setup, args->players, args->unk08);
}

GameEvent *func_ov010_02150310(GameSystem *gsys, BtlSetup *setup, BattlePlayers *players, u32 a3) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov010_0215033c, sizeof(EventMakeWork));
    EventMakeWork *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->setup = setup;
    work->players = players;
    work->unk0C = a3;
    return event;
}

GameEventReturnCode func_ov010_0215033c(GameEvent *event, u32 *state, void *data) {
    EventMakeWork *work = data;
    GameSystem *gsys = work->gsys;
    BattleParam *param;

    switch (*state) {
    case 0:
        param = &work->param;
        work->param.gameData = GSYS_GetGameData(gsys);
        param->setup = work->setup;
        param->players = work->players;
        param->rule = work->players->rule;
        param->unk10 = work->players->unk4C;
        param->unk14 = work->unk0C;
        GSYS_QueueProc(gsys, -1, &data_ov010_0215039c, param);
        (*state)++;
        break;
    case 1:
        if (!GSYS_GetProcMgrState(gsys)) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}
