// DS Protect (lib/dsprot: overlays 165 and 337), which the host build doesn't compile (port/hal/replaced.txt). On the
// DS it decrypts its own ARM code, checksums it and looks for emulators and flash cartridges, and a failed check is a
// pirated copy's penalty: random numbers drawn twice more every frame, a heap leak per map change, a crash leaving the
// party list. The port plays a genuine cartridge (PLAN.md §4.5), so every check passes and does what a passing check
// does on the DS. The game's own checksums of DS Protect's code, which a host's machine code would never pass, are
// skipped on hosts: by DSPROT_CHECKED_CALL and DSPROT_CHECKED_RUN (lib/dsprot/include/dsprot/dsprot.h) and by the
// PLATFORM_PC blocks of src/system/game_system.c and src/ov012/event_battle.c.
#include "types.h"
#include "dsprot/dsprot.h"
#include <stdio.h>
#include <stdlib.h>

// Overlay 337's pair of callbacks, which the game sets before each check in an order picked by the VBlank count, and
// the index of the one a passing check calls (dsprot_run_multi.c)
u32 data_ov337_02182440;
DSProtCallback data_ov337_02182444[2];

// Decrypts overlay 337's code on the DS; a host has nothing to decrypt
void DSProt_Init(void) {
}

// What a passing check of overlay 337 does: calls the game's callback at data_ov337_02182440 with the arguments and
// returns what it returns (dsprot_run_multi.c's DSProt_RunChecks1 to 4, once their codes add up to a multiple of 241
// or 251)
static void *Overlay337_Pass(void *arg0, void *arg1) {
    return data_ov337_02182444[data_ov337_02182440](arg0, arg1);
}

void *DSProt_CallRunChecks1(void *arg0, void *arg1) {
    return Overlay337_Pass(arg0, arg1);
}

void *DSProt_CallRunChecks2(void *arg0, void *arg1) {
    return Overlay337_Pass(arg0, arg1);
}

void *DSProt_CallRunChecks3(void *arg0, void *arg1) {
    return Overlay337_Pass(arg0, arg1);
}

void *DSProt_CallRunChecks4(void *arg0, void *arg1) {
    return Overlay337_Pass(arg0, arg1);
}

// Overlay 165's check, which the party list calls as it closes: a passing one returns what the callback returns
// (dsprot_run.c's DSProt_RunChecks)
void *DSProt_CallRunChecks(DSProtCallback callback, void *arg0, void *arg1) {
    return callback != NULL ? callback(arg0, arg1) : NULL;
}

// What the party list calls when overlay 165's code doesn't add up: on the DS it clears the stack and stops the game.
// A genuine cartridge's code always adds up, so reaching it is the port's error
void *DSProt_CallCrash(void *arg0, void *arg1) {
    (void)arg0;
    (void)arg1;
    fprintf(stderr, "port: DS Protect's crash was called, which a genuine cartridge never does\n");
    abort();
}
