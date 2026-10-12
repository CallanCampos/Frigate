#ifndef POKEBW2_GFL_MATH_UTIL_H
#define POKEBW2_GFL_MATH_UTIL_H

#include "types.h"
#include "nitro/fx.h"

// The sine and cosine of an angle in whole degrees, 0 at 360 and over, or for any angle
fx16 func_02044304(u16 degrees);
fx16 func_02044330(u16 degrees);
fx16 func_02044360(u16 degrees);
fx16 func_02044388(u16 degrees);
// An angle in whole degrees as a 16-bit angle, 0 at 360 and over
u16 func_020443b4(u16 degrees);
// The angle, as a 16-bit angle, of the arc of a circle of the radius as long as how far the point (x3, y3) is from the
// line through the other two, negative on one side
int func_020443d8(int x1, int y1, int x2, int y2, int x3, int y3, int radius);

#endif // POKEBW2_GFL_MATH_UTIL_H
