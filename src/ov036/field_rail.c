// The rail system of the no-grid maps: the rail units that move along the lines and points of a map's rails, and
// the camera the rails set. The name is the ROM's own, from GFL_HeapAllocate's file argument. The curves' conversions
// between rail and world positions and the camera functions that follow in the ROM are field_rail_func.c
// RailUnit_MoveOnLine/TryEnterLine/ClampSideToLineWidth/ApplyLineStep, RailDirection_*, RailLine_IsPointA/B,
// RailLine_CheckEndpointDirections, RailPoint_GetLineDirection/CheckConnections, RailDataHandle_Clear/Copy,
// RailDisabledLines_* and GetRailLineIfEnabled are our descriptive names.
// RailUnit_GetMoveDirection/IsMovePending/GetLineDirection/RequestMove/UpdateMove are descriptive names too.
// RailUnit_GetRailPos/IsRailPosInBounds/PredictNextPos/GetPreviousCalcPos describe the stored-position API.
#include "types.h"
#include "field/field_rail.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/math.h"
#include "nitro/mi.h"

static BOOL RailUnit_HasStepped(RailUnit *unit);
static RailLine *GetLineContinueInDirection(const RailPoint *point, u32 dir, const RailDataHandle *data,
                                            RailDisabledLines *disabled, BOOL checkDisabled);
static u32 RailUnit_TryEnterLine(RailUnit *unit, RailLine *line, u32 dir, s32 front, s32 side, s32 length, s32 width);
static void RailUnit_SetLineInfo(RailUnit *unit, RailLine *line, u32 dir, s32 front, s32 side, s32 length, s32 width);
static BOOL RailLine_IsPointA(const RailDataHandle *data, RailLine *line, RailPoint *point);
static BOOL RailLine_IsPointB(const RailDataHandle *data, RailLine *line, RailPoint *point);
static void SetupLineWidthForLineChange(const RailDataHandle *data, const RailLine *line, s32 *outNext, s32 *outCur,
                                        RailDisabledLines *disabled, fx32 tileWidth);
static void RailDataHandle_Clear(RailDataHandle *data);
static void RailDataHandle_Copy(RailDataHandle *data, RailDataHandle *src);
static RailPoint *GetRailPoint(const RailDataHandle *data, u32 id);
static RailLine *GetRailLine(const RailDataHandle *data, u32 id);
static RailCamera *GetRailCamera(const RailDataHandle *data, u32 id);
static RailCurve *GetRailCurve(const RailDataHandle *data, u32 id);
static RailCameraFunc GetRailCameraCurveFunc(const RailDataHandle *data, u32 id);
static RailToWorldFunc GetHandler1CurveFunc(const RailDataHandle *data, u32 id);
static WorldToRailFunc GetHandler2CurveFunc(const RailDataHandle *data, u32 id);
static u32 RailUnit_MoveOnLine(RailUnit *unit, u32 dir, s32 step);
static void RailUnit_SetDataCore(RailUnit *unit, const RailDataHandle *data, FieldRailSystem *rail);
static void RailUnit_SetData(RailUnit *unit, const RailDataHandle *data);
static u32 RailUnit_GetState(RailUnit *unit);
static BOOL RailUnit_IsActive(RailUnit *unit);
static RailCamera *GetRailCameraByHandlerImpl(RailUnit *unit);
static void RailUnit_GetSpotPos(RailUnit *unit, VecFx32 *pos);
static void RailUnit_GetLocationFromSpot(const RailUnit *unit, RailPosition *pos);
static s32 GetLineTileLengthWorldSpace(const RailLine *line, fx32 tileWidth, const RailDataHandle *data);
static s32 GetLerpedLineWidthByLineYPos(const RailLine *line, s32 y, s32 length, const RailDataHandle *data);
static u32 RailUnit_ClampSideToLineWidth(RailUnit *unit, RailLine *line, s32 length, u32 dir);
static BOOL FieldRailSystem_ProbeStep(FieldRailSystem *rail, RailPosition *pos, u32 dir, s32 extra, VecFx32 *out);
static u32 RailDirection_GetOpposite(u32 dir);
static u32 RailDirection_TurnRight(u32 dir);
static u32 RailDirection_TurnLeft(u32 dir);
static void RailDisabledLines_Clear(RailDisabledLines *disabled);
static void RailDisabledLines_Add(RailDisabledLines *disabled, u16 lineId);
static void RailDisabledLines_Remove(RailDisabledLines *disabled, u16 lineId);
static BOOL RailDisabledLines_Contains(RailDisabledLines *disabled, u16 lineId);
static BOOL RailDisabledLines_ContainsLine(RailDisabledLines *disabled, RailLine *line, const RailDataHandle *data);
static RailLine *GetRailLineIfEnabled(RailDisabledLines *disabled, const RailDataHandle *data, u32 lineId);
static void RailUnit_ApplyLineStep(RailDisabledLines *disabled, RailUnit *unit, s32 frontStep, s32 sideStep, u32 dir);
static BOOL RailDisabledLines_AllowsPosition(RailDisabledLines *disabled, RailLine *line, s32 front, s32 side,
                                             const RailDataHandle *data, fx32 tileWidth);
static u32 RailPoint_GetLineDirection(RailPoint *point, RailLine *line, const RailDataHandle *data);
static BOOL RailLine_CheckEndpointDirections(RailLine *line, const RailDataHandle *data);
static BOOL RailPoint_CheckConnections(RailPoint *point, const RailDataHandle *data);
static void FieldRailSystem_DebugPrintPos(const char *prefix, VecFx32 *pos, const char *suffix);
static void RailPoint_DebugPrint(const char *prefix, RailPoint *point, const char *suffix);
static void RailUnit_DebugCheck(RailUnit *unit);
static int RailDataHandle_GetPointIndex(const RailDataHandle *data, RailPoint *point);
static int RailDataHandle_GetLineIndex(const RailDataHandle *data, RailLine *line);
static BOOL FieldRailSystem_IntersectSegmentPlane(const VecFx32 *a, const VecFx32 *b, const VecFx32 *p3,
                                                  const VecFx32 *p4, const VecFx32 *p5, VecFx32 *out);

// The step lengths of the speed indices
static u8 sPendingSteps[RAIL_SPEED_MAX] = { 1, 2, 4, 8, 16 };

FieldRailSystem *FieldRailSystem_Create(HeapID heapId, u32 unitCount, FieldCamera *camera) {
    u32 i;
    FieldRailSystem *rail = GFL_HeapAllocate(heapId, sizeof(FieldRailSystem), TRUE, "field_rail.c", 302);

    rail->heapId = heapId;
    rail->loaded = 0;
    rail->cameraEnabled = 1;
    rail->camera = camera;
    rail->cameraParent = NULL;
    rail->units = GFL_HeapAllocate(heapId, unitCount * sizeof(RailUnit), TRUE, "field_rail.c", 311);
    rail->unitCount = unitCount;
    for (i = 0; i < unitCount; i++) {
        RailUnit_SetDataCore(&rail->units[i], &rail->data, rail);
    }
    rail->playerUnit = GFL_HeapAllocate(heapId, sizeof(RailUnit), TRUE, "field_rail.c", 319);
    RailUnit_SetDataCore(rail->playerUnit, &rail->data, rail);
    rail->playerUnit->acquired = 1;
    return rail;
}

void FieldRailSystem_Free(FieldRailSystem *rail) {
    GFL_HeapFree(rail->playerUnit);
    GFL_HeapFree(rail->units);
    GFL_HeapFree(rail);
}

void FieldRailSystem_Load(FieldRailSystem *rail, RailDataHandle *data) {
    rail->loaded = 1;
    RailDisabledLines_Clear(&rail->disabledLines);
    RailDataHandle_Copy(&rail->data, data);
}

void FieldRailSystem_Reset(FieldRailSystem *rail) {
    rail->loaded = 0;
    RailDataHandle_Clear(&rail->data);
    RailDisabledLines_Clear(&rail->disabledLines);
}

BOOL FieldRailSystem_HasData(FieldRailSystem *rail) {
    return rail->data.pointCount != 0;
}

RailUnit *FieldRailSystem_AcquireUnit(FieldRailSystem *rail) {
    RailUnit *unit = NULL;
    s32 i;

    for (i = 0; i < rail->unitCount; i++) {
        if (RailUnit_GetState(&rail->units[i]) == 0) {
            unit = &rail->units[i];
            break;
        }
    }
    unit->acquired = 1;
    return unit;
}

void FieldRailSystem_ReleaseUnit(FieldRailSystem *rail, RailUnit *unit) {
    RailUnit_SetDataCore(unit, &rail->data, rail);
    unit->acquired = 0;
}

