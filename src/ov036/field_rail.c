// The rail system of the no-grid maps: the rail units that move along the lines and points of a map's rails, and
// the camera the rails set. The name is the ROM's own, from GFL_HeapAllocate's file argument. The curves' conversions
// between rail and world positions and the camera functions that follow in the ROM are field_rail_func.c
#include "types.h"
#include "field/field_rail.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/mi.h"

static BOOL func_ov036_021b09fc(RailUnit *unit);
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
static void func_ov036_021b1bec(const RailUnit *unit, RailPosition *pos);
static s32 GetLineTileLengthWorldSpace(RailLine *line, fx32 tileWidth, const RailDataHandle *data);
static s32 GetLerpedLineWidthByLineYPos(RailLine *line, s32 y, s32 length, const RailDataHandle *data);
static u32 func_ov036_021b1cec(RailUnit *move, RailLine *line, s32 len, u32 arg);
static BOOL func_ov036_021b1d1c(FieldRailSystem *rail, RailPosition *pos, u32 dir, s32 extra, VecFx32 *out);
static u32 func_ov036_021b1d88(u32 dir);
static u32 func_ov036_021b1db8(u32 dir);
static u32 func_ov036_021b1de8(u32 dir);
static void func_ov036_021b1e18(RailDisabledLines *disabled);
static void func_ov036_021b1e2c(RailDisabledLines *disabled, u16 lineId);
static void func_ov036_021b1e50(RailDisabledLines *disabled, u16 lineId);
static BOOL func_ov036_021b1e74(RailDisabledLines *disabled, u16 lineId);
static BOOL func_ov036_021b1e8c(RailDisabledLines *disabled, RailLine *line, const RailDataHandle *data);
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
static int func_ov036_021b21b0(const RailDataHandle *data, RailLine *line);
static BOOL func_ov036_021b21dc(const VecFx32 *a, const VecFx32 *b, const VecFx32 *p3, const VecFx32 *p4,
                                const VecFx32 *p5, VecFx32 *out);

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

