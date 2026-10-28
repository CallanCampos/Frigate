// pleasure_boat.c: the Royal Unova's cruise
#include "types.h"
#include "constants/sound.h"
#include "field/field_script_event.h"
#include "field/pleasure_boat.h"
#include "field/pleasure_boat_rooms.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"

#define PLEASURE_BOAT_PERIOD 1350
// The cruise ends once this many periods have passed
#define PLEASURE_BOAT_PERIOD_COUNT 4

// What PleasureBoat_GetTimeEvent finds
enum {
    PLEASURE_BOAT_TIME_NONE,
    PLEASURE_BOAT_TIME_PERIOD_END,
    PLEASURE_BOAT_TIME_CRUISE_END,
};

static BOOL PleasureBoat_GetTimeEvent(PleasureBoat *boat);

PleasureBoat *PleasureBoat_Create(BOOL altTrainers) {
    PleasureBoat *boat = GFL_HeapAllocate(HEAPID_TRIAL_HOUSE, sizeof(PleasureBoat), TRUE, "pleasure_boat.c", 49);

    boat->periodEnded = FALSE;
    boat->clock = 0;
    boat->period = 0;
    boat->infoCounts[0] = 0;
    boat->infoCounts[1] = 0;
    boat->infoCounts[2] = 0;
    boat->infoCounts[3] = 0;
    boat->clockStopped = FALSE;
    GFL_OvlLoad(OVERLAY_ID(89));
    PleasureBoat_SetUpRooms(boat, altTrainers);
    GFL_OvlUnload(OVERLAY_ID(89));
    return boat;
}

void PleasureBoat_AckEvent(PleasureBoat *boat) {
    if (boat != NULL && !boat->clockStopped && boat->periodEnded) {
        GFL_SndSEPlay(SEQ_SE_FLD_78);
        boat->period++;
        boat->periodEnded = FALSE;
    }
}

GameEvent *PleasureBoat_Update(GameSystem *gsys, PleasureBoat *boat) {
    GameEvent *event;
    BOOL created;

    if (boat == NULL) {
        return NULL;
    }
    if (boat->clockStopped) {
        return NULL;
    }
    boat->clock++;
    switch (PleasureBoat_GetTimeEvent(boat)) {
    case PLEASURE_BOAT_TIME_PERIOD_END:
        boat->periodEnded = TRUE;
        created = FALSE;
        break;
    case PLEASURE_BOAT_TIME_CRUISE_END:
        event = EventScriptCall_Create(gsys, 1, NULL, 0x8015);
        created = TRUE;
        break;
    default:
        created = FALSE;
        break;
    }
    if (!created) {
        event = NULL;
    }
    return event;
}

void PleasureBoat_AdvanceClock(PleasureBoat *boat, u32 steps, BOOL stopBefore) {
    s32 offset = boat->clock % PLEASURE_BOAT_PERIOD;
    s32 time = steps * 30;

    offset += time;
    if (offset >= PLEASURE_BOAT_PERIOD) {
        if (stopBefore) {
            boat->clock = (boat->period + 1) * PLEASURE_BOAT_PERIOD - 1;
        } else {
            boat->clock = (boat->period + 1) * PLEASURE_BOAT_PERIOD;
            boat->period++;
        }
    } else {
        boat->clock += (s16)time;
    }
}

u32 PleasureBoat_GetInfo(PleasureBoat *boat, u32 info) {
    u32 value;
    s32 i;

    switch (info) {
    case PLEASURE_BOAT_INFO_STARBOARD_PEOPLE:
        value = boat->infoCounts[0];
        break;
    case PLEASURE_BOAT_INFO_PORT_PEOPLE:
        value = boat->infoCounts[1];
        break;
    case PLEASURE_BOAT_INFO_OTHER_DOOR_TRAINERS:
        value = boat->infoCounts[2];
        break;
    case PLEASURE_BOAT_INFO_BLUE_DOOR_TRAINERS:
        value = boat->infoCounts[3];
        break;
    case PLEASURE_BOAT_INFO_TRAINERS:
        value = boat->trainerCount;
        break;
    case PLEASURE_BOAT_INFO_TRAINERS_DEFEATED:
        value = 0;
        for (i = 0; i < PLEASURE_BOAT_ROOM_COUNT; i++) {
            if (boat->rooms[i].defeated != 0) {
                value++;
            }
        }
        break;
    default:
        value = 0;
        break;
    }
    return value;
}

u32 PleasureBoat_GetPassengerParam(PleasureBoat *boat, u32 room) {
    return boat->rooms[room].kind;
}

u32 PleasureBoat_GetPassengerType(PleasureBoat *boat, u32 room) {
    return boat->rooms[room].trainerId;
}

u32 PleasureBoat_GetPassengerExtra(PleasureBoat *boat, u32 room, u32 index) {
    return boat->rooms[room].values[index];
}

BOOL PleasureBoat_GetPassengerOccupied(PleasureBoat *boat, u32 room) {
    return boat->rooms[room].defeated;
}

void PleasureBoat_SetTrainerDefeated(PleasureBoat *boat, u32 trainer, BOOL defeated) {
    boat->rooms[trainer].defeated = defeated;
}

static BOOL PleasureBoat_GetTimeEvent(PleasureBoat *boat) {
    if (boat->clock >= PLEASURE_BOAT_PERIOD_COUNT * PLEASURE_BOAT_PERIOD &&
        boat->period >= PLEASURE_BOAT_PERIOD_COUNT) {
        return PLEASURE_BOAT_TIME_CRUISE_END;
    }
    return boat->clock % PLEASURE_BOAT_PERIOD == 0;
}

void PleasureBoat_StopClock(PleasureBoat *boat) {
    boat->clockStopped = TRUE;
}
