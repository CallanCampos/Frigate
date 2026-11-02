#ifndef POKEBW2_FIELD_GIMMICK_ROLLER_COASTER_H
#define POKEBW2_FIELD_GIMMICK_ROLLER_COASTER_H

#include "types.h"
#include "struct_decls.h"

// The roller coaster of Nimbasa City's zone 585 (gimmick 48), overlay 102

#define ROLLER_COASTER_CAR_COUNT 4

// The gimmick's saved state
typedef struct {
    fx32 frames[ROLLER_COASTER_CAR_COUNT];
    u8 switches[ROLLER_COASTER_CAR_COUNT];
    // Whether the first ride of cars 1 and 2 has been shown
    u16 introSeen[2];
} RollerCoasterSave;

// Overlay 36's gimmick table calls these
void RollerCoaster_Init(Field *field);
void RollerCoaster_End(Field *field);
void RollerCoaster_Update(Field *field);

// Overlay 36's script commands call these. Starts the coaster's ambience at volume 0
void RollerCoaster_StartAmbience(GameSystem *gsys);
// Presses one of the four switches, which changes its car's path. NULL for a switch out of range
GameEvent *RollerCoaster_CreateSwitchEvent(GameSystem *gsys, u8 switchId);
// Marks the first ride of car 1 or 2 as shown, so that later rides skip its scripts
void RollerCoaster_SetCarIntroSeen(GameSystem *gsys, u16 car);

// Overlay 36's auto tile check calls this: the ride when the player faces up at a station where a car waits
GameEvent *RollerCoaster_CheckRide(GameSystem *gsys);

#endif // POKEBW2_FIELD_GIMMICK_ROLLER_COASTER_H
