// The functions the rail data picks by ID: the curves' conversions between rail and world positions, and the rails'
// camera functions. The name is descriptive: the ROM names neither this file nor its functions' source. The size sort
// sets it apart from field_rail.c, whose .rodata ends with the default camera and curve: with the two local up vectors
// of RailMath_RailToWorldNoCurve and RailMath_RailToWorldSlerpXYZ in field_rail.c's sort, its first up vector can only
// come before both defaults as a shared const, and with them here, as a local initializer like the others.
// FieldRailSystem_IntersectSegmentPlane, which only field_rail.c's CalculateRailCurves calls, could open this file as
// well RailMath_SlerpDirections, RailMath_TryProjectWorldPointToSegment and the progress helper are our descriptive
// names.
#include "types.h"
#include "field/field_camera.h"
#include "field/field_rail.h"
#include "gfl/std.h"
#include "nitro/fx.h"

// Position and length in the line's own units, before fixed-point conversion.
typedef struct {
    s32 position;
    s32 length;
} RailLineProgress;

static inline fx32 RailLineProgress_GetRatio(const RailLineProgress *progress) {
    return FX_Div(progress->position << FX32_SHIFT, progress->length << FX32_SHIFT);
}

static void RailMath_SlerpDirections(VecFx32 *out, const VecFx32 *a, const VecFx32 *b, fx32 t);
static void InterpolateRailPointsGlobal(VecFx32 *out, const VecFx32 *a, const VecFx32 *b, fx32 t);
static void FieldRailCamera_SetOrbit(FieldRailSystem *rail, u16 angle, s32 dist, VecFx32 *target);
static BOOL RailMath_TryProjectWorldPointToSegment(RailPoint *p0, RailPoint *p1, const VecFx32 *pos, VecFx32 *out,
                                                   fx32 *along, VecFx32 *cross);
static void FieldRailCamera_ApplyPoint(FieldRailSystem *rail, RailUnit *unit, RailPoint *point);

