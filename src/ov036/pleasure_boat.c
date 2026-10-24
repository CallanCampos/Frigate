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

static BOOL PleasureBoat_GetTimeEvent(PleasureBoat *boat);

PleasureBoat *PleasureBoat_Create(BOOL a0) {
    PleasureBoat *boat = GFL_HeapAllocate(HEAPID_TRIAL_HOUSE, sizeof(PleasureBoat), TRUE, "pleasure_boat.c", 49);

    boat->unk0 = 0;
    boat->clock = 0;
    boat->period = 0;
    boat->infoCounts[0] = 0;
    boat->infoCounts[1] = 0;
    boat->infoCounts[2] = 0;
    boat->infoCounts[3] = 0;
    boat->unk1B4 = 0;
    GFL_OvlLoad(OVERLAY_ID(89));
    PleasureBoat_SetUpRooms(boat, a0);
    GFL_OvlUnload(OVERLAY_ID(89));
    return boat;
}

void PleasureBoat_AckEvent(PleasureBoat *boat) {
    if (boat != NULL && boat->unk1B4 == 0 && boat->unk0 != 0) {
        GFL_SndSEPlay(SEQ_SE_FLD_78);
        boat->period++;
        boat->unk0 = 0;
    }
}

GameEvent *PleasureBoat_Update(GameSystem *gsys, PleasureBoat *boat) {
    GameEvent *event;
    BOOL created;

    if (boat == NULL) {
        return NULL;
    }
    if (boat->unk1B4 != 0) {
        return NULL;
    }
    boat->clock++;
    switch (PleasureBoat_GetTimeEvent(boat)) {
    case 1:
        boat->unk0 = 1;
        created = FALSE;
        break;
    case 2:
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
    case PLEASURE_BOAT_INFO_UNK_2:
        value = boat->infoCounts[2];
        break;
    case PLEASURE_BOAT_INFO_UNK_3:
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
    return boat->rooms[room].unk4;
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
    if (boat->clock >= 0x1518 && boat->period >= 4) {
        return 2;
    }
    return boat->clock % PLEASURE_BOAT_PERIOD == 0;
}

void PleasureBoat_StopClock(PleasureBoat *boat) {
    boat->unk1B4 = 1;
}