void FieldRailSystem_GetCameraParentPos(FieldRailSystem *rail, VecFx32 *pos) {
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

void func_ov036_021b0774(const RailUnit *unit, RailPosition *pos) {
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
    s32 savedFront;
    s32 length;
    s32 nextFront;
    RailPosition tmp;
    VecFx32 from;
    VecFx32 to;

    savedSide = unit->side;
    unit->side = 0;
    func_ov036_021b1bac(unit, &from);
    unit->side = savedSide;
    savedFront = unit->front;
    length = unit->length;
    nextFront = savedFront + 16;
    if (nextFront < length) {
        unit->front += 16;
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

RailUnit *func_ov036_021b0c00(FieldRailSystem *rail) {
    return rail->playerUnit;
}

void func_ov036_021b0c04(const FieldRailSystem *rail, const RailPosition *in, RailPosition *out) {
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

s32 func_ov036_021b0cb8(RailUnit *unit) {
    return unit->width;
}

fx32 RailUnit_GetTileWidth(RailUnit *unit) {
    return unit->tileWidth;
}

RailCamera *GetRailCameraByCamHandler(RailUnit *unit) {
    return GetRailCameraByHandlerImpl(unit);
}

void func_ov036_021b0cc8(RailUnit *unit, VecFx32 *pos) {
    s32 saved = unit->side;

    unit->side = 0;
    func_ov036_021b1bac(unit, pos);
    unit->side = saved;
}

void func_ov036_021b0cdc(RailUnit *unit, s32 front, VecFx32 *pos) {
    s32 saved = unit->front;

    unit->front = front;
    func_ov036_021b1bac(unit, pos);
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

s32 func_ov036_021b0d2c(s32 value) {
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

void SetWkToActorRailPos(const RailUnit *unit, u16 *a1, u16 *a2, u16 *a3) {
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
    RailPoint *point;

    if (!func_ov036_021b1f74(unit->disabled, line, front, side, unit->handle, unit->tileWidth)) {
        return 0;
    }
    if ((side < 0 ? -side : side) > width) {
        return 0;
    }
    if ((front >= length || front < 0)) {
        if (front >= length) {
            point = GetRailPoint(unit->handle, line->pointB);
            next = GetLineContinueInDirection(point, line->dir, unit->handle, unit->disabled, TRUE);
        } else {
            point = GetRailPoint(unit->handle, line->pointA);
            next =
                GetLineContinueInDirection(point, func_ov036_021b1d88(line->dir), unit->handle, unit->disabled, TRUE);
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
            s32 tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
            width = GetLerpedLineWidthByLineYPos(next0, m->front - len, tileLen, m->handle);
            r = func_ov036_021b0f00(m, next0, dir, m->front - len, m->side, tileLen, width);
            if (r != 0) {
                return r;
            }
        }
        if (next1 != NULL && m->side <= 0) {
            if (func_ov036_021b1028(m->handle, next1, point)) {
                func_ov036_021b2060(point, m->handle);
                s32 tileLen = GetLineTileLengthWorldSpace(next1, m->tileWidth, m->handle);
                width = GetLerpedLineWidthByLineYPos(next1, -m->side, tileLen, m->handle);
                r = func_ov036_021b0f00(m, next1, dir, -m->side, m->front - len, tileLen, width);
                if (r != 0) {
                    return r;
                }
            } else if (func_ov036_021b1040(m->handle, next1, point)) {
                func_ov036_021b2060(point, m->handle);
                s32 tileLen = GetLineTileLengthWorldSpace(next1, m->tileWidth, m->handle);
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
                s32 tileLen = GetLineTileLengthWorldSpace(next2, m->tileWidth, m->handle);
                width = GetLerpedLineWidthByLineYPos(next2, m->side, tileLen, m->handle);
                r = func_ov036_021b0f00(m, next2, dir, m->side, -(m->front - len), tileLen, width);
                if (r != 0) {
                    return r;
                }
            } else if (func_ov036_021b1040(m->handle, next2, point)) {
                func_ov036_021b2060(point, m->handle);
                s32 tileLen = GetLineTileLengthWorldSpace(next2, m->tileWidth, m->handle);
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
                        s32 tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
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
                        s32 tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
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
                        s32 tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
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
                        s32 tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
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
                    s32 tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
                    width = GetLerpedLineWidthByLineYPos(next0, m->side, tileLen, m->handle);
                    r = func_ov036_021b0f00(m, next0, dir, m->side, -m->front, tileLen, width);
                    if (r != 0) {
                        return r;
                    }
                }
                if (func_ov036_021b1040(m->handle, next0, point)) {
                    func_ov036_021b2060(point, m->handle);
                    s32 tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
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
                    s32 tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
                    width = GetLerpedLineWidthByLineYPos(next0, m->side, tileLen, m->handle);
                    r = func_ov036_021b0f00(m, next0, dir, m->side, len - m->front, tileLen, width);
                    if (r != 0) {
                        return r;
                    }
                }
                if (func_ov036_021b1040(m->handle, next0, point)) {
                    func_ov036_021b2060(point, m->handle);
                    s32 tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
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
        s32 tileLen = GetLineTileLengthWorldSpace(next2, m->tileWidth, m->handle);
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
            s32 tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
            width = GetLerpedLineWidthByLineYPos(next0, -m->side, tileLen, m->handle);
            r = func_ov036_021b0f00(m, next0, dir, -m->side, m->front, tileLen, width);
            if (r != 0) {
                return r;
            }
        } else if (func_ov036_021b1040(m->handle, next0, point)) {
            func_ov036_021b2060(point, m->handle);
            s32 tileLen = GetLineTileLengthWorldSpace(next0, m->tileWidth, m->handle);
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
            s32 tileLen = GetLineTileLengthWorldSpace(next1, m->tileWidth, m->handle);
            width = GetLerpedLineWidthByLineYPos(next1, m->side, tileLen, m->handle);
            r = func_ov036_021b0f00(m, next1, dir, m->side, -m->front, tileLen, width);
            if (r != 0) {
                return r;
            }
        }
        if (func_ov036_021b1040(m->handle, next1, point)) {
            func_ov036_021b2060(point, m->handle);
            s32 tileLen = GetLineTileLengthWorldSpace(next1, m->tileWidth, m->handle);
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

static void func_ov036_021b1bec(const RailUnit *unit, RailPosition *pos) {
    u16 onLine = unit->onLine;
    s32 id;

    pos->componentIsLine = onLine;
    if (onLine == 0) {
        id = func_ov036_021b2184(unit->handle, unit->component.point);
    } else {
        id = func_ov036_021b21b0(unit->handle, unit->component.line);
    }
    pos->componentId = id;
    pos->posFront = unit->front / 16;
    pos->posSide = unit->side / 16;
    pos->railDirection = unit->dir;
}

static s32 GetLineTileLengthWorldSpace(RailLine *line, fx32 tileWidth, const RailDataHandle *data) {
    return line->length << 4;
}

static s32 GetLerpedLineWidthByLineYPos(RailLine *line, s32 y, s32 length, const RailDataHandle *data) {
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

static BOOL func_ov036_021b1e8c(RailDisabledLines *disabled, RailLine *line, const RailDataHandle *data) {
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

static int func_ov036_021b21b0(const RailDataHandle *data, RailLine *line) {
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
    if (t > len) {
        return FALSE;
    }
    if (t <= 0x1000) {
        return FALSE;
    }
    VEC_Set(out, p4->x + FX_Mul(dir.x, t), p4->y + FX_Mul(dir.y, t), p4->z + FX_Mul(dir.z, t));
    return TRUE;
}
