#ifndef POKEBW2_FIELD_FIELD_RAIL_H
#define POKEBW2_FIELD_FIELD_RAIL_H

// The rail system of the no-grid maps, where the player moves along rails. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except FieldRailSystem_Free, FieldRailSystem_Reset and
// FieldRailSystem_SetLineEnabled, RailUnit_GetMoveDirection, RailUnit_IsMovePending, RailUnit_GetLineDirection,
// RailUnit_RequestMove, RailUnit_UpdateMove, RailUnit_GetRailPos, RailUnit_IsRailPosInBounds,
// RailUnit_PredictNextPos and RailUnit_GetPreviousCalcPos, which are our descriptive names. Layouts from swan's
// RailPoint, RailLine, RailCurve, RailCamera, RailDataHandle, RailUnit and FieldRailSystem. Additional movement state
// field names describe recovered behavior; direction follows swan's Direction field.

#include "types.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The ID of no point, line, camera or curve, and of a free slot of RailDisabledLines
#define RAIL_ID_NONE 0xffff

// The lines that meet at a point, and the lines that can be off at once
#define RAIL_POINT_LINE_MAX 4
#define RAIL_DISABLED_LINE_MAX 18

// The directions on the rails, which ConvDirToRailDir gives for the actors' directions, clockwise from up
enum {
    RAIL_DIR_NONE,
    RAIL_DIR_UP,
    RAIL_DIR_RIGHT,
    RAIL_DIR_DOWN,
    RAIL_DIR_LEFT,
};

// The speeds of RailUnit_RequestMove: a step of 1, 2, 4, 8 or 16 sixteenths of a tile a frame
enum {
    RAIL_SPEED_1,
    RAIL_SPEED_2,
    RAIL_SPEED_4,
    RAIL_SPEED_8,
    RAIL_SPEED_16,
    RAIL_SPEED_MAX,
};

// A point of the rails, where up to four lines meet: each line's ID (RAIL_ID_NONE for none), the RAIL_DIR_ direction
// it leaves in and the rail's width there
struct RailPoint {
    u32 lineIds[RAIL_POINT_LINE_MAX];
    u32 dirs[RAIL_POINT_LINE_MAX];
    s32 widths[RAIL_POINT_LINE_MAX];
    VecFx32 pos;
    u32 cameraId;
    char name[48];
};

// A line between two points, length tiles long
struct RailLine {
    u32 pointA;
    u32 pointB;
    u32 dir;
    u32 curveId;
    u32 cameraId;
    u32 length;
    char name[48];
};

// A camera of the rails: the camera function to call, and its parameters
struct RailCamera {
    u32 funcId;
    // The function's parameters, which each reads as its own: a pitch, yaw and zoom, an eye and a target, or an angle,
    // a distance and the targets it moves between
    s32 params[8];
};

// How a line bends: the function pair that converts between rail and world positions, and the center of its arc
struct RailCurve {
    u32 handlerId;
    VecFx32 center;
    u8 unk10[0x14];
};

typedef void (*RailCameraFunc)(FieldRailSystem *rail);
typedef void (*RailToWorldFunc)(RailUnit *unit, VecFx32 *pos);
typedef BOOL (*WorldToRailFunc)(u32 lineId, FieldRailSystem *rail, const VecFx32 *pos, RailPosition *railPos,
                                VecFx32 *alignedPos);

// The points, lines, cameras and curves of a map's rails, and the function tables that the cameras and curves index
struct RailDataHandle {
    u16 pointCount;
    u16 lineCount;
    u16 cameraCount;
    u16 curveCount;
    u16 camFuncCount;
    u16 railToWorldCount;
    u16 worldToRailCount;
    u16 unkE;
    RailPoint *points;
    RailLine *lines;
    RailCamera *cameras;
    RailCurve *curves;
    RailCameraFunc *camFuncs;
    RailToWorldFunc *railToWorldFuncs;
    WorldToRailFunc *worldToRailFuncs;
    fx32 tileWidth;
};

// The lines that are off, RAIL_ID_NONE in a free slot
struct RailDisabledLines {
    u16 lineIds[RAIL_DISABLED_LINE_MAX];
};

