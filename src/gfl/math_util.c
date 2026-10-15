#include "types.h"
#include "gfl/math_util.h"
#include "nitro/fx.h"

// Trigonometry in whole degrees and a point's distance from a line. The file's name is a guess, after pokeplatinum's
// math_util.c, which has the same helpers

static int func_02044404(int x1, int y1, int x2, int y2, int x3, int y3);

fx16 func_02044304(u16 degrees) {
    if (degrees >= 360) {
        return 0;
    }
    return FX_SinIdx(degrees * 0xffff / 360);
}

fx16 func_02044330(u16 degrees) {
    if (degrees >= 360) {
        return 0;
    }
    return FX_CosIdx(degrees * 0xffff / 360);
}

fx16 func_02044360(u16 degrees) {
    return FX_SinIdx(degrees % 360 * 0xffff / 360);
}

fx16 func_02044388(u16 degrees) {
    return FX_CosIdx(degrees % 360 * 0xffff / 360);
}

u16 func_020443b4(u16 degrees) {
    if (degrees >= 360) {
        return 0;
    }
    return degrees * 0xffff / 360;
}

int func_020443d8(int x1, int y1, int x2, int y2, int x3, int y3, int radius) {
    int dist = func_02044404(x1, y1, x2, y2, x3, y3);
    int circumference = FX_MulFunc((radius * 2) << FX32_SHIFT, FX32_CONST(3.14)) >> FX32_SHIFT;

    return (dist << 16) / circumference;
}

static int func_02044404(int x1, int y1, int x2, int y2, int x3, int y3) {
    VecFx32 a;
    VecFx32 b;
    VecFx32 dir;
    VecFx32 cross;
    VecFx32 origin;
    fx32 side;
    int dist;

    a.x = x1 << FX32_SHIFT;
    a.y = y1 << FX32_SHIFT;
    a.z = 0;
    b.x = x2 << FX32_SHIFT;
    b.y = y2 << FX32_SHIFT;
    b.z = 0;
    origin.x = x3 << FX32_SHIFT;
    origin.y = y3 << FX32_SHIFT;
    origin.z = 0;
    VEC_Subtract(&a, &origin, &a);
    VEC_Subtract(&b, &origin, &b);
    cross.x = 0;
    cross.y = 0;
    cross.z = FX_MulFunc(a.x, b.y) - FX_MulFunc(b.x, a.y);
    side = cross.x + cross.y + cross.z;
    dir.x = a.y;
    dir.y = a.x;
    dir.z = 0;
    VEC_Normalize(&dir, &dir);
    VEC_Subtract(&b, &a, &cross);
    dist = VEC_DotProduct(&dir, &cross) >> FX32_SHIFT;
    if (dist < 0) {
        dist = -dist;
    }
    if (side <= 0) {
        dist *= -1;
    }
    return dist;
}
