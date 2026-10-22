#ifndef POKEBW2_SYSTEM_WIPE_SUB_H
#define POKEBW2_SYSTEM_WIPE_SUB_H

#include "types.h"
#include "system/wipe.h"

// The wipe patterns (wipe_sub.c): each runs a frame of a screen's wipe and returns TRUE when it is done. The even ones
// cover the screen and the odd ones uncover it

// The steps of a pattern, in its WipeScreen's seq: setting up, running, freeing its work (where GFL_WipeForceEnd sends
// it) and done
enum {
    WIPE_SEQ_INIT,
    WIPE_SEQ_MAIN,
    WIPE_SEQ_EXIT,
    WIPE_SEQ_DONE,
};

// Master brightness fades
BOOL WipeFunc_BrightnessOut(WipeScreen *screen);
BOOL WipeFunc_BrightnessIn(WipeScreen *screen);
// Scanline wipes
BOOL WipeFunc_LinesDownOut(WipeScreen *screen);
BOOL WipeFunc_LinesDownIn(WipeScreen *screen);
BOOL WipeFunc_LinesUpOut(WipeScreen *screen);
BOOL WipeFunc_LinesUpIn(WipeScreen *screen);
// Window rectangles
BOOL WipeFunc_ShrinkLeftOut(WipeScreen *screen);
BOOL WipeFunc_GrowRightIn(WipeScreen *screen);
// H-blank circles
BOOL WipeFunc_CircleOut(WipeScreen *screen);
BOOL WipeFunc_CircleIn(WipeScreen *screen);
// Window rectangles
BOOL WipeFunc_GrowRightOut(WipeScreen *screen);
BOOL WipeFunc_ShrinkLeftIn(WipeScreen *screen);

#endif // POKEBW2_SYSTEM_WIPE_SUB_H