// RailUnit's component tag; a unit has no component before placement
enum {
    RAIL_COMPONENT_POINT,
    RAIL_COMPONENT_LINE,
    RAIL_COMPONENT_NONE,
};

// Something that moves along the rails: the point it is at, or the line, with its position along the line (front)
// and across it (side), in sixteenths of a tile
struct RailUnit {
    u16 onLine; // RAIL_COMPONENT_*, the tag of component
    u16 acquired;
    union {
        RailPoint *point;
        RailLine *line;
    } component;
    u16 positionInitialized; // Set after a rail position is initialized
    s32 front;
    s32 length;
    s32 side;
    s32 width;
    fx32 tileWidth;
    u32 dir;
    const RailDataHandle *handle;
    RailDisabledLines *disabled;
    u8 movePending;
    s8 moveProgress; // Attempted sixteenths toward the current 16-sixteenth request
    u16 pendingDir;
    u16 pendingSpeedIndex;
    u8 direction;          // Placement/accepted move direction, distinct from dir in the stored rail position
    u8 lastMoveSpeedIndex; // Accepted request speed index; placement initializes it to 1
    // Set by the frame whose pending move went along the rail
    u8 stepped;
    VecFx32 calcPos;
    VecFx32 prevCalcPos;
    VecFx32 nextVec;
    RailPosition railPos;
    RailPosition prevRailPos;
    RailPosition nextPos;
    u8 nextCached;
    u8 nextOk;
    u8 nextDir;
    FieldRailSystem *rail;
};

struct FieldRailSystem {
    HeapID heapId;
    u16 loaded;
    u16 cameraEnabled;
    RailUnit *units;
    s32 unitCount;
    FieldCamera *camera;
    RailUnit *cameraParent;
    u32 cameraDirty;
    RailUnit *playerUnit;
    RailDataHandle data;
    RailDisabledLines disabledLines;
};

// rail_attr.c: the tile attributes of the rail lines
typedef struct RailAttr RailAttr;

RailAttr *AllocateRailAttrBlock(u32 heapId);
void FieldRailTilemap_Delete(RailAttr *attr);
void FieldRailTilemap_Load(RailAttr *attr, u32 fileId, u32 heapId);
void FieldRailTilemap_Release(RailAttr *attr);
u32 FieldRailTilemap_GetTileAtPos(RailAttr *attr, const RailPosition *pos);
// Whether a tile class is one of the rails' special tiles
BOOL FieldRailTilemap_IsSignBoard(u16 tileClass);
BOOL FieldRailTilemap_IsJumpStart(u32 tileClass);
BOOL FieldRailTilemap_IsForceMoveUp(u32 tileClass);
BOOL FieldRailTilemap_IsForceMoveDown(u32 tileClass);
BOOL FieldRailTilemap_IsForceMoveLeft(u32 tileClass);
BOOL FieldRailTilemap_IsForceMoveRight(u32 tileClass);
BOOL FieldRailTilemap_IsForceMoveStop(u32 tileClass);
BOOL FieldRailTilemap_IsSlipDown(u32 tileClass);

// field_rail.c: the rail system
FieldRailSystem *FieldRailSystem_Create(HeapID heapId, u32 unitCount, FieldCamera *camera);
void FieldRailSystem_Free(FieldRailSystem *rail);
// Starts on the rail data, with every line on
void FieldRailSystem_Load(FieldRailSystem *rail, RailDataHandle *data);
void FieldRailSystem_Reset(FieldRailSystem *rail);
BOOL FieldRailSystem_HasData(FieldRailSystem *rail);
RailUnit *FieldRailSystem_AcquireUnit(FieldRailSystem *rail);
void FieldRailSystem_ReleaseUnit(FieldRailSystem *rail, RailUnit *unit);
void FieldRailSystem_SetCameraParent(FieldRailSystem *rail, RailUnit *unit);
void FieldRailSystem_ClearCameraParent(FieldRailSystem *rail);
void FieldRailSystem_GetCameraParentPos(FieldRailSystem *rail, VecFx32 *pos);
// Does nothing: what is left of a check whether the rails are loaded and active
void FieldRailSystem_Update(FieldRailSystem *rail);
// Updates the camera from its parent unit when the unit asks for it, the camera is dirty or force is set
BOOL FieldRailSystem_UpdateCamera(FieldRailSystem *rail, BOOL force);
// Finds where the segment from start to end meets a rail
BOOL CalculateRailCurves(FieldRailSystem *rail, const VecFx32 *start, const VecFx32 *end, RailPosition *railPos,
                         VecFx32 *hit);
