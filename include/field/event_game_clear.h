#ifndef POKEBW2_FIELD_EVENT_GAME_CLEAR_H
#define POKEBW2_FIELD_EVENT_GAME_CLEAR_H

#include "types.h"
#include "app/unova_link.h"
#include "demo/staff_roll.h"
#include "gfl/proc.h"
#include "struct_decls.h"
#include "system/game_event.h"

struct GameClearWork {
    GameSystem *gameSystem;
    GameData *gameData;
    u32 unk08;
    u8 unk0C[4];
    PlayerInfo *unk10;
    // The parameters of the procs that the event runs
    struct {
        GameSystem *gsys;
        u32 param;
    } ov295Param;
    struct {
        PokeParty *party;
        PlayerInfo *playerInfo;
        PlayTime *playTime;
    } ov265Param;
    StaffRollParam staffRollParam;
    struct {
        u32 unk00;
    } ov267Param;
    UnovaLinkParam unovaLinkParam;
    // The state that runs, from the sequence that SetGameClearStatusSequence builds
    u32 current;
    u32 states[31];
    s32 counter;
};

// The procs it runs, in overlays 265, 266 (demo/staff_roll.h), 267 (demo/the_end.h) and 295
extern const GameProcFunctions data_ov265_0219b7d0;
extern const GameProcFunctions GAME_CLEAR_SAVE_PROC_FUNCTIONS;

GameEventReturnCode EventGameClear_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventGameClear_Create(GameSystem *gsys, u32 param);
void SetGameClearGameData(GameClearWork *work);
void func_ov012_0215a50c(GameClearWork *work);
void SetGameClearStatusSequence(GameClearWork *work);
void EventGameClear_GiveMonotypeMedals(GameClearWork *work);
u32 EventGameClear_Get3DDemoID(void);
void EventGameClear_NextState(GameClearWork *work, u32 *state);
void func_ov012_0215a670(GameClearWork *work);

// Wrappers that load overlay 35 for what it creates
GameEvent *CallCreateGameEntryPointEvent(GameSystem *gsys, GameSystemProcData *procData);
GameEvent *EventMapChangeBlackout_CreateExternal(GameSystem *gsys);
void LoadFieldGlueOverlay(void);
void UnloadFieldGlueOverlay(void);

#endif // POKEBW2_FIELD_EVENT_GAME_CLEAR_H
