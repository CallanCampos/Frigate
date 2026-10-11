#ifndef POKEBW2_APP_CDEMO_SCRIPT_H
#define POKEBW2_APP_CDEMO_SCRIPT_H

// The demo script half of the opening demo: the ROM names cdemo_main.c only; this file's name is a guess, split off
// where cdemo_main.c's .rodata, sorted by size, starts again

#include "types.h"
#include "app/cdemo_main.h"

// The state machine and the script of the demos (cdemo_script.c, overlay 264). The names are ours

// The commands of a demo's script, and their length in words with the command. A handler returns 0 to run again next
// frame, 1 to go on to the next command now and 2 to go on after this frame
enum {
    // frames
    CDEMO_CMD_WAIT,
    // division, sync: the screens fade in from black, and from white, then fade out to black and to white
    CDEMO_CMD_FADE_IN_BLACK,
    CDEMO_CMD_FADE_OUT_BLACK,
    CDEMO_CMD_FADE_IN_WHITE,
    CDEMO_CMD_FADE_OUT_WHITE,
    // Waits for the fade to end
    CDEMO_CMD_WAIT_WIPE,
    // palette, file: loads a BG's characters and palette
    CDEMO_CMD_LOAD_GRAPHICS,
    // bg, enabled
    CDEMO_CMD_SET_BG_ENABLED,
    // bg, priority
    CDEMO_CMD_SET_BG_PRIORITY,
    // layer, a, b
    CDEMO_CMD_SET_LAYER_PARAMS,
    // a, b: waits in the idle state
    CDEMO_CMD_IDLE,
    CDEMO_CMD_RESET_LAYERS,
    CDEMO_CMD_NOP_12,
    CDEMO_CMD_FUNC_13,
    // bgm: plays a BGM
    CDEMO_CMD_PLAY_BGM,
    CDEMO_CMD_NOP_15,
    CDEMO_CMD_NOP_16,
    CDEMO_CMD_NOP_17,
    // Starts the animation, which plays one file of the archive for each step
    CDEMO_CMD_START_ANIMATION,
    CDEMO_CMD_NOP_19,
    CDEMO_CMD_END,
    CDEMO_CMD_COUNT,
};

// Runs a frame of the demo, which returns FALSE once it ends
BOOL CDemo_Update(CDemo *demo);

#endif // POKEBW2_APP_CDEMO_SCRIPT_H
