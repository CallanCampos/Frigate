// The rail data's tables of the curves' conversions between rail and world positions and of the rails' camera
// functions, which a reset gives the rail data. The name is descriptive. The size sort sets the file apart from
// fldmmdl_rail.c: without these tables in that file's sort, its direction sets come out in the ROM's order when they
// are declared in DIR_LISTS's order, and with them only a scrambled order does
#include "types.h"
#include "field/field_actor.h"
#include "field/field_rail.h"
#include "nitro/mi.h"

// The curves' conversions between rail and world positions, and the rails' camera functions, by their IDs in the
// rail data. Names from swan
static RailCameraFunc RAIL_CAMERA_CURVE_FUNC_SET[20] = {
    RailCameraFuncSetStaticPYR, RailCameraFuncSetStaticPYDist, func_ov036_021b2cfc, func_ov036_021b2eb0,
    func_ov036_021b2f04,        func_ov036_021b309c,           func_ov036_021b3164, func_ov036_021b3294,
    func_ov036_021b3368,        func_ov036_021b33e4,           func_ov036_021b3450, func_ov036_021b34e4,
    func_ov036_021b3568,        func_ov036_021b3590,           func_ov036_021b2e48, func_ov036_021b2cc4,
    func_ov036_021b2e78,        func_ov036_021b35b8,           func_ov036_021b365c, func_ov036_021b3628,
};
static RailToWorldFunc RAIL_CONV_RAIL_TO_WORLD_FUNCS[3] = {
    RailMath_RailToWorldNoCurve,
    RailMath_RailToWorldSlerpXZ,
    RailMath_RailToWorldSlerpXYZ,
};
static WorldToRailFunc RAIL_CONV_WORLD_TO_RAIL_FUNCS[3] = {
    RailMath_WorldToRailNoCurve,
    RailMath_WorldToRailSlerp,
    RailMath_WorldToRailSlerp,
};

void RailDataHandle_Reset(RailDataHandle *handle) {
    MI_CpuFill8(handle, 0, sizeof(RailDataHandle));
    handle->camFuncCount = NELEMS(RAIL_CAMERA_CURVE_FUNC_SET);
    handle->railToWorldCount = NELEMS(RAIL_CONV_RAIL_TO_WORLD_FUNCS);
    handle->worldToRailCount = NELEMS(RAIL_CONV_WORLD_TO_RAIL_FUNCS);
    handle->camFuncs = RAIL_CAMERA_CURVE_FUNC_SET;
    handle->railToWorldFuncs = RAIL_CONV_RAIL_TO_WORLD_FUNCS;
    handle->worldToRailFuncs = RAIL_CONV_WORLD_TO_RAIL_FUNCS;
}