void FieldRailSystem_SetCameraParent(FieldRailSystem *rail, RailUnit *unit) {
    rail->cameraParent = unit;
    rail->cameraDirty = 1;
}

void FieldRailSystem_ClearCameraParent(FieldRailSystem *rail) {
    rail->cameraParent = NULL;
}

void FieldRailSystem_GetCameraParentPos(FieldRailSystem *rail, VecFx32 *pos) {
    if (rail->cameraParent != NULL) {
        RailUnit_GetCalcPos(rail->cameraParent, pos);
    }
}

void FieldRailSystem_Update(FieldRailSystem *rail) {
    if (rail->loaded == 0 || rail->cameraEnabled == 0) {
        return;
    }
}

BOOL FieldRailSystem_UpdateCamera(FieldRailSystem *rail, BOOL force) {
    RailUnit *unit = rail->cameraParent;
    RailCameraFunc func = NULL;
    u32 cameraId;

    if (rail->loaded == 0) {
        return FALSE;
    }
    if (rail->cameraEnabled == 0) {
        return FALSE;
    }
    if (unit == NULL) {
        return FALSE;
    }
    if (!RailUnit_IsActive(unit)) {
        return FALSE;
    }
    if (!RailUnit_HasStepped(unit) && rail->cameraDirty == 0 && !force) {
        return FALSE;
    }
    switch (unit->onLine) {
    case RAIL_COMPONENT_POINT:
        cameraId = unit->component.point->cameraId;
        if (cameraId != RAIL_ID_NONE) {
            func = GetRailCameraCurveFunc(unit->handle, GetRailCamera(unit->handle, cameraId)->funcId);
        }
        break;
    case RAIL_COMPONENT_LINE:
        cameraId = unit->component.line->cameraId;
        if (cameraId != RAIL_ID_NONE) {
            func = GetRailCameraCurveFunc(unit->handle, GetRailCamera(unit->handle, cameraId)->funcId);
        }
        break;
    }
    if (func != NULL) {
        func(rail);
    }
    rail->cameraDirty = 0;
    return TRUE;
}

BOOL CalculateRailCurves(FieldRailSystem *rail, const VecFx32 *start, const VecFx32 *end, RailPosition *railPos,
                         VecFx32 *hit) {
    VecFx32 cross;
    VecFx32 diff;
    VecFx32 normal;
    VecFx32 up = { 0, FX32_ONE, 0 };
    VecFx32 bestHit;
    VecFx32 startPos = *start;
    VecFx32 endPos = *end;
    RailPosition bestRailPos;
    s32 i;
    fx32 bestDist;
    BOOL found = FALSE;

    for (i = 0; i < rail->data.lineCount; i++) {
        RailLine *line = GetRailLineIfEnabled(&rail->disabledLines, &rail->data, i);
        RailPoint *pointA;
        RailPoint *pointB;
        RailCurve *curve;
        WorldToRailFunc handler;

        if (line == NULL) {
            continue;
        }
        pointA = GetRailPoint(&rail->data, line->pointA);
        pointB = GetRailPoint(&rail->data, line->pointB);
        curve = GetRailCurve(&rail->data, line->curveId);
        VEC_Subtract(&pointA->pos, &pointB->pos, &diff);
        VEC_CrossProduct(&diff, &up, &normal);
        if (!FieldRailSystem_IntersectSegmentPlane(&diff, &normal, &pointB->pos, &startPos, &endPos, &cross)) {
            continue;
        }
        handler = GetHandler2CurveFunc(&rail->data, curve->handlerId);
        if (handler == NULL) {
            continue;
        }
        if (!handler(i, rail, &cross, railPos, hit)) {
            continue;
        }
        if (!found) {
            bestRailPos = *railPos;
            bestHit = *hit;
            bestDist = VEC_Distance(&startPos, hit);
            found = TRUE;
        } else {
            fx32 dist = VEC_Distance(&startPos, hit);

            if (dist < bestDist) {
                bestRailPos = *railPos;
                bestHit = *hit;
                bestDist = dist;
            }
        }
    }
    if (found) {
        *railPos = bestRailPos;
        *hit = bestHit;
    }
    return found;
}

void FieldRailSystem_SetLineEnabled(FieldRailSystem *rail, u32 lineId, BOOL enabled) {
    if (!enabled) {
        if (!RailDisabledLines_Contains(&rail->disabledLines, lineId)) {
            RailDisabledLines_Add(&rail->disabledLines, lineId);
        }
    } else {
        RailDisabledLines_Remove(&rail->disabledLines, lineId);
    }
}

fx32 FieldRailSystem_GetUnitScale(FieldRailSystem *rail) {
    return FX_Mul(rail->data.tileWidth, 16 * FX32_ONE);
}

void FieldRailSystem_NormalizePos(FieldRailSystem *rail, RailDataHandle *data, u16 componentId, u16 front, u16 side,
                                  RailPosition *pos) {
    RailUnit *unit = rail->playerUnit;

    unit->handle = data;
    pos->componentId = componentId;
    pos->componentIsLine = 1;
    pos->posSide = 0;
    pos->posFront = front;
    RailUnit_SetRailPos(unit, pos);
    pos->posSide = side - unit->width / 16;
    unit->handle = &rail->data;
}

BOOL FieldRailSystem_StepLocation(FieldRailSystem *rail, const RailPosition *pos, u32 arg, VecFx32 *calcPos) {
    RailUnit_SetRailPos(rail->playerUnit, pos);
    RailUnit_RequestMove(rail->playerUnit, RAIL_SPEED_16, arg);
    RailUnit_UpdateMove(rail->playerUnit);
    RailUnit_GetCalcPos(rail->playerUnit, calcPos);
    if (GFL_STD_MemCmp(pos, &rail->playerUnit->railPos, sizeof(RailPosition)) == 0) {
        return FALSE;
    }
    return RailUnit_IsRailPosInBounds(rail->playerUnit, &rail->playerUnit->railPos);
}

BOOL FieldRailSystem_TryStepLocation(FieldRailSystem *rail, const RailPosition *railPos, u32 railDir,
                                     RailPosition *next) {
    RailPosition start = *railPos;

    RailUnit_SetRailPos(rail->playerUnit, railPos);
    RailUnit_RequestMove(rail->playerUnit, RAIL_SPEED_16, railDir);
    RailUnit_UpdateMove(rail->playerUnit);
    RailUnit_GetRailPos(rail->playerUnit, next);
    if (GFL_STD_MemCmp(&start, next, sizeof(RailPosition)) == 0) {
        return FALSE;
    }
    return RailUnit_IsRailPosInBounds(rail->playerUnit, next);
}

void FieldRailSystem_GetLocationPos(FieldRailSystem *rail, const RailPosition *railPos, VecFx32 *pos) {
    RailUnit_SetRailPos(rail->playerUnit, railPos);
    RailUnit_GetCalcPos(rail->playerUnit, pos);
}

s32 FieldRailSystem_GetLocationWidthGrid(FieldRailSystem *rail, const RailPosition *pos) {
    RailUnit_SetRailPos(rail->playerUnit, pos);
    return rail->playerUnit->width / 16;
}

void FieldRailSystem_SetPlayerPos(FieldRailSystem *rail, const RailPosition *pos) {
    RailCameraFunc func = NULL;
    RailUnit *prevParent;
    RailUnit *unit;
    u32 cameraId;

    RailUnit_SetRailPos(rail->playerUnit, pos);
    prevParent = rail->cameraParent;
    unit = rail->playerUnit;
    rail->cameraParent = unit;
    switch (unit->onLine) {
    case RAIL_COMPONENT_POINT:
        cameraId = unit->component.point->cameraId;
        if (cameraId != RAIL_ID_NONE) {
            func = GetRailCameraCurveFunc(unit->handle, GetRailCamera(unit->handle, cameraId)->funcId);
        }
        break;
    case RAIL_COMPONENT_LINE:
        cameraId = unit->component.line->cameraId;
        if (cameraId != RAIL_ID_NONE) {
            func = GetRailCameraCurveFunc(unit->handle, GetRailCamera(unit->handle, cameraId)->funcId);
        }
        break;
    }
    if (func != NULL) {
        func(rail);
    }
    rail->cameraParent = prevParent;
}

void RailUnit_GetRailPos(const RailUnit *unit, RailPosition *pos) {
    *pos = unit->railPos;
}

