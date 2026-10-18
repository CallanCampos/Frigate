// The sound driver's trace (--sound-trace FILE, port/app/sound_trace.h): after each run of the host ARM7 sound driver's
// thread (port/hal/snd7), the record that port_snd7_trace_record fills with the driver's work in the DS's layout,
// stamped here with the video frame the run fell in and GFL's frame count (port/hal/gfl/gfl_frame.h), goes to the file
// in the format that pbw-oracle's --sound-trace writes for the DS's (port/tools/oracle/sndtrace.h). It reads the state
// of the game's music, sound effects and cries and changes nothing; port/tools/oracle/sound.py compares the two
// programs' traces (port/tools/oracle/README.md, "Sound").
#include "app/sound_trace.h"
#include "hal/gfl/gfl_frame.h"
#include "hal/os/port_os.h"
#include "hal/snd7/snd7_output.h"
#include "tools/oracle/sndtrace.h"
#include <stdio.h>
#include <string.h>

// The first VBlank's cycle (line 192 of frame 1): frame N ends with the N-th
#define FIRST_VBLANK_CYCLE ((uint64_t)PORT_OS_CYCLES_PER_LINE * 192u)

static FILE *sFile;
static const char *sPath;
static int sFailed;
static uint8_t sRecord[PBW_SNDTRACE_RECORD_SIZE];

// The frame a cycle falls in, from 1: the driver runs lazily, up to the virtual clock, so the OS's count of VBlanks
// can already be past the run's. A run at a VBlank's cycle comes after it: the OS core raises the VBlank before
// anything else at its cycle runs (the ARM7's catch-up in the frame hook, the ARM9's pass), as the oracle's frame
// moves on at its VBlank
static uint32_t FrameOf(uint64_t cycle) {
    if (cycle < FIRST_VBLANK_CYCLE) {
        return 1;
    }
    return (uint32_t)((cycle - FIRST_VBLANK_CYCLE) / PORT_OS_CYCLES_PER_FRAME) + 2;
}

static void Record(int periodic, void *user) {
    uint64_t cycle;

    (void)user;
    if (sFile == NULL || sFailed) {
        return;
    }
    port_snd7_trace_record(sRecord, periodic);
    cycle = (uint64_t)sRecord[PBW_SNDTRACE_CYCLE] | (uint64_t)sRecord[PBW_SNDTRACE_CYCLE + 1] << 8 |
            (uint64_t)sRecord[PBW_SNDTRACE_CYCLE + 2] << 16 | (uint64_t)sRecord[PBW_SNDTRACE_CYCLE + 3] << 24 |
            (uint64_t)sRecord[PBW_SNDTRACE_CYCLE + 4] << 32 | (uint64_t)sRecord[PBW_SNDTRACE_CYCLE + 5] << 40 |
            (uint64_t)sRecord[PBW_SNDTRACE_CYCLE + 6] << 48 | (uint64_t)sRecord[PBW_SNDTRACE_CYCLE + 7] << 56;
    pbw_sndtrace_put32(sRecord + PBW_SNDTRACE_FRAME, FrameOf(cycle));
    pbw_sndtrace_put32(sRecord + PBW_SNDTRACE_COUNTER, port_gfl_frame_count());
    if (fwrite(sRecord, 1, sizeof(sRecord), sFile) != sizeof(sRecord)) {
        fprintf(stderr, "port: --sound-trace: writing %s failed; the rest isn't written\n", sPath);
        sFailed = 1;
    }
}

int port_sound_trace_open(const char *path) {
    sFile = fopen(path, "wb");
    if (sFile == NULL || fwrite(PBW_SNDTRACE_MAGIC, 1, PBW_SNDTRACE_MAGIC_SIZE, sFile) != PBW_SNDTRACE_MAGIC_SIZE) {
        fprintf(stderr, "port: --sound-trace: can't write %s\n", path);
        if (sFile != NULL) {
            fclose(sFile);
            sFile = NULL;
        }
        return 0;
    }
    sPath = path;
    sFailed = 0;
    port_snd7_set_trace(Record, NULL);
    return 1;
}

int port_sound_trace_close(void) {
    int ok;

    if (sFile == NULL) {
        return 1;
    }
    port_snd7_set_trace(NULL, NULL);
    ok = fclose(sFile) == 0 && !sFailed;
    sFile = NULL;
    return ok;
}
