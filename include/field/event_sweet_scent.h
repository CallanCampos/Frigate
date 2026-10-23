#ifndef POKEBW2_FIELD_EVENT_SWEET_SCENT_H
#define POKEBW2_FIELD_EVENT_SWEET_SCENT_H

#include "system/game_event.h"

struct SweetScentEventData {
    u8 partySlot;
    u8 padding[3];
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    FieldPlayer *player;
};

struct SweetScentScreenWork {
    u8 bgId;
    u8 alpha;
    u8 step;
    u8 timer;
    FieldDispControl *displayControl;
};

struct SweetScentPalette {
    u16 first;
    u16 second;
};

GameEvent *EventSweetScent_Create(Field *field, GameSystem *gsys);
GameEvent *func_ov033_021785d4(GameSystem *gsys, Field *field, u8 partySlot);
GameEventReturnCode EventSweetScent_Callback(GameEvent *event, u32 *state, void *data);
BOOL EventSweetScent_CheckWeatherClear(SweetScentEventData *work);
void func_ov033_02178748(GameEvent *event, GameSystem *gsys, Field *field);
GameEventReturnCode EventSweetScent_FadeMain(GameEvent *event, u32 *state, void *data);
void EventSweetScent_FadeSetup(SweetScentScreenWork *work);
void EventSweetScent_FadeCleanup(SweetScentScreenWork *work);

#endif // POKEBW2_FIELD_EVENT_SWEET_SCENT_H
