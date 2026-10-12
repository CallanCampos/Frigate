#ifndef POKEBW2_APP_DEMO3D_DEMO3D_EXP_H
#define POKEBW2_APP_DEMO3D_DEMO3D_EXP_H

// A 3D demo's own additions (overlay 293). Only the Royal Unova's cruise has any: the ship's place on a map on the
// lower screen ("demo3d_exp_c_cruiser.c")

#include "types.h"
#include "app/demo3d/demo3d_engine.h"
#include "app/demo3d/demo3d_graphic.h"
#include "gfl/heap.h"

typedef struct Demo3DExp Demo3DExp;

typedef struct {
    void (*init)(Demo3DExp *exp);
    void (*main)(Demo3DExp *exp);
    void (*exit)(Demo3DExp *exp);
} Demo3DExpFuncs;

struct Demo3DExp {
    HeapID heapId;
    Demo3DGraphic *graphic;
    Demo3DEngine *engine;
    const Demo3DExpFuncs *funcs;
    void *work;
    // Whether a key ends the demo
    BOOL skippable;
};

extern const Demo3DExpFuncs gDemo3DExpCCruiserFuncs;

#endif // POKEBW2_APP_DEMO3D_DEMO3D_EXP_H
