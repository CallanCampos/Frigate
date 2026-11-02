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

// The steps of the event
enum {
    STEP_CHECK_EGG,
    STEP_SET_PARAM,
    STEP_FADE_OUT,
    STEP_CLOSE_FIELD,
    STEP_START_DEMO,
    STEP_OPEN_FIELD,
    STEP_FADE_IN,
    STEP_END,
};

GameEventReturnCode EventEggDemo_Callback(GameEvent *event, u32 *state, void *data);

GameEventReturnCode EventEggDemo_Callback(GameEvent *event, u32 *state, void *data) {
    EventEggDemo *work = data;
    GameSystem *gsys = work->gsys;
    GameData *gameData = work->gameData;
    Field *field = work->field;

    switch (*state) {
    case STEP_CHECK_EGG:
        if (!PokeParty_GetParam(work->pkm, PKM_PARAM_IS_EGG, NULL) ||
            PokeParty_GetParam(work->pkm, PKM_PARAM_BAD_EGG, NULL) == 1) {
            *state = STEP_END;
        } else {
            *state = STEP_SET_PARAM;
        }
        break;
    case STEP_SET_PARAM:
        work->demo.gameData = gameData;
        work->demo.pkm = work->pkm;
        *state = STEP_FADE_OUT;
        break;
    case STEP_FADE_OUT:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
        *state = STEP_CLOSE_FIELD;
        break;
    case STEP_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        *state = STEP_START_DEMO;
        break;
    case STEP_START_DEMO:
        GSYS_QueueProc(gsys, OVERLAY_EGG_DEMO, &EGG_DEMO_PROC_FUNCTIONS, &work->demo);
        *state = STEP_OPEN_FIELD;
        break;
    case STEP_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        *state = STEP_FADE_IN;
        break;
    case STEP_FADE_IN:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        GameBeacon_BroadcastEggHatched((u16)PokeParty_GetParam(work->pkm, PKM_PARAM_SPECIES, NULL));
        *state = STEP_END;
        break;
    case STEP_END:
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
