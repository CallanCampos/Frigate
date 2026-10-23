#include "types.h"
#include "app/pkwd_book.h"
#include "app/pkwd_record_monitor.h"
#include "app/pkwd_selectscreen.h"
#include "field/pokewood_system.h"
#include "gfl/heap.h"
#include "save/pokewood.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The event that keeps a movie's results and writes the Pokewood block to its slot
typedef struct {
    GameData *gameData;
    PokewoodSystem *sys;
    u32 slot;
    u16 state;
    // Set to 1 when the save ends with 2, and 0 when it ends with 3
    u16 *var;
} PokewoodSaveEvent;

PkwdBookParam *PokewoodSetup_CreateBookParam(HeapID heapId, GameSystem *gsys, u16 *var) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    PokewoodSave *save = PokewoodSave_Get(gameData);
    PkwdBookParam *param = GFL_HeapAllocate(heapId, sizeof(PkwdBookParam), TRUE, "pokewood_setup.c", 75);

    param->playerInfo = playerInfo;
    param->save = save;
    // The script's variable holds the book's signed result
    param->result = (s16 *)var;
    return param;
}

void PokewoodSetup_FreeBookParam(PkwdBookParam *param) {
    GFL_HeapFree(param);
}

PkwdSelectScreenParam *PokewoodSetup_CreateMenuParam(HeapID heapId, GameSystem *gsys, u16 mode, u16 *var) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    PokewoodSave *save = PokewoodSave_Get(gameData);
    SaveControl *saveControl = GameData_GetSaveControl(gameData);
    PkwdSelectScreenParam *param =
        GFL_HeapAllocate(heapId, sizeof(PkwdSelectScreenParam), TRUE, "pokewood_setup.c", 111);

    param->playerInfo = playerInfo;
    param->saveControl = saveControl;
    param->result = var;
    param->save = save;
    param->mode = mode;
    return param;
}

void PokewoodSetup_FreeMenuParam(PkwdSelectScreenParam *param) {
    GFL_HeapFree(param);
}

PkwdRecordMonitorParam *PokewoodSetup_CreateRecordMonitorParam(HeapID heapId, GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    PokewoodSave *save = PokewoodSave_Get(gameData);
    PkwdRecordMonitorParam *param =
        GFL_HeapAllocate(heapId, sizeof(PkwdRecordMonitorParam), TRUE, "pokewood_setup.c", 189);

    param->playerInfo = playerInfo;
    param->save = save;
    return param;
}

void PokewoodSetup_FreeRecordMonitorParam(PkwdRecordMonitorParam *param) {
    GFL_HeapFree(param);
}

static void Pokewood_KeepMovieResults(PokewoodSystem *sys, GameData *gameData, PokewoodSave *save, u32 slot) {
    func_02011124(save, PokewoodSystem_GetMovie(sys));
    func_020111a8(save, slot, 0);
    PokewoodSystem_PushLastSeries(gameData, sys, slot);
    Pokewood_CheckMedals(gameData);
    PokewoodSystem_KeepBestRecord(gameData, sys);
    PokewoodSystem_RecordScore(gameData, sys);
    PokewoodSystem_UpdateCastFame(gameData, sys);
    RecordAddOne(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData)), 0x83);
}

static GameEventReturnCode EventPokewoodSave_Callback(GameEvent *event, u32 *state, void *data) {
    PokewoodSaveEvent *wk = data;
    PokewoodSave *save = PokewoodSave_Get(wk->gameData);

    switch (*state) {
    case 0:
        Pokewood_KeepMovieResults(wk->sys, wk->gameData, save, wk->slot);
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

GameEvent *EventPokewoodSave_Create(GameSystem *gsys, u16 *var) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventPokewoodSave_Callback, sizeof(PokewoodSaveEvent));
    PokewoodSaveEvent *wk = GameEvent_GetData(event);

    wk->gameData = GSYS_GetGameData(gsys);
    wk->sys = *GameData_GetPokewoodSystemPtr(wk->gameData);
    wk->slot = PokewoodSystem_GetSaveSlot(wk->sys);
    wk->state = 0;
    wk->var = var;
    return event;
}
