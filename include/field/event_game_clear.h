#ifndef POKEBW2_FIELD_EVENT_GAME_CLEAR_H
#define POKEBW2_FIELD_EVENT_GAME_CLEAR_H

#include "types.h"
#include "app/unova_link.h"
#include "demo/staff_roll.h"
#include "field/game_clear_save.h"
#include "gfl/proc.h"
#include "struct_decls.h"
#include "system/game_event.h"

// The game clear event's states, which SetGameClearStatusSequence puts in the order they run in: the Hall of Fame,
// the credits, the save and the end, then the two warps of the ending with their scripts and the music
enum {
    GAME_CLEAR_STATE_START,
    GAME_CLEAR_STATE_FADE_OUT,
    GAME_CLEAR_STATE_END_COMM,
    GAME_CLEAR_STATE_WAIT_COMM,
    GAME_CLEAR_STATE_CLOSE_FIELD,
    GAME_CLEAR_STATE_HALL_OF_FAME,
    GAME_CLEAR_STATE_WAIT_HALL_OF_FAME,
    GAME_CLEAR_STATE_STAFF_ROLL,
    GAME_CLEAR_STATE_WAIT_STAFF_ROLL,
    GAME_CLEAR_STATE_DEMO,
    GAME_CLEAR_STATE_POST_HOF_SETUP,
    GAME_CLEAR_STATE_UNOVA_LINK,
    GAME_CLEAR_STATE_SAVE,
    GAME_CLEAR_STATE_WAIT,
    GAME_CLEAR_STATE_THE_END,
    GAME_CLEAR_STATE_WARP_ENDING_A,
    GAME_CLEAR_STATE_WARP_ENDING_B,
    GAME_CLEAR_STATE_SCRIPT_ENDING_A,
    GAME_CLEAR_STATE_SCRIPT_ENDING_B,
    GAME_CLEAR_STATE_OPEN_FIELD,
    GAME_CLEAR_STATE_FADE_IN,
    GAME_CLEAR_STATE_STOP_BGM,
    GAME_CLEAR_STATE_PLAY_BGM,
    GAME_CLEAR_STATE_FADE_OUT_BGM,
    GAME_CLEAR_STATE_WAIT_BGM,
    GAME_CLEAR_STATE_END,
    GAME_CLEAR_STATE_IDLE,
};

struct GameClearWork {
    GameSystem *gameSystem;
    GameData *gameData;
    // Whether the Champion was beaten before, as GameClearSaveParam has it
    u32 repeat;
    u8 unk0C[4];
    PlayerInfo *playerInfo;
    // The parameters of the procs that the event runs
    GameClearSaveParam ov295Param;
    struct {
        PokeParty *party;
        PlayerInfo *playerInfo;
        PlayTime *playTime;
    } ov265Param;
    StaffRollParam staffRollParam;
    struct {
        // The kanji setting, which the end screen doesn't read
        u32 kanji;
    } ov267Param;
    UnovaLinkParam unovaLinkParam;
    // The state that runs, from the sequence that SetGameClearStatusSequence builds
    u32 current;
    u32 states[31];
    s32 counter;
};

// The procs it runs, in overlays 265, 266 (demo/staff_roll.h), 267 (demo/the_end.h) and 295
extern const GameProcFunctions HALL_OF_FAME_DEMO_PROC_FUNCTIONS;
extern const GameProcFunctions GAME_CLEAR_SAVE_PROC_FUNCTIONS;

GameEventReturnCode EventGameClear_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventGameClear_Create(GameSystem *gsys, u32 repeat);
void SetGameClearGameData(GameClearWork *work);
void SetGameClearStaffRollParam(GameClearWork *work);
void SetGameClearStatusSequence(GameClearWork *work);
void EventGameClear_GiveMonotypeMedals(GameClearWork *work);
u32 EventGameClear_Get3DDemoID(void);
void EventGameClear_NextState(GameClearWork *work, u32 *state);
void EventGameClear_SetHallOfFameMinutes(GameClearWork *work);

// Wrappers that load overlay 35 for what it creates
GameEvent *CallCreateGameEntryPointEvent(GameSystem *gsys, GameSystemProcData *procData);
GameEvent *EventMapChangeBlackout_CreateExternal(GameSystem *gsys);
void LoadFieldGlueOverlay(void);
void UnloadFieldGlueOverlay(void);

#endif // POKEBW2_FIELD_EVENT_GAME_CLEAR_H