static void RailMath_SlerpDirections(VecFx32 *out, const VecFx32 *a, const VecFx32 *b, fx32 t) {
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

    RailMath_SlerpDirections(out, a, b, t);
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
    RailLineProgress progress;
    s32 posH;
    s32 width;

    GetCurrentLine(unit);
    progress.position = GetCurrentLinePosV(unit);
    progress.length = GetCurrentLineLength(unit);
    t = RailLineProgress_GetRatio(&progress);
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
    RailPoint *point0;
    RailPoint *point1;
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
    RailLineProgress progress;
    fx32 t;
    s32 posH;
    s32 width;
    fx32 offset;

    GetCurrentLine(unit);
    point0 = GetCurrentRailLinePoint1(unit);
    point1 = GetCurrentRailLinePoint2(unit);
    pos0 = &point0->pos;
    pos1 = &point1->pos;
    progress.position = GetCurrentLinePosV(unit);
    progress.length = GetCurrentLineLength(unit);
    t = RailLineProgress_GetRatio(&progress);
    out->y = pos1->y - pos0->y;
    out->y = FX_Div(FX_Mul(out->y, progress.position << FX32_SHIFT), progress.length << FX32_SHIFT);
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
    RailLineProgress progress;
    s32 posH;
    s32 width;
    fx32 offset;

    GetCurrentLine(unit);
    p0 = GetCurrentRailLinePoint1(unit);
    p1 = GetCurrentRailLinePoint2(unit);
    pos0 = &p0->pos;
    pos1 = &p1->pos;
    progress.position = GetCurrentLinePosV(unit);
    progress.length = GetCurrentLineLength(unit);
    t = RailLineProgress_GetRatio(&progress);
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
    fx32 along;
    RailPosition check;
    s32 w0;
    s32 w1;
    s32 length;
    u32 front;
    s32 base;
    s32 i;

    if (!RailMath_TryProjectWorldPointToSegment(p0, p1, pos, &hit, &along, &cross)) {
        return FALSE;
    }
    VEC_Set(&a, pos->x, 0, pos->z);
    b.x = hit.x;
    b.z = hit.z;
    b.y = 0;
    front = FX_Div(VEC_Distance(&a, &b), tileWidth) >> 12;
    GetRailHnd2Curve(rail, line->curveId);
    length = line->length * 16;
    base = length - (FX_Div(along, tileWidth) >> 12);
    for (i = 0; i < 4; i++) {
        if (lineId == p0->lineIds[i]) {
            w0 = p0->widths[i];
        }
        if (lineId == p1->lineIds[i]) {
            w1 = p1->widths[i];
        }
    }
    if (front <= w0 + (u32)(base * (w1 - w0)) / (u32)length + 0x10) {
        outPos->componentIsLine = 1;
        outPos->componentId = lineId;
        outPos->railDirection = RAIL_DIR_DOWN;
        if (cross.y >= 0) {
            outPos->posSide = FieldRailSystem_RoundToGrid(front);
        } else {
            outPos->posSide = -FieldRailSystem_RoundToGrid(front);
        }
        outPos->posFront = FieldRailSystem_RoundToGrid(base);
        FieldRailSystem_ClampLocation(rail, outPos, &check);
        FieldRailSystem_GetLocationPos(rail, &check, outWorld);
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
    VecFx32 cur;
    VecFx32 prev;
    VecFx32 d;
    VecFx32 e;
    VecFx32 dn;
    VecFx32 hit;
    VecFx32 cross;
    VecFx32 b;
    VecFx32 a;
    VecFx32 n1;
    VecFx32 n2;
    fx32 along;
    RailPosition check;
    RailUnit *unit;
    u32 length;
    fx32 tileWidth;
    u32 step;

    if (!RailMath_TryProjectWorldPointToSegment(p0, p1, pos, &hit, &along, &cross)) {
        return FALSE;
    }
    step = 1;
    outPos->componentIsLine = 1;
    outPos->railDirection = RAIL_DIR_DOWN;
    outPos->componentId = lineId;
    outPos->posSide = 0;
    outPos->posFront = 0;
    unit = FieldRailSystem_GetProbeWork(rail);
    RailUnit_SetRailPos(unit, outPos);
    length = (u32)GetCurrentLineLength(unit) >> 4;
    RailUnit_GetCalcPos(unit, &cur);
    prev.x = cur.x;
    prev.y = cur.y;
    prev.z = cur.z;
    tileWidth = FieldRailSystem_GetTileWidth(rail);
    if (length > 1) {
        do {
            s32 width;
            fx32 mag;
            fx32 ratio;
            fx32 dot;

            outPos->posFront = step;
            RailUnit_SetRailPos(unit, outPos);
            RailUnit_GetCalcPos(unit, &cur);
            width = RailUnit_GetWidthLimit(unit);
            VEC_Subtract(&prev, &cur, &d);
            VEC_Subtract(pos, &cur, &e);
            VEC_Normalize(&d, &dn);
            mag = VEC_Mag(&d);
            VEC_Normalize(&d, &n1);
            VEC_Normalize(&e, &n2);
            VEC_CrossProduct(&n1, &n2, &cross);
            d = dn;
            ratio = VEC_Mag(&e);
            VEC_Normalize(&e, &e);
            ratio = FX_Div(ratio, mag);
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
                VEC_Set(&b, pos->x, 0, pos->z);
                a.x = hit.x;
                a.y = 0;
                a.z = hit.z;
                u32 front = FX_Div(VEC_Distance(&b, &a), tileWidth) >> 12;
                if (width + 0x10 >= front) {
                    if (cross.y >= 0) {
                        outPos->posSide = FieldRailSystem_RoundToGrid(front);
                    } else {
                        outPos->posSide = -FieldRailSystem_RoundToGrid(front);
                    }
                    FieldRailSystem_ClampLocation(rail, outPos, &check);
                    FieldRailSystem_GetLocationPos(rail, outPos, outWorld);
                    if (GFL_STD_MemCmp(outPos, &check, sizeof(RailPosition)) == 0) {
                        return TRUE;
                    }
                    return FALSE;
                }
            }
            prev.x = cur.x;
            prev.y = cur.y;
            prev.z = cur.z;
            step++;
        } while (step < length);
    }
    return FALSE;
}

