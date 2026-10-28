#ifndef POKEBW2_FIELD_PLEASURE_BOAT_H
#define POKEBW2_FIELD_PLEASURE_BOAT_H

#include "types.h"
#include "struct_decls.h"

// The Royal Unova's cruise (pleasure_boat.c, in overlay 36): its clock, and the people and Trainers aboard

// What PleasureBoat_GetInfo counts: the Trainers aboard on each side, in the cabins with blue doors (overlay 89's
// rooms 9 to 14) and in the others, all the Trainers, and those defeated
enum {
    PLEASURE_BOAT_INFO_STARBOARD_PEOPLE,
    PLEASURE_BOAT_INFO_PORT_PEOPLE,
    PLEASURE_BOAT_INFO_OTHER_DOOR_TRAINERS,
    PLEASURE_BOAT_INFO_BLUE_DOOR_TRAINERS,
    PLEASURE_BOAT_INFO_TRAINERS,
    PLEASURE_BOAT_INFO_TRAINERS_DEFEATED,
};

#define PLEASURE_BOAT_ROOM_COUNT 15

// A room of the ship and who is in it. Overlay 89 fills it
typedef struct {
    // Which of the kind's people the room has
    u32 index;
    // The Trainer's ID, or 0 for anyone else
    u32 trainerId;
    // The object code of the person
    u32 objCode;
    // 0 for a Trainer, 1 for a person and 2 for a pair of them
    u32 kind;
    u32 values[2];
    // Whether the Trainer was defeated
    u32 defeated;
} PleasureBoatRoom;

struct PleasureBoat {
    // Set when a period ends, until PleasureBoat_AckEvent plays its sound and counts it
    u32 periodEnded;
    // The clock, which counts periods of 1350
    s16 clock;
    // How many Trainers are aboard, by the day of the week
    u16 trainerCount;
    // The last period reached
    s32 period;
    PleasureBoatRoom rooms[PLEASURE_BOAT_ROOM_COUNT];
    // The answers to PLEASURE_BOAT_INFO_STARBOARD_PEOPLE to PLEASURE_BOAT_INFO_BLUE_DOOR_TRAINERS
    u8 infoCounts[4];
    // Set by PleasureBoat_StopClock
    u32 clockStopped;
};

// Sets up a cruise, with overlay 89, and with the second table of Trainers when altTrainers is set
PleasureBoat *PleasureBoat_Create(BOOL altTrainers);
// Frees the cruise and clears the pointer
void PleasureBoat_Free(PleasureBoat **boat);
// Returns PLEASURE_BOAT_INFO_*
u32 PleasureBoat_GetInfo(PleasureBoat *boat, u32 info);
// Moves the clock on by steps * 30. The clock counts periods of 1350, and the cruise plays SEQ_SE_FLD_78 as one ends.
// If this reaches the end of the period, the clock stops just short of it when stopBefore is set, so that the period
// ends on the next update, and otherwise starts the next period
void PleasureBoat_AdvanceClock(PleasureBoat *boat, u32 steps, BOOL stopBefore);
void PleasureBoat_SetTrainerDefeated(PleasureBoat *boat, u32 trainer, BOOL defeated);
void PleasureBoat_StopClock(PleasureBoat *boat);
void PleasureBoat_AckEvent(PleasureBoat *boat);
// Moves the clock on by one, and returns the script event that ends the cruise once it is over, or NULL
GameEvent *PleasureBoat_Update(GameSystem *gsys, PleasureBoat *boat);
u32 PleasureBoat_GetPassengerParam(PleasureBoat *boat, u32 room);
u32 PleasureBoat_GetPassengerType(PleasureBoat *boat, u32 room);
u32 PleasureBoat_GetPassengerExtra(PleasureBoat *boat, u32 room, u32 index);
BOOL PleasureBoat_GetPassengerOccupied(PleasureBoat *boat, u32 room);

// Overlay 12's event_royal_unova.c
BOOL Scrcmd_RoyalUnova_PleasureBoatGetRoomInfo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_RoyalUnova_CallRoyalUnovaView(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_PLEASURE_BOAT_H
