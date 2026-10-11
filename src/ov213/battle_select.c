#include "battle/battle_select.h"
#include "types.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "battle/btl_setup.h"
#include "battle/regulation.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_sync.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The net command that sends the team picked, after the party list's commands
#define BATTLE_SELECT_NET_CMD_BASE 0x2f00

typedef struct {
    u32 state;
    PokeListParam pokeList;
    PStatusParam status;
    // Frames counted toward the next second of the party list's time limit
    u8 frames;
    u32 lastVBlankCount;
    GameProcManager *procManager;
    BattleSelectParam param;
    HeapID heapId;
} BattleSelectWork;

static void *BattleSelect_GetPartyBuffer(int netId, void *work, int size);
static void BattleSelect_RecvParty(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL BattleSelect_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BattleSelect_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BattleSelect_Main(GameProc *proc, u32 *state, void *param, void *work);
static void BattleSelect_SetupPokeList(BattleSelectParam *param, BattleSelectWork *wk, PokeListParam *pokeList);
static void BattleSelect_SetupStatus(BattleSelectParam *param, BattleSelectWork *wk, PStatusParam *status);

static const NetCommand sBattleSelectNetCommands[] = {
    { BattleSelect_RecvParty, BattleSelect_GetPartyBuffer },
};

const GameProcFunctions BATTLE_SELECT_PROC_FUNCTIONS = {
    BattleSelect_Init,
    BattleSelect_Main,
    BattleSelect_Exit,
};

static void *BattleSelect_GetPartyBuffer(int netId, void *work, int size) {
    BattleSelectWork *wk = work;
    return wk->param.parties[netId];
}

static void BattleSelect_RecvParty(int netId, int size, const void *data, void *work, NetHandle *handle) {
    if (handle != func_02040440()) {
        return;
    }
    if (netId == func_02042a6c(func_02040440())) {
        return;
    }
}

static BOOL BattleSelect_Init(GameProc *proc, u32 *state, void *param, void *work) {
    BattleSelectWork *wk = GFL_ProcInitSubsystem(proc, sizeof(BattleSelectWork), HEAPID_GAMEEVENT);
    sys_memset(wk, 0, sizeof(BattleSelectWork));
    wk->param = *(BattleSelectParam *)param;
    wk->heapId = HEAPID_GAMEEVENT;
    wk->procManager = CreateGameProcManager(wk->heapId);
    func_02040c20(BATTLE_SELECT_NET_CMD_BASE, sBattleSelectNetCommands, NELEMS(sBattleSelectNetCommands), wk);
    BattleSelect_SetupPokeList(param, wk, &wk->pokeList);
    BattleSelect_SetupStatus(param, wk, &wk->status);
    wk->frames = 0;
    wk->lastVBlankCount = OS_GetVBlankCount();
    wk->state = 0;
    return TRUE;
}

static BOOL BattleSelect_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    BattleSelectWork *wk = work;
    func_ov164_021998c8((NetSyncWork *)&wk->pokeList);
    GFL_OvlUnload(OVERLAY_ID(164));
    FreeGameProcManager(wk->procManager);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BattleSelect_Main(GameProc *proc, u32 *state, void *param, void *work) {
    BattleSelectWork *wk = work;
    BattleSelectParam *select = param;
    u8 i;

    if (wk->pokeList.timerEnabled == TRUE && wk->pokeList.timeLeft != 0) {
        u32 vblankCount = OS_GetVBlankCount();
        wk->frames += (u8)(vblankCount - wk->lastVBlankCount);
        wk->lastVBlankCount = vblankCount;
        if (wk->frames > 60) {
            wk->pokeList.timeLeft--;
            wk->frames -= 60;
        }
    }
    func_ov164_021998d4((NetSyncWork *)&wk->pokeList);

    switch (wk->state) {
    case 0:
        GFL_OvlLoad(OVERLAY_ID(165));
        QueueGameProc(wk->procManager, OVERLAY_NONE, &POKELIST_PROC_FUNCTIONS, &wk->pokeList);
        wk->state = 1;
        break;
    case 1:
        if (!GFL_ProcMgrUpdate(wk->procManager)) {
            wk->state = 2;
        }
        break;
    case 2:
        GFL_OvlUnload(OVERLAY_ID(165));
        if (wk->pokeList.result == 1) {
            wk->status.partyIndex = wk->pokeList.index;
            wk->state = 3;
        } else {
            select->result = 0;
            wk->state = 6;
        }
        break;
    case 6:
        for (i = 0; i < 6; i++) {
            if (wk->pokeList.picked[i] != 0) {
                PokeParty_AddPkm(select->unk1C, PokeParty_GetPkm(select->party, wk->pokeList.picked[i] - 1));
            }
        }
        wk->state = 7;
        break;
    case 7:
        if (func_02042c18(func_02040440(), 0xff, BATTLE_SELECT_NET_CMD_BASE, PokeParty_GetSaveDataSize(),
                          select->unk1C, 0, FALSE, TRUE)) {
            func_02040624(func_02040440(), 20, 0x2f);
            wk->state = 8;
        }
        break;
    case 8:
        if (func_02040664(func_02040440(), 20, 0x2f)) {
            return TRUE;
        }
        break;
    case 3:
        GFL_OvlLoad(OVERLAY_PSTATUS);
        QueueGameProc(wk->procManager, OVERLAY_NONE, &PSTATUS_PROC_FUNCTIONS, &wk->status);
        wk->state = 4;
        break;
    case 4:
        if (wk->pokeList.timerEnabled == TRUE && wk->pokeList.timeLeft == 0) {
            wk->status.forceExit = TRUE;
        }
        if (GFL_NetErrCheck()) {
            wk->status.forceExit = TRUE;
        }
        if (!GFL_ProcMgrUpdate(wk->procManager)) {
            wk->state = 5;
        }
        break;
    case 5:
        GFL_OvlUnload(OVERLAY_PSTATUS);
        wk->pokeList.index = wk->status.partyIndex;
        wk->state = 0;
        break;
    }

    // Outside the screens, a lost connection ends the selection
    if (func_02042788() && wk->state != 1 && wk->state != 2 && wk->state != 4 && wk->state != 5) {
        if (DWCRapCommon_CheckError(1, 1)) {
            select->result = 1;
            return TRUE;
        }
    }
    return FALSE;
}

static void BattleSelect_SetupPokeList(BattleSelectParam *param, BattleSelectWork *wk, PokeListParam *pokeList) {
    Regulation *regulation = param->regulation;
    u8 i;

    GameData_GetSaveControl(param->gameData);
    PokeListParam_Setup(pokeList, param->gameData, 0x1a, param->party);
    func_0201f63c(regulation, pokeList->party);
    pokeList->regulation = regulation;
    switch (regulation->unkBA) {
    case 0:
        pokeList->unk48 = 0;
        pokeList->partnerCount = 1;
        break;
    case 1:
        pokeList->unk48 = 1;
        pokeList->partnerCount = 1;
        break;
    case 2:
        pokeList->unk48 = 1;
        pokeList->partnerCount = 1;
        break;
    case 3:
        pokeList->unk48 = 1;
        pokeList->partnerCount = 1;
        break;
    case 4:
        pokeList->unk48 = 2;
        pokeList->partnerCount = 2;
        break;
    }
    pokeList->item = 0;
    pokeList->move = 0;
    pokeList->moveSlot = 0;
    pokeList->index = 0;
    pokeList->result = 0;
    for (i = 0; i < 6; i++) {
        pokeList->picked[i] = 0;
    }
    pokeList->unk64 = 0;
    pokeList->learnIndex = 0;
    pokeList->unk68 = 0;
    pokeList->partners[1].party = param->otherParty;
    pokeList->partners[1].name = param->otherName;
    pokeList->partners[1].gender = param->otherGender;
    pokeList->showPartners = regulation->showPartners;
    if (regulation->timeLimit != 0) {
        pokeList->timerEnabled = TRUE;
        pokeList->timeLeft = regulation->timeLimit;
    } else {
        pokeList->timerEnabled = FALSE;
        pokeList->timeLeft = 0;
    }
    pokeList->battleMsg = 0;
    GFL_OvlLoad(OVERLAY_ID(164));
    func_ov164_021998c0((NetSyncWork *)pokeList);
}

static void BattleSelect_SetupStatus(BattleSelectParam *param, BattleSelectWork *wk, PStatusParam *status) {
    SaveControl *save = GameData_GetSaveControl(param->gameData);
    PokeDexSave *pokedex;

    GetGameDataPlayerInfo(param->gameData);
    pokedex = GameData_GetPokedex(param->gameData);
    status->party = param->party;
    status->gameData = param->gameData;
    status->trainerData = getTrainerDataBlkAddress(save);
    status->dataType = PSTATUS_DATA_PARTY;
    status->mode = PSTATUS_MODE_1;
    status->partyCount = PokeParty_GetPkmCount(param->party);
    status->partyIndex = 0;
    status->page = PSTATUS_PAGE_INFO;
    status->slot = 0;
    status->result = 0;
    status->move = 0;
    status->isNationalDex = PokeDex_IsNationalObtained(pokedex);
    status->fromFieldMenu = FALSE;
    status->forceExit = FALSE;
}
