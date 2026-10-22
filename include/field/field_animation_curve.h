#ifndef POKEBW2_FIELD_FIELD_ANIMATION_CURVE_H
#define POKEBW2_FIELD_FIELD_ANIMATION_CURVE_H

// These field animations use a scaled square root. The helper name and shared header are descriptive guesses.

#include "types.h"
#include "math.h"

static inline f32 FieldAnimationCurve_Sqrt(f32 coefficient, f32 time) {
    return sqrt(coefficient * time);
}

#endif // POKEBW2_FIELD_FIELD_ANIMATION_CURVE_H
