// The sound driver's trace (port/app/sound_trace.h)
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
// can already be past the run's
static uint32_t FrameOf(uint64_t cycle) {
    if (cycle <= FIRST_VBLANK_CYCLE) {
        return 1;
    }
    return (uint32_t)((cycle - FIRST_VBLANK_CYCLE - 1) / PORT_OS_CYCLES_PER_FRAME) + 2;
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
