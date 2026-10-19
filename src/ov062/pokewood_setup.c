#include "types.h"
#include "field/pokewood_system.h"
#include "gfl/heap.h"
#include "save/pokewood.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

typedef struct {
    PlayerInfo *playerInfo;
    PokewoodSave *save;
    u16 *var;
} Ov062Param6680;

typedef struct {
    PlayerInfo *playerInfo;
    SaveControl *saveControl;
    u16 *var;
    PokewoodSave *save;
    u32 unk10;
} Ov062Param66c0;

typedef struct {
    PlayerInfo *playerInfo;
    PokewoodSave *save;
} Ov062Param6714;

// The event that keeps a movie's results and writes the Pokewood block to its slot
typedef struct {
    GameData *gameData;
    PokewoodSystem *sys;
    u32 slot;
    u16 state;
    // Set to 1 when the save ends with 2, and 0 when it ends with 3
    u16 *var;
} PokewoodSaveEvent;

void *func_ov062_021e6680(HeapID heapId, GameSystem *gsys, u16 *var) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    PokewoodSave *save = PokewoodSave_Get(gameData);
    Ov062Param6680 *param = GFL_HeapAllocate(heapId, sizeof(Ov062Param6680), TRUE, "pokewood_setup.c", 75);

    param->playerInfo = playerInfo;
    param->save = save;
    param->var = var;
    return param;
}

void func_ov062_021e66b8(void *param) {
    GFL_HeapFree(param);
}

void *func_ov062_021e66c0(HeapID heapId, GameSystem *gsys, u16 a2, u16 *var) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    PokewoodSave *save = PokewoodSave_Get(gameData);
    SaveControl *saveControl = GameData_GetSaveControl(gameData);
    Ov062Param66c0 *param = GFL_HeapAllocate(heapId, sizeof(Ov062Param66c0), TRUE, "pokewood_setup.c", 111);

    param->playerInfo = playerInfo;
    param->saveControl = saveControl;
    param->var = var;
    param->save = save;
    param->unk10 = a2;
    return param;
}

void func_ov062_021e670c(void *param) {
    GFL_HeapFree(param);
}

void *func_ov062_021e6714(HeapID heapId, GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    PokewoodSave *save = PokewoodSave_Get(gameData);
    Ov062Param6714 *param = GFL_HeapAllocate(heapId, sizeof(Ov062Param6714), TRUE, "pokewood_setup.c", 189);

    param->playerInfo = playerInfo;
    param->save = save;
    return param;
}

void func_ov062_021e674c(void *param) {
    GFL_HeapFree(param);
}

static void func_ov062_021e6754(PokewoodSystem *sys, GameData *gameData, PokewoodSave *save, u32 slot) {
    func_02011124(save, PokewoodSystem_GetMovie(sys));
    func_020111a8(save, slot, 0);
    func_ov062_021e663c(gameData, sys, slot);
    Pokewood_CheckMedals(gameData);
    PokewoodSystem_KeepBestRecord(gameData, sys);
    func_ov062_021e6584(gameData, sys);
    PokewoodSystem_UpdateCastFame(gameData, sys);
    RecordAddOne(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData)), 0x83);
}

static GameEventReturnCode func_ov062_021e67b0(GameEvent *event, u32 *state, void *data) {
    PokewoodSaveEvent *wk = data;
    PokewoodSave *save = PokewoodSave_Get(wk->gameData);

    switch (*state) {
    case 0:
        func_ov062_021e6754(wk->sys, wk->gameData, save, wk->slot);
        allocatePokewoodBlk(HEAPID_FIELDMAP);
        setPokewoodBlk(PokewoodSystem_GetPokewoodBlk(wk->sys));
        *state = 1;
    case 1:
        switch (Pokewood_SaveDownloadedMovie(wk->gameData, HEAPID_FIELDMAP, wk->slot, &wk->state)) {
        case 3:
            if (wk->var != NULL) {
                *wk->var = 0;
            }
            *state = 2;
            break;
        case 2:
            if (wk->var != NULL) {
                *wk->var = 1;
            }
            *state = 2;
            break;
        }
        break;
    case 2:
        freeAndClearPokewoodBlk();
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *func_ov062_021e682c(GameSystem *gsys, u16 *var) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov062_021e67b0, sizeof(PokewoodSaveEvent));
    PokewoodSaveEvent *wk = GameEvent_GetData(event);

    wk->gameData = GSYS_GetGameData(gsys);
    wk->sys = *GameData_GetPokewoodSystemPtr(wk->gameData);
    wk->slot = func_ov062_021e62c8(wk->sys);
    wk->state = 0;
    wk->var = var;
    return event;
}
