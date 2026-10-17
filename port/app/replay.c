// Input replays (port/app/replay.h)
#include "app/replay.h"
#include "hal/gfl/gfl_frame.h"
#include "hal/input/port_input.h"
#include "hal/tp/tp_host.h"
#include "tools/oracle/pbwinput.h"
#include <stdio.h>
#include <string.h>

static PbwInput sInput;
static PbwGameClock sClock;
static int sActive;

int port_replay_open(const char *path) {
    char error[256];

    if (!pbw_input_load(&sInput, path, error, sizeof(error))) {
        fprintf(stderr, "port: --input %s: %s\n", path, error);
        return 0;
    }
    if (sInput.version[0] != '\0' && strcmp(sInput.version, PORT_VERSION_NAME) != 0) {
        fprintf(stderr, "port: --input %s is for %s, and this program is %s\n", path, sInput.version,
                PORT_VERSION_NAME);
        pbw_input_free(&sInput);
        return 0;
    }
    pbw_game_clock_init(&sClock);
    sActive = 1;
    return 1;
}

int port_replay_active(void) {
    return sActive;
}

const char *port_replay_time(void) {
    return sInput.time[0] != '\0' ? sInput.time : PBW_INPUT_DEFAULT_TIME;
}

const char *port_replay_version(void) {
    return sActive && sInput.version[0] != '\0' ? sInput.version : NULL;
}

// The buttons and lid where the game reads them, and the touch where the ARM7's touch service samples it. The input
// file's button bits are NitroSDK's PAD_* values, as port_input's are
static void Apply(const PbwInputState *state) {
    port_input_set(state->buttons, state->lidClosed);
    port_tp_set_touch(state->touching, state->touchX, state->touchY);
}

void port_replay_start(void) {
    PbwInputState state = pbw_input_state(&sInput, 1);

    Apply(&state);
}

void port_replay_vblank(uint32_t videoFrame) {
    uint32_t frame = pbw_input_frame_at_vblank(&sInput, &sClock, videoFrame, port_gfl_frame_count());
    PbwInputState state = pbw_input_state(&sInput, frame);

    Apply(&state);
}

void port_replay_close(void) {
    if (sActive) {
        pbw_input_free(&sInput);
        sActive = 0;
    }
}
