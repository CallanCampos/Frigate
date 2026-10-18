#ifndef PORT_APP_LAUNCHER_SPEC_H
#define PORT_APP_LAUNCHER_SPEC_H

// What Frigate's test launcher opens (port/app/launcher.c, ADR 0012): the program's option
//
//   --launch APP[@COUNT][,APP[@COUNT]]...
//
// names apps of the field menu (party, pokedex, bag, card, summary), opened one after the other once a continued game
// asks for the field, each at the first pass of the game's main loop at which GFL's frame count reaches COUNT (at once
// without one). Plain C11 without the decomp's headers, so that it is tested on its own
// (port/tests/app/test_launcher_spec.c).
#include <stddef.h>
#include <stdint.h>

// How many apps one --launch can name
#define PORT_LAUNCH_MAX_ENTRIES 16

// An app the launcher can open: its name in --launch, its index in the game's table of the apps the field opens
// (FIELD_PROC_LINK_LIST, src/ov012/event_field_proclink.c; include/field/app_call.h's FIELD_APP_*), and the event flag
// the field menu needs set to offer it (src/ov036/field_menu.c's FieldMenu_GetMenuType), 0 for none
typedef struct PortLaunchApp {
    const char *name;
    int appId;
    unsigned flag;
} PortLaunchApp;

typedef struct PortLaunchEntry {
    const PortLaunchApp *app;
    // The frame count to open it at, and whether one was given (without one it opens at once)
    uint32_t count;
    int hasCount;
} PortLaunchEntry;

typedef struct PortLaunchSpec {
    PortLaunchEntry entries[PORT_LAUNCH_MAX_ENTRIES];
    size_t count;
} PortLaunchSpec;

// The app named name (length bytes of it), or NULL
const PortLaunchApp *port_launch_find_app(const char *name, size_t length);

// Reads --launch's value into spec. Returns 1, or 0 with why in error (a name that isn't an app, a count that isn't a
// number, an empty entry, too many entries, or counts that go down)
int port_launch_parse(const char *text, PortLaunchSpec *spec, char *error, size_t errorSize);

#endif // PORT_APP_LAUNCHER_SPEC_H
