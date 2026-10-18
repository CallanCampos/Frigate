#ifndef PORT_APP_LAUNCHER_H
#define PORT_APP_LAUNCHER_H

// Frigate's test launcher (launcher.c, ADR 0012): with --launch, a continued game opens the field menu's apps in place
// of the field while the field's code (overlay 36) has no C. Off without the option. Plain C types only, so that
// main.c, which doesn't include the decomp's headers, can use it; the game's own seam is declared in
// port/include/port_host.h.

// Takes --launch's value (NULL when the option isn't given). Returns 0, after saying why, when it isn't a valid list of
// apps: the program then exits with status 2 before the game starts
int port_launcher_configure(const char *spec);

// Whether the launcher is done and the program should stop, and with what exit status: 0 when every app it was asked
// for opened and closed, 2 when the save can't open one (the field menu wouldn't offer it). Asked at each frame
int port_launcher_finished(int *status);

// Says so when --launch was given but the game never asked for the field, so nothing was opened. Called at exit
void port_launcher_report_unused(void);

#endif // PORT_APP_LAUNCHER_H
