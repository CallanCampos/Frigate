// Streaming --record input for field play, touch menus and battles (app/main.c, replay.c, hal/input and hal/tp), in
// oracle/pbwinput's video-frame format. Owns only its newly created file and a bounded last-state/commit record.
#ifndef PORT_APP_RECORD_H
#define PORT_APP_RECORD_H
#include "tools/oracle/pbwinput.h"
#include <stdint.h>
#include <stdio.h>

// Reserve a new output exclusively before startup. Protected paths include input, the active save and other outputs;
// their .tmp save target and run directory are checked too. Existing files/symlinks are never replaced.
int port_record_create(const char *path, const char *const *protectedPaths, size_t count, const char *runDir);
// Write and flush the complete clock/seed/initial header before the game can consume a seed.
int port_record_start(const PbwInput *header);
int port_record_frame(uint64_t frame, const PbwInputState *state);
int port_record_active(void);
int port_record_close(void);
// Capture the effective HAL state after replay/window input, before ARM7 touch sampling.
PbwInputState port_record_input(void);
// Host OS entropy, never buildSeed's timing fallback. Predeclare the entire bounded list before startup.
int port_record_random_seeds(uint64_t *seeds, size_t count);

#ifdef PORT_RECORD_TEST
// Fault injection replaces only file I/O boundaries; tests still write/read the real new file and use real parser.
typedef struct PortRecordIO {
    size_t (*write)(const void *, size_t, FILE *);
    int (*flush)(FILE *);
    int (*close)(FILE *);
    int (*truncate)(FILE *, int64_t);
} PortRecordIO;
void port_record_test_io(const PortRecordIO *ops);
#endif
#endif
