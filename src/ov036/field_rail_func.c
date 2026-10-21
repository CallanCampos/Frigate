// The functions the rail data picks by ID: the curves' conversions between rail and world positions, and the rails'
// camera functions. The name is descriptive: the ROM names neither this file nor its functions' source. The size sort
// sets it apart from field_rail.c, whose .rodata ends with the default camera and curve: with the two local up vectors
// of RailMath_RailToWorldNoCurve and RailMath_RailToWorldSlerpXYZ in field_rail.c's sort, its first up vector can only
// come before both defaults as a shared const, and with them here, as a local initializer like the others.
// func_ov036_021b21dc, which only field_rail.c's CalculateRailCurves calls, could open this file as well
// RailMath_SlerpDirections and RailMath_TryProjectWorldPointToSegment are our descriptive names.
#include "types.h"
#include "field/field_camera.h"
#include "field/field_rail.h"
#include "gfl/std.h"
#include "nitro/fx.h"

static void RailMath_SlerpDirections(VecFx32 *out, const VecFx32 *a, const VecFx32 *b, fx32 t);
static void InterpolateRailPointsGlobal(VecFx32 *out, const VecFx32 *a, const VecFx32 *b, fx32 t);
static void func_ov036_021b3194(FieldRailSystem *rail, u16 angle, s32 dist, VecFx32 *target);
static BOOL RailMath_TryProjectWorldPointToSegment(RailPoint *p0, RailPoint *p1, const VecFx32 *pos, VecFx32 *out,
                                                   fx32 *along, VecFx32 *cross);
static void func_ov036_021b39c8(FieldRailSystem *rail, RailUnit *unit, RailPoint *point);

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
    s32 posV;
    s32 length;
    fx32 t;
    s32 posH;
    s32 width;
    fx32 offset;

    GetCurrentLine(unit);
    point0 = GetCurrentRailLinePoint1(unit);
    point1 = GetCurrentRailLinePoint2(unit);
    pos0 = &point0->pos;
    pos1 = &point1->pos;
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
        outPos->railDirection = 3;
        if (cross.y >= 0) {
            outPos->posSide = func_ov036_021b0d2c(front);
        } else {
            outPos->posSide = -func_ov036_021b0d2c(front);
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
    outPos->railDirection = 3;
    outPos->componentId = lineId;
    outPos->posSide = 0;
    outPos->posFront = 0;
    unit = func_ov036_021b0c00(rail);
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
            width = func_ov036_021b0cb8(unit);
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
                        outPos->posSide = func_ov036_021b0d2c(front);
                    } else {
                        outPos->posSide = -func_ov036_021b0d2c(front);
                    }
                    func_ov036_021b0c04(rail, outPos, &check);
                    func_ov036_021b06ec(rail, outPos, outWorld);
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
    pitch0 = (u16)c0->unk4;
    yaw0 = (u16)c0->unk8;
    zoom0 = c0->unkC;
    yaw1 = (u16)c1->unk8;
    zoom1 = c1->unkC;
    diff = (u16)c1->unk4 - pitch0;
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
    FieldCamera_ClearBind(FieldRailSystem_GetFieldCamera(rail));
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

    if (onLine == RAIL_COMPONENT_LINE) {
        func_ov036_021b39c8(rail, unit, point);
    }
}

void func_ov036_021b3590(FieldRailSystem *rail) {
    RailUnit *unit = FieldRailSystem_GetCameraParentUnit(rail);
    u16 onLine = RailUnit_IsOnLine(unit);
    RailPoint *point = GetCurrentRailLinePoint2(unit);

    if (onLine == RAIL_COMPONENT_LINE) {
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
    pitch0 = (u16)c0->unk4;
    yaw0 = (u16)c0->unk8;
    zoom0 = c0->unkC;
    yaw1 = (u16)c1->unk8;
    zoom1 = c1->unkC;
    diff = (u16)c1->unk4 - pitch0;
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
    offset.x = c0->unk10 + FX_Mul(c1->unk10 - c0->unk10, t);
    offset.y = c0->unk14 + FX_Mul(c1->unk14 - c0->unk14, t);
    offset.z = c0->unk18 + FX_Mul(c1->unk18 - c0->unk18, t);
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
