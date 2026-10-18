// Frigate's test launcher (launcher.c, ADR 0012): with --launch, a continued game opens the field menu's apps (the
// party list, the Pokédex, the bag, the trainer card, the summary) in place of the field while the field's code
// (overlay 36) has no C, for the tests that hold those apps to melonDS. Off without the option. This header is what the
// program (port/app/main.c) asks of it: the option, when to stop, and the run's exit status. Plain C types only, so
// that main.c, which doesn't include the decomp's headers, can use it; the game's own seam is declared in
// port/include/port_host.h.
#ifndef PORT_APP_LAUNCHER_H
#define PORT_APP_LAUNCHER_H

// The exit statuses a run with --launch ends with as far as the launcher goes. The run's own status comes first when it
// has one (port/app/main.c's list): a trap's 70, a deadlock's 72, the watchdog's 73, or a 1 of its own (the window
// didn't open, a write to the run directory or the sound trace failed during the run, a module ran out of host memory
// or found a build that can't work), each said on its own "port:" line. A 1 or 2 is the launcher's when a
// "port: --launch" line says so
// Every app opened and closed, each showing the field menu again or closing it (event type 0 or 1)
#define PORT_LAUNCH_STATUS_DONE 0
// The launch didn't finish: the run ended first (--frames, the window closed), the game never asked for the field, the
// launcher stepped aside (the field's process table, FIELDMAP_PROC_FUNCS, has C), or an app wanted what only the field
// does (an app that runs in it, or event type 2, 3 or 5)
#define PORT_LAUNCH_STATUS_UNFINISHED 1
// The save's field menu doesn't offer one of the apps, so nothing was opened. A --launch list the launcher can't read
// ends the run with the same status, a bad option, before the game starts (port_launcher_configure)
#define PORT_LAUNCH_STATUS_NOT_OFFERED 2

// Takes --launch's value (NULL when the option isn't given). Returns 0, after saying why, when it isn't a valid list of
// apps: the program then exits with status 2 before the game starts
int port_launcher_configure(const char *spec);

// Whether the launcher is done and the program should stop. Asked at each frame
int port_launcher_finished(void);

// The run's status as far as the launcher goes, one of PORT_LAUNCH_STATUS_* (DONE without --launch). When the launch
// didn't finish, says what never opened or never closed. Called once, when the run ends
int port_launcher_end(void);

#endif // PORT_APP_LAUNCHER_H