void RailCameraFuncSetStaticPYR(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailCamera *cam = GetRailCameraByCamHandler(unit);
    VecFx32 eye;

    FieldRailSystem_GetCameraParentPos(rail, &eye);
    FieldCamera_SetTransformType(FieldRailSystem_GetFieldCamera(rail), 2);
    eye.x += cam->params[0];
    eye.y += cam->params[1];
    eye.z += cam->params[2];
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
    pitch = cam->params[0];
    zoom = cam->params[2];
    FieldCamera_CoordsSetYaw(camera, cam->params[1]);
    FieldCamera_CoordsSetPitch(camera, pitch);
    FieldCamera_CoordsSetZoom(camera, zoom);
}

void FieldRailCamera_FixedAngleAtCentre(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 target;

    RailCameraFuncSetStaticPYDist(rail);
    FieldCamera_ClearBind(camera);
    RailUnit_GetCentrePos(unit, &target);
    FieldCamera_CoordsSetTarget(camera, &target);
}

void FieldRailCamera_BlendAngles(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailPoint *p0 = GetCurrentRailLinePoint1(unit);
    RailPoint *p1 = GetCurrentRailLinePoint2(unit);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    RailCamera *c0;
    RailCamera *c1;
    s32 length;
    s32 posV;
    fx32 t;
    s32 pitch0;
    s32 yaw0;
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
    pitch0 = (u16)c0->params[0];
    yaw0 = (u16)c0->params[1];
    zoom0 = c0->params[2];
    yaw1 = (u16)c1->params[1];
    zoom1 = c1->params[2];
    diff = (u16)c1->params[0] - pitch0;
    if (diff >= 0x8000) {
        diff -= 0x10000;
    } else if (diff <= -0x8000) {
        diff += 0x10000;
    }
    {
        u16 pitch = pitch0 + (FX_Mul(diff << 12, t) >> 12);

        diff = yaw1 - yaw0;
        if (diff >= 0x8000) {
            diff -= 0x10000;
        } else if (diff <= -0x8000) {
            diff += 0x10000;
        }
        u16 yaw = yaw0 + (FX_Mul(diff << 12, t) >> 12);

        FieldCamera_CoordsSetYaw(camera, yaw);
        FieldCamera_CoordsSetPitch(camera, pitch);
    }
    FieldCamera_CoordsSetZoom(camera, zoom0 + FX_Mul(zoom1 - zoom0, t));
}

void FieldRailCamera_BlendAnglesAtFocus(FieldRailSystem *rail) {
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 target;

    FieldRailCamera_BlendAngles(rail);
    FieldCamera_ClearBind(camera);
    FieldRailSystem_GetCameraParentPos(rail, &target);
    FieldCamera_CoordsSetTarget(camera, &target);
}

void FieldRailCamera_BlendAnglesAtCentre(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 target;

    FieldRailCamera_BlendAngles(rail);
    FieldCamera_ClearBind(camera);
    RailUnit_GetCentrePos(unit, &target);
    FieldCamera_CoordsSetTarget(camera, &target);
}

void FieldRailCamera_FixedEyeTarget(FieldRailSystem *rail) {
    RailCamera *cam = GetRailCameraByCamHandler(FieldRailSystem_GetCameraParentUnit(rail));
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 eye;
    VecFx32 target;

    FieldCamera_SetTransformType(camera, 2);
    FieldCamera_ClearBind(FieldRailSystem_GetFieldCamera(rail));
    eye.x = cam->params[0];
    eye.y = cam->params[1];
    eye.z = cam->params[2];
    target.x = cam->params[3];
    target.y = cam->params[4];
    target.z = cam->params[5];
    FieldCamera_CoordsSetEye(camera, &eye);
    FieldCamera_CoordsSetTarget(camera, &target);
}

