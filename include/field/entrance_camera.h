#ifndef POKEBW2_FIELD_ENTRANCE_CAMERA_H
#define POKEBW2_FIELD_ENTRANCE_CAMERA_H

// Overlay 36's entrance_camera.c: the camera of the entrance events, which moves to or from a camera file's position
// as the player comes out of an entrance or goes into it, and the events of a warp that walk the player through the
// entrance

#include "types.h"
#include "struct_decls.h"

// What the entrance camera plays: the warp's transition, coming out of the entrance or going into it
typedef struct {
    u32 transitionType;
    BOOL isArrival;
    BOOL isDeparture;
} EntranceCameraParam;

// The event that walks the player into the entrance
GameEvent *func_ov036_021b74a0(GameSystem *gsys, WarpSequence *warp);

// The entrance camera's lifetime, parameters and animation
EntranceCamera *EntranceCamera_Create(Field *field);
void EntranceCamera_Destroy(EntranceCamera *camera);
void EntranceCamera_Configure(EntranceCamera *camera, const EntranceCameraParam *param);
void EntranceCamera_StartAnimation(EntranceCamera *camera);
void EntranceCamera_FinishCamera(EntranceCamera *camera);
// The selected arrival or departure configuration flag
BOOL func_ov036_021b7ae0(EntranceCamera *camera);
// Whether the configured animation has a duration of zero frames
BOOL EntranceCamera_HasZeroFrames(EntranceCamera *camera);

#endif // POKEBW2_FIELD_ENTRANCE_CAMERA_H
