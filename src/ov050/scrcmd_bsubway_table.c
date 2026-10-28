#include "types.h"
#include "field/field_script.h"
#include "field/scrcmd_bsubway.h"

// The command table of the Battle Subway's script plugin. The overlay has it after the rest of its rodata, which MWCC
// lays out by size within a file, so it is in a file of its own, linked after the commands.

const FieldScriptCommand BSUBWAY_SCRIPT_COMMANDS[] = {
    Scrcmd_Bsubway_ClearWork, Scrcmd_Bsubway_CreateWork,      Scrcmd_Bsubway_FreeWork,
    Scrcmd_Bsubway_Tool,      (FieldScriptCommand)0xFFFFFFFF,
};
