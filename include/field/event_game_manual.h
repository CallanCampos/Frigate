#ifndef POKEBW2_FIELD_EVENT_GAME_MANUAL_H
#define POKEBW2_FIELD_EVENT_GAME_MANUAL_H

#include "system/game_event.h"

struct GameManualSubwork {
    GameData *gameData;
    u32 result;
};

struct GameManualEventWork {
    GameSystem *gsys;
    Field *field;
    GameManualSubwork *subwork;
    u16 *result;
};

extern const char data_ov033_0217c610[];
extern const GameProcFunctions gManualProcFunctions;

GameEvent *EventGameManual_Create(GameSystem *gsys, u16 *result);
GameEventReturnCode EventGameManual_Main(GameEvent *event, u32 *state, void *data);
void EventGameManual_SetResult(GameManualEventWork *work);

#endif // POKEBW2_FIELD_EVENT_GAME_MANUAL_H