// Turns a line on or off: the system keeps up to 18 lines that are off, and finds no line where one of them is
void FieldRailSystem_SetLineEnabled(FieldRailSystem *rail, u32 lineId, BOOL enabled);
// The length of a step on the rails
fx32 FieldRailSystem_GetUnitScale(FieldRailSystem *rail);
// Sets pos to line componentId of the rail data, front steps along it and side steps from its edge
void FieldRailSystem_NormalizePos(FieldRailSystem *rail, RailDataHandle *data, u16 componentId, u16 front, u16 side,
                                  RailPosition *pos);
// The world position a step from the rail position in a direction, and whether that step stays on the rails
BOOL FieldRailSystem_StepLocation(FieldRailSystem *rail, const RailPosition *pos, u32 railDir, VecFx32 *calcPos);
// The rail position a step from railPos in a direction, and whether that step stays on the rails
BOOL FieldRailSystem_TryStepLocation(FieldRailSystem *rail, const RailPosition *railPos, u32 railDir,
                                     RailPosition *next);
// The world position of the rail position
void FieldRailSystem_GetLocationPos(FieldRailSystem *rail, const RailPosition *railPos, VecFx32 *pos);
s32 FieldRailSystem_GetLocationWidthGrid(FieldRailSystem *rail, const RailPosition *pos);
void FieldRailSystem_SetPlayerPos(FieldRailSystem *rail, const RailPosition *pos);
void RailUnit_GetRailPos(const RailUnit *unit, RailPosition *pos);
// Checks the component ID and, for lines, the front and side bounds
BOOL RailUnit_IsRailPosInBounds(RailUnit *unit, const RailPosition *pos);
// Caches a one-tile attempt in dir until the unit moves. Either output may be NULL; requested outputs are filled
// even when the attempt fails.
u8 RailUnit_PredictNextPos(RailUnit *unit, u32 dir, RailPosition *outPos, VecFx32 *outVec);
void RailUnit_GetPreviousCalcPos(RailUnit *unit, VecFx32 *pos);
void RailUnit_SetRailPos(RailUnit *unit, const RailPosition *pos);
void RailUnit_GetCalcPos(RailUnit *unit, VecFx32 *pos);
// The placement direction or most recent accepted movement result
u8 RailUnit_GetMoveDirection(RailUnit *unit);
// Whether a move is queued or in progress
u8 RailUnit_IsMovePending(RailUnit *unit);
// The unit vector along the unit's line
void RailUnit_GetStepVector(RailUnit *unit, VecFx16 *dir);
// The current line's direction; requires a unit placed on a line
u32 RailUnit_GetLineDirection(RailUnit *unit);
// Queues a move when idle, at a RAIL_SPEED_ speed in a RAIL_DIR_ direction
BOOL RailUnit_RequestMove(RailUnit *unit, u32 speedIndex, u32 dir);
// Attempts the indexed step, ending the request on completion or failure
void RailUnit_UpdateMove(RailUnit *unit);
FieldCamera *FieldRailSystem_GetFieldCamera(FieldRailSystem *rail);
RailUnit *FieldRailSystem_GetCameraParentUnit(FieldRailSystem *rail);
RailPoint *FieldRailSystem_GetPoint(FieldRailSystem *rail, u32 id);
RailLine *FieldRailSystem_GetLine(FieldRailSystem *rail, u32 id);
RailCurve *GetRailHnd2Curve(FieldRailSystem *rail, u32 id);
fx32 FieldRailSystem_GetTileWidth(FieldRailSystem *rail);
RailUnit *FieldRailSystem_GetProbeWork(FieldRailSystem *rail);
void FieldRailSystem_ClampLocation(const FieldRailSystem *rail, const RailPosition *in, RailPosition *out);
// Returns the RAIL_COMPONENT_* tag
u16 RailUnit_IsOnLine(RailUnit *unit);
RailLine *GetCurrentLine(RailUnit *unit);
s32 GetCurrentLinePosV(RailUnit *unit);
s32 GetCurrentLineLength(RailUnit *unit);
s32 GetCurrentLinePosH(RailUnit *unit);
s32 RailUnit_GetWidthLimit(RailUnit *unit);
fx32 RailUnit_GetTileWidth(RailUnit *unit);
RailCamera *GetRailCameraByCamHandler(RailUnit *unit);
void RailUnit_GetCentrePos(RailUnit *unit, VecFx32 *pos);
void RailUnit_GetPosAtLineOffset(RailUnit *unit, s32 front, VecFx32 *pos);
RailPoint *GetCurrentRailLinePoint1(RailUnit *unit);
RailPoint *GetCurrentRailLinePoint2(RailUnit *unit);
RailCurve *RailUnit_GetCurve(RailUnit *unit);
RailCamera *GetRailCameraByPoint(RailUnit *unit, RailPoint *point);
s32 FieldRailSystem_RoundToGrid(s32 value);
// Whether two positions are within dist of each other across and half a tile up or down
BOOL FieldRailSystem_IsPosNear(const VecFx32 *a, const VecFx32 *b, fx32 dist);
// Writes the unit's rail position to three script variables, or sets it from three values: the line, the steps along
// it and the steps across it from its edge
void SetWkToActorRailPos(const RailUnit *unit, u16 *lineIndex, u16 *front, u16 *side);
void RailUnit_SetLineLocation(RailUnit *unit, u16 lineIndex, u16 front, u16 side);
// field_rail_func.c: the curves' conversions between rail and world positions
void RailMath_RailToWorldNoCurve(RailUnit *unit, VecFx32 *out);
void RailMath_RailToWorldSlerpXZ(RailUnit *unit, VecFx32 *out);
void RailMath_RailToWorldSlerpXYZ(RailUnit *unit, VecFx32 *out);
BOOL RailMath_WorldToRailNoCurve(u32 lineId, FieldRailSystem *rail, const VecFx32 *pos, RailPosition *outPos,
                                 VecFx32 *outWorld);