BOOL RailUnit_IsRailPosInBounds(RailUnit *unit, const RailPosition *pos) {
    RailLine *line;
    RailLine *lines;
    s32 front;
    s32 side;
    s32 length;
    s32 width;

    if (pos->componentIsLine == 0) {
        if (pos->componentId >= unit->handle->pointCount) {
            return FALSE;
        }
    } else {
        if (pos->componentId >= unit->handle->lineCount) {
            return FALSE;
        }
        lines = unit->handle->lines;
        line = &lines[pos->componentId];
        front = pos->posFront * 16;
        side = pos->posSide * 16;
        length = GetLineTileLengthWorldSpace(line, unit->tileWidth, unit->handle);
        width = GetLerpedLineWidthByLineYPos(line, front, length, unit->handle);
        if (-width > side || side > width) {
            return FALSE;
        }
        if (front > length || front < 0) {
            return FALSE;
        }
    }
    return TRUE;
}

u8 RailUnit_PredictNextPos(RailUnit *unit, u32 dir, RailPosition *outPos, VecFx32 *outVec) {
    if (!unit->nextCached || dir != unit->nextDir) {
        unit->nextOk = FieldRailSystem_TryStepLocation(unit->rail, &unit->railPos, dir, &unit->nextPos);
        FieldRailSystem_GetLocationPos(unit->rail, &unit->nextPos, &unit->nextVec);
        unit->nextCached = 1;
        unit->nextDir = dir;
    }
    if (outPos != NULL) {
        MI_CpuCopy8(&unit->nextPos, outPos, sizeof(RailPosition));
    }
    if (outVec != NULL) {
        *outVec = unit->nextVec;
    }
    return unit->nextOk;
}

void RailUnit_GetPreviousCalcPos(RailUnit *unit, VecFx32 *pos) {
    *pos = unit->prevCalcPos;
}

void RailUnit_SetRailPos(RailUnit *unit, const RailPosition *pos) {
    RailLine *line = NULL;
    s32 front;
    s32 side;
    u32 dir;
    s32 length;
    s32 width;
    s32 i;

    RailUnit_SetData(unit, unit->handle);
    if (pos->componentIsLine == 0) {
        RailPoint *point;

        unit->onLine = RAIL_COMPONENT_POINT;
        point = &unit->handle->points[pos->componentId];
        unit->component.point = point;
        unit->width = point->widths[0];
        for (i = 0; i < RAIL_POINT_LINE_MAX; i++) {
            u32 lineId = unit->component.point->lineIds[i];

            if (lineId != RAIL_ID_NONE && line == NULL) {
                line = &unit->handle->lines[lineId];
                if (line->pointA == pos->componentId) {
                    front = 0;
                    side = 0;
                    dir = unit->component.point->dirs[i];
                } else if (line->pointB == pos->componentId) {
                    front = GetLineTileLengthWorldSpace(line, unit->tileWidth, unit->handle);
                    side = 0;
                    dir = unit->component.point->dirs[i];
                }
            }
        }
    } else {
        u32 posFront;

        line = &unit->handle->lines[pos->componentId];
        posFront = pos->posFront;
        if (posFront >= line->length) {
            posFront--;
        }
        front = posFront * 16;
        side = pos->posSide * 16;
        dir = pos->railDirection;
    }
    length = GetLineTileLengthWorldSpace(line, unit->tileWidth, unit->handle);
    width = GetLerpedLineWidthByLineYPos(line, front, length, unit->handle);
    RailUnit_SetLineInfo(unit, line, dir, front, side, length, width);
    unit->lastMoveSpeedIndex = RAIL_SPEED_2;
    unit->direction = dir;
    unit->positionInitialized = 1;
    RailUnit_GetSpotPos(unit, &unit->calcPos);
    unit->railPos = *pos;
    unit->prevRailPos = *pos;
    unit->prevCalcPos = unit->calcPos;
}

void RailUnit_GetCalcPos(RailUnit *unit, VecFx32 *pos) {
    *pos = unit->calcPos;
}

u8 RailUnit_GetMoveDirection(RailUnit *unit) {
    return unit->direction;
}

u8 RailUnit_IsMovePending(RailUnit *unit) {
    return unit->movePending;
}

static BOOL RailUnit_HasStepped(RailUnit *unit) {
    return unit->stepped;
}

void RailUnit_GetStepVector(RailUnit *unit, VecFx16 *dir) {
    s32 savedSide;
    s32 savedFront;
    s32 length;
    s32 nextFront;
    RailPosition tmp;
    VecFx32 from;
    VecFx32 to;

    savedSide = unit->side;
    unit->side = 0;
    RailUnit_GetSpotPos(unit, &from);
    unit->side = savedSide;
    savedFront = unit->front;
    length = unit->length;
    nextFront = savedFront + 16;
    if (nextFront < length) {
        unit->front += 16;
        unit->side = 0;
        RailUnit_GetSpotPos(unit, &to);
        unit->side = savedSide;
        unit->front = savedFront;
    } else {
        RailUnit_GetLocationFromSpot(unit, &tmp);
        tmp.posSide = 0;
        FieldRailSystem_ProbeStep(unit->rail, &tmp, unit->component.line->dir, nextFront - length, &to);
    }
    VEC_Subtract(&to, &from, &to);
    VEC_Normalize(&to, &to);
    VEC_Fx16Set(dir, to.x, to.y, to.z);
}

u32 RailUnit_GetLineDirection(RailUnit *unit) {
    return unit->component.line->dir;
}

BOOL RailUnit_RequestMove(RailUnit *unit, u32 speedIndex, u32 dir) {
    if (unit->movePending == 0) {
        unit->movePending = 1;
        unit->moveProgress = 0;
        unit->pendingDir = dir;
        unit->pendingSpeedIndex = speedIndex;
        return TRUE;
    }
    return FALSE;
}

void RailUnit_UpdateMove(RailUnit *unit) {
    u32 resultDir;
    u16 onLine;
    u8 step;
    u16 dir;
    s32 rem;

    if (unit->rail->loaded == 0 || unit->positionInitialized == 0 || !RailUnit_IsActive(unit)) {
        return;
    }
    resultDir = RAIL_DIR_NONE;
    unit->stepped = FALSE;
    if (unit->movePending == 0) {
        return;
    }
    dir = unit->pendingDir;
    onLine = unit->onLine;
    step = sPendingSteps[unit->pendingSpeedIndex];
    unit->moveProgress += step;
    if (onLine == RAIL_COMPONENT_LINE) {
        resultDir = RailUnit_MoveOnLine(unit, dir, step);
    }
    if (resultDir != RAIL_DIR_NONE) {
        unit->direction = resultDir;
        unit->lastMoveSpeedIndex = unit->pendingSpeedIndex;
        unit->stepped = TRUE;
        RailUnit_GetSpotPos(unit, &unit->calcPos);
    }
    if (onLine != unit->onLine) {
        RailUnit_DebugCheck(unit);
    }
    if (unit->moveProgress < 16 && resultDir != RAIL_DIR_NONE) {
        return;
    }
    unit->movePending = 0;
    unit->nextCached = 0;
    unit->prevRailPos = unit->railPos;
    RailUnit_GetLocationFromSpot(unit, &unit->railPos);
    unit->prevCalcPos = unit->calcPos;
    rem = unit->front % 16;
    if (rem == 0) {
        return;
    }
    if (rem > 8) {
        unit->front += 16 - rem;
    } else {
        unit->front -= rem;
    }
}

FieldCamera *FieldRailSystem_GetFieldCamera(FieldRailSystem *rail) {
    return rail->camera;
}

RailUnit *FieldRailSystem_GetCameraParentUnit(FieldRailSystem *rail) {
    return rail->cameraParent;
}

RailPoint *FieldRailSystem_GetPoint(FieldRailSystem *rail, u32 id) {
    return GetRailPoint(&rail->data, id);
}

RailLine *FieldRailSystem_GetLine(FieldRailSystem *rail, u32 id) {
    return GetRailLine(&rail->data, id);
}

RailCurve *GetRailHnd2Curve(FieldRailSystem *rail, u32 id) {
    return GetRailCurve(&rail->data, id);
}

fx32 FieldRailSystem_GetTileWidth(FieldRailSystem *rail) {
    return rail->data.tileWidth;
}

RailUnit *FieldRailSystem_GetProbeWork(FieldRailSystem *rail) {
    return rail->playerUnit;
}

void FieldRailSystem_ClampLocation(const FieldRailSystem *rail, const RailPosition *in, RailPosition *out) {
    RailPosition pos = *in;
    RailLine *lines = rail->data.lines;
    RailLine *line;
    s32 front;
    s32 side;
    s32 length;
    s32 width;

    *out = pos;
    line = &lines[pos.componentId];
    front = pos.posFront * 16;
    side = pos.posSide * 16;
    length = GetLineTileLengthWorldSpace(line, rail->data.tileWidth, &rail->data);
    width = GetLerpedLineWidthByLineYPos(line, front, length, &rail->data);
    if (front >= length) {
        out->posFront = (length - 1) / 16;
    }
    if (MATH_ABS(side) > width) {
        if (side > 0) {
            out->posSide = width / 16;
        } else {
            out->posSide = -width / 16;
        }
    }
}

