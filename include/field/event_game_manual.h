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

GameEvent *func_ov033_02179dd4(GameSystem *gsys, u16 *result);
GameEventReturnCode func_ov033_02179e28(GameEvent *event, u32 *state, void *data);
void func_ov033_02179e80(GameManualEventWork *work);

#endif // POKEBW2_FIELD_EVENT_GAME_MANUAL_H
