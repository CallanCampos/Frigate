#ifndef POKEBW2_APP_DEMO3D_DEMO3D_CMD_H
#define POKEBW2_APP_DEMO3D_DEMO3D_CMD_H

// The 3D demos' commands (overlay 293): sound, brightness, lights, messages and pictures. The ROM has no name for
// this file; demo3d_cmd.c is a guess

#include "types.h"
#include "app/demo3d/demo3d_cmdsys.h"
#include "app/demo3d/demo3d_data.h"
#include "app/demo3d/demo3d_engine.h"

typedef void (*Demo3DCmdFunc)(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);

extern Demo3DCmdFunc gDemo3DCmdFuncs[DEMO3D_CMD_COUNT + 1];

#endif // POKEBW2_APP_DEMO3D_DEMO3D_CMD_H
