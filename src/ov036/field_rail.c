// The rail system of the no-grid maps: the rail units that move along the lines and points of a map's rails, the
// conversions between rail and world positions, and the cameras the rails set. The name is the ROM's own, from
// GFL_HeapAllocate's file argument
#include "types.h"
#include "field/field_camera.h"
#include "field/field_rail.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/mi.h"

static void FieldRailSystem_GetCameraParentPos(FieldRailSystem *rail, VecFx32 *pos);
static BOOL func_ov036_021b09fc(RailUnit *unit);
static FieldCamera *FieldRailSystem_GetFieldCamera(FieldRailSystem *rail);
static RailUnit *FieldRailSystem_GetCameraParentUnit(FieldRailSystem *rail);
static RailPoint *FieldRailSystem_GetPoint(FieldRailSystem *rail, u32 id);
static RailLine *FieldRailSystem_GetLine(FieldRailSystem *rail, u32 id);
static RailCurve *GetRailHnd2Curve(FieldRailSystem *rail, u32 id);
static fx32 FieldRailSystem_GetTileWidth(FieldRailSystem *rail);
static RailUnit *func_ov036_021b0c00(FieldRailSystem *rail);
static void func_ov036_021b0c04(FieldRailSystem *rail, const RailPosition *in, RailPosition *out);
static u16 RailUnit_IsOnLine(RailUnit *unit);
static RailLine *GetCurrentLine(RailUnit *unit);
static s32 GetCurrentLinePosV(RailUnit *unit);
static s32 GetCurrentLineLength(RailUnit *unit);
static s32 GetCurrentLinePosH(RailUnit *unit);
static s32 func_ov036_021b0cb8(RailUnit *unit);
static fx32 RailUnit_GetTileWidth(RailUnit *unit);
static RailCamera *GetRailCameraByCamHandler(RailUnit *unit);
static void func_ov036_021b0cc8(RailUnit *unit, VecFx32 *pos);
static void func_ov036_021b0cdc(RailUnit *unit, s32 front, VecFx32 *pos);
static RailPoint *GetCurrentRailLinePoint1(RailUnit *unit);
static RailPoint *GetCurrentRailLinePoint2(RailUnit *unit);
static RailCurve *RailUnit_GetCurve(RailUnit *unit);
static RailCamera *GetRailCameraByPoint(RailUnit *unit, RailPoint *point);
static s32 func_ov036_021b0d2c(s32 value);
static RailLine *GetLineContinueInDirection(RailPoint *point, u32 dir, RailDataHandle *data,
                                            RailDisabledLines *disabled, BOOL checkDisabled);
static u32 func_ov036_021b0f00(RailUnit *unit, RailLine *line, u32 dir, s32 front, s32 side, s32 length, s32 width);
static void RailUnit_SetLineInfo(RailUnit *unit, RailLine *line, u32 dir, s32 front, s32 side, s32 length, s32 width);
static BOOL func_ov036_021b1028(RailDataHandle *data, RailLine *line, RailPoint *point);
static BOOL func_ov036_021b1040(RailDataHandle *data, RailLine *line, RailPoint *point);
static void SetupLineWidthForLineChange(RailDataHandle *data, RailLine *line, s32 *outNext, s32 *outCur,
                                        RailDisabledLines *disabled, fx32 tileWidth);
static void func_ov036_021b1170(RailDataHandle *data);
static void func_ov036_021b117c(RailDataHandle *data, RailDataHandle *src);
static RailPoint *GetRailPoint(RailDataHandle *data, u32 id);
static RailLine *GetRailLine(RailDataHandle *data, u32 id);
static RailCamera *GetRailCamera(RailDataHandle *data, u32 id);
static RailCurve *GetRailCurve(RailDataHandle *data, u32 id);
static RailCameraFunc GetRailCameraCurveFunc(RailDataHandle *data, u32 id);
static RailToWorldFunc GetHandler1CurveFunc(RailDataHandle *data, u32 id);
static WorldToRailFunc GetHandler2CurveFunc(RailDataHandle *data, u32 id);
static s32 func_ov036_021b1290(RailUnit *m, u32 dir, s32 step);
static void RailUnit_SetDataCore(RailUnit *unit, RailDataHandle *data, FieldRailSystem *rail);
static void RailUnit_SetData(RailUnit *unit, RailDataHandle *data);
static u32 RailUnit_GetState(RailUnit *unit);
static BOOL func_ov036_021b1b60(RailUnit *unit);
static RailCamera *GetRailCameraByHandlerImpl(RailUnit *unit);
static void func_ov036_021b1bac(RailUnit *unit, VecFx32 *pos);
static void func_ov036_021b1bec(RailUnit *unit, RailPosition *pos);
static s32 GetLineTileLengthWorldSpace(RailLine *line, fx32 tileWidth, RailDataHandle *data);
static s32 GetLerpedLineWidthByLineYPos(RailLine *line, s32 y, s32 length, RailDataHandle *data);
static u32 func_ov036_021b1cec(RailUnit *move, RailLine *line, s32 len, u32 arg);
static BOOL func_ov036_021b1d1c(FieldRailSystem *rail, RailPosition *pos, u32 dir, s32 extra, VecFx32 *out);
static u32 func_ov036_021b1d88(u32 dir);
static u32 func_ov036_021b1db8(u32 dir);
static u32 func_ov036_021b1de8(u32 dir);
static void func_ov036_021b1e18(RailDisabledLines *disabled);
static void func_ov036_021b1e2c(RailDisabledLines *disabled, u16 lineId);
static void func_ov036_021b1e50(RailDisabledLines *disabled, u16 lineId);
static BOOL func_ov036_021b1e74(RailDisabledLines *disabled, u16 lineId);
static BOOL func_ov036_021b1e8c(RailDisabledLines *disabled, RailLine *line, RailDataHandle *data);
static RailLine *func_ov036_021b1ec4(RailDisabledLines *disabled, RailDataHandle *data, u32 lineId);
static void func_ov036_021b1ee4(RailDisabledLines *disabled, RailUnit *move, s32 dy, s32 dx, u32 dir);
static BOOL func_ov036_021b1f74(RailDisabledLines *disabled, RailLine *line, s32 front, s32 side, RailDataHandle *data,
                                fx32 tileWidth);
static u32 func_ov036_021b1fc0(RailPoint *point, RailLine *line, RailDataHandle *data);
static BOOL func_ov036_021b1ff0(RailLine *line, RailDataHandle *data);
static BOOL func_ov036_021b2060(RailPoint *point, RailDataHandle *data);
static void func_ov036_021b211c(const char *prefix, VecFx32 *pos, const char *suffix);
static void func_ov036_021b2120(const char *prefix, RailPoint *point, const char *suffix);
static void func_ov036_021b2130(RailUnit *unit);
static int func_ov036_021b2184(RailDataHandle *data, RailPoint *point);
static int func_ov036_021b21b0(RailDataHandle *data, RailLine *line);
static BOOL func_ov036_021b21dc(const VecFx32 *a, const VecFx32 *b, const VecFx32 *p3, const VecFx32 *p4,
                                const VecFx32 *p5, VecFx32 *out);
static void func_ov036_021b2404(VecFx32 *out, const VecFx32 *a, const VecFx32 *b, fx32 t);
static void InterpolateRailPointsGlobal(VecFx32 *out, const VecFx32 *a, const VecFx32 *b, fx32 t);
static void func_ov036_021b3194(FieldRailSystem *rail, u16 angle, s32 dist, VecFx32 *target);
static BOOL func_ov036_021b3830(RailPoint *p0, RailPoint *p1, const VecFx32 *pos, VecFx32 *out, fx32 *along,
                                VecFx32 *cross);
static void func_ov036_021b39c8(FieldRailSystem *rail, RailUnit *unit, RailPoint *point);

// The step lengths of the speed indices
static u8 sPendingSteps[] = { 1, 2, 4, 8, 16 };

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
    func_ov036_021b1e18(&rail->disabledLines);
    func_ov036_021b117c(&rail->data, data);
}

