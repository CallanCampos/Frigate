#ifndef POKEBW2_GFL_MATH_UTIL_H
#define POKEBW2_GFL_MATH_UTIL_H

#include "types.h"
#include "nitro/fx.h"

// The sine and cosine of an angle in whole degrees, 0 at 360 and over, or for any angle
fx16 GFL_MathSinDeg(u16 degrees);
fx16 GFL_MathCosDeg(u16 degrees);
fx16 GFL_MathSinDegWrap(u16 degrees);
fx16 GFL_MathCosDegWrap(u16 degrees);
// An angle in whole degrees as a 16-bit angle, 0 at 360 and over
u16 GFL_MathDegToAngleIdx(u16 degrees);
// The angle, as a 16-bit angle, of the arc of a circle of the radius as long as how far the point (x3, y3) is from the
// line through the other two, negative on one side
int GFL_MathArcAngle(int x1, int y1, int x2, int y2, int x3, int y3, int radius);

#endif // POKEBW2_GFL_MATH_UTIL_H