u16 RailUnit_IsOnLine(RailUnit *unit) {
    return unit->onLine;
}

RailLine *GetCurrentLine(RailUnit *unit) {
    return unit->component.line;
}

s32 GetCurrentLinePosV(RailUnit *unit) {
    return unit->front;
}

s32 GetCurrentLineLength(RailUnit *unit) {
    return unit->length;
}

s32 GetCurrentLinePosH(RailUnit *unit) {
    return unit->side;
}

s32 RailUnit_GetWidthLimit(RailUnit *unit) {
    return unit->width;
}

fx32 RailUnit_GetTileWidth(RailUnit *unit) {
    return unit->tileWidth;
}

RailCamera *GetRailCameraByCamHandler(RailUnit *unit) {
    return GetRailCameraByHandlerImpl(unit);
}

void RailUnit_GetCentrePos(RailUnit *unit, VecFx32 *pos) {
    s32 saved = unit->side;

    unit->side = 0;
    RailUnit_GetSpotPos(unit, pos);
    unit->side = saved;
}

void RailUnit_GetPosAtLineOffset(RailUnit *unit, s32 front, VecFx32 *pos) {
    s32 saved = unit->front;

    unit->front = front;
    RailUnit_GetSpotPos(unit, pos);
    unit->front = saved;
}

RailPoint *GetCurrentRailLinePoint1(RailUnit *unit) {
    return GetRailPoint(unit->handle, unit->component.line->pointA);
}

RailPoint *GetCurrentRailLinePoint2(RailUnit *unit) {
    return GetRailPoint(unit->handle, unit->component.line->pointB);
}

RailCurve *RailUnit_GetCurve(RailUnit *unit) {
    return GetRailCurve(unit->handle, unit->component.line->curveId);
}

RailCamera *GetRailCameraByPoint(RailUnit *unit, RailPoint *point) {
    return GetRailCamera(unit->handle, point->cameraId);
}

s32 FieldRailSystem_RoundToGrid(s32 value) {
    u32 magnitude = MATH_ABS(value);

    if ((magnitude & 0xf) >= 8) {
        if (value >= 0) {
            return value / 16 + 1;
        }
        return value / 16 - 1;
    }
    return value / 16;
}

BOOL FieldRailSystem_IsPosNear(const VecFx32 *a, const VecFx32 *b, fx32 dist) {
    s32 dx = (a->x >> 12) - (b->x >> 12);
    s32 dz = (a->z >> 12) - (b->z >> 12);
    s32 distSq = dx * dx + dz * dz;
    s32 dy = MATH_ABS(a->y - b->y);
    s32 radius;
    s32 radiusSq;

    radius = (dist >> 12) * 2;
    radiusSq = radius * radius;
    if (dy < 0x8000 && distSq < radiusSq) {
        return TRUE;
    }
    return FALSE;
}

void SetWkToActorRailPos(const RailUnit *unit, u16 *lineIndex, u16 *front, u16 *side) {
    s32 width = GetLerpedLineWidthByLineYPos(unit->component.line, 0, unit->length, unit->handle);
    s32 endWidth = GetLerpedLineWidthByLineYPos(unit->component.line, unit->length, unit->length, unit->handle);
    s32 index;
    s32 i;

    if (width < endWidth) {
        width = endWidth;
    }
    index = 0;
    for (i = 0; i < unit->handle->lineCount; i++) {
        if (unit->component.line == &unit->handle->lines[i]) {
            index = i;
        }
    }
    *lineIndex = index;
    *front = unit->front / 16;
    *side = unit->side / 16 + width / 16;
}

void RailUnit_SetLineLocation(RailUnit *unit, u16 lineIndex, u16 front, u16 side) {
    RailPosition pos;

    RailUnit_GetRailPos(unit, &pos);
    pos.componentId = lineIndex;
    pos.posSide = 0;
    pos.posFront = front;
    RailUnit_SetRailPos(unit, &pos);
    pos.posSide = side - unit->width / 16;
    RailUnit_GetSpotPos(unit, &unit->calcPos);
    unit->railPos = pos;
    unit->prevRailPos = pos;
    unit->prevCalcPos = unit->calcPos;
}

static RailLine *GetLineContinueInDirection(const RailPoint *point, u32 dir, const RailDataHandle *data,
                                            RailDisabledLines *disabled, BOOL checkDisabled) {
    s32 i;

    for (i = 0; i < RAIL_POINT_LINE_MAX; i++) {
        if (dir == point->dirs[i]) {
            if (checkDisabled) {
                return GetRailLineIfEnabled(disabled, data, point->lineIds[i]);
            }
            return GetRailLine(data, point->lineIds[i]);
        }
    }
    return NULL;
}

static u32 RailUnit_TryEnterLine(RailUnit *unit, RailLine *line, u32 dir, s32 front, s32 side, s32 length, s32 width) {
    RailLine *next;
    RailPoint *point;

    if (!RailDisabledLines_AllowsPosition(unit->disabled, line, front, side, unit->handle, unit->tileWidth)) {
        return RAIL_DIR_NONE;
    }
    if (MATH_ABS(side) > width) {
        return RAIL_DIR_NONE;
    }
    if ((front >= length || front < 0)) {
        if (front >= length) {
            point = GetRailPoint(unit->handle, line->pointB);
            next = GetLineContinueInDirection(point, line->dir, unit->handle, unit->disabled, TRUE);
        } else {
            point = GetRailPoint(unit->handle, line->pointA);
            next = GetLineContinueInDirection(point, RailDirection_GetOpposite(line->dir), unit->handle, unit->disabled,
                                              TRUE);
        }
        if (next == NULL) {
            if (front > length) {
                return RAIL_DIR_NONE;
            }
        } else {
            line = next;
            if (front >= length) {
                front -= length;
                length = GetLineTileLengthWorldSpace(line, unit->tileWidth, unit->handle);
                width = GetLerpedLineWidthByLineYPos(line, front, length, unit->handle);
            } else if (front < 0) {
                length = GetLineTileLengthWorldSpace(line, unit->tileWidth, unit->handle);
                front += length;
                width = GetLerpedLineWidthByLineYPos(line, front, length, unit->handle);
            }
            if (front >= length) {
                return RAIL_DIR_NONE;
            }
            if (MATH_ABS(side) > width) {
                return RAIL_DIR_NONE;
            }
        }
    }
    RailUnit_SetLineInfo(unit, line, dir, front, side, length, width);
    RailLine_CheckEndpointDirections(line, unit->handle);
    return dir;
}

static void RailUnit_SetLineInfo(RailUnit *unit, RailLine *line, u32 dir, s32 front, s32 side, s32 length, s32 width) {
    unit->onLine = RAIL_COMPONENT_LINE;
    unit->component.line = line;
    unit->side = side;
    unit->length = length;
    unit->dir = dir;
    unit->front = front;
    unit->width = width;
}

static BOOL RailLine_IsPointA(const RailDataHandle *data, RailLine *line, RailPoint *point) {
    if (GetRailPoint(data, line->pointA) == point) {
        return TRUE;
    }
    return FALSE;
}

static BOOL RailLine_IsPointB(const RailDataHandle *data, RailLine *line, RailPoint *point) {
    if (GetRailPoint(data, line->pointB) == point) {
        return TRUE;
    }
    return FALSE;
}

static void SetupLineWidthForLineChange(const RailDataHandle *data, const RailLine *line, s32 *outNext, s32 *outCur,
                                        RailDisabledLines *disabled, fx32 tileWidth) {
    RailPoint *point;
    s32 nextWidth;
    RailLine *l1;
    RailLine *l2;
    RailLine *l3;
    s32 length;
    s32 curWidth;
    s32 w;
    s32 len;

    nextWidth = 0;
    point = GetRailPoint(data, line->pointA);
    l1 = GetLineContinueInDirection(point, RailDirection_GetOpposite(line->dir), data, disabled, TRUE);
    l2 = GetLineContinueInDirection(point, RailDirection_TurnLeft(line->dir), data, disabled, TRUE);
    l3 = GetLineContinueInDirection(point, RailDirection_TurnRight(line->dir), data, disabled, TRUE);
    length = GetLineTileLengthWorldSpace(line, tileWidth, data);
    curWidth = GetLerpedLineWidthByLineYPos(line, 0, length, data);

    if (l1 != NULL) {
        len = GetLineTileLengthWorldSpace(l1, tileWidth, data);
        w = GetLerpedLineWidthByLineYPos(l1, len, len, data);
        if (w > curWidth) {
            curWidth = w;
        }
    }
    if (l2 != NULL) {
        len = GetLineTileLengthWorldSpace(l2, tileWidth, data);
        if (RailLine_IsPointA(data, l2, point)) {
            w = GetLerpedLineWidthByLineYPos(l2, 0, len, data);
        } else {
            w = GetLerpedLineWidthByLineYPos(l2, len, len, data);
        }
        if (w > 0) {
            nextWidth = w;
        }
    }
    if (l3 != NULL) {
        len = GetLineTileLengthWorldSpace(l3, tileWidth, data);
        if (RailLine_IsPointA(data, l3, point)) {
            w = GetLerpedLineWidthByLineYPos(l3, 0, len, data);
        } else {
            w = GetLerpedLineWidthByLineYPos(l3, len, len, data);
        }
        if (w > nextWidth) {
            nextWidth = w;
        }
    }
    *outNext = nextWidth;
    *outCur = curWidth;
}