void FieldRailSystem_Reset(FieldRailSystem *rail) {
    rail->loaded = 0;
    func_ov036_021b1170(&rail->data);
    func_ov036_021b1e18(&rail->disabledLines);
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

static void FieldRailSystem_GetCameraParentPos(FieldRailSystem *rail, VecFx32 *pos) {
    if (rail->cameraParent != NULL) {
        RailUnit_GetCalcPos(rail->cameraParent, pos);
    }
}

void func_ov036_021b0398(FieldRailSystem *rail) {
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
    if (!func_ov036_021b1b60(unit)) {
        return FALSE;
    }
    if (!func_ov036_021b09fc(unit) && rail->cameraDirty == 0 && !force) {
        return FALSE;
    }
    switch (unit->onLine) {
    case 0:
        cameraId = unit->component.point->cameraId;
        if (cameraId != 0xffff) {
            func = GetRailCameraCurveFunc(unit->handle, GetRailCamera(unit->handle, cameraId)->funcId);
        }
        break;
    case 1:
        cameraId = unit->component.line->cameraId;
        if (cameraId != 0xffff) {
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
        RailLine *line = func_ov036_021b1ec4(&rail->disabledLines, &rail->data, i);
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
        if (!func_ov036_021b21dc(&diff, &normal, &pointB->pos, &startPos, &endPos, &cross)) {
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
        if (!func_ov036_021b1e74(&rail->disabledLines, lineId)) {
            func_ov036_021b1e2c(&rail->disabledLines, lineId);
        }
    } else {
        func_ov036_021b1e50(&rail->disabledLines, lineId);
    }
}

fx32 func_ov036_021b05ec(FieldRailSystem *rail) {
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

BOOL func_ov036_021b0644(FieldRailSystem *rail, const RailPosition *pos, u32 arg, VecFx32 *calcPos) {
    RailUnit_SetRailPos(rail->playerUnit, pos);
    func_ov036_021b0a94(rail->playerUnit, 4, arg);
    func_ov036_021b0ac0(rail->playerUnit);
    RailUnit_GetCalcPos(rail->playerUnit, calcPos);
    if (GFL_STD_MemCmp(pos, &rail->playerUnit->railPos, sizeof(RailPosition)) == 0) {
        return FALSE;
    }
    return func_ov036_021b0794(rail->playerUnit, &rail->playerUnit->railPos);
}

BOOL func_ov036_021b068c(FieldRailSystem *rail, const RailPosition *railPos, u32 railDir, RailPosition *next) {
    RailPosition start = *railPos;

    RailUnit_SetRailPos(rail->playerUnit, railPos);
    func_ov036_021b0a94(rail->playerUnit, 4, railDir);
    func_ov036_021b0ac0(rail->playerUnit);
    func_ov036_021b0774(rail->playerUnit, next);
    if (GFL_STD_MemCmp(&start, next, sizeof(RailPosition)) == 0) {
        return FALSE;
    }
    return func_ov036_021b0794(rail->playerUnit, next);
}

void func_ov036_021b06ec(FieldRailSystem *rail, const RailPosition *railPos, VecFx32 *pos) {
    RailUnit_SetRailPos(rail->playerUnit, railPos);
    RailUnit_GetCalcPos(rail->playerUnit, pos);
}

s32 func_ov036_021b0704(FieldRailSystem *rail, const RailPosition *pos) {
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
    case 0:
        cameraId = unit->component.point->cameraId;
        if (cameraId != 0xffff) {
            func = GetRailCameraCurveFunc(unit->handle, GetRailCamera(unit->handle, cameraId)->funcId);
        }
        break;
    case 1:
        cameraId = unit->component.line->cameraId;
        if (cameraId != 0xffff) {
            func = GetRailCameraCurveFunc(unit->handle, GetRailCamera(unit->handle, cameraId)->funcId);
        }
        break;
    }
    if (func != NULL) {
        func(rail);
    }
    rail->cameraParent = prevParent;
}

void func_ov036_021b0774(RailUnit *unit, RailPosition *pos) {
    *pos = unit->railPos;
}

BOOL func_ov036_021b0794(RailUnit *unit, const RailPosition *pos) {
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

u8 func_ov036_021b0814(RailUnit *unit, u32 dir, RailPosition *outPos, VecFx32 *outVec) {
    if (!unit->nextCached || dir != unit->nextDir) {
        unit->nextOk = func_ov036_021b068c(unit->rail, &unit->railPos, dir, &unit->nextPos);
        func_ov036_021b06ec(unit->rail, &unit->nextPos, &unit->nextVec);
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

void func_ov036_021b0888(RailUnit *unit, VecFx32 *pos) {
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

        unit->onLine = 0;
        point = &unit->handle->points[pos->componentId];
        unit->component.point = point;
        unit->width = point->widths[0];
        for (i = 0; i < 4; i++) {
            u32 lineId = unit->component.point->lineIds[i];

            if (lineId != 0xffff && line == NULL) {
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
    unit->unk33 = 1;
    unit->unk32 = dir;
    unit->active = 1;
    func_ov036_021b1bac(unit, &unit->calcPos);
    unit->railPos = *pos;
    unit->prevRailPos = *pos;
    unit->prevCalcPos = unit->calcPos;
}

void RailUnit_GetCalcPos(RailUnit *unit, VecFx32 *pos) {
    *pos = unit->calcPos;
}

u8 func_ov036_021b09ec(RailUnit *unit) {
    return unit->unk32;
}

u8 func_ov036_021b09f4(RailUnit *unit) {
    return unit->pending;
}

static BOOL func_ov036_021b09fc(RailUnit *unit) {
    return unit->unk34;
}

void func_ov036_021b0a04(RailUnit *unit, VecFx16 *dir) {
    s32 savedSide;
    s32 nextFront;
    s32 savedFront;
    s32 length;
    RailPosition tmp;
    VecFx32 from;
    VecFx32 to;

    savedSide = unit->side;
    unit->side = 0;
    func_ov036_021b1bac(unit, &from);
    unit->side = savedSide;
    savedFront = unit->front;
    nextFront = savedFront + 16;
    length = unit->length;
    if (nextFront < length) {
        unit->front = savedFront + 16;
        unit->side = 0;
        func_ov036_021b1bac(unit, &to);
        unit->side = savedSide;
        unit->front = savedFront;
    } else {
        func_ov036_021b1bec(unit, &tmp);
        tmp.posSide = 0;
        func_ov036_021b1d1c(unit->rail, &tmp, unit->component.line->dir, nextFront - length, &to);
    }
    VEC_Subtract(&to, &from, &to);
    VEC_Normalize(&to, &to);
    VEC_Fx16Set(dir, to.x, to.y, to.z);
}

u32 func_ov036_021b0a8c(RailUnit *unit) {
    return unit->component.line->dir;
}

BOOL func_ov036_021b0a94(RailUnit *unit, u32 a, u32 b) {
    if (unit->pending == 0) {
        unit->pending = 1;
        unit->counter = 0;
        unit->pendingArgB = b;
        unit->pendingArgA = a;
        return TRUE;
    }
    return FALSE;
}

void func_ov036_021b0ac0(RailUnit *unit) {
    u32 moved;
    u16 onLine;
    u8 step;
    u16 argB;
    s32 rem;

    if (unit->rail->loaded == 0 || unit->active == 0 || !func_ov036_021b1b60(unit)) {
        return;
    }
    moved = 0;
    unit->unk34 = 0;
    if (unit->pending == 0) {
        return;
    }
    argB = unit->pendingArgB;
    onLine = unit->onLine;
    step = sPendingSteps[unit->pendingArgA];
    unit->counter += (s8)step;
    if (onLine == 1) {
        moved = func_ov036_021b1290(unit, argB, step);
    }
    if (moved != 0) {
        unit->unk32 = moved;
        unit->unk33 = unit->pendingArgA;
        unit->unk34 = 1;
        func_ov036_021b1bac(unit, &unit->calcPos);
    }
    if (onLine != unit->onLine) {
        func_ov036_021b2130(unit);
    }
    if (unit->counter < 16 && moved != 0) {
        return;
    }
    unit->pending = 0;
    unit->nextCached = 0;
    unit->prevRailPos = unit->railPos;
    func_ov036_021b1bec(unit, &unit->railPos);
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

static FieldCamera *FieldRailSystem_GetFieldCamera(FieldRailSystem *rail) {
    return rail->camera;
}

static RailUnit *FieldRailSystem_GetCameraParentUnit(FieldRailSystem *rail) {
    return rail->cameraParent;
}

static RailPoint *FieldRailSystem_GetPoint(FieldRailSystem *rail, u32 id) {
    return GetRailPoint(&rail->data, id);
}

static RailLine *FieldRailSystem_GetLine(FieldRailSystem *rail, u32 id) {
    return GetRailLine(&rail->data, id);
}

static RailCurve *GetRailHnd2Curve(FieldRailSystem *rail, u32 id) {
    return GetRailCurve(&rail->data, id);
}

static fx32 FieldRailSystem_GetTileWidth(FieldRailSystem *rail) {
    return rail->data.tileWidth;
}

static RailUnit *func_ov036_021b0c00(FieldRailSystem *rail) {
    return rail->playerUnit;
}

static void func_ov036_021b0c04(FieldRailSystem *rail, const RailPosition *in, RailPosition *out) {
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
    if ((side < 0 ? -side : side) > width) {
        if (side > 0) {
            out->posSide = width / 16;
        } else {
            out->posSide = -width / 16;
        }
    }
}

static u16 RailUnit_IsOnLine(RailUnit *unit) {
    return unit->onLine;
}

static RailLine *GetCurrentLine(RailUnit *unit) {
    return unit->component.line;
}

static s32 GetCurrentLinePosV(RailUnit *unit) {
    return unit->front;
}

static s32 GetCurrentLineLength(RailUnit *unit) {
    return unit->length;
}

static s32 GetCurrentLinePosH(RailUnit *unit) {
    return unit->side;
}

static s32 func_ov036_021b0cb8(RailUnit *unit) {
    return unit->width;
}

static fx32 RailUnit_GetTileWidth(RailUnit *unit) {
    return unit->tileWidth;
}

static RailCamera *GetRailCameraByCamHandler(RailUnit *unit) {
    return GetRailCameraByHandlerImpl(unit);
}

static void func_ov036_021b0cc8(RailUnit *unit, VecFx32 *pos) {
    s32 saved = unit->side;

    unit->side = 0;
    func_ov036_021b1bac(unit, pos);
    unit->side = saved;
}

static void func_ov036_021b0cdc(RailUnit *unit, s32 front, VecFx32 *pos) {
    s32 saved = unit->front;

    unit->front = front;
    func_ov036_021b1bac(unit, pos);
    unit->front = saved;
}

static RailPoint *GetCurrentRailLinePoint1(RailUnit *unit) {
    return GetRailPoint(unit->handle, unit->component.line->pointA);
}

static RailPoint *GetCurrentRailLinePoint2(RailUnit *unit) {
    return GetRailPoint(unit->handle, unit->component.line->pointB);
}

static RailCurve *RailUnit_GetCurve(RailUnit *unit) {
    return GetRailCurve(unit->handle, unit->component.line->curveId);
}

static RailCamera *GetRailCameraByPoint(RailUnit *unit, RailPoint *point) {
    return GetRailCamera(unit->handle, point->cameraId);
}

static s32 func_ov036_021b0d2c(s32 value) {
    u32 abs = value < 0 ? -value : value;

    if ((abs & 0xf) >= 8) {
        if (value >= 0) {
            return value / 16 + 1;
        }
        return value / 16 - 1;
    }
    return value / 16;
}

BOOL func_ov036_021b0d64(const VecFx32 *a, const VecFx32 *b, fx32 dist) {
    s32 dx = (a->x >> 12) - (b->x >> 12);
    s32 dz = (a->z >> 12) - (b->z >> 12);
    s32 distSq = dx * dx + dz * dz;
    s32 dy = a->y - b->y;
    s32 radius;
    s32 radiusSq;

    if (dy < 0) {
        dy = -dy;
    }
    radius = (dist >> 12) * 2;
    radiusSq = radius * radius;
    if (dy < 0x8000 && distSq < radiusSq) {
        return TRUE;
    }
    return FALSE;
}

void SetWkToActorRailPos(RailUnit *unit, u16 *a1, u16 *a2, u16 *a3) {
    s32 width = GetLerpedLineWidthByLineYPos(unit->component.line, 0, unit->length, unit->handle);
    s32 endWidth = GetLerpedLineWidthByLineYPos(unit->component.line, unit->length, unit->length, unit->handle);
    RailDataHandle *handle;
    s32 i;
    s32 index;

    if (width < endWidth) {
        width = endWidth;
    }
    handle = unit->handle;
    index = 0;
    for (i = 0; i < handle->lineCount; i++) {
        if (unit->component.line == &handle->lines[i]) {
            index = i;
        }
    }
    *a1 = index;
    *a2 = unit->front / 16;
    *a3 = unit->side / 16 + width / 16;
}

void func_ov036_021b0e38(RailUnit *unit, u16 a1, u16 a2, u16 a3) {
    RailPosition pos;

    func_ov036_021b0774(unit, &pos);
    pos.componentId = a1;
    pos.posSide = 0;
    pos.posFront = a2;
    RailUnit_SetRailPos(unit, &pos);
    pos.posSide = a3 - unit->width / 16;
    func_ov036_021b1bac(unit, &unit->calcPos);
    unit->railPos = pos;
    unit->prevRailPos = pos;
    unit->prevCalcPos = unit->calcPos;
}

static RailLine *GetLineContinueInDirection(RailPoint *point, u32 dir, RailDataHandle *data,
                                            RailDisabledLines *disabled, BOOL checkDisabled) {
    s32 i;
    u32(*ids)[4] = &point->lineIds;

    for (i = 0; i < 4; i++) {
        if (dir == point->dirs[i]) {
            if (checkDisabled) {
                return func_ov036_021b1ec4(disabled, data, (*ids)[i]);
            }
            return GetRailLine(data, (*ids)[i]);
        }
    }
    return NULL;
}

static u32 func_ov036_021b0f00(RailUnit *unit, RailLine *line, u32 dir, s32 front, s32 side, s32 length, s32 width) {
    RailLine *next;

    if (!func_ov036_021b1f74(unit->disabled, line, front, side, unit->handle, unit->tileWidth)) {
        return 0;
    }
    if ((side < 0 ? -side : side) > width) {
        return 0;
    }
    if ((front >= length || front < 0)) {
        if (front >= length) {
            next = GetLineContinueInDirection(GetRailPoint(unit->handle, line->pointB), line->dir, unit->handle,
                                              unit->disabled, TRUE);
        } else {
            next = GetLineContinueInDirection(GetRailPoint(unit->handle, line->pointA), func_ov036_021b1d88(line->dir),
                                              unit->handle, unit->disabled, TRUE);
        }
        if (next == NULL) {
            if (front > length) {
                return 0;
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
                return 0;
            }
            if ((side < 0 ? -side : side) > width) {
                return 0;
            }
        }
    }
    RailUnit_SetLineInfo(unit, line, dir, front, side, length, width);
    func_ov036_021b1ff0(line, unit->handle);
    return dir;
}

static void RailUnit_SetLineInfo(RailUnit *unit, RailLine *line, u32 dir, s32 front, s32 side, s32 length, s32 width) {
    unit->onLine = 1;
    unit->component.line = line;
    unit->side = side;
    unit->length = length;
    unit->dir = dir;
    unit->front = front;
    unit->width = width;
}

static BOOL func_ov036_021b1028(RailDataHandle *data, RailLine *line, RailPoint *point) {
    if (GetRailPoint(data, line->pointA) == point) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_021b1040(RailDataHandle *data, RailLine *line, RailPoint *point) {
    if (GetRailPoint(data, line->pointB) == point) {
        return TRUE;
    }
    return FALSE;
}

static void SetupLineWidthForLineChange(RailDataHandle *data, RailLine *line, s32 *outNext, s32 *outCur,
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
    l1 = GetLineContinueInDirection(point, func_ov036_021b1d88(line->dir), data, disabled, TRUE);
    l2 = GetLineContinueInDirection(point, func_ov036_021b1de8(line->dir), data, disabled, TRUE);
    l3 = GetLineContinueInDirection(point, func_ov036_021b1db8(line->dir), data, disabled, TRUE);
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
        if (func_ov036_021b1028(data, l2, point)) {
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
        if (func_ov036_021b1028(data, l3, point)) {
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

static void func_ov036_021b1170(RailDataHandle *data) {
    MI_CpuFill8(data, 0, sizeof(RailDataHandle));
}

static void func_ov036_021b117c(RailDataHandle *data, RailDataHandle *src) {
    MI_CpuCopy8(src, data, sizeof(RailDataHandle));
}

static RailPoint *GetRailPoint(RailDataHandle *data, u32 id) {
    if (data->pointCount > id) {
        return &data->points[id];
    }
    if (id == 0xffff) {
        return NULL;
    }
    return data->points;
}

static RailLine *GetRailLine(RailDataHandle *data, u32 id) {
    if (data->lineCount > id) {
        return &data->lines[id];
    }
    if (id == 0xffff) {
        return NULL;
    }
    return data->lines;
}

static const RailCamera DEFAULT_RAIL_CAMERA = { 0xffff };

static RailCamera *GetRailCamera(RailDataHandle *data, u32 id) {
    if (data->cameraCount > id) {
        return &data->cameras[id];
    }
    if (id == 0xffff) {
        return NULL;
    }
    return (RailCamera *)&DEFAULT_RAIL_CAMERA;
}

static const RailCurve DEFAULT_RAIL_CURVE = { 0xffff, { 0xff, 0, 0 } };

static RailCurve *GetRailCurve(RailDataHandle *data, u32 id) {
    if (data->curveCount > id) {
        return &data->curves[id];
    }
    if (id == 0xffff) {
        return NULL;
    }
    return (RailCurve *)&DEFAULT_RAIL_CURVE;
}

static RailCameraFunc GetRailCameraCurveFunc(RailDataHandle *data, u32 id) {
    if (data->camFuncCount > id) {
        return data->camFuncs[id];
    }
    if (id == 0xffff) {
        return NULL;
    }
    return data->camFuncs[0];
}

static RailToWorldFunc GetHandler1CurveFunc(RailDataHandle *data, u32 id) {
    if (data->railToWorldCount > id) {
        return data->railToWorldFuncs[id];
    }
    if (id == 0xffff) {
        return NULL;
    }
    return data->railToWorldFuncs[0];
}

static WorldToRailFunc GetHandler2CurveFunc(RailDataHandle *data, u32 id) {
    if (data->worldToRailCount > id) {
        return data->worldToRailFuncs[id];
    }
    if (id == 0xffff) {
        return NULL;
    }
    return data->worldToRailFuncs[0];
}

static s32 func_ov036_021b1290(RailUnit *m, u32 dir, s32 step) {
    RailLine *line = m->component.line;
    s32 len = m->length;
    RailPoint *point;
    RailLine *next0;
    RailLine *next1;
    RailLine *next2;
    BOOL flag;
    s32 tileLen;
    s32 width;
    s32 r;
    s32 ax;

    if (dir == line->dir) {
        point = GetRailPoint(m->handle, line->pointB);
        flag = FALSE;
        next0 = GetLineContinueInDirection(point, dir, m->handle, m->disabled, 0);
        next1 = GetLineContinueInDirection(point, func_ov036_021b1de8(dir), m->handle, m->disabled, 0);
        next2 = GetLineContinueInDirection(point, func_ov036_021b1db8(dir), m->handle, m->disabled, 0);
        func_ov036_021b1ee4(m->disabled, m, step, 0, dir);
        if (len > m->front) {
            width = GetLerpedLineWidthByLineYPos(line, m->front, len, m->handle);
            ax = m->side;
            if (ax < 0) {
                ax = -ax;
            }
            if (ax <= width) {
                return dir;
            }
            flag = TRUE;
        }
        if (next0 != NULL && !flag) {
            func_ov036_021b2060(point, m->handle);
            tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
            width = GetLerpedLineWidthByLineYPos(next0, m->front - len, tileLen, m->handle);
            r = func_ov036_021b0f00(m, next0, dir, m->front - len, m->side, tileLen, width);
            if (r != 0) {
                return r;
            }
        }
        if (next1 != NULL && m->side <= 0) {
            if (func_ov036_021b1028(m->handle, next1, point)) {
                func_ov036_021b2060(point, m->handle);
                tileLen = GetLineTileLengthWorldSpace(next1, m->tileWidth, m->handle);
                width = GetLerpedLineWidthByLineYPos(next1, -m->side, tileLen, m->handle);
                r = func_ov036_021b0f00(m, next1, dir, -m->side, m->front - len, tileLen, width);
                if (r != 0) {
                    return r;
                }
            } else if (func_ov036_021b1040(m->handle, next1, point)) {
                func_ov036_021b2060(point, m->handle);
                tileLen = GetLineTileLengthWorldSpace(next1, m->tileWidth, m->handle);
                width = GetLerpedLineWidthByLineYPos(next1, tileLen + m->side, tileLen, m->handle);
                r = func_ov036_021b0f00(m, next1, dir, tileLen + m->side, -(m->front - len), tileLen, width);
                if (r != 0) {
                    return r;
                }
            }
        }
        if (next2 != NULL && m->side >= 0) {
            if (func_ov036_021b1028(m->handle, next2, point)) {
                func_ov036_021b2060(point, m->handle);
                tileLen = GetLineTileLengthWorldSpace(next2, m->tileWidth, m->handle);
                width = GetLerpedLineWidthByLineYPos(next2, m->side, tileLen, m->handle);
                r = func_ov036_021b0f00(m, next2, dir, m->side, -(m->front - len), tileLen, width);
                if (r != 0) {
                    return r;
                }
            } else if (func_ov036_021b1040(m->handle, next2, point)) {
                func_ov036_021b2060(point, m->handle);
                tileLen = GetLineTileLengthWorldSpace(next2, m->tileWidth, m->handle);
                width = GetLerpedLineWidthByLineYPos(next2, tileLen - m->side, tileLen, m->handle);
                r = func_ov036_021b0f00(m, next2, dir, tileLen - m->side, m->front - len, tileLen, width);
                if (r != 0) {
                    return r;
                }
            }
        }
        if (!flag) {
            if (m->front <= len) {
                return dir;
            }
            m->front -= step;
            return 0;
        }
        return func_ov036_021b1cec(m, line, len, dir);
    }
    if (dir != func_ov036_021b1d88(line->dir)) {
        if (dir != func_ov036_021b1db8(line->dir)) {
            if (dir != func_ov036_021b1de8(line->dir)) {
                return 0;
            }
            func_ov036_021b1ee4(m->disabled, m, 0, -step, dir);
            if (m->side < 0 && m->side % 16 == 0) {
                point = GetRailPoint(m->handle, line->pointA);
                next0 = GetLineContinueInDirection(point, dir, m->handle, m->disabled, 0);
                if (next0 != NULL) {
                    if (func_ov036_021b1028(m->handle, next0, point)) {
                        func_ov036_021b2060(point, m->handle);
                        tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
                        ax = m->side;
                        if (ax < 0) {
                            ax = -ax;
                        }
                        width = GetLerpedLineWidthByLineYPos(next0, ax, tileLen, m->handle);
                        ax = m->side;
                        if (ax < 0) {
                            ax = -ax;
                        }
                        r = func_ov036_021b0f00(m, next0, dir, ax, m->front, tileLen, width);
                        if (r != 0) {
                            return r;
                        }
                    }
                    if (func_ov036_021b1040(m->handle, next0, point)) {
                        func_ov036_021b2060(point, m->handle);
                        tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
                        ax = m->side;
                        if (ax < 0) {
                            ax = -ax;
                        }
                        width = GetLerpedLineWidthByLineYPos(next0, tileLen - ax, tileLen, m->handle);
                        ax = m->side;
                        if (ax < 0) {
                            ax = -ax;
                        }
                        r = func_ov036_021b0f00(m, next0, dir, tileLen - ax, -m->front, tileLen, width);
                        if (r != 0) {
                            return r;
                        }
                    }
                }
                point = GetRailPoint(m->handle, line->pointB);
                next0 = GetLineContinueInDirection(point, dir, m->handle, m->disabled, 0);
                if (next0 != NULL) {
                    if (func_ov036_021b1028(m->handle, next0, point)) {
                        func_ov036_021b2060(point, m->handle);
                        tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
                        ax = m->side;
                        if (ax < 0) {
                            ax = -ax;
                        }
                        width = GetLerpedLineWidthByLineYPos(next0, ax, tileLen, m->handle);
                        ax = m->side;
                        if (ax < 0) {
                            ax = -ax;
                        }
                        r = func_ov036_021b0f00(m, next0, dir, ax, -(len - m->front), tileLen, width);
                        if (r != 0) {
                            return r;
                        }
                    }
                    if (func_ov036_021b1040(m->handle, next0, point)) {
                        func_ov036_021b2060(point, m->handle);
                        tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
                        ax = m->side;
                        if (ax < 0) {
                            ax = -ax;
                        }
                        width = GetLerpedLineWidthByLineYPos(next0, tileLen - ax, tileLen, m->handle);
                        ax = m->side;
                        if (ax < 0) {
                            ax = -ax;
                        }
                        r = func_ov036_021b0f00(m, next0, dir, tileLen - ax, len - m->front, tileLen, width);
                        if (r != 0) {
                            return r;
                        }
                    }
                }
            }
            if (m->side < 0 && -m->side <= m->width) {
                return func_ov036_021b1cec(m, line, len, dir);
            }
            return dir;
        }
        func_ov036_021b1ee4(m->disabled, m, 0, step, dir);
        if (m->side > 0 && m->side % 16 == 0) {
            point = GetRailPoint(m->handle, line->pointA);
            next0 = GetLineContinueInDirection(point, dir, m->handle, m->disabled, 0);
            if (next0 != NULL) {
                if (func_ov036_021b1028(m->handle, next0, point)) {
                    func_ov036_021b2060(point, m->handle);
                    tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
                    width = GetLerpedLineWidthByLineYPos(next0, m->side, tileLen, m->handle);
                    r = func_ov036_021b0f00(m, next0, dir, m->side, -m->front, tileLen, width);
                    if (r != 0) {
                        return r;
                    }
                }
                if (func_ov036_021b1040(m->handle, next0, point)) {
                    func_ov036_021b2060(point, m->handle);
                    tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
                    width = GetLerpedLineWidthByLineYPos(next0, tileLen - m->side, tileLen, m->handle);
                    r = func_ov036_021b0f00(m, next0, dir, tileLen - m->side, m->front, tileLen, width);
                    if (r != 0) {
                        return r;
                    }
                }
            }
            point = GetRailPoint(m->handle, line->pointB);
            next0 = GetLineContinueInDirection(point, dir, m->handle, m->disabled, 0);
            if (next0 != NULL) {
                if (func_ov036_021b1028(m->handle, next0, point)) {
                    func_ov036_021b2060(point, m->handle);
                    tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
                    width = GetLerpedLineWidthByLineYPos(next0, m->side, tileLen, m->handle);
                    r = func_ov036_021b0f00(m, next0, dir, m->side, len - m->front, tileLen, width);
                    if (r != 0) {
                        return r;
                    }
                }
                if (func_ov036_021b1040(m->handle, next0, point)) {
                    func_ov036_021b2060(point, m->handle);
                    tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
                    width = GetLerpedLineWidthByLineYPos(next0, tileLen - m->side, tileLen, m->handle);
                    r = func_ov036_021b0f00(m, next0, dir, tileLen - m->side, -(len - m->front), tileLen, width);
                    if (r != 0) {
                        return r;
                    }
                }
            }
        }
        if (m->side > 0 && m->side <= m->width) {
            return func_ov036_021b1cec(m, line, len, dir);
        }
        return dir;
    }
    point = GetRailPoint(m->handle, line->pointA);
    flag = FALSE;
    next0 = GetLineContinueInDirection(point, func_ov036_021b1db8(dir), m->handle, m->disabled, 0);
    next1 = GetLineContinueInDirection(point, func_ov036_021b1de8(dir), m->handle, m->disabled, 0);
    next2 = GetLineContinueInDirection(point, dir, m->handle, m->disabled, 0);
    func_ov036_021b1ee4(m->disabled, m, -step, 0, dir);
    if (m->front >= 0) {
        width = GetLerpedLineWidthByLineYPos(line, m->front, len, m->handle);
        ax = m->side;
        if (ax < 0) {
            ax = -ax;
        }
        if (ax <= width) {
            return dir;
        }
        flag = TRUE;
    }
    if (next2 != NULL && !flag) {
        func_ov036_021b2060(point, m->handle);
        tileLen = GetLineTileLengthWorldSpace(next2, m->tileWidth, m->handle);
        ax = m->front;
        if (ax < 0) {
            ax = -ax;
        }
        width = GetLerpedLineWidthByLineYPos(next2, tileLen - ax, tileLen, m->handle);
        ax = m->front;
        if (ax < 0) {
            ax = -ax;
        }
        r = func_ov036_021b0f00(m, next2, dir, tileLen - ax, m->side, tileLen, width);
        if (r != 0) {
            return r;
        }
    }
    if (next0 != NULL && m->side <= 0) {
        if (func_ov036_021b1028(m->handle, next0, point)) {
            func_ov036_021b2060(point, m->handle);
            tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
            width = GetLerpedLineWidthByLineYPos(next0, -m->side, tileLen, m->handle);
            r = func_ov036_021b0f00(m, next0, dir, -m->side, m->front, tileLen, width);
            if (r != 0) {
                return r;
            }
        } else if (func_ov036_021b1040(m->handle, next0, point)) {
            func_ov036_021b2060(point, m->handle);
            tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
            width = GetLerpedLineWidthByLineYPos(next0, tileLen + m->side, tileLen, m->handle);
            r = func_ov036_021b0f00(m, next0, dir, tileLen + m->side, -m->front, tileLen, width);
            if (r != 0) {
                return r;
            }
        }
    }
    if (next1 != NULL && m->side >= 0) {
        if (func_ov036_021b1028(m->handle, next1, point)) {
            func_ov036_021b2060(point, m->handle);
            tileLen = GetLineTileLengthWorldSpace(next1, m->tileWidth, m->handle);
            width = GetLerpedLineWidthByLineYPos(next1, m->side, tileLen, m->handle);
            r = func_ov036_021b0f00(m, next1, dir, m->side, -m->front, tileLen, width);
            if (r != 0) {
                return r;
            }
        }
        if (func_ov036_021b1040(m->handle, next1, point)) {
            func_ov036_021b2060(point, m->handle);
            tileLen = GetLineTileLengthWorldSpace(next1, m->tileWidth, m->handle);
            width = GetLerpedLineWidthByLineYPos(next1, tileLen - m->side, tileLen, m->handle);
            r = func_ov036_021b0f00(m, next1, dir, tileLen - m->side, m->front, tileLen, width);
            if (r != 0) {
                return r;
            }
        }
    }
    if (flag) {
        return func_ov036_021b1cec(m, line, len, dir);
    }
    m->front += step;
    return 0;
}

static void RailUnit_SetDataCore(RailUnit *unit, RailDataHandle *data, FieldRailSystem *rail) {
    unit->handle = data;
    unit->tileWidth = data->tileWidth;
    unit->onLine = 2;
    unit->unk33 = 0;
    unit->unk32 = 0;
    unit->pending = 0;
    unit->rail = rail;
    unit->component.point = NULL;
    unit->front = 0;
    unit->side = 0;
    unit->dir = 0;
    unit->disabled = &rail->disabledLines;
    unit->nextCached = 0;
    unit->active = 0;
}

static void RailUnit_SetData(RailUnit *unit, RailDataHandle *data) {
    RailUnit_SetDataCore(unit, data, unit->rail);
}

static u32 RailUnit_GetState(RailUnit *unit) {
    return unit->acquired;
}

static BOOL func_ov036_021b1b60(RailUnit *unit) {
    if (unit->onLine < 2 && unit->component.point != NULL && unit->acquired != 0) {
        return TRUE;
    }
    return FALSE;
}

static RailCamera *GetRailCameraByHandlerImpl(RailUnit *unit) {
    switch (unit->onLine) {
    case 0:
        return GetRailCamera(unit->handle, unit->component.point->cameraId);
    case 1:
        return GetRailCamera(unit->handle, unit->component.line->cameraId);
    }
    return (RailCamera *)&DEFAULT_RAIL_CAMERA;
}

static void func_ov036_021b1bac(RailUnit *unit, VecFx32 *pos) {
    if (unit->onLine == 0) {
        *pos = unit->component.point->pos;
    } else if (unit->onLine == 1) {
        RailCurve *curve = GetRailCurve(unit->handle, unit->component.line->curveId);
        RailToWorldFunc func = GetHandler1CurveFunc(unit->handle, curve->handlerId);

        if (func != NULL) {
            func(unit, pos);
        }
    }
}

static void func_ov036_021b1bec(RailUnit *unit, RailPosition *pos) {
    u16 onLine = unit->onLine;
    s32 id;
    s32 front;

    pos->componentIsLine = onLine;
    if (onLine == 0) {
        id = func_ov036_021b2184(unit->handle, unit->component.point);
    } else {
        id = func_ov036_021b21b0(unit->handle, unit->component.line);
    }
    front = unit->front;
    pos->componentId = id;
    pos->posFront = front / 16;
    pos->posSide = unit->side / 16;
    pos->railDirection = unit->dir;
}

static s32 GetLineTileLengthWorldSpace(RailLine *line, fx32 tileWidth, RailDataHandle *data) {
    return line->length << 4;
}

static s32 GetLerpedLineWidthByLineYPos(RailLine *line, s32 y, s32 length, RailDataHandle *data) {
    RailPoint *pointA = &data->points[line->pointA];
    RailPoint *pointB = &data->points[line->pointB];
    s32 widthA;
    s32 widthB;
    s32 diff;
    s32 i;

    widthB = -1;
    widthA = widthB;
    for (i = 0; i < 4; i++) {
        if (pointA->lineIds[i] != 0xffff && line == &data->lines[pointA->lineIds[i]]) {
            widthA = pointA->widths[i];
        }
        if (pointB->lineIds[i] != 0xffff && line == &data->lines[pointB->lineIds[i]]) {
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

static u32 func_ov036_021b1cec(RailUnit *move, RailLine *line, s32 len, u32 arg) {
    s32 width = GetLerpedLineWidthByLineYPos(line, move->front, len, move->handle);
    s32 side = move->side;
    s32 absSide;

    move->width = width;
    absSide = side < 0 ? -side : side;
    if (absSide >= width) {
        if (side > 0) {
            move->side = width;
        } else {
            width = -width;
            move->side = width;
        }
    }
    return arg;
}

static BOOL func_ov036_021b1d1c(FieldRailSystem *rail, RailPosition *pos, u32 dir, s32 extra, VecFx32 *out) {
    s32 i;

    RailUnit_SetRailPos(rail->playerUnit, pos);
    func_ov036_021b0a94(rail->playerUnit, 4, dir);
    func_ov036_021b0ac0(rail->playerUnit);
    func_ov036_021b0a94(rail->playerUnit, 0, dir);
    for (i = 0; i < 16; i++) {
        if (extra == i) {
            RailUnit_GetCalcPos(rail->playerUnit, out);
        }
        func_ov036_021b0ac0(rail->playerUnit);
    }
    if (GFL_STD_MemCmp(pos, &rail->playerUnit->railPos, sizeof(RailPosition)) == 0) {
        return FALSE;
    }
    return func_ov036_021b0794(rail->playerUnit, &rail->playerUnit->railPos);
}

static u32 func_ov036_021b1d88(u32 dir) {
    switch (dir) {
    case 1:
        return 3;
    case 3:
        return 1;
    case 4:
        return 2;
    case 2:
        return 4;
    }
    return 0;
}

static u32 func_ov036_021b1db8(u32 dir) {
    switch (dir) {
    case 1:
        return 2;
    case 3:
        return 4;
    case 4:
        return 1;
    case 2:
        return 3;
    }
    return 0;
}

static u32 func_ov036_021b1de8(u32 dir) {
    switch (dir) {
    case 1:
        return 4;
    case 3:
        return 2;
    case 4:
        return 3;
    case 2:
        return 1;
    }
    return 0;
}

static void func_ov036_021b1e18(RailDisabledLines *disabled) {
    s32 i;

    for (i = 0; i < 18; i++) {
        disabled->lineIds[i] = 0xffff;
    }
}

static void func_ov036_021b1e2c(RailDisabledLines *disabled, u16 lineId) {
    s32 i;

    for (i = 0; i < 18; i++) {
        if (disabled->lineIds[i] == 0xffff) {
            disabled->lineIds[i] = lineId;
            return;
        }
    }
}

static void func_ov036_021b1e50(RailDisabledLines *disabled, u16 lineId) {
    s32 i;

    for (i = 0; i < 18; i++) {
        if (disabled->lineIds[i] == lineId) {
            disabled->lineIds[i] = 0xffff;
            return;
        }
    }
}

static BOOL func_ov036_021b1e74(RailDisabledLines *disabled, u16 lineId) {
    s32 i;

    for (i = 0; i < 18; i++) {
        if (disabled->lineIds[i] == lineId) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL func_ov036_021b1e8c(RailDisabledLines *disabled, RailLine *line, RailDataHandle *data) {
    u32 index = 0;
    s32 i;

    for (i = 0; i < data->lineCount; i++) {
        if (line == &data->lines[i]) {
            index = i;
        }
    }
    return func_ov036_021b1e74(disabled, index);
}

static RailLine *func_ov036_021b1ec4(RailDisabledLines *disabled, RailDataHandle *data, u32 lineId) {
    if (func_ov036_021b1e74(disabled, lineId) == 0) {
        return GetRailLine(data, lineId);
    } else {
        return NULL;
    }
}

static void func_ov036_021b1ee4(RailDisabledLines *disabled, RailUnit *move, s32 dy, s32 dx, u32 dir) {
    s32 nextWidth;
    s32 curWidth;

    if (func_ov036_021b1e8c(disabled, move->component.line, move->handle)) {
        RailPoint *point = GetRailPoint(move->handle, move->component.line->pointA);
        RailLine *next = GetLineContinueInDirection(point, dir, move->handle, disabled, FALSE);

        SetupLineWidthForLineChange(move->handle, move->component.line, &nextWidth, &curWidth, disabled,
                                    move->tileWidth);
        move->front += dy;
        move->side += dx;
        if (move->front > nextWidth) {
            move->front = nextWidth;
        }
        if (next == NULL) {
            s32 side = move->side;
            s32 absSide = side < 0 ? -side : side;

            if (absSide > curWidth) {
                if (side > 0) {
                    move->side = curWidth;
                } else {
                    move->side = -curWidth;
                }
            }
        }
    } else {
        move->front += dy;
        move->side += dx;
    }
}

static BOOL func_ov036_021b1f74(RailDisabledLines *disabled, RailLine *line, s32 front, s32 side, RailDataHandle *data,
                                fx32 tileWidth) {
    s32 nextWidth;
    s32 curWidth;

    if (func_ov036_021b1e8c(disabled, line, data)) {
        SetupLineWidthForLineChange(data, line, &nextWidth, &curWidth, disabled, tileWidth);
        if (front <= nextWidth) {
            if (side < 0) {
                side = -side;
            }
            if (side <= curWidth) {
                return TRUE;
            }
        }
        return FALSE;
    }
    return TRUE;
}

static u32 func_ov036_021b1fc0(RailPoint *point, RailLine *line, RailDataHandle *data) {
    int i;

    for (i = 0; i < 4; i++) {
        if (GetRailLine(data, point->lineIds[i]) == line) {
            return point->dirs[i];
        }
    }
    return 0;
}

static BOOL func_ov036_021b1ff0(RailLine *line, RailDataHandle *data) {
    RailPoint *pointA;
    RailPoint *pointB;
    u32 dirA;
    u32 dirB;

    if (line->pointA == 0xffff || line->pointB == 0xffff) {
        return FALSE;
    }
    pointA = GetRailPoint(data, line->pointA);
    pointB = GetRailPoint(data, line->pointB);
    dirA = func_ov036_021b1fc0(pointA, line, data);
    if (dirA == 0) {
        return FALSE;
    }
    if (dirA != line->dir) {
        return FALSE;
    }
    dirB = func_ov036_021b1fc0(pointB, line, data);
    if (dirB == 0) {
        return FALSE;
    }
    if (dirB == func_ov036_021b1d88(line->dir)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_021b2060(RailPoint *point, RailDataHandle *data) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if ((point->dirs[i] == 0) != (point->lineIds[i] == 0xffff)) {
            return FALSE;
        }
        if (point->lineIds[i] != 0xffff) {
            RailLine *line = GetRailLine(data, point->lineIds[i]);
            RailPoint *pointA = GetRailPoint(data, line->pointA);
            RailPoint *pointB = GetRailPoint(data, line->pointB);
            s32 j;

            if (pointA != point && pointB != point) {
                return FALSE;
            }
            if (point->dirs[i] != 0) {
                for (j = i + 1; j < 4; j++) {
                    if (point->dirs[i] == func_ov036_021b1d88(point->dirs[j]) &&
                        line->dir != GetRailLine(data, point->lineIds[j])->dir) {
                        return FALSE;
                    }
                }
            }
        }
    }
    return TRUE;
}

static void func_ov036_021b211c(const char *prefix, VecFx32 *pos, const char *suffix) {
}

static void func_ov036_021b2120(const char *prefix, RailPoint *point, const char *suffix) {
    if (point != NULL) {
        func_ov036_021b211c(prefix, &point->pos, suffix);
    }
}

static void func_ov036_021b2130(RailUnit *unit) {
    if (unit->onLine == 0) {
        func_ov036_021b2120(NULL, unit->component.point, "\n");
    } else if (unit->onLine == 1) {
        RailPoint *pointA = GetRailPoint(unit->handle, unit->component.line->pointA);
        RailPoint *pointB = GetRailPoint(unit->handle, unit->component.line->pointB);

        func_ov036_021b2120("start", pointA, NULL);
        func_ov036_021b2120("-end", pointB, "\n");
    }
}

static int func_ov036_021b2184(RailDataHandle *data, RailPoint *point) {
    int i;

    for (i = 0; i < data->pointCount; i++) {
        if (point == &data->points[i]) {
            return i;
        }
    }
    return 0;
}

static int func_ov036_021b21b0(RailDataHandle *data, RailLine *line) {
    int i;

    for (i = 0; i < data->lineCount; i++) {
        if (line == &data->lines[i]) {
            return i;
        }
    }
    return 0;
}

static BOOL func_ov036_021b21dc(const VecFx32 *a, const VecFx32 *b, const VecFx32 *p3, const VecFx32 *p4,
                                const VecFx32 *p5, VecFx32 *out) {
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
    if (len < t) {
        return FALSE;
    }
    if (t <= 0x1000) {
        return FALSE;
    }
    out->x = p4->x + FX_Mul(dir.x, t);
    out->y = p4->y + FX_Mul(dir.y, t);
    out->z = p4->z + FX_Mul(dir.z, t);
    return TRUE;
}

static void func_ov036_021b2404(VecFx32 *out, const VecFx32 *a, const VecFx32 *b, fx32 t) {
    VecFx32 na;
    VecFx32 nb;
    s32 angle;
    u16 sinA;

    VEC_Normalize(a, &na);
    VEC_Normalize(b, &nb);
    angle = FX_AcosIdx(VEC_DotProduct(&na, &nb));
    sinA = FX_SinIdx(angle);
    if (sinA == 0) {
        *out = nb;
        return;
    }
    angle <<= 12;
    vecfx_mul(&na, FX_Div((u16)FX_SinIdx(FX_Mul(angle, 0x1000 - t) >> 12), sinA), &na);
    vecfx_mul(&nb, FX_Div((u16)FX_SinIdx(FX_Mul(angle, t) >> 12), sinA), &nb);
    VEC_Add(&na, &nb, out);
    VEC_Normalize(out, out);
}

static void InterpolateRailPointsGlobal(VecFx32 *out, const VecFx32 *a, const VecFx32 *b, fx32 t) {
    VecFx32 zero = { 0, 0, 0 };
    fx32 lenA;
    fx32 lenB;

    func_ov036_021b2404(out, a, b, t);
    lenA = VEC_Mag(a);
    lenB = VEC_Mag(b);
    VEC_MultAdd(lenA + FX_Mul(lenB - lenA, t), out, &zero, out);
}

void RailMath_RailToWorldNoCurve(RailUnit *unit, VecFx32 *out) {
    VecFx32 dir;
    RailPoint *p0;
    RailPoint *p1;
    VecFx32 *start;
    fx32 t;
    s32 posV;
    s32 posH;
    s32 width;

    GetCurrentLine(unit);
    posV = GetCurrentLinePosV(unit);
    t = FX_Div(posV << 12, GetCurrentLineLength(unit) << 12);
    p0 = GetCurrentRailLinePoint1(unit);
    p1 = GetCurrentRailLinePoint2(unit);
    start = &p0->pos;
    VEC_Subtract(&p1->pos, start, &dir);
    VEC_MultAdd(t, &dir, start, out);
    {
        VecFx32 up = { 0, FX32_ONE, 0 };
        VecFx32 side;

        posH = GetCurrentLinePosH(unit);
        width = RailUnit_GetTileWidth(unit);
        VEC_CrossProduct(&dir, &up, &side);
        VEC_Normalize(&side, &side);
        VEC_MultAdd(FX_Mul(posH << 12, width), &side, out, out);
    }
}

void RailMath_RailToWorldSlerpXZ(RailUnit *unit, VecFx32 *out) {
    VecFx32 *pos0;
    VecFx32 *pos1;
    RailCurve *curve;
    VecFx32 center;
    VecFx32 a;
    VecFx32 b;
    VecFx32 interp;
    VecFx32 dir;
    VecFx32 side;
    VecFx32 cross;
    s32 posV;
    s32 length;
    fx32 t;
    s32 posH;
    s32 width;
    fx32 offset;

    GetCurrentLine(unit);
    pos0 = &GetCurrentRailLinePoint1(unit)->pos;
    pos1 = &GetCurrentRailLinePoint2(unit)->pos;
    posV = GetCurrentLinePosV(unit);
    length = GetCurrentLineLength(unit);
    t = FX_Div(posV << 12, length << 12);
    out->y = pos1->y - pos0->y;
    out->y = FX_Div(FX_Mul(out->y, posV << 12), length << 12);
    out->y += pos0->y;
    curve = RailUnit_GetCurve(unit);
    center.x = curve->center.x;
    center.y = out->y;
    center.z = curve->center.z;
    VEC_Subtract(pos0, &center, &a);
    VEC_Subtract(pos1, &center, &b);
    a.y = 0;
    b.y = 0;
    VEC_Subtract(&b, &a, &dir);
    dir.y = 0;
    VEC_Normalize(&dir, &dir);
    InterpolateRailPointsGlobal(&interp, &a, &b, t);
    VEC_Add(&center, &interp, out);
    posH = GetCurrentLinePosH(unit);
    width = RailUnit_GetTileWidth(unit);
    offset = FX_Mul(posH << 12, width);
    VEC_Normalize(&interp, &side);
    VEC_CrossProduct(&side, &dir, &cross);
    if (cross.y < 0) {
        VEC_MultAdd(-offset, &side, out, out);
    } else {
        VEC_MultAdd(offset, &side, out, out);
    }
}

void RailMath_RailToWorldSlerpXYZ(RailUnit *unit, VecFx32 *out) {
    RailPoint *p0;
    RailPoint *p1;
    VecFx32 *pos0;
    VecFx32 *pos1;
    RailCurve *curve;
    VecFx32 center;
    VecFx32 a;
    VecFx32 b;
    VecFx32 interp;
    VecFx32 dir;
    VecFx32 side;
    VecFx32 cross;
    fx32 t;
    s32 posV;
    s32 length;
    s32 posH;
    s32 width;
    fx32 offset;

    GetCurrentLine(unit);
    p0 = GetCurrentRailLinePoint1(unit);
    p1 = GetCurrentRailLinePoint2(unit);
    pos0 = &p0->pos;
    pos1 = &p1->pos;
    posV = GetCurrentLinePosV(unit);
    length = GetCurrentLineLength(unit);
    t = FX_Div(posV << 12, length << 12);
    curve = RailUnit_GetCurve(unit);
    center.x = curve->center.x;
    center.y = curve->center.y;
    center.z = curve->center.z;
    VEC_Subtract(pos0, &center, &a);
    VEC_Subtract(pos1, &center, &b);
    VEC_Subtract(&b, &a, &dir);
    VEC_Normalize(&dir, &dir);
    InterpolateRailPointsGlobal(&interp, &a, &b, t);
    VEC_Add(&center, &interp, out);
    posH = GetCurrentLinePosH(unit);
    width = RailUnit_GetTileWidth(unit);
    offset = FX_Mul(posH << 12, width);
    if (interp.x != 0 && interp.z != 0) {
        interp.y = 0;
    } else {
        VecFx32 up = { 0, FX32_ONE, 0 };

        VEC_Subtract(pos1, pos0, &interp);
        VEC_CrossProduct(&interp, &up, &interp);
    }
    VEC_Normalize(&interp, &side);
    VEC_CrossProduct(&side, &dir, &cross);
    if (cross.y < 0) {
        VEC_MultAdd(-offset, &side, out, out);
    } else {
        VEC_MultAdd(offset, &side, out, out);
    }
}

BOOL RailMath_WorldToRailNoCurve(u32 lineId, FieldRailSystem *rail, const VecFx32 *pos, RailPosition *outPos,
                                 VecFx32 *outWorld) {
    RailLine *line = FieldRailSystem_GetLine(rail, lineId);
    RailPoint *p0 = FieldRailSystem_GetPoint(rail, line->pointA);
    RailPoint *p1 = FieldRailSystem_GetPoint(rail, line->pointB);
    fx32 tileWidth = FieldRailSystem_GetTileWidth(rail);
    VecFx32 hit;
    VecFx32 cross;
    VecFx32 a;
    VecFx32 b;
    RailPosition check;
    fx32 along;
    u32 front;
    s32 length;
    s32 base;
    s32 w0;
    s32 w1;
    s32 i;

    if (!func_ov036_021b3830(p0, p1, pos, &hit, &along, &cross)) {
        return FALSE;
    }
    a.x = pos->x;
    a.z = pos->z;
    b.x = hit.x;
    b.z = hit.z;
    a.y = 0;
    b.y = 0;
    front = FX_Div(VEC_Distance(&a, &b), tileWidth) >> 12;
    GetRailHnd2Curve(rail, line->curveId);
    length = line->length;
    base = length * 16 - (FX_Div(along, tileWidth) >> 12);
    for (i = 0; i < 4; i++) {
        if (lineId == p0->lineIds[i]) {
            w0 = p0->widths[i];
        }
        if (lineId == p1->lineIds[i]) {
            w1 = p1->widths[i];
        }
    }
    if (front <= w0 + (u32)(base * (w1 - w0)) / (u32)(length * 16) + 0x10) {
        outPos->componentIsLine = 1;
        outPos->componentId = lineId;
        outPos->railDirection = 3;
        if (cross.y < 0) {
            outPos->posSide = -func_ov036_021b0d2c(front);
        } else {
            outPos->posSide = func_ov036_021b0d2c(front);
        }
        outPos->posFront = func_ov036_021b0d2c(base);
        func_ov036_021b0c04(rail, outPos, &check);
        func_ov036_021b06ec(rail, &check, outWorld);
        if (GFL_STD_MemCmp(outPos, &check, sizeof(RailPosition)) == 0) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

BOOL RailMath_WorldToRailSlerp(u32 lineId, FieldRailSystem *rail, const VecFx32 *pos, RailPosition *outPos,
                               VecFx32 *outWorld) {
    RailLine *line = FieldRailSystem_GetLine(rail, lineId);
    RailPoint *p0 = FieldRailSystem_GetPoint(rail, line->pointA);
    RailPoint *p1 = FieldRailSystem_GetPoint(rail, line->pointB);
    VecFx32 hit;
    VecFx32 cross;
    VecFx32 cur;
    VecFx32 prev;
    VecFx32 d;
    VecFx32 e;
    VecFx32 dn;
    VecFx32 n1;
    VecFx32 n2;
    VecFx32 crossStep;
    VecFx32 a;
    VecFx32 b;
    RailPosition check;
    fx32 along;
    RailUnit *unit;
    u32 length;
    fx32 tileWidth;
    u32 step;

    if (!func_ov036_021b3830(p0, p1, pos, &hit, &along, &cross)) {
        return FALSE;
    }
    step = 1;
    outPos->componentIsLine = 1;
    outPos->railDirection = 3;
    outPos->componentId = lineId;
    outPos->posSide = 0;
    outPos->posFront = 0;
    unit = func_ov036_021b0c00(rail);
    RailUnit_SetRailPos(unit, outPos);
    length = GetCurrentLineLength(unit);
    RailUnit_GetCalcPos(unit, &cur);
    prev = cur;
    tileWidth = FieldRailSystem_GetTileWidth(rail);
    if (length >> 4 > 1) {
        do {
            s32 width;
            fx32 mag;
            fx32 ratio;
            fx32 dot;

            outPos->posFront = step;
            RailUnit_SetRailPos(unit, outPos);
            RailUnit_GetCalcPos(unit, &cur);
            width = func_ov036_021b0cb8(unit);
            VEC_Subtract(&prev, &cur, &d);
            VEC_Subtract(pos, &cur, &e);
            VEC_Normalize(&d, &dn);
            mag = VEC_Mag(&d);
            VEC_Normalize(&d, &n1);
            VEC_Normalize(&e, &n2);
            VEC_CrossProduct(&n1, &n2, &crossStep);
            d = dn;
            ratio = FX_Div(VEC_Mag(&e), mag);
            VEC_Normalize(&e, &e);
            e.x = FX_Mul(e.x, ratio);
            e.y = FX_Mul(e.y, ratio);
            e.z = FX_Mul(e.z, ratio);
            dot = VEC_DotProduct(&d, &e);
            if (dot <= 0x1000 && dot >= 0) {
                fx32 t = FX_Mul(dot, mag);

                hit.x = FX_Mul(dn.x, t);
                hit.y = FX_Mul(dn.y, t);
                hit.z = FX_Mul(dn.z, t);
                VEC_Add(&cur, &hit, &hit);
                b.x = pos->x;
                b.y = 0;
                b.z = pos->z;
                a.x = hit.x;
                a.y = 0;
                a.z = hit.z;
                if ((u32)(FX_Div(VEC_Distance(&b, &a), tileWidth) >> 12) <= width + 0x10) {
                    if (crossStep.y < 0) {
                        outPos->posSide = -func_ov036_021b0d2c(FX_Div(VEC_Distance(&b, &a), tileWidth) >> 12);
                    } else {
                        outPos->posSide = func_ov036_021b0d2c(FX_Div(VEC_Distance(&b, &a), tileWidth) >> 12);
                    }
                    func_ov036_021b0c04(rail, outPos, &check);
                    func_ov036_021b06ec(rail, outPos, outWorld);
                    if (GFL_STD_MemCmp(outPos, &check, sizeof(RailPosition)) != 0) {
                        return FALSE;
                    }
                    return TRUE;
                }
            }
            prev = cur;
            step++;
        } while (step < length >> 4);
    }
    return FALSE;
}

void RailCameraFuncSetStaticPYR(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailCamera *cam = GetRailCameraByCamHandler(unit);
    VecFx32 eye;

    FieldRailSystem_GetCameraParentPos(rail, &eye);
    FieldCamera_SetTransformType(FieldRailSystem_GetFieldCamera(rail), 2);
    eye.x += cam->unk4;
    eye.y += cam->unk8;
    eye.z += cam->unkC;
    FieldCamera_CoordsSetEye(FieldRailSystem_GetFieldCamera(rail), &eye);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
}

void RailCameraFuncSetStaticPYDist(FieldRailSystem *rail) {
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    RailCamera *cam = GetRailCameraByCamHandler(FieldRailSystem_GetCameraParentUnit(rail));
    VecFx32 pos;
    u16 pitch;
    s32 zoom;

    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
    FieldRailSystem_GetCameraParentPos(rail, &pos);
    pitch = cam->unk4;
    zoom = cam->unkC;
    FieldCamera_CoordsSetYaw(camera, cam->unk8);
    FieldCamera_CoordsSetPitch(camera, pitch);
    FieldCamera_CoordsSetZoom(camera, zoom);
}

void func_ov036_021b2cc4(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 target;

    RailCameraFuncSetStaticPYDist(rail);
    FieldCamera_ClearBind(camera);
    func_ov036_021b0cc8(unit, &target);
    FieldCamera_CoordsSetTarget(camera, &target);
}

void func_ov036_021b2cfc(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailPoint *p0 = GetCurrentRailLinePoint1(unit);
    RailPoint *p1 = GetCurrentRailLinePoint2(unit);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    RailCamera *c0;
    RailCamera *c1;
    s32 length;
    s32 posV;
    fx32 t;
    u16 yaw0;
    u16 pitch0;
    s32 zoom0;
    s32 yaw1;
    s32 zoom1;
    s32 diff;

    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
    c0 = GetRailCameraByPoint(unit, p0);
    c1 = GetRailCameraByPoint(unit, p1);
    length = GetCurrentLineLength(unit);
    posV = GetCurrentLinePosV(unit);
    t = FX_Div(posV << 12, length << 12);
    pitch0 = c0->unk4;
    yaw0 = c0->unk8;
    zoom0 = c0->unkC;
    yaw1 = c1->unk8;
    zoom1 = c1->unkC;
    diff = (u16)c1->unk4 - pitch0;
    if (diff >= 0x8000) {
        diff -= 0x10000;
    } else if (diff <= -0x8000) {
        diff += 0x10000;
    }
    {
        fx32 pitchStep = FX_Mul(diff << 12, t);

        diff = (u16)yaw1 - yaw0;
        if (diff >= 0x8000) {
            diff -= 0x10000;
        } else if (diff <= -0x8000) {
            diff += 0x10000;
        }
        u16 yaw = yaw0 + (FX_Mul(diff << 12, t) >> 12);
        u16 pitch = pitch0 + (pitchStep >> 12);

        FieldCamera_CoordsSetYaw(camera, yaw);
        FieldCamera_CoordsSetPitch(camera, pitch);
    }
    FieldCamera_CoordsSetZoom(camera, zoom0 + FX_Mul(zoom1 - zoom0, t));
}

void func_ov036_021b2e48(FieldRailSystem *rail) {
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 target;

    func_ov036_021b2cfc(rail);
    FieldCamera_ClearBind(camera);
    FieldRailSystem_GetCameraParentPos(rail, &target);
    FieldCamera_CoordsSetTarget(camera, &target);
}

void func_ov036_021b2e78(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 target;

    func_ov036_021b2cfc(rail);
    FieldCamera_ClearBind(camera);
    func_ov036_021b0cc8(unit, &target);
    FieldCamera_CoordsSetTarget(camera, &target);
}

void func_ov036_021b2eb0(FieldRailSystem *rail) {
    RailCamera *cam = GetRailCameraByCamHandler(FieldRailSystem_GetCameraParentUnit(rail));
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 eye;
    VecFx32 target;

    FieldCamera_SetTransformType(camera, 2);
    FieldCamera_ClearBind(FieldRailSystem_GetFieldCamera(rail));
    eye.x = cam->unk4;
    eye.y = cam->unk8;
    eye.z = cam->unkC;
    target.x = cam->unk10;
    target.y = cam->unk14;
    target.z = cam->unk18;
    FieldCamera_CoordsSetEye(camera, &eye);
    FieldCamera_CoordsSetTarget(camera, &target);
}

void func_ov036_021b2f04(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    RailPoint *p0 = GetCurrentRailLinePoint1(unit);
    RailPoint *p1 = GetCurrentRailLinePoint2(unit);
    RailCamera *c0 = GetRailCameraByPoint(unit, p0);
    RailCamera *c1 = GetRailCameraByPoint(unit, p1);
    VecFx32 eye;
    VecFx32 target;
    u32 length;
    s32 posV;

    FieldCamera_SetTransformType(camera, 2);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
    length = GetCurrentLineLength(unit);
    posV = GetCurrentLinePosV(unit);
    posV *= 0x1000;
    length *= 0x1000;
    eye.x = FX_Div(FX_Mul(c1->unk4 - c0->unk4, posV), length);
    eye.y = FX_Div(FX_Mul(c1->unk8 - c0->unk8, posV), length);
    eye.z = FX_Div(FX_Mul(c1->unkC - c0->unkC, posV), length);
    target.x = FX_Div(FX_Mul(c1->unk10 - c0->unk10, posV), length);
    target.y = FX_Div(FX_Mul(c1->unk14 - c0->unk14, posV), length);
    target.z = FX_Div(FX_Mul(c1->unk18 - c0->unk18, posV), length);
    eye.x += c0->unk4;
    eye.y += c0->unk8;
    eye.z += c0->unkC;
    target.x += c0->unk10;
    target.y += c0->unk14;
    target.z += c0->unk18;
    FieldCamera_CoordsSetEye(camera, &eye);
    FieldCamera_CoordsSetTarget(camera, &target);
}

void func_ov036_021b309c(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailCamera *c0;
    RailCamera *c1;
    s32 length;
    s32 posV;
    s32 pitchStep;
    s32 distStep;
    u16 angle;
    s32 dist;
    VecFx32 target;

    FieldRailSystem_GetFieldCamera(rail);
    if (RailUnit_IsOnLine(unit) == 1) {
        RailPoint *p0 = GetCurrentRailLinePoint1(unit);
        RailPoint *p1 = GetCurrentRailLinePoint2(unit);

        c0 = GetRailCameraByPoint(unit, p0);
        c1 = GetRailCameraByPoint(unit, p1);
        length = GetCurrentLineLength(unit);
        posV = GetCurrentLinePosV(unit);
    } else {
        c0 = GetRailCameraByCamHandler(unit);
        c1 = GetRailCameraByCamHandler(unit);
        length = 1;
        posV = 0;
    }
    pitchStep = posV * (c1->unk4 - c0->unk4) / length;
    angle = c0->unk4 + pitchStep;
    distStep = FX_Div(FX_Mul(c1->unk8 - c0->unk8, posV << 12), length << 12);
    dist = c0->unk8 + distStep;
    target.x = c0->unkC;
    target.y = c0->unk10;
    target.z = c0->unk14;
    func_ov036_021b3194(rail, angle, dist, &target);
}

void func_ov036_021b3164(FieldRailSystem *rail) {
    RailCamera *cam = GetRailCameraByCamHandler(FieldRailSystem_GetCameraParentUnit(rail));
    u16 angle = cam->unk4;
    s32 dist = cam->unk8;
    VecFx32 target = { cam->unkC, cam->unk10, cam->unk14 };

    func_ov036_021b3194(rail, angle, dist, &target);
}

static void func_ov036_021b3194(FieldRailSystem *rail, u16 angle, s32 dist, VecFx32 *target) {
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 eye;
    VecFx32 base;
    VecFx32 diff;
    VecFx32 dir;
    fx32 horiz;

    FieldCamera_SetTransformType(camera, 2);
    FieldCamera_ClearBind(camera);
    base = *target;
    eye.y = base.y;
    base.y = 0;
    FieldRailSystem_GetCameraParentPos(rail, &diff);
    diff.y = 0;
    VEC_Subtract(&diff, &base, &diff);
    VEC_Normalize(&diff, &dir);
    eye.y += FX_Mul(FX_SinIdx(angle), dist);
    horiz = FX_Mul(FX_CosIdx(angle), dist);
    eye.x = FX_Mul(dir.x, horiz) + base.x;
    eye.z = FX_Mul(dir.z, horiz) + base.z;
    FieldCamera_CoordsSetTarget(camera, target);
    FieldCamera_CoordsSetEye(camera, &eye);
}

void func_ov036_021b3294(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailCamera *cam;
    s32 length;
    s32 posV;
    u16 angle;
    s32 dist;
    VecFx32 target;

    RailUnit_IsOnLine(unit);
    length = GetCurrentLineLength(unit);
    posV = GetCurrentLinePosV(unit);
    cam = GetRailCameraByCamHandler(unit);
    angle = cam->unk4;
    dist = cam->unk8;
    target.x = cam->unkC + FX_Div(FX_Mul(cam->unk18 - cam->unkC, posV * 0x1000), length * 0x1000);
    target.y = cam->unk10 + FX_Div(FX_Mul(cam->unk1C - cam->unk10, posV * 0x1000), length * 0x1000);
    target.z = cam->unk14 + FX_Div(FX_Mul(cam->unk20 - cam->unk14, posV * 0x1000), length * 0x1000);
    func_ov036_021b3194(rail, angle, dist, &target);
}

void func_ov036_021b3368(FieldRailSystem *rail) {
    RailCamera *cam = GetRailCameraByCamHandler(FieldRailSystem_GetCameraParentUnit(rail));
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 base;
    VecFx32 pos;
    u16 yaw;

    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
    FieldRailSystem_GetCameraParentPos(rail, &pos);
    pos.y = 0;
    base.x = cam->unkC;
    base.y = 0;
    base.z = cam->unk10;
    VEC_Subtract(&pos, &base, &pos);
    VEC_Normalize(&pos, &pos);
    yaw = FX_Atan2Idx(pos.x, pos.z);
    FieldCamera_CoordsSetPitch(camera, cam->unk4);
    FieldCamera_CoordsSetZoom(camera, cam->unk8);
    FieldCamera_CoordsSetYaw(camera, yaw);
}

void func_ov036_021b33e4(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailCamera *cam = GetRailCameraByCamHandler(unit);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx16 dir;
    s32 yaw;

    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
    func_ov036_021b0a04(unit, &dir);
    yaw = FX_Atan2Idx(-dir.x, -dir.z);
    FieldCamera_CoordsSetPitch(camera, cam->unk8);
    FieldCamera_CoordsSetZoom(camera, cam->unkC);
    FieldCamera_CoordsSetYaw(camera, yaw + cam->unk4);
}

void func_ov036_021b3450(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailCamera *cam = GetRailCameraByCamHandler(unit);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx16 dir;
    VecFx16 flat;
    s32 yaw;
    s32 pitch;

    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
    func_ov036_021b0a04(unit, &dir);
    yaw = FX_Atan2Idx(-dir.x, -dir.z);
    flat.x = dir.x;
    flat.z = dir.z;
    flat.y = 0;
    pitch = FX_Atan2Idx(-dir.y, VEC_Fx16Mag(&flat));
    FieldCamera_CoordsSetPitch(camera, pitch + cam->unk8);
    FieldCamera_CoordsSetZoom(camera, cam->unkC);
    FieldCamera_CoordsSetYaw(camera, yaw + cam->unk4);
}

void func_ov036_021b34e4(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailCamera *cam = GetRailCameraByCamHandler(unit);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx16 dir;
    VecFx32 target;
    s32 yaw;

    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ClearBind(FieldRailSystem_GetFieldCamera(rail));
    func_ov036_021b0a04(unit, &dir);
    yaw = FX_Atan2Idx(-dir.x, -dir.z);
    FieldCamera_CoordsSetPitch(camera, cam->unk8);
    FieldCamera_CoordsSetZoom(camera, cam->unkC);
    FieldCamera_CoordsSetYaw(camera, yaw + cam->unk4);
    func_ov036_021b0cc8(unit, &target);
    FieldCamera_CoordsSetTarget(FieldRailSystem_GetFieldCamera(rail), &target);
}

void func_ov036_021b3568(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    u16 onLine = RailUnit_IsOnLine(unit);
    RailPoint *point = GetCurrentRailLinePoint1(unit);

    if (onLine == 1) {
        func_ov036_021b39c8(rail, unit, point);
    }
}

void func_ov036_021b3590(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    u16 onLine = RailUnit_IsOnLine(unit);
    RailPoint *point = GetCurrentRailLinePoint2(unit);

    if (onLine == 1) {
        func_ov036_021b39c8(rail, unit, point);
    }
}

void func_ov036_021b35b8(FieldRailSystem *rail) {
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    RailCamera *cam;
    VecFx32 pos;
    VecFx32 offset;
    u16 pitch;
    s32 zoom;

    cam = GetRailCameraByCamHandler(FieldRailSystem_GetCameraParentUnit(rail));
    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
    FieldRailSystem_GetCameraParentPos(rail, &pos);
    pitch = cam->unk4;
    zoom = cam->unkC;
    FieldCamera_CoordsSetYaw(camera, cam->unk8);
    FieldCamera_CoordsSetPitch(camera, pitch);
    FieldCamera_CoordsSetZoom(camera, zoom);
    offset.x = cam->unk10;
    offset.y = cam->unk14;
    offset.z = cam->unk18;
    FieldCamera_CoordsSetTargetOffset(camera, &offset);
}

void func_ov036_021b3628(FieldRailSystem *rail) {
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 target;

    func_ov036_021b35b8(rail);
    FieldCamera_ClearBind(camera);
    FieldRailSystem_GetCameraParentPos(rail, &target);
    FieldCamera_CoordsSetTarget(camera, &target);
    FieldCamera_EnableDelay(camera);
}

void func_ov036_021b365c(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailPoint *p0 = GetCurrentRailLinePoint1(unit);
    RailPoint *p1 = GetCurrentRailLinePoint2(unit);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    RailCamera *c0;
    RailCamera *c1;
    s32 length;
    s32 posV;
    fx32 t;
    u16 pitch0;
    u16 yaw0;
    s32 zoom0;
    s32 yaw1;
    s32 zoom1;
    s32 diff;
    VecFx32 offset;

    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
    c0 = GetRailCameraByPoint(unit, p0);
    c1 = GetRailCameraByPoint(unit, p1);
    length = GetCurrentLineLength(unit);
    posV = GetCurrentLinePosV(unit);
    t = FX_Div(posV << 12, length << 12);
    pitch0 = c0->unk4;
    yaw0 = c0->unk8;
    zoom0 = c0->unkC;
    yaw1 = c1->unk8;
    zoom1 = c1->unkC;
    diff = (u16)c1->unk4 - pitch0;
    if (diff >= 0x8000) {
        diff -= 0x10000;
    } else if (diff <= -0x8000) {
        diff += 0x10000;
    }
    {
        fx32 pitchStep = FX_Mul(diff << 12, t);

        diff = (u16)yaw1 - yaw0;
        if (diff >= 0x8000) {
            diff -= 0x10000;
        } else if (diff <= -0x8000) {
            diff += 0x10000;
        }
        u16 yaw = yaw0 + (FX_Mul(diff << 12, t) >> 12);
        u16 pitch = pitch0 + (pitchStep >> 12);

        FieldCamera_CoordsSetYaw(camera, yaw);
        FieldCamera_CoordsSetPitch(camera, pitch);
    }
    FieldCamera_CoordsSetZoom(camera, zoom0 + FX_Mul(zoom1 - zoom0, t));
    offset.x = c0->unk10 + FX_Mul(c1->unk10 - c0->unk10, t);
    offset.y = c0->unk14 + FX_Mul(c1->unk14 - c0->unk14, t);
    offset.z = c0->unk18 + FX_Mul(c1->unk18 - c0->unk18, t);
    FieldCamera_CoordsSetTargetOffset(camera, &offset);
}

static BOOL func_ov036_021b3830(RailPoint *p0, RailPoint *p1, const VecFx32 *pos, VecFx32 *out, fx32 *along,
                                VecFx32 *cross) {
    VecFx32 lineDir;
    VecFx32 toPos;
    VecFx32 ndir;
    VecFx32 a;
    VecFx32 b;
    fx32 len;
    fx32 mag;
    fx32 ratio;
    fx32 dot;
    fx32 dy;
    fx32 mult;

    VEC_Subtract(&p0->pos, &p1->pos, &lineDir);
    VEC_Subtract(pos, &p1->pos, &toPos);
    VEC_Normalize(&lineDir, &ndir);
    len = VEC_Mag(&lineDir);
    lineDir = ndir;
    mag = VEC_Mag(&toPos);
    VEC_Normalize(&toPos, &toPos);
    ratio = FX_Div(mag, len);
    toPos.x = FX_Mul(toPos.x, ratio);
    toPos.y = FX_Mul(toPos.y, ratio);
    toPos.z = FX_Mul(toPos.z, ratio);
    dot = VEC_DotProduct(&lineDir, &toPos);
    *along = dot;
    if (dot > 0x1000) {
        return FALSE;
    }
    if (dot < 0) {
        return FALSE;
    }
    *along = FX_Mul(dot, len);
    VEC_Normalize(&lineDir, &a);
    VEC_Normalize(&toPos, &b);
    VEC_CrossProduct(&a, &b, cross);
    mult = *along;
    out->x = FX_Mul(lineDir.x, mult);
    out->y = FX_Mul(lineDir.y, mult);
    out->z = FX_Mul(lineDir.z, mult);
    VEC_Add(&p1->pos, out, out);
    if (VEC_Distance(pos, out) > 0xfffff) {
        return FALSE;
    }
    dy = pos->y - out->y;
    if (dy < 0) {
        dy = -dy;
    }
    if (dy > 0x8000) {
        return FALSE;
    }
    return TRUE;
}

static void func_ov036_021b39c8(FieldRailSystem *rail, RailUnit *unit, RailPoint *point) {
    VecFx32 target;
    RailCamera *cam;
    FieldCamera *camera;
    s32 front;

    if (GetCurrentRailLinePoint1(unit) == point) {
        front = 0;
    } else {
        front = GetCurrentLineLength(unit);
    }
    func_ov036_021b0cdc(unit, front, &target);
    cam = GetRailCameraByPoint(unit, point);
    camera = FieldRailSystem_GetFieldCamera(rail);
    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ClearBind(camera);
    FieldCamera_CoordsSetTarget(camera, &target);
    FieldCamera_CoordsSetPitch(camera, cam->unk4);
    FieldCamera_CoordsSetYaw(camera, cam->unk8);
    FieldCamera_CoordsSetZoom(camera, cam->unkC);
}