BOOL RailMath_WorldToRailSlerp(u32 lineId, FieldRailSystem *rail, const VecFx32 *pos, RailPosition *outPos,
                               VecFx32 *outWorld);
// The camera functions of the rails' cameras
void RailCameraFuncSetStaticPYR(FieldRailSystem *rail);
void RailCameraFuncSetStaticPYDist(FieldRailSystem *rail);
void FieldRailCamera_FixedAngleAtCentre(FieldRailSystem *rail);
void FieldRailCamera_BlendAngles(FieldRailSystem *rail);
void FieldRailCamera_BlendAnglesAtFocus(FieldRailSystem *rail);
void FieldRailCamera_BlendAnglesAtCentre(FieldRailSystem *rail);
void FieldRailCamera_FixedEyeTarget(FieldRailSystem *rail);
void FieldRailCamera_BlendEyeTarget(FieldRailSystem *rail);
void FieldRailCamera_BlendOrbit(FieldRailSystem *rail);
void FieldRailCamera_Orbit(FieldRailSystem *rail);
void FieldRailCamera_OrbitMovingPoint(FieldRailSystem *rail);
void FieldRailCamera_YawFromPoint(FieldRailSystem *rail);
void FieldRailCamera_FollowHeading(FieldRailSystem *rail);
void FieldRailCamera_FollowHeadingSlope(FieldRailSystem *rail);
void FieldRailCamera_FollowHeadingAtCentre(FieldRailSystem *rail);
void FieldRailCamera_ApplyLineStart(FieldRailSystem *rail);
void FieldRailCamera_ApplyLineEnd(FieldRailSystem *rail);
void FieldRailCamera_FixedAngleShift(FieldRailSystem *rail);
void FieldRailCamera_FixedAngleShiftAtFocus(FieldRailSystem *rail);
void FieldRailCamera_BlendAnglesShift(FieldRailSystem *rail);

#endif // POKEBW2_FIELD_FIELD_RAIL_H
