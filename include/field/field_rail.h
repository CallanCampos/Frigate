#ifndef POKEBW2_FIELD_FIELD_RAIL_H
#define POKEBW2_FIELD_FIELD_RAIL_H

// The rail system of the no-grid maps, where the player moves along rails. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except FieldRailSystem_Free, FieldRailSystem_Reset and
// FieldRailSystem_SetLineEnabled. Layouts from swan's RailPoint, RailLine, RailCurve, RailCamera, RailDataHandle,
// RailUnit and FieldRailSystem

#include "types.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// A point of the rails, where up to four lines meet: each line's ID (0xffff for none), the direction it leaves in and
// the rail's width there
struct RailPoint {
    u32 lineIds[4];
    u32 dirs[4];
    s32 widths[4];
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
    s32 unk4;
    s32 unk8;
    s32 unkC;
    s32 unk10;
    s32 unk14;
    s32 unk18;
    s32 unk1C;
    s32 unk20;
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

// The lines that are off, 0xffff in a free slot
struct RailDisabledLines {
    u16 lineIds[18];
};

// Something that moves along the rails: the point it is at, or the line, with its position along the line (front)
// and across it (side), in sixteenths of a tile
struct RailUnit {
    u16 onLine;
    u16 acquired;
    union {
        RailPoint *point;
        RailLine *line;
    } component;
    u16 active;
    s32 front;
    s32 length;
    s32 side;
    s32 width;
    fx32 tileWidth;
    u32 dir;
    RailDataHandle *handle;
    RailDisabledLines *disabled;
    u8 pending;
    s8 counter;
    u16 pendingArgB;
    u16 pendingArgA;
    u8 unk32;
    u8 unk33;
    u8 unk34;
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
    u16 heapId;
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
void func_ov036_021b3a58(RailAttr *attr);
void FieldRailTilemap_Load(RailAttr *attr, u32 fileId, u32 heapId);
void func_ov036_021b3ad0(RailAttr *attr);
u32 FieldRailTilemap_GetTileAtPos(RailAttr *attr, const RailPosition *pos);
// Whether a tile class is one of the rails' special tiles
BOOL func_ov036_021b3b3c(u16 tileClass);
BOOL func_ov036_021b3b54(u32 tileClass);
BOOL func_ov036_021b3b64(u32 tileClass);
BOOL func_ov036_021b3b70(u32 tileClass);
BOOL func_ov036_021b3b7c(u32 tileClass);
BOOL func_ov036_021b3b88(u32 tileClass);
BOOL func_ov036_021b3b94(u32 tileClass);
BOOL func_ov036_021b3ba0(u32 tileClass);

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
// Does nothing: what is left of a check whether the rails are loaded and active
void func_ov036_021b0398(FieldRailSystem *rail);
// Updates the camera from its parent unit when the unit asks for it, the camera is dirty or force is set
BOOL FieldRailSystem_UpdateCamera(FieldRailSystem *rail, BOOL force);
// Finds where the segment from start to end meets a rail
BOOL CalculateRailCurves(FieldRailSystem *rail, const VecFx32 *start, const VecFx32 *end, RailPosition *railPos,
                         VecFx32 *hit);
// Turns a line on or off: the system keeps up to 18 lines that are off, and finds no line where one of them is
void FieldRailSystem_SetLineEnabled(FieldRailSystem *rail, u32 lineId, BOOL enabled);
// The length of a step on the rails
fx32 func_ov036_021b05ec(FieldRailSystem *rail);
// Sets pos to line componentId of the rail data, front steps along it and side steps from its edge
void FieldRailSystem_NormalizePos(FieldRailSystem *rail, RailDataHandle *data, u16 componentId, u16 front, u16 side,
                                  RailPosition *pos);
// The world position a step from the rail position in a direction, and whether that step stays on the rails
BOOL func_ov036_021b0644(FieldRailSystem *rail, const RailPosition *pos, u32 railDir, VecFx32 *calcPos);
// The rail position a step from railPos in a direction, and whether that step stays on the rails
BOOL func_ov036_021b068c(FieldRailSystem *rail, const RailPosition *railPos, u32 railDir, RailPosition *next);
// The world position of the rail position
void func_ov036_021b06ec(FieldRailSystem *rail, const RailPosition *railPos, VecFx32 *pos);
s32 func_ov036_021b0704(FieldRailSystem *rail, const RailPosition *pos);
void FieldRailSystem_SetPlayerPos(FieldRailSystem *rail, const RailPosition *pos);
void func_ov036_021b0774(RailUnit *unit, RailPosition *pos);
// Whether the rail position is on the rail data
BOOL func_ov036_021b0794(RailUnit *unit, const RailPosition *pos);
// The rail and world position a step from the unit in a direction, kept until the unit moves, and whether there is one
u8 func_ov036_021b0814(RailUnit *unit, u32 dir, RailPosition *outPos, VecFx32 *outVec);
void func_ov036_021b0888(RailUnit *unit, VecFx32 *pos);
void RailUnit_SetRailPos(RailUnit *unit, const RailPosition *pos);
void RailUnit_GetCalcPos(RailUnit *unit, VecFx32 *pos);
u8 func_ov036_021b09ec(RailUnit *unit);
// Whether the unit has a step to take
u8 func_ov036_021b09f4(RailUnit *unit);
// The unit vector along the unit's line
void func_ov036_021b0a04(RailUnit *unit, VecFx16 *dir);
u32 func_ov036_021b0a8c(RailUnit *unit);
// Starts a step of speed index speed in a direction, FALSE if the unit is already taking one
BOOL func_ov036_021b0a94(RailUnit *unit, u32 speed, u32 dir);
// Moves the unit on with its step
void func_ov036_021b0ac0(RailUnit *unit);
// Whether two positions are within dist of each other across and half a tile up or down
BOOL func_ov036_021b0d64(const VecFx32 *a, const VecFx32 *b, fx32 dist);
// Writes the unit's rail position to three script variables, or sets it from three values
void SetWkToActorRailPos(RailUnit *unit, u16 *a1, u16 *a2, u16 *a3);
void func_ov036_021b0e38(RailUnit *unit, u16 a1, u16 a2, u16 a3);
// The curves' conversions between rail and world positions
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
void func_ov036_021b2cc4(FieldRailSystem *rail);
void func_ov036_021b2cfc(FieldRailSystem *rail);
void func_ov036_021b2e48(FieldRailSystem *rail);
void func_ov036_021b2e78(FieldRailSystem *rail);
void func_ov036_021b2eb0(FieldRailSystem *rail);
void func_ov036_021b2f04(FieldRailSystem *rail);
void func_ov036_021b309c(FieldRailSystem *rail);
void func_ov036_021b3164(FieldRailSystem *rail);
void func_ov036_021b3294(FieldRailSystem *rail);
void func_ov036_021b3368(FieldRailSystem *rail);
void func_ov036_021b33e4(FieldRailSystem *rail);
void func_ov036_021b3450(FieldRailSystem *rail);
void func_ov036_021b34e4(FieldRailSystem *rail);
void func_ov036_021b3568(FieldRailSystem *rail);
void func_ov036_021b3590(FieldRailSystem *rail);
void func_ov036_021b35b8(FieldRailSystem *rail);
void func_ov036_021b3628(FieldRailSystem *rail);
void func_ov036_021b365c(FieldRailSystem *rail);

#endif // POKEBW2_FIELD_FIELD_RAIL_H
