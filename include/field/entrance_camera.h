#ifndef POKEBW2_FIELD_ENTRANCE_CAMERA_H
#define POKEBW2_FIELD_ENTRANCE_CAMERA_H

// Overlay 36's entrance_camera.c: the camera of the entrance events, which moves to or from a camera file's position
// as the player comes out of an entrance or goes into it, and the events of a warp that walk the player through the
// entrance

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/game_event.h"

// What the entrance camera plays: the warp's transition, coming out of the entrance or going into it
typedef struct {
    u32 transitionType;
    BOOL isArrival;
    BOOL isDeparture;
} EntranceCameraParam;

// An entrance camera's file in archive 0xb1, one for each transition type from WARP_TRANSITION_CAMERA: where the camera
// starts or ends, in whole units
typedef struct {
    u32 unk0;
    u32 pitch;
    u32 yaw;
    s32 distance;
    BOOL useTarget;
    s32 target[3];
    s32 offset[3];
    u32 frames;
    BOOL hasDepartureAnimation;
    BOOL hasArrivalAnimation;
    // The camera animation's releaseBind and disableBoundary
    BOOL releaseBind;
    BOOL disableBoundary;
} EntranceCameraFile;

// The events of a warp's ways out of a zone and into one, which EventWarpSequence_CreateOut and
// EventWarpSequence_CreateIn pick by its type
GameEventReturnCode EventWarpSequenceOut_RunPlain(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventWarpSequenceOut_RunGroundAnime(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventWarpSequenceOut_RunObjectAnime(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventWarpSequenceOut_RunAppear(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventWarpSequenceOut_RunCamera(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventWarpSequenceOut_RunFadeSe(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventWarpSequenceIn_RunPlain(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventWarpSequenceIn_RunGroundAnime(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventWarpSequenceIn_RunObjectAnime(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventWarpSequence_CallbackSpinDown(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventWarpSequenceIn_RunCamera(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventWarpSequenceIn_RunFadeSe(GameEvent *event, u32 *state, void *data);

// The event that walks the player into the entrance
GameEvent *EventWarpSequence_CreateWalkEvent(GameSystem *gsys, WarpSequence *warp);

// The entrance camera's lifetime, parameters and animation
EntranceCamera *EntranceCamera_Create(Field *field);
void EntranceCamera_Destroy(EntranceCamera *camera);
void EntranceCamera_Configure(EntranceCamera *camera, const EntranceCameraParam *param);
void EntranceCamera_StartAnimation(EntranceCamera *camera);
void EntranceCamera_FinishCamera(EntranceCamera *camera);
// Whether the configured arrival or departure has an animation
BOOL EntranceCamera_IsActive(EntranceCamera *camera);
// Whether the configured animation has a duration of zero frames
BOOL EntranceCamera_HasZeroFrames(EntranceCamera *camera);
EntranceCamera *EntranceCamera_Allocate(HeapID heapId);
void EntranceCamera_Free(EntranceCamera *camera);
void EntranceCamera_Init(EntranceCamera *camera, Field *field);
void EntranceCamera_CreateController(EntranceCamera *camera, HeapID heapId);
void EntranceCamera_FreeController(EntranceCamera *camera);
// Sets up the animation from the player's direction, or from the transition type's file
void EntranceCamera_ConfigureDefaultDeparture(EntranceCamera *camera);
void EntranceCamera_ConfigureDefaultArrival(EntranceCamera *camera);
void EntranceCamera_ConfigureFileDeparture(EntranceCamera *camera);
void EntranceCamera_ConfigureFileArrival(EntranceCamera *camera);
// Reads the file of a transition type, and moves the camera's target to a position
void EntranceCamera_ReadFile(EntranceCameraFile *dest, u32 index);
void FieldCamera_ChangeRefPos(FieldCamera *camera, const VecFx32 *target);
FieldCamera *EntranceCamera_GetFieldCamera(EntranceCamera *camera);
EntranceCameraParam *EntranceCamera_GetParam(EntranceCamera *camera);
void EntranceCamera_SetParam(EntranceCamera *camera, const EntranceCameraParam *param);
u32 EntranceCamera_GetTransitionType(EntranceCamera *camera);
// Whether the transition type has a camera file, whether the camera doesn't depart, and whether it animates
BOOL EntranceCamera_HasRecord(EntranceCamera *camera);
BOOL EntranceCamera_AimsAtEntrance(EntranceCamera *camera);
BOOL EntranceCamera_IsPhaseActive(EntranceCamera *camera);

#endif // POKEBW2_FIELD_ENTRANCE_CAMERA_H