void FieldRailCamera_BlendEyeTarget(FieldRailSystem *rail) {
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
    FieldCamera_ClearBind(FieldRailSystem_GetFieldCamera(rail));
    length = GetCurrentLineLength(unit);
    posV = GetCurrentLinePosV(unit);
    posV *= 0x1000;
    length *= 0x1000;
    eye.x = FX_Div(FX_Mul(c1->params[0] - c0->params[0], posV), length);
    eye.y = FX_Div(FX_Mul(c1->params[1] - c0->params[1], posV), length);
    eye.z = FX_Div(FX_Mul(c1->params[2] - c0->params[2], posV), length);
    target.x = FX_Div(FX_Mul(c1->params[3] - c0->params[3], posV), length);
    target.y = FX_Div(FX_Mul(c1->params[4] - c0->params[4], posV), length);
    target.z = FX_Div(FX_Mul(c1->params[5] - c0->params[5], posV), length);
    eye.x += c0->params[0];
    eye.y += c0->params[1];
    eye.z += c0->params[2];
    target.x += c0->params[3];
    target.y += c0->params[4];
    target.z += c0->params[5];
    FieldCamera_CoordsSetEye(camera, &eye);
    FieldCamera_CoordsSetTarget(camera, &target);
}

void FieldRailCamera_BlendOrbit(FieldRailSystem *rail) {
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
    if (RailUnit_IsOnLine(unit) == RAIL_COMPONENT_LINE) {
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
    pitchStep = posV * (c1->params[0] - c0->params[0]) / length;
    angle = c0->params[0] + pitchStep;
    distStep = FX_Div(FX_Mul(c1->params[1] - c0->params[1], posV << 12), length << 12);
    dist = c0->params[1] + distStep;
    target.x = c0->params[2];
    target.y = c0->params[3];
    target.z = c0->params[4];
    FieldRailCamera_SetOrbit(rail, angle, dist, &target);
}

void FieldRailCamera_Orbit(FieldRailSystem *rail) {
    RailCamera *cam = GetRailCameraByCamHandler(FieldRailSystem_GetCameraParentUnit(rail));
    u16 angle = cam->params[0];
    s32 dist = cam->params[1];
    VecFx32 target = { cam->params[2], cam->params[3], cam->params[4] };

    FieldRailCamera_SetOrbit(rail, angle, dist, &target);
}

static void FieldRailCamera_SetOrbit(FieldRailSystem *rail, u16 angle, s32 dist, VecFx32 *target) {
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 diff;
    VecFx32 base;
    VecFx32 dir;
    VecFx32 eye;
    fx32 height;
    fx32 horiz;

    FieldCamera_SetTransformType(camera, 2);
    FieldCamera_ClearBind(camera);
    base = *target;
    height = base.y;
    base.y = 0;
    FieldRailSystem_GetCameraParentPos(rail, &diff);
    diff.y = 0;
    VEC_Subtract(&diff, &base, &diff);
    VEC_Normalize(&diff, &dir);
    eye.y = FX_Mul(FX_SinIdx(angle), dist);
    horiz = FX_Mul(FX_CosIdx(angle), dist);
    eye.x = FX_Mul(dir.x, horiz);
    eye.z = FX_Mul(dir.z, horiz);
    eye.x += base.x;
    eye.y += height;
    eye.z += base.z;
    FieldCamera_CoordsSetTarget(camera, target);
    FieldCamera_CoordsSetEye(camera, &eye);
}

void FieldRailCamera_OrbitMovingPoint(FieldRailSystem *rail) {
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
    angle = cam->params[0];
    dist = cam->params[1];
    target.x = cam->params[2] + FX_Div(FX_Mul(cam->params[5] - cam->params[2], posV * 0x1000), length * 0x1000);
    target.y = cam->params[3] + FX_Div(FX_Mul(cam->params[6] - cam->params[3], posV * 0x1000), length * 0x1000);
    target.z = cam->params[4] + FX_Div(FX_Mul(cam->params[7] - cam->params[4], posV * 0x1000), length * 0x1000);
    FieldRailCamera_SetOrbit(rail, angle, dist, &target);
}

void FieldRailCamera_YawFromPoint(FieldRailSystem *rail) {
    RailCamera *cam = GetRailCameraByCamHandler(FieldRailSystem_GetCameraParentUnit(rail));
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 base;
    VecFx32 pos;
    u16 yaw;

    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
    FieldRailSystem_GetCameraParentPos(rail, &pos);
    pos.y = 0;
    base.x = cam->params[2];
    base.y = 0;
    base.z = cam->params[3];
    VEC_Subtract(&pos, &base, &pos);
    VEC_Normalize(&pos, &pos);
    yaw = FX_Atan2Idx(pos.x, pos.z);
    FieldCamera_CoordsSetPitch(camera, cam->params[0]);
    FieldCamera_CoordsSetZoom(camera, cam->params[1]);
    FieldCamera_CoordsSetYaw(camera, yaw);
}

void FieldRailCamera_FollowHeading(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailCamera *cam = GetRailCameraByCamHandler(unit);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx16 dir;
    s32 yaw;

    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
    RailUnit_GetStepVector(unit, &dir);
    yaw = FX_Atan2Idx(-dir.x, -dir.z);
    FieldCamera_CoordsSetPitch(camera, cam->params[1]);
    FieldCamera_CoordsSetZoom(camera, cam->params[2]);
    FieldCamera_CoordsSetYaw(camera, yaw + cam->params[0]);
}

void FieldRailCamera_FollowHeadingSlope(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailCamera *cam = GetRailCameraByCamHandler(unit);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx16 dir;
    VecFx16 flat;
    s32 yaw;
    s32 pitch;

    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ResetBind(FieldRailSystem_GetFieldCamera(rail));
    RailUnit_GetStepVector(unit, &dir);
    yaw = FX_Atan2Idx(-dir.x, -dir.z);
    flat.x = dir.x;
    flat.z = dir.z;
    flat.y = 0;
    pitch = FX_Atan2Idx(-dir.y, VEC_Fx16Mag(&flat));
    FieldCamera_CoordsSetPitch(camera, pitch + cam->params[1]);
    FieldCamera_CoordsSetZoom(camera, cam->params[2]);
    FieldCamera_CoordsSetYaw(camera, yaw + cam->params[0]);
}

void FieldRailCamera_FollowHeadingAtCentre(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailCamera *cam = GetRailCameraByCamHandler(unit);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx16 dir;
    VecFx32 target;
    s32 yaw;

    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ClearBind(FieldRailSystem_GetFieldCamera(rail));
    RailUnit_GetStepVector(unit, &dir);
    yaw = FX_Atan2Idx(-dir.x, -dir.z);
    FieldCamera_CoordsSetPitch(camera, cam->params[1]);
    FieldCamera_CoordsSetZoom(camera, cam->params[2]);
    FieldCamera_CoordsSetYaw(camera, yaw + cam->params[0]);
    RailUnit_GetCentrePos(unit, &target);
    FieldCamera_CoordsSetTarget(FieldRailSystem_GetFieldCamera(rail), &target);
}

void FieldRailCamera_ApplyLineStart(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    u16 onLine = RailUnit_IsOnLine(unit);
    RailPoint *point = GetCurrentRailLinePoint1(unit);

    if (onLine == RAIL_COMPONENT_LINE) {
        FieldRailCamera_ApplyPoint(rail, unit, point);
    }
}

void FieldRailCamera_ApplyLineEnd(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    u16 onLine = RailUnit_IsOnLine(unit);
    RailPoint *point = GetCurrentRailLinePoint2(unit);

    if (onLine == RAIL_COMPONENT_LINE) {
        FieldRailCamera_ApplyPoint(rail, unit, point);
    }
}

void FieldRailCamera_FixedAngleShift(FieldRailSystem *rail) {
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
    pitch = cam->params[0];
    zoom = cam->params[2];
    FieldCamera_CoordsSetYaw(camera, cam->params[1]);
    FieldCamera_CoordsSetPitch(camera, pitch);
    FieldCamera_CoordsSetZoom(camera, zoom);
    offset.x = cam->params[3];
    offset.y = cam->params[4];
    offset.z = cam->params[5];
    FieldCamera_CoordsSetTargetOffset(camera, &offset);
}

void FieldRailCamera_FixedAngleShiftAtFocus(FieldRailSystem *rail) {
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    VecFx32 target;

    FieldRailCamera_FixedAngleShift(rail);
    FieldCamera_ClearBind(camera);
    FieldRailSystem_GetCameraParentPos(rail, &target);
    FieldCamera_CoordsSetTarget(camera, &target);
    FieldCamera_EnableDelay(camera);
}

void FieldRailCamera_BlendAnglesShift(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    RailPoint *p0 = GetCurrentRailLinePoint1(unit);
    RailPoint *p1 = GetCurrentRailLinePoint2(unit);
    FieldCamera *camera = FieldRailSystem_GetFieldCamera(rail);
    RailCamera *c1;
    RailCamera *c0;
    s32 length;
    s32 posV;
    fx32 t;
    s32 pitch0;
    s32 yaw0;
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
    pitch0 = (u16)c0->params[0];
    yaw0 = (u16)c0->params[1];
    zoom0 = c0->params[2];
    yaw1 = (u16)c1->params[1];
    zoom1 = c1->params[2];
    diff = (u16)c1->params[0] - pitch0;
    if (diff >= 0x8000) {
        diff -= 0x10000;
    } else if (diff <= -0x8000) {
        diff += 0x10000;
    }
    {
        u16 pitch = pitch0 + (FX_Mul(diff << 12, t) >> 12);

        diff = yaw1 - yaw0;
        if (diff >= 0x8000) {
            diff -= 0x10000;
        } else if (diff <= -0x8000) {
            diff += 0x10000;
        }
        u16 yaw = yaw0 + (FX_Mul(diff << 12, t) >> 12);

        FieldCamera_CoordsSetYaw(camera, yaw);
        FieldCamera_CoordsSetPitch(camera, pitch);
    }
    FieldCamera_CoordsSetZoom(camera, zoom0 + FX_Mul(zoom1 - zoom0, t));
    offset.x = c0->params[3] + FX_Mul(c1->params[3] - c0->params[3], t);
    offset.y = c0->params[4] + FX_Mul(c1->params[4] - c0->params[4], t);
    offset.z = c0->params[5] + FX_Mul(c1->params[5] - c0->params[5], t);
    FieldCamera_CoordsSetTargetOffset(camera, &offset);
}

static BOOL RailMath_TryProjectWorldPointToSegment(RailPoint *p0, RailPoint *p1, const VecFx32 *pos, VecFx32 *out,
                                                   fx32 *along, VecFx32 *cross) {
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
    VEC_Set(out, FX_Mul(ndir.x, mult), FX_Mul(ndir.y, mult), FX_Mul(ndir.z, mult));
    VEC_Add(&p1->pos, out, out);
    if (VEC_Distance(pos, out) >= 0x100000) {
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

static void FieldRailCamera_ApplyPoint(FieldRailSystem *rail, RailUnit *unit, RailPoint *point) {
    VecFx32 target;
    RailCamera *cam;
    FieldCamera *camera;
    s32 front;

    if (GetCurrentRailLinePoint1(unit) == point) {
        front = 0;
    } else {
        front = GetCurrentLineLength(unit);
    }
    RailUnit_GetPosAtLineOffset(unit, front, &target);
    cam = GetRailCameraByPoint(unit, point);
    camera = FieldRailSystem_GetFieldCamera(rail);
    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_ClearBind(camera);
    FieldCamera_CoordsSetTarget(camera, &target);
    FieldCamera_CoordsSetPitch(camera, cam->params[0]);
    FieldCamera_CoordsSetYaw(camera, cam->params[1]);
    FieldCamera_CoordsSetZoom(camera, cam->params[2]);
}
