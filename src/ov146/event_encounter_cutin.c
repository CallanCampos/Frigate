#include "types.h"
#include "field/encounter_effect.h"
#include "field/event_encounter_cutin.h"
#include "field/field.h"
#include "gfl/fade.h"
#include "system/game_event.h"
#include "system/game_system.h"

struct EncounterCutinWork {
    s32 state;
    u32 selection;
    u32 param;
};

// The create functions of the encounter effects that show a cut-in, by the cut-in they show
GameEvent *EventEncountEffectCutin_Create01(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 1, param);
}

GameEvent *EventEncountEffectCutin_Create02(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 2, param);
}

GameEvent *EventEncountEffectCutin_Create03(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 3, param);
}

GameEvent *EventEncountEffectCutin_Create04(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 4, param);
}

GameEvent *EventEncountEffectCutin_Create05(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 5, param);
}

GameEvent *EventEncountEffectCutin_Create06(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 6, param);
}

GameEvent *EventEncountEffectCutin_Create07(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 7, param);
}

GameEvent *EventEncountEffectCutin_Create08(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 8, param);
}

GameEvent *EventEncountEffectCutin_Create09(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 9, param);
}

GameEvent *EventEncountEffectCutin_Create10(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 10, param);
}

GameEvent *EventEncountEffectCutin_Create11(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 11, param);
}

GameEvent *EventEncountEffectCutin_Create12(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 12, param);
}

GameEvent *EventEncountEffectCutin_Create00(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 0, param);
}

GameEvent *EventEncountEffectCutin_Create15(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 15, param);
}

GameEvent *EventEncountEffectCutin_Create14(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 14, param);
}

GameEvent *EventEncountEffectCutin_Create16(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 16, param);
}

GameEvent *EventEncountEffectCutin_Create13(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 13, param);
}

GameEvent *EventEncountEffectCutin_Create17(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 17, param);
}

GameEvent *EventEncountEffectCutin_Create18(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 18, param);
}

GameEvent *EventEncountEffectCutin_Create19(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 19, param);
}

GameEvent *EventEncountEffectCutin_Create20(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 20, param);
}

GameEvent *EventEncountEffectCutin_Create21(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 21, param);
}

GameEvent *EventEncountEffectCutin_Create22(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 22, param);
}

GameEvent *EventEncountEffectCutin_Create23(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 23, param);
}

GameEvent *EventEncountEffectCutin_Create24(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 24, param);
}

GameEvent *EventEncountEffectCutin_Create25(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 25, param);
}

GameEvent *EventEncountEffectCutin_Create26(GameSystem *gsys, Field *field, u32 param) {
    return EventEncountEffectCutin_Create(gsys, field, 26, param);
}

GameEvent *EventEncountEffectCutin_Create(GameSystem *gsys, Field *field, u32 selection, u32 param) {
    EncounterCutinWork *work;

    work = EncEff_AllocWorkArea(Field_GetEncEff(GSYS_GetField(gsys)), 0xc, 0x50);
    work->state = 0;
    work->selection = selection;
    work->param = param;
    return GameEvent_Create(gsys, NULL, EventEncountEffectCutin_Callback, 0);
}

GameEventReturnCode EventEncountEffectCutin_Callback(GameEvent *event, u32 *state, void *data) {
    GameSystem *gsys;
    Field *field;
    EncounterCutinWork *work;
    GameEvent *next;

    gsys = GameEvent_GetGameSystem(event);
    field = GSYS_GetField(gsys);
    work = EncEff_GetWorkArea(Field_GetEncEff(field));

    switch (*state) {
    case 0:
        GFL_FadeSet(4, 0, 0x10, 0);
        (*state)++;
        // Fall through to check the fade in the same frame.
    case 1:
        if (!GFL_FadeIsRunning()) {
            GFL_FadeSet(4, 0x10, 0, 0);
            (*state)++;
        }
        break;
    case 2:
        if (!GFL_FadeIsRunning()) {
            work->state++;
            if (work->state < 2) {
                GFL_FadeSet(4, 0, 0x10, 0);
                *state = 1;
            } else {
                next = EventFieldEffect_CreateBattleCutin(gsys, Field_Get3DCi(field), work->selection, work->param);
                if (next != NULL) {
                    GameEvent_ChainNext(event, next);
                } else {
                    next = GameEvent_Create(gsys, event, EventEncountEffectCutinFallback_Callback, 0);
                    GameEvent_ChainNext(event, next);
                }
                (*state)++;
            }
        }
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventEncountEffectCutinFallback_Callback(GameEvent *event, u32 *state, void *data) {
    GSYS_GetField(GameEvent_GetGameSystem(event));
    switch (*state) {
    case 0:
        GFL_FadeSet(3, 0, 0x10, 3);
        (*state)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}
