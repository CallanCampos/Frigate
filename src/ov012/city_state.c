#include "types.h"
#include "field/zone.h"
#include "system/game_data.h"

void CityState_ApplyVersionZones(CityState *state) {
    if (state->city == 1) {
        SetAllowVersionSpecificZone(0, 1);
        SetAllowVersionSpecificZone(1, 1);
    }
    if (state->city == 0) {
        SetAllowVersionSpecificZone(0, 0);
        SetAllowVersionSpecificZone(1, 0);
    }
}

void CityState_Init(CityState *state) {
    state->initialized = 0;
    state->city = 2;
}

// Whether a warp's transition type is one of an entrance camera's
BOOL IsEntranceCameraTransition(s32 transitionType) {
    if (transitionType >= WARP_TRANSITION_CAMERA) {
        return TRUE;
    }
    return FALSE;
}