static void RailDataHandle_Clear(RailDataHandle *data) {
    MI_CpuFill8(data, 0, sizeof(RailDataHandle));
}

static void RailDataHandle_Copy(RailDataHandle *data, RailDataHandle *src) {
    MI_CpuCopy8(src, data, sizeof(RailDataHandle));
}

static RailPoint *GetRailPoint(const RailDataHandle *data, u32 id) {
    if (data->pointCount > id) {
        return &data->points[id];
    }
    if (id == RAIL_ID_NONE) {
        return NULL;
    }
    return data->points;
}

static RailLine *GetRailLine(const RailDataHandle *data, u32 id) {
    if (data->lineCount > id) {
        return &data->lines[id];
    }
    if (id == RAIL_ID_NONE) {
        return NULL;
    }
    return data->lines;
}

static const RailCamera DEFAULT_RAIL_CAMERA = { RAIL_ID_NONE };

static RailCamera *GetRailCamera(const RailDataHandle *data, u32 id) {
    if (data->cameraCount > id) {
        return &data->cameras[id];
    }
    if (id == RAIL_ID_NONE) {
        return NULL;
    }
    return (RailCamera *)&DEFAULT_RAIL_CAMERA;
}

static const RailCurve DEFAULT_RAIL_CURVE = { RAIL_ID_NONE, { 0xff, 0, 0 } };

static RailCurve *GetRailCurve(const RailDataHandle *data, u32 id) {
    if (data->curveCount > id) {
        return &data->curves[id];
    }
    if (id == RAIL_ID_NONE) {
        return NULL;
    }
    return (RailCurve *)&DEFAULT_RAIL_CURVE;
}

static RailCameraFunc GetRailCameraCurveFunc(const RailDataHandle *data, u32 id) {
    if (data->camFuncCount > id) {
        return data->camFuncs[id];
    }
    if (id == RAIL_ID_NONE) {
        return NULL;
    }
    return data->camFuncs[0];
}

static RailToWorldFunc GetHandler1CurveFunc(const RailDataHandle *data, u32 id) {
    if (data->railToWorldCount > id) {
        return data->railToWorldFuncs[id];
    }
    if (id == RAIL_ID_NONE) {
        return NULL;
    }
    return data->railToWorldFuncs[0];
}

static WorldToRailFunc GetHandler2CurveFunc(const RailDataHandle *data, u32 id) {
    if (data->worldToRailCount > id) {
        return data->worldToRailFuncs[id];
    }
    if (id == RAIL_ID_NONE) {
        return NULL;
    }
    return data->worldToRailFuncs[0];
}

