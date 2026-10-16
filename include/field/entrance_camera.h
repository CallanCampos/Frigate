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

// The entrance camera: create, free, set up for a warp, start, finish, and whether it has started and ended
EntranceCamera *func_ov036_021b7a34(Field *field);
void func_ov036_021b7a58(EntranceCamera *camera);
void func_ov036_021b7a68(EntranceCamera *camera, const EntranceCameraParam *param);
void func_ov036_021b7ac0(EntranceCamera *camera);
void func_ov036_021b7ad4(EntranceCamera *camera);
BOOL func_ov036_021b7ae0(EntranceCamera *camera);
BOOL func_ov036_021b7ae8(EntranceCamera *camera);

#endif // POKEBW2_FIELD_ENTRANCE_CAMERA_H
