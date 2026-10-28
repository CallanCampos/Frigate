#include "types.h"
#include "constants/pokemon.h"
#include "demo/egg_demo.h"
#include "field/event_egg_demo.h"
#include "field/field_event.h"
#include "field/game_beacon_set.h"
#include "pml/poke_party.h"
#include "system/game_event.h"
#include "system/game_system.h"

struct EventEggDemo {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    PartyPkm *pkm;
    EggDemoParam demo;
};

GameEventReturnCode EventEggDemo_Callback(GameEvent *event, u32 *state, void *data);

GameEventReturnCode EventEggDemo_Callback(GameEvent *event, u32 *state, void *data) {
    EventEggDemo *work = data;
    GameSystem *gsys = work->gsys;
    GameData *gameData = work->gameData;
    Field *field = work->field;

    switch (*state) {
    case 0:
        if (!PokeParty_GetParam(work->pkm, PKM_PARAM_IS_EGG, NULL) || PokeParty_GetParam(work->pkm, 3, NULL) == 1) {
            *state = 7;
        } else {
            *state = 1;
        }
        break;
    case 1:
        work->demo.gameData = gameData;
        work->demo.pkm = work->pkm;
        *state = 2;
        break;
    case 2:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
        *state = 3;
        break;
    case 3:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        *state = 4;
        break;
    case 4:
        GSYS_QueueProc(gsys, OVERLAY_EGG_DEMO, &EGG_DEMO_PROC_FUNCTIONS, &work->demo);
        *state = 5;
        break;
    case 5:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        *state = 6;
        break;
    case 6:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        GameBeacon_BroadcastEggHatched((u16)PokeParty_GetParam(work->pkm, PKM_PARAM_SPECIES, NULL));
        *state = 7;
        break;
    case 7:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventEggDemo_Create(GameSystem *gsys, PartyPkm *pkm) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventEggDemo_Callback, sizeof(EventEggDemo));
    EventEggDemo *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->gameData = GSYS_GetGameData(gsys);
    work->pkm = pkm;
    return event;
}
