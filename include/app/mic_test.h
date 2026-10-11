#ifndef POKEBW2_APP_MIC_TEST_H
#define POKEBW2_APP_MIC_TEST_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// The microphone test, overlay 253 (mictest.c, by the ROM's string), which the title screen starts by holding Down,
// X and Y and the start menu lists: the player speaks into the microphone, and a speaker on the screen shakes and sends
// up notes. Its Back button returns to the title screen
#define OVERLAY_MIC_TEST OVERLAY_ID(253)

extern const GameProcFunctions MIC_TEST_PROC_FUNCTIONS;

#endif // POKEBW2_APP_MIC_TEST_H
