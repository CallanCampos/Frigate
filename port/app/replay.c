// Input replays (--input FILE, port/app/replay.h): at each VBlank, the buttons, lid and touch an input file holds for
// the frame go to the game in place of the window's (port/hal/input, port/hal/tp), with the file's frames counted in
// the game's own passes (GFL's frame count, port/hal/gfl/gfl_frame.h) or in VBlanks. It also gives port/app/main.c the
// file's time and its clock line, which sets the real-time clock at power-on and can pin it there (port/hal/rtc), and
// its seed lines, the random seeds a DS built, which the game takes in place of its own (port/hal/gfl/random_seed.h).
// The replay tests and the oracle's comparisons use it to bring a run to the same moments of the game as melonDS given
// the same file.
#include "app/replay.h"
#include "hal/gfl/gfl_frame.h"
#include "hal/gfl/random_seed.h"
#include "hal/input/port_input.h"
#include "hal/tp/tp_host.h"
#include "tools/oracle/pbwinput.h"
#include <stdio.h>
#include <string.h>

// Every seed a file can give fits where the platform layer keeps them
_Static_assert(PBW_INPUT_MAX_SEEDS <= PORT_RANDOM_SEED_MAX_GIVEN, "an input file's seeds must fit random_seed.c's");

// The file's input, empty without one, and the game clock, which runs either way for the run directory's game frames
// (port/app/run_dir.h); zero-initialised, as pbw_input_init and pbw_game_clock_init leave them
static PbwInput sInput;
static PbwGameClock sClock;
static uint32_t sGameFrame = 1;
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

int port_replay_clock_pinned(void) {
    return sActive && sInput.clock == PBW_INPUT_CLOCK_PINNED;
}

const char *port_replay_frames(void) {
    if (!sActive) {
        return NULL;
    }
    return sInput.frames == PBW_INPUT_VIDEO_FRAMES ? "video" : "game";
}

size_t port_replay_seeds(const uint64_t **seeds) {
    *seeds = sInput.seeds;
    return sActive ? sInput.seedCount : 0;
}

// The buttons and lid where the game reads them, and the touch where the ARM7's touch service samples it. The input
// file's button bits are NitroSDK's PAD_* values, as port_input's are
static void Apply(const PbwInputState *state) {
    port_input_set(state->buttons, state->lidClosed);
    port_tp_set_touch(state->touching, state->touchX, state->touchY);
}

void port_replay_start(void) {
    PbwInputState state = pbw_input_initial_state(&sInput);

    Apply(&state);
}

void port_replay_vblank(uint32_t videoFrame) {
    uint32_t frame = pbw_input_frame_at_vblank(&sInput, &sClock, videoFrame, port_gfl_frame_count());
    PbwInputState state;

    sGameFrame = sClock.frames + 1;
    if (sActive) {
        state = pbw_input_state(&sInput, frame);
        Apply(&state);
    }
}

uint32_t port_replay_game_frame(void) {
    return sGameFrame;
}

void port_replay_close(void) {
    if (sActive) {
        pbw_input_free(&sInput);
        sActive = 0;
    }
}