// Moves the unit step sixteenths of a tile along or across its line, onto a line that continues from one of the
// line's points where it reaches one; the result is the direction moved, or RAIL_DIR_NONE when the unit can't move
static u32 RailUnit_MoveOnLine(RailUnit *unit, u32 dir, s32 step) {
    RailLine *line = unit->component.line;
    s32 length = unit->length;
    s32 nextLength;

    if (dir == line->dir) {
        // Along the line toward point B, then onto the line ahead there or one to the left or right
        RailPoint *point;
        RailLine *ahead;
        RailLine *left;
        RailLine *right;
        BOOL outsideWidth;
        s32 width;
        u32 resultDir;

        point = GetRailPoint(unit->handle, line->pointB);
        outsideWidth = FALSE;
        ahead = GetLineContinueInDirection(point, dir, unit->handle, unit->disabled, FALSE);
        left = GetLineContinueInDirection(point, RailDirection_TurnLeft(dir), unit->handle, unit->disabled, FALSE);
        right = GetLineContinueInDirection(point, RailDirection_TurnRight(dir), unit->handle, unit->disabled, FALSE);
        RailUnit_ApplyLineStep(unit->disabled, unit, step, 0, dir);
        if (unit->front < length) {
            width = GetLerpedLineWidthByLineYPos(line, unit->front, length, unit->handle);
            if (width >= MATH_ABS(unit->side)) {
                return dir;
            }
            outsideWidth = TRUE;
        }
        if (ahead != NULL && !outsideWidth) {
            RailPoint_CheckConnections(point, unit->handle);
            nextLength = GetLineTileLengthWorldSpace(ahead, unit->tileWidth, unit->handle);
            width = GetLerpedLineWidthByLineYPos(ahead, unit->front - length, nextLength, unit->handle);
            resultDir = RailUnit_TryEnterLine(unit, ahead, dir, unit->front - length, unit->side, nextLength, width);
            if (resultDir != RAIL_DIR_NONE) {
                return resultDir;
            }
        }
        if (left != NULL && unit->side <= 0) {
            if (RailLine_IsPointA(unit->handle, left, point)) {
                RailPoint_CheckConnections(point, unit->handle);
                nextLength = GetLineTileLengthWorldSpace(left, unit->tileWidth, unit->handle);
                width = GetLerpedLineWidthByLineYPos(left, -unit->side, nextLength, unit->handle);
                resultDir =
                    RailUnit_TryEnterLine(unit, left, dir, -unit->side, unit->front - length, nextLength, width);
                if (resultDir != RAIL_DIR_NONE) {
                    return resultDir;
                }
            } else if (RailLine_IsPointB(unit->handle, left, point)) {
                RailPoint_CheckConnections(point, unit->handle);
                nextLength = GetLineTileLengthWorldSpace(left, unit->tileWidth, unit->handle);
                width = GetLerpedLineWidthByLineYPos(left, nextLength + unit->side, nextLength, unit->handle);
                resultDir = RailUnit_TryEnterLine(unit, left, dir, nextLength + unit->side, -(unit->front - length),
                                                  nextLength, width);
                if (resultDir != RAIL_DIR_NONE) {
                    return resultDir;
                }
            }
        }
        if (right != NULL && unit->side >= 0) {
            if (RailLine_IsPointA(unit->handle, right, point)) {
                RailPoint_CheckConnections(point, unit->handle);
                nextLength = GetLineTileLengthWorldSpace(right, unit->tileWidth, unit->handle);
                width = GetLerpedLineWidthByLineYPos(right, unit->side, nextLength, unit->handle);
                resultDir =
                    RailUnit_TryEnterLine(unit, right, dir, unit->side, -(unit->front - length), nextLength, width);
                if (resultDir != RAIL_DIR_NONE) {
                    return resultDir;
                }
            } else if (RailLine_IsPointB(unit->handle, right, point)) {
                RailPoint_CheckConnections(point, unit->handle);
                nextLength = GetLineTileLengthWorldSpace(right, unit->tileWidth, unit->handle);
                width = GetLerpedLineWidthByLineYPos(right, nextLength - unit->side, nextLength, unit->handle);
                resultDir = RailUnit_TryEnterLine(unit, right, dir, nextLength - unit->side, unit->front - length,
                                                  nextLength, width);
                if (resultDir != RAIL_DIR_NONE) {
                    return resultDir;
                }
            }
        }
        if (outsideWidth) {
            return RailUnit_ClampSideToLineWidth(unit, line, length, dir);
        }
        if (unit->front <= length) {
            return dir;
        }
        unit->front -= step;
        return RAIL_DIR_NONE;
    } else if (dir == RailDirection_GetOpposite(line->dir)) {
        // Back along the line toward point A, the same way
        RailPoint *point;
        RailLine *right;
        RailLine *left;
        RailLine *ahead;
        BOOL outsideWidth;
        s32 width;
        u32 resultDir;

        point = GetRailPoint(unit->handle, line->pointA);
        right = GetLineContinueInDirection(point, RailDirection_TurnRight(dir), unit->handle, unit->disabled, FALSE);
        outsideWidth = FALSE;
        left = GetLineContinueInDirection(point, RailDirection_TurnLeft(dir), unit->handle, unit->disabled, FALSE);
        ahead = GetLineContinueInDirection(point, dir, unit->handle, unit->disabled, FALSE);
        RailUnit_ApplyLineStep(unit->disabled, unit, -step, 0, dir);
        if (unit->front >= 0) {
            width = GetLerpedLineWidthByLineYPos(line, unit->front, length, unit->handle);
            if (width >= MATH_ABS(unit->side)) {
                return dir;
            }
            outsideWidth = TRUE;
        }
        if (ahead != NULL && !outsideWidth) {
            RailPoint_CheckConnections(point, unit->handle);
            nextLength = GetLineTileLengthWorldSpace(ahead, unit->tileWidth, unit->handle);
            width = GetLerpedLineWidthByLineYPos(ahead, nextLength - MATH_ABS(unit->front), nextLength, unit->handle);
            resultDir = RailUnit_TryEnterLine(unit, ahead, dir, nextLength - MATH_ABS(unit->front), unit->side,
                                              nextLength, width);
            if (resultDir != RAIL_DIR_NONE) {
                return resultDir;
            }
        }
        if (right != NULL && unit->side <= 0) {
            if (RailLine_IsPointA(unit->handle, right, point)) {
                RailPoint_CheckConnections(point, unit->handle);
                nextLength = GetLineTileLengthWorldSpace(right, unit->tileWidth, unit->handle);
                width = GetLerpedLineWidthByLineYPos(right, -unit->side, nextLength, unit->handle);
                resultDir = RailUnit_TryEnterLine(unit, right, dir, -unit->side, unit->front, nextLength, width);
                if (resultDir != RAIL_DIR_NONE) {
                    return resultDir;
                }
            } else if (RailLine_IsPointB(unit->handle, right, point)) {
                RailPoint_CheckConnections(point, unit->handle);
                nextLength = GetLineTileLengthWorldSpace(right, unit->tileWidth, unit->handle);
                width = GetLerpedLineWidthByLineYPos(right, nextLength + unit->side, nextLength, unit->handle);
                resultDir =
                    RailUnit_TryEnterLine(unit, right, dir, nextLength + unit->side, -unit->front, nextLength, width);
                if (resultDir != RAIL_DIR_NONE) {
                    return resultDir;
                }
            }
        }
        if (left != NULL && unit->side >= 0) {
            if (RailLine_IsPointA(unit->handle, left, point)) {
                RailPoint_CheckConnections(point, unit->handle);
                nextLength = GetLineTileLengthWorldSpace(left, unit->tileWidth, unit->handle);
                width = GetLerpedLineWidthByLineYPos(left, unit->side, nextLength, unit->handle);
                resultDir = RailUnit_TryEnterLine(unit, left, dir, unit->side, -unit->front, nextLength, width);
                if (resultDir != RAIL_DIR_NONE) {
                    return resultDir;
                }
            }
            if (RailLine_IsPointB(unit->handle, left, point)) {
                RailPoint_CheckConnections(point, unit->handle);
                nextLength = GetLineTileLengthWorldSpace(left, unit->tileWidth, unit->handle);
                width = GetLerpedLineWidthByLineYPos(left, nextLength - unit->side, nextLength, unit->handle);
                resultDir =
                    RailUnit_TryEnterLine(unit, left, dir, nextLength - unit->side, unit->front, nextLength, width);
                if (resultDir != RAIL_DIR_NONE) {
                    return resultDir;
                }
            }
        }
        if (outsideWidth) {
            return RailUnit_ClampSideToLineWidth(unit, line, length, dir);
        }
        unit->front += step;
        return RAIL_DIR_NONE;
    } else if (dir == RailDirection_TurnRight(line->dir)) {
        // To the right of the line: at each whole tile, onto a line that leaves point A or point B in dir
        RailPoint *pointA;
        RailPoint *pointB;
        RailLine *next;
        s32 width;
        u32 resultDir;

        RailUnit_ApplyLineStep(unit->disabled, unit, 0, step, dir);
        if (unit->side > 0 && unit->side % 16 == 0) {
            pointA = GetRailPoint(unit->handle, line->pointA);
            next = GetLineContinueInDirection(pointA, dir, unit->handle, unit->disabled, FALSE);
            if (next != NULL) {
                if (RailLine_IsPointA(unit->handle, next, pointA)) {
                    RailPoint_CheckConnections(pointA, unit->handle);
                    nextLength = GetLineTileLengthWorldSpace(next, unit->tileWidth, unit->handle);
                    width = GetLerpedLineWidthByLineYPos(next, unit->side, nextLength, unit->handle);
                    resultDir = RailUnit_TryEnterLine(unit, next, dir, unit->side, -unit->front, nextLength, width);
                    if (resultDir != RAIL_DIR_NONE) {
                        return resultDir;
                    }
                }
                if (RailLine_IsPointB(unit->handle, next, pointA)) {
                    RailPoint_CheckConnections(pointA, unit->handle);
                    nextLength = GetLineTileLengthWorldSpace(next, unit->tileWidth, unit->handle);
                    width = GetLerpedLineWidthByLineYPos(next, nextLength - unit->side, nextLength, unit->handle);
                    resultDir =
                        RailUnit_TryEnterLine(unit, next, dir, nextLength - unit->side, unit->front, nextLength, width);
                    if (resultDir != RAIL_DIR_NONE) {
                        return resultDir;
                    }
                }
            }
            pointB = GetRailPoint(unit->handle, line->pointB);
            next = GetLineContinueInDirection(pointB, dir, unit->handle, unit->disabled, FALSE);
            if (next != NULL) {
                if (RailLine_IsPointA(unit->handle, next, pointB)) {
                    RailPoint_CheckConnections(pointB, unit->handle);
                    nextLength = GetLineTileLengthWorldSpace(next, unit->tileWidth, unit->handle);
                    width = GetLerpedLineWidthByLineYPos(next, unit->side, nextLength, unit->handle);
                    resultDir =
                        RailUnit_TryEnterLine(unit, next, dir, unit->side, length - unit->front, nextLength, width);
                    if (resultDir != RAIL_DIR_NONE) {
                        return resultDir;
                    }
                }
                if (RailLine_IsPointB(unit->handle, next, pointB)) {
                    RailPoint_CheckConnections(pointB, unit->handle);
                    nextLength = GetLineTileLengthWorldSpace(next, unit->tileWidth, unit->handle);
                    width = GetLerpedLineWidthByLineYPos(next, nextLength - unit->side, nextLength, unit->handle);
                    resultDir = RailUnit_TryEnterLine(unit, next, dir, nextLength - unit->side, -(length - unit->front),
                                                      nextLength, width);
                    if (resultDir != RAIL_DIR_NONE) {
                        return resultDir;
                    }
                }
            }
        }
        if (unit->side > 0 && unit->side <= unit->width) {
            return RailUnit_ClampSideToLineWidth(unit, line, length, dir);
        }
        return dir;
    } else if (dir == RailDirection_TurnLeft(line->dir)) {
        // To the left of the line, the same way
        RailPoint *point;
        RailLine *nextA;
        RailLine *nextB;
        s32 width;
        u32 resultDir;

        RailUnit_ApplyLineStep(unit->disabled, unit, 0, -step, dir);
        if (unit->side < 0 && MATH_ABS(unit->side) % 16 == 0) {
            point = GetRailPoint(unit->handle, line->pointA);
            nextA = GetLineContinueInDirection(point, dir, unit->handle, unit->disabled, FALSE);
            if (nextA != NULL) {
                if (RailLine_IsPointA(unit->handle, nextA, point)) {
                    RailPoint_CheckConnections(point, unit->handle);
                    nextLength = GetLineTileLengthWorldSpace(nextA, unit->tileWidth, unit->handle);
                    width = GetLerpedLineWidthByLineYPos(nextA, MATH_ABS(unit->side), nextLength, unit->handle);
                    resultDir =
                        RailUnit_TryEnterLine(unit, nextA, dir, MATH_ABS(unit->side), unit->front, nextLength, width);
                    if (resultDir != RAIL_DIR_NONE) {
                        return resultDir;
                    }
                }
                if (RailLine_IsPointB(unit->handle, nextA, point)) {
                    RailPoint_CheckConnections(point, unit->handle);
                    nextLength = GetLineTileLengthWorldSpace(nextA, unit->tileWidth, unit->handle);
                    width = GetLerpedLineWidthByLineYPos(nextA, nextLength - MATH_ABS(unit->side), nextLength,
                                                         unit->handle);
                    resultDir = RailUnit_TryEnterLine(unit, nextA, dir, nextLength - MATH_ABS(unit->side), -unit->front,
                                                      nextLength, width);
                    if (resultDir != RAIL_DIR_NONE) {
                        return resultDir;
                    }
                }
            }
            point = GetRailPoint(unit->handle, line->pointB);
            nextB = GetLineContinueInDirection(point, dir, unit->handle, unit->disabled, FALSE);
            if (nextB != NULL) {
                if (RailLine_IsPointA(unit->handle, nextB, point)) {
                    RailPoint_CheckConnections(point, unit->handle);
                    nextLength = GetLineTileLengthWorldSpace(nextB, unit->tileWidth, unit->handle);
                    width = GetLerpedLineWidthByLineYPos(nextB, MATH_ABS(unit->side), nextLength, unit->handle);
                    resultDir = RailUnit_TryEnterLine(unit, nextB, dir, MATH_ABS(unit->side), -(length - unit->front),
                                                      nextLength, width);
                    if (resultDir != RAIL_DIR_NONE) {
                        return resultDir;
                    }
                }
                if (RailLine_IsPointB(unit->handle, nextB, point)) {
                    RailPoint_CheckConnections(point, unit->handle);
                    nextLength = GetLineTileLengthWorldSpace(nextB, unit->tileWidth, unit->handle);
                    width = GetLerpedLineWidthByLineYPos(nextB, nextLength - MATH_ABS(unit->side), nextLength,
                                                         unit->handle);
                    resultDir = RailUnit_TryEnterLine(unit, nextB, dir, nextLength - MATH_ABS(unit->side),
                                                      length - unit->front, nextLength, width);
                    if (resultDir != RAIL_DIR_NONE) {
                        return resultDir;
                    }
                }
            }
        }
        if (unit->side < 0 && MATH_ABS(unit->side) <= unit->width) {
            return RailUnit_ClampSideToLineWidth(unit, line, length, dir);
        }
        return dir;
    }
    return RAIL_DIR_NONE;
}

