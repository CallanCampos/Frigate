// The host time each frame of a run costs, by part (frame_time.h), for --frame-times (port/app/main.c): the parts'
// sums for the frame being made, a row a frame kept in memory, and at the end of the run the file, one row a frame in
// microseconds, and the summary on stderr. It reads only the host's clock (port/hal/host) and touches no part of the
// game, so the frames it times are the frames made without it. port/tools/frame_times.py reads the file, and the
// timing tests (port/cmake/frame_times.cmake) hold a route's frames to a budget through it.
#include "hal/os/frame_time.h"
#include "hal/host/host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Row {
    uint32_t frame;
    uint64_t wall;
    uint64_t parts[PORT_FRAME_TIME_PARTS];
} Row;

// The columns after frame, wall and busy, in PortFrameTimePart's order, then what no part covers
static const char *const sPartNames[PORT_FRAME_TIME_PARTS] = {
    "render3d", "geometry", "render2d", "capture", "arm7", "present", "pace",
};

int port_frame_time_on;
static char *sPath;
static Row *sRows;
static size_t sCount;
static size_t sCapacity;
static uint64_t sFrameStart;
static uint64_t sParts[PORT_FRAME_TIME_PARTS];

uint64_t port_frame_time_begin(void) {
    return port_frame_time_on ? port_host_now_ns() : 0;
}

void port_frame_time_end(PortFrameTimePart part, uint64_t begin) {
    if (port_frame_time_on && begin != 0) {
        sParts[part] += port_host_now_ns() - begin;
    }
}

int port_frame_time_open(const char *path) {
    FILE *file = fopen(path, "w");

    if (file == NULL) {
        return 0;
    }
    fclose(file);
    free(sPath);
    sPath = malloc(strlen(path) + 1);
    if (sPath == NULL) {
        fprintf(stderr, "port: out of memory for --frame-times\n");
        exit(1);
    }
    strcpy(sPath, path);
    sCount = 0;
    port_frame_time_on = 1;
    port_frame_time_start();
    return 1;
}

void port_frame_time_start(void) {
    memset(sParts, 0, sizeof(sParts));
    sFrameStart = port_host_now_ns();
}

void port_frame_time_frame(uint32_t frame) {
    uint64_t now;
    Row *row;

    if (!port_frame_time_on) {
        return;
    }
    now = port_host_now_ns();
    if (sCount == sCapacity) {
        size_t capacity = sCapacity != 0 ? sCapacity * 2 : 16384;
        Row *rows = realloc(sRows, capacity * sizeof(*rows));
        if (rows == NULL) {
            fprintf(stderr, "port: out of memory for --frame-times\n");
            exit(1);
        }
        sRows = rows;
        sCapacity = capacity;
    }
    row = &sRows[sCount++];
    row->frame = frame;
    row->wall = now - sFrameStart;
    memcpy(row->parts, sParts, sizeof(sParts));
    memset(sParts, 0, sizeof(sParts));
    sFrameStart = now;
}

static int CompareTimes(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;
    return x < y ? -1 : x > y;
}

// The value at fraction q of the sorted times, by nearest rank: the smallest that at least q of them don't exceed
static uint64_t Quantile(const uint64_t *sorted, size_t count, double q) {
    size_t rank = (size_t)(q * (double)count + 0.999999);
    return sorted[rank > 0 ? rank - 1 : 0];
}

static double Milliseconds(uint64_t ns) {
    return (double)ns / 1e6;
}

// Nanoseconds as the file's whole microseconds, rounded
static unsigned long long Microseconds(uint64_t ns) {
    return (unsigned long long)((ns + 500) / 1000);
}

static uint64_t Busy(const Row *row) {
    uint64_t pace = row->parts[PORT_FRAME_TIME_PACE];
    return row->wall > pace ? row->wall - pace : 0;
}

int port_frame_time_close(void) {
    FILE *file;
    uint64_t *busy;
    int ok;

    if (!port_frame_time_on) {
        return 1;
    }
    port_frame_time_on = 0;
    file = fopen(sPath, "w");
    if (file == NULL) {
        fprintf(stderr, "port: --frame-times %s: can't be written\n", sPath);
        return 0;
    }
    fprintf(file, "frame\twall_us\tbusy_us");
    for (int part = 0; part < PORT_FRAME_TIME_PARTS; part++) {
        fprintf(file, "\t%s_us", sPartNames[part]);
    }
    fprintf(file, "\tother_us\n");
    busy = malloc((sCount != 0 ? sCount : 1) * sizeof(*busy));
    for (size_t i = 0; i < sCount; i++) {
        const Row *row = &sRows[i];
        uint64_t covered = 0;

        fprintf(file, "%u\t%llu\t%llu", row->frame, Microseconds(row->wall), Microseconds(Busy(row)));
        for (int part = 0; part < PORT_FRAME_TIME_PARTS; part++) {
            covered += row->parts[part];
            fprintf(file, "\t%llu", Microseconds(row->parts[part]));
        }
        fprintf(file, "\t%llu\n", Microseconds(row->wall > covered ? row->wall - covered : 0));
        if (busy != NULL) {
            busy[i] = Busy(row);
        }
    }
    ok = fflush(file) == 0 && !ferror(file);
    ok = fclose(file) == 0 && ok;
    if (!ok) {
        fprintf(stderr, "port: --frame-times %s: the write failed\n", sPath);
    }
    if (busy != NULL && sCount != 0) {
        qsort(busy, sCount, sizeof(*busy), CompareTimes);
        fprintf(stderr,
                "port: frame times of %zu frames in %s: without the pacing wait, median %.2f ms, 95th percentile "
                "%.2f ms, most %.2f ms\n",
                sCount, sPath, Milliseconds(Quantile(busy, sCount, 0.5)), Milliseconds(Quantile(busy, sCount, 0.95)),
                Milliseconds(busy[sCount - 1]));
    }
    free(busy);
    free(sRows);
    sRows = NULL;
    sCount = sCapacity = 0;
    return ok;
}