static void RailUnit_SetDataCore(RailUnit *unit, const RailDataHandle *data, FieldRailSystem *rail) {
    unit->handle = data;
    unit->onLine = RAIL_COMPONENT_NONE;
    unit->tileWidth = data->tileWidth;
    unit->lastMoveSpeedIndex = RAIL_SPEED_1;
    unit->direction = RAIL_DIR_NONE;
    unit->movePending = 0;
    unit->rail = rail;
    unit->component.point = NULL;
    unit->front = 0;
    unit->side = 0;
    unit->dir = RAIL_DIR_NONE;
    unit->disabled = &rail->disabledLines;
    unit->nextCached = 0;
    unit->positionInitialized = 0;
}

static void RailUnit_SetData(RailUnit *unit, const RailDataHandle *data) {
    RailUnit_SetDataCore(unit, data, unit->rail);
}

static u32 RailUnit_GetState(RailUnit *unit) {
    return unit->acquired;
}

static BOOL RailUnit_IsActive(RailUnit *unit) {
    if (unit->onLine < RAIL_COMPONENT_NONE && unit->component.point != NULL && unit->acquired != 0) {
        return TRUE;
    }
    return FALSE;
}

static RailCamera *GetRailCameraByHandlerImpl(RailUnit *unit) {
    switch (unit->onLine) {
    case RAIL_COMPONENT_POINT:
        return GetRailCamera(unit->handle, unit->component.point->cameraId);
    case RAIL_COMPONENT_LINE:
        return GetRailCamera(unit->handle, unit->component.line->cameraId);
    }
    return (RailCamera *)&DEFAULT_RAIL_CAMERA;
}

static void RailUnit_GetSpotPos(RailUnit *unit, VecFx32 *pos) {
    if (unit->onLine == RAIL_COMPONENT_POINT) {
        *pos = unit->component.point->pos;
    } else if (unit->onLine == RAIL_COMPONENT_LINE) {
        RailCurve *curve = GetRailCurve(unit->handle, unit->component.line->curveId);
        RailToWorldFunc func = GetHandler1CurveFunc(unit->handle, curve->handlerId);

        if (func != NULL) {
            func(unit, pos);
        }
    }
}

static void RailUnit_GetLocationFromSpot(const RailUnit *unit, RailPosition *pos) {
    u16 onLine = unit->onLine;
    s32 id;

    pos->componentIsLine = onLine;
    if (onLine == RAIL_COMPONENT_POINT) {
        id = RailDataHandle_GetPointIndex(unit->handle, unit->component.point);
    } else {
        id = RailDataHandle_GetLineIndex(unit->handle, unit->component.line);
    }
    pos->componentId = id;
    pos->posFront = unit->front / 16;
    pos->posSide = unit->side / 16;
    pos->railDirection = unit->dir;
}

static s32 GetLineTileLengthWorldSpace(const RailLine *line, fx32 tileWidth, const RailDataHandle *data) {
    return line->length << 4;
}

static s32 GetLerpedLineWidthByLineYPos(const RailLine *line, s32 y, s32 length, const RailDataHandle *data) {
    RailPoint *pointA = &data->points[line->pointA];
    RailPoint *pointB = &data->points[line->pointB];
    s32 widthA;
    s32 widthB;
    s32 diff;
    s32 i;

    widthB = -1;
    widthA = widthB;
    for (i = 0; i < RAIL_POINT_LINE_MAX; i++) {
        if (pointA->lineIds[i] != RAIL_ID_NONE && line == &data->lines[pointA->lineIds[i]]) {
            widthA = pointA->widths[i];
        }
        if (pointB->lineIds[i] != RAIL_ID_NONE && line == &data->lines[pointB->lineIds[i]]) {
            widthB = pointB->widths[i];
        }
    }
    if (widthA < 0) {
        widthA = widthB;
    }
    if (widthB < 0) {
        widthB = widthA;
    }
    if ((u32)y > (u32)(length - 16)) {
        y = length - 16;
    }
    diff = widthB - widthA;
    length -= 16;
    if (length != 0) {
        diff = diff * y / length;
    }
    return diff + widthA;
}

// Refreshes the width, clamps the lateral position and preserves the requested direction.
static u32 RailUnit_ClampSideToLineWidth(RailUnit *unit, RailLine *line, s32 length, u32 dir) {
    s32 width = GetLerpedLineWidthByLineYPos(line, unit->front, length, unit->handle);
    s32 side = unit->side;

    unit->width = width;
    if (MATH_ABS(side) >= width) {
        if (side > 0) {
            unit->side = width;
        } else {
            unit->side = -width;
        }
    }
    return dir;
}

static BOOL FieldRailSystem_ProbeStep(FieldRailSystem *rail, RailPosition *pos, u32 dir, s32 extra, VecFx32 *out) {
    s32 i;

    RailUnit_SetRailPos(rail->playerUnit, pos);
    RailUnit_RequestMove(rail->playerUnit, RAIL_SPEED_16, dir);
    RailUnit_UpdateMove(rail->playerUnit);
    RailUnit_RequestMove(rail->playerUnit, RAIL_SPEED_1, dir);
    for (i = 0; i < 16; i++) {
        if (extra == i) {
            RailUnit_GetCalcPos(rail->playerUnit, out);
        }
        RailUnit_UpdateMove(rail->playerUnit);
    }
    if (GFL_STD_MemCmp(pos, &rail->playerUnit->railPos, sizeof(RailPosition)) == 0) {
        return FALSE;
    }
    return RailUnit_IsRailPosInBounds(rail->playerUnit, &rail->playerUnit->railPos);
}

static u32 RailDirection_GetOpposite(u32 dir) {
    switch (dir) {
    case RAIL_DIR_UP:
        return RAIL_DIR_DOWN;
    case RAIL_DIR_DOWN:
        return RAIL_DIR_UP;
    case RAIL_DIR_LEFT:
        return RAIL_DIR_RIGHT;
    case RAIL_DIR_RIGHT:
        return RAIL_DIR_LEFT;
    }
    return RAIL_DIR_NONE;
}

static u32 RailDirection_TurnRight(u32 dir) {
    switch (dir) {
    case RAIL_DIR_UP:
        return RAIL_DIR_RIGHT;
    case RAIL_DIR_DOWN:
        return RAIL_DIR_LEFT;
    case RAIL_DIR_LEFT:
        return RAIL_DIR_UP;
    case RAIL_DIR_RIGHT:
        return RAIL_DIR_DOWN;
    }
    return RAIL_DIR_NONE;
}

static u32 RailDirection_TurnLeft(u32 dir) {
    switch (dir) {
    case RAIL_DIR_UP:
        return RAIL_DIR_LEFT;
    case RAIL_DIR_DOWN:
        return RAIL_DIR_RIGHT;
    case RAIL_DIR_LEFT:
        return RAIL_DIR_DOWN;
    case RAIL_DIR_RIGHT:
        return RAIL_DIR_UP;
    }
    return RAIL_DIR_NONE;
}

static void RailDisabledLines_Clear(RailDisabledLines *disabled) {
    s32 i;

    for (i = 0; i < RAIL_DISABLED_LINE_MAX; i++) {
        disabled->lineIds[i] = RAIL_ID_NONE;
    }
}

static void RailDisabledLines_Add(RailDisabledLines *disabled, u16 lineId) {
    s32 i;

    for (i = 0; i < RAIL_DISABLED_LINE_MAX; i++) {
        if (disabled->lineIds[i] == RAIL_ID_NONE) {
            disabled->lineIds[i] = lineId;
            return;
        }
    }
}

static void RailDisabledLines_Remove(RailDisabledLines *disabled, u16 lineId) {
    s32 i;

    for (i = 0; i < RAIL_DISABLED_LINE_MAX; i++) {
        if (disabled->lineIds[i] == lineId) {
            disabled->lineIds[i] = RAIL_ID_NONE;
            return;
        }
    }
}

static BOOL RailDisabledLines_Contains(RailDisabledLines *disabled, u16 lineId) {
    s32 i;

    for (i = 0; i < RAIL_DISABLED_LINE_MAX; i++) {
        if (disabled->lineIds[i] == lineId) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL RailDisabledLines_ContainsLine(RailDisabledLines *disabled, RailLine *line, const RailDataHandle *data) {
    u32 index = 0;
    s32 i;

    for (i = 0; i < data->lineCount; i++) {
        if (line == &data->lines[i]) {
            index = i;
        }
    }
    return RailDisabledLines_Contains(disabled, index);
}

static RailLine *GetRailLineIfEnabled(RailDisabledLines *disabled, const RailDataHandle *data, u32 lineId) {
    if (RailDisabledLines_Contains(disabled, lineId) == 0) {
        return GetRailLine(data, lineId);
    } else {
        return NULL;
    }
}

// Applies rail-space deltas, limiting movement at disabled-line junctions.
static void RailUnit_ApplyLineStep(RailDisabledLines *disabled, RailUnit *unit, s32 frontStep, s32 sideStep, u32 dir) {
    s32 nextWidth;
    s32 curWidth;

    if (RailDisabledLines_ContainsLine(disabled, unit->component.line, unit->handle)) {
        RailPoint *point = GetRailPoint(unit->handle, unit->component.line->pointA);
        RailLine *next = GetLineContinueInDirection(point, dir, unit->handle, disabled, FALSE);

        SetupLineWidthForLineChange(unit->handle, unit->component.line, &nextWidth, &curWidth, disabled,
                                    unit->tileWidth);
        unit->front += frontStep;
        unit->side += sideStep;
        if (unit->front > nextWidth) {
            unit->front = nextWidth;
        }
        if (next == NULL) {
            s32 side = unit->side;

            if (MATH_ABS(side) > curWidth) {
                if (side > 0) {
                    unit->side = curWidth;
                } else {
                    unit->side = -curWidth;
                }
            }
        }
    } else {
        unit->front += frontStep;
        unit->side += sideStep;
    }
}

// Only disabled lines impose these junction bounds; enabled lines pass this check.
static BOOL RailDisabledLines_AllowsPosition(RailDisabledLines *disabled, RailLine *line, s32 front, s32 side,
                                             const RailDataHandle *data, fx32 tileWidth) {
    s32 nextWidth;
    s32 curWidth;

    if (RailDisabledLines_ContainsLine(disabled, line, data)) {
        SetupLineWidthForLineChange(data, line, &nextWidth, &curWidth, disabled, tileWidth);
        if (front <= nextWidth && MATH_ABS(side) <= curWidth) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

static u32 RailPoint_GetLineDirection(RailPoint *point, RailLine *line, const RailDataHandle *data) {
    int i;

    for (i = 0; i < RAIL_POINT_LINE_MAX; i++) {
        if (GetRailLine(data, point->lineIds[i]) == line) {
            return point->dirs[i];
        }
    }
    return RAIL_DIR_NONE;
}

static BOOL RailLine_CheckEndpointDirections(RailLine *line, const RailDataHandle *data) {
    RailPoint *pointA;
    RailPoint *pointB;
    u32 dirA;
    u32 dirB;

    if (line->pointA == RAIL_ID_NONE || line->pointB == RAIL_ID_NONE) {
        return FALSE;
    }
    pointA = GetRailPoint(data, line->pointA);
    pointB = GetRailPoint(data, line->pointB);
    dirA = RailPoint_GetLineDirection(pointA, line, data);
    if (dirA == RAIL_DIR_NONE) {
        return FALSE;
    }
    if (dirA != line->dir) {
        return FALSE;
    }
    dirB = RailPoint_GetLineDirection(pointB, line, data);
    if (dirB == RAIL_DIR_NONE) {
        return FALSE;
    }
    if (dirB == RailDirection_GetOpposite(line->dir)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL RailPoint_CheckConnections(RailPoint *point, const RailDataHandle *data) {
    s32 i;

    for (i = 0; i < RAIL_POINT_LINE_MAX; i++) {
        if ((point->dirs[i] == RAIL_DIR_NONE) != (point->lineIds[i] == RAIL_ID_NONE)) {
            return FALSE;
        }
        if (point->lineIds[i] != RAIL_ID_NONE) {
            RailLine *line = GetRailLine(data, point->lineIds[i]);
            RailPoint *pointA = GetRailPoint(data, line->pointA);
            RailPoint *pointB = GetRailPoint(data, line->pointB);
            s32 j;

            if (pointA != point && pointB != point) {
                return FALSE;
            }
            if (point->dirs[i] != RAIL_DIR_NONE) {
                for (j = i + 1; j < 4; j++) {
                    if (point->dirs[i] == RailDirection_GetOpposite(point->dirs[j]) &&
                        line->dir != GetRailLine(data, point->lineIds[j])->dir) {
                        return FALSE;
                    }
                }
            }
        }
    }
    return TRUE;
}

static void FieldRailSystem_DebugPrintPos(const char *prefix, VecFx32 *pos, const char *suffix) {
}

static void RailPoint_DebugPrint(const char *prefix, RailPoint *point, const char *suffix) {
    if (point != NULL) {
        FieldRailSystem_DebugPrintPos(prefix, &point->pos, suffix);
    }
}

static void RailUnit_DebugCheck(RailUnit *unit) {
    if (unit->onLine == RAIL_COMPONENT_POINT) {
        RailPoint_DebugPrint(NULL, unit->component.point, "\n");
    } else if (unit->onLine == RAIL_COMPONENT_LINE) {
        RailPoint *pointA = GetRailPoint(unit->handle, unit->component.line->pointA);
        RailPoint *pointB = GetRailPoint(unit->handle, unit->component.line->pointB);

        RailPoint_DebugPrint("start", pointA, NULL);
        RailPoint_DebugPrint("-end", pointB, "\n");
    }
}

static int RailDataHandle_GetPointIndex(const RailDataHandle *data, RailPoint *point) {
    int i;

    for (i = 0; i < data->pointCount; i++) {
        if (point == &data->points[i]) {
            return i;
        }
    }
    return 0;
}

static int RailDataHandle_GetLineIndex(const RailDataHandle *data, RailLine *line) {
    int i;

    for (i = 0; i < data->lineCount; i++) {
        if (line == &data->lines[i]) {
            return i;
        }
    }
    return 0;
}

static BOOL FieldRailSystem_IntersectSegmentPlane(const VecFx32 *a, const VecFx32 *b, const VecFx32 *p3,
                                                  const VecFx32 *p4, const VecFx32 *p5, VecFx32 *out) {
    VecFx32 n;
    VecFx32 dir;
    VecFx32 na;
    VecFx32 nb;
    fx32 len;
    fx32 num;
    fx32 denom;
    fx32 t;

    VEC_Subtract(p5, p4, &dir);
    len = VEC_Mag(&dir);
    VEC_Normalize(&dir, &dir);
    VEC_Normalize(a, &na);
    VEC_Normalize(b, &nb);
    VEC_CrossProduct(&na, &nb, &n);
    VEC_Normalize(&n, &n);
    num = (FX_Mul(n.x, p3->x) + FX_Mul(n.y, p3->y) + FX_Mul(n.z, p3->z)) -
          (FX_Mul(n.x, p4->x) + FX_Mul(n.y, p4->y) + FX_Mul(n.z, p4->z));
    denom = FX_Mul(n.x, dir.x) + FX_Mul(n.y, dir.y) + FX_Mul(n.z, dir.z);
    if (denom == 0) {
        return FALSE;
    }
    t = FX_Div(num, denom);
    if (t > len) {
        return FALSE;
    }
    if (t <= 0x1000) {
        return FALSE;
    }
    VEC_Set(out, p4->x + FX_Mul(dir.x, t), p4->y + FX_Mul(dir.y, t), p4->z + FX_Mul(dir.z, t));
    return TRUE;
}
